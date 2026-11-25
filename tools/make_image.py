"""Ricostruisce l'immagine di memoria del gioco come appare dopo il caricamento.

Replica la sequenza di boot originale:
  1. il turbo loader carica il programma principale in $0801-$80F4
  2. la routine a $07E8 somma 1 a ogni byte da $0803 a $A002 (de-offuscamento)
  3. un file Kernal di 2 byte ($0000) viene caricato in $0801
  4. il boot salta a JMP ($0351) -> vettore dall'ultimo header = $0B41

Uso:
    python -I tools/make_image.py <cartella_estratta> <output.prg>
Output: PRG decodificato ($0801-$80F4) pronto per il disassembler.
"""
import glob
import os
import sys


def main():
    src, out = sys.argv[1], sys.argv[2]
    turbo = sorted(glob.glob(os.path.join(src, 'turbo_*_0801.prg')))
    if not turbo:
        raise SystemExit("file turbo $0801 non trovato: esegui prima tools/tap.py")
    prg = bytearray(open(turbo[0], 'rb').read())
    body = prg[2:]
    for i in range(2, len(body)):          # $0803 in poi
        body[i] = (body[i] + 1) & 0xFF
    tiny = glob.glob(os.path.join(src, 'kernal_*_0801.bin'))
    if tiny:
        t = open(tiny[0], 'rb').read()
        body[0:len(t)] = t
    # vettore di partenza dall'ultimo header Kernal
    hdrs = sorted(glob.glob(os.path.join(src, 'kernal_*_hdr.bin')))
    h = open(hdrs[-1], 'rb').read()
    entry = h[21] | h[22] << 8
    open(out, 'wb').write(bytes(prg[:2]) + bytes(body))
    print(f"immagine scritta: {out}  (${0x801:04X}-${0x801 + len(body) - 1:04X}), entry=${entry:04X}")


if __name__ == '__main__':
    main()
