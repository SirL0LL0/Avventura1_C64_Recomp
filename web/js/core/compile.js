/* Compilatore AST BASIC -> P-code, compatibile con il runtime originale.
 * Riproduce le scelte del compilatore originale (verificato byte per byte su Avventura 1),
 * cosi' un programma modificato puo' girare su un C64 vero con il runtime della copia dell'utente.
 */
(function (AV) {
  'use strict';
  const { nameBytes, LAYOUT, FUNCS } = AV.pcode;

  const BIN = { '>': 1, '=': 2, '>=': 3, '<': 4, '<>': 5, '<=': 6, '+': 7, '-': 8, '*': 9, '/': 10, '^': 11, 'AND': 12, 'OR': 13 };
  const FN = {};
  FUNCS.forEach((f, i) => { FN[f] = 0x20 + i; });

  class CompileError extends Error {
    constructor(msg, line) { super((line !== undefined ? `riga ${line}: ` : '') + msg); this.line = line; }
  }

  /** double -> float Commodore a 5 byte */
  function toCbmFloat(v) {
    if (v === 0) return [0, 0, 0, 0, 0];
    const sign = v < 0 ? 1 : 0;
    let a = Math.abs(v);
    let e = Math.floor(Math.log2(a)) + 1;
    let m = a / Math.pow(2, e);
    while (m >= 1) { m /= 2; e++; }
    while (m < 0.5) { m *= 2; e--; }
    let mant = Math.round(m * 4294967296);
    if (mant >= 4294967296) { mant /= 2; e++; }
    if (e + 128 > 255) throw new CompileError('numero troppo grande');
    if (e + 128 <= 0) return [0, 0, 0, 0, 0];
    const b1 = Math.floor(mant / 16777216) & 0x7F | (sign << 7);
    return [e + 128, b1, Math.floor(mant / 65536) & 255, Math.floor(mant / 256) & 255, mant & 255];
  }

  function compile(prog, runtime) {
    const code = [];                 // P-code principale (byte), indirizzi relativi
    const fixAbs = [];               // { at, line } salti assoluti da risolvere
    const fixRel = [];               // { at, from, line:'next' } IF -> riga successiva
    const lineOff = new Map();
    const vars = new Map();          // nome -> indice
    const arrays = new Map();        // nome -> { n, nd, explicit }
    let curLine;
    const err = m => { throw new CompileError(m, curLine); };
    const b = (...xs) => { for (const x of xs) code.push(x & 0xFF); };

    const varIndex = name => {
      if (!vars.has(name)) vars.set(name, vars.size);
      return vars.get(name);
    };
    const arrInfo = (name, nd) => {
      if (!arrays.has(name)) arrays.set(name, { n: LAYOUT.ARRAY_BASE + arrays.size, nd, explicit: false });
      const a = arrays.get(name);
      if (a.nd !== nd) err(`numero di indici errato per ${name}`);
      return a;
    };
    const varRef = (name, store) => {
      const i = varIndex(name), s = store ? 0x40 : 0;
      if (i < 32) b(0x80 | s | i);
      else if (i < 768) b(0xA0 | s | (i >> 8), i & 255);
      else b(0xA3 | s, i >> 8, i & 255);
    };
    const arrRef = (a, store) => {
      const s = store ? 0x40 : 0;
      if (a.n < 256) b(0xA4 | s, a.n); else b(0xA5 | s, a.n >> 8, a.n & 255);
    };
    const num = e => {
      const v = e.v;
      if (!e.flt && Number.isInteger(v) && v >= 0 && v <= 32767) {
        if (v < 16) return b(0xB0 + v);
        if (v < 32) return b(0xF0 + v - 16);
        if (v < 256) return b(0xA6, v);
        return b(0xA7, v >> 8, v & 255);
      }
      b(0xA8, ...toCbmFloat(v));
    };
    const str = bytes => {
      if (bytes.length > 255) err('stringa troppo lunga');
      if (bytes.length <= 7) b(0xE8 + bytes.length, ...bytes);
      else b(0xE7, bytes.length, ...bytes);
    };

    function expr(e) {
      switch (e.k) {
        case 'num': return num(e);
        case 'str': return str(e.b);
        case 'var': return varRef(e.name, false);
        case 'arr': { e.idx.forEach(expr); return arrRef(arrInfo(e.name, e.idx.length), false); }
        case 'sys': return b({ PI: 0xAA, ST: 0xAB, TI: 0xAC, 'TI$': 0xAE }[e.name]);
        case 'bin': expr(e.l); expr(e.r); return b(BIN[e.op]);
        case 'un':
          if (e.op === '-' && e.e.k === 'num') return num({ k: 'num', v: -e.e.v, flt: true });
          expr(e.e); return b(e.op === '-' ? 0x0E : 0x0F);
        case 'fn': {
          const want = { 'LEFT$': 2, 'RIGHT$': 2, 'MID$': 3 }[e.name] || 1;
          let args = e.args;
          if (e.name === 'MID$' && args.length === 2) args = args.concat([{ k: 'num', v: 255 }]);
          if (args.length !== want) err(`numero di argomenti errato per ${e.name}`);
          args.forEach(expr); return b(FN[e.name]);
        }
      }
      err('espressione non supportata');
    }
    const target = (t, store) => {
      if (t.k === 'var') return varRef(t.name, store);
      if (t.k === 'arr') { t.idx.forEach(expr); return arrRef(arrInfo(t.name, t.idx.length), store); }
      err('destinazione non valida');
    };
    const jump = (op, line) => { b(op); fixAbs.push({ at: code.length, line }); b(0, 0); };

    function printStmt(s) {
      const file = !!s.chan;
      if (file) {
        expr(s.chan);
        if (!s.items.length) return b(0x44);
        b(0x42);
      } else if (!s.items.length) return b(0x3F);
      s.items.forEach((it, k) => {
        const last = k === s.items.length - 1;
        if (it.comma) { b(0x3B); if (last && s.cr) b(0x3F); return; }
        expr(it.e);
        if (it.fn) { b(it.fn === 'TAB(' ? 0x41 : 0x40); if (last && s.cr) b(0x3F); return; }
        if (last && s.cr) return b(file ? 0x43 : 0x3E);
        b(it.sep === ',' ? 0x3D : 0x3C);
      });
      if (file && !s.cr) b(0x45);
    }

    function stmt(s) {
      switch (s.t) {
        case 'let':
          if (s.target.k === 'arr') {             // indici, poi valore, poi scrittura
            s.target.idx.forEach(expr); expr(s.e);
            return arrRef(arrInfo(s.target.name, s.target.idx.length), true);
          }
          expr(s.e); return target(s.target, true);
        case 'print': return printStmt(s);
        case 'if': {
          expr(s.cond); b(0x1F);
          fixRel.push({ at: code.length, from: code.length - 1 }); b(0);
          s.then.forEach(stmt);
          return;
        }
        case 'goto': return jump(0x19, s.line);
        case 'gosub': return jump(0x1A, s.line);
        case 'on': {
          expr(s.e);
          b(s.kind === 'GOTO' ? 0x1B : 0x1C, 2 + 2 * s.lines.length);
          for (const l of s.lines) { fixAbs.push({ at: code.length, line: l }); b(0, 0); }
          return;
        }
        case 'for':
          expr(s.from); varRef(s.v.name, true);
          expr(s.to); if (s.step) expr(s.step);
          varRef(s.v.name, false); return b(s.step ? 0x12 : 0x11);
        case 'next':
          if (!s.vars.length) return b(0x13);
          for (const v of s.vars) { target(v, false); b(0x14); }
          return;
        case 'return': return b(0x1D);
        case 'end': return b(0x4F);
        case 'stop': return b(0x49);
        case 'clr': return b(0x15);
        case 'new': return b(0x4E);
        case 'restore': return b(0x1E);
        case 'poke': expr(s.val); expr(s.addr); return b(0x17);
        case 'sys': expr(s.addr); return b(0x18, 0x3A);
        case 'wait':
          [...s.args].reverse().forEach(expr); return b(s.args.length === 3 ? 0x4D : 0x4C);
        case 'get':
          if (s.chan) { expr(s.chan); b(0x48); }
          s.vars.forEach((v, k) => { target(v, false); b(k === s.vars.length - 1 ? 0x47 : 0x46); });
          return;
        case 'input': {
          if (!s.chan) err('INPUT da tastiera non ancora supportato dal compilatore');
          expr(s.chan); b(0x48);
          const n = s.vars.length;
          s.vars.forEach((v, k) => {
            target(v, false);
            const first = k === 0, last = k === n - 1;
            b(first && last ? 0x53 : first ? 0x55 : last ? 0x51 : 0x57);
          });
          return;
        }
        case 'read':
          for (const v of s.vars) { target(v, false); b(v.name.endsWith('$') ? 0x4A : 0x4B); }
          return;
        case 'dim':
          for (const a of s.arrays) {
            a.dims.forEach(expr);
            const info = arrInfo(a.name, a.dims.length);
            info.explicit = true;
            const slot = 2 * info.n + 1;
            b(0x10, a.dims.length, ...nameBytes(a.name), slot >> 8, slot & 255);
          }
          return;
        case 'open': case 'load': case 'save': case 'verify':
          [...s.args].reverse().forEach(expr);
          return b({ open: 0x60, load: 0x5D, save: 0x5E, verify: 0x5F }[s.t], s.args.length);
        case 'close': expr(s.args[0]); return b(0x61);
        case 'rem': return;
      }
      err('istruzione non supportata dal compilatore: ' + s.t);
    }

    // --- codice principale
    b(0x15);                                         // CLR iniziale
    const lines = prog.lines;
    lines.forEach((l, k) => {
      curLine = l.n;
      lineOff.set(l.n, code.length);
      const fixStart = fixRel.length;
      l.stmts.forEach(stmt);
      const next = code.length;
      for (let i = fixStart; i < fixRel.length; i++) {
        const f = fixRel[i];
        const d = next - f.from;
        if (d > 255) err('riga troppo lunga dopo un IF (salto oltre 255 byte)');
        code[f.at] = d;
      }
    });
    curLine = undefined;
    b(0x4F, 0x00);                                   // END + terminatore

    // --- init: array fittizio che riserva gli slot + DIM impliciti
    const init = [];
    const realArrays = [...arrays.entries()];
    if (realArrays.length) {
      const k = realArrays.length - 1;
      init.push(...encodeInt(k), 0x10, 0x01, 0xDA, 0xAA, 0x00, 0x00);
      for (const [name, a] of realArrays) {
        if (a.explicit) continue;
        const slot = 2 * a.n + 1;
        for (let d = 0; d < a.nd; d++) init.push(0xBA);          // dimensione implicita 10
        init.push(0x10, a.nd, ...nameBytes(name), slot >> 8, slot & 255);
      }
    }
    init.push(0x16);

    // --- DATA
    const data = [];
    for (const d of prog.data || []) { if (d.length > 127) throw new CompileError('DATA troppo lunga'); data.push(d.length, ...d); }
    data.push(0xFF);

    // --- layout
    const initPC = LAYOUT.INIT;
    const datPtr = initPC + init.length;
    const startPC = datPtr + data.length;
    const vartab = startPC + code.length;
    const arytab = vartab + 7 * vars.size;
    if (arytab > 0x9F00) throw new CompileError('programma troppo grande per la memoria del C64');
    for (const f of fixAbs) {
      if (!lineOff.has(f.line)) throw new CompileError(`riga ${f.line} inesistente`);
      const a = startPC + lineOff.get(f.line);
      code[f.at] = a >> 8; code[f.at + 1] = a & 255;
    }
    const mem = new Uint8Array(arytab - 0x0801);
    mem.set(runtime.subarray(0, LAYOUT.HEADER - 0x0801), 0);
    const put = (a, bytes) => mem.set(bytes, a - 0x0801);
    const w = v => [v & 255, v >> 8];
    put(LAYOUT.HEADER, [...w(vartab), ...w(arytab), ...w(arytab), ...w(initPC), ...w(datPtr), ...w(startPC)]);
    put(initPC, init);
    put(datPtr, data);
    put(startPC, code);
    let a = vartab;
    for (const name of vars.keys()) { put(a, nameBytes(name)); a += 7; }
    return { bytes: mem, start: 0x0801, vars: [...vars.keys()], sizes: { pcode: code.length, data: data.length, vars: vars.size } };
  }

  function encodeInt(v) {
    if (v < 16) return [0xB0 + v];
    if (v < 32) return [0xF0 + v - 16];
    if (v < 256) return [0xA6, v];
    return [0xA7, v >> 8, v & 255];
  }

  /** PRG eseguibile su C64: stub "0 SYS2881" + immagine (l'entry originale del runtime e' $0B41) */
  function toPrg(compiled) {
    const out = new Uint8Array(2 + compiled.bytes.length);
    out[0] = 0x01; out[1] = 0x08;
    out.set(compiled.bytes, 2);
    const stub = [0x0B, 0x08, 0x00, 0x00, 0x9E, 0x32, 0x38, 0x38, 0x31, 0x00, 0x00, 0x00];   // 0 SYS2881
    out.set(stub, 2);
    return out;
  }

  AV.compile = { compile, toPrg, toCbmFloat, CompileError };
})(globalThis.AV = globalThis.AV || {});
