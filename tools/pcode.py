"""Decodificatore del P-code (BASIC compilato) di Avventura 1.

Modello ricavato dall'analisi del runtime ($081F-$1783): vedi docs/PCODE.md.
I nomi delle variabili vengono letti dall'area variabili (VARTAB), quelli degli
array dalle istruzioni DIM.

Uso:
    python -I tools/pcode.py <immagine.prg> <output.txt> [--from $1AC4] [--to $7DAB]
"""
import sys

BINOPS = {1: '>', 2: '=', 3: '>=', 4: '<', 5: '<>', 6: '<=', 7: '+', 8: '-', 9: '*', 10: '/', 11: '^',
          12: 'AND', 13: 'OR'}
FUNCS = ['SGN', 'INT', 'ABS', 'USR', 'FRE', 'POS', 'SQR', 'RND', 'LOG', 'EXP', 'COS', 'SIN', 'TAN', 'ATN',
         'PEEK', 'LEN', 'STR$', 'VAL', 'ASC', 'CHR$', 'LEFT$', 'RIGHT$', 'MID$']

# opcode -> (nome, lunghezza). Lunghezze speciali: 'b1' = nel byte 1, 'b5' = nel byte 5
OPS = {
    0x00: ('END_OF_CODE', 1),
    0x0E: ('NEG', 1), 0x0F: ('NOT', 1),
    0x10: ('DIM', 6),
    0x11: ('FOR', 1), 0x12: ('FOR_STEP', 1), 0x13: ('NEXT', 1), 0x14: ('NEXT_VAR', 1),
    0x15: ('CLR', 1), 0x16: ('INIT_RET', 1), 0x17: ('POKE', 1), 0x18: ('SYS', 1),
    0x19: ('GOTO', 3), 0x1A: ('GOSUB', 3), 0x1B: ('ON_GOTO', 'b1'), 0x1C: ('ON_GOSUB', 'b1'),
    0x1D: ('RETURN', 1), 0x1E: ('RESTORE', 1), 0x1F: ('IF_NOT', 2),
    0x37: ('DEF_FN', 'b5'), 0x38: ('FN', 3), 0x39: ('FN_END', 1), 0x3A: ('NOP', 1),
    0x3B: ('PRINT_COMMA', 1), 0x3C: ('PRINT;', 1), 0x3D: ('PRINT,', 1), 0x3E: ('PRINT', 1),
    0x3F: ('PRINT_CR', 1), 0x40: ('SPC(', 1), 0x41: ('TAB(', 1), 0x42: ('OUTPUT#', 1),
    0x43: ('PRINT#_CR', 1), 0x44: ('PRINT#_EMPTY', 1), 0x45: ('CLRCHN', 1),
    0x46: ('GET_46', 1), 0x47: ('GET', 1), 0x48: ('INPUT#', 1), 0x49: ('STOP', 1),
    0x4A: ('READ$', 1), 0x4B: ('READ', 1), 0x4C: ('WAIT', 1), 0x4D: ('WAIT3', 1),
    0x4E: ('NEW', 1), 0x4F: ('END', 1), 0x50: ('INPUT_START', 1),
    0x59: ('FLAG_SET40', 1), 0x5A: ('FLAG_CLR40', 1), 0x5B: ('FLAG_SET80', 1), 0x5C: ('FLAG_CLR80', 1),
    0x5D: ('LOAD', 2), 0x5E: ('SAVE', 2), 0x5F: ('VERIFY', 2), 0x60: ('OPEN', 2), 0x61: ('CLOSE', 1),
}
for _k, _v in BINOPS.items(): OPS[_k] = (_v, 1)
for _i, _f in enumerate(FUNCS): OPS[0x20 + _i] = (_f, 1)
for _k in range(0x51, 0x59): OPS.setdefault(_k, (f'INPUT_{_k:02X}', 1))


def varname(b0, b1):
    c0 = chr(b0 & 0x7F)
    c1 = chr(b1 & 0x7F) if b1 & 0x7F else ''
    if not (c0.isalnum() and (c1 == '' or c1.isalnum())):
        return None
    s = c0 + c1
    if b0 & 0x80 and b1 & 0x80: return s + '%'
    if b1 & 0x80: return s + '$'
    if b0 & 0x80: return 'FN' + s
    return s


