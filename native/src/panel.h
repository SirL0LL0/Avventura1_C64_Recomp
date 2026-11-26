// Pannello laterale dell'Espansione: una "seconda schermata" PETSCII (18x34 caratteri)
// con ritratto del soggetto corrente, nome, caratteristiche e inventario.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

class VM;

struct Picture {
    std::string name;
    std::vector<std::string> keys;     // parole che, trovate nel nome, scelgono questa immagine
    std::string title;                 // didascalia
    uint8_t chars[16 * 16];
    uint8_t colors[16 * 16];
    int bg = 0;                        // colore di sfondo del riquadro
};

// carica le immagini dal testo di art.h (pixel art 32x32 -> caratteri a quadranti)
void loadPictures(std::vector<Picture>& out);
const Picture* findPicture(const std::vector<Picture>& pics, const std::string& subject);

class Panel {
public:
    static const int W = 18, H = 34;
    uint8_t chars[W * H];
    uint8_t colors[W * H];
    int bg = 0;

    void clear(int color);
    void text(int x, int y, const std::string& ascii, int color, bool rvs = false);
    void box(int x0, int y0, int x1, int y1, int color);
    void build(VM* vm, const Picture* pic, const std::string& subject);
    void render(uint32_t* dst, int pitch) const;     // W*8 x H*8 pixel
};
