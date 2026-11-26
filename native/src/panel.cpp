#include "panel.h"
#include "art.h"
#include "font_data.h"
#include "screen.h"
#include "vm.h"
#include <cctype>
#include <cmath>
#include <cstring>
#include <sstream>

namespace {

// colori della pixel art: lettera -> colore C64
int colorOf(char c) {
    switch (c) {
        case 'k': return 0; case 'w': return 1; case 'r': return 2; case 'c': return 3;
        case 'p': return 4; case 'g': return 5; case 'b': return 6; case 'y': return 7;
        case 'o': return 8; case 'n': return 9; case 'l': return 10; case 'd': return 11;
        case 'm': return 12; case 'G': return 13; case 'B': return 14; case 'L': return 15;
    }
    return -1;   // '.' = sfondo
}

// maschera 2x2 (TL=8 TR=4 BL=2 BR=1) -> codice schermo (bit 7 = inverso)
const uint8_t QUAD[16] = {
    0x20, 0x6C, 0x7B, 0x62, 0x7C, 0xE1, 0xFF, 0xFE,   // 0..7
    0x7E, 0x7F, 0x61, 0xFC, 0xE2, 0xFB, 0xEC, 0xA0,   // 8..15
};

std::string lower(std::string s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; }

} // namespace

void loadPictures(std::vector<Picture>& out) {
    std::istringstream is(ART_TEXT);
    std::string line;
    Picture* p = nullptr;
    std::vector<std::string> rows;
    auto finish = [&]() {
        if (!p) return;
        rows.resize(32, std::string(32, '.'));
        for (auto& r : rows) r.resize(32, '.');
        for (int cy = 0; cy < 16; cy++)
            for (int cx = 0; cx < 16; cx++) {
                char px[4] = {rows[cy * 2][cx * 2], rows[cy * 2][cx * 2 + 1], rows[cy * 2 + 1][cx * 2], rows[cy * 2 + 1][cx * 2 + 1]};
                // colore di primo piano = il piu' frequente tra quelli non di sfondo
                int cnt[16] = {0}, best = -1;
                for (char c : px) { int k = colorOf(c); if (k >= 0) cnt[k]++; }
                for (int k = 0; k < 16; k++) if (cnt[k] && (best < 0 || cnt[k] > cnt[best])) best = k;
                int mask = 0;
                if (best >= 0) for (int i = 0; i < 4; i++) if (colorOf(px[i]) == best) mask |= 8 >> i;
                p->chars[cy * 16 + cx] = QUAD[mask];
                p->colors[cy * 16 + cx] = (uint8_t)(best < 0 ? 1 : best);
            }
        p = nullptr;
    };
    while (std::getline(is, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("pic ", 0) == 0) {
            finish();
            out.emplace_back();
            p = &out.back();
            rows.clear();
            // pic nome | parole chiave separate da virgole | didascalia | sfondo
            std::string rest = line.substr(4);
            std::vector<std::string> f;
            size_t s = 0, e;
            while ((e = rest.find('|', s)) != std::string::npos) { f.push_back(rest.substr(s, e - s)); s = e + 1; }
            f.push_back(rest.substr(s));
            auto trim = [](std::string x) { while (!x.empty() && x.front() == ' ') x.erase(0, 1); while (!x.empty() && x.back() == ' ') x.pop_back(); return x; };
            p->name = trim(f[0]);
            if (f.size() > 1) { std::istringstream ks(f[1]); std::string k; while (std::getline(ks, k, ',')) if (!trim(k).empty()) p->keys.push_back(lower(trim(k))); }
            p->title = f.size() > 2 ? trim(f[2]) : p->name;
            p->bg = f.size() > 3 ? colorOf(trim(f[3])[0]) : 0;
            if (p->bg < 0) p->bg = 0;
            if (p->keys.empty()) p->keys.push_back(lower(p->name));
            continue;
        }
        if (p && !line.empty() && line.find_first_not_of(".kwrcpgbyonldmGBL") == std::string::npos && rows.size() < 32) rows.push_back(line);
    }
    finish();
}

const Picture* findPicture(const std::vector<Picture>& pics, const std::string& subject) {
    std::string s = lower(subject);
    const Picture* best = nullptr;
    size_t bestLen = 0;
    for (auto& p : pics)
        for (auto& k : p.keys)
            if (s.find(k) != std::string::npos && k.size() > bestLen) { best = &p; bestLen = k.size(); }
    return best;
}

// ------------------------------------------------------------------ pannello
static uint8_t screenCode(char c) {
    if (c >= 'a' && c <= 'z') return (uint8_t)(c - 'a' + 1);
    if (c >= 'A' && c <= 'Z') return (uint8_t)(c - 'A' + 0x41);
    if (c == '@') return 0;
    if (c == '#') return 0x1C;        // £
    if (c >= 0x20 && c <= 0x3F) return (uint8_t)c;
    return '?';
}

void Panel::clear(int color) { std::memset(chars, 0x20, sizeof chars); std::memset(colors, 1, sizeof colors); bg = color; }

