// Colonna sonora originale del progetto per due SID (6 voci).
// Musiche di sottofondo, lente e d'atmosfera; batteria solo in battaglia e nella fanfara.
// Formato:
//   song NOME / tempo T (tick da 1/50 s per riga) / bar R (righe per battuta) / loop 0|1
//   chords ACCORDI... (uno per battuta: C Dm F# Bb7 Esus ...)
//   inst NOME wave=tri|saw|pulse|noise|pulsesaw ad=XX sr=XX pw=XXX sweep=N vib=centesimi,velocita' delay=N filt=0|1
//   filter SID cutoff=XXX res=X mode=lp|bp|hp
//   vN mel  STRUMENTO : NOTA.durata ...   (r = pausa, [ ... ]N = ripeti N volte)
//   vN echo STRUMENTO : vSORGENTE ritardo ottava
//   vN bass STRUMENTO : PATTERN  (R fondamentale, O ottava, F quinta, T terza, . prolunga, - pausa)
//   vN arp  STRUMENTO              (arpeggio veloce dell'accordo, stile C64)
//   vN harp STRUMENTO : ottava passo   (accordo spezzato lento, una nota ogni "passo" righe)
//   vN pad  STRUMENTO : ottava [nota]  (nota dell'accordo tenuta: 0 fondamentale, 1 terza, 2 quinta)
//   vN drum - : K.n S.n H.n r.n     (cassa, rullante, charleston)
// Voci 1-3: primo SID (sinistra), 4-6: secondo SID (destra; la 6 e' condivisa con gli effetti).
#pragma once

static const char* SONGS_TEXT = R"SONGS(
; ------------------------------------------------------------------ TITOLO: re dorico, ballata epica
song titolo
tempo 15
bar 8
chords Dm C Bb C Dm F G A Dm C Bb C Dm F G A
inst lead wave=pulse ad=5A sr=AA pw=500 sweep=6 vib=16,3 delay=14
inst eco wave=tri ad=5A sr=6A
inst basso wave=tri ad=6A sr=AB
inst arpa wave=pulse ad=0A sr=0A pw=700 filt=1
inst pad wave=saw ad=BA sr=AB filt=1
filter 2 cutoff=1C0 res=9 mode=lp
v1 mel lead : D4.2 F4.2 A4.3 G4.1 | E4.2 G4.2 C5.3 B4.1 | A4.2 F4.2 D4.2 F4.2 | G4.4 E4.4 | D5.3 C5.1 A4.2 F4.2 | C5.3 A4.1 F4.2 A4.2 | B4.2 D5.2 G4.2 B4.2 | C#5.4 E5.4 | A4.2 D5.2 F5.3 E5.1 | E5.2 C5.2 G4.3 A4.1 | Bb4.2 A4.2 F4.2 D4.2 | E4.3 F4.1 G4.4 | F5.3 E5.1 D5.2 A4.2 | C5.2 F5.2 E5.2 C5.2 | D5.2 B4.2 G4.2 D5.2 | E5.4 C#5.4
v2 echo eco : v1 4 -1
v3 bass basso : R.......
v4 harp arpa : 3 1
v5 pad pad : 3 0
v6 pad pad : 3 2

; ------------------------------------------------------------------ TAVERNA: sol maggiore, giga tranquilla
song taverna
tempo 10
bar 6
chords G C G D G C D G G C G D G C D G
inst lead wave=pulse ad=4A sr=99 pw=700 sweep=-5 vib=10,4 delay=8
inst eco wave=tri ad=4A sr=79
inst basso wave=tri ad=5A sr=AA
inst arpa wave=pulse ad=09 sr=09 pw=600 filt=1
filter 2 cutoff=260 res=6 mode=lp
v1 mel lead : G4.1 B4.1 D5.1 B4.1 G4.1 B4.1 | C5.1 E5.1 C5.1 G4.1 E4.1 G4.1 | B4.1 D5.1 B4.1 G4.2 A4.1 | F#4.1 A4.1 D5.1 C5.1 A4.1 F#4.1 | G4.1 B4.1 D5.1 G5.2 F#5.1 | E5.1 C5.1 E5.1 G5.2 E5.1 | D5.1 C5.1 B4.1 A4.1 F#4.1 A4.1 | G4.3 D4.3 | B4.2 C5.1 D5.2 B4.1 | C5.2 E5.1 G5.2 E5.1 | D5.1 B4.1 G4.1 D5.1 B4.1 G4.1 | A4.2 B4.1 C5.2 A4.1 | B4.1 D5.1 G5.1 D5.1 B4.1 D5.1 | C5.1 E5.1 A5.1 G5.1 E5.1 C5.1 | D5.2 F#5.1 A5.1 F#5.1 D5.1 | G5.3 r.3
v2 echo eco : v1 3 -1
v3 bass basso : R..F..
v4 harp arpa : 3 2

