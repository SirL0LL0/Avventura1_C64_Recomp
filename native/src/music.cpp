#include "music.h"
#include "songs.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <sstream>

MusicPlayer g_music;

namespace {

const int RATE = 44100;
const int SAMPLES_PER_TICK = RATE / 50;     // 50 Hz come il C64 PAL

int noteToMidi(const std::string& t, size_t& i) {
    static const int base[7] = {9, 11, 0, 2, 4, 5, 7};   // A B C D E F G
    char c = (char)std::toupper((unsigned char)t[i]);
    if (c < 'A' || c > 'G') return -1;
    int n = base[c - 'A'];
    i++;
    if (i < t.size() && t[i] == '#') { n++; i++; }
    else if (i < t.size() && t[i] == 'b') { n--; i++; }
    int oct = 4;
    if (i < t.size() && std::isdigit((unsigned char)t[i])) { oct = t[i] - '0'; i++; }
    return 12 * (oct + 1) + n;
}

// accordo: fondamentale (classe di altezza) + intervalli
bool parseChord(const std::string& t, int& root, std::vector<int>& iv) {
    static const int base[7] = {9, 11, 0, 2, 4, 5, 7};
    if (t.empty()) return false;
    char c = (char)std::toupper((unsigned char)t[0]);
    if (c < 'A' || c > 'G') return false;
    size_t i = 1;
    int n = base[c - 'A'];
    if (i < t.size() && t[i] == '#') { n++; i++; }
    else if (i < t.size() && t[i] == 'b') { n--; i++; }
    root = (n + 12) % 12;
    std::string suf = t.substr(i);
    if (suf == "m") iv = {0, 3, 7};
    else if (suf == "7") iv = {0, 4, 7, 10};
    else if (suf == "m7") iv = {0, 3, 7, 10};
    else if (suf == "dim") iv = {0, 3, 6};
    else if (suf == "sus") iv = {0, 5, 7};
    else iv = {0, 4, 7};
    return true;
}

// espande i gruppi ripetuti "[ ... ]N"
std::string expand(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] != '[') { out += s[i]; continue; }
        int depth = 1; size_t j = i + 1;
        while (j < s.size() && depth) { if (s[j] == '[') depth++; else if (s[j] == ']') depth--; j++; }
        std::string inner = expand(s.substr(i + 1, j - i - 2));
        int n = 0;
        while (j < s.size() && std::isdigit((unsigned char)s[j])) n = n * 10 + (s[j++] - '0');
        if (n == 0) n = 1;
        for (int k = 0; k < n; k++) out += " " + inner + " ";
        i = j - 1;
    }
    return out;
}

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> o;
    std::istringstream is(s);
    std::string w;
    while (is >> w) o.push_back(w);
    return o;
}

int hexv(const std::string& s) { return (int)std::strtol(s.c_str(), nullptr, 16); }

uint8_t waveOf(const std::string& w) {
    if (w == "tri") return 1;
    if (w == "saw") return 2;
    if (w == "pulse") return 4;
    if (w == "noise") return 8;
    if (w == "pulsesaw") return 6;
    return 4;
}

