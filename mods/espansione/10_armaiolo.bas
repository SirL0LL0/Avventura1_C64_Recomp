' ============================================================================
' Armi comuni, armature, Pane elfico e l'Armaiolo di Babbakesch.
'   UA$/UB  arma comune e suo bonus in % (si somma ad AT come la Spada magica)
'   UR$/UR  armatura e protezione (1-3)
'   UP      pani elfici (F3 quando il gioco chiede "premi un tasto": +3 Ro)
' ============================================================================
!HOOK 1230 GOTO 7200
!HOOK 4160 GOSUB 7400
!HOOK 3240 GOSUB 7410
!HOOK 4600 GOSUB 7420
!HOOK 470 GOSUB 7520
!HOOK 4420 GOSUB 7530
!HOOK 4490 GOSUB 7540
' --- il bazar: quattro botteghe
7200 PRINT "{clr}Nel bazar di Babbakesch trovi 4 botteghe:":PRINT "{down}{rvon}* 1 *{rvof} il Fabbro magico":PRINT "{down}{rvon}* 2 *{rvof} l'Alchimista":PRINT "{down}{rvon}* 3 *{rvof} lo Stregone":PRINT "{down}{rvon}{lblu}* 4 *{rvof}{wht} l'Armaiolo{down}":FOR MI=1 TO 20:PRINT "- ";:NEXT
7205 PRINT "{down}{rvon}{grn}* 5 *{wht}{rvof} voglio rivedere il personaggio":PRINT "{down}{rvon}{yel}* 6 *{wht}{rvof} ritorno a casa ":MP$="babbakesch":POKE 56834,1
7210 GOSUB 2760:IF N>6 THEN PRINT "{up}{up}":GOTO 7210
7220 ON N GOTO 1260,1380,1470,7300,1560,1620
' --- l'Armaiolo
7300 PRINT "{clr}La bottega dell' {rvon}ARMAIOLO{rvof}":MP$="armaiolo":POKE 56834,1:PRINT "{down}Armi (ne porti una, oltre alla spada):"
7305 PRINT "{rvon}* 1 *{rvof} Pugnale +10%  {yel}£ 800{wht}":PRINT "{rvon}* 2 *{rvof} Lancia +15%  {yel}£ 1500{wht}":PRINT "{rvon}* 3 *{rvof} Ascia +20%  {yel}£ 2500{wht}"
7306 PRINT "{down}Armature (attutiscono i colpi):":PRINT "{rvon}* 4 *{rvof} Giaco di cuoio (prot.1)  {yel}£ 1500{wht}":PRINT "{rvon}* 5 *{rvof} Cotta di maglia (prot.2)  {yel}£ 4000{wht}":PRINT "{rvon}* 6 *{rvof} Corazza elfica (prot.3)  {yel}£ 9000{wht}"
7307 PRINT "{down}{rvon}* 7 *{rvof} Pane elfico +3 Ro (F3)  {yel}£ 300{wht}":PRINT "{rvon}* 8 *{rvof} Grazie, tornero' piu' tardi"
7310 GOSUB 2760:IF N>8 THEN PRINT "{up}{up}":GOTO 7310
7315 PRINT "{down}":ON N GOTO 7320,7330,7340,7350,7360,7370,7380,1370
7320 MC=800:MW=10:MA$="Pugnale":GOTO 7390
7330 MC=1500:MW=15:MA$="Lancia":GOTO 7390
7340 MC=2500:MW=20:MA$="Ascia":GOTO 7390
7350 MC=1500:MW=1:MA$="Giaco di cuoio":GOTO 7395
7360 MC=4000:MW=2:MA$="Cotta di maglia":GOTO 7395
7370 MC=9000:MW=3:MA$="Corazza elfica":GOTO 7395
7380 IF DN<300 THEN 4280
7385 DN=DN-300:UP=UP+1:POKE 56833,4:PRINT "{down}Hai comprato un Pane elfico (ne hai";UP;"{left})":PRINT "Mangialo con F3 quando ti viene chiesto":PRINT "di premere un tasto.":GOTO 1570
' --- acquisto di un'arma: sostituisce quella vecchia
7390 IF DN<MC THEN 4280
7391 IF UA$<>"" THEN PRINT "Lasci in cambio il tuo ";UA$;"."
7392 DN=DN-MC:AT=AT-UB+MW:UB=MW:UA$=MA$:POKE 56833,4:MP$="armaiolo":POKE 56834,1:PRINT "{down}Hai acquistato:{yel} ";UA$:PRINT "{wht}La capacita' di combattimento e' ora{rvon}";AT;"{left}%{rvof}":GOTO 1570
' --- acquisto di un'armatura
7395 IF DN<MC THEN 4280
7396 IF MW<=UR THEN PRINT "La tua armatura e' gia' buona quanto":PRINT "questa o migliore !":GOTO 1570
7397 DN=DN-MC:UR=MW:UR$=MA$:POKE 56833,4:PRINT "{down}Indossi ora:{yel} ";UR$;"{wht} (prot.";UR;"{left})":GOTO 1570
' --- duello: l'armatura riduce la ferita (prima della riga che la sottrae)
7400 IF UR=0 THEN RETURN
7401 HY=HY-UR:IF HY<1 THEN HY=1
7402 PRINT "{down}{lblu}La tua armatura attutisce il colpo{wht}":POKE 56833,1:RETURN
' --- mischia: l'armatura puo' fermare il colpo (20% per punto di protezione)
7410 IF CS<>1 OR UR=0 THEN RETURN
7411 IF INT(10*RND(0))<UR*2 THEN CS=0:PRINT "{lblu}*{wht} La tua armatura ferma il colpo !":POKE 56833,1
7412 RETURN
' --- F3 al "premi un tasto": mangia un Pane elfico
7420 IF P$<>"{f3}" OR UP=0 THEN RETURN
7421 IF RF>=RO THEN PRINT "{down}Non hai fame: la tua Robustezza e' piena.":RETURN
7422 UP=UP-1:RF=RF+3:IF RF>RO THEN RF=RO
7423 POKE 56833,5:PRINT "{down}{lgrn}Mangi un Pane elfico: Ro";RF;"{left}/";RO;"{left}{wht}":RETURN
' --- scheda del personaggio: equipaggiamento della mod
7520 IF UA$<>"" THEN PRINT "Arma: ";UA$;" (+";UB;"{left}%)"
7521 IF UR$<>"" THEN PRINT "Armatura: ";UR$;" (prot.";UR;"{left})"
7522 IF UP>0 THEN PRINT UP;"pani elfici (F3)"
7523 RETURN
' --- salvataggio esteso (in coda al file: i vecchi salvataggi restano validi)
7530 PRINT#8,"MOD2":PRINT#8,UA$:PRINT#8,UB:PRINT#8,UR$:PRINT#8,UR:PRINT#8,UP:PRINT#8,UK:PRINT#8,MF:PRINT#8,MD:PRINT#8,MQ:RETURN
7540 INPUT#8,M$:IF M$<>"MOD1" AND M$<>"MOD2" THEN RETURN
7541 INPUT#8,UA$,UB,UR$,UR,UP:IF M$="MOD2" THEN INPUT#8,UK,MF,MD,MQ
7542 RETURN
