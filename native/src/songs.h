// Colonna sonora originale del progetto per due SID (6 voci).
// Formato:
//   song NOME / tempo T (tick da 1/50 s per riga) / bar R (righe per battuta) / loop 0|1
//   chords ACCORDI... (uno per battuta: C Dm F# Bb7 Esus ...)
//   inst NOME wave=tri|saw|pulse|noise|pulsesaw ad=XX sr=XX pw=XXX sweep=N vib=centesimi,velocita' delay=N filt=0|1
//   filter SID cutoff=XXX res=X mode=lp|bp|hp
//   vN mel  STRUMENTO : NOTA.durata ...   (r = pausa, [ ... ]N = ripeti N volte)
//   vN echo STRUMENTO : vSORGENTE ritardo ottava
//   vN bass STRUMENTO : PATTERN  (R fondamentale, O ottava, F quinta, T terza, . prolunga, - pausa)
//   vN arp  STRUMENTO              (arpeggio dell'accordo, stile C64)
//   vN pad  STRUMENTO : ottava
//   vN drum - : K.n S.n H.n r.n     (cassa, rullante, charleston)
// Voci 1-3: primo SID (sinistra), 4-6: secondo SID (destra; la 6 e' condivisa con gli effetti).
#pragma once

static const char* SONGS_TEXT = R"SONGS(
; ------------------------------------------------------------------ TITOLO: re dorico, epico
song titolo
tempo 8
bar 8
chords Dm C Bb C Dm F G A Dm C Bb C Dm F G A
inst lead wave=pulse ad=0A sr=A9 pw=500 sweep=10 vib=18,4 delay=10
inst eco wave=pulse ad=0A sr=59 pw=300 sweep=6
inst basso wave=tri ad=08 sr=A6
inst arp wave=pulse ad=00 sr=A4 pw=800 filt=1
inst pad wave=tri ad=A9 sr=C8
filter 2 cutoff=300 res=8 mode=lp
v1 mel lead : D4.2 F4.2 A4.3 G4.1 | E4.2 G4.2 C5.3 B4.1 | A4.2 F4.2 D4.2 F4.2 | G4.4 E4.4 | D5.3 C5.1 A4.2 F4.2 | C5.3 A4.1 F4.2 A4.2 | B4.2 D5.2 G4.2 B4.2 | C#5.4 E5.4 | A4.2 D5.2 F5.3 E5.1 | E5.2 C5.2 G4.3 A4.1 | Bb4.2 A4.2 F4.2 D4.2 | E4.3 F4.1 G4.4 | F5.3 E5.1 D5.2 A4.2 | C5.2 F5.2 E5.2 C5.2 | D5.2 B4.2 G4.2 D5.2 | E5.4 C#5.4
v2 echo eco : v1 3 0
v3 bass basso : R..O.RO.
v4 arp arp
v5 pad pad : 3
v6 drum - : [K.2 H.1 H.1 S.2 H.1 H.1]15 K.1 K.1 S.1 S.1 S.1 S.1 S.2

; ------------------------------------------------------------------ TAVERNA: sol maggiore, giga in 6/8
song taverna
tempo 5
bar 6
chords G C G D G C D G G C G D G C D G
inst lead wave=pulse ad=09 sr=88 pw=700 sweep=-8 vib=10,6 delay=6
inst organo wave=saw ad=09 sr=66
inst basso wave=tri ad=07 sr=95
inst arp wave=pulse ad=00 sr=94 pw=600 filt=1
filter 2 cutoff=420 res=6 mode=lp
v1 mel lead : G4.1 B4.1 D5.1 B4.1 G4.1 B4.1 | C5.1 E5.1 C5.1 G4.1 E4.1 G4.1 | B4.1 D5.1 B4.1 G4.2 A4.1 | F#4.1 A4.1 D5.1 C5.1 A4.1 F#4.1 | G4.1 B4.1 D5.1 G5.2 F#5.1 | E5.1 C5.1 E5.1 G5.2 E5.1 | D5.1 C5.1 B4.1 A4.1 F#4.1 A4.1 | G4.3 D4.3 | B4.2 C5.1 D5.2 B4.1 | C5.2 E5.1 G5.2 E5.1 | D5.1 B4.1 G4.1 D5.1 B4.1 G4.1 | A4.2 B4.1 C5.2 A4.1 | B4.1 D5.1 G5.1 D5.1 B4.1 D5.1 | C5.1 E5.1 A5.1 G5.1 E5.1 C5.1 | D5.2 F#5.1 A5.1 F#5.1 D5.1 | G5.3 r.3
v2 echo organo : v1 0 -1
v3 bass basso : R..F..
v4 arp arp
v6 drum - : [K.1 H.1 H.1 S.1 H.1 H.1]16