std::vector<Song> parseSongs(const char* text) {
    std::vector<Song> out;
    std::istringstream is(text);
    std::string line;
    Song* s = nullptr;
    std::vector<std::string> chords;
    while (std::getline(is, line)) {
        size_t sc = line.find(';');
        if (sc != std::string::npos) line = line.substr(0, sc);
        auto w = split(line);
        if (w.empty()) continue;
        if (w[0] == "song") { out.emplace_back(); s = &out.back(); s->name = w[1]; chords.clear(); continue; }
        if (!s) continue;
        if (w[0] == "tempo") s->tempo = std::atoi(w[1].c_str());
        else if (w[0] == "bar") s->bar = std::atoi(w[1].c_str());
        else if (w[0] == "loop") s->loop = w[1] != "0";
        else if (w[0] == "chords") chords.assign(w.begin() + 1, w.end());
        else if (w[0] == "filter") {
            int sidn = std::atoi(w[1].c_str()) - 1;
            for (size_t k = 2; k < w.size(); k++) {
                auto eq = w[k].find('=');
                std::string key = w[k].substr(0, eq), v = w[k].substr(eq + 1);
                if (key == "cutoff") s->cutoff[sidn] = (uint16_t)hexv(v);
                if (key == "res") s->res[sidn] = (uint8_t)hexv(v);
                if (key == "mode") { s->lp[sidn] = v.find("lp") != std::string::npos; s->bp[sidn] = v.find("bp") != std::string::npos; s->hp[sidn] = v.find("hp") != std::string::npos; }
            }
        } else if (w[0] == "inst") {
            Instrument in;
            for (size_t k = 2; k < w.size(); k++) {
                auto eq = w[k].find('=');
                std::string key = w[k].substr(0, eq), v = eq == std::string::npos ? "" : w[k].substr(eq + 1);
                if (key == "wave") in.wave = waveOf(v);
                else if (key == "ad") { int x = hexv(v); in.attack = x >> 4; in.decay = x & 15; }
                else if (key == "sr") { int x = hexv(v); in.sustain = x >> 4; in.release = x & 15; }
                else if (key == "pw") in.pw = (uint16_t)hexv(v);
                else if (key == "sweep") in.sweep = std::atoi(v.c_str());
                else if (key == "vib") { auto c = v.find(','); in.vibDepth = (float)std::atof(v.substr(0, c).c_str()); in.vibSpeed = (float)std::atof(v.substr(c + 1).c_str()) * 0.1f; }
                else if (key == "delay") in.vibDelay = std::atoi(v.c_str());
                else if (key == "filt") in.filtered = v == "1";
            }
            s->inst[w[1]] = in;
        } else if (w[0].size() == 2 && w[0][0] == 'v' && std::isdigit((unsigned char)w[0][1])) {
            int vi = w[0][1] - '1';
            if (vi < 0 || vi > 5) continue;
            VoiceTrack& vt = s->v[vi];
            std::string type = w[1];
            vt.inst = w.size() > 2 ? w[2] : "";
            size_t colon = line.find(':');
            std::string body = colon == std::string::npos ? "" : expand(line.substr(colon + 1));
            auto tok = split(body);
            int row = 0;
            if (type == "mel" || type == "drum") {
                for (auto& t : tok) {
                    if (t == "|") continue;
                    size_t dot = t.find('.');
                    std::string n = t.substr(0, dot);
                    int len = dot == std::string::npos ? 1 : std::atoi(t.c_str() + dot + 1);
                    NoteEvent e{row, len, -1, 0};
                    if (type == "drum") {
                        if (n == "K") e.kind = 2; else if (n == "S") e.kind = 3; else if (n == "H") e.kind = 4; else e.kind = -1;
                        e.midi = 0;
                    } else if (n != "r") { size_t i = 0; e.midi = noteToMidi(n, i); }
                    if (e.midi >= 0 || (type == "drum" && e.kind > 0)) vt.ev.push_back(e);
                    row += len;
                }
            } else if (type == "echo") {
                // echo: copia di un'altra voce gia' definita, ritardata
                int src = std::atoi(tok.size() > 0 ? tok[0].c_str() + 1 : "1") - 1;
                int delay = tok.size() > 1 ? std::atoi(tok[1].c_str()) : 2;
                int oct = tok.size() > 2 ? std::atoi(tok[2].c_str()) : 0;
                if (src >= 0 && src < 6) for (auto e : s->v[src].ev) { e.row += delay; e.midi += oct * 12; vt.ev.push_back(e); }
            } else {
                // voci ad accordi: un accordo per battuta
                for (auto& c : chords) {
                    int root; std::vector<int> iv;
                    if (!parseChord(c, root, iv)) { row += s->bar; continue; }
                    if (type == "arp") {
                        NoteEvent e{row, s->bar, 60 + root, 1};
                        vt.ev.push_back(e); vt.arps.push_back(iv);
                    } else if (type == "pad") {
                        // pad : ottava [nota dell'accordo: 0 fondamentale, 1 terza, 2 quinta]
                        int oct = tok.empty() ? 4 : std::atoi(tok[0].c_str());
                        size_t k = tok.size() > 1 ? (size_t)std::atoi(tok[1].c_str()) : 1;
                        if (k >= iv.size()) k = iv.size() - 1;
                        vt.ev.push_back({row, s->bar, 12 * (oct + 1) + root + iv[k], 0});
                    } else if (type == "harp") {
                        // harp : ottava passo  -> accordo spezzato lento (1 3 5 8 5 3 ...)
                        int oct = tok.empty() ? 4 : std::atoi(tok[0].c_str());
                        int step = tok.size() > 1 ? std::max(1, std::atoi(tok[1].c_str())) : 1;
                        int seq[6] = {iv[0], iv[1], iv[2], 12, iv[2], iv[1]};
                        for (int r = 0, k = 0; r < s->bar; r += step, k++)
                            vt.ev.push_back({row + r, std::min(step, s->bar - r), 12 * (oct + 1) + root + seq[k % 6], 0});
                    } else if (type == "bass") {
                        std::string pat = tok.empty() ? "R......." : tok[0];
                        int r = row;
                        for (size_t k = 0; k < pat.size(); k++, r++) {
                            char p = pat[k];
                            int m = -1;
                            if (p == 'R') m = 36 + root; else if (p == 'O') m = 48 + root; else if (p == 'F') m = 36 + root + 7;
                            else if (p == 'T') m = 36 + root + iv[1];
                            if (m < 0) continue;
                            int len = 1;
                            while (k + len < pat.size() && pat[k + len] == '.') len++;
                            vt.ev.push_back({r, len, m, 0});
                        }
                    }
                    row += s->bar;
                }
            }
            if (row > s->rows) s->rows = row;
            for (auto& e : vt.ev) if (e.row + e.len > s->rows) s->rows = e.row + e.len;
        }
    }
    return out;
}

