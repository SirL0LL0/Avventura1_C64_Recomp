// Lettura del nastro .tap dell'utente e ricostruzione della memoria del gioco dopo il boot.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct GameImage {
    std::vector<uint8_t> mem;   // 64 KB
    uint16_t entry = 0;
    uint16_t end = 0;           // primo indirizzo dopo il programma
};

// Ritorna false e imposta err se il file non e' valido.
bool loadGameFromTap(const std::vector<uint8_t>& tap, GameImage& out, std::string& err);
// Controllo di integrita' del layout del programma compilato (header, P-code, variabili).
bool validateImage(const GameImage& g, std::string& err);
