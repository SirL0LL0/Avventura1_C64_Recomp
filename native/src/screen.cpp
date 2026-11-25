#include "screen.h"
#include "font_data.h"
#include <cstring>

// tavolozza "Pepto"
const uint32_t C64_PALETTE[16] = {0x000000, 0xffffff, 0x68372b, 0x70a4b2, 0x6f3d86, 0x588d43, 0x352879, 0xb8c76f,
                                  0x6f4f25, 0x433900, 0x9a6759, 0x444444, 0x6c6c6c, 0x9ad284, 0x6c5eb5, 0x959595};

int petsciiToScreen(uint8_t b) {
    if (b < 0x20) return -1;
    if (b < 0x40) return b;
    if (b < 0x60) return b - 0x40;
    if (b < 0x80) return b - 0x20;
    if (b < 0xA0) return -1;
    if (b < 0xC0) return b - 0x40;
    if (b < 0xFF) return b - 0x80;
    return 0x5E;
}

static int colorCode(uint8_t b) {
    switch (b) {
        case 0x90: return 0; case 0x05: return 1; case 0x1C: return 2; case 0x9F: return 3;
        case 0x9C: return 4; case 0x1E: return 5; case 0x1F: return 6; case 0x9E: return 7;
        case 0x81: return 8; case 0x95: return 9; case 0x96: return 10; case 0x97: return 11;
        case 0x98: return 12; case 0x99: return 13; case 0x9A: return 14; case 0x9B: return 15;
    }
    return -1;
}

void Screen::clear() {
    std::memset(chars, 0x20, sizeof chars);
    std::memset(colors, color, sizeof colors);
    std::memset(link, 0, sizeof link);
    x = y = 0;
}

void Screen::scrollUp() {
    std::memmove(chars, chars + W, W * (H - 1));
    std::memmove(colors, colors + W, W * (H - 1));
    std::memset(chars + W * (H - 1), 0x20, W);
    std::memset(colors + W * (H - 1), color, W);
    std::memmove(link, link + 1, H - 1);
    link[H - 1] = 0; link[0] = 0;
}

void Screen::newRow() { if (y < H - 1) y++; else scrollUp(); }

int Screen::col() const { return (y - lineStart(y)) * W + x; }

void Screen::put(uint8_t code) {
    int i = y * W + x;
    chars[i] = code | (rvs ? 0x80 : 0);
    colors[i] = (uint8_t)color;
    if (++x >= W) {
        x = 0;
        bool growing = (y == lineStart(y));
        newRow();
        link[y] = growing ? 1 : 0;
    }
}

void Screen::cr() {
    y = lineEnd(y);
    x = 0;
    rvs = false;
    newRow();
    link[y] = 0;
}

void Screen::out(uint8_t b) {
    int c = colorCode(b);
    if (c >= 0) { color = c; return; }
    switch (b) {
        case 0x0D: case 0x8D: cr(); return;
        case 0x0E: lower = true; return;
        case 0x8E: lower = false; return;
        case 0x12: rvs = true; return;
        case 0x92: rvs = false; return;
        case 0x13: x = y = 0; return;
        case 0x93: clear(); return;
        case 0x11: newRow(); return;
        case 0x91: if (y > 0) y--; return;
        case 0x1D: if (++x >= W) { x = 0; newRow(); } return;
        case 0x9D: if (x > 0) x--; else if (y > 0) { y--; x = W - 1; } return;
        case 0x14: {
            if (x == 0 && y == 0) return;
            out(0x9D);
            int i = y * W + x;
            std::memmove(chars + i, chars + i + 1, W - 1 - x);
            chars[y * W + W - 1] = 0x20;
            return;
        }
        case 0x94: {
            int i = y * W + x;
            std::memmove(chars + i + 1, chars + i, W - 1 - x);
            chars[i] = 0x20;
            return;
        }
    }
    int sc = petsciiToScreen(b);
    if (sc >= 0) put((uint8_t)sc);
}

void Screen::render(uint32_t* dst, int pitch, bool cursor, uint64_t ms) const {
    const uint32_t bc = 0xFF000000 | C64_PALETTE[border & 15], bgc = 0xFF000000 | C64_PALETTE[bg & 15];
    for (int py = 0; py < PIX_H; py++) {
        uint32_t* row = dst + py * pitch;
        for (int px = 0; px < PIX_W; px++)
            if (px < BORDER_X || px >= BORDER_X + 320 || py < BORDER_Y || py >= BORDER_Y + 200) row[px] = bc;
    }
    const uint8_t(*font)[8] = lower ? FONT_LOWER : FONT_UPPER;
    bool curOn = cursor && ((ms / 333) & 1);
    for (int r = 0; r < H; r++)
        for (int c = 0; c < W; c++) {
            int i = r * W + c;
            uint8_t code = chars[i];
            bool inv = code & 0x80, here = curOn && r == y && c == x;
            if (here) inv = !inv;
            uint32_t fg = 0xFF000000 | C64_PALETTE[(here ? color : colors[i]) & 15];
            const uint8_t* g = font[code & 0x7F];
            for (int yy = 0; yy < 8; yy++) {
                uint8_t bits = inv ? (uint8_t)~g[yy] : g[yy];
                uint32_t* p = dst + (BORDER_Y + r * 8 + yy) * pitch + BORDER_X + c * 8;
                for (int xx = 0; xx < 8; xx++) p[xx] = (bits & (0x80 >> xx)) ? fg : bgc;
            }
        }
}