// effetti sonori: sequenze (forma d'onda, nota, tick) sulla voce 3 del secondo SID
struct SfxStep { uint8_t wave; int midi; int ticks; };
struct SfxDef { const char* name; uint8_t ad, sr; std::vector<SfxStep> steps; };
const std::vector<SfxDef>& sfxTable() {
    static const std::vector<SfxDef> t = {
        {"colpo", 0x00, 0x09, {{8, 80, 2}, {1, 40, 2}, {1, 34, 3}}},
        {"ferita", 0x00, 0x0A, {{8, 60, 3}, {2, 38, 4}, {2, 33, 4}}},
        {"mancato", 0x00, 0x05, {{8, 96, 2}, {8, 90, 2}}},
        {"moneta", 0x00, 0x08, {{4, 88, 3}, {4, 95, 6}}},
        {"pozione", 0x02, 0x48, {{1, 72, 3}, {1, 76, 3}, {1, 79, 3}, {1, 84, 6}}},
        {"livello", 0x00, 0x69, {{6, 72, 4}, {6, 76, 4}, {6, 79, 4}, {6, 84, 10}}},
        {"clic", 0x00, 0x02, {{4, 84, 1}, {4, 91, 2}}},
        {"magia", 0x08, 0x8A, {{1, 84, 2}, {1, 88, 2}, {1, 91, 2}, {1, 96, 2}, {1, 91, 2}, {1, 96, 6}}},
        {"porta", 0x00, 0x0B, {{2, 30, 4}, {8, 40, 6}}},
    };
    return t;
}

