' ============================================================================
' Le missioni della sacerdotessa del Tempio di LYSS (dall'atrio del Tempio, a Kho).
'   MQ Calice di LYSS (40_calice)   MR reliquie del santo   MT strega del Lago di Lava
' ============================================================================
9000 POKE 56832,6:MP$="tempio":POKE 56834,1:PRINT "{clr}{rvon}LE MISSIONI DELLA SACERDOTESSA{rvof}"
9001 PRINT "{down}{rvon}* 1 *{rvof} Il Calice di LYSS";:IF MQ=1 THEN PRINT "{lgrn} (compiuta){wht}";
9002 PRINT:PRINT "    nascosto nella Citta' Morta"
9003 PRINT "{down}{rvon}* 2 *{rvof} Le reliquie del santo";:IF MR=1 THEN PRINT "{lgrn} (compiuta){wht}";
9004 PRINT:PRINT "    rubate dagli Zombi della Collina":PRINT "    Stregata"
9005 PRINT "{down}{rvon}* 3 *{rvof} La strega del Lago";:IF MT=1 THEN PRINT "{lgrn} (compiuta){wht}";
9006 PRINT:PRINT "    di Lava, che maledice i villaggi":PRINT "    della valle"
9007 PRINT "{down}{rvon}* 4 *{rvof} tornare nell'atrio"
9008 GOSUB 2760:ON N GOTO 9010,9012,9014,7720:PRINT "{up}{up}":GOTO 9008
9010 IF MQ=1 THEN 9016
9011 GOTO 8700
9012 IF MR=1 THEN 9016
9013 GOTO 9020
9014 IF MT=1 THEN 9016
9015 GOTO 9040
9016 PRINT "{down}Hai gia' compiuto questa missione.":GOSUB 4580:GOTO 9000
' --- le reliquie del santo: furtivita', poi la mischia contro gli Zombi
9020 PRINT "{clr}La sacerdotessa ti mostra un'urna":PRINT "vuota: 'Gli Zombi della Collina":PRINT "Stregata hanno rubato le reliquie di":PRINT "san Galvano. Riportale qui !'":GOSUB 4580
9021 TK=TK+1:POKE 56832,3:MP$="zombi":POKE 56834,1:PRINT "{clr}Di notte sali sulla Collina Stregata.":GOSUB 50:GOSUB 4290:IF ZZ=1 THEN PRINT "{down}Gli Zombi ti hanno fiutato: combatti!":GOSUB 50
9022 GOSUB 2790:IF ZZ=0 THEN NM=INT(NM/2)+1:PRINT "{down}Li cogli di sorpresa !":PRINT "Restano solo";NM;"{left} Zombi."
9023 NM$=" Zombi ":IM$="":GOSUB 4580:PRINT "{clr}":GOSUB 3150:IF FU=1 THEN PRINT "{clr}Fuggi dalla collina: le reliquie":PRINT "restano agli Zombi.":GOSUB 4580:GOTO 7720
9024 MR=1:DN=DN+2000:POKE 56832,7:MP$="tempio":POKE 56834,1:PRINT "{clr}{lgrn}Riporti le reliquie al Tempio !{wht}":PRINT "{down}La sacerdotessa ti dona £ 2000."
9025 IF UK=0 THEN UK=1:PRINT "{down}{lblu}Poi benedice la tua arma senza":PRINT "chiedere nulla in cambio.{wht}":GOTO 9027
9026 IF FA<20 THEN FA=FA+1:PRINT "{down}La tua fama cresce: Fascino";FA
9027 GOSUB 160:GOSUB 4580:GOTO 7720
' --- la strega del Lago di Lava: il viaggio, poi il duello
9040 MP$="strega":POKE 56834,1:PRINT "{clr}'Sul Lago di Lava purpurea vive una":PRINT "strega che maledice i nostri villaggi.":PRINT "Spezza il suo potere !' dice la":PRINT "sacerdotessa.":GOSUB 4580
9041 TK=TK+2:POKE 56832,3:PRINT "{clr}Cammini due settimane fino al lago,":PRINT "tra fumi rossi e rocce roventi...":GOSUB 50:IM$="":NM$="":FU=0:GOSUB 3010:IF I>3 THEN FU=0:GOSUB 3550
9042 GOSUB 160:GOSUB 230:PRINT "{clr}Sull'isola nera la strega ti attende:":PRINT "'Nessuno spezza le mie maledizioni !'":GOSUB 4580
9043 MP$="strega":POKE 56834,1:MS$=LA$:LA$="Strega del Lago":HS=0:GOSUB 3950:LA$=MS$
9044 MT=1:DN=DN+2500:POKE 56832,7:MP$="tempio":POKE 56834,1:PRINT "{clr}{lgrn}La strega e' sconfitta !{wht}":PRINT "{down}Le maledizioni si spezzano: la":PRINT "sacerdotessa ti dona £ 2500."
9045 IF IN<20 THEN IN=IN+1:PRINT "{down}La sua saggezza ti illumina:":PRINT "Intelligenza";IN
9046 IF MF+MD>0 THEN FO=FO+MF:AT=AT+5*MF:DE=DE+MD:MF=0:MD=0:PRINT "{down}{lgrn}Ritrovi le forze tolte dalle":PRINT "maledizioni.{wht}"
9047 GOSUB 160:GOSUB 4580:GOTO 7720
