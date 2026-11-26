' ============================================================================
' Maledizioni registrate (curabili al Tempio) e tre mostri nuovi per le cacce:
'   Basilisco (pietrifica: toglie Destrezza), Idra (si rigenera),
'   Spettro del Re (non-morto: senza arma benedetta lo si ferisce appena).
'   MN = mostro nuovo della caccia corrente (0 = mostro originale)
' ============================================================================
!HOOK 4230 GOTO 8500
!HOOK 4250 GOTO 8510
!HOOK 510 GOSUB 8605
!HOOK 520 GOSUB 8600
!HOOK 550 GOSUB 8610
!HOOK 4050 GOSUB 8620
!HOOK 4110 GOSUB 8630
!HOOK 4200 GOSUB 8640
' --- sguardo del Vampiro: sostituisce la routine originale
8500 IF UK=1 THEN PRINT "{down}{lblu}La tua arma benedetta respinge lo":PRINT "sguardo del ";LA$;"{wht}":RETURN
8501 PRINT "{down}Lo sguardo del ";LA$:FO=FO-1:AT=AT-5:PA=PA-5:MF=MF+1:PRINT "ti prosciuga: perdi un punto di Forza":PRINT "(ora";FO;"{left}).":IF FO<=0 THEN 2680
8502 RETURN
' --- urlo del Licantropo: sostituisce la routine originale
8510 IF UK=1 THEN PRINT "{down}{lblu}La tua arma benedetta ti protegge":PRINT "dall'urlo del ";LA$;"{wht}":RETURN
8511 PRINT "{down}L'urlo del ";LA$:DE=DE-1:HP=HP-5:MD=MD+1:PRINT "ti gela il sangue: perdi un punto di":PRINT "Destrezza (ora";DE;"{left}).":IF DE<=0 THEN 2680
8512 RETURN
' --- mostri nuovi nella lista delle cacce (3 volte su 10)
8600 IF INT(10*RND(0))>2 THEN RETURN
8601 MN=INT(3*RND(0))+1:ON MN GOTO 8602,8603,8604
8602 LA$="Basilisco":RETURN
8603 LA$="Idra delle paludi":RETURN
8604 LA$="Spettro del Re":RETURN
8605 MN=0:RETURN
8610 IF MN=1 THEN A9=2000
8611 IF MN=2 THEN A9=2500
8612 IF MN=3 THEN A9=3500
8613 RETURN
' --- duello: Robustezza del mostro prima del tuo colpo
8620 MH=HR:RETURN
' --- Spettro: senza arma benedetta il colpo lo ferisce appena
8630 IF LEFT$(LA$,3)<>"Spe" OR UK=1 OR HR>=MH THEN RETURN
8631 HR=MH-INT((MH-HR)/3):PRINT "{down}{lblu}La lama attraversa lo spettro: senza":PRINT "un'arma benedetta lo ferisci appena!{wht}":RETURN
' --- fine del turno di duello: poteri speciali
8640 IF LEFT$(LA$,3)="Idr" AND HR>0 AND HR<HZ THEN HR=HR+2:PRINT "{down}{lgrn}Una testa mozzata ricresce: l'Idra":PRINT "si rigenera !{wht}"
8641 IF LEFT$(LA$,3)<>"Bas" THEN RETURN
8642 IF INT(4*RND(0))>0 THEN RETURN
8643 IF UK=1 THEN PRINT "{down}{lblu}L'arma benedetta devia lo sguardo":PRINT "pietrificante del Basilisco !{wht}":RETURN
8644 DE=DE-1:MD=MD+1:PRINT "{down}Lo sguardo del Basilisco ti pietrifica":PRINT "le membra: Destrezza";DE:IF DE<=0 THEN 2680
8645 RETURN
