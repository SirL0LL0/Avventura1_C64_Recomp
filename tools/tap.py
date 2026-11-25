"""Estrattore TAP per "Avventura 1 (fantasy)" - C64.

Decodifica un file .tap (C64-TAPE-RAW v0/v1):
  * blocchi Kernal ROM standard (header + dati, doppia copia, checksum XOR)
  * blocchi Turbo Tape 64 (pilot $02, sync $09..$01, bit MSB-first, checksum XOR)

Uso:
    python -I tools/tap.py <file.tap> <cartella_output>

Produce in <cartella_output>:
    kernal_NN_<start>.bin   dati di ogni file Kernal (senza indirizzo)
    kernal_NN_hdr.bin       blocco header (192 byte, come in $033C)
    turbo_NN_<start>.prg    ogni file turbo come PRG (indirizzo di caricamento + dati)
    manifest.txt            riassunto
Nessun dato originale e' incluso nel progetto: tutto viene ricavato dalla copia dell'utente.
"""
import os
import sys

# soglie (valori TAP = cicli/8)
K_SHORT_MAX = 0x37   # Kernal: S < 0x37 <= M < 0x4C <= L < 0x70
K_MED_MAX = 0x4C
K_LONG_MAX = 0x70
T_MIN, T_MAX, T_THRESH = 0x10, 0x2E, 0x21   # Turbo Tape: 0 ~ 0x1A, 1 ~ 0x28


def read_pulses(path):
    d = open(path, 'rb').read()
    if d[:12] != b'C64-TAPE-RAW':
        raise SystemExit("non e' un file TAP C64")
    ver = d[12]
    data = d[20:]
    pulses = []
    i = 0
    while i < len(data):
        b = data[i]
        if b == 0:
            if ver == 0:
                pulses.append(0x100); i += 1
            else:
                pulses.append((data[i + 1] | data[i + 2] << 8 | data[i + 3] << 16) // 8); i += 4
        else:
            pulses.append(b); i += 1
    return pulses


def kernal_blocks(pulses):
    """Ritorna lista di (indice_impulso, bytes) dei blocchi Kernal decodificati."""
    def cls(p):
        if p < K_SHORT_MAX: return 'S'
        if p < K_MED_MAX: return 'M'
        if p < K_LONG_MAX: return 'L'
        return 'X'
    seq = ''.join(cls(p) for p in pulses)
    blocks, cur, j = [], [], 0
    while j < len(seq) - 20:
        if seq[j] == 'L' and seq[j + 1] == 'M':
            val, ok, par = 0, True, 1
            for k in range(9):
                pr = seq[j + 2 + 2 * k: j + 4 + 2 * k]
                if pr == 'SM': bit = 0
                elif pr == 'MS': bit = 1
                else: ok = False; break
                if k < 8:
                    val |= bit << k; par ^= bit
                elif bit != par:
                    ok = False
            if ok:
                cur.append(val); j += 20; continue
        if len(cur) > 8:
            blocks.append((j, bytes(cur)))
        cur = []
        j += 1
    if len(cur) > 8:
        blocks.append((j, bytes(cur)))
    return blocks


def kernal_files(blocks):
    """Accoppia header/dati (prima copia valida), verifica il checksum."""
    out = []
    pending_hdr = None
    for pos, raw in blocks:
        cd = raw[0]
        payload, chk = raw[9:-1], raw[-1]
        x = 0
        for b in payload: x ^= b
        good = (x == chk)
        second = cd < 0x80          # la seconda copia ha countdown $09..$01
        if second:
            continue                # usiamo la prima copia (verificata)
        if not good:
            print(f"ATTENZIONE: checksum Kernal errato a impulso {pos}")
        if payload and payload[0] in (1, 3) and len(payload) == 192:
            pending_hdr = payload
        elif pending_hdr is not None:
            start = pending_hdr[1] | pending_hdr[2] << 8
            end = pending_hdr[3] | pending_hdr[4] << 8
            out.append(dict(hdr=pending_hdr, start=start, end=end, data=payload[:end - start],
                            name=pending_hdr[5:21].decode('latin1'), ok=good))
            pending_hdr = None
    return out


def turbo_files(pulses):
    """Decodifica i file Turbo Tape 64 (header + dati)."""
    SYNC = [9, 8, 7, 6, 5, 4, 3, 2, 1]
    L = len(pulses)

    def isT(p): return T_MIN <= p <= T_MAX

    def read_byte(n):
        v = 0
        for k in range(8):
            if n + k >= L or not isT(pulses[n + k]): return None
            v = (v << 1) | (1 if pulses[n + k] > T_THRESH else 0)
        return v

    def find_sync(n):
        sr = 0
        while n < L:
            p = pulses[n]; n += 1
            if not isT(p): sr = 0; continue
            sr = ((sr << 1) | (1 if p > T_THRESH else 0)) & 0xFF
            if sr == 9:
                m, ok = n, True
                for want in SYNC[1:]:
                    v = read_byte(m)
                    if v != want: ok = False; break
                    m += 8
                if ok:
                    return m
        return None

    files = []
    n = 0
    while True:
        n = find_sync(n)
        if n is None: break
        typ = read_byte(n)
        if typ not in (1, 2):         # aspetta un header
            n += 8; continue
        hb = []
        m = n + 8
        for _ in range(4):
            hb.append(read_byte(m)); m += 8
        start = hb[0] | hb[1] << 8
        end = hb[2] | hb[3] << 8
        n = find_sync(m)
        if n is None: break
        if read_byte(n) != 0:
            print("ATTENZIONE: blocco dati turbo inatteso"); continue
        n += 8
        body = []
        for _ in range(end - start):
            body.append(read_byte(n)); n += 8
        chk = read_byte(n); n += 8
        x = 0
        for b in body: x ^= b
        files.append(dict(start=start, end=end, data=bytes(body), ok=(x == chk)))
    return files


def main():
    if len(sys.argv) != 3:
        print(__doc__); sys.exit(1)
    tap, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    pulses = read_pulses(tap)
    man = []
    for i, f in enumerate(kernal_files(kernal_blocks(pulses))):
        open(os.path.join(out, f"kernal_{i:02d}_hdr.bin"), 'wb').write(f['hdr'])
        open(os.path.join(out, f"kernal_{i:02d}_{f['start']:04x}.bin"), 'wb').write(f['data'])
        man.append(f"kernal {i:02d}  ${f['start']:04X}-${f['end']:04X}  {'OK' if f['ok'] else 'ERRORE'}  nome={f['name']!r}")
    for i, f in enumerate(turbo_files(pulses)):
        open(os.path.join(out, f"turbo_{i:02d}_{f['start']:04x}.prg"), 'wb').write(
            bytes([f['start'] & 255, f['start'] >> 8]) + f['data'])
        man.append(f"turbo  {i:02d}  ${f['start']:04X}-${f['end']:04X}  {'OK' if f['ok'] else 'ERRORE'}")
    open(os.path.join(out, 'manifest.txt'), 'w').write("\n".join(man) + "\n")
    print("\n".join(man))


if __name__ == '__main__':
    main()
