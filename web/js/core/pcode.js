/* Decoder del P-code e decompilatore P-code -> AST BASIC.
 * Formato documentato in docs/PCODE.md.
 *
 * AST (usato anche da basic.js, compile.js e dal motore):
 *   Program  { lines: [{ n, stmts }], data: [[byte...]...] }
 *   Expr     { k:'num', v, flt? } | { k:'str', b:[bytes] } | { k:'var', name }
 *            { k:'arr', name, idx:[Expr] } | { k:'bin', op, l, r } | { k:'un', op, e }
 *            { k:'fn', name, args:[Expr] } | { k:'sys', name:'PI'|'ST'|'TI'|'TI$' }
 *   Stmt     vedi la funzione statementsFrom() e il README del formato in basic.js
 */
(function (AV) {
  'use strict';

  const BINOPS = { 1: '>', 2: '=', 3: '>=', 4: '<', 5: '<>', 6: '<=', 7: '+', 8: '-', 9: '*', 10: '/', 11: '^', 12: 'AND', 13: 'OR' };
  const FUNCS = ['SGN', 'INT', 'ABS', 'USR', 'FRE', 'POS', 'SQR', 'RND', 'LOG', 'EXP', 'COS', 'SIN', 'TAN', 'ATN',
    'PEEK', 'LEN', 'STR$', 'VAL', 'ASC', 'CHR$', 'LEFT$', 'RIGHT$', 'MID$'];
  const ARITY = { 'LEFT$': 2, 'RIGHT$': 2, 'MID$': 3 };

  // indirizzi fissi del layout prodotto dal compilatore originale
  const LAYOUT = { RUNTIME_START: 0x0801, RUNTIME_END: 0x1783, HEADER: 0x1784, INIT: 0x1790, ARRAY_BASE: 3 };

  function varName(b0, b1) {
    const c0 = String.fromCharCode(b0 & 0x7F), c1 = (b1 & 0x7F) ? String.fromCharCode(b1 & 0x7F) : '';
    const ok = ch => /[A-Z0-9]/.test(ch);
    if (!ok(c0) || (c1 && !ok(c1))) return null;
    const s = c0 + c1;
    if ((b0 & 0x80) && (b1 & 0x80)) return s + '%';
    if (b1 & 0x80) return s + '$';
    if (b0 & 0x80) return 'FN' + s;
    return s;
  }

  function nameBytes(name) {
    let base = name, t = '';
    if (name.endsWith('$') || name.endsWith('%')) { t = name.slice(-1); base = name.slice(0, -1); }
    base = (base + '\0').slice(0, 2);
    let b0 = base.charCodeAt(0), b1 = base.charCodeAt(1);
    if (t === '$') b1 |= 0x80;
    if (t === '%') { b0 |= 0x80; b1 |= 0x80; }
    return [b0, b1];
  }

  /** decodifica una singola istruzione all'indirizzo a */
  function decodeAt(mem, a) {
    const b = mem[a], n = i => mem[a + i];
    if (b >= 0x80) {
      const lo = b & 0x3F, store = !!(b & 0x40);
      if (b === 0xE6) return { a, len: 1, k: 'store' };
      if (b === 0xE7) { const l = n(1); return { a, len: 2 + l, k: 'str', b: Array.from(mem.subarray(a + 2, a + 2 + l)) }; }
      if (b >= 0xE8 && b <= 0xEF) { const l = b & 7; return { a, len: 1 + l, k: 'str', b: Array.from(mem.subarray(a + 1, a + 1 + l)) }; }
      if (b >= 0xF0) return { a, len: 1, k: 'int', v: 16 + (b & 15) };
      if (lo < 0x20) return { a, len: 1, k: 'var', i: lo, store };
      if (lo <= 0x22) return { a, len: 2, k: 'var', i: (lo & 3) * 256 + n(1), store };
      if (lo === 0x23) return { a, len: 3, k: 'var', i: n(1) * 256 + n(2), store };
      if (lo === 0x24) return { a, len: 2, k: 'arr', i: n(1), store };
      if (lo === 0x25) return { a, len: 3, k: 'arr', i: n(1) * 256 + n(2), store };
      if (lo === 0x26) return { a, len: 2, k: 'int', v: n(1) };
      if (lo === 0x27) return { a, len: 3, k: 'int', v: n(1) * 256 + n(2) };
      if (lo === 0x28 || lo === 0x29) return { a, len: 6, k: 'flt', bytes: [n(1), n(2), n(3), n(4), n(5)] };
      if (lo === 0x2A) return { a, len: 1, k: 'sysv', name: 'PI' };
      if (lo === 0x2B) return { a, len: 1, k: 'sysv', name: 'ST' };
      if (lo === 0x2C) return { a, len: 1, k: 'sysv', name: 'TI' };
      if (lo === 0x2E || lo === 0x2F) return { a, len: 1, k: 'sysv', name: 'TI$' };
      if (lo >= 0x30) return { a, len: 1, k: 'int', v: lo & 15 };
      throw new Error(`byte P-code non valido $${b.toString(16)} a $${a.toString(16)}`);
    }
    const r = { a, len: 1, k: 'op', op: b };
    if (b >= 1 && b <= 13) { r.k = 'bin'; r.name = BINOPS[b]; return r; }
    if (b >= 0x20 && b <= 0x36) { r.k = 'fn'; r.name = FUNCS[b - 0x20]; return r; }
    switch (b) {
      case 0x10: r.len = 6; r.nd = n(1); r.nb = [n(2), n(3)]; r.slot = n(4) * 256 + n(5); break;
      case 0x18: r.len = 2; break;                              // SYS + ':' ($3A) per il TXTPTR
      case 0x19: case 0x1A: r.len = 3; r.target = n(1) * 256 + n(2); break;
      case 0x1B: case 0x1C: {
        r.len = n(1); r.targets = [];
        for (let k = 0; k < (n(1) - 2) / 2; k++) r.targets.push(n(2 + 2 * k) * 256 + n(3 + 2 * k));
        break;
      }
      case 0x1F: r.len = 2; r.target = a + n(1); break;
      case 0x37: r.len = n(5); break;
      case 0x38: r.len = 3; break;
      case 0x5D: case 0x5E: case 0x5F: case 0x60: r.len = 2; r.np = n(1); break;
      default:
        if (b > 0x61 || (b > 0 && b < 0x0E && !(b >= 1 && b <= 13))) throw new Error(`opcode non valido $${b.toString(16)} a $${a.toString(16)}`);
    }
    return r;
  }

  function fltValue(f) {
    if (f[0] === 0) return 0;
    const sign = f[1] & 0x80 ? -1 : 1;
    const mant = (((f[1] | 0x80) * 16777216) + f[2] * 65536 + f[3] * 256 + f[4]) / 4294967296;
    return sign * mant * Math.pow(2, f[0] - 128);
  }

  /** legge le informazioni del programma compilato da un'immagine di memoria (64K) */
  function readProgram(mem) {
    const w = a => mem[a] | mem[a + 1] << 8;
    const hdr = {
      vartab: w(0x1784), arytab: w(0x1786), strend: w(0x1788),
      initPC: w(0x178A), datptr: w(0x178C), startPC: w(0x178E),
    };
    const vars = [];
    for (let a = hdr.vartab; a + 7 <= hdr.arytab; a += 7) vars.push(varName(mem[a], mem[a + 1]) || `V${vars.length}`);
    const data = [];
    let a = hdr.datptr;
    while (mem[a] < 0x80) { data.push(Array.from(mem.subarray(a + 1, a + 1 + mem[a]))); a += 1 + mem[a]; }
    const dataEnd = a;
    const decodeRange = (from, to) => {
      const out = [];
      for (let x = from; x <= to;) { const i = decodeAt(mem, x); out.push(i); x += i.len; }
      return out;
    };
    // il P-code finisce col terminatore $00 subito prima di VARTAB
    const init = decodeRange(hdr.initPC, hdr.datptr - 1);
    const main = decodeRange(hdr.startPC, hdr.vartab - 2);
    const arrays = {};
    for (const i of init.concat(main)) if (i.op === 0x10) arrays[(i.slot - 1) / 2] = { name: varName(i.nb[0], i.nb[1]), nd: i.nd };
    return { hdr, vars, arrays, data, dataEnd, init, main };
  }

  // ------------------------------------------------------------------ decompilazione
  function decompile(mem) {
    const P = readProgram(mem);
    const vn = i => P.vars[i] || `V${i}`;
    const arrInfo = i => P.arrays[i] || { name: `A${i}`, nd: 1 };
    const stmts = [];               // { a, s } istruzioni "piatte" con indirizzo d'inizio
    let stack = [];
    let stmtStart = null;
    const pop = () => { if (!stack.length) throw new Error('stack vuoto: P-code inatteso'); return stack.pop(); };
    const popN = k => { const r = []; for (let i = 0; i < k; i++) r.unshift(pop()); return r; };
    const emit = (s, a) => { stmts.push({ a: stmtStart, s }); stmtStart = null; if (stack.length) throw new Error(`stack non vuoto dopo istruzione a $${a.toString(16)}`); };
    const ref = e => { if (e.k !== 'var' && e.k !== 'arr') throw new Error('attesa variabile'); return e; };

    const ins = P.main;
    let startIdx = 0;
    if (ins[0] && ins[0].op === 0x15) startIdx = 1;          // CLR iniziale inserito dal compilatore
    let endIdx = ins.length;
    if (ins[endIdx - 1] && ins[endIdx - 1].op === 0x4F) endIdx--;   // END finale inserito dal compilatore

    for (let x = startIdx; x < endIdx; x++) {
      const i = ins[x];
      if (stmtStart === null) stmtStart = i.a;
      switch (i.k) {
        case 'int': stack.push({ k: 'num', v: i.v }); continue;
        case 'flt': { const v = fltValue(i.bytes); stack.push({ k: 'num', v, flt: true }); continue; }
        case 'str': stack.push({ k: 'str', b: i.b }); continue;
        case 'sysv': stack.push({ k: 'sys', name: i.name }); continue;
        case 'var':
          if (i.store) emit({ t: 'let', target: { k: 'var', name: vn(i.i) }, e: pop() }, i.a);
          else stack.push({ k: 'var', name: vn(i.i) });
          continue;
        case 'arr': {
          const ai = arrInfo(i.i);
          if (i.store) { const v = pop(); const idx = popN(ai.nd); emit({ t: 'let', target: { k: 'arr', name: ai.name, idx }, e: v }, i.a); }
          else stack.push({ k: 'arr', name: ai.name, idx: popN(ai.nd) });
          continue;
        }
        case 'bin': { const r = pop(), l = pop(); stack.push({ k: 'bin', op: i.name, l, r }); continue; }
        case 'fn': stack.push({ k: 'fn', name: i.name, args: popN(ARITY[i.name] || 1) }); continue;
        case 'store': throw new Error('STORE esplicito non gestito');
      }
      const op = i.op;
      switch (op) {
        case 0x0E: stack.push({ k: 'un', op: '-', e: pop() }); continue;
        case 0x0F: stack.push({ k: 'un', op: 'NOT', e: pop() }); continue;
        case 0x10: { const ai = { name: varName(i.nb[0], i.nb[1]) }; emit({ t: 'dim', arrays: [{ name: ai.name, dims: popN(i.nd) }] }, i.a); break; }
        case 0x11: case 0x12: {
          const v = ref(pop()), step = op === 0x12 ? pop() : null, to = pop();
          emit({ t: 'forhead', v, to, step }, i.a); break;
        }
        case 0x13: emit({ t: 'next', vars: [] }, i.a); break;
        case 0x14: emit({ t: 'next', vars: [ref(pop())] }, i.a); break;
        case 0x15: emit({ t: 'clr' }, i.a); break;
        case 0x17: { const addr = pop(), val = pop(); emit({ t: 'poke', addr, val }, i.a); break; }
        case 0x18: emit({ t: 'sys', addr: pop() }, i.a); break;
        case 0x19: emit({ t: 'goto', dest: i.target }, i.a); break;
        case 0x1A: emit({ t: 'gosub', dest: i.target }, i.a); break;
        case 0x1B: case 0x1C: emit({ t: 'on', e: pop(), kind: op === 0x1B ? 'GOTO' : 'GOSUB', targets: i.targets }, i.a); break;
        case 0x1D: emit({ t: 'return' }, i.a); break;
        case 0x1E: emit({ t: 'restore' }, i.a); break;
        case 0x1F: emit({ t: 'ifnot', cond: pop(), dest: i.target }, i.a); break;
        case 0x3A: emit({ t: 'nop' }, i.a); break;
        case 0x3B: emit({ t: 'pitem', it: { comma: true } }, i.a); break;
        case 0x3C: emit({ t: 'pitem', it: { e: pop(), sep: ';' } }, i.a); break;
        case 0x3D: emit({ t: 'pitem', it: { e: pop(), sep: ',' } }, i.a); break;
        case 0x3E: emit({ t: 'pitem', it: { e: pop(), sep: '' }, cr: true }, i.a); break;
        case 0x3F: emit({ t: 'pcr' }, i.a); break;
        case 0x40: emit({ t: 'pitem', it: { fn: 'SPC(', e: pop(), sep: ';' } }, i.a); break;
        case 0x41: emit({ t: 'pitem', it: { fn: 'TAB(', e: pop(), sep: ';' } }, i.a); break;
        case 0x42: emit({ t: 'pchan', chan: pop() }, i.a); break;
        case 0x43: emit({ t: 'pitem', it: { e: pop(), sep: '' }, cr: true, file: true }, i.a); break;
        case 0x44: emit({ t: 'print', chan: pop(), items: [], cr: true }, i.a); break;
        case 0x45: emit({ t: 'pend' }, i.a); break;
        case 0x46: case 0x47: emit({ t: 'get', vars: [ref(pop())], more: op === 0x46 }, i.a); break;
        case 0x48: emit({ t: 'inchan', chan: pop() }, i.a); break;
        case 0x49: emit({ t: 'stop' }, i.a); break;
        case 0x4A: case 0x4B: emit({ t: 'read', vars: [ref(pop())] }, i.a); break;
        case 0x4C: { const a2 = popN(2); emit({ t: 'wait', args: [a2[1], a2[0]] }, i.a); break; }
        case 0x4D: { const a3 = popN(3); emit({ t: 'wait', args: a3.reverse() }, i.a); break; }
        case 0x4E: emit({ t: 'new' }, i.a); break;
        case 0x4F: emit({ t: 'end' }, i.a); break;
        case 0x5D: case 0x5E: case 0x5F: case 0x60: {
          const args = popN(i.np).reverse();
          const nm = { 0x5D: 'load', 0x5E: 'save', 0x5F: 'verify', 0x60: 'open' }[op];
          emit({ t: nm, args }, i.a); break;
        }
        case 0x61: emit({ t: 'close', args: [pop()] }, i.a); break;
        default:
          if (op >= 0x51 && op <= 0x58) { emit({ t: 'initem', v: ref(pop()), op }, i.a); break; }
          throw new Error(`istruzione non supportata dal decompilatore: $${op.toString(16)} a $${i.a.toString(16)}`);
      }
    }
    if (stack.length) throw new Error('espressione incompleta alla fine del programma');

    // ---------------------------------------------------------- inizi riga
    const starts = new Set([stmts[0].a]);
    for (const { s } of stmts) {
      if (s.dest !== undefined) starts.add(s.dest);
      if (s.targets) s.targets.forEach(t => starts.add(t));
    }
    // FOR: unisce "X = inizio" con l'intestazione FOR successiva
    const flat = [];
    for (const st of stmts) {
      if (st.s.t === 'forhead') {
        const prev = flat[flat.length - 1];
        if (!prev || prev.s.t !== 'let' || prev.s.target.k !== 'var' || prev.s.target.name !== st.s.v.name || starts.has(st.a))
          throw new Error(`FOR senza assegnazione iniziale a $${st.a.toString(16)}`);
        prev.s = { t: 'for', v: st.s.v, from: prev.s.e, to: st.s.to, step: st.s.step };
        continue;
      }
      if (st.s.t === 'nop') {                    // il byte ':' dopo SYS viene ricreato dal compilatore
        const prev = flat[flat.length - 1];
        if (prev && prev.s.t === 'sys') continue;
        throw new Error('NOP inatteso');
      }
      flat.push(st);
    }

    // ---------------------------------------------------------- costruzione righe e IF
    let pos = 0;
    const lines = [];
    const collect = (until, inIf) => {
      const out = [];
      while (pos < flat.length) {
        const st = flat[pos];
        if (until !== null && st.a === until) break;
        if (out.length && starts.has(st.a)) {
          if (inIf) throw new Error(`salto dentro il corpo di un IF a $${st.a.toString(16)}`);
          break;
        }
        pos++;
        if (st.s.t === 'ifnot') {
          const body = collect(st.s.dest, true);
          out.push({ t: 'if', cond: st.s.cond, then: body });
          if (!inIf) break;                       // dopo un IF la riga finisce sempre
          continue;
        }
        out.push(st.s);
      }
      return out;
    };
    const addrOfLine = [];
    while (pos < flat.length) {
      const a = flat[pos].a;
      const st = collect(null, false);
      lines.push({ a, stmts: st });
    }
    // numerazione e risoluzione dei salti
    const num = new Map();
    lines.forEach((l, k) => { l.n = (k + 1) * 10; num.set(l.a, l.n); });
    const endAddr = flat.length ? null : null;
    const fix = list => {
      for (const s of list) {
        if (s.dest !== undefined) { if (!num.has(s.dest)) throw new Error(`destinazione senza riga $${s.dest.toString(16)}`); s.line = num.get(s.dest); delete s.dest; }
        if (s.targets) { s.lines = s.targets.map(t => { if (!num.has(t)) throw new Error('destinazione ON senza riga'); return num.get(t); }); delete s.targets; }
        if (s.t === 'if') fix(s.then);
      }
    };
    lines.forEach(l => { fix(l.stmts); l.stmts = mergeSeq(l.stmts); });
    for (const l of lines) delete l.a;
    return { lines, data: P.data };
  }

  /** ricompone PRINT, PRINT#, INPUT#, NEXT multipli a partire dalle istruzioni elementari */
  function mergeSeq(list) {
    const out = [];
    let P = null;
    const close = () => { P = null; };
    for (let k = 0; k < list.length; k++) {
      const s = list[k];
      if (s.t === 'if') { close(); s.then = mergeSeq(s.then); out.push(s); continue; }
      if (s.t === 'pchan') { close(); P = { t: 'print', chan: s.chan, items: [], cr: false }; out.push(P); continue; }
      if (s.t === 'pitem') {
        if (!P) { P = { t: 'print', chan: null, items: [], cr: false }; out.push(P); }
        P.items.push(s.it);
        if (s.cr) { P.cr = true; close(); }
        continue;
      }
      if (s.t === 'pend') { if (!P || !P.chan) throw new Error('fine PRINT# inattesa'); close(); continue; }
      if (s.t === 'pcr') {
        const last = P && P.items[P.items.length - 1];
        if (P && last && (last.fn || last.comma)) { last.sep = ''; P.cr = true; close(); continue; }
        close();
        out.push({ t: 'print', chan: null, items: [], cr: true });
        continue;
      }
      close();
      if (s.t === 'inchan') { out.push({ t: 'input', chan: s.chan, vars: [], ops: [] }); continue; }
      if (s.t === 'initem') {
        const I = out[out.length - 1];
        if (!I || I.t !== 'input') throw new Error('INPUT senza canale non supportato');
        I.vars.push(s.v); I.ops.push(s.op); continue;
      }
      if (s.t === 'next' && s.vars.length) {
        const prev = out[out.length - 1];
        if (prev && prev.t === 'next' && prev.vars.length && !prev._closed) { prev.vars.push(s.vars[0]); continue; }
      }
      out.push(s);
    }
    return out;
  }

  AV.pcode = { decodeAt, readProgram, decompile, fltValue, varName, nameBytes, LAYOUT, BINOPS, FUNCS, ARITY };
})(globalThis.AV = globalThis.AV || {});