; ------------------------------------------------------------------ VIAGGIO: mi dorico, marcia
song viaggio
tempo 9
bar 8
chords Em D C D Em D C B Em D C D Em D C B
inst lead wave=pulse ad=0B sr=A9 pw=400 sweep=14 vib=20,3 delay=12
inst eco wave=tri ad=0A sr=79
inst basso wave=tri ad=08 sr=A6
inst pad wave=saw ad=B9 sr=A9 filt=1
filter 2 cutoff=200 res=A mode=lp
v1 mel lead : [E4.3 G4.1 B4.4 | A4.3 F#4.1 D4.4 | E4.2 G4.2 C5.2 B4.2 | A4.6 r.2 | B4.3 D5.1 E5.4 | F#5.3 E5.1 D5.4 | E5.2 D5.2 C5.2 B4.2 | B4.4 D#5.4]2
v2 echo eco : v1 4 -1
v3 bass basso : R...F...
v5 pad pad : 3
v6 drum - : [K.2 H.2 S.2 H.2]16

; ------------------------------------------------------------------ BATTAGLIA: la minore, incalzante
song battaglia
tempo 4
bar 8
chords Am Am F G Am Am Bb E Am Am F G Am Am Bb E
inst lead wave=pulsesaw ad=08 sr=A8 pw=600 vib=14,7 delay=6
inst eco wave=pulse ad=08 sr=68 pw=300 sweep=20
inst basso wave=pulse ad=05 sr=84 pw=300 filt=1
inst arp wave=saw ad=00 sr=A3
filter 1 cutoff=280 res=C mode=lp
v1 mel lead : [A4.2 C5.2 E5.2 D5.1 C5.1 | B4.2 A4.2 G#4.4 | F4.2 A4.2 C5.3 A4.1 | G4.2 B4.2 D5.4 | E5.2 D5.1 C5.1 B4.2 A4.2 | C5.2 B4.1 A4.1 G#4.2 E4.2 | F4.2 Bb4.2 D5.2 F5.2 | E5.4 G#4.4]2
v2 echo eco : v1 2 0
v3 bass basso : RRORRROR
v4 arp arp
v6 drum - : [K.1 H.1 S.1 H.1 K.1 K.1 S.1 H.1]16

; ------------------------------------------------------------------ CITTA': re frigio dominante, orientale
song citta
tempo 7
bar 8
chords D Eb D D Gm D Eb D D Eb D D Gm D Eb D
inst lead wave=saw ad=09 sr=99 vib=30,5 delay=4
inst eco wave=pulse ad=09 sr=69 pw=200 sweep=12
inst basso wave=tri ad=09 sr=B8
inst pad wave=pulse ad=C9 sr=B9 pw=800 filt=1
filter 2 cutoff=250 res=9 mode=bp
v1 mel lead : [D4.1 Eb4.1 F#4.2 G4.2 A4.2 | Bb4.2 A4.1 G4.1 F#4.2 Eb4.2 | D4.2 F#4.1 A4.1 C5.2 Bb4.2 | A4.4 r.4 | D5.2 C5.1 Bb4.1 A4.2 G4.2 | F#4.2 G4.1 A4.1 Bb4.2 A4.2 | G4.1 F#4.1 Eb4.2 F#4.2 G4.2 | D4.6 r.2]2
v2 echo eco : v1 2 1
v3 bass basso : R...F...
v5 pad pad : 3
v6 drum - : [K.1 r.1 H.1 K.1 S.1 r.1 H.1 H.1]16

; ------------------------------------------------------------------ TEMPIO: fa lidio, solenne
song tempio
tempo 12
bar 8
chords F G F C Dm G F C
inst lead wave=tri ad=6A sr=AA vib=12,3 delay=14
inst coro wave=pulse ad=99 sr=99 pw=900 sweep=4 filt=1
inst basso wave=tri ad=99 sr=B9
filter 1 cutoff=180 res=8 mode=lp
v1 mel lead : A4.4 C5.4 | B4.4 D5.4 | C5.6 A4.2 | G4.8 | F4.4 A4.4 | B4.4 G4.4 | A4.4 F4.4 | E4.8
v2 pad coro : 4
v3 bass basso : R.......

; ------------------------------------------------------------------ EROE: fanfara (una volta)
song eroe
tempo 7
bar 8
loop 0
chords C F G C C F G C
inst lead wave=pulse ad=08 sr=A9 pw=400 sweep=16 vib=16,5 delay=8
inst eco wave=saw ad=08 sr=89
inst basso wave=tri ad=08 sr=A8
inst arp wave=pulse ad=00 sr=A5 pw=700
v1 mel lead : C4.2 E4.2 G4.2 C5.2 | A4.3 C5.1 F5.4 | D5.2 B4.2 G4.2 D5.2 | C5.8 | E5.2 G5.2 E5.2 C5.2 | F5.3 E5.1 D5.4 | B4.2 D5.2 G5.2 F5.2 | E5.8
v2 echo eco : v1 0 -1
v3 bass basso : R...O...
v4 arp arp
v6 drum - : [K.2 S.2 K.2 S.2]6 K.1 K.1 S.2 K.1 K.1 S.2 K.8

; ------------------------------------------------------------------ MORTE: la minore, lento (una volta)
song morte
tempo 14
bar 8
loop 0
chords Am F Dm E
inst lead wave=tri ad=8B sr=8B vib=25,2 delay=6
inst pad wave=saw ad=CB sr=8B filt=1
inst basso wave=tri ad=AB sr=AB
filter 2 cutoff=120 res=6 mode=lp
v1 mel lead : A4.4 G4.2 E4.2 | F4.4 E4.2 C4.2 | D4.4 F4.2 A4.2 | G#4.8
v3 bass basso : R.......
v5 pad pad : 3
)SONGS";
