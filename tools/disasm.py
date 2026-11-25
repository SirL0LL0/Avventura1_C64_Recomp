"""Disassembler annotato per C64 (output sintassi 64tass).

Uso:
    python -I tools/disasm.py <immagine.prg> <annotazioni.ann> <output.asm>

Il file annotazioni (nostro, versionato) descrive SOLO metadati:
    entry   $0B41                 punto d'ingresso per la traccia ricorsiva
    label   $0B41 nome            etichetta
    comment $0B41 testo           riga di commento sopra l'indirizzo
    note    $0B41 testo           commento a fine riga
    code    $A-$B                 forza regione codice (tracciata da $A)
    bytes   $A-$B [N]             dati grezzi (N per riga, default 16)
    words   $A-$B [code]          tabella di puntatori (.word); 'code' = traccia i target
    pstr    $A-$B                 stringhe con prefisso lunghezza
    text    $A-$B                 testo PETSCII grezzo
    equ     nome $ADDR            simbolo esterno (ROM, I/O, zero page)
Regioni non coperte e non raggiunte dalla traccia diventano .byte.
L'.asm generato contiene dati originali: va in build/ e non si distribuisce.
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from opcodes import OPS, SIZE, STOPS


def num(s):
    s = s.strip()
    if s.startswith('$'): return int(s[1:], 16)
    if s.lower().startswith('0x'): return int(s, 16)
    return int(s)


class Ann:
    KINDS = ('code', 'bytes', 'words', 'pstr', 'text')

    def __init__(self, path):
        self.entries, self.labels, self.comments, self.notes = [], {}, {}, {}
        self.regions, self.equ = [], {}
        for ln, raw in enumerate(open(path, encoding='utf-8'), 1):
            line = raw.rstrip('\n')
            if not line.strip() or line.lstrip().startswith('#'): continue
            parts = line.split(None, 2)
            k = parts[0].lower()
            try:
                if k == 'entry':
                    for p in parts[1:]:
                        for x in p.split(): self.entries.append(num(x))
                elif k == 'label':
                    self.labels[num(parts[1])] = parts[2].strip()
                elif k == 'comment':
                    self.comments.setdefault(num(parts[1]), []).append(parts[2] if len(parts) > 2 else '')
                elif k == 'note':
                    self.notes[num(parts[1])] = parts[2]
                elif k == 'equ':
                    self.equ[num(parts[2])] = parts[1]
                elif k in self.KINDS:
                    a, b = parts[1].split('-')
                    extra = parts[2].strip() if len(parts) > 2 else ''
                    self.regions.append((num(a), num(b), k, extra))
                else:
                    raise ValueError(k)
            except Exception as e:
                raise SystemExit(f"{path}:{ln}: riga non valida ({e}): {line}")


class Dis:
    def __init__(self, prg, ann):
        self.base = prg[0] | prg[1] << 8
        self.mem = prg[2:]
        self.end = self.base + len(self.mem)
        self.ann = ann
        self.insn = {}          # addr -> (mn, mode, val, size)
        self.targets = {}       # addr -> 'sub'/'jmp'/'br'/'tbl'/'data'
        self.region_at = {}
        for r in ann.regions:
            for p in range(r[0], r[1] + 1): self.region_at[p] = r

    def byte(self, a): return self.mem[a - self.base]
    def word(self, a): return self.byte(a) | self.byte(a + 1) << 8
    def inside(self, a): return self.base <= a < self.end

    # ------------------------------------------------------------------ traccia
    def trace(self):
        todo = list(self.ann.entries)
        for a, b, k, x in self.ann.regions:
            if k == 'code': todo.append(a)
            if k == 'words':
                for p in range(a, b, 2):
                    t = self.word(p)
                    if self.inside(t):
                        self.targets.setdefault(t, 'tbl')
                        if 'code' in x: todo.append(t)
        while todo:
            pc = todo.pop()
            while self.inside(pc) and pc not in self.insn:
                r = self.region_at.get(pc)
                if r and r[2] != 'code': break
                op = self.byte(pc)
                if op not in OPS:
                    print(f"ATTENZIONE: opcode illegale ${op:02X} a ${pc:04X}"); break
                n, m = OPS[op]; s = SIZE[m]
                if pc + s > self.end: break
                v = self.byte(pc + 1) if s == 2 else (self.word(pc + 1) if s == 3 else None)
                self.insn[pc] = (n, m, v, s)
                nxt = pc + s
                if m == 'rel':
                    t = (nxt + (v - 256 if v > 127 else v)) & 0xFFFF
                    self.targets.setdefault(t, 'br'); todo.append(t)
                elif n == 'JSR':
                    self.targets[v] = 'sub'; todo.append(v)
                elif n == 'JMP' and m == 'abs':
                    self.targets.setdefault(v, 'jmp'); todo.append(v); break
                elif n == 'JMP':
                    self.targets.setdefault(v, 'data'); break
                elif n in STOPS:
                    break
                elif m not in ('imp', 'acc', 'imm') and v is not None:
                    self.targets.setdefault(v, 'data')
                pc = nxt

    # ------------------------------------------------------------------ nomi
    def name(self, a):
        if a in self.ann.labels: return self.ann.labels[a]
        if a in self.ann.equ: return self.ann.equ[a]
        if not self.inside(a): return None
        t = self.targets.get(a)
        if t == 'sub': return f"s{a:04X}"
        if t in ('jmp', 'br', 'tbl'): return f"l{a:04X}"
        if t == 'data': return f"d{a:04X}"
        return None

    def operand(self, pc, n, m, v, s):
        if m == 'imp': return ''
        if m == 'acc': return 'a'
        if m == 'imm': return f"#${v:02X}"
        if m == 'rel':
            t = (pc + 2 + (v - 256 if v > 127 else v)) & 0xFFFF
            return self.name(t) or f"${t:04X}"
        nm = self.name(v)
        if s == 2:
            txt = nm or f"${v:02X}"
        else:
            txt = nm or f"${v:04X}"
            if v < 0x100 and m in ('abs', 'abx', 'aby'):
                txt = "@w " + txt                    # forza indirizzamento assoluto
        return {'zp': txt, 'abs': txt, 'zpx': txt + ',x', 'abx': txt + ',x', 'zpy': txt + ',y',
                'aby': txt + ',y', 'izx': f"({txt},x)", 'izy': f"({txt}),y", 'ind': f"({txt})"}[m]

    # ------------------------------------------------------------------ output
    def emit(self, out):
        labels = {}
        for a in set(self.ann.labels) | set(self.targets):
            if self.inside(a):
                nm = self.name(a)
                if nm: labels[a] = nm
        recs = []            # (addr, testo) ; addr=None per righe senza indirizzo
        line_starts = []     # indirizzi di inizio riga emessa
        emitted = set()

        def put_label(a):
            for c in self.ann.comments.get(a, []):
                recs.append((None, f"; {c}" if c else ";"))
            if a in labels:
                recs.append((a, labels[a])); emitted.add(a)

        def put_line(a, text):
            line_starts.append(a)
            recs.append((a, text))

        pc = self.base
        while pc < self.end:
            r = self.region_at.get(pc)
            if pc in self.insn and (not r or r[2] == 'code'):
                put_label(pc)
                n, m, v, s = self.insn[pc]
                raw = ' '.join('%02X' % self.byte(pc + i) for i in range(s))
                line = f"        {n.lower()} {self.operand(pc, n, m, v, s)}".rstrip()
                note = self.ann.notes.get(pc, '')
                put_line(pc, f"{line:<40}; {pc:04X}  {raw:<9}" + (f" {note}" if note else '').rstrip())
                pc += s
                continue
            kind = r[2] if r and r[2] != 'code' else 'bytes'
            if r and r[2] != 'code':
                stop = r[1] + 1
            else:
                stop = pc + 1
                while stop < self.end and stop not in self.insn and stop not in self.region_at \
                        and stop not in labels and stop not in self.ann.comments and stop - pc < 16:
                    stop += 1
            if kind == 'words':
                while pc < stop:
                    put_label(pc)
                    w = self.word(pc)
                    expr = self.name(w) or f"${w:04X}"
                    note = self.ann.notes.get(pc, '')
                    put_line(pc, f"        .word {expr:<22}; {pc:04X}" + (f" {note}" if note else ''))
                    pc += 2
                continue
            if kind == 'pstr':
                while pc < stop:
                    put_label(pc)
                    ln = self.byte(pc)
                    chunk = bytes(self.byte(pc + 1 + i) for i in range(ln))
                    put_line(pc, f"        .byte {ln}" + (", " + petscii_expr(chunk) if ln else '') + f"   ; {pc:04X}")
                    pc += 1 + ln
                continue
            per = 48 if kind == 'text' else (int(r[3]) if r and r[3].isdigit() else 16)
            while pc < stop:
                put_label(pc)
                e = pc + 1
                while e < stop and e - pc < per and e not in labels and e not in self.ann.comments: e += 1
                chunk = bytes(self.byte(i) for i in range(pc, e))
                if kind == 'text':
                    put_line(pc, f"        .byte " + petscii_expr(chunk) + f"   ; {pc:04X}")
                else:
                    vals = ', '.join('$%02X' % c for c in chunk)
                    put_line(pc, f"        .byte {vals:<40}; {pc:04X}")
                pc = e

        # etichette che cadono dentro una riga (es. trucco BIT $2C): definite come base+offset
        import bisect
        line_starts.sort()
        inner = {}
        need_base = set()
        for a, nm in labels.items():
            if a in emitted: continue
            i = bisect.bisect_right(line_starts, a) - 1
            p = line_starts[i]
            inner[a] = (p, nm)
            need_base.add(p)
        final = []
        for a, text in recs:
            if a in need_base and a not in emitted and text != labels.get(a) and a in line_starts:
                if a not in labels:
                    labels[a] = f"l{a:04X}"
                final.append(labels[a]); emitted.add(a)
            final.append(text)
        hdr = ["; Avventura 1 (fantasy) - C64",
               "; Generato da tools/disasm.py - NON distribuire (contiene dati originali)",
               "; Sintassi: 64tass", ""]
        for a, nm in sorted(self.ann.equ.items()):
            hdr.append(f"{nm:<20} = ${a:04X}" if a > 0xFF else f"{nm:<20} = ${a:02X}")
        hdr += ["", f"* = ${self.base:04X}", ""]
        tail = ["", "; --- etichette interne a istruzioni/dati (ingressi sovrapposti) ---"]
        for a, (p, nm) in sorted(inner.items()):
            tail.append(f"{nm:<20} = {labels[p]}+{a - p}")
        open(out, 'w', encoding='utf-8').write("\n".join(hdr + final + tail) + "\n")


def petscii_expr(b):
    """Byte -> espressione leggibile: caratteri $20-$5F come stringa, il resto numerico."""
    parts, cur = [], ''
    for c in b:
        if 0x20 <= c <= 0x5F and c not in (0x22, 0x5C):
            cur += chr(c)
        else:
            if cur: parts.append('"' + cur + '"'); cur = ''
            parts.append('$%02X' % c)
    if cur: parts.append('"' + cur + '"')
    return ', '.join(parts)


def main():
    prg = open(sys.argv[1], 'rb').read()
    ann = Ann(sys.argv[2])
    d = Dis(prg, ann)
    d.trace()
    d.emit(sys.argv[3])
    nb = sum(v[3] for v in d.insn.values())
    print(f"scritto {sys.argv[3]}: {len(d.insn)} istruzioni, {nb} byte di codice")


if __name__ == '__main__':
    main()
