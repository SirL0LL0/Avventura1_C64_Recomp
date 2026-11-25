"""Pipeline completa: dalla copia .tap dell'utente ai sorgenti di lavoro in build/.

Uso (dalla cartella del progetto):
    python -I tools/build.py "originali/avventura-1-(fantasy)-versione-commerciale.tap"

Passi:
  1. tap.py         estrae i file Kernal e Turbo Tape          -> build/extract/
  2. make_image.py  ricostruisce la memoria dopo il boot        -> build/avventura1.prg
  3. disasm.py      disassembla il runtime con le annotazioni   -> build/avventura1.asm
  4. asm6502.py     riassembla e verifica identita' byte a byte -> build/rebuilt.prg
  5. pcode.py       decodifica il P-code con i nomi variabili   -> build/pcode.txt
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
T = lambda *p: os.path.join(ROOT, 'tools', *p)
B = lambda *p: os.path.join(ROOT, 'build', *p)


def run(*args):
    print('>', ' '.join(os.path.basename(a) if os.path.isabs(a) else a for a in args))
    r = subprocess.run([sys.executable, '-I', *args])
    if r.returncode:
        sys.exit(r.returncode)


def main():
    if len(sys.argv) != 2:
        print(__doc__); sys.exit(1)
    os.makedirs(B(), exist_ok=True)
    run(T('tap.py'), sys.argv[1], B('extract'))
    run(T('make_image.py'), B('extract'), B('avventura1.prg'))
    run(T('disasm.py'), B('avventura1.prg'), os.path.join(ROOT, 'annotations', 'avventura1.ann'), B('avventura1.asm'))
    run(T('asm6502.py'), B('avventura1.asm'), B('rebuilt.prg'), '--compare', B('avventura1.prg'))
    run(T('pcode.py'), B('avventura1.prg'), B('pcode.txt'))
    print('\nFatto. File di lavoro in build/ (non distribuire: contengono dati originali).')


if __name__ == '__main__':
    main()
