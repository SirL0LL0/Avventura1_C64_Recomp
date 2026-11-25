"""Mini-assembler 6502 per il sottoinsieme di sintassi 64tass prodotto da disasm.py.

Serve a verificare il round-trip (sorgente -> binario identico) senza dipendenze esterne.
Il sorgente resta compatibile con 64tass vero, che si potra' usare per le modifiche.

Uso:
    python -I tools/asm6502.py <sorgente.asm> <output.prg> [--compare originale.prg]
Supporta: '* = expr', 'nome = expr', etichette a colonna 0, .byte, .word,
istruzioni ufficiali; espressioni: numeri ($hex, %bin, dec), simboli, + - , <expr, >expr.
"""
import os
import re
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from opcodes import REV, BRANCHES

ZP_OF = {'abs': 'zp', 'abx': 'zpx', 'aby': 'zpy'}


class AsmError(Exception):
    pass


def split_args(s):
    out, cur, q = [], '', False
    for c in s:
        if c == '"': q = not q
        if c == ',' and not q:
            out.append(cur.strip()); cur = ''
        else:
            cur += c
    if cur.strip(): out.append(cur.strip())
    return out


def strip_comment(line):
    q = False
    for i, c in enumerate(line):
        if c == '"': q = not q
        if c == ';' and not q: return line[:i]
    return line


class Asm:
    def __init__(self):
        self.sym = {}

    def ev(self, e, final):
        e = e.strip()
        if e.startswith('<'): return self.ev(e[1:], final) & 0xFF
        if e.startswith('>'): return (self.ev(e[1:], final) >> 8) & 0xFF
        toks = re.findall(r"[+-]|[^+-]+", e)
        total, sign = 0, 1
        for t in toks:
            t = t.strip()
            if t == '+': sign = 1; continue
            if t == '-': sign = -1; continue
            if t.startswith('$'): v = int(t[1:], 16)
            elif t.startswith('%'): v = int(t[1:], 2)
            elif t.isdigit(): v = int(t)
            elif t == '*': v = self.pc
            elif t in self.sym: v = self.sym[t]
            elif not final: return None
            else: raise AsmError(f"simbolo indefinito: {t}")
            total += sign * v
        return total

    def run(self, src):
        lines = open(src, encoding='utf-8').read().split('\n')
        for final in (False, True):
            self.out = {}
            self.pc = 0
            self.org = None
            for ln, raw in enumerate(lines, 1):
                try:
                    self.line(strip_comment(raw).rstrip(), final)
                except AsmError as e:
                    raise SystemExit(f"{src}:{ln}: {e}\n    {raw}")
        lo, hi = min(self.out), max(self.out)
        return lo, bytes(self.out.get(a, 0) for a in range(lo, hi + 1))

    def emitb(self, *bs):
        for b in bs:
            self.out[self.pc] = b & 0xFF
            self.pc += 1

    def line(self, l, final):
        if not l.strip(): return
        m = re.match(r"^\*\s*=\s*(.+)$", l.strip())
        if m:
            self.pc = self.ev(m.group(1), True); return
        m = re.match(r"^([A-Za-z_][\w]*)\s*=\s*(.+)$", l.strip())
        if m:
            v = self.ev(m.group(2), final)
            if v is not None: self.sym[m.group(1)] = v
            return
        if not l[0].isspace():
            name, _, rest = l.partition(' ')
            self.sym[name.strip()] = self.pc if not final or name.strip() not in self.sym else self.sym[name.strip()]
            if final and self.sym[name.strip()] != self.pc:
                raise AsmError(f"etichetta {name} instabile")
            l = ' ' + rest
            if not rest.strip(): return
        s = l.strip()
        if s.startswith('.byte'):
            for a in split_args(s[5:]):
                if a.startswith('"'):
                    for c in a[1:-1]: self.emitb(ord(c))
                else:
                    v = self.ev(a, final)
                    self.emitb(v if v is not None else 0)
            return
        if s.startswith('.word'):
            for a in split_args(s[5:]):
                v = self.ev(a, final)
                v = 0 if v is None else v
                self.emitb(v & 0xFF, v >> 8)
            return
        mn, _, op = s.partition(' ')
        mn = mn.upper(); op = op.strip()
        if mn in BRANCHES:
            t = self.ev(op, final)
            off = 0 if t is None else t - (self.pc + 2)
            if final and not -128 <= off <= 127: raise AsmError("branch fuori portata")
            self.emitb(REV[(mn, 'rel')], off & 0xFF); return
        if op == '':
            self.emitb(REV[(mn, 'imp')] if (mn, 'imp') in REV else REV[(mn, 'acc')]); return
        if op.lower() == 'a':
            self.emitb(REV[(mn, 'acc')]); return
        if op.startswith('#'):
            v = self.ev(op[1:], final)
            self.emitb(REV[(mn, 'imm')], 0 if v is None else v); return
        force_w = False
        if op.startswith('@w '):
            force_w = True; op = op[3:].strip()
        mode = 'abs'
        mm = re.match(r"^\((.+),\s*x\)$", op, re.I)
        if mm: mode, op = 'izx', mm.group(1)
        else:
            mm = re.match(r"^\((.+)\),\s*y$", op, re.I)
            if mm: mode, op = 'izy', mm.group(1)
            else:
                mm = re.match(r"^\((.+)\)$", op)
                if mm: mode, op = 'ind', mm.group(1)
                else:
                    mm = re.match(r"^(.+),\s*([xy])$", op, re.I)
                    if mm: mode, op = ('abx' if mm.group(2).lower() == 'x' else 'aby'), mm.group(1)
        v = self.ev(op, final)
        if mm and op.startswith('@w '):
            force_w = True; v = self.ev(op[3:], final)
        if mode in ('izx', 'izy'):
            self.emitb(REV[(mn, mode)], 0 if v is None else v); return
        if mode in ZP_OF and not force_w and v is not None and v < 0x100 and (mn, ZP_OF[mode]) in REV:
            self.emitb(REV[(mn, ZP_OF[mode])], v); return
        if (mn, mode) not in REV: raise AsmError(f"modo non valido {mn} {mode}")
        v = 0 if v is None else v
        self.emitb(REV[(mn, mode)], v & 0xFF, (v >> 8) & 0xFF)


def main():
    src, outp = sys.argv[1], sys.argv[2]
    a = Asm()
    lo, data = a.run(src)
    open(outp, 'wb').write(bytes([lo & 0xFF, lo >> 8]) + data)
    print(f"assemblato {outp}: ${lo:04X}-${lo + len(data) - 1:04X} ({len(data)} byte)")
    if '--compare' in sys.argv:
        ref = open(sys.argv[sys.argv.index('--compare') + 1], 'rb').read()
        mine = open(outp, 'rb').read()
        if ref == mine:
            print("VERIFICA OK: binario identico all'originale")
        else:
            n = min(len(ref), len(mine))
            diff = [i for i in range(n) if ref[i] != mine[i]]
            print(f"VERIFICA FALLITA: lunghezze {len(ref)}/{len(mine)}, {len(diff)} byte diversi")
            for i in diff[:20]:
                print(f"  ${lo + i - 2:04X}: orig {ref[i]:02X} nostro {mine[i]:02X}")
            sys.exit(1)


if __name__ == '__main__':
    main()
