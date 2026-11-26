' ============================================================================
' Missione: il Calice di LYSS (affidata dalla sacerdotessa del Tempio di Kho).
' Tre stanze nelle catacombe della Citta' Morta: trappola (Destrezza),
' enigma (Intelligenza), Spettro guardiano (serve l'arma benedetta).
'   MQ = 1 missione compiuta
' ============================================================================
8700 MP$="tempio":POKE 56834,1:PRINT "{clr}La sacerdotessa abbassa il velo:":PRINT "{down}'Il Calice di LYSS e' stato rubato e":PRINT "nascosto nelle catacombe della Citta'":PRINT "Morta. Riportalo e la dea ti ricompensera'.'"
8701 PRINT "{down}{yel}Si dice che lo custodisca uno spettro:":PRINT "servira' un'arma benedetta.{wht}"
8702 PRINT "{down}{rvon}* 1 *{rvof} accetto la missione":PRINT "{down}{rvon}* 2 *{rvof} non ora"
8705 GOSUB 2760:IF N>2 THEN PRINT "{up}{up}":GOTO 8705
8706 IF N=2 THEN 7720
' --- discesa nelle catacombe
8710 TK=TK+1:POKE 56832,3:MP$="catacombe":POKE 56834,1:PRINT "{clr}Raggiungi la Citta' Morta e scendi":PRINT "nelle catacombe con una torcia...":GOSUB 50
' --- prima stanza: la trappola
8720 PRINT "{down}{rvon}1' stanza{rvof} Il pavimento scricchiola:":PRINT "una trappola! Per saltare in tempo devi":PRINT "usare la Destrezza (";DE;"{left}) su 21.":GOSUB 20:GOSUB 50
8721 IF DE>=D6 THEN PRINT "{down}{lgrn}Salti oltre la botola appena in tempo!{wht}":GOTO 8730
8722 RF=RF-3:POKE 56833,2:PRINT "{down}Cadi tra le lance: ricevi 3 ferite.":PRINT "Robustezza";RF:IF RF<=0 THEN PRINT "{down}Sei morto !":GOTO 2680
' --- seconda stanza: l'enigma
8730 GOSUB 4580:PRINT "{clr}{rvon}2' stanza{rvof} Una porta di bronzo porta":PRINT "inciso un enigma. Per risolverlo devi":PRINT "usare l'Intelligenza (";IN;"{left}) su 21.":GOSUB 20:GOSUB 50
8731 IF IN>=D6 THEN PRINT "{down}{lgrn}La porta si apre con un sospiro di":PRINT "polvere antica.{wht}":GOTO 8740
8732 TK=TK+1:PRINT "{down}Non trovi la soluzione: perdi una":PRINT "settimana a cercare un altro passaggio.":GOSUB 160:GOSUB 230
' --- terza stanza: lo Spettro guardiano
8740 GOSUB 4580:PRINT "{clr}{rvon}3' stanza{rvof} Sull'altare brilla il":PRINT "Calice... e dalle ombre si leva uno":PRINT "{lblu}Spettro guardiano{wht} !":GOSUB 50:IF UK=0 THEN PRINT "{down}{yel}La tua arma non e' benedetta...{wht}"
8741 GOSUB 4580:MS$=LA$:LA$="Spettro guardiano":HS=0:GOSUB 3950:LA$=MS$
' --- ricompensa
8750 MQ=1:DN=DN+3000:POKE 56832,7:POKE 56833,6:MP$="tempio":POKE 56834,1:PRINT "{clr}Riporti il Calice al Tempio di LYSS !":PRINT "{down}La sacerdotessa ti dona £ 3000 e":PRINT "l'{yel}Amuleto di LYSS{wht} (+1 Intelligenza)."
8751 IF WX<49 THEN WX=WX+1:OS$(WX)="Amuleto di LYSS":IN=IN+1
8752 IF FA<20 THEN FA=FA+1:PRINT "{down}La tua fama cresce: Fascino";FA
8753 GOSUB 160:GOSUB 4580:GOTO 7720
