"""Strumento di esplorazione: traccia il codice da uno o piu' entry point e stampa
coverage, chiamate a routine e target indiretti. Uso:
    python -I tools/explore.py build/avventura1.prg 0B41 [altri...]
"""
import sys, os, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from opcodes import OPS, SIZE, BRANCHES, STOPS

prg = open(sys.argv[1], 'rb').read()
BASE = prg[0] | prg[1] << 8
MEM = prg[2:]
END = BASE + len(MEM)
entries = [int(x, 16) for x in sys.argv[2:]]

code = {}
calls = collections.Counter()
refs = collections.defaultdict(set)
todo = list(entries)
bad = []
while todo:
    pc = todo.pop()
    while BASE <= pc < END and pc not in code:
        op = MEM[pc - BASE]
        if op not in OPS:
            bad.append(pc); break
        n, m = OPS[op]; s = SIZE[m]
        a = MEM[pc - BASE + 1: pc - BASE + s]
        v = a[0] if s == 2 else (a[0] | a[1] << 8 if s == 3 else None)
        code[pc] = (n, m, v, s)
        nxt = pc + s
        if m == 'rel':
            t = (nxt + (v - 256 if v > 127 else v)) & 0xFFFF
            todo.append(t)
        elif n == 'JSR':
            calls[v] += 1
            todo.append(v)
        elif n == 'JMP' and m == 'abs':
            todo.append(v); break
        elif n == 'JMP':
            print(f"JMP indiretto a {pc:04X} via ({v:04X})"); break
        elif n in STOPS:
            break
        if m in ('abs', 'abx', 'aby') and v is not None:
            refs[v].add(pc)
        pc = nxt

cov = sorted(code)
print("istruzioni:", len(code), " byte:", sum(code[p][3] for p in code))
# range contigui
rng = []
for p in cov:
    e = p + code[p][3]
    if rng and rng[-1][1] == p: rng[-1][1] = e
    else: rng.append([p, e])
print("range codice:")
for a, b in rng: print(f"  ${a:04X}-${b - 1:04X}")
print("opcode illegali raggiunti:", [hex(x) for x in bad])
print("routine piu' chiamate:")
for t, c in calls.most_common(40): print(f"  ${t:04X} x{c}")
ext = sorted(t for t in refs if not (BASE <= t < END))
print("riferimenti esterni:", " ".join(f"{t:04X}" for t in ext))
