# Avventura 1 (fantasy) – C64: progetto di reverse engineering

Strumenti per analizzare, disassemblare e (in futuro) modificare il gioco per Commodore 64
*Avventura 1 (fantasy)*.

**Nessun dato del gioco originale è incluso o distribuito.** Ogni utente fornisce la propria copia `.tap`.
Gli strumenti ricavano tutto in `build/`, che è escluso dal versionamento. Il repository contiene
solo codice e annotazioni nostri.

## Uso

Requisiti: Python 3, senza dipendenze esterne.

```powershell
python -I tools\build.py "originali\avventura-1-(fantasy)-versione-commerciale.tap"
```

Il comando produce in `build/`:

| File | Contenuto |
|------|-----------|
| `extract/` | file estratti dal nastro (Kernal e Turbo Tape, checksum verificati) |
| `avventura1.prg` | immagine di memoria dopo il boot (de-offuscata) |
| `avventura1.asm` | disassemblato annotato del runtime e dei dati (sintassi 64tass) |
| `rebuilt.prg` | riassemblato da `avventura1.asm`, **verificato identico byte per byte** |
| `pcode.txt` | listato del programma BASIC compilato, con i nomi originali delle variabili |

## Struttura

```
tools/
  tap.py         decodifica TAP: Kernal ROM e Turbo Tape 64
  make_image.py  replica la sequenza di boot (de-offuscamento +1, entry $0B41)
  opcodes.py     tabella opcode 6502
  disasm.py      disassembler ricorsivo guidato dal file annotazioni
  asm6502.py     mini-assembler (sottoinsieme 64tass) per il controllo del round-trip
  pcode.py       decodificatore del P-code
  explore.py     traccia veloce del codice raggiungibile (esplorazione)
  build.py       pipeline completa
annotations/
  avventura1.ann etichette, commenti e regioni (solo metadati nostri)
docs/
  PCODE.md       specifica del formato P-code e mappa di memoria
```

## Risultati dell'analisi

* **Nastro**: boot Kernal → schermata → turbo loader a `$C9E8` → programma in Turbo Tape (`$0801-$80F4`).
  Sul nastro i byte sono offuscati con −1.
* **Non è assembly scritto a mano**: è un **programma BASIC compilato in P-code**, con un runtime
  di circa 3,7 KB che usa la ROM BASIC.
* Il P-code (circa 25 KB) si decodifica **al 100%**. Contiene 120 variabili con i nomi originali
  (`TK`, `BT`, `P$`, `NP$`, …), 3 array, 48 voci `DATA` (nomi dei mostri), 905 stringhe di testo,
  237 `GOSUB` e 224 `GOTO`.
* Il gioco salva e carica il personaggio su un file (`OPEN`/`PRINT#`).

Dettagli in [docs/PCODE.md](docs/PCODE.md).

## Note legali

Il gioco è un'opera commerciale protetta da copyright. Questo progetto contiene solo strumenti
originali e metadati descrittivi. File generati come `build/*.asm` o `build/pcode.txt` contengono dati
derivati dall'originale e **non vanno distribuiti**.
