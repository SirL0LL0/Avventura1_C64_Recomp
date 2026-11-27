' ============================================================================
' Viaggi verso terre lontane: Kho (Tempio di LYSS, Guaritore) e Kebir (Arena, Mercato).
'   UK  arma benedetta (protegge dalle maledizioni, ferisce i non-morti)
'   MF/MD  punti di Forza/Destrezza tolti dalle maledizioni (il Tempio li ridona)
' ============================================================================
'   UV  offerta 3 della taverna: 0 la Carovana originale, 1 Kho, 2 Kebir, 3 il pellegrino
'       (un solo viaggio per volta, a sorte, al posto della Carovana)
!HOOK 560 GOTO 7590
!HOOK 680 GOTO 7605
!HOOK 3430 GOTO 7695
' --- le offerte della taverna (riga 560 originale, con la terza a sorte)
7590 UV=0:IF INT(3*RND(0))=0 THEN UV=INT(3*RND(0))+1
7591 PRINT "{clr}{up}{rvon}* 1 *{rvof} Un ";LA$;" che":PRINT "vive presso ";LB$:PRINT "terrorizza gli abitanti della regione ;":PRINT "taglia di {rvon}{yel}£";A9;"{left}{wht}{rvof} a chi lo uccidera'.":GOSUB 50:GOSUB 570:IF UV>0 THEN ON UV GOTO 7595,7596,7597
7592 PRINT "{down}{rvon}* 3 *{rvof} Sta' per partire la Carovana":PRINT "che tutti i mesi si reca nella favolosa":PRINT "citta' di {rvon}{lgrn}Babbakesch{rvof}{wht} ,dove vengono fab-":PRINT "bricate armi e oggetti magici !":GOSUB 50:GOTO 620
7595 PRINT "{down}{rvon}{lgrn}* 3 *{rvof}{wht} Parte una carovana di mercanti":PRINT "per {lgrn}Kho{wht}, villaggio ai bordi della":PRINT "giungla dove sorge il Tempio di LYSS.":PRINT "Due settimane di cammino.":GOSUB 50:GOTO 620
7596 PRINT "{down}{rvon}{yel}* 3 *{rvof}{wht} Una nave mercantile salpa per":PRINT "{yel}Kebir{wht}, la citta' del Sultano: Arena e":PRINT "gran mercato: £ 400 o lavori a bordo,":PRINT "due settimane di mare.":GOSUB 50:GOTO 620
7597 PRINT "{down}{rvon}{lblu}* 3 *{rvof}{wht} Fratello Anselmo, un pellegrino,":PRINT "cerca una scorta fino al Tempio di LYSS":PRINT "a Kho: 3 settimane, compenso £ 800.":GOSUB 50:GOTO 620
7605 IF N=3 AND UV>0 THEN ON UV GOTO 7620,7650,8900
7606 ON N GOTO 760,1660,850,2090,690
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
7722 PRINT "{down}{down}{rvon}* 1 *{rvof} I riti della dea":PRINT "{down}{rvon}{yel}* 2 *{rvof}{wht} Le missioni della sacerdotessa":PRINT "{down}{rvon}* 3 *{rvof} uscire dal Tempio"
7725 GOSUB 2760:IF N>3 THEN PRINT "{up}{up}":GOTO 7725
7726 ON N GOTO 7727,9000,7700
' --- i riti della dea
7727 PRINT "{clr}{rvon}I RITI DELLA DEA{rvof}":PRINT "{down}{rvon}* 1 *{rvof} Purificazione  {yel}£ 600{wht} a punto":PRINT "    ridona Forza e Destrezza perdute":PRINT "{down}{rvon}* 2 *{rvof} Benedizione dell'arma  {yel}£ 1500{wht}":PRINT "    ripara dalle maledizioni e ferisce":PRINT "    i non-morti"
7728 PRINT "{down}{rvon}* 3 *{rvof} Offerta alla dea  {yel}£ 1000{wht}":PRINT "    +1 Fascino":PRINT "{down}{rvon}* 4 *{rvof} tornare nell'atrio"
7729 GOSUB 2760:ON N GOTO 7730,7740,7750,7720:PRINT "{up}{up}":GOTO 7729
7730 MX=MF+MD:IF MX=0 THEN PRINT "{down}Non c'e' nulla da purificare in te.":GOTO 7758
7731 MC=600*MX:IF DN<MC THEN PRINT "{down}Servono £";MC;"{left}: non ne hai abbastanza.":GOTO 7758
7732 DN=DN-MC:FO=FO+MF:AT=AT+5*MF:DE=DE+MD:MF=0:MD=0:POKE 56833,8:PRINT "{down}{lgrn}La luce della dea ti avvolge:":PRINT "hai ritrovato le forze perdute !{wht}":GOSUB 160:GOTO 7758
7740 IF UK=1 THEN PRINT "{down}La tua arma e' gia' benedetta.":GOTO 7758
7741 IF DN<1500 THEN PRINT "{down}Non hai abbastanza denaro.":GOTO 7758
7742 DN=DN-1500:UK=1:POKE 56833,8:PRINT "{down}{lblu}La sacerdotessa benedice la tua arma:":PRINT "ora risplende di luce argentea !{wht}":GOTO 7758
7750 IF DN<1000 THEN PRINT "{down}Non hai abbastanza denaro.":GOTO 7758
7751 IF FA>19 THEN PRINT "{down}La dea gradisce, ma il tuo Fascino":PRINT "e' gia' al massimo.":GOTO 7758
7752 DN=DN-1000:FA=FA+1:POKE 56833,6:PRINT "{down}La dea gradisce l'offerta:":PRINT "il tuo Fascino sale a";FA
7758 PRINT "{down}Possiedi £";DN:GOSUB 4580:GOTO 7727
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
' --- per mare: passaggio pagato o lavoro nella ciurma (UD 1 andata, 2 ritorno; UC 1 ciurma)
'     la ciurma non puo' fuggire dalla mischia, ma sceglie una manovra navale con un tiro di dado
7650 MP$="nave":POKE 56834,1:PRINT "{clr}Al porto una nave mercantile salpa":PRINT "per {yel}Kebir{wht}: due settimane di mare.":UD=1:GOTO 7655
7655 PRINT "{down}{rvon}* 1 *{rvof} pagare il passaggio  {yel}£ 400{wht}":PRINT "{down}{rvon}* 2 *{rvof} arruolarti nella ciurma":PRINT "    niente fuga, ma manovre navali":PRINT "    e una parte del bottino":PRINT "{down}{rvon}* 3 *{rvof} rinunciare"
7656 GOSUB 2760:ON N GOTO 7660,7662,7664:PRINT "{up}{up}":GOTO 7656
7660 IF DN<400 THEN PRINT "{down}Non hai le £ 400 del passaggio.":GOSUB 4580:ON UD GOTO 490,7800
7661 DN=DN-400:UC=0:GOTO 7666
7662 UC=1:PRINT "{down}Il nostromo ti squadra: 'A bordo, e":PRINT "niente lamentele !'":GOSUB 50:GOTO 7666
7664 ON UD GOTO 490,7800
7666 TK=TK+2:POKE 56832,3:PRINT "{clr}La nave prende il largo...":GOSUB 50:IF INT(3*RND(0))>0 THEN PRINT "{down}Il mare e' calmo: viaggio tranquillo.":GOSUB 50:GOTO 7690
7667 MP$="pirati":POKE 56834,1:PRINT "{down}{rvon}PIRATI delle Isole Nere !{rvof}":PRINT "Una galea nera punta sulla nave !":GOSUB 50:GOSUB 2790:IF UC=0 THEN 7685
7668 GOSUB 4580:PRINT "{clr}Il capitano ti chiama al timone:":PRINT "{down}{rvon}* 1 *{rvof} virare sottovento":PRINT "    (Destrezza)":PRINT "{down}{rvon}* 2 *{rvof} caricare la balista":PRINT "    (Forza)":PRINT "{down}{rvon}* 3 *{rvof} speronare la galea":PRINT "    (Fortuna)"
7669 GOSUB 2760:ON N GOTO 7670,7673,7676:PRINT "{up}{up}":GOTO 7669
7670 MW=DE:GOSUB 7680:IF ZZ=1 THEN NM=INT(NM/2)+1:PRINT "{down}{lgrn}La nave sfugge alla galea: solo";NM:PRINT "pirati riescono ad abbordare.{wht}":GOTO 7685
7671 PRINT "{down}La virata e' lenta: la galea vi":PRINT "abborda con tutti i pirati !":GOTO 7685
7673 MW=FO:GOSUB 7680:IF ZZ=1 THEN MX=INT(NM/3)+1:NM=NM-MX:PRINT "{down}{lgrn}La balista abbatte";MX;"{left} pirati !{wht}":GOTO 7685
7674 RF=RF-1:POKE 56833,2:PRINT "{down}La balista s'inceppa e ti ferisce:":PRINT "Robustezza";RF:IF RF<=0 THEN PRINT "{down}Sei morto !":GOTO 2680
7675 GOTO 7685
7676 MW=FR:GOSUB 7680:IF ZZ=1 THEN PRINT "{down}{lgrn}Lo sperone squarcia la galea, che":PRINT "affonda con tutti i pirati !{wht}":DN=DN+300:POKE 56833,4:PRINT "Ripescate un forziere: £ 300 a te.":GOSUB 50:GOTO 7690
7677 NM=NM+2:RF=RF-2:POKE 56833,2:PRINT "{down}L'urto ti getta a terra: 2 ferite,":PRINT "e i pirati abbordano in";NM;"{left} !":IF RF<=0 THEN PRINT "{down}Sei morto !":GOTO 2680
7678 GOTO 7685
' tiro di dado della manovra: MW = caratteristica, ZZ = 1 se riesce
7680 PRINT "{down}Devi tirare non piu' di";MW;"{left} su 21.":GOSUB 20:GOSUB 50:ZZ=0:IF MW>=D6 THEN ZZ=1
7681 RETURN
7685 NM$=" pirati ":IM$="":GOSUB 4580:PRINT "{clr}":GOSUB 3150
7686 IF FU=1 THEN PRINT "{down}Ti nascondi nella stiva finche' i":PRINT "pirati se ne vanno.":GOSUB 50:GOTO 7690
7687 IF UC=1 THEN DN=DN+300:POKE 56833,4:PRINT "{down}La tua parte del bottino: £ 300.":GOSUB 50
7690 GOSUB 160:GOSUB 230:UC=0:IF UD=1 THEN PRINT "{clr}":GOTO 7800
7691 PRINT "{down}Dopo 2 settimane sei di nuovo a casa.":GOSUB 4580:GOTO 340
' --- in mischia: chi e' della ciurma non puo' fuggire (riga 3430 originale)
7695 IF UC=1 THEN PRINT "{down}Della ciurma non si fugge: combatti !":GOSUB 240:GOTO 3510
7696 GOSUB 60:PRINT "{down}Puoi fare la scelta di :":PRINT "* 1 * Tentare la fuga":PRINT "* 2 * Continuare il combattimento":GOTO 3440
' --- Kebir
7800 POKE 56832,5:MP$="kebir":POKE 56834,1:PRINT "{clr}{yel}KEBIR{wht}, la citta' del Sultano":PRINT "{down}Minareti bianchi, vele colorate e il":PRINT "frastuono del gran mercato del porto."
7805 PRINT "{down}{rvon}* 1 *{rvof} l'Arena dei campioni":PRINT "{down}{rvon}* 2 *{rvof} il Mercato delle spezie":PRINT "{down}{rvon}{yel}* 3 *{rvof}{wht} il banditore del Sultano":PRINT "{down}{rvon}* 4 *{rvof} rivedere il personaggio":PRINT "{down}{rvon}* 5 *{rvof} tornare a casa per mare"
7810 GOSUB 2760:IF N>5 THEN PRINT "{up}{up}":GOTO 7810
7815 ON N GOTO 7820,7860,9100,7890,7895
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
7895 MP$="nave":POKE 56834,1:PRINT "{clr}Al porto di Kebir una nave salpa":PRINT "verso casa: due settimane di mare.":UD=2:GOTO 7655
