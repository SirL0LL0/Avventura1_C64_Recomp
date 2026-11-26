#include "music.h"
#include "songs.h"
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
                        int oct = tok.empty() ? 4 : std::atoi(tok[0].c_str());
                        vt.ev.push_back({row, s->bar, 12 * (oct + 1) + root + iv[1 < iv.size() ? 1 : 0], 0});
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

} // namespace

std::vector<std::string> MusicPlayer::songNames() const {
    std::vector<std::string> n;
    for (auto& s : songs_) n.push_back(s.name);
    return n;
}

bool MusicPlayer::open() {
    songs_ = parseSongs(SONGS_TEXT);
    sid_[0] = new Sid(RATE);
    sid_[1] = new Sid(RATE);
    SDL_AudioSpec spec{SDL_AUDIO_F32, 2, RATE};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, callback, this);
    if (!stream_) return false;
    SDL_ResumeAudioStreamDevice(stream_);
    return true;
}

float MusicPlayer::exportWav(const std::string& song, double seconds, const std::string& path) {
    if (songs_.empty()) songs_ = parseSongs(SONGS_TEXT);
    if (!sid_[0]) { sid_[0] = new Sid(RATE); sid_[1] = new Sid(RATE); }
    int idx = -1;
    for (size_t i = 0; i < songs_.size(); i++) if (songs_[i].name == song) idx = (int)i;
    if (idx < 0) return -1;
    startSong(idx);
    request_ = -2; pendingSong_ = -2; fadeOut_ = 1;
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
    if (rq != -2) { pendingSong_ = rq; fadeOut_ = cur_ >= 0 ? 0.999f : 0.0f; }
    if (pendingSong_ != -2) {
        // dissolvenza rapida prima di cambiare brano
        fadeOut_ -= 0.12f;
        if (fadeOut_ <= 0) { startSong(pendingSong_); pendingSong_ = -2; fadeOut_ = 1; }
    }
    int sr = sfxRequest_.exchange(-1);
    if (sr >= 0) { sfxIdx_ = sr; sfxTick_ = 0; }
    bool sfxActive = sfxIdx_ >= 0;
    if (cur_ >= 0) {
        Song& s = songs_[cur_];
        long row = tickCount_ / s.tempo;
        if (s.rows > 0 && row >= s.rows) {
            if (s.loop) { tickCount_ = 0; row = 0; }
            else { for (int k = 0; k < 2; k++) for (auto& vo : sid_[k]->v) vo.gate = false; cur_ = -1; }
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
        float a = sid_[0]->sample() * mv * fadeOut_;
        // il secondo SID porta anche gli effetti sulla voce 3: volume misto
        float b = sid_[1]->sample() * (sfxIdx_ >= 0 ? (mv * fadeOut_ * 0.6f + sv * 0.4f) : mv * fadeOut_);
        out[2 * i] = (a * 0.75f + b * 0.35f) * 0.9f;
        out[2 * i + 1] = (a * 0.35f + b * 0.75f) * 0.9f;
    }
}
