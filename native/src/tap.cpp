// Porting di web/js/core/tap.js: Kernal ROM + Turbo Tape 64, de-offuscamento del boot.
#include "tap.h"
#include <cstring>

namespace {

std::vector<int> readPulses(const std::vector<uint8_t>& u, std::string& err) {
    std::vector<int> p;
    if (u.size() < 20 || std::memcmp(u.data(), "C64-TAPE-RAW", 12) != 0) { err = "Il file non e' un .tap del Commodore 64"; return p; }
    int ver = u[12];
    p.reserve(u.size());
    for (size_t i = 20; i < u.size();) {
        uint8_t b = u[i];
        if (b == 0) {
            if (ver == 0) { p.push_back(0x100); i += 1; }
            else { if (i + 3 >= u.size()) break; p.push_back((u[i + 1] | u[i + 2] << 8 | u[i + 3] << 16) / 8); i += 4; }
        } else { p.push_back(b); i += 1; }
    }
    return p;
}

struct KFile { std::vector<uint8_t> hdr, data; int start = 0, end = 0; };

std::vector<KFile> kernalFiles(const std::vector<int>& p, std::string& err) {
    std::vector<uint8_t> seq(p.size());
    for (size_t i = 0; i < p.size(); i++) seq[i] = p[i] < 0x37 ? 1 : p[i] < 0x4C ? 2 : p[i] < 0x70 ? 3 : 0;
    std::vector<std::vector<uint8_t>> blocks;
    std::vector<uint8_t> cur;
    size_t j = 0;
    while (j + 20 < seq.size()) {
        if (seq[j] == 3 && seq[j + 1] == 2) {
            int val = 0, par = 1; bool ok = true;
            for (int k = 0; k < 9; k++) {
                int a = seq[j + 2 + 2 * k], b = seq[j + 3 + 2 * k], bit;
                if (a == 1 && b == 2) bit = 0; else if (a == 2 && b == 1) bit = 1; else { ok = false; break; }
                if (k < 8) { val |= bit << k; par ^= bit; } else if (bit != par) ok = false;
            }
            if (ok) { cur.push_back((uint8_t)val); j += 20; continue; }
        }
        if (cur.size() > 8) blocks.push_back(cur);
        cur.clear();
        j++;
    }
    if (cur.size() > 8) blocks.push_back(cur);
    std::vector<KFile> out;
    std::vector<uint8_t> hdr;
    for (auto& raw : blocks) {
        if (raw[0] < 0x80) continue;
        std::vector<uint8_t> payload(raw.begin() + 9, raw.end() - 1);
        uint8_t x = 0; for (auto b : payload) x ^= b;
        if (x != raw.back()) { err = "Checksum errato in un blocco Kernal del nastro"; return {}; }
        if ((payload[0] == 1 || payload[0] == 3) && payload.size() == 192) { hdr = payload; continue; }
        if (!hdr.empty()) {
            KFile f; f.hdr = hdr; f.start = hdr[1] | hdr[2] << 8; f.end = hdr[3] | hdr[4] << 8;
            if (f.end < f.start || (size_t)(f.end - f.start) > payload.size()) { hdr.clear(); continue; }
            f.data.assign(payload.begin(), payload.begin() + (f.end - f.start));
            out.push_back(f);
            hdr.clear();
        }
    }
    return out;
}

struct TFile { int start = 0, end = 0; std::vector<uint8_t> data; };

std::vector<TFile> turboFiles(const std::vector<int>& p, std::string& err) {
    const size_t L = p.size();
    auto isT = [&](int v) { return v >= 0x10 && v <= 0x2E; };
    auto rb = [&](size_t n) -> int {
        int v = 0;
        for (int k = 0; k < 8; k++) { if (n + k >= L || !isT(p[n + k])) return -1; v = (v << 1) | (p[n + k] > 0x21 ? 1 : 0); }
        return v;
    };
    auto findSync = [&](size_t n) -> long {
        int sr = 0;
        while (n < L) {
            int v = p[n++];
            if (!isT(v)) { sr = 0; continue; }
            sr = ((sr << 1) | (v > 0x21 ? 1 : 0)) & 0xFF;
            if (sr == 9) {
                size_t m = n; bool ok = true;
                for (int want = 8; want >= 1; want--) { if (rb(m) != want) { ok = false; break; } m += 8; }
                if (ok) return (long)m;
            }
        }
        return -1;
    };
    std::vector<TFile> files;
    long n = 0;
    for (;;) {
        n = findSync(n);
        if (n < 0) break;
        int typ = rb(n);
        if (typ != 1 && typ != 2) { n += 8; continue; }
        size_t m = n + 8;
        int h[4];
        for (int i = 0; i < 4; i++) { h[i] = rb(m); m += 8; }
        TFile f; f.start = h[0] | h[1] << 8; f.end = h[2] | h[3] << 8;
        if (f.end <= f.start) break;
        n = findSync(m);
        if (n < 0 || rb(n) != 0) break;
        n += 8;
        f.data.resize(f.end - f.start);
        uint8_t x = 0;
        for (auto& b : f.data) { int v = rb(n); if (v < 0) { err = "Nastro troncato nel blocco turbo"; return {}; } b = (uint8_t)v; x ^= b; n += 8; }
        int chk = rb(n); n += 8;
        if (chk != x) { err = "Checksum errato nel blocco turbo del nastro"; return {}; }
        files.push_back(std::move(f));
    }
    return files;
}

} // namespace

bool loadGameFromTap(const std::vector<uint8_t>& tap, GameImage& g, std::string& err) {
    auto p = readPulses(tap, err);
    if (!err.empty()) return false;
    auto kf = kernalFiles(p, err);
    if (!err.empty()) return false;
    auto tf = turboFiles(p, err);
    if (!err.empty()) return false;
    const TFile* main = nullptr;
    for (auto& f : tf) if (f.start == 0x0801) main = &f;
    if (!main || kf.empty()) { err = "Programma non trovato: non sembra il nastro di Avventura 1"; return false; }
    g.mem.assign(0x10000, 0);
    std::memcpy(&g.mem[0x0801], main->data.data(), main->data.size());
    for (size_t a = 0x0803; a < 0x0801 + main->data.size(); a++) g.mem[a] = (uint8_t)(g.mem[a] + 1);
    for (auto& f : kf) if (f.start == 0x0801) std::memcpy(&g.mem[0x0801], f.data.data(), f.data.size());
    auto& last = kf.back().hdr;
    g.entry = (uint16_t)(last[21] | last[22] << 8);
    g.end = (uint16_t)(0x0801 + main->data.size());
    return validateImage(g, err);
}

bool validateImage(const GameImage& g, std::string& err) {
    auto w = [&](int a) { return g.mem[a] | g.mem[a + 1] << 8; };
    int vartab = w(0x1784), arytab = w(0x1786), init = w(0x178A), dat = w(0x178C), start = w(0x178E);
    bool ok = g.entry == 0x0B41 && init == 0x1790 && dat > init && start > dat && vartab > start &&
              arytab >= vartab && (arytab - vartab) % 7 == 0 && arytab <= g.end + 1 && g.mem[vartab - 1] == 0;
    if (!ok) err = "La copia non corrisponde alla versione attesa di Avventura 1";
    return ok;
}