// ------------------------------------------------------------------ brani generati
// Ogni brano ha tonalita', modo e due progressioni di 8 battute; la melodia e' composta a ogni
// giro da frasi di 2 battute (motivo, variazione, nuova frase, cadenza): riconoscibile ma mai identica.
struct GenDef {
    const char* name; int root; int mode; int tempo; int progA[8], progB[8];
    uint8_t leadWave; uint16_t padCut;
};
const int MODES[5][7] = {
    {0, 2, 4, 5, 7, 9, 11},   // ionico
    {0, 2, 3, 5, 7, 9, 10},   // dorico
    {0, 2, 3, 5, 7, 8, 10},   // eolio
    {0, 2, 4, 6, 7, 9, 11},   // lidio
    {0, 2, 4, 5, 7, 9, 10},   // misolidio
};
const GenDef GEN[] = {
    {"valle", 62, 1, 20, {0, 0, 3, 3, 0, 0, 6, 6}, {5, 5, 3, 3, 0, 6, 4, 4}, 1, 0x150},
    {"focolare", 67, 0, 19, {0, 5, 3, 4, 0, 5, 3, 4}, {3, 4, 5, 5, 3, 4, 0, 0}, 4, 0x170},
    {"sentiero", 64, 2, 21, {0, 5, 2, 6, 0, 5, 3, 4}, {3, 0, 6, 2, 5, 3, 6, 6}, 1, 0x140},
    {"stelle", 65, 3, 22, {0, 1, 0, 1, 0, 1, 5, 4}, {3, 2, 1, 1, 0, 1, 5, 0}, 1, 0x130},
    {"mare", 69, 4, 20, {0, 0, 6, 6, 3, 3, 0, 0}, {4, 3, 6, 0, 4, 3, 6, 6}, 4, 0x160},
};

struct Rng { uint32_t s; int operator()(int n) { s = s * 1664525u + 1013904223u; return (int)((s >> 8) % (uint32_t)n); } };

struct MNote { int row, len, deg; bool rest; };

