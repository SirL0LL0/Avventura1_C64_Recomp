/* Decodifica di file .tap del C64 (Kernal ROM + Turbo Tape 64) e ricostruzione
 * dell'immagine di memoria di "Avventura 1" dopo il boot.
 * Porting JS di tools/tap.py + tools/make_image.py: gira nel browser sulla copia dell'utente.
 */
(function (AV) {
  'use strict';

  function readPulses(u8) {
    const sig = String.fromCharCode.apply(null, u8.subarray(0, 12));
    if (sig !== 'C64-TAPE-RAW') throw new Error('Il file non è un .tap del Commodore 64');
    const ver = u8[12];
    const p = [];
    for (let i = 20; i < u8.length;) {
      const b = u8[i];
      if (b === 0) {
        if (ver === 0) { p.push(0x100); i += 1; }
        else { p.push(((u8[i + 1] | u8[i + 2] << 8 | u8[i + 3] << 16) / 8) | 0); i += 4; }
      } else { p.push(b); i += 1; }
    }
    return p;
  }

  function kernalBlocks(p) {
    const cls = v => v < 0x37 ? 1 : v < 0x4C ? 2 : v < 0x70 ? 3 : 0;   // S M L X
    const seq = new Uint8Array(p.length);
    for (let i = 0; i < p.length; i++) seq[i] = cls(p[i]);
    const blocks = [];
    let cur = [];
    let j = 0;
    while (j < seq.length - 20) {
      if (seq[j] === 3 && seq[j + 1] === 2) {
        let val = 0, ok = true, par = 1;
        for (let k = 0; k < 9; k++) {
          const a = seq[j + 2 + 2 * k], b = seq[j + 3 + 2 * k];
          let bit;
          if (a === 1 && b === 2) bit = 0; else if (a === 2 && b === 1) bit = 1; else { ok = false; break; }
          if (k < 8) { val |= bit << k; par ^= bit; } else if (bit !== par) ok = false;
        }
        if (ok) { cur.push(val); j += 20; continue; }
      }
      if (cur.length > 8) blocks.push(cur);
      cur = [];
      j++;
    }
    if (cur.length > 8) blocks.push(cur);
    return blocks;
  }

  function kernalFiles(blocks) {
    const out = [];
    let hdr = null;
    for (const raw of blocks) {
      if (raw[0] < 0x80) continue;                 // seconda copia
      const payload = raw.slice(9, raw.length - 1);
      let x = 0;
      for (const b of payload) x ^= b;
      if (x !== raw[raw.length - 1]) throw new Error('Checksum errato in un blocco Kernal del nastro');
      if ((payload[0] === 1 || payload[0] === 3) && payload.length === 192) { hdr = payload; continue; }
      if (hdr) {
        const start = hdr[1] | hdr[2] << 8, end = hdr[3] | hdr[4] << 8;
        out.push({ hdr, start, end, data: payload.slice(0, end - start) });
        hdr = null;
      }
    }
    return out;
  }

  function turboFiles(p) {
    const L = p.length;
    const isT = v => v >= 0x10 && v <= 0x2E;
    const rb = n => {
      let v = 0;
      for (let k = 0; k < 8; k++) {
        if (n + k >= L || !isT(p[n + k])) return -1;
        v = (v << 1) | (p[n + k] > 0x21 ? 1 : 0);
      }
      return v;
    };
    const findSync = n => {
      let sr = 0;
      while (n < L) {
        const v = p[n++];
        if (!isT(v)) { sr = 0; continue; }
        sr = ((sr << 1) | (v > 0x21 ? 1 : 0)) & 0xFF;
        if (sr === 9) {
          let m = n, ok = true;
          for (let want = 8; want >= 1; want--) { if (rb(m) !== want) { ok = false; break; } m += 8; }
          if (ok) return m;
        }
      }
      return -1;
    };
    const files = [];
    let n = 0;
    for (;;) {
      n = findSync(n);
      if (n < 0) break;
      const typ = rb(n);
      if (typ !== 1 && typ !== 2) { n += 8; continue; }
      let m = n + 8;
      const h = [];
      for (let i = 0; i < 4; i++) { h.push(rb(m)); m += 8; }
      const start = h[0] | h[1] << 8, end = h[2] | h[3] << 8;
      n = findSync(m);
      if (n < 0 || rb(n) !== 0) break;
      n += 8;
      const body = new Uint8Array(end - start);
      let x = 0;
      for (let i = 0; i < body.length; i++) { const v = rb(n); body[i] = v; x ^= v; n += 8; }
      const chk = rb(n); n += 8;
      if (chk !== x) throw new Error('Checksum errato nel blocco turbo del nastro');
      files.push({ start, end, data: body });
    }
    return files;
  }

  /** .tap (Uint8Array) -> { image: Uint8Array (memoria $0000-$FFFF), entry, base, end } */
  function loadGameFromTap(u8) {
    const p = readPulses(u8);
    const kf = kernalFiles(kernalBlocks(p));
    const tf = turboFiles(p);
    const main = tf.find(f => f.start === 0x0801);
    if (!main) throw new Error('Programma principale non trovato: non sembra il nastro di Avventura 1');
    const mem = new Uint8Array(0x10000);
    mem.set(main.data, 0x0801);
    for (let a = 0x0803; a < 0x0801 + main.data.length; a++) mem[a] = (mem[a] + 1) & 0xFF;   // de-offuscamento
    const tiny = kf.find(f => f.start === 0x0801);
    if (tiny) mem.set(tiny.data, 0x0801);
    const last = kf[kf.length - 1].hdr;
    const entry = last[21] | last[22] << 8;
    return { mem, entry, base: 0x0801, end: 0x0801 + main.data.length };
  }

  AV.tap = { readPulses, kernalBlocks, kernalFiles, turboFiles, loadGameFromTap };
})(globalThis.AV = globalThis.AV || {});
