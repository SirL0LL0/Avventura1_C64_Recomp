// Emulazione semplificata del chip audio SID 6581 del C64 (3 voci, ADSR, forme d'onda, filtro).
// Non e' un emulatore ciclo per ciclo: riproduce il carattere del suono, non i difetti analogici.
#pragma once
#include <cstdint>

struct SidVoice {
    // "registri"
    uint16_t freq = 0;        // valore registro frequenza (PAL: Hz = freq * 985248 / 16777216)
    uint16_t pw = 0x800;      // ampiezza impulso 12 bit
    uint8_t wave = 0;         // bit: 1 tri 2 saw 4 pulse 8 noise
    bool gate = false;
    uint8_t attack = 0, decay = 0, sustain = 15, release = 0;
    bool filtered = false;
    // stato interno
    uint32_t acc = 0;
    uint32_t lfsr = 0x7FFFF8;
    float env = 0;
    int envState = 3;         // 0 attack 1 decay/sustain 3 release
    bool lastGate = false;
    float noiseOut = 0;
};

class Sid {
public:
    SidVoice v[3];
    uint16_t cutoff = 0x400;  // 11 bit
    uint8_t resonance = 0;    // 4 bit
    bool lp = true, bp = false, hp = false;
    uint8_t volume = 15;

    explicit Sid(double sampleRate);
    float sample();           // un campione mono (-1..1 circa)

private:
    double rate_;
    float low_ = 0, band_ = 0;
    float voiceOut(SidVoice& vo, double dt);
};

// conversione nota MIDI -> registro frequenza SID (PAL)
uint16_t sidFreqForNote(double midi);
