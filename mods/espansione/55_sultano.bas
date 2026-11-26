' ============================================================================
' Le missioni del Sultano di Kebir, proclamate dal banditore al bazar.
' Mostri nuovi con le regole del duello originale (iniziale del nome, riga 3970):
'   "Djinn delle sabbie" comincia per D -> combatte al 95% come draghi e demoni.
'   MG scorpione ucciso   MJ tomba del Re Antico   MK djinn della lampada
' ============================================================================
9100 POKE 56832,5:MP$="banditore":POKE 56834,1:PRINT "{clr}Al gran bazar il {rvon}BANDITORE{rvof} del Sultano":PRINT "batte il gong e legge un proclama:"
9101 PRINT "{down}{rvon}* 1 *{rvof} Lo Scorpione gigante";:IF MG=1 THEN PRINT "{lgrn} (compiuta){wht}";
9102 PRINT:PRINT "    del Deserto di Sale  {yel}£ 3000{wht}"
9103 PRINT "{down}{rvon}* 2 *{rvof} La tomba del Re Antico";:IF MJ=1 THEN PRINT "{lgrn} (compiuta){wht}";
9104 PRINT:PRINT "    profanata dalle Mummie  {yel}£ 2000{wht}"
9105 PRINT "{down}{rvon}* 3 *{rvof} Il Djinn della lampada";:IF MK=1 THEN PRINT "{lgrn} (compiuta){wht}";
9106 PRINT:PRINT "    fuggito dal tesoro del Sultano":PRINT "    {yel}£ 4000{wht}"
9107 PRINT "{down}{rvon}* 4 *{rvof} tornare al porto"
9108 GOSUB 2760:ON N GOTO 9110,9112,9114,7800:PRINT "{up}{up}":GOTO 9108
9110 IF MG=1 THEN 9116
9111 GOTO 9120
9112 IF MJ=1 THEN 9116
9113 GOTO 9140
9114 IF MK=1 THEN 9116
9115 GOTO 9160
9116 PRINT "{down}Hai gia' compiuto questa impresa.":GOSUB 4580:GOTO 9100
' --- lo Scorpione gigante: un duello, poi il veleno (prova di Destrezza)
9120 PRINT "{clr}'Uno Scorpione grande come un cammello":PRINT "assale le carovane del Deserto di Sale.":PRINT "Il Sultano paga £ 3000 per la sua":PRINT "coda !'":GOSUB 4580
9121 TK=TK+1:POKE 56832,3:MP$="deserto":POKE 56834,1:PRINT "{clr}Una settimana di marcia tra le dune":PRINT "bianche di sale...":GOSUB 50:IM$="":NM$="":FU=0:GOSUB 3010:IF I>3 THEN FU=0:GOSUB 3550
9122 GOSUB 160:GOSUB 230:PRINT "{clr}La sabbia si solleva: lo {rvon}SCORPIONE{rvof}":PRINT "ti sbarra la strada !":GOSUB 4580
9123 MS$=LA$:LA$="Scorpione gigante":HS=0:GOSUB 3950:LA$=MS$
9124 PRINT "{clr}Mentre muore, lo scorpione ti colpisce":PRINT "con il pungiglione ! Per schivarlo devi":PRINT "usare la Destrezza (";DE;"{left}) su 21.":GOSUB 20:GOSUB 50
9125 IF DE>=D6 THEN PRINT "{down}{lgrn}Lo schivi per un soffio !{wht}":GOTO 9127
9126 RF=RF-2:POKE 56833,2:PRINT "{down}Il veleno ti brucia: 2 ferite.":PRINT "Robustezza";RF:IF RF<=0 THEN PRINT "{down}Sei morto !":GOTO 2680
9127 MG=1:DN=DN+3000:POKE 56832,7:MP$="banditore":POKE 56834,1:PRINT "{down}Porti la coda al palazzo: il Sultano":PRINT "ti paga £ 3000.":GOSUB 160:GOSUB 4580:GOTO 7800
' --- la tomba del Re Antico: furtivita', le Mummie in mischia, poi il loro Re
9140 PRINT "{clr}'Ladri di tombe hanno svegliato le":PRINT "Mummie del Re Antico: ora escono di":PRINT "notte e assalgono Kebir. Riportale nel":PRINT "sonno eterno !'":GOSUB 4580
9141 TK=TK+1:POKE 56832,3:MP$="mummia":POKE 56834,1:PRINT "{clr}Scendi nella tomba con una torcia.":GOSUB 50:GOSUB 4290:IF ZZ=1 THEN PRINT "{down}Le Mummie si voltano verso di te !":GOSUB 50
9142 GOSUB 2790:IF ZZ=0 THEN NM=INT(NM/2)+1:PRINT "{down}Ne sorprendi alcune ancora immobili:":PRINT "restano";NM;"{left} Mummie."
9143 NM$=" Mummie ":IM$="":GOSUB 4580:PRINT "{clr}":GOSUB 3150:IF FU=1 THEN PRINT "{clr}Fuggi dalla tomba: le Mummie restano":PRINT "padrone della notte.":GOSUB 4580:GOTO 7800
9144 PRINT "{clr}Dal sarcofago d'oro si alza la":PRINT "{yel}Mummia del Re{wht} !":GOSUB 4580:MS$=LA$:LA$="Mummia del Re":HS=0:GOSUB 3950:LA$=MS$
9145 MJ=1:DN=DN+2000:POKE 56832,7:MP$="banditore":POKE 56834,1:PRINT "{clr}{lgrn}La tomba e' di nuovo in pace !{wht}":PRINT "{down}Il Sultano ti paga £ 2000 e ti dona":PRINT "l'{yel}Amuleto del Re{wht} (+1 Intelligenza)."
9146 IF WX<49 THEN WX=WX+1:OS$(WX)="Amuleto del Re":IF IN<20 THEN IN=IN+1
9147 GOSUB 160:GOSUB 4580:GOTO 7800
' --- il Djinn della lampada: un indovinello (Intelligenza), altrimenti il duello
9160 PRINT "{clr}'Il Djinn della lampada e' fuggito dal":PRINT "tesoro e devasta i giardini del":PRINT "palazzo. Rimettilo nella lampada !'":GOSUB 4580
9161 POKE 56832,4:MP$="djinn":POKE 56834,1:PRINT "{clr}Tra le fontane appare il {rvon}DJINN{rvof}, alto":PRINT "come una torre: 'Risolvi il mio":PRINT "enigma o combatti !'":PRINT "Usa l'Intelligenza (";IN;"{left}) su 21.":GOSUB 20:GOSUB 50
9162 IF IN>=D6 THEN PRINT "{down}{lgrn}Risolvi l'enigma: il Djinn ride e":PRINT "torna da solo nella lampada !{wht}":POKE 56833,8:GOTO 9165
9163 PRINT "{down}Non trovi la risposta: il Djinn":PRINT "scatena il vento del deserto !":GOSUB 4580:MS$=LA$:LA$="Djinn delle sabbie":HS=0:GOSUB 3950:LA$=MS$
9164 PRINT "{clr}Sconfitto, il Djinn rientra fumando":PRINT "nella lampada."
9165 MK=1:DN=DN+4000:POKE 56832,7:MP$="banditore":POKE 56834,1:PRINT "{down}Il Sultano ti paga £ 4000.":IF FR<20 THEN FR=FR+1:PRINT "{down}Il Djinn ti lascia un dono:":PRINT "Fortuna";FR
9166 GOSUB 4580:GOTO 7800
