// Riconoscimento delle scelte proposte dal gioco sullo schermo C64, per renderle cliccabili.
// Schemi riconosciuti: "* n *", "(S/N)", "premi un tasto", "per bere F1",
// "Batti il numero" senza elenco (tastierino 1-9).
#pragma once
#include "screen.h"
#include <cstdint>
#include <string>
#include <vector>

struct Choice {
    int x0, y0, x1, y1;   // celle (inclusive); y0 < 0 = pulsante del tastierino (posizione in x0)
    uint8_t key;          // tasto PETSCII da inviare
    bool pad = false;
};

std::vector<Choice> detectChoices(const Screen& s);
