#include "panel.h"
#include "art_data.h"
#include "font_data.h"
#include "screen.h"
#include "vm.h"
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <sstream>

namespace {
std::string lower(std::string s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; }
int hexDigit(char c) { return c >= 'a' ? c - 'a' + 10 : c - '0'; }
} // namespace

void loadPictures(std::vector<Picture>& out) {
    for (int i = 0; i < ART_COUNT; i++) {
        const ArtDef& d = ART_DEFS[i];
        Picture p;
        p.name = d.name;
        p.title = d.title;
        p.w = d.w; p.h = d.h;
        p.px.resize((size_t)d.w * d.h);
        for (size_t k = 0; k < p.px.size() && d.px[k]; k++) p.px[k] = (uint8_t)hexDigit(d.px[k]);
        std::istringstream ks(d.keys);
        std::string k;
        while (std::getline(ks, k, ',')) if (!k.empty()) p.keys.push_back(lower(k));
        out.push_back(std::move(p));
    }
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

const Picture* pictureByName(const std::vector<Picture>& pics, const std::string& name) {
    for (auto& p : pics) if (p.name == name) return &p;
    return nullptr;
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
    pic_ = pic && pic->w == 128 && pic->h == 128 ? pic : nullptr;
    // cornice dell'illustrazione (l'immagine viene disegnata in render)
    box(0, 0, 17, 17, 12);
    if (!pic_) {
        text(3, 7, "AVVENTURA 1", 11);
        text(4, 9, "ESPANSIONE", 12);
    }
    std::string cap = pic_ ? pic_->title : "";
    if (!subject.empty() && pic_ && pic_->title.empty()) cap = asciiOf(subject);
    if (cap.size() > 18) cap = cap.substr(0, 18);
    text((W - (int)cap.size()) / 2, 18, cap, 7);
    for (auto& it : items) { it.count = 0; it.usable = false; it.px = it.py = 0; }
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
    std::snprintf(buf, sizeof buf, "Fo%3d  In%3d", num("FO"), num("IN")); text(2, 21, buf, 15);
    std::snprintf(buf, sizeof buf, "De%3d  Fr%3d", num("DE"), num("FR")); text(2, 22, buf, 15);
    std::snprintf(buf, sizeof buf, "Fa%3d  At%3d%%", num("FA"), num("AT")); text(2, 23, buf, 15);
    int rf = num("RF"), ro = num("RO");
    std::snprintf(buf, sizeof buf, "Ro %2d/%-2d", rf, ro); text(2, 24, buf, 1);
    // barra della robustezza
    int bar = ro > 0 ? (rf * 12 + ro / 2) / ro : 0;
    if (bar < 0) bar = 0;
    if (bar > 12) bar = 12;
    int bc = rf * 3 <= ro ? 2 : rf * 3 <= ro * 2 ? 7 : 5;
    for (int i = 0; i < 12; i++) { chars[25 * W + 3 + i] = i < bar ? 0xA0 : 0x66; colors[25 * W + 3 + i] = (uint8_t)(i < bar ? bc : 11); }
    std::snprintf(buf, sizeof buf, "#%-7d  S.%2d", num("DN"), num("TK")); text(2, 26, buf, 7);
    // equipaggiamento della mod (se presente)
    std::string arma, armat;
    bool blessed = vm->varNumber("UK", v) && v == 1;
    // oggetti: Filtri magici (iniziano per F, come nel gioco) e Spada magica (S, "+NN")
    std::vector<std::string> os;
    int filtri = 0, spada = 0, wx = num("WX");
    if (vm->arrayStrings("OS$", os))
        for (int i = 1; i <= wx && i < (int)os.size(); i++) {
            if (os[i].empty()) continue;
            uint8_t c0 = (uint8_t)os[i][0];
            if (c0 == 0xC6 || c0 == 'F') filtri++;
            if (c0 == 0xD3 || c0 == 'S') { size_t p = os[i].find('+'); if (p != std::string::npos) spada = std::atoi(os[i].c_str() + p + 1); }
        }
    const char* sname = spada == 50 ? "Lama di Fiamma" : spada == 75 ? "Spada del Vento" : spada == 100 ? "Spada dei Re" : spada ? "Spada Magica" : "";
    int row = 27;
    if (*sname) text(2, row++, sname, blessed ? 14 : 7);
    if (vm->varString("UA$", arma) && !arma.empty()) text(2, row++, asciiOf(arma).substr(0, 14), blessed && !*sname ? 14 : 3);
    if (blessed && row == 27) text(2, row++, "arma benedetta", 14);
    if (vm->varString("UR$", armat) && !armat.empty() && row < 30) text(2, row++, asciiOf(armat).substr(0, 14), 3);
    items[0].count = filtri; items[0].key = 0x85; items[0].keyName = "F1"; items[0].name = "Filtro magico";
    items[1].count = vm->varNumber("UP", v) ? (int)v : 0; items[1].key = 0x86; items[1].keyName = "F3"; items[1].name = "Pane elfico";
    for (int i = 0; i < 2; i++) {
        Consumable& it = items[i];
        int cx = 2 + i * 8;
        it.px = cx * 8; it.py = 30 * 8;
        std::snprintf(buf, sizeof buf, "x%d", it.count);
        text(cx + 2, 30, buf, it.count ? 1 : 11);
        text(cx + 2, 31, it.keyName, it.count ? 13 : 11);
        if (it.count) text(2, 32, "clic per usare", 11);
    }
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
    auto blit = [&](const Picture* p, int ox, int oy, bool dim) {
        for (int y = 0; y < p->h; y++)
            for (int x = 0; x < p->w; x++) {
                int c = p->px[y * p->w + x];
                if (dim && c) c = 11;
                dst[(oy + y) * pitch + ox + x] = 0xFF000000 | C64_PALETTE[c & 15];
            }
    };
    if (pic_) blit(pic_, 8, 8, false);
    for (auto& it : items) if (it.icon && it.py > 0) blit(it.icon, it.px, it.py, it.count == 0);
}