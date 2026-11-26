// Colonna sonora originale del progetto per due SID (6 voci).
// Qui solo i temi brevi (fanfara, morte): la colonna sonora e' generata in music.cpp.
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