void Panel::text(int x, int y, const std::string& s, int color, bool rvs) {
    for (size_t i = 0; i < s.size() && x + (int)i < W; i++) {
        if (y < 0 || y >= H || x + (int)i < 0) continue;
        chars[y * W + x + i] = screenCode(s[i]) | (rvs ? 0x80 : 0);
        colors[y * W + x + i] = (uint8_t)color;
    }
}

void Panel::box(int x0, int y0, int x1, int y1, int color) {
    auto put = [&](int x, int y, uint8_t c) { chars[y * W + x] = c; colors[y * W + x] = (uint8_t)color; };
    for (int x = x0 + 1; x < x1; x++) { put(x, y0, 0x40); put(x, y1, 0x40); }
    for (int y = y0 + 1; y < y1; y++) { put(x0, y, 0x5D); put(x1, y, 0x5D); }
    put(x0, y0, 0x70); put(x1, y0, 0x6E); put(x0, y1, 0x6D); put(x1, y1, 0x7D);
}

static std::string asciiOf(const std::string& pet) {
    std::string o;
    for (unsigned char c : pet) {
        if (c >= 0x41 && c <= 0x5A) o += (char)(c + 32);
        else if (c >= 0xC1 && c <= 0xDA) o += (char)(c - 0x80);
        else if (c >= 0x20 && c <= 0x40) o += (char)c;
    }
    while (!o.empty() && o.front() == ' ') o.erase(0, 1);
    while (!o.empty() && o.back() == ' ') o.pop_back();
    return o;
}

void Panel::build(VM* vm, const Picture* pic, const std::string& subject) {
    clear(0);
    // cornice del ritratto
    box(0, 0, 17, 17, 12);
    if (pic) {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                chars[(y + 1) * W + x + 1] = pic->chars[y * 16 + x];
                colors[(y + 1) * W + x + 1] = pic->colors[y * 16 + x];
            }
    } else {
        text(3, 7, "AVVENTURA 1", 11);
        text(4, 9, "ESPANSIONE", 12);
    }
    std::string cap = pic ? pic->title : "";
    if (!subject.empty() && pic && pic->title.empty()) cap = asciiOf(subject);
    if (cap.size() > 18) cap = cap.substr(0, 18);
    text((W - (int)cap.size()) / 2, 18, cap, 7);
    if (!vm) return;
    // scheda del personaggio
    std::string np;
    double v;
    vm->varString("NP$", np);
    std::string name = asciiOf(np);
    if (name.empty()) name = "Avventuriero";
    box(0, 20, 17, 33, 14);
    text(2, 20, " " + name.substr(0, 13) + " ", 1, true);
    auto num = [&](const char* n) { return vm->varNumber(n, v) ? (int)v : 0; };
    char buf[40];
    std::snprintf(buf, sizeof buf, "Fo%3d  In%3d", num("FO"), num("IN")); text(2, 22, buf, 15);
    std::snprintf(buf, sizeof buf, "De%3d  Fr%3d", num("DE"), num("FR")); text(2, 23, buf, 15);
    std::snprintf(buf, sizeof buf, "Fa%3d  At%3d%%", num("FA"), num("AT")); text(2, 24, buf, 15);
    int rf = num("RF"), ro = num("RO");
    std::snprintf(buf, sizeof buf, "Ro %2d/%-2d", rf, ro); text(2, 26, buf, 1);
    // barra della robustezza
    int bar = ro > 0 ? (rf * 12 + ro / 2) / ro : 0;
    if (bar < 0) bar = 0;
    if (bar > 12) bar = 12;
    int bc = rf * 3 <= ro ? 2 : rf * 3 <= ro * 2 ? 7 : 5;
    for (int i = 0; i < 12; i++) { chars[27 * W + 3 + i] = i < bar ? 0xA0 : 0x66; colors[27 * W + 3 + i] = (uint8_t)(i < bar ? bc : 11); }
    std::snprintf(buf, sizeof buf, "#%-7d  S.%2d", num("DN"), num("TK")); text(2, 29, buf, 7);
    // equipaggiamento della mod (se presente)
    std::string arma, armat;
    if (vm->varString("UA$", arma) && !arma.empty()) text(2, 31, asciiOf(arma).substr(0, 14), 3);
    if (vm->varString("UR$", armat) && !armat.empty()) text(2, 32, asciiOf(armat).substr(0, 14), 3);
}

void Panel::render(uint32_t* dst, int pitch) const {
    const uint32_t bgc = 0xFF000000 | C64_PALETTE[bg & 15];
    for (int r = 0; r < H; r++)
        for (int c = 0; c < W; c++) {
            int i = r * W + c;
            uint8_t code = chars[i];
            bool inv = code & 0x80;
            uint32_t fg = 0xFF000000 | C64_PALETTE[colors[i] & 15];
            const uint8_t* g = FONT_LOWER[code & 0x7F];
            for (int yy = 0; yy < 8; yy++) {
                uint8_t bits = inv ? (uint8_t)~g[yy] : g[yy];
                uint32_t* p = dst + (r * 8 + yy) * pitch + c * 8;
                for (int xx = 0; xx < 8; xx++) p[xx] = (bits & (0x80 >> xx)) ? fg : bgc;
            }
        }
}
