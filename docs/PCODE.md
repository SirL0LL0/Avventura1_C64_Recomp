# Formato del P-code di "Avventura 1"

Il gioco è un programma **BASIC V2 compilato** in P-code (bytecode) ed eseguito da un piccolo
runtime in linguaggio macchina (circa 3,7 KB, `$081F-$1783`). Il compilatore non è ancora identificato
(lo stile ricorda Austro-Speed/Blitz!). Il runtime usa direttamente le routine della ROM BASIC
per virgola mobile, stringhe e I/O.

Tutto quanto segue è stato ricavato dal disassemblato e verificato decodificando l'intero programma
(`$1AC4-$7DAB`, 5.374 istruzioni, nessun opcode sconosciuto, fine esatta sul terminatore `$00`).

## Mappa di memoria

| Indirizzo     | Contenuto |
|---------------|-----------|
| `$0801-$081E` | stub BASIC azzerato (era la riga `SYS`) |
| `$081F`       | `JMP $0CB5` (ingresso del runtime) |
| `$0822-$08E7` | tabella di dispatch degli opcode `$0E-$70` (`$0806 + 2*op`) |
| `$08E8-$08F9` | potenze di 10 e vettori delle operazioni FP |
| `$08FA-$1783` | runtime (VM, operatori, gestori delle istruzioni) |
| `$1784-$179E` | header: `VARTAB`, `ARYTAB`, `STREND`, PC di init, `DATPTR`, PC iniziale |
| `$1790-$179E` | P-code di init (eseguito da `CLR`: i `DIM` globali) |
| `$179F-$1AC3` | valori delle istruzioni `DATA`: 48 stringhe con prefisso di lunghezza, terminate da `$FF` |
| `$1AC4-$7DAB` | P-code del programma (terminatore `$00` a `$7DAC`) |
| `$7DAD-$80F4` | area variabili: 120 record da 7 byte (nome di 2 byte e valore di 5) |

## Boot (dal nastro)

1. Il boot Kernal a `$02B0` parte in automatico (vettore `IMAIN` `$0302` sovrascritto).
2. Carica la schermata (`$0400`), poi il turbo loader (`$C9E8`), che carica il programma in Turbo Tape.
3. `$07E8` (nascosto nel blocco della schermata) **somma 1 a ogni byte** da `$0803` a `$A002`.
   È l'offuscamento dei dati sul nastro.
4. Un file di 2 byte scrive `$0000` in `$0801`, viene ripristinato `IMAIN` e il boot esegue `JMP ($0351)`.
   L'ultimo header nel buffer della cassetta contiene il vettore `$0B41`.

## VM

* PC del P-code in `$39/$3A`, opcode corrente in `$3C`, dispatch con `JMP ($0055)`.
* È una macchina a stack: gli operandi vengono messi sullo stack del 6502 e l'accumulatore è
  il FAC della ROM (`$61-$66`) oppure un intero a 16 bit (`$64/$65`). Il tipo è in `$0E`
  (`$80` = intero, `$00` = float, `$01` = stringa).
* Le espressioni sono in **notazione postfissa**: `INT 21, INT 0, RND, *, INT, INT 1, +` = `INT(RND(0)*21)+1`.

### Opcode `$80-$FF`: operandi

| Byte | Lunghezza | Significato |
|------|-----------|-------------|
| `$80-$9F` | 1 | legge la variabile `n` (0-31) |
| `$A0-$A2` | 2 | legge la variabile `(b&3)*256 + byte1` |
| `$A3` | 3 | legge la variabile `byte1*256 + byte2` |
| `$A4`/`$A5` | 2/3 | elemento di array (indici presi dallo stack) |
| `$A6` | 2 | costante intera a 8 bit |
| `$A7` | 3 | costante intera a 16 bit (big-endian) |
| `$A8`/`$A9` | 6 | costante float (5 byte, formato FP Commodore) |
| `$AA`/`$AB`/`$AC`/`$AE`/`$AF` | 1 | `π`, `ST`, `TI`, `TI$` |
| `$B0-$BF` | 1 | costante intera 0-15 |
| `$C0-$E5` | come sopra | **assegnazione**: come `$80-$A5`, ma scrive la variabile o l'array |
| `$E6` | 1 | assegnazione esplicita |
| `$E7` | 2+n | stringa costante, lunghezza nel byte 1 |
| `$E8-$EF` | 1+n | stringa costante corta, n = b&7 |
| `$F0-$FF` | 1 | costante intera 16-31 |

