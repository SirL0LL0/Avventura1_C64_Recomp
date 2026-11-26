' ============================================================================
' Missioni: la scorta del pellegrino (dai Viaggi) e la catena del Conte Movas
' (dal Barone Gudrund, a fine ingaggio).
'   UE  fughe durante la scorta     UM  tappa raggiunta contro Movas (0-3)
' ============================================================================
!HOOK 2500 GOTO 8800
' --- il Barone: missione segreta (al posto della scelta originale)
8800 IF UM<3 THEN PRINT "{down}{rvon}{pur}* 4 *{rvof}{wht} Una missione segreta del Barone"
8801 GOSUB 2760:IF N=4 AND UM<3 THEN 8810
8802 IF N>3 THEN PRINT "{up}{up}":GOTO 8801
8803 GOTO 2510
8810 UM=UM+1:TK=TK+2:POKE 56832,3:MP$="armigeri":POKE 56834,1:ON UM GOTO 8820,8840,8860
' --- 1: la spia
8820 PRINT "{clr}Il Barone ti riceve in segreto:":PRINT "'Una spia del Conte Movas porta i nostri":PRINT "piani al castello nemico. Fermala!'":GOSUB 4580
8821 PRINT "{clr}Raggiungi la spia scortata da armigeri.":GOSUB 2790:NM$="armigeri ":IM$="":GOSUB 4580:PRINT "{clr}":GOSUB 3150:IF FU=1 THEN UM=UM-1:PRINT "{clr}La spia e' fuggita: missione fallita.":GOSUB 4580:GOTO 2490
8822 DN=DN+1500:POKE 56833,4:PRINT "{clr}{lgrn}Hai recuperato i piani del Barone !{wht}":PRINT "{down}Il Barone ti paga £ 1500.":GOSUB 4580:GOTO 2490
' --- 2: la torre di guardia
8840 PRINT "{clr}'La torre di guardia di Movas controlla":PRINT "il passo. Espugnala!' ordina il Barone.":GOSUB 4580:PRINT "{clr}Di notte ti avvicini alla torre...":GOSUB 4290:IF ZZ=1 THEN PRINT "{down}Le sentinelle ti hanno visto: combatti!":GOSUB 50
8841 GOSUB 2790:IF ZZ=0 THEN NM=INT(NM/2)+1:PRINT "{down}Sorprendi la guarnigione: restano";NM;"{left} armigeri."
8842 NM$="armigeri ":IM$="":GOSUB 4580:PRINT "{clr}":GOSUB 3150:IF FU=1 THEN UM=UM-1:PRINT "{clr}Ti ritiri: la torre resta a Movas.":GOSUB 4580:GOTO 2490
8843 DN=DN+2500:POKE 56833,4:PRINT "{clr}{lgrn}La torre e' espugnata !{wht}":IF FA<20 THEN FA=FA+1
8844 PRINT "{down}Ricompensa £ 2500. Possiedi £";DN:GOSUB 4580:GOTO 2490
' --- 3: il duello con il Conte
8860 PRINT "{clr}'E' giunta l'ora: affronta il Conte":PRINT "Movas nel suo castello!' dice il Barone.":GOSUB 4580:MP$="conte movas":POKE 56834,1:PRINT "{clr}Nella sala del trono il Conte sguaina":PRINT "la spada nera e ti sfida a duello !":GOSUB 4580
8861 MS$=LA$:LA$="Conte Movas":HS=0:GOSUB 3950:LA$=MS$
8862 DN=DN+5000:FA=FA+2:IF FA>20 THEN FA=20
8863 POKE 56832,7:PRINT "{clr}{yel}Il Conte Movas e' sconfitto !{wht}":PRINT "{down}Il Barone ti nomina Cavaliere e ti dona":PRINT "£ 5000: la tua fama corre per la valle.":PRINT "{down}Possiedi £";DN;" - Fascino";FA:GOSUB 4580:GOTO 2490
' --- la scorta del pellegrino
8900 TK=TK+3:POKE 56832,3:MP$="pellegrino":POKE 56834,1:PRINT "{clr}Fratello Anselmo, un anziano pellegrino,":PRINT "deve raggiungere il Tempio di LYSS.":PRINT "{down}Lo scorterai lungo la strada della":PRINT "giungla per 3 settimane.":GOSUB 4580:UE=0
8905 FOR UW=1 TO 2:PRINT "{clr}";UW;"{left}' settimana con il pellegrino":GOSUB 50:IM$="":NM$="":FU=0:GOSUB 3010:IF I>3 THEN FU=0:GOSUB 3550
8906 IF FU=1 THEN UE=UE+1
8907 GOSUB 160:GOSUB 230:NEXT
8910 IF UE>0 THEN PRINT "{clr}Fuggendo hai lasciato solo il pellegrino:":PRINT "lo ritrovi ferito, ma vivo. Ti paga":PRINT "solo meta' del compenso.":MW=400:GOTO 8920
8915 PRINT "{clr}Arrivate sani e salvi al Tempio di LYSS!":MW=800:IF FA<20 THEN FA=FA+1:PRINT "{down}Il pellegrino loda il tuo valore:":PRINT "Fascino";FA
8920 DN=DN+MW:POKE 56833,4:PRINT "{down}Ricevi £";MW;"{left}. Possiedi £";DN:GOSUB 4580:GOTO 7700