void generateSong(Song& s, const GenDef& g, uint32_t seed) {
    Rng rnd{seed * 2654435761u + 7};
    const int* sc = MODES[g.mode];
    auto midiOf = [&](int base, int deg) { int o = deg >= 0 ? deg / 7 : -((-deg + 6) / 7); return base + 12 * o + sc[((deg % 7) + 7) % 7]; };
    s = Song();
    s.name = g.name; s.tempo = g.tempo; s.bar = 8; s.loop = false;
    s.cutoff[0] = 0x300; s.res[0] = 4;
    s.cutoff[1] = g.padCut; s.res[1] = 6;
    Instrument lead; lead.wave = g.leadWave; lead.attack = 6; lead.decay = 10; lead.sustain = 9; lead.release = 11;
    lead.pw = 0x600; lead.sweep = g.leadWave == 4 ? 3 : 0; lead.vibDepth = 12; lead.vibSpeed = 0.2f; lead.vibDelay = 18;
    lead.filtered = g.leadWave == 4;
    Instrument eco = lead; eco.wave = 1; eco.sustain = 6; eco.filtered = false; eco.vibDepth = 0;
    Instrument basso; basso.wave = 1; basso.attack = 8; basso.decay = 10; basso.sustain = 10; basso.release = 11;
    Instrument arpa; arpa.wave = 4; arpa.pw = 0x700; arpa.attack = 0; arpa.decay = 10; arpa.sustain = 0; arpa.release = 11; arpa.filtered = true;
    Instrument pad; pad.wave = 2; pad.attack = 12; pad.decay = 10; pad.sustain = 10; pad.release = 12; pad.filtered = true;
    s.inst["lead"] = lead; s.inst["eco"] = eco; s.inst["basso"] = basso; s.inst["arpa"] = arpa; s.inst["pad"] = pad;
    s.v[0].inst = "lead"; s.v[1].inst = "eco"; s.v[2].inst = "basso"; s.v[3].inst = "arpa"; s.v[4].inst = "pad"; s.v[5].inst = "pad";
    // accordi delle 32 battute: A A B A
    int chord[32];
    for (int p = 0; p < 4; p++) for (int b = 0; b < 8; b++) chord[p * 8 + b] = p == 2 ? g.progB[b] : g.progA[b];
    chord[31] = 0;
    auto chordTone = [&](int bar, int deg) {        // grado dell'accordo piu' vicino a deg
        int best = deg, bd = 99;
        for (int o = -1; o <= 2; o++) for (int k = 0; k < 3; k++) {
            int d = chord[bar] + 2 * k + 7 * o;
            if (std::abs(d - deg) < bd) { bd = std::abs(d - deg); best = d; }
        }
        return best;
    };
    static const int RH[][4] = {{4, 4, 0, 0}, {2, 2, 4, 0}, {3, 1, 4, 0}, {6, 2, 0, 0}, {2, 2, 2, 2}, {4, 2, 2, 0}, {8, 0, 0, 0}, {2, 6, 0, 0}, {3, 3, 2, 0}};
    auto phrase = [&](int bar0) {                    // frase di 2 battute
        std::vector<MNote> ph;
        int deg = chordTone(bar0, 2 + rnd(5));
        for (int b = 0; b < 2; b++) {
            const int* r = RH[rnd(9)];
            int row = b * 8;
            for (int k = 0; k < 4 && r[k]; k++) {
                if (row % 4 == 0) deg = chordTone(bar0 + b, deg);
                ph.push_back({row, r[k], deg, k > 0 && rnd(7) == 0});
                static const int STEP[] = {-2, -1, -1, 0, 1, 1, 2};
                deg += STEP[rnd(7)];
                deg = std::max(0, std::min(9, deg));
                row += r[k];
            }
        }
        return ph;
    };
    auto vary = [&](std::vector<MNote> ph, int bar0, bool cadence) {
        int n = (int)ph.size();
        for (int i = std::max(1, n - 2); i < n; i++) ph[i].deg = std::max(0, std::min(9, ph[i].deg + (rnd(3) - 1)));
        if (cadence) {                               // ultima nota lunga sulla fondamentale
            MNote& last = ph.back();
            last.rest = false;
            int best = chord[bar0 + 1];
            for (int o = 0; o <= 1; o++) if (std::abs(chord[bar0 + 1] + 7 * o - last.deg) < std::abs(best - last.deg)) best = chord[bar0 + 1] + 7 * o;
            last.deg = best;
            last.len = 16 - last.row;
        }
        return ph;
    };
    std::vector<MNote> motif = phrase(0);
    for (int p = 0; p < 4; p++) {
        int base = p * 8;
        std::vector<MNote> A = p == 2 ? phrase(base) : vary(motif, base, false);
        std::vector<MNote> B = phrase(base + 4);
        std::vector<MNote> phs[4] = {A, vary(A, base + 2, false), B, vary(p == 2 ? B : A, base + 6, true)};
        for (int k = 0; k < 4; k++) {
            if (p == 0 && k == 0) continue;          // introduzione: solo arpa e accordi
            for (auto& n : phs[k]) {
                if (n.rest) continue;
                int row = (base + k * 2) * 8 + n.row;
                s.v[0].ev.push_back({row, n.len, midiOf(g.root, n.deg), 0});
                if (p > 0) s.v[1].ev.push_back({row + 3, n.len, midiOf(g.root - 12, n.deg - (p == 2 ? 2 : 0)), 0});
            }
        }
    }
    // basso, arpa e accordi
    for (int b = 0; b < 32; b++) {
        int row = b * 8, d = chord[b];
        int rootM = midiOf(g.root - 24, d);
        if (rootM < 36) rootM += 12;
        if (b % 8 >= 4 && rnd(2)) { s.v[2].ev.push_back({row, 4, rootM, 0}); s.v[2].ev.push_back({row + 4, 4, midiOf(g.root - 24, d + 4) + (rootM - midiOf(g.root - 24, d)), 0}); }
        else s.v[2].ev.push_back({row, 8, rootM, 0});
        int step = (b / 8 == 1) ? 4 : 2;
        static const int ARP[] = {0, 2, 4, 7, 4, 2};
        for (int r = 0, k = 0; r < 8; r += step, k++) s.v[3].ev.push_back({row + r, step, midiOf(g.root - 12, d + ARP[(k + b) % 6]), 0});
        s.v[4].ev.push_back({row, 8, midiOf(g.root - 12, d + 2), 0});
        s.v[5].ev.push_back({row, 8, midiOf(g.root - 12, d + 4), 0});
    }
    s.rows = 32 * 8 + 8;                             // una battuta di coda per le code dei suoni
}

} // namespace