L'indirizzo della variabile `n` è `VARTAB + 7*n + 2`.

### Opcode `$01-$7F`: operatori e istruzioni

| Op | Lungh. | Istruzione | | Op | Lungh. | Istruzione |
|----|---|---|---|----|---|---|
| `$01-$06` | 1 | `> = >= < <> <=` | | `$3A` | 1 | NOP |
| `$07-$0B` | 1 | `+ - * / ^` | | `$3B` | 1 | `,` in PRINT (tabulazione) |
| `$0C/$0D` | 1 | `AND` / `OR` | | `$3C/$3D/$3E` | 1 | PRINT valore con `;` / `,` / a capo |
| `$0E/$0F` | 1 | meno unario / `NOT` | | `$3F` | 1 | PRINT vuoto (a capo) |
| `$10` | 6 | `DIM` [ndim][nome][slot] | | `$40/$41` | 1 | `SPC(` / `TAB(` |
| `$11/$12` | 1 | `FOR` / `FOR..STEP` | | `$42-$45` | 1 | `PRINT#` (CHKOUT, a capo, CLRCHN) |
| `$13/$14` | 1 | `NEXT` / `NEXT var` | | `$46/$47` | 1 | `GET` |
| `$15` | 1 | `CLR` (esegue l'init a `$1790`) | | `$48` | 1 | `INPUT#`/`GET#` (CHKIN) |
| `$16` | 1 | fine init | | `$49` | 1 | `STOP` |
| `$17` | 1 | `POKE` | | `$4A/$4B` | 1 | `READ` stringa / numero |
| `$18` | 1 | `SYS` | | `$4C/$4D` | 1 | `WAIT` (2/3 argomenti) |
| `$19` | 3 | `GOTO` hi lo | | `$4E/$4F` | 1 | `NEW` / `END` |
| `$1A` | 3 | `GOSUB` hi lo | | `$50-$58` | 1 | `INPUT` (varianti) |
| `$1B/$1C` | n | `ON..GOTO`/`ON..GOSUB` [n][indirizzi] | | `$59-$5C` | 1 | flag del runtime (STOP, INPUT) |
| `$1D` | 1 | `RETURN` | | `$5D-$5F` | 2 | `LOAD`/`SAVE`/`VERIFY` [nparam] |
| `$1E` | 1 | `RESTORE` | | `$60` | 2 | `OPEN` [nparam] |
| `$1F` | 2 | `IF` falso: salta di [byte1] | | `$61` | 1 | `CLOSE` |
| `$20-$36` | 1 | `SGN INT ABS USR FRE POS SQR RND LOG EXP COS SIN TAN ATN PEEK LEN STR$ VAL ASC CHR$ LEFT$ RIGHT$ MID$` | | `$62-$70` | - | non validi (SYNTAX ERROR) |
| `$37` | n | `DEF FN` | | `$00` | - | passa all'interprete ROM (qui solo terminatore) |
| `$38/$39` | 3/1 | chiamata `FN` / fine corpo FN | | | | |

Gli indirizzi di salto sono assoluti. Le 466 destinazioni corrispondono agli inizi delle righe
BASIC originali, ma i numeri di riga non sono conservati.

## Testo

Le stringhe usano il set di caratteri maiuscolo/minuscolo del C64: `$41-$5A` sono le minuscole,
`$C1-$DA` le maiuscole e `$5C` è `£`. I codici di controllo (colori, cursore, `{93}` = CLR)
sono indicati tra graffe nel listato.
