#include "choices.h"
#include <cctype>

namespace {

// riga di schermo -> testo minuscolo (ignora l'inverso)
std::string rowText(const Screen& s, int y) {
    std::string t(Screen::W, ' ');
    for (int x = 0; x < Screen::W; x++) {
        int c = s.chars[y * Screen::W + x] & 0x7F;
        char ch = ' ';
        if (s.lower) {
            if (c >= 1 && c <= 26) ch = (char)('a' + c - 1);
            else if (c >= 0x41 && c <= 0x5A) ch = (char)('a' + c - 0x41);
        } else if (c >= 1 && c <= 26) ch = (char)('a' + c - 1);
        if (c >= 0x20 && c <= 0x3F) ch = (char)c;
        t[x] = ch;
    }
    return t;
}

// una cella "si vede" se contiene un carattere o e' colorata in inverso (barre, cornici)
bool reversed(const Screen& s, int x, int y) { return s.chars[y * Screen::W + x] & 0x80; }
bool visible(const Screen& s, int x, int y) { return reversed(s, x, y) || (s.chars[y * Screen::W + x] & 0x7F) != 0x20; }
// Un "segmento" e' un gruppo di celle visibili separato dal resto da almeno 3 celle vuote:
// cosi' la scelta non si allarga a disegni vicini (es. le torri del castello nel menu).
// Dopo uno spazio, un blocco in inverso che segue testo normale e' un altro disegno: ci si ferma.
int segEnd(const Screen& s, int y, int x) {
    int last = x, gap = 0;
    for (int i = x; i < Screen::W; i++) {
        if (visible(s, i, y)) {
            if (gap > 0 && reversed(s, i, y) && !reversed(s, last, y)) break;
            last = i; gap = 0;
        } else if (++gap >= 3) break;
    }
    return last;
}
int segStart(const Screen& s, int y, int x) {
    int first = x, gap = 0;
    for (int i = x; i >= 0; i--) {
        if (visible(s, i, y)) {
            if (gap > 0 && reversed(s, i, y) && !reversed(s, first, y)) break;
            first = i; gap = 0;
        } else if (++gap >= 3) break;
    }
    return first;
}
// prima cella visibile nell'intervallo [x0,x1], o -1
int firstIn(const Screen& s, int y, int x0, int x1) {
    for (int x = x0; x <= x1 && x < Screen::W; x++) if (visible(s, x, y)) return x;
    return -1;
}
// estende x0 verso sinistra sulla barra in inverso contigua (es. "{rvon} * 1 *")
int growLeft(const Screen& s, int y, int x0) { while (x0 > 0 && reversed(s, x0 - 1, y)) x0--; return x0; }

// cerca "* c *" con spazi facoltativi; ritorna la colonna del primo '*' e il carattere
bool starItem(const std::string& t, int& col, char& key) {
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] != '*') continue;
        size_t j = i + 1;
        while (j < t.size() && t[j] == ' ') j++;
        if (j >= t.size() || !std::isalnum((unsigned char)t[j])) continue;
        size_t k = j + 1;
        while (k < t.size() && t[k] == ' ') k++;
        if (k < t.size() && t[k] == '*') { col = (int)i; key = t[j]; return true; }
    }
    return false;
}

uint8_t petsciiKey(char c) {
    if (c >= 'a' && c <= 'z') return (uint8_t)(c - 'a' + 0x41);
    return (uint8_t)c;
}

} // namespace

std::vector<Choice> detectChoices(const Screen& s) {
    std::vector<Choice> out;
    std::string rows[Screen::H];
    for (int y = 0; y < Screen::H; y++) rows[y] = rowText(s, y);
    bool numberPrompt = false;
    for (int y = 0; y < Screen::H; y++) {
        const std::string& t = rows[y];
        if (t.find("batti il numero") != std::string::npos) numberPrompt = true;
        int col; char key;
        if (starItem(t, col, key)) {
            // include la barra in inverso che precede l'asterisco (es. "{rvon} * 1 *")
            int x0 = growLeft(s, y, col), x1 = segEnd(s, y, col);
            Choice c{x0, y, x1, y, petsciiKey(key)};
            // la descrizione puo' continuare sulle righe seguenti, nelle stesse colonne,
            // finche' non trova una riga vuota in quella zona
            for (int yy = y + 1; yy < Screen::H; yy++) {
                const std::string& n = rows[yy];
                int dc; char dk;
                int a0 = firstIn(s, yy, c.x0, c.x1);
                if (a0 < 0 || starItem(n, dc, dk) || n.find("batti") != std::string::npos ||
                    n.find("premi un tasto") != std::string::npos || n.find("- - -") != std::string::npos) break;
                c.y1 = yy;
                int a1 = segEnd(s, yy, a0);
                if (a1 > c.x1) c.x1 = a1;
            }
            out.push_back(c);
            continue;
        }
        // (S/N)
        size_t sl = t.find('/');
        while (sl != std::string::npos) {
            size_t a = sl, b = sl;
            while (a > 0 && t[a - 1] == ' ') a--;
            while (b + 1 < t.size() && t[b + 1] == ' ') b++;
            if (a >= 1 && b + 1 < t.size() && t[a - 1] == 's' && t[b + 1] == 'n') {
                int sx = (int)a - 1, nx = (int)b + 1;
                int l = sx, r = nx;
                if (l > 0 && t[l - 1] == ' ') l--;
                if (l > 0 && t[l - 1] == '(') l--;
                if (r + 1 < (int)t.size() && t[r + 1] == ' ') r++;
                if (r + 1 < (int)t.size() && t[r + 1] == ')') r++;
                out.push_back({l, y, (int)sl - 1 < sx ? sx : (int)sl - 1, y, 0x53});
                out.push_back({(int)sl + 1 > nx ? nx : (int)sl + 1, y, r, y, 0x4E});
                break;
            }
            sl = t.find('/', sl + 1);
        }
        // "per bere ... F1"
        size_t f1 = t.find("f1");
        size_t pk = t.find("premi un tasto");
        if (f1 != std::string::npos && (pk != std::string::npos || t.find("bere") != std::string::npos)) {
            size_t st = t.find("per bere");
            if (st == std::string::npos) st = f1;
            out.push_back({(int)st, y, (int)f1 + 1, y, 0x85});
        }
        if (pk != std::string::npos) {
            size_t en = t.find(';', pk);
            int st = segStart(s, y, (int)pk);
            int e = en == std::string::npos ? segEnd(s, y, (int)pk) : (int)en - 1;   // tutta la barra colorata
            out.push_back({st, y, e, y, 0x20});
        }
    }
    bool hasNumbers = false;
    for (auto& c : out) if (c.key >= '0' && c.key <= '9') hasNumbers = true;
    if (numberPrompt && !hasNumbers)
        for (int k = 1; k <= 9; k++) { Choice c{k - 1, -1, 0, -1, (uint8_t)('0' + k)}; c.pad = true; out.push_back(c); }
    return out;
}