std::vector<std::string> MusicPlayer::songNames() const {
    std::vector<std::string> n;
    for (auto& s : songs_) n.push_back(s.name);
    return n;
}

static void addGenerated(std::vector<Song>& songs, std::vector<int>& pl) {
    pl.clear();
    for (size_t i = 0; i < sizeof(GEN) / sizeof(GEN[0]); i++) { songs.emplace_back(); generateSong(songs.back(), GEN[i], 1); pl.push_back((int)songs.size() - 1); }
}

bool MusicPlayer::open() {
    songs_ = parseSongs(SONGS_TEXT);
    addGenerated(songs_, playlist_);
    { SDL_Time t = 0; SDL_GetCurrentTime(&t); seed_ = (uint32_t)(t / 1000000); plPos_ = (int)(seed_ % playlist_.size()); }
    sid_[0] = new Sid(RATE);
    sid_[1] = new Sid(RATE);
    SDL_AudioSpec spec{SDL_AUDIO_F32, 2, RATE};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, callback, this);
    if (!stream_) return false;
    SDL_ResumeAudioStreamDevice(stream_);
    return true;
}

float MusicPlayer::exportWav(const std::string& song, double seconds, const std::string& path) {
    if (songs_.empty()) { songs_ = parseSongs(SONGS_TEXT); addGenerated(songs_, playlist_); }
    if (!sid_[0]) { sid_[0] = new Sid(RATE); sid_[1] = new Sid(RATE); }
    int idx = -1;
    for (size_t i = 0; i < songs_.size(); i++) if (songs_[i].name == song) idx = (int)i;
    if (idx < 0) return -1;
    startSong(idx);
    request_ = -2; pendingSong_ = -2; fadeOut_ = 1; gain_ = 1;
    int frames = (int)(seconds * RATE);
    std::vector<float> buf(frames * 2);
    render(buf.data(), frames);
    float peak = 0;
    std::vector<int16_t> pcm(buf.size());
    for (size_t i = 0; i < buf.size(); i++) {
        float x = buf[i];
        if (!(x == x)) x = 0;                 // NaN
        peak = std::fmax(peak, std::fabs(x));
        pcm[i] = (int16_t)(std::fmax(-1.0f, std::fmin(1.0f, x)) * 32000);
    }
    SDL_IOStream* f = SDL_IOFromFile(path.c_str(), "wb");
    if (!f) return -1;
    auto w32 = [&](uint32_t v) { SDL_WriteU32LE(f, v); };
    auto w16 = [&](uint16_t v) { SDL_WriteU16LE(f, v); };
    uint32_t data = (uint32_t)(pcm.size() * 2);
    SDL_WriteIO(f, "RIFF", 4); w32(36 + data); SDL_WriteIO(f, "WAVEfmt ", 8);
    w32(16); w16(1); w16(2); w32(RATE); w32(RATE * 4); w16(4); w16(16);
    SDL_WriteIO(f, "data", 4); w32(data);
    SDL_WriteIO(f, pcm.data(), data);
    SDL_CloseIO(f);
    cur_ = -1;
    return peak;
}

void MusicPlayer::close() {
    if (stream_) SDL_DestroyAudioStream(stream_);
    stream_ = nullptr;
    delete sid_[0]; delete sid_[1];
    sid_[0] = sid_[1] = nullptr;
}

void MusicPlayer::play(const std::string& name) {
    if (name == currentName_) return;
    currentName_ = name;
    fastFade_ = name == "morte";                 // il tema della morte parte subito
    if (name == "colonna") { request_ = -3; return; }
    int idx = -1;
    for (size_t i = 0; i < songs_.size(); i++) if (songs_[i].name == name) idx = (int)i;
    request_ = idx;
}

