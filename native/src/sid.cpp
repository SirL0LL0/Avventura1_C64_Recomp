#include "sid.h"
#include <cmath>

namespace {
// tempi in millisecondi dal datasheet del 6581
const float ATTACK_MS[16] = {2, 8, 16, 24, 38, 56, 68, 80, 100, 250, 500, 800, 1000, 3000, 5000, 8000};
const float DECAY_MS[16] = {6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000};
const double PAL_CLOCK = 985248.0;
}

uint16_t sidFreqForNote(double midi) {
    double hz = 440.0 * std::pow(2.0, (midi - 69.0) / 12.0);
    double reg = hz * 16777216.0 / PAL_CLOCK;
    if (reg > 65535) reg = 65535;
    if (reg < 0) reg = 0;
    return (uint16_t)reg;
}

Sid::Sid(double sampleRate) : rate_(sampleRate) {}

float Sid::voiceOut(SidVoice& vo, double dt) {
    // ---- inviluppo ADSR
    if (vo.gate && !vo.lastGate) vo.envState = 0;
    if (!vo.gate && vo.lastGate) vo.envState = 3;
    vo.lastGate = vo.gate;
    float sus = vo.sustain / 15.0f;
    float ms = (float)(dt * 1000.0);
    switch (vo.envState) {
        case 0:
            vo.env += ms / ATTACK_MS[vo.attack & 15];
            if (vo.env >= 1) { vo.env = 1; vo.envState = 1; }
            break;
        case 1: {
            // discesa "esponenziale" come nel chip: piu' lenta vicino allo zero
            float k = vo.env > 0.36f ? 1.0f : vo.env > 0.12f ? 0.5f : 0.25f;
            if (vo.env > sus) { vo.env -= k * ms / DECAY_MS[vo.decay & 15]; if (vo.env < sus) vo.env = sus; }
            break;
        }
        default: {
            float k = vo.env > 0.36f ? 1.0f : vo.env > 0.12f ? 0.5f : 0.25f;
            vo.env -= k * ms / DECAY_MS[vo.release & 15];
            if (vo.env < 0) vo.env = 0;
        }
    }
    if (vo.env <= 0 || vo.wave == 0) return 0;
    // ---- oscillatore (accumulatore a 24 bit)
    uint32_t step = (uint32_t)(vo.freq * PAL_CLOCK * dt);
    uint32_t prev = vo.acc;
    vo.acc = (vo.acc + step) & 0xFFFFFF;
    float p = vo.acc / 16777216.0f;
    float out;
    if (vo.wave & 8) {
        // rumore: LFSR a 23 bit cadenzato dal bit 19 dell'accumulatore
        if ((prev ^ vo.acc) & 0x080000 || step > 0x080000) {
            uint32_t bit = ((vo.lfsr >> 22) ^ (vo.lfsr >> 17)) & 1;
            vo.lfsr = ((vo.lfsr << 1) | bit) & 0x7FFFFF;
            vo.noiseOut = ((vo.lfsr >> 15) & 0xFF) / 127.5f - 1.0f;
        }
        out = vo.noiseOut;
    } else if (vo.wave & 4) {
        out = p < (vo.pw & 0xFFF) / 4096.0f ? 1.0f : -1.0f;
        if (vo.wave & 2) out = out > 0 ? (2 * p - 1) : -0.2f;          // impulso+dente: suono "metallico"
    } else if (vo.wave & 2) {
        out = 2 * p - 1;
    } else {
        out = p < 0.5f ? 4 * p - 1 : 3 - 4 * p;
    }
    return out * vo.env;
}

float Sid::sample() {
    double dt = 1.0 / rate_;
    float direct = 0, filt = 0;
    for (auto& vo : v) {
        float s = voiceOut(vo, dt) * 0.33f;
        if (vo.filtered) filt += s; else direct += s;
    }
    // filtro a variabili di stato (Chamberlin)
    float fc = 30.0f + cutoff * 5.8f;
    if (fc > 11000) fc = 11000;
    float f = 2.0f * (float)std::sin(3.14159265 * fc / rate_);
    float q = 1.0f / (0.7f + resonance / 15.0f * 1.7f);
    float high = filt - low_ - q * band_;
    band_ += f * high;
    low_ += f * band_;
    float fo = (lp ? low_ : 0) + (bp ? band_ : 0) + (hp ? high : 0);
    return (direct + fo) * (volume / 15.0f);
}
