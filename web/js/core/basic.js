/* BASIC V2: conversione AST <-> testo (.bas).
 *
 * Istruzioni (campo t):
 *   let{target,e}  print{chan,items,cr}  if{cond,then}  goto{line}  gosub{line}
 *   on{e,kind,lines}  for{v,from,to,step}  next{vars}  return  end  stop  clr  new  restore
 *   poke{addr,val}  sys{addr}  wait{args}  get{vars}  input{chan,vars,prompt}  read{vars}
 *   dim{arrays:[{name,dims}]}  open{args}  close{args}  load/save/verify{args}  rem{text}
 * Elementi di PRINT: {e,sep} | {fn:'TAB('|'SPC(', e, sep} | {comma:true}; sep = ';' ',' o '' (fine con a capo)
 */
(function (AV) {
  'use strict';
  const { bytesToText, textToBytes } = AV.petscii;

  // ---------------------------------------------------------------- render
  const PREC = { 'OR': 1, 'AND': 2, 'NOT': 3, '=': 4, '<>': 4, '<': 4, '>': 4, '<=': 4, '>=': 4,
    '+': 5, '-': 5, '*': 6, '/': 6, 'NEG': 7, '^': 8 };

  function fmtNum(e) {
    const v = e.v;
    if (e.flt && Number.isInteger(v) && Math.abs(v) <= 32767) return v + '.0';
    let s = String(v);
    if (s.includes('e')) s = s.replace('e', 'E');
    if (s.startsWith('0.')) s = s.slice(1);
    return s;
  }

  function prec(e) {
    if (e.k === 'bin') return PREC[e.op];
    if (e.k === 'un') return e.op === 'NOT' ? PREC.NOT : PREC.NEG;
    return 99;
  }

  function expr(e) {
    switch (e.k) {
      case 'num': return fmtNum(e);
      case 'str': return '"' + bytesToText(e.b) + '"';
      case 'var': return e.name;
      case 'sys': return e.name === 'PI' ? 'π' : e.name;
      case 'arr': return e.name + '(' + e.idx.map(expr).join(',') + ')';
      case 'fn': return e.name + '(' + e.args.map(expr).join(',') + ')';
      case 'un': {
        const p = prec(e), inner = prec(e.e) < p ? '(' + expr(e.e) + ')' : expr(e.e);
        return e.op === 'NOT' ? 'NOT ' + inner : '-' + inner;
      }
      case 'bin': {
        const p = prec(e);
        const L = prec(e.l) < p ? '(' + expr(e.l) + ')' : expr(e.l);
        const R = prec(e.r) <= p ? '(' + expr(e.r) + ')' : expr(e.r);
        const op = (e.op === 'AND' || e.op === 'OR') ? ' ' + e.op + ' ' : e.op;
        return L + op + R;
      }
    }
    throw new Error('espressione sconosciuta ' + e.k);
  }

  function printItems(items) {
    let s = '';
    for (const it of items) {
      if (it.comma) { s += ','; continue; }
      s += it.fn ? it.fn + expr(it.e) + ')' : expr(it.e);
      s += it.sep || '';
    }
    return s;
  }

  function stmt(s) {
    switch (s.t) {
      case 'let': return expr(s.target) + '=' + expr(s.e);
      case 'print': {
        const head = s.chan ? 'PRINT#' + expr(s.chan) : 'PRINT';
        if (!s.items.length) return head;
        return head + (s.chan ? ',' : ' ') + printItems(s.items);
      }
      case 'if': {
        let r = 'IF ' + expr(s.cond) + ' THEN ';
        let body = s.then;
        if (body.length && body[0].t === 'goto') { r += body[0].line; body = body.slice(1); if (body.length) r += ':'; }
        return r + body.map(stmt).join(':');
      }
      case 'goto': return 'GOTO ' + s.line;
      case 'gosub': return 'GOSUB ' + s.line;
      case 'on': return 'ON ' + expr(s.e) + ' ' + s.kind + ' ' + s.lines.join(',');
      case 'for': return 'FOR ' + expr(s.v) + '=' + expr(s.from) + ' TO ' + expr(s.to) + (s.step ? ' STEP ' + expr(s.step) : '');
      case 'next': return 'NEXT' + (s.vars.length ? ' ' + s.vars.map(expr).join(',') : '');
      case 'return': case 'end': case 'stop': case 'clr': case 'new': case 'restore': return s.t.toUpperCase();
      case 'poke': return 'POKE ' + expr(s.addr) + ',' + expr(s.val);
      case 'sys': return 'SYS ' + expr(s.addr);
      case 'wait': return 'WAIT ' + s.args.map(expr).join(',');
      case 'get': return 'GET ' + (s.chan ? '#' + expr(s.chan) + ',' : '') + s.vars.map(expr).join(',');
      case 'input': return 'INPUT' + (s.chan ? '#' + expr(s.chan) + ',' : ' ') + (s.prompt ? '"' + bytesToText(s.prompt) + '";' : '') + s.vars.map(expr).join(',');
      case 'read': return 'READ ' + s.vars.map(expr).join(',');
      case 'dim': return 'DIM ' + s.arrays.map(a => a.name + '(' + a.dims.map(expr).join(',') + ')').join(',');
      case 'open': case 'close': case 'load': case 'save': case 'verify': return s.t.toUpperCase() + ' ' + s.args.map(expr).join(',');
      case 'rem': return 'REM' + s.text;
      case 'data': return 'DATA ' + s.items.map(dataItem).join(',');
    }
    throw new Error('istruzione sconosciuta ' + s.t);
  }

  function dataItem(b) {
    const t = bytesToText(b);
    if (/^[ -~]*$/.test(t) && /^-?[0-9.]+$/.test(t)) return t;
    return '"' + t + '"';
  }

  /** AST -> testo .bas. opts.dataLine = numero della prima riga DATA */
  function render(prog, opts = {}) {
    const out = [];
    if (opts.header) for (const h of opts.header) out.push(h);
    for (const l of prog.lines) out.push(l.n + ' ' + l.stmts.map(stmt).join(':'));
    if (prog.data && prog.data.length) {
      let n = opts.dataLine || (Math.ceil(((prog.lines.length ? prog.lines[prog.lines.length - 1].n : 0) + 1) / 1000) * 1000 + 1000);
      let cur = [];
      const flush = () => { if (cur.length) { out.push(n + ' DATA ' + cur.map(dataItem).join(',')); n += 10; cur = []; } };
      let len = 0;
      for (const d of prog.data) {
        const t = dataItem(d);
        if (len + t.length > 70) { flush(); len = 0; }
        cur.push(d); len += t.length + 1;
      }
      flush();
    }
    return out.join('\n') + '\n';
  }

  // ---------------------------------------------------------------- parser
  // parole chiave nell'ordine della tabella del C64 (conta per il riconoscimento)
  const KEYWORDS = ['END', 'FOR', 'NEXT', 'DATA', 'INPUT#', 'INPUT', 'DIM', 'READ', 'LET', 'GOTO', 'RUN', 'IF',
    'RESTORE', 'GOSUB', 'RETURN', 'REM', 'STOP', 'ON', 'WAIT', 'LOAD', 'SAVE', 'VERIFY', 'DEF', 'POKE', 'PRINT#',
    'PRINT', 'CONT', 'LIST', 'CLR', 'CMD', 'SYS', 'OPEN', 'CLOSE', 'GET', 'NEW', 'TAB(', 'TO', 'FN', 'SPC(', 'THEN',
    'NOT', 'STEP', 'AND', 'OR', 'SGN', 'INT', 'ABS', 'USR', 'FRE', 'POS', 'SQR', 'RND', 'LOG', 'EXP', 'COS', 'SIN',
    'TAN', 'ATN', 'PEEK', 'LEN', 'STR$', 'VAL', 'ASC', 'CHR$', 'LEFT$', 'RIGHT$', 'MID$', 'GO'];
  const FUNCS = new Set(['SGN', 'INT', 'ABS', 'USR', 'FRE', 'POS', 'SQR', 'RND', 'LOG', 'EXP', 'COS', 'SIN', 'TAN',
    'ATN', 'PEEK', 'LEN', 'STR$', 'VAL', 'ASC', 'CHR$', 'LEFT$', 'RIGHT$', 'MID$']);

  class ParseError extends Error {
    constructor(msg, line) { super((line !== undefined ? `riga ${line}: ` : '') + msg); this.line = line; }
  }

  /** tokenizza il corpo di una riga (stile "crunch" del C64) */
  function tokenize(src, lineNo) {
    const T = [];
    let i = 0;
    const up = src.toUpperCase();
    while (i < src.length) {
      const c = src[i];
      if (c === ' ' || c === '\t' || c === '\r') { i++; continue; }
      if (c === '"') {
        let j = i + 1;
        while (j < src.length && src[j] !== '"') j++;
        let bytes;
        try { bytes = textToBytes(src.slice(i + 1, j)); } catch (e) { throw new ParseError(e.message, lineNo); }
        T.push({ t: 'str', b: bytes });
        i = j + 1;
        continue;
      }
      if (c === 'π') { T.push({ t: 'id', v: 'π' }); i++; continue; }
      let kw = null;
      for (const k of KEYWORDS) if (up.startsWith(k, i)) { kw = k; break; }
      if (kw) {
        i += kw.length;
        if (kw === 'REM') { T.push({ t: 'kw', v: 'REM' }, { t: 'rest', v: src.slice(i) }); break; }
        if (kw === 'DATA') {
          // DATA: testo grezzo fino a ':' fuori dalle virgolette
          let j = i, q = false;
          while (j < src.length && (q || src[j] !== ':')) { if (src[j] === '"') q = !q; j++; }
          T.push({ t: 'kw', v: 'DATA' }, { t: 'rest', v: src.slice(i, j) });
          i = j;
          continue;
        }
        T.push({ t: 'kw', v: kw });
        continue;
      }
      if (/[0-9.]/.test(c)) {
        const m = /^(\d*\.?\d*)(E[+-]?\d+)?/i.exec(src.slice(i));
        const txt = m[0];
        if (txt === '.' || !txt) throw new ParseError('numero non valido', lineNo);
        T.push({ t: 'num', v: parseFloat(txt), flt: /[.E]/i.test(txt) });
        i += txt.length;
        continue;
      }
      if (/[A-Z]/i.test(c)) {
        let name = up[i]; i++;
        while (i < src.length && /[A-Z0-9]/i.test(src[i])) {
          let isKw = false;
          for (const k of KEYWORDS) if (up.startsWith(k, i)) { isKw = true; break; }
          if (isKw) break;
          name += up[i]; i++;
        }
        if (src[i] === '$' || src[i] === '%') { name += src[i]; i++; }
        const suf = /[$%]$/.test(name) ? name.slice(-1) : '';
        const base = suf ? name.slice(0, -1) : name;
        T.push({ t: 'id', v: base.slice(0, 2) + suf, full: name });
        continue;
      }
      const two = src.slice(i, i + 2);
      if (['<=', '>=', '<>', '=<', '=>', '><'].includes(two)) {
        T.push({ t: 'op', v: two === '=<' ? '<=' : two === '=>' ? '>=' : two === '><' ? '<>' : two }); i += 2; continue;
      }
      if ('+-*/^=<>(),;:#?'.includes(c)) {
        if (c === '?') T.push({ t: 'kw', v: 'PRINT' }); else T.push({ t: 'op', v: c });
        i++; continue;
      }
      if (c === '↑') { T.push({ t: 'op', v: '^' }); i++; continue; }
      throw new ParseError('carattere inatteso ' + JSON.stringify(c), lineNo);
    }
    return T;
  }

  function parseLine(body, lineNo, dataOut) {
    const T = tokenize(body, lineNo);
    let p = 0;
    const err = m => { throw new ParseError(m, lineNo); };
    const peek = () => T[p];
    const isOp = v => T[p] && T[p].t === 'op' && T[p].v === v;
    const isKw = v => T[p] && T[p].t === 'kw' && T[p].v === v;
    const eatOp = v => { if (!isOp(v)) err(`atteso '${v}'`); p++; };
    const eatKw = v => { if (!isKw(v)) err(`atteso ${v}`); p++; };
    const atEnd = () => p >= T.length || isOp(':');

    function primary() {
      const t = peek();
      if (!t) err('espressione incompleta');
      if (t.t === 'num') { p++; return t.flt ? { k: 'num', v: t.v, flt: true } : { k: 'num', v: t.v }; }
      if (t.t === 'str') { p++; return { k: 'str', b: t.b }; }
      if (t.t === 'op' && t.v === '(') { p++; const e = expression(); eatOp(')'); return e; }
      if (t.t === 'op' && t.v === '-') { p++; return { k: 'un', op: '-', e: unaryOperand() }; }
      if (t.t === 'op' && t.v === '+') { p++; return unaryOperand(); }
      if (t.t === 'kw' && FUNCS.has(t.v)) {
        p++; eatOp('(');
        const args = [expression()];
        while (isOp(',')) { p++; args.push(expression()); }
        eatOp(')');
        return { k: 'fn', name: t.v, args };
      }
      if (t.t === 'kw' && t.v === 'NOT') { p++; return { k: 'un', op: 'NOT', e: binary(PREC.NOT + 1) }; }
      if (t.t === 'id') {
        p++;
        if (t.v === 'π') return { k: 'sys', name: 'PI' };
        if (['ST', 'TI', 'TI$'].includes(t.v) && !isOp('(')) return { k: 'sys', name: t.v };
        if (isOp('(')) {
          p++;
          const idx = [expression()];
          while (isOp(',')) { p++; idx.push(expression()); }
          eatOp(')');
          return { k: 'arr', name: t.v, idx };
        }
        return { k: 'var', name: t.v };
      }
      err('espressione non valida');
    }
    // il meno unario lega meno di ^ ma piu' di * /
    function unaryOperand() { return power(); }
    function power() {
      let l = primary();
      while (isOp('^')) { p++; l = { k: 'bin', op: '^', l, r: primary() }; }
      return l;
    }
    function binary(minPrec) {
      let l = minPrec > PREC['^'] ? primary() : (minPrec > PREC.NEG ? power() : power());
      for (;;) {
        const t = peek();
        if (!t) break;
        let op = null;
        if (t.t === 'op' && PREC[t.v] !== undefined && t.v !== '^') op = t.v;
        if (t.t === 'kw' && (t.v === 'AND' || t.v === 'OR')) op = t.v;
        if (!op) break;
        const pr = PREC[op];
        if (pr < minPrec) break;
        p++;
        const r = binary(pr + 1);
        l = { k: 'bin', op, l, r };
      }
      return l;
    }
    function expression() {
      // NOT a livello basso: NOT x AND y = (NOT x) AND y ? No: in BASIC V2 NOT lega piu' di AND ma meno dei confronti
      if (isKw('NOT')) {
        p++;
        let e = { k: 'un', op: 'NOT', e: binary(PREC.NOT + 1) };
        // continua con AND/OR
        for (;;) {
          const t = peek();
          if (t && t.t === 'kw' && (t.v === 'AND' || t.v === 'OR')) {
            p++; const pr = PREC[t.v];
            e = { k: 'bin', op: t.v, l: e, r: binary(pr + 1) };
          } else break;
        }
        return e;
      }
      return binary(1);
    }
    function ref() {
      const t = peek();
      if (!t || t.t !== 'id' || t.v === 'π') err('attesa variabile');
      const e = primary();
      if (e.k !== 'var' && e.k !== 'arr') err('attesa variabile');
      return e;
    }
    function lineNum() {
      const t = peek();
      if (!t || t.t !== 'num' || t.flt) err('atteso numero di riga');
      p++; return t.v;
    }
    function list(fn) { const r = [fn()]; while (isOp(',')) { p++; r.push(fn()); } return r; }

    function printStmt(chan) {
      const items = [];
      while (!atEnd()) {
        if (isOp(';')) { p++; const last = items[items.length - 1]; if (last && !last.comma) last.sep = ';'; continue; }
        if (isOp(',')) {
          p++;
          const last = items[items.length - 1];
          if (last && !last.comma && !last.fn && !last.sep) last.sep = ','; else items.push({ comma: true });
          continue;
        }
        const last = items[items.length - 1];
        if (last && !last.comma && !last.sep) last.sep = ';';      // elementi adiacenti: PRINT A"B"
        if (isKw('TAB(') || isKw('SPC(')) {
          const fn = peek().v; p++;
          const e = expression(); eatOp(')');
          items.push({ fn, e, sep: '' });
          continue;
        }
        items.push({ e: expression(), sep: '' });
      }
      const last = items[items.length - 1];
      const cr = !last || (!last.comma && !last.sep);
      return { t: 'print', chan: chan || null, items, cr };
    }

    function statements(inIf) {
      const out = [];
      for (;;) {
        if (p >= T.length) break;
        if (isOp(':')) { p++; continue; }
        const s = statement();
        if (s === null) continue;
        out.push(s);
        if (s.t === 'if' || s.t === 'rem') break;
        if (p < T.length && !isOp(':')) err('fine istruzione attesa');
      }
      return out;
    }

    function statement() {
      const t = peek();
      if (t.t === 'id') {
        const target = ref(); eatOp('=');
        return { t: 'let', target, e: expression() };
      }
      if (t.t !== 'kw') err('istruzione non valida');
      p++;
      switch (t.v) {
        case 'LET': { const target = ref(); eatOp('='); return { t: 'let', target, e: expression() }; }
        case 'PRINT': return printStmt(null);
        case 'PRINT#': { const chan = expression(); if (isOp(',')) { p++; return printStmt(chan); } return { t: 'print', chan, items: [], cr: true }; }
        case 'IF': {
          const cond = expression();
          let then;
          if (isKw('GOTO')) { p++; then = [{ t: 'goto', line: lineNum() }]; then = then.concat(statements(true)); }
          else {
            eatKw('THEN');
            if (peek() && peek().t === 'num') { then = [{ t: 'goto', line: lineNum() }]; then = then.concat(statements(true)); }
            else then = statements(true);
          }
          return { t: 'if', cond, then };
        }
        case 'GOTO': return { t: 'goto', line: lineNum() };
        case 'GO': eatKw('TO'); return { t: 'goto', line: lineNum() };
        case 'GOSUB': return { t: 'gosub', line: lineNum() };
        case 'ON': {
          const e = expression();
          let kind;
          if (isKw('GOTO')) kind = 'GOTO'; else if (isKw('GOSUB')) kind = 'GOSUB'; else err('atteso GOTO o GOSUB');
          p++;
          return { t: 'on', e, kind, lines: list(lineNum) };
        }
        case 'FOR': {
          const v = ref(); if (v.k !== 'var') err('variabile FOR non valida');
          eatOp('='); const from = expression(); eatKw('TO'); const to = expression();
          let step = null;
          if (isKw('STEP')) { p++; step = expression(); }
          return { t: 'for', v, from, to, step };
        }
        case 'NEXT': return { t: 'next', vars: atEnd() ? [] : list(ref) };
        case 'RETURN': case 'END': case 'STOP': case 'CLR': case 'NEW': case 'RESTORE':
          return { t: t.v.toLowerCase() };
        case 'POKE': { const addr = expression(); eatOp(','); return { t: 'poke', addr, val: expression() }; }
        case 'SYS': return { t: 'sys', addr: expression() };
        case 'WAIT': return { t: 'wait', args: list(expression) };
        case 'GET': {
          let chan = null;
          if (isOp('#')) { p++; chan = expression(); eatOp(','); }
          return { t: 'get', chan, vars: list(ref) };
        }
        case 'INPUT#': { const chan = expression(); eatOp(','); return { t: 'input', chan, vars: list(ref) }; }
        case 'INPUT': {
          let prompt = null;
          if (peek() && peek().t === 'str') { prompt = peek().b; p++; eatOp(';'); }
          return { t: 'input', chan: null, prompt, vars: list(ref) };
        }
        case 'READ': return { t: 'read', vars: list(ref) };
        case 'DIM': return {
          t: 'dim', arrays: list(() => {
            const id = peek(); if (!id || id.t !== 'id') err('atteso nome array'); p++;
            eatOp('('); const dims = list(expression); eatOp(')');
            return { name: id.v, dims };
          })
        };
        case 'OPEN': case 'CLOSE': case 'LOAD': case 'SAVE': case 'VERIFY':
          return { t: t.v.toLowerCase(), args: atEnd() ? [] : list(expression) };
        case 'REM': { const r = T[p++]; return { t: 'rem', text: r ? r.v : '' }; }
        case 'DATA': { const r = T[p++]; if (dataOut) parseData(r.v, lineNo).forEach(d => dataOut.push(d)); return null; }
      }
      err('istruzione non supportata: ' + t.v);
    }

    return statements(false);
  }

  function parseData(txt, lineNo) {
    const items = [];
    let i = 0;
    for (;;) {
      while (txt[i] === ' ') i++;
      let item;
      if (txt[i] === '"') {
        const j = txt.indexOf('"', i + 1);
        item = txt.slice(i + 1, j < 0 ? txt.length : j);
        i = j < 0 ? txt.length : j + 1;
        while (i < txt.length && txt[i] !== ',') i++;
      } else {
        let j = txt.indexOf(',', i);
        if (j < 0) j = txt.length;
        item = txt.slice(i, j).replace(/\s+$/, '');
        i = j;
      }
      try { items.push(textToBytes(item)); } catch (e) { throw new ParseError(e.message, lineNo); }
      if (txt[i] !== ',') break;
      i++;
    }
    return items;
  }

  /** testo .bas -> AST { lines, data } */
  function parse(text) {
    const lines = [];
    const data = [];
    let prev = -1;
    text.split(/\r?\n/).forEach((raw, k) => {
      const line = raw.replace(/^﻿/, '');
      if (!line.trim()) return;
      const m = /^\s*(\d+)\s?(.*)$/.exec(line);
      if (!m) throw new ParseError(`riga ${k + 1} del file senza numero`, undefined);
      const n = +m[1];
      if (n <= prev) throw new ParseError(`numero di riga ${n} non crescente`, n);
      prev = n;
      const stmts = parseLine(m[2], n, data);
      lines.push({ n, stmts });
    });
    // le righe composte solo da DATA restano come righe vuote (non generano codice)
    return { lines: lines.filter(l => l.stmts.length || false), data, allLines: lines };
  }

  AV.basic = { render, parse, expr, stmt, tokenize, ParseError, KEYWORDS };
})(globalThis.AV = globalThis.AV || {});