void MusicPlayer::sfx(const std::string& name) {
    auto& t = sfxTable();
    for (size_t i = 0; i < t.size(); i++) if (name == t[i].name) sfxRequest_ = (int)i;
}

void SDLCALL MusicPlayer::callback(void* ud, SDL_AudioStream* s, int additional, int) {
    auto* self = (MusicPlayer*)ud;
    int frames = additional / (int)(sizeof(float) * 2);
    if (frames <= 0) return;
    std::vector<float> buf(frames * 2);
    self->render(buf.data(), frames);
    SDL_PutAudioStreamData(s, buf.data(), frames * 2 * (int)sizeof(float));
}

void MusicPlayer::startPlaylistSong() {
    if (playlist_.empty()) return;
    int k = plPos_++ % (int)playlist_.size();
    generateSong(songs_[playlist_[k]], GEN[k], seed_++);
    startSong(playlist_[k]);
}

void MusicPlayer::startSong(int idx) {
    cur_ = idx;
    tickCount_ = 0;
    for (auto& v : vs_) v = VState();
    for (int k = 0; k < 2; k++) for (auto& vo : sid_[k]->v) { vo.gate = false; }
    if (idx >= 0) {
        Song& s = songs_[idx];
        for (int k = 0; k < 2; k++) {
            sid_[k]->cutoff = s.cutoff[k]; sid_[k]->resonance = s.res[k];
            sid_[k]->lp = s.lp[k]; sid_[k]->bp = s.bp[k]; sid_[k]->hp = s.hp[k];
        }
    }
}

void MusicPlayer::applyNote(int vi, const NoteEvent& e, const Instrument& in, int t) {
    SidVoice& vo = sid_[vi / 3]->v[vi % 3];
    Song& s = songs_[cur_];
    int total = e.len * s.tempo;
    vo.attack = in.attack; vo.decay = in.decay; vo.sustain = in.sustain; vo.release = in.release;
    vo.filtered = in.filtered;
    if (e.kind >= 2) {                                   // batteria
        if (e.kind == 2) { vo.wave = t < 1 ? 8 : 1; vo.attack = 0; vo.decay = 7; vo.sustain = 0; vo.release = 6; vo.freq = sidFreqForNote(52 - t * 5 < 28 ? 28 : 52 - t * 5); }
        else if (e.kind == 3) { vo.wave = 8; vo.attack = 0; vo.decay = 8; vo.sustain = 0; vo.release = 7; vo.freq = sidFreqForNote(t < 1 ? 70 : 88); }
        else { vo.wave = 8; vo.attack = 0; vo.decay = 3; vo.sustain = 0; vo.release = 2; vo.freq = sidFreqForNote(108); }
        vo.gate = t < 2;
        return;
    }
    int midi = e.midi;
    if (e.kind == 1) {
        auto& ev = s.v[vi].ev;
        size_t k = &e - &ev[0];
        auto& iv = s.v[vi].arps[k];
        midi = e.midi + iv[(t / 2) % iv.size()];
    }
    double m = midi;
    VState& st = vs_[vi];
    if (t == 0) { st.pw = in.pw; st.vibPhase = 0; }
    if (in.vibDepth > 0 && t > in.vibDelay) {
        st.vibPhase += in.vibSpeed;
        m += in.vibDepth / 100.0 * std::sin(st.vibPhase);
    }
    if (in.sweep) {
        int pw = st.pw + in.sweep;
        if (pw > 0xE00 || pw < 0x200) pw = st.pw;  // fermo ai limiti
        st.pw = (uint16_t)pw;
    }
    vo.wave = in.wave;
    vo.pw = st.pw;
    vo.freq = sidFreqForNote(m);
    vo.gate = t < total - 1 || e.kind == 1;           // un tick di gate spento per riattaccare la nota
}

