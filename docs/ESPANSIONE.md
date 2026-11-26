# Studio per l'espansione di Avventura 1

Branch di lavoro: `mod/espansione`. Il gioco originale resta invariato su `main`.

Questo documento descrive come funziona il gioco, ricavato dal listato decompilato (`build/avventura1.bas`,
che si genera dalla propria copia), e propone come ampliarlo **seguendo le regole che il gioco già usa**.
I numeri di riga sono quelli del listato decompilato.

---

## 0. Stato del lavoro (branch `mod/espansione`, solo PC)

| Parte | Stato |
|-------|-------|
| Audio: due SID emulati (6 voci in stereo), player, 8 brani originali, effetti sonori | fatto |
| Sistema di mod: modulo P-code separato, agganci alle righe originali, registri `$DE00` | fatto |
| Pannello con illustrazione, didascalia, scheda del personaggio e icone cliccabili dei consumabili | fatto |
| Ritratti: mostri delle cacce (11), luoghi, negozi, oggetti, morte, Eroe | fatto |
| Musica e immagini di scena in tutto il gioco originale | fatto |
| Armi comuni, armature (contano nel duello e nella mischia), Pane elfico (F3), Armaiolo | fatto |
| Salvataggi estesi e compatibili con quelli vecchi | fatto |
| Ritratti per gruppi di nemici, imboscate e personaggi (maga, guerriero, avventuriero, pellegrino) | fatto (40 in tutto) |
| Viaggi verso Kho e Kebir, con incontri e pirati: in taverna compaiono a caso (1 volta su 3) come offerta 3 al posto della Carovana | fatto |
| Kho: Tempio di LYSS (purificazione, benedizione dell'arma, offerte) e Guaritore | fatto |
| Kebir: Arena dei campioni (duello a premio) e Mercato delle spezie | fatto |
| Maledizioni di Vampiro e Licantropo registrate e curabili; l'arma benedetta le respinge | fatto |
| Mostri nuovi nelle cacce: Basilisco, Idra delle paludi, Spettro del Re | fatto |
| Missione: il Calice di LYSS (dal Tempio di Kho; trappola, enigma, Spettro guardiano) | fatto |
| Salvataggi MOD2: anche arma benedetta, maledizioni e missioni (legge MOD1 e originali) | fatto |
| Missione: scorta del pellegrino a Kho (dai Viaggi) | fatto |
| Catena del Conte Movas dal Barone Gudrund: spia, torre di guardia, duello finale | fatto |

### Compilare e provare la mod

```powershell
node tools\avv.js mod "originali\<tuo>.tap" mods\espansione mods\espansione.avm
Copy-Item mods\espansione.avm build\native\mods\
.\build\native\vmtest.exe "originali\<tuo>.tap" --mod mods\espansione.avm SPACE 2 1 --set DN=20000 SPACE SPACE 3 2 SPACE SPACE 4 6
```

Per test ripetibili: `set AVV_SEED=1` (il generatore casuale della VM parte sempre uguale).

Nel launcher, il pulsante **VERSIONE** passa da "Originale" alle mod trovate nella cartella `mods\` accanto all'eseguibile.

### Registri di espansione (`POKE 56832+n`)

| Indirizzo | Uso |
|-----------|-----|
| 56832 | musica: 1 titolo, 2 taverna, 3 viaggio, 4 battaglia, 5 città, 6 tempio, 7 eroe, 8 morte |
| 56833 | effetto: 1 colpo, 2 ferita, 3 mancato, 4 moneta, 5 pozione, 6 livello, 7 clic, 8 magia, 9 porta |
| 56834 | immagine: 1 mostra il soggetto scritto in `MP$` (per esempio `MP$=LA$`), 0 nasconde |

Le immagini si scelgono per parola chiave contenuta nel nome. Le illustrazioni (128×128, tavolozza del C64, con ombreggiature retinate) sono disegnate da codice in `tools/art/` (`pictures.js`, `parts.js`, `engine.js`). `node tools/art/build.js anteprima.png` rigenera `native/src/art_data.h` e un foglio d'anteprima. Ci sono anche le icone dei consumabili: nella scheda il Filtro (F1) e il Pane elfico (F3) si usano con un clic quando il gioco aspetta il loro tasto. La colonna sonora è fissa: 5 brani lenti (valle, focolare, sentiero, stelle, mare) si alternano e a ogni giro la melodia viene ricomposta da `generateSong` in `native/src/music.cpp` sugli stessi accordi. La partita non cambia più musica a ogni schermata: restano solo la fanfara (registro 7) e il tema della morte (8), in `native/src/songs.h`. I personaggi sono mezzibusti (`bust` in `tools/art/parts.js`).

---

## 1. Come funziona il gioco

### Personaggio

| Variabile | Significato | Note |
|-----------|-------------|------|
| `NP$` | nome | |
| `FO IN DE FR FA` | Forza, Intelligenza, Destrezza, Fortuna, Fascino | massimo 20 (riga 160) |
| `RO` / `RF` | Robustezza massima / attuale | `RF` = punti vita; a 0 si muore |
| `AT` | capacità di combattimento in % | parte da `FO*5`, cresce combattendo, +5 per ogni punto di Forza |
| `DN` | denaro (£) | |
| `TK` | settimane di avventura | |
| `EE` | quante caratteristiche sono a 20 | calcolato alla riga 160 |

- **Vittoria:** tutte e 6 le caratteristiche a 20 e `AT>200` diventi un Eroe (righe 470, 2660).
- **Prove:** la routine 20 tira un dado a 21 (`D6`). La prova riesce se la caratteristica è almeno pari al tiro (`IN>=D6`, `FR>=D6`, ...).
- **Creazione:** 3 personaggi pronti (righe 2720-2740) oppure casuale, con valori 6+2d6 e un nome a caso tra 6 (righe 330, 4310).

### Oggetti

- Gli oggetti sono stringhe in `OS$(1..WX)` (`DIM OS$(50)`). **Il tipo è la prima lettera del nome:**

| Lettera | Oggetto | Effetto |
|---------|---------|---------|
| `F` | Filtro magico | consumabile: con F1 riporta `RF` a `RO` (righe 240-300) |
| `T` | Talismano | +1 Fortuna, applicato all'acquisto |
| `A` | Amuleto | +1 Intelligenza, applicato all'acquisto |
| `S` | Spada magica | `KS=1`, +50/75/100 a `AT`; **una sola**; serve contro i Lupi Mannari/Licantropi |
| `x` | slot usato | il Filtro bevuto diventa `"x"` (lo slot non si libera: `WX` non cala) |

- I bonus si sommano una volta sola alle caratteristiche, non vengono ricalcolati dagli oggetti.
- Conseguenza importante: **un oggetto nuovo che inizia per F, T, A o S verrebbe contato come Filtro,
  Talismano, Amuleto o Spada** (righe 240 e 355-380).

### Struttura di una partita

```
Taverna del Gobbo (490)  ── ogni settimana ──┐
  * 1 * Caccia a un mostro      (760)   → viaggio, incontri, duello col mostro, taglia
  * 2 * Missione di recupero    (1660)  → 4 settimane, luogo (4 a caso), furtività, duello, ricompensa
  * 3 * Carovana per Babbakesch (850)   → biglietto £1000 oppure scorta (4 settimane di eventi)
           └─ Babbakesch (1220): Fabbro magico, Alchimista, Stregone
  * 4 * Mercenario del Barone Gudrund (2090) → 4 missioni settimanali da una tabella di 12
  * 5 * Riposo in taverna (690)  → £100 a settimana, +1 Robustezza a settimana
  * 9 * Salva su nastro (4390)
```

### Dati di testo

- Le 48 voci `DATA` sono **6 gruppi da 8**. La routine 320 legge 8 voci e ne sceglie una a caso:
  mostri, luoghi, frasi d'apertura, oggetti rubati, committenti, ricompense. Alla riga 610 c'è `RESTORE`.
- Sono tabelle nel codice, una `ON X GOTO` con 12 voci ciascuna:
  - gruppi di nemici (2850), con il flag `A$="1"` se si possono rendere amici;
  - mostri delle imboscate (3590);
  - bottino (2600);
  - missioni del Barone (2530-2650).

### Combattimento

- **Mischia contro gruppi (3150):** `NM` nemici con abilità `AM`. Il giocatore colpisce con probabilità `AT`.
  Le ferite vanno da 1 a 2 per colpo e a ogni turno si può fuggire (prova di Fortuna).
- **Duello col mostro (3950):** attacco `HA`, parata `HP`, vita `HR`, destrezza `HD`; inizia chi ha più Destrezza.
  Le regole speciali dipendono dall'**iniziale del mostro** (`LA$`):
  - `D` Drago: attacco a 95;
  - `G` Golem: molto resistente;
  - `V` Vampiro: toglie Forza;
  - `L` Licantropo: toglie Destrezza; senza Spada magica non si può ferire.
- I danni subiti nel duello (`HY`, 3-6 punti) non hanno nessuna riduzione: **oggi non esistono armature**.

### Salvataggio

`PRINT#` su nastro in ordine fisso: `NP$ TK FO IN DE FR RO RF FA AT DN WX` e poi gli oggetti (4390-4480).

---

## 2. Vincoli tecnici

| Vincolo | Valore | Conseguenza |
|---------|--------|-------------|
| Nomi di variabile | 2 caratteri significativi | **prefissi liberi: `M` e `U`** (es. `MA`, `MK$`, `UA%`); da usare per tutte le variabili nuove |
| Righe libere | 4670-5990, 6150-63999 | il codice nuovo va qui; nelle righe originali si mettono solo agganci (`GOSUB`) |
| Memoria su C64 vero | ~7,9 KB liberi (fino a `$9FFF`) | espansione piccola se vogliamo che giri anche sul C64 |
| Memoria nella VM per PC | ~32 KB liberi (fino a `$FFFF`) | espansione grande se il gioco è solo per PC |
| Riga con `IF` | il resto della riga deve stare in 255 byte di P-code | il compilatore segnala l'errore |
| Oggetti | massimo 50 (`OS$(50)`), slot usati non riciclati | serve una piccola manutenzione dell'inventario |
| `DATA` | letti in gruppi da 8 con `RESTORE` | le tabelle nuove vanno in array letti una volta all'avvio, oppure in `ON X GOTO` come fa l'originale |

### Come distribuire le modifiche senza distribuire il gioco

Il listato decompilato contiene il testo originale, quindi non si pubblica. Propongo un **file di mod**
(per esempio `mods/espansione.avm`) che contiene solo il nostro codice:

- **righe nuove** nelle zone libere, scritte da noi;
- **agganci**: istruzioni del tipo "all'inizio della riga 1230 inserisci `GOSUB 7000`" oppure
  "sostituisci la riga 1230 con: ...", dove la riga sostitutiva è scritta da noi.

Un comando (`node tools/avv.js mod build`) prende il listato della copia dell'utente, applica la mod,
compila e produce l'immagine del gioco modificato in `dati\`. Il launcher offre poi la scelta
"Originale" o "Espansione". Più avanti il parser e il compilatore si possono portare in C++, così la mod
si applica direttamente nell'app.

**Compatibilità dei salvataggi:** i dati nuovi si aggiungono **in fondo** al file, dopo gli oggetti.
Un salvataggio vecchio letto dall'espansione dà 0 per i campi mancanti (la VM restituisce un campo vuoto a fine file).

---

## 3. Proposte di espansione (coerenti con le regole esistenti)

### A. Armi e armature

- **Armature:** sono la novità più naturale, perché oggi nessun oggetto riduce i danni.
  - Nuovo tipo di oggetto con un'iniziale libera, per esempio `C` per Corazza o `E` per Elmo.
  - Variabile `MR` = protezione. Nel duello (riga 4160) il danno `HY` cala di `MR` (minimo 1).
    Nella mischia (righe 3240-3250) una prova `MR*10` su 100 può annullare la ferita.
  - Una sola armatura alla volta, come la Spada (`MK=1`).
  - Esempi: Giaco di cuoio (protezione 1, £1500), Cotta di maglia (2, £4000), Corazza elfica (3, £9000).
- **Armi comuni:** lame non magiche (+15 / +25 a `AT`), con un'iniziale libera (`B` di "Brando", `L` di "Lancia"...).
  Restano compatibili con la regola "una sola Spada magica". La Spada magica resta l'unica arma contro i Licantropi.
- **Dove comprarle:** un nuovo negozio **Armaiolo** a Babbakesch. Il menu dei negozi (riga 1230) passa da 3 a 4
  negozi più le 2 voci fisse, con una riga di menu riscritta da noi.
- **Nuovi consumabili** sul modello del Filtro, si usano con un tasto funzione:
  - Pane elfico (F3): +2 Robustezza attuale;
  - Unguento (F5): annulla l'effetto dello sguardo del Vampiro.

### B. Nuove missioni

Seguono lo schema esistente: settimane di viaggio con `GOSUB 3010/3550` per gli incontri, una prova di
caratteristica, un duello (`GOSUB 3950`) e una ricompensa.

- **Scorta del pellegrino** al Tempio di LYSS (il Sacerdote di LYSS esiste già tra i committenti):
  3 settimane; la ricompensa è un Amuleto o un punto di Fascino.
- **Le catacombe della Città Morta** (il luogo esiste già): esplorazione su più stanze con trappole
  (prove di Destrezza e Intelligenza) e un tesoro.
- **La caccia del Conte Movas** (gli armigeri di Movas esistono già): una catena di 3 missioni per il Barone
  Gudrund che finisce con un duello contro il Conte.
- **Nuovi mostri per la caccia**, con regole speciali per iniziale come Drago, Golem, Vampiro e Licantropo:
  - `B` Basilisco: lo sguardo toglie Destrezza; si evita con uno specchio, che è un oggetto nuovo;
  - `I` Idra: dopo ogni colpo andato a segno ha una possibilità di rigenerare.
- In taverna le nuove offerte compaiono a rotazione al posto di una esistente (una sesta riga farebbe scorrere lo schermo). Le righe stampate devono stare in 39 colonne: `node tools/checklen.js mods/espansione` le controlla.

### C. Nuove città

Il gioco cita già luoghi che non si possono visitare. Diventano città con lo stesso modello di Babbakesch:
viaggio di 4 settimane, panorama disegnato, negozi, ritorno.

- **Kebir** (il Sultano di Kebir è un committente), raggiungibile per mare:
  - viaggio con il rischio dei pirati delle Isole Nere;
  - Mercato delle spezie (consumabili) e Arena: combattimenti a premio che aumentano `AT`.
- **Kho**, città ai bordi della giungla (il Cerusico di KHO e i Boscimani esistono già):
  - un guaritore che alza la Robustezza massima, a caro prezzo;
  - un **Tempio** che benedice la spada.
- In taverna l'opzione `* 3 *` diventa "Viaggi": Babbakesch, Kebir, Kho.

### D. Manutenzione (facoltativa, come mod "Correzioni")

- Riga 2660: `IF EE<>6 AND AT<200` andrebbe con `OR`. Oggi lo schermo da Eroe si mostra in casi sbagliati;
  il controllo vero è alla riga 470.
- Riga 3770: `FU=1:RETURN:IF GW=1 THEN RETURN` contiene codice irraggiungibile.
- Gli slot degli oggetti consumati (`"x"`) non vengono riutilizzati: con molti acquisti si arriva a 50 e il gioco va in errore.

---

## 4. Ordine di lavoro consigliato

1. **Strumenti per le mod:** formato `.avm`, `mod build`, scelta "Originale/Espansione" nel launcher,
   salvataggi estesi e compatibili.
2. **Armi e armature + Armaiolo:** tocca combattimento, inventario, negozi e salvataggio, quindi mette alla prova tutto il sistema.
3. **Una missione nuova** (Scorta del pellegrino) come modello per le altre.
4. **Una città nuova** (Kebir) con i suoi negozi.
5. Altri mostri, oggetti e missioni, ripetendo i modelli dei punti 2-4.

Decisione da prendere prima del punto 2: **espansione solo PC** (fino a ~32 KB in più, nessun limite pratico)
oppure **compatibile anche con il C64 vero** (~7,9 KB, quindi meno contenuti ma il `.prg` gira sul C64).
