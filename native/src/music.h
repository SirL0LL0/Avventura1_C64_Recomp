// Player musicale per due SID (6 voci): brani scritti in un formato testuale compatto
// (vedi songs.h), effetti sonori, uscita audio SDL3.
#pragma once
#include "sid.h"
#include <SDL3/SDL.h>
#include <atomic>
#include <map>
#include <string>
#include <vector>

struct Instrument {
    uint8_t wave = 4, attack = 0, decay = 9, sustain = 8, release = 9;
    uint16_t pw = 0x800;
    int sweep = 0;            // variazione ampiezza impulso per tick
    float vibDepth = 0;       // centesimi di semitono
    float vibSpeed = 0;       // radianti per tick
    int vibDelay = 8;         // tick prima del vibrato
    bool filtered = false;
};

struct NoteEvent { int row, len, midi; int kind; };   // kind: 0 nota, 1 arpeggio, 2 kick, 3 snare, 4 hat

struct VoiceTrack {
    std::string inst;
    std::vector<NoteEvent> ev;
    std::vector<std::vector<int>> arps;   // intervalli dell'arpeggio per evento kind==1
};

struct Song {
    std::string name;
    int tempo = 8, bar = 8, rows = 0;
    bool loop = true;
    uint16_t cutoff[2] = {0x500, 0x500};
    uint8_t res[2] = {4, 4};
    bool lp[2] = {true, true}, bp[2] = {false, false}, hp[2] = {false, false};
    std::map<std::string, Instrument> inst;
    VoiceTrack v[6];
};

class MusicPlayer {
public:
    bool open();                          // apre il dispositivo audio
    void close();
    void play(const std::string& song);   // "" = silenzio; non riparte se e' gia' in corso
    void sfx(const std::string& name);
    void setVolumes(float music, float sfx) { musicVol_ = music; sfxVol_ = sfx; }
    const std::string& current() const { return currentName_; }
    std::vector<std::string> songNames() const;
    // genera un brano su file WAV senza dispositivo audio (test e ascolto); ritorna il picco
    float exportWav(const std::string& song, double seconds, const std::string& path);

private:
    static void SDLCALL callback(void* ud, SDL_AudioStream* s, int additional, int total);
    void render(float* out, int frames);
    void tick();
    void startSong(int idx);
    void applyNote(int vi, const NoteEvent& e, const Instrument& in, int tickInNote);

    SDL_AudioStream* stream_ = nullptr;
    Sid* sid_[2] = {nullptr, nullptr};
    std::vector<Song> songs_;
    std::atomic<int> request_{-2};        // -2 nessuna richiesta, -1 silenzio
    std::atomic<int> sfxRequest_{-1};
    std::atomic<float> musicVol_{0.8f}, sfxVol_{0.9f};
    std::string currentName_;
    int cur_ = -1;
    long tickCount_ = 0;
    int samplesToTick_ = 0;
    // stato per voce
    struct VState { int evIdx = -1; uint16_t pw = 0x800; float vibPhase = 0; int noteTick = 0; float fade = 1; };
    VState vs_[6];
    // effetti
    int sfxIdx_ = -1, sfxTick_ = 0;
    float fadeOut_ = 1.0f;                // volume del brano 0..1 (dissolvenze)
    float gain_ = 0.0f;                   // volume ammorbidito usato in uscita
    int pendingSong_ = -2;
};

extern MusicPlayer g_music;