class Prog:
    def __init__(self, prg):
        self.base = prg[0] | prg[1] << 8
        self.mem = prg[2:]
        w = lambda a: self.m(a) | self.m(a + 1) << 8
        self.vartab, self.arytab = w(0x1784), w(0x1786)
        self.vars = {}
        a, i = self.vartab, 0
        while a + 7 <= self.arytab:
            self.vars[i] = varname(self.m(a), self.m(a + 1)) or f'v{i}'
            a += 7; i += 1
        self.arrays = {}

    def m(self, a): return self.mem[a - self.base]

    def scan_dims(self, a, end):
        while a <= end:
            if self.m(a) == 0x10:
                off = self.m(a + 4) * 256 + self.m(a + 5)
                self.arrays[(off - 1) // 2] = (varname(self.m(a + 2), self.m(a + 3)) or f'?{self.m(a + 2):02X}{self.m(a + 3):02X}') + '()'
            a += self.decode(a)[0]

    def vn(self, i): return self.vars.get(i, f'v{i}')
    def an(self, i): return self.arrays.get(i, f'a{i}()')

    def decode(self, a):
        b = self.m(a)
        nxt = lambda i: self.m(a + i)
        if b >= 0x80:
            lo = b & 0x3F
            store = b & 0x40
            if b == 0xE6: return 1, 'STORE'
            if b == 0xE7:
                n = nxt(1); return 2 + n, 'STR ' + pet(bytes(self.m(a + 2 + i) for i in range(n)))
            if 0xE8 <= b <= 0xEF:
                n = b & 7; return 1 + n, 'STR ' + pet(bytes(self.m(a + 1 + i) for i in range(n)))
            if b >= 0xF0: return 1, f'INT {16 + (b & 0x0F)}'
            pre = '-> ' if store else ''
            if lo < 0x20: return 1, f'{pre}VAR {self.vn(lo)}'
            if lo <= 0x22: return 2, f'{pre}VAR {self.vn((lo & 3) * 256 + nxt(1))}'
            if lo == 0x23: return 3, f'{pre}VAR {self.vn(nxt(1) * 256 + nxt(2))}'
            if lo == 0x24: return 2, f'{pre}ARR {self.an(nxt(1))}'
            if lo == 0x25: return 3, f'{pre}ARR {self.an(nxt(1) * 256 + nxt(2))}'
            if lo == 0x26: return 2, f'INT {nxt(1)}'
            if lo == 0x27: return 3, f'INT {nxt(1) * 256 + nxt(2)}'
            if lo in (0x28, 0x29): return 6, 'FLT ' + fmt_float(bytes(nxt(i) for i in range(1, 6)))
            if lo == 0x2A: return 1, 'PI'
            if lo == 0x2B: return 1, 'ST'
            if lo == 0x2C: return 1, 'TI'
            if lo in (0x2E, 0x2F): return 1, 'TI$'
            if lo >= 0x30: return 1, f'INT {lo & 0x0F}'
            return 1, f'??{b:02X}'
        if b not in OPS:
            return 1, f'??OP {b:02X}'
        name, ln = OPS[b]
        if ln == 'b1':
            n = (nxt(1) - 2) // 2
            return nxt(1), f'{name} ' + ', '.join(f'${nxt(2 + 2 * k) * 256 + nxt(3 + 2 * k):04X}' for k in range(n))
        if ln == 'b5':
            return nxt(5), f'{name} ' + ' '.join('%02X' % nxt(i) for i in range(1, 6))
        if name in ('GOTO', 'GOSUB', 'FN'):
            return 3, f'{name} ${nxt(1) * 256 + nxt(2):04X}'
        if name == 'IF_NOT':
            return 2, f'IF_NOT else ${a + nxt(1):04X}'
        if name == 'DIM':
            nm = varname(nxt(2), nxt(3)) or f'?{nxt(2):02X}{nxt(3):02X}'
            return 6, f'DIM {nm}  ({nxt(1)} dim, slot {nxt(4) * 256 + nxt(5)})'
        if ln == 2:
            return 2, f'{name} nparam={nxt(1)}'
        return ln, name


def fmt_float(f):
    """Formato FP Commodore a 5 byte -> stringa decimale."""
    if f[0] == 0: return '0'
    sign = -1 if f[1] & 0x80 else 1
    mant = ((f[1] | 0x80) << 24 | f[2] << 16 | f[3] << 8 | f[4]) / float(1 << 32)
    v = sign * mant * 2.0 ** (f[0] - 128)
    return repr(int(v)) if v == int(v) else f'{v:.9g}'


def pet(bs):
    """PETSCII (charset maiuscolo/minuscolo) -> testo leggibile; controlli tra {}."""
    s = ''
    for c in bs:
        if 0x41 <= c <= 0x5A: s += chr(c).lower()
        elif 0xC1 <= c <= 0xDA: s += chr(c - 0x80)
        elif c == 0x5C: s += '£'
        elif 0x20 <= c < 0x5F and c != 0x22: s += chr(c)
        else: s += '{%02X}' % c
    return '"' + s + '"'


def main():
    prg = open(sys.argv[1], 'rb').read()
    p = Prog(prg)
    arg = lambda k, d: int(sys.argv[sys.argv.index(k) + 1].lstrip('$'), 16) if k in sys.argv else d
    a, end = arg('--from', 0x1AC4), arg('--to', 0x7DAB)
    p.scan_dims(0x1790, 0x179E)
    p.scan_dims(a, end)
    # destinazioni di salto -> etichette (sono gli inizi delle righe BASIC originali)
    targets = set()
    x = a
    while x <= end:
        ln, txt = p.decode(x)
        for tok in txt.replace(',', ' ').split():
            if tok.startswith('$') and len(tok) == 5:
                targets.add(int(tok[1:], 16))
        x += ln
    out, unknown = [], 0
    while a <= end:
        ln, txt = p.decode(a)
        if '??' in txt: unknown += 1
        if a in targets: out.append(f'\nL{a:04X}:')
        raw = ' '.join('%02X' % p.m(a + i) for i in range(min(ln, 6))) + (' ..' if ln > 6 else '')
        out.append(f'  {a:04X}  {raw:<20} {txt}')
        a += ln
    open(sys.argv[2], 'w', encoding='utf-8').write('\n'.join(out) + '\n')
    print(f'{len(out)} righe, {unknown} sconosciute, fine a ${a:04X}, {len(targets)} destinazioni di salto, '
          f'{len(p.vars)} variabili, {len(p.arrays)} array')


if __name__ == '__main__':
    main()
