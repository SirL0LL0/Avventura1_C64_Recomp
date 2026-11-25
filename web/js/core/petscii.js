/* PETSCII <-> testo leggibile (set di caratteri maiuscolo/minuscolo del C64).
 *
 * Convenzione del formato .bas (compatibile in spirito con petcat):
 *   - fuori dalle stringhe: parole chiave e variabili in MAIUSCOLO ASCII
 *   - dentro le stringhe: a-z = PETSCII $41-$5A, A-Z = $C1-$DA, £ = $5C,
 *     codici di controllo tra graffe: {clr} {wht} {down} {12 rght} {$a4} ...
 */
(function (AV) {
  'use strict';

  const CTRL = {
    0x03: 'stop', 0x05: 'wht', 0x08: 'lock', 0x09: 'unlock', 0x0D: 'return', 0x0E: 'lower',
    0x11: 'down', 0x12: 'rvon', 0x13: 'home', 0x14: 'del', 0x1C: 'red', 0x1D: 'rght', 0x1E: 'grn',
    0x1F: 'blu', 0x81: 'orng', 0x85: 'f1', 0x86: 'f3', 0x87: 'f5', 0x88: 'f7', 0x89: 'f2',
    0x8A: 'f4', 0x8B: 'f6', 0x8C: 'f8', 0x8D: 'sret', 0x8E: 'upper', 0x90: 'blk', 0x91: 'up',
    0x92: 'rvof', 0x93: 'clr', 0x94: 'inst', 0x95: 'brn', 0x96: 'lred', 0x97: 'gry1', 0x98: 'gry2',
    0x99: 'lgrn', 0x9A: 'lblu', 0x9B: 'gry3', 0x9C: 'pur', 0x9D: 'left', 0x9E: 'yel', 0x9F: 'cyn',
    0xA0: 'sspc',
  };
  const CTRL_REV = {};
  for (const k in CTRL) CTRL_REV[CTRL[k]] = +k;

  /** byte PETSCII -> carattere stampabile del formato testo, o null se va scritto tra graffe */
  function printable(b) {
    if (b >= 0x41 && b <= 0x5A) return String.fromCharCode(b + 32);   // a-z
    if (b >= 0xC1 && b <= 0xDA) return String.fromCharCode(b - 0x80);  // A-Z
    if (b === 0x5C) return '£';
    if (b === 0x5E) return '↑';
    if (b === 0x5F) return '←';
    if (b >= 0x20 && b <= 0x40 && b !== 0x22) return String.fromCharCode(b);
    if (b === 0x5B || b === 0x5D) return String.fromCharCode(b);
    return null;
  }

  function escName(b) {
    return CTRL[b] || ('$' + b.toString(16).padStart(2, '0'));
  }

  /** array di byte -> testo con escape (senza virgolette) */
  function bytesToText(bytes) {
    let s = '';
    for (let i = 0; i < bytes.length;) {
      const b = bytes[i];
      const p = printable(b);
      if (p !== null) { s += p; i++; continue; }
      let n = 1;
      while (i + n < bytes.length && bytes[i + n] === b) n++;
      if (n >= 3) { s += `{${n} ${escName(b)}}`; i += n; }
      else { s += `{${escName(b)}}`; i++; }
    }
    return s;
  }

  /** testo con escape -> array di byte; lancia Error se non valido */
  function textToBytes(s) {
    const out = [];
    for (let i = 0; i < s.length; i++) {
      const c = s[i];
      if (c === '{') {
        const j = s.indexOf('}', i);
        if (j < 0) throw new Error('graffa non chiusa');
        let body = s.slice(i + 1, j).trim().toLowerCase();
        let n = 1;
        const m = /^(\d+)\s+(.+)$/.exec(body);
        if (m) { n = +m[1]; body = m[2]; }
        let b;
        if (body[0] === '$') b = parseInt(body.slice(1), 16);
        else if (body in CTRL_REV) b = CTRL_REV[body];
        else throw new Error('codice sconosciuto {' + body + '}');
        if (!(b >= 0 && b <= 255)) throw new Error('codice non valido {' + body + '}');
        for (let k = 0; k < n; k++) out.push(b);
        i = j;
        continue;
      }
      const code = c.charCodeAt(0);
      if (c >= 'a' && c <= 'z') out.push(code - 32);
      else if (c >= 'A' && c <= 'Z') out.push(code + 0x80);
      else if (c === '£') out.push(0x5C);
      else if (c === '↑') out.push(0x5E);
      else if (c === '←') out.push(0x5F);
      else if (code >= 0x20 && code <= 0x40 && c !== '"') out.push(code);
      else if (c === '[' || c === ']') out.push(code);
      else throw new Error('carattere non rappresentabile: ' + JSON.stringify(c));
    }
    return out;
  }

  /** PETSCII -> codice schermo (per la visualizzazione) */
  function toScreenCode(b) {
    if (b < 0x20) return -1;
    if (b < 0x40) return b;
    if (b < 0x60) return b - 0x40;
    if (b < 0x80) return b - 0x20;
    if (b < 0xA0) return -1;
    if (b < 0xC0) return b - 0x40;
    if (b < 0xFF) return b - 0x80;
    return 0x5E;
  }

  AV.petscii = { CTRL, CTRL_REV, bytesToText, textToBytes, toScreenCode, printable };
})(globalThis.AV = globalThis.AV || {});
