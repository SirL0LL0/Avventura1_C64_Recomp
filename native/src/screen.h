// Schermo testo del C64 40x25 con editor di schermo semplificato (righe logiche, colori, inverso).
#pragma once
#include <cstdint>

extern const uint32_t C64_PALETTE[16];   // 0xRRGGBB

class Screen {
public:
    static const int W = 40, H = 25;
    static const int PIX_W = 384, PIX_H = 272, BORDER_X = 32, BORDER_Y = 36;

    uint8_t chars[W * H];
    uint8_t colors[W * H];
    uint8_t link[H];
    int border = 14, bg = 6, color = 14;
    bool lower = false, rvs = false;
    int x = 0, y = 0;

    Screen() { clear(); }
    void clear();
    void out(uint8_t b);            // CHROUT
    void cr();
    int col() const;                // colonna logica (0-79)
    // disegna 384x272 pixel ARGB8888 in dst (pitch in pixel)
    void render(uint32_t* dst, int pitch, bool cursor, uint64_t ms) const;

private:
    int lineStart(int r) const { while (r > 0 && link[r]) r--; return r; }
    int lineEnd(int r) const { while (r < H - 1 && link[r + 1]) r++; return r; }
    void scrollUp();
    void newRow();
    void put(uint8_t code);
};

int petsciiToScreen(uint8_t b);
