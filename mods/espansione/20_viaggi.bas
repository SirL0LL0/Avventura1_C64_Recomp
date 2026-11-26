' ============================================================================
' Viaggi verso terre lontane: Kho (Tempio di LYSS, Guaritore) e Kebir (Arena, Mercato).
'   UK  arma benedetta (protegge dalle maledizioni, ferisce i non-morti)
'   MF/MD  punti di Forza/Destrezza tolti dalle maledizioni (il Tempio li ridona)
' ============================================================================
!HOOK 640 GOSUB 7600
!HOOK 660 GOTO 7605
7600 PRINT "{rvon}{lgrn}* 6 *{rvof}{wht} Viaggiare: Kho e Kebir":RETURN
7605 IF N=6 THEN 7610
7606 IF N>5 THEN PRINT "{up}{up}":GOTO 650
7607 GOTO 670
7610 PRINT "{clr}{yel}Viaggi verso terre lontane{wht}":MP$="carovana":POKE 56834,1
7611 PRINT "{down}{rvon}* 1 *{rvof} Kho, ai bordi della giungla":PRINT "      2 settimane di cammino":PRINT "{down}{rvon}* 2 *{rvof} Kebir, la citta' del Sultano":PRINT "      2 settimane per mare, nave £ 400":PRINT "{down}{rvon}* 3 *{rvof} restare in citta'"
7612 GOSUB 2760:IF N>3 THEN PRINT "{up}{up}":GOTO 7612
7613 ON N GOTO 7620,7650,490
' --- in cammino verso Kho: gli incontri del gioco originale
7620 TK=TK+2:POKE 56832,3:MP$="kho":POKE 56834,1:PRINT "{clr}Ti incammini verso Kho, per i sentieri":PRINT "che costeggiano la giungla...":GOSUB 50:IM$="":NM$="":GOSUB 3010:IF I<=3 THEN 7625
7621 FU=0:GOSUB 3550
7625 GOSUB 160:GOSUB 230:PRINT "{clr}":GOTO 7700
' --- Kho
7700 POKE 56832,6:MP$="kho":POKE 56834,1:PRINT "{clr}{lgrn}KHO{wht}, villaggio ai bordi della giungla":PRINT "{down}Capanne su palafitte, fumo d'incenso e":PRINT "il richiamo dei tamburi dei Boscimani."
7705 PRINT "{down}{rvon}* 1 *{rvof} il Tempio di LYSS":PRINT "{down}{rvon}* 2 *{rvof} il Guaritore":PRINT "{down}{rvon}* 3 *{rvof} rivedere il personaggio":PRINT "{down}{rvon}* 4 *{rvof} tornare a casa (2 settimane)"
7710 GOSUB 2760:IF N>4 THEN PRINT "{up}{up}":GOTO 7710
7715 ON N GOTO 7720,7760,7790,7795
' --- il Tempio di LYSS
7720 POKE 56832,6:MP$="tempio":POKE 56834,1:PRINT "{clr}Il {rvon}TEMPIO DI LYSS{rvof}":PRINT "{down}Una sacerdotessa velata ti accoglie tra":PRINT "colonne d'avorio e lampade d'oro."
7722 PRINT "{down}{rvon}* 1 *{rvof} Purificazione (£ 600 a punto)":PRINT "    ridona Forza e Destrezza perdute"
7723 PRINT "{rvon}* 2 *{rvof} Benedizione dell'arma (£ 1500)":PRINT "    ripara dalle maledizioni e ferisce":PRINT "    i non-morti"
7724 PRINT "{rvon}* 3 *{rvof} Offerta alla dea (£ 1000, +1 Fa)":PRINT "{rvon}* 4 *{rvof} uscire dal Tempio":IF MQ=0 THEN PRINT "{rvon}{yel}* 5 *{rvof}{wht} La missione della sacerdotessa"
7725 GOSUB 2760:IF N>5 OR (N=5 AND MQ=1) THEN PRINT "{up}{up}":GOTO 7725
7726 ON N GOTO 7730,7740,7750,7700,8700
7730 MX=MF+MD:IF MX=0 THEN PRINT "{down}Non c'e' nulla da purificare in te.":GOTO 7758
7731 MC=600*MX:IF DN<MC THEN PRINT "{down}Servono £";MC;"{left}: non ne hai abbastanza.":GOTO 7758
7732 DN=DN-MC:FO=FO+MF:AT=AT+5*MF:DE=DE+MD:MF=0:MD=0:POKE 56833,8:PRINT "{down}{lgrn}La luce della dea ti avvolge:":PRINT "hai ritrovato le forze perdute !{wht}":GOSUB 160:GOTO 7758
7740 IF UK=1 THEN PRINT "{down}La tua arma e' gia' benedetta.":GOTO 7758
7741 IF DN<1500 THEN PRINT "{down}Non hai abbastanza denaro.":GOTO 7758
7742 DN=DN-1500:UK=1:POKE 56833,8:PRINT "{down}{lblu}La sacerdotessa benedice la tua arma:":PRINT "ora risplende di luce argentea !{wht}":GOTO 7758
7750 IF DN<1000 THEN PRINT "{down}Non hai abbastanza denaro.":GOTO 7758
7751 IF FA>19 THEN PRINT "{down}La dea gradisce, ma il tuo Fascino":PRINT "e' gia' al massimo.":GOTO 7758
7752 DN=DN-1000:FA=FA+1:POKE 56833,6:PRINT "{down}La dea gradisce l'offerta:":PRINT "il tuo Fascino sale a";FA
7758 PRINT "{down}Possiedi £";DN:GOSUB 4580:GOTO 7720
' --- il Guaritore
7760 MP$="guaritore":POKE 56834,1:PRINT "{clr}La capanna del {rvon}GUARITORE{rvof}":PRINT "{down}Erbe appese, ossa e pozioni fumanti."
7761 PRINT "{down}{rvon}* 1 *{rvof} Curare le ferite  £ 200":PRINT "{down}{rvon}* 2 *{rvof} Rito della forza vitale":PRINT "    +1 Robustezza massima  £ 4000":PRINT "{down}{rvon}* 3 *{rvof} uscire"
7762 GOSUB 2760:IF N>3 THEN PRINT "{up}{up}":GOTO 7762
7763 ON N GOTO 7765,7770,7700
7765 IF RF>=RO THEN PRINT "{down}Non hai ferite da curare.":GOTO 7778
7766 IF DN<200 THEN PRINT "{down}Non hai abbastanza denaro.":GOTO 7778
7767 DN=DN-200:RF=RO:POKE 56833,5:PRINT "{down}Le erbe del guaritore ti risanano:":PRINT "sei a Ro";RF:GOTO 7778
7770 IF RO>19 THEN PRINT "{down}La tua Robustezza e' gia' al massimo.":GOTO 7778
7771 IF DN<4000 THEN PRINT "{down}Non hai abbastanza denaro.":GOTO 7778
7772 DN=DN-4000:RO=RO+1:RF=RF+1:POKE 56833,6:PRINT "{down}Il rito ti rende piu' resistente:":PRINT "Robustezza massima";RO
7778 PRINT "{down}Possiedi £";DN:GOSUB 4580:GOTO 7760
7790 SU=1:GOSUB 340:SU=0:GOTO 7700
7795 TK=TK+2:POKE 56832,3:PRINT "{clr}Riprendi la strada di casa...":GOSUB 50:IM$="":NM$="":GOSUB 3010:GOSUB 160:GOSUB 230:PRINT "{down}Dopo 2 settimane sei di nuovo a casa.":GOSUB 4580:GOTO 340
' --- per mare verso Kebir
7650 IF DN<400 THEN PRINT "{clr}Il capitano vuole £ 400 per il viaggio":PRINT "e tu non li hai.":GOSUB 4580:GOTO 490
7651 DN=DN-400:TK=TK+2:POKE 56832,3:MP$="kebir":POKE 56834,1:PRINT "{clr}Ti imbarchi su una nave mercantile...":GOSUB 50
7652 IF INT(3*RND(0))>0 THEN PRINT "{down}Il mare e' calmo, il viaggio tranquillo.":GOSUB 50:GOTO 7660
7653 PRINT "{down}{rvon}PIRATI delle Isole Nere !{rvof}":PRINT "Una galea nera abborda la nave !":GOSUB 50:GOSUB 2790:NM$=" pirati ":IM$="":GOSUB 4580:PRINT "{clr}":GOSUB 3150
7654 IF FU=1 THEN PRINT "{down}Ti nascondi nella stiva finche' i pirati":PRINT "se ne vanno.":GOSUB 50
7660 GOSUB 160:GOSUB 230:PRINT "{clr}":GOTO 7800
' --- Kebir
7800 POKE 56832,5:MP$="kebir":POKE 56834,1:PRINT "{clr}{yel}KEBIR{wht}, la citta' del Sultano":PRINT "{down}Minareti bianchi, vele colorate e il":PRINT "frastuono del gran mercato del porto."
7805 PRINT "{down}{rvon}* 1 *{rvof} l'Arena dei campioni":PRINT "{down}{rvon}* 2 *{rvof} il Mercato delle spezie":PRINT "{down}{rvon}* 3 *{rvof} rivedere il personaggio":PRINT "{down}{rvon}* 4 *{rvof} tornare a casa (nave £ 400)"
7810 GOSUB 2760:IF N>4 THEN PRINT "{up}{up}":GOTO 7810
7815 ON N GOTO 7820,7860,7890,7895
' --- l'Arena: duello contro il Campione del Sultano
7820 POKE 56832,4:MP$="arena":POKE 56834,1:PRINT "{clr}L'{rvon}ARENA DEI CAMPIONI{rvof}":PRINT "{down}Il pubblico urla mentre entri nella":PRINT "sabbia. Il Campione del Sultano ti":PRINT "aspetta con la scimitarra in pugno."
7821 MW=500+INT(AT*4):PRINT "{down}Premio per la vittoria: £";MW
7822 PRINT "{down}{rvon}* 1 *{rvof} combattere":PRINT "{down}{rvon}* 2 *{rvof} ritirarsi"
7823 GOSUB 2760:IF N>2 THEN PRINT "{up}{up}":GOTO 7823
7824 IF N=2 THEN 7800
7825 MS$=LA$:LA$="Campione dell'Arena":HS=0:GOSUB 3950:LA$=MS$
7826 DN=DN+MW:POKE 56833,4:PRINT "{clr}La folla ti acclama !":PRINT "{down}Ricevi £";MW;"dal Sultano.":IF FA<20 THEN FA=FA+1:PRINT "{down}Il tuo Fascino sale a";FA
7827 GOSUB 4580:GOTO 7800
' --- il Mercato delle spezie
7860 MP$="spezie":POKE 56834,1:PRINT "{clr}Il {rvon}MERCATO DELLE SPEZIE{rvof}":PRINT "{down}{rvon}* 1 *{rvof} Pane elfico +3 Ro (F3)  £ 200":PRINT "{down}{rvon}* 2 *{rvof} Filtro magico  £ 800":PRINT "{down}{rvon}* 3 *{rvof} uscire"
7861 GOSUB 2760:IF N>3 THEN PRINT "{up}{up}":GOTO 7861
7862 ON N GOTO 7865,7870,7800
7865 IF DN<200 THEN PRINT "{down}Non hai abbastanza denaro.":GOTO 7878
7866 DN=DN-200:UP=UP+1:POKE 56833,4:PRINT "{down}Hai";UP;"{left} pani elfici.":GOTO 7878
7870 IF DN<800 THEN PRINT "{down}Non hai abbastanza denaro.":GOTO 7878
7871 IF WX>=49 THEN PRINT "{down}Non hai piu' posto nella bisaccia.":GOTO 7878
7872 DN=DN-800:WX=WX+1:OS$(WX)="Filtro magico":POKE 56833,4:PRINT "{down}Hai comprato un Filtro magico."
7878 PRINT "{down}Possiedi £";DN:GOSUB 4580:GOTO 7860
7890 SU=1:GOSUB 340:SU=0:GOTO 7800
7895 IF DN<400 THEN PRINT "{down}Ti servono £ 400 per la nave.":GOSUB 4580:GOTO 7800
7896 DN=DN-400:TK=TK+2:POKE 56832,3:PRINT "{clr}Riprendi il mare verso casa...":GOSUB 50:GOSUB 160:GOSUB 230:PRINT "{down}Dopo 2 settimane sei di nuovo a casa.":GOSUB 4580:GOTO 340
