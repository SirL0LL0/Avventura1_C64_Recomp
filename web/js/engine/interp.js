/* Interprete BASIC V2 per il motore moderno di Avventura 1.
 * Esegue l'AST prodotto da AV.basic.parse (quindi anche listati modificati).
 * Semantica allineata al C64: variabili a 2 caratteri significativi, vero = -1,
 * stampa dei numeri con spazio iniziale/finale, FOR/NEXT e GOSUB su uno stack unico.
 *
 * Il "tempo" e' simulato: ogni istruzione costa un'unita' e l'host decide quante unita'
 * eseguire per fotogramma (velocita' originale, veloce, turbo).
 */
(function (AV) {
  'use strict';
  const { textToBytes } = AV.petscii;

  const ERR = {
    SYNTAX: 'SYNTAX', TYPE: 'TYPE MISMATCH', QTY: 'ILLEGAL QUANTITY', UNDEF: "UNDEF'D STATEMENT",
    RETURN: 'RETURN WITHOUT GOSUB', NEXT: 'NEXT WITHOUT FOR', DATA: 'OUT OF DATA', SUB: 'BAD SUBSCRIPT',
    REDIM: "REDIM'D ARRAY", DIV0: 'DIVISION BY ZERO', OVERFLOW: 'OVERFLOW', STRLEN: 'STRING TOO LONG',
    FILE_OPEN: 'FILE OPEN', NOT_OPEN: 'FILE NOT OPEN', NOT_FOUND: 'FILE NOT FOUND', STACK: 'OUT OF MEMORY',
  };

  class BasicError extends Error {
    constructor(code, line) { super(code); this.code = code; this.line = line; }
  }

  const S = s => String.fromCharCode.apply(null, s);          // byte[] -> stringa (codici 0-255)
  const isStrName = n => n.endsWith('$');
  const isIntName = n => n.endsWith('%');

  /** formato numerico del C64 (FOUT): " 5", "-3.5", " 1.23456789E+10" */
  function fmtNumber(v) {
    if (!isFinite(v)) throw new BasicError(ERR.OVERFLOW);
    let s;
    if (v === 0) s = '0';
    else {
      const a = Math.abs(v);
      if (a >= 1e9 || a < 0.01) {
        let [m, e] = a.toExponential(8).split('e');
        m = m.replace(/\.?0+$/, '');
        const ex = parseInt(e, 10);
        s = m + 'E' + (ex < 0 ? '-' : '+') + String(Math.abs(ex)).padStart(2, '0');
      } else {
        s = String(parseFloat(a.toPrecision(9)));
        if (s.startsWith('0.')) s = s.slice(1);
      }
      if (v < 0) return '-' + s;
    }
    return ' ' + s;
  }

  function val(str) {
    const m = /^\s*([+-]?)\s*(\d*\.?\d*)(?:\s*E\s*([+-]?\d+))?/i.exec(str.replace(/ /g, ''));
    if (!m || !m[2] || m[2] === '.') return 0;
    return parseFloat((m[1] || '') + m[2] + (m[3] ? 'e' + m[3] : '')) || 0;
  }

  class Machine {
    /**
     * host: {
     *   screen: AV.screen.Screen,
     *   tapeWrite(name, text) -> Promise   salvataggio su nastro virtuale
     *   tapeRead(name) -> Promise<{name,text}|null>
     *   onEnd(reason)                      'end' | 'reset' | 'error'
     * }
     */
    constructor(prog, host) {
      this.host = host;
      this.scr = host.screen;
      this.compileFlat(prog);
      this.data = prog.data || [];
      this.keys = [];
      this.reset();
    }

    compileFlat(prog) {
      const code = [], lineIdx = new Map(), fix = [];
      for (const l of prog.lines) {
        lineIdx.set(l.n, code.length);
        const ifs = [];
        const walk = list => {
          for (const s of list) {
            if (s.t === 'if') { const ins = { t: 'if', cond: s.cond, ln: l.n, jump: -1 }; code.push(ins); ifs.push(ins); walk(s.then); }
            else { s.ln = l.n; code.push(s); }
          }
        };
        walk(l.stmts);
        const next = code.length;
        ifs.forEach(i => { i.jump = next; });
      }
      code.push({ t: 'end', ln: -1, implicit: true });
      for (const s of code) {
        if (s.t === 'goto' || s.t === 'gosub') s.to = this.lineRef(lineIdx, s.line, s);
        if (s.t === 'on') s.to = s.lines.map(n => this.lineRef(lineIdx, n, s));
      }
      this.code = code;
      this.lineIdx = lineIdx;
    }

    lineRef(idx, n, s) {
      if (!idx.has(n)) throw new BasicError(ERR.UNDEF, s.ln);
      return idx.get(n);
    }

    reset() {
      this.vars = new Map();
      this.arrays = new Map();
      this.stack = [];
      this.pc = 0;
      this.dataPtr = 0;
      this.files = new Map();
      this.halted = false;
      this.wait = null;            // Promise in corso (I/O)
      this.startTime = performance.now();
      this.mem = new Map();
      this.channelOut = null;
      this.channelIn = null;
    }

    // ------------------------------------------------------------ tastiera
    pushKey(b) { if (this.keys.length < 10) this.keys.push(b); }

    // ------------------------------------------------------------ variabili
    getVar(name) {
      if (name === 'ST') return 0;
      if (this.vars.has(name)) return this.vars.get(name);
      return isStrName(name) ? '' : 0;
    }
    setVar(name, v, line) {
      if (isStrName(name)) { if (typeof v !== 'string') throw new BasicError(ERR.TYPE, line); }
      else {
        if (typeof v !== 'number') throw new BasicError(ERR.TYPE, line);
        if (isIntName(name)) { v = Math.floor(v); if (v < -32768 || v > 32767) throw new BasicError(ERR.QTY, line); }
      }
      this.vars.set(name, v);
    }
    array(name, nd, line) {
      let a = this.arrays.get(name);
      if (!a) a = this.dimArray(name, new Array(nd).fill(10), line);
      if (a.dims.length !== nd) throw new BasicError(ERR.SUB, line);
      return a;
    }
    dimArray(name, dims, line) {
      if (this.arrays.has(name)) throw new BasicError(ERR.REDIM, line);
      const size = dims.reduce((p, d) => p * (d + 1), 1);
      const a = { dims, data: new Array(size).fill(isStrName(name) ? '' : 0) };
      this.arrays.set(name, a);
      return a;
    }
    arrIndex(a, idx, line) {
      let off = 0;
      for (let k = idx.length - 1; k >= 0; k--) {
        const i = Math.floor(idx[k]);
        if (i < 0 || i > a.dims[k]) throw new BasicError(ERR.SUB, line);
        off = off * (a.dims[k] + 1) + i;
      }
      return off;
    }
    assign(t, v, line) {
      if (t.k === 'var') return this.setVar(t.name, v, line);
      const idx = t.idx.map(e => this.num(e, line));
      const a = this.array(t.name, idx.length, line);
      if (isStrName(t.name) ? typeof v !== 'string' : typeof v !== 'number') throw new BasicError(ERR.TYPE, line);
      if (isIntName(t.name)) v = Math.floor(v);
      a.data[this.arrIndex(a, idx, line)] = v;
    }

    // ------------------------------------------------------------ espressioni
    num(e, line) { const v = this.ev(e, line); if (typeof v !== 'number') throw new BasicError(ERR.TYPE, line); return v; }
    str(e, line) { const v = this.ev(e, line); if (typeof v !== 'string') throw new BasicError(ERR.TYPE, line); return v; }
    int16(v, line) { v = Math.floor(v); if (v < -32768 || v > 32767) throw new BasicError(ERR.QTY, line); return v; }
    byteArg(e, line) { const v = Math.floor(this.num(e, line)); if (v < 0 || v > 255) throw new BasicError(ERR.QTY, line); return v; }

    ev(e, line) {
      switch (e.k) {
        case 'num': return e.v;
        case 'str': return e.s !== undefined ? e.s : (e.s = S(e.b));
        case 'var': return this.getVar(e.name);
        case 'sys':
          if (e.name === 'PI') return Math.PI;
          if (e.name === 'ST') return 0;
          if (e.name === 'TI') return Math.floor((performance.now() - this.startTime) * 0.06);
          { const t = Math.floor((performance.now() - this.startTime) / 1000);
            const p = n => String(n).padStart(2, '0');
            return p(Math.floor(t / 3600) % 24) + p(Math.floor(t / 60) % 60) + p(t % 60); }
        case 'arr': {
          const idx = e.idx.map(x => this.num(x, line));
          const a = this.array(e.name, idx.length, line);
          return a.data[this.arrIndex(a, idx, line)];
        }
        case 'un':
          if (e.op === '-') return -this.num(e.e, line);
          return ~this.int16(this.num(e.e, line), line);
        case 'bin': return this.binop(e.op, this.ev(e.l, line), this.ev(e.r, line), line);
        case 'fn': return this.fn(e, line);
      }
      throw new BasicError(ERR.SYNTAX, line);
    }

    binop(op, a, b, line) {
      const sa = typeof a === 'string', sb = typeof b === 'string';
      if (sa !== sb) throw new BasicError(ERR.TYPE, line);
      switch (op) {
        case '+':
          if (sa) { const r = a + b; if (r.length > 255) throw new BasicError(ERR.STRLEN, line); return r; }
          return a + b;
        case '=': return a === b ? -1 : 0;
        case '<>': return a !== b ? -1 : 0;
        case '<': return a < b ? -1 : 0;
        case '>': return a > b ? -1 : 0;
        case '<=': return a <= b ? -1 : 0;
        case '>=': return a >= b ? -1 : 0;
      }
      if (sa) throw new BasicError(ERR.TYPE, line);
      switch (op) {
        case '-': return a - b;
        case '*': return a * b;
        case '/': if (b === 0) throw new BasicError(ERR.DIV0, line); return a / b;
        case '^': return Math.pow(a, b);
        case 'AND': return this.int16(a, line) & this.int16(b, line);
        case 'OR': return this.int16(a, line) | this.int16(b, line);
      }
      throw new BasicError(ERR.SYNTAX, line);
    }

    fn(e, line) {
      const A = e.args, n = i => this.num(A[i], line), s = i => this.str(A[i], line);
      switch (e.name) {
        case 'SGN': return Math.sign(n(0));
        case 'INT': return Math.floor(n(0));
        case 'ABS': return Math.abs(n(0));
        case 'SQR': { const v = n(0); if (v < 0) throw new BasicError(ERR.QTY, line); return Math.sqrt(v); }
        case 'RND': { n(0); return Math.random(); }
        case 'LOG': { const v = n(0); if (v <= 0) throw new BasicError(ERR.QTY, line); return Math.log(v); }
        case 'EXP': return Math.exp(n(0));
        case 'COS': return Math.cos(n(0));
        case 'SIN': return Math.sin(n(0));
        case 'TAN': return Math.tan(n(0));
        case 'ATN': return Math.atan(n(0));
        case 'FRE': return 20000;
        case 'POS': return this.scr.col % 40;
        case 'PEEK': return this.peek(Math.floor(n(0)));
        case 'USR': throw new BasicError(ERR.QTY, line);
        case 'LEN': return s(0).length;
        case 'STR$': return fmtNumber(n(0));
        case 'VAL': return val(s(0));
        case 'ASC': { const v = s(0); if (!v.length) throw new BasicError(ERR.QTY, line); return v.charCodeAt(0); }
        case 'CHR$': return String.fromCharCode(this.byteArg(A[0], line));
        case 'LEFT$': return s(0).slice(0, this.byteArg(A[1], line));
        case 'RIGHT$': { const v = s(0), k = this.byteArg(A[1], line); return k >= v.length ? v : v.slice(v.length - k); }
        case 'MID$': {
          const v = s(0), st = this.byteArg(A[1], line);
          if (st < 1) throw new BasicError(ERR.QTY, line);
          const k = A.length > 2 ? this.byteArg(A[2], line) : 255;
          return v.substr(st - 1, k);
        }
      }
      throw new BasicError(ERR.SYNTAX, line);
    }

    // ------------------------------------------------------------ memoria simulata
    peek(a) {
      const s = this.scr;
      if (a === 53280) return s.border | 0xF0;
      if (a === 53281) return s.bg | 0xF0;
      if (a === 646) return s.color;
      if (a === 197) return this.keys.length ? 0 : 64;
      if (a === 198) return this.keys.length;
      if (a >= 1024 && a < 2024) return s.chars[a - 1024];
      if (a >= 55296 && a < 56296) return s.colors[a - 55296] | 0xF0;
      return this.mem.get(a) || 0;
    }
    poke(a, v, line) {
      if (a < 0 || a > 65535) throw new BasicError(ERR.QTY, line);
      const s = this.scr;
      if (a === 53280) { s.border = v & 15; s.dirty = true; return; }
      if (a === 53281) { s.bg = v & 15; s.dirty = true; return; }
      if (a === 646) { s.color = v & 15; return; }
      if (a === 198) { if (v === 0) this.keys.length = 0; return; }
      if (a >= 1024 && a < 2024) { s.chars[a - 1024] = v; s.dirty = true; return; }
      if (a >= 55296 && a < 56296) { s.colors[a - 55296] = v & 15; s.dirty = true; return; }
      this.mem.set(a, v);
    }

    // ------------------------------------------------------------ output
    out(str) {
      if (this.channelOut) { this.channelOut.buf += str; return; }
      for (let i = 0; i < str.length; i++) this.scr.out(str.charCodeAt(i));
      this.cost += str.length * 0.15;
    }

    printStmt(s) {
      const line = s.ln;
      let ch = null;
      if (s.chan) {
        const n = this.byteArg(s.chan, line);
        ch = this.files.get(n);
        if (!ch) throw new BasicError(ERR.NOT_OPEN, line);
        if (ch.mode !== 'w') throw new BasicError(ERR.NOT_OPEN, line);
      }
      const prev = this.channelOut;
      this.channelOut = ch;
      const toFile = !!ch;
      for (const it of s.items) {
        if (it.comma) { this.tabZone(toFile); continue; }
        if (it.fn) {
          const k = this.byteArg(it.e, line);
          if (toFile) this.out(' '.repeat(it.fn === 'SPC(' ? k : 0));
          else {
            const n = it.fn === 'SPC(' ? k : Math.max(0, k - this.scr.col);
            for (let i = 0; i < n; i++) this.scr.out(0x1D);
          }
          continue;
        }
        const v = this.ev(it.e, line);
        if (typeof v === 'string') this.out(v);
        else { this.out(fmtNumber(v)); this.out(toFile ? ' ' : '\x1D'); }
        if (it.sep === ',') this.tabZone(toFile);
      }
      if (s.cr) this.out('\r');
      this.channelOut = prev;
    }

    tabZone(toFile) {
      if (toFile) { this.out('\x1D'); return; }
      const c = this.scr.col % 40;
      const n = 10 - (c % 10);
      for (let i = 0; i < n; i++) this.scr.out(0x1D);
    }

    // ------------------------------------------------------------ I/O su nastro virtuale
    openFile(args, line) {
      const n = this.byteArg(args[0], line);
      if (this.files.has(n)) throw new BasicError(ERR.FILE_OPEN, line);
      const dev = args.length > 1 ? this.byteArg(args[1], line) : 1;
      const sa = args.length > 2 ? this.byteArg(args[2], line) : 0;
      const name = args.length > 3 ? this.str(args[3], line) : '';
      if (dev !== 1 && dev !== 8) { this.files.set(n, { mode: 'null', buf: '' }); return; }
      if (sa === 0) {
        const f = { mode: 'r', buf: '', pos: 0, name };
        this.files.set(n, f);
        this.wait = this.host.tapeRead(name).then(r => {
          if (!r) throw new BasicError(ERR.NOT_FOUND, line);
          f.buf = r.text; f.name = r.name;
        });
      } else {
        this.files.set(n, { mode: 'w', buf: '', name });
      }
    }
    closeFile(nE, line) {
      const n = this.byteArg(nE, line);
      const f = this.files.get(n);
      if (!f) return;
      this.files.delete(n);
      if (f.mode === 'w') this.wait = this.host.tapeWrite(f.name, f.buf);
    }
    readField(f) {
      // INPUT#: campi separati da CR o virgola; le virgolette racchiudono stringhe
      let s = f.buf, i = f.pos;
      while (i < s.length && s[i] === ' ') i++;
      let v = '';
      if (s[i] === '"') {
        i++;
        while (i < s.length && s[i] !== '"') v += s[i++];
        i++;
        while (i < s.length && s[i] !== '\r' && s[i] !== ',') i++;
      } else {
        while (i < s.length && s[i] !== '\r' && s[i] !== ',') v += s[i++];
      }
      if (i < s.length) i++;
      f.pos = i;
      return { v, eof: i >= s.length };
    }

    // ------------------------------------------------------------ esecuzione
    /** esegue fino a `budget` unita' di tempo; ritorna 'run' | 'wait' | 'halt' */
    run(budget) {
      if (this.halted) return 'halt';
      if (this.wait) return 'wait';
      this.cost = 0;
      try {
        while (this.cost < budget) {
          const s = this.code[this.pc++];
          this.cost += 1;
          this.exec(s);
          if (this.halted) return 'halt';
          if (this.wait) {
            const w = this.wait;
            w.then(() => { if (this.wait === w) this.wait = null; },
              e => { if (this.wait === w) { this.wait = null; this.fail(e, s.ln); } });
            return 'wait';
          }
        }
      } catch (e) {
        const s = this.code[this.pc - 1];
        this.fail(e, s ? s.ln : -1);
        return 'halt';
      }
      return 'run';
    }

    fail(e, line) {
      this.halted = true;
      if (!(e instanceof BasicError)) { console.error(e); e = new BasicError(e.message || String(e)); }
      const where = e.line !== undefined ? e.line : line;
      this.scr.cr();
      const msg = '?' + e.code + '  ERROR' + (where >= 0 ? ' IN ' + where : '');
      for (const ch of msg) this.scr.out(ch.charCodeAt(0) >= 0x41 && ch.charCodeAt(0) <= 0x5A && this.scr.lower ? ch.charCodeAt(0) + 0x80 : ch.charCodeAt(0));
      this.scr.cr();
      this.error = { code: e.code, line: where };
      this.host.onEnd('error', this.error);
    }

    exec(s) {
      const line = s.ln;
      switch (s.t) {
        case 'let': return this.assign(s.target, this.ev(s.e, line), line);
        case 'print': return this.printStmt(s);
        case 'if': {
          const c = this.ev(s.cond, line);
          if (typeof c === 'string' ? c.length === 0 : c === 0) this.pc = s.jump;
          return;
        }
        case 'goto': this.pc = s.to; return;
        case 'gosub':
          if (this.stack.length > 200) throw new BasicError(ERR.STACK, line);
          this.stack.push({ k: 'gosub', ret: this.pc }); this.pc = s.to; return;
        case 'on': {
          const k = Math.floor(this.num(s.e, line));
          if (k < 0 || k > 255) throw new BasicError(ERR.QTY, line);
          if (k >= 1 && k <= s.to.length) {
            if (s.kind === 'GOSUB') this.stack.push({ k: 'gosub', ret: this.pc });
            this.pc = s.to[k - 1];
          }
          return;
        }
        case 'return': {
          while (this.stack.length && this.stack[this.stack.length - 1].k !== 'gosub') this.stack.pop();
          const f = this.stack.pop();
          if (!f) throw new BasicError(ERR.RETURN, line);
          this.pc = f.ret; return;
        }
        case 'for': {
          const name = s.v.name;
          this.setVar(name, this.num(s.from, line), line);
          const to = this.num(s.to, line), step = s.step ? this.num(s.step, line) : 1;
          // un FOR sulla stessa variabile rimuove il ciclo precedente e quelli interni
          for (let i = this.stack.length - 1; i >= 0; i--) {
            const f = this.stack[i];
            if (f.k === 'gosub') break;
            if (f.name === name) { this.stack.length = i; break; }
          }
          this.stack.push({ k: 'for', name, to, step, body: this.pc });
          return;
        }
        case 'next': {
          const names = s.vars.length ? s.vars.map(v => v.name) : [null];
          for (let n = 0; n < names.length; n++) {
            let i = this.stack.length - 1;
            for (; i >= 0; i--) {
              const f = this.stack[i];
              if (f.k === 'gosub') { i = -1; break; }
              if (names[n] === null || f.name === names[n]) break;
            }
            if (i < 0) throw new BasicError(ERR.NEXT, line);
            this.stack.length = i + 1;
            const f = this.stack[i];
            const v = this.getVar(f.name) + f.step;
            this.setVar(f.name, v, line);
            if (f.step >= 0 ? v <= f.to : v >= f.to) { this.pc = f.body; return; }
            this.stack.length = i;
          }
          return;
        }
        case 'end': this.halted = true; this.host.onEnd('end'); return;
        case 'stop': this.halted = true; this.host.onEnd('stop', { line }); return;
        case 'clr': this.vars.clear(); this.arrays.clear(); this.stack.length = 0; this.dataPtr = 0; return;
        case 'new': this.halted = true; this.host.onEnd('end'); return;
        case 'restore': this.dataPtr = 0; return;
        case 'poke': return this.poke(Math.floor(this.num(s.addr, line)), this.byteArg(s.val, line), line);
        case 'sys': {
          const a = Math.floor(this.num(s.addr, line));
          if (a === 64738) { this.halted = true; this.host.onEnd('reset'); return; }
          return;            // altre SYS ignorate
        }
        case 'wait': return;
        case 'get': {
          for (const v of s.vars) {
            let ch = '';
            if (this.keys.length) ch = String.fromCharCode(this.keys.shift());
            if (isStrName(v.name)) this.assign(v, ch, line);
            else this.assign(v, ch >= '0' && ch <= '9' ? ch.charCodeAt(0) - 48 : 0, line);
          }
          if (!this.keys.length) this.cost += 2;      // attesa tasto: libera la CPU
          return;
        }
        case 'input': {
          if (!s.chan) return this.keyboardInput(s);
          const n = this.byteArg(s.chan, line);
          const f = this.files.get(n);
          if (!f || f.mode !== 'r') throw new BasicError(ERR.NOT_OPEN, line);
          for (const v of s.vars) {
            const r = this.readField(f);
            this.assign(v, isStrName(v.name) ? r.v : val(r.v), line);
          }
          return;
        }
        case 'read':
          for (const v of s.vars) {
            if (this.dataPtr >= this.data.length) throw new BasicError(ERR.DATA, line);
            const d = S(this.data[this.dataPtr++]);
            this.assign(v, isStrName(v.name) ? d : val(d), line);
          }
          return;
        case 'dim':
          for (const a of s.arrays) this.dimArray(a.name, a.dims.map(e => Math.floor(this.num(e, line))), line);
          return;
        case 'open': return this.openFile(s.args, line);
        case 'close': return this.closeFile(s.args[0], line);
        case 'load': case 'save': case 'verify': return;
        case 'rem': return;
      }
      throw new BasicError(ERR.SYNTAX, line);
    }

    keyboardInput(s) {
      const line = s.ln;
      if (s.prompt) this.out(S(s.prompt));
      this.out('? ');
      this.wait = this.host.readLine().then(text => {
        const parts = text.split(',');
        s.vars.forEach((v, k) => {
          const t = parts[k] !== undefined ? parts[k] : '';
          this.assign(v, isStrName(v.name) ? t.trim() : val(t), line);
        });
      });
    }
  }

  AV.interp = { Machine, BasicError, fmtNumber, val, ERR };
})(globalThis.AV = globalThis.AV || {});