; ------------------------------------------------------------------ VIAGGIO: mi dorico, cammino nella nebbia
song viaggio
tempo 16
bar 8
chords Em D C D Em D C B Em D C D Em D C B
inst lead wave=tri ad=6A sr=AA vib=18,2 delay=16
inst eco wave=pulse ad=6A sr=5A pw=300 sweep=4 filt=1
inst basso wave=tri ad=8A sr=AB
inst arpa wave=pulse ad=0A sr=0A pw=500 filt=1
inst pad wave=saw ad=CA sr=AC filt=1
filter 2 cutoff=170 res=A mode=lp
v1 mel lead : [E4.3 G4.1 B4.4 | A4.3 F#4.1 D4.4 | E4.2 G4.2 C5.2 B4.2 | A4.6 r.2 | B4.3 D5.1 E5.4 | F#5.3 E5.1 D5.4 | E5.2 D5.2 C5.2 B4.2 | B4.4 D#5.4]2
v2 pad pad : 3 1
v3 bass basso : R.......
v4 harp arpa : 3 2
v5 echo eco : v1 6 0

; ------------------------------------------------------------------ BATTAGLIA: la minore, tesa ma non frenetica
song battaglia
tempo 7
bar 8
chords Am Am F G Am Am Bb E Am Am F G Am Am Bb E
inst lead wave=pulsesaw ad=28 sr=A9 pw=600 vib=12,5 delay=8
inst eco wave=pulse ad=28 sr=69 pw=300 sweep=10
inst basso wave=pulse ad=05 sr=85 pw=300 filt=1
inst pad wave=saw ad=A9 sr=AA
filter 1 cutoff=220 res=B mode=lp
v1 mel lead : [A4.2 C5.2 E5.2 D5.1 C5.1 | B4.2 A4.2 G#4.4 | F4.2 A4.2 C5.3 A4.1 | G4.2 B4.2 D5.4 | E5.2 D5.1 C5.1 B4.2 A4.2 | C5.2 B4.1 A4.1 G#4.2 E4.2 | F4.2 Bb4.2 D5.2 F5.2 | E5.4 G#4.4]2
v2 echo eco : v1 3 0
v3 bass basso : R.R.O.R.
v4 pad pad : 3 1
v6 drum - : [K.2 H.2 S.2 H.2]16

; ------------------------------------------------------------------ CITTA': re frigio dominante, mercato al tramonto
song citta
tempo 13
bar 8
chords D Eb D D Gm D Eb D D Eb D D Gm D Eb D
inst lead wave=saw ad=59 sr=9A vib=26,3 delay=8 filt=1
inst eco wave=pulse ad=59 sr=6A pw=200 sweep=6
inst basso wave=tri ad=8A sr=BB
inst arpa wave=pulse ad=09 sr=09 pw=400 filt=1
inst pad wave=pulse ad=CA sr=BB pw=800 filt=1
filter 1 cutoff=300 res=6 mode=lp
filter 2 cutoff=200 res=9 mode=bp
v1 mel lead : [D4.1 Eb4.1 F#4.2 G4.2 A4.2 | Bb4.2 A4.1 G4.1 F#4.2 Eb4.2 | D4.2 F#4.1 A4.1 C5.2 Bb4.2 | A4.4 r.4 | D5.2 C5.1 Bb4.1 A4.2 G4.2 | F#4.2 G4.1 A4.1 Bb4.2 A4.2 | G4.1 F#4.1 Eb4.2 F#4.2 G4.2 | D4.6 r.2]2
v2 echo eco : v1 4 -1
v3 bass basso : R...F...
v4 harp arpa : 3 2
v5 pad pad : 3 1

; ------------------------------------------------------------------ TEMPIO: fa lidio, coro solenne
song tempio
tempo 20
bar 8
chords F G F C Dm G F C
inst lead wave=tri ad=8A sr=AB vib=10,2 delay=18
inst coro wave=pulse ad=CA sr=AB pw=900 sweep=3 filt=1
inst basso wave=tri ad=AA sr=BB
filter 1 cutoff=160 res=8 mode=lp
filter 2 cutoff=180 res=7 mode=lp
v1 mel lead : A4.4 C5.4 | B4.4 D5.4 | C5.6 A4.2 | G4.8 | F4.4 A4.4 | B4.4 G4.4 | A4.4 F4.4 | E4.8
v2 pad coro : 4 1
v3 bass basso : R.......
v4 pad coro : 3 0
v5 pad coro : 3 2

; ------------------------------------------------------------------ EROE: fanfara (una volta)
song eroe
tempo 9
bar 8
loop 0
chords C F G C C F G C
inst lead wave=pulse ad=28 sr=AA pw=400 sweep=10 vib=14,4 delay=10
inst eco wave=saw ad=28 sr=8A
inst basso wave=tri ad=08 sr=A9
inst pad wave=saw ad=88 sr=AA filt=1
filter 2 cutoff=280 res=6 mode=lp
v1 mel lead : C4.2 E4.2 G4.2 C5.2 | A4.3 C5.1 F5.4 | D5.2 B4.2 G4.2 D5.2 | C5.8 | E5.2 G5.2 E5.2 C5.2 | F5.3 E5.1 D5.4 | B4.2 D5.2 G5.2 F5.2 | E5.8
v2 echo eco : v1 0 -1
v3 bass basso : R...O...
v4 pad pad : 3 1
v6 drum - : [K.4 S.4]6 K.2 K.2 S.4 K.8

; ------------------------------------------------------------------ MORTE: la minore, lento (una volta)
song morte
tempo 20
bar 8
loop 0
chords Am F Dm E
inst lead wave=tri ad=AB sr=8C vib=22,2 delay=10
inst pad wave=saw ad=CB sr=8C filt=1
inst basso wave=tri ad=AB sr=AC
filter 2 cutoff=110 res=6 mode=lp
v1 mel lead : A4.4 G4.2 E4.2 | F4.4 E4.2 C4.2 | D4.4 F4.2 A4.2 | G#4.8
v3 bass basso : R.......
v4 pad pad : 3 0
v5 pad pad : 3 1
)SONGS";