void MusicPlayer::tick() {
    int rq = request_.exchange(-2);
    if (rq != -2) { pendingSong_ = rq; if (cur_ < 0) fadeOut_ = 0; }
    if (pendingSong_ != -2) {
        // dissolvenza in uscita (circa 1,5 s), poi il nuovo brano entra piano
        fadeOut_ -= fastFade_ ? 1.0f / 4 : 1.0f / 75;
        if (fadeOut_ <= 0) {
            fadeOut_ = 0;
            plMode_ = pendingSong_ == -3;
            if (plMode_) startPlaylistSong(); else startSong(pendingSong_);
            pendingSong_ = -2;
        }
    } else if (fadeOut_ < 1) {
        fadeOut_ += fastFade_ ? 1.0f / 6 : 1.0f / 125;   // dissolvenza in entrata (circa 2,5 s)
        if (fadeOut_ > 1) fadeOut_ = 1;
    }
    int sr = sfxRequest_.exchange(-1);
    if (sr >= 0) { sfxIdx_ = sr; sfxTick_ = 0; }
    bool sfxActive = sfxIdx_ >= 0;
    if (cur_ >= 0) {
        Song& s = songs_[cur_];
        long row = tickCount_ / s.tempo;
        if (s.rows > 0 && row >= s.rows) {
            if (s.loop) { tickCount_ = 0; row = 0; }
            else {
                for (int k = 0; k < 2; k++) for (auto& vo : sid_[k]->v) vo.gate = false;
                cur_ = -1;
                if (plMode_) { startPlaylistSong(); return; }   // brano successivo della colonna sonora
            }
        }
        if (cur_ >= 0) {
            for (int vi = 0; vi < 6; vi++) {
                if (vi == 5 && sfxActive) continue;
                VoiceTrack& vt = s.v[vi];
                if (vt.ev.empty()) continue;
                const NoteEvent* cur = nullptr;
                for (auto& e : vt.ev) if (row >= e.row && row < e.row + e.len) { cur = &e; }
                SidVoice& vo = sid_[vi / 3]->v[vi % 3];
                if (!cur) { vo.gate = false; continue; }
                auto it = s.inst.find(vt.inst);
                Instrument in = it == s.inst.end() ? Instrument() : it->second;
                int t = (int)(tickCount_ - (long)cur->row * s.tempo);
                applyNote(vi, *cur, in, t);
            }
            tickCount_++;
        }
    }
    if (sfxActive) {
        auto& d = sfxTable()[sfxIdx_];
        SidVoice& vo = sid_[1]->v[2];
        int acc = 0;
        const SfxStep* step = nullptr;
        for (auto& st : d.steps) { if (sfxTick_ < acc + st.ticks) { step = &st; break; } acc += st.ticks; }
        if (!step) { vo.gate = false; sfxIdx_ = -1; }
        else {
            vo.attack = d.ad >> 4; vo.decay = d.ad & 15; vo.sustain = d.sr >> 4; vo.release = d.sr & 15;
            vo.wave = step->wave; vo.pw = 0x800; vo.freq = sidFreqForNote(step->midi); vo.filtered = false;
            vo.gate = true;
            sfxTick_++;
        }
    }
}

void MusicPlayer::render(float* out, int frames) {
    float mv = musicVol_.load(), sv = sfxVol_.load();
    for (int i = 0; i < frames; i++) {
        if (--samplesToTick_ <= 0) { tick(); samplesToTick_ = SAMPLES_PER_TICK; }
        // guadagno ammorbidito campione per campione (curva quadratica: piu' naturale all'orecchio)
        gain_ += (fadeOut_ * fadeOut_ - gain_) * 0.0015f;
        float g = mv * gain_;
        float a = sid_[0]->sample() * g;
        // il secondo SID porta anche gli effetti sulla voce 3: volume misto
        float b = sid_[1]->sample() * (sfxIdx_ >= 0 ? (g * 0.6f + sv * 0.4f) : g);
        out[2 * i] = (a * 0.75f + b * 0.35f) * 0.9f;
        out[2 * i + 1] = (a * 0.35f + b * 0.75f) * 0.9f;
    }
}
