!NAME Espansione
' ============================================================================
' Avventura 1 - Espansione (mod). Codice originale del progetto.
' Registri di espansione (POKE 56832+n):
'   56832 musica: 1 titolo 2 taverna 3 viaggio 4 battaglia 5 citta 6 tempio 7 eroe 8 morte
'   56833 effetto: 1 colpo 2 ferita 3 mancato 4 moneta 5 pozione 6 livello 7 clic 8 magia 9 porta
'   56834 immagine: 1 = mostra il soggetto scritto in MP$, 0 = nascondi
' Variabili della mod: solo prefissi M e U.
' ============================================================================
!HOOK 10 GOSUB 7000
!HOOK 340 GOSUB 7100
!HOOK 490 GOSUB 7100
!HOOK 760 GOSUB 7111
!HOOK 850 GOSUB 7112
!HOOK 1760 GOSUB 7113
!HOOK 2090 GOSUB 7114
!HOOK 1220 GOSUB 7120
!HOOK 1260 GOSUB 7121
!HOOK 1470 GOSUB 7122
!HOOK 3150 GOSUB 7130
!HOOK 3950 GOSUB 7135
!HOOK 3540 GOSUB 7140
!HOOK 4220 GOSUB 7140
!HOOK 2670 GOSUB 7150
!HOOK 2680 GOSUB 7160
' --- inizializzazione (a ogni nuova partita, prima della riga 10)
7000 MV=1:POKE 56832,1:MP$="":POKE 56834,0:RETURN
' --- taverna e scheda del personaggio
7100 POKE 56832,2:MP$="taverna":POKE 56834,1:RETURN
' --- partenze: caccia, carovana, missione, Barone
7111 POKE 56832,3:MP$=LA$:POKE 56834,1:RETURN
7112 POKE 56832,3:MP$="carovana":POKE 56834,1:RETURN
7113 POKE 56832,3:MP$=ZH$:POKE 56834,1:RETURN
7114 POKE 56832,3:MP$="briganti":POKE 56834,1:RETURN
' --- Babbakesch e negozi
7120 POKE 56832,5:MP$="babbakesch":POKE 56834,1:RETURN
7121 MP$="spada":POKE 56834,1:RETURN
7122 MP$="filtro":POKE 56834,1:RETURN
' --- combattimenti
7130 POKE 56832,4:MP$=NM$+IM$:POKE 56834,1:RETURN
7135 POKE 56832,4:MP$=LA$:IF LA$="" THEN MP$="bandito"
7136 POKE 56834,1:RETURN
7140 POKE 56832,3:RETURN
' --- fine partita
7150 POKE 56832,7:MP$="eroe":POKE 56834,1:RETURN
7160 IF RF<=0 OR FO<=0 OR DE<=0 THEN POKE 56832,8:MP$="sei morto":POKE 56834,1
7170 RETURN
