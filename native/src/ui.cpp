#include "ui.h"
#include "font_data.h"
#include "music.h"
#include <cmath>
#include <cstdlib>

static int glyphOf(unsigned char c) {
    if (c >= 'a' && c <= 'z') return c - 96;
    if (c >= 'A' && c <= 'Z') return c;            // maiuscole nel set minuscolo: $41-$5A
    if (c == '@') return 0;
    if (c >= 0x20 && c <= 0x3F) return c;
    if (c == '[') return 0x1B;
    if (c == ']') return 0x1D;
    if (c == '_') return 0x64;
    if (c == '|') return 0x5D;
    if (c == '~') return 0x66;
    return '?';
}

bool Ui::init(SDL_Renderer* renderer) {
    r = renderer;
    SDL_Surface* s = SDL_CreateSurface(128 * 8, 8, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return false;
    auto* px = (uint32_t*)s->pixels;
    int pitch = s->pitch / 4;
    for (int g = 0; g < 128; g++)
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++)
                px[y * pitch + g * 8 + x] = (FONT_LOWER[g][y] & (0x80 >> x)) ? 0xFFFFFFFF : 0x00000000;
    font_ = SDL_CreateTextureFromSurface(r, s);
    SDL_DestroySurface(s);
    if (!font_) return false;
    SDL_SetTextureScaleMode(font_, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(font_, SDL_BLENDMODE_BLEND);
    return true;
}

void Ui::beginFrame() {
    lastCount_ = count_;
    count_ = 0;
    hint_.clear();
    if (lastCount_ > 0) {
        if (key_ == SDLK_DOWN || key_ == SDLK_TAB) focus_ = (focus_ + 1) % lastCount_;
        if (key_ == SDLK_UP) focus_ = (focus_ + lastCount_ - 1) % lastCount_;
        if (focus_ >= lastCount_) focus_ = 0;
    }
    optDir_ = 0;
}

void Ui::endFrame() {
    click_ = false;
    key_ = 0;
    mouseMoved_ = false;
}

void Ui::handleEvent(const SDL_Event& e) {
    switch (e.type) {
        case SDL_EVENT_MOUSE_MOTION: mx_ = e.motion.x; my_ = e.motion.y; mouseMoved_ = true; break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: if (e.button.button == SDL_BUTTON_LEFT) { down_ = true; mx_ = e.button.x; my_ = e.button.y; } break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.button.button == SDL_BUTTON_LEFT) { if (down_) click_ = true; down_ = false; mx_ = e.button.x; my_ = e.button.y; }
            break;
        case SDL_EVENT_KEY_DOWN: key_ = e.key.key; break;
    }
}

void Ui::fill(float x, float y, float w, float h, Color c) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_FRect f{x, y, w, h};
    SDL_RenderFillRect(r, &f);
}

void Ui::frame(float x, float y, float w, float h, Color c, float t) {
    fill(x, y, w, t, c); fill(x, y + h - t, w, t, c);
    fill(x, y, t, h, c); fill(x + w - t, y, t, h, c);
}

void Ui::panel(float x, float y, float w, float h) {
    fill(x + 6, y + 6, w, h, Color{0, 0, 0, 120});
    fill(x, y, w, h, pal::panel);
    frame(x, y, w, h, pal::panelEdge, 3);
    frame(x + 6, y + 6, w - 12, h - 12, Color{60, 48, 100}, 1);
    // borchie agli angoli
    for (float cx : {x - 4, x + w - 8}) for (float cy : {y - 4, y + h - 8}) { fill(cx, cy, 12, 12, pal::goldDark); fill(cx + 3, cy + 3, 6, 6, pal::gold); }
}

void Ui::text(float x, float y, const std::string& s, float scale, Color c, Align a, bool shadow) {
    float w = textWidth(s, scale);
    if (a == CENTER) x -= w / 2;
    else if (a == RIGHT) x -= w;
    for (int pass = shadow ? 0 : 1; pass < 2; pass++) {
        if (pass == 0) { SDL_SetTextureColorMod(font_, 0, 0, 0); SDL_SetTextureAlphaMod(font_, 150); }
        else { SDL_SetTextureColorMod(font_, c.r, c.g, c.b); SDL_SetTextureAlphaMod(font_, c.a); }
        float off = pass == 0 ? std::fmax(2.0f, scale) : 0;
        float cx = x;
        for (unsigned char ch : s) {
            int g = glyphOf(ch);
            SDL_FRect src{(float)(g * 8), 0, 8, 8};
            SDL_FRect dst{cx + off, y + off, 8 * scale, 8 * scale};
            SDL_RenderTexture(r, font_, &src, &dst);
            cx += 8 * scale;
        }
    }
}

void Ui::background(double t) {
    // cielo: gradiente verticale notte -> viola
    for (int i = 0; i < 48; i++) {
        float k = i / 47.0f;
        Color c{(uint8_t)(10 + 40 * k), (uint8_t)(8 + 14 * k), (uint8_t)(30 + 46 * k)};
        fill(0, H * i / 48.0f, W, H / 48.0f + 1, c);
    }
    // stelle che brillano (posizioni pseudo-casuali fisse)
    unsigned seed = 12345;
    auto rnd = [&]() { seed = seed * 1103515245u + 12345u; return (seed >> 8) & 0xFFFF; };
    for (int i = 0; i < 140; i++) {
        float x = rnd() % W, y = rnd() % (H * 2 / 3);
        float ph = (rnd() % 628) / 100.0f, sp = 0.5f + (rnd() % 100) / 60.0f;
        float b = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(t * sp + ph));
        uint8_t v = (uint8_t)(120 + 135 * b);
        float sz = (i % 9 == 0) ? 3 : 2;
        fill(x, y, sz, sz, Color{v, v, (uint8_t)std::fmin(255, v + 20), (uint8_t)(255 * b)});
    }
    // luna
    float lx = W - 250.0f, ly = 150;
    for (int rr = 70; rr > 0; rr -= 6) fill(lx - rr, ly - rr, rr * 2, rr * 2, Color{255, 240, 200, 6});
    for (int yy = -34; yy <= 34; yy += 2) {
        float hw = std::sqrt(34.0f * 34 - yy * yy);
        fill(lx - hw, ly + yy, hw * 2, 2, Color{246, 236, 205});
        float hw2 = std::sqrt(std::fmax(0.0f, 30.0f * 30 - (yy + 6) * (yy + 6)));
        if (hw2 > 0) fill(lx - hw2 + 14, ly + yy, hw2 * 2, 2, Color{38, 26, 64});
    }
    // montagne a strati (silhouette)
    auto ridge = [&](float base, float amp, float freq, float ofs, Color c) {
        for (int x = 0; x < W; x += 4) {
            float h = base + amp * (0.6f * std::sin(x * freq + ofs) + 0.4f * std::sin(x * freq * 2.7f + ofs * 1.3f));
            fill((float)x, h, 4, H - h, c);
        }
    };
    ridge(560, 60, 0.006f, 1.0f, Color{40, 30, 72});
    ridge(630, 45, 0.011f, 2.0f, Color{28, 20, 52});
    ridge(700, 30, 0.017f, 4.0f, Color{16, 12, 32});
    // castello sulla collina
    float cx = 210, cy = 520;
    Color k{12, 9, 24};
    fill(cx, cy, 120, 120, k);
    fill(cx - 24, cy - 40, 36, 160, k); fill(cx + 108, cy - 52, 36, 172, k);
    fill(cx + 44, cy - 80, 32, 200, k);
    for (float tx : {cx - 24, cx + 108, cx + 44}) for (int j = 0; j < 3; j++) fill(tx + j * 12, (tx == cx + 44 ? cy - 92 : tx == cx - 24 ? cy - 52 : cy - 64), 8, 12, k);
    float flick = 0.7f + 0.3f * std::sin(t * 7.0) * std::sin(t * 3.1);
    Color win{255, (uint8_t)(170 + 60 * flick), 80, (uint8_t)(200 * flick + 55)};
    fill(cx + 54, cy - 50, 10, 14, win); fill(cx - 12, cy - 10, 8, 12, win); fill(cx + 120, cy - 20, 8, 12, win);
}

int Ui::nextWidget(float x, float y, float w, float h, bool enabled, bool& hot, bool& focused) {
    int id = count_++;
    bool inside = mx_ >= x && mx_ < x + w && my_ >= y && my_ < y + h;
    if (inside && mouseMoved_ && enabled) focus_ = id;
    hot = inside && enabled;
    focused = focus_ == id;
    return id;
}

bool Ui::button(const std::string& label, float x, float y, float w, float h, bool enabled, const std::string& hint) {
    bool hot, focused;
    nextWidget(x, y, w, h, enabled, hot, focused);
    bool act = enabled && ((hot && click_) || (focused && (key_ == SDLK_RETURN || key_ == SDLK_KP_ENTER || key_ == SDLK_SPACE)));
    if (act) g_music.sfx("clic");
    Color base = enabled ? (focused ? Color{92, 62, 150} : Color{44, 34, 82}) : Color{36, 32, 50};
    fill(x + 4, y + 4, w, h, Color{0, 0, 0, 110});
    fill(x, y, w, h, base);
    fill(x, y, w, 3, focused && enabled ? pal::gold : Color{80, 66, 130});
    frame(x, y, w, h, focused && enabled ? pal::gold : pal::panelEdge, 2);
    if (focused && enabled) {
        float px = x + 18, py = y + h / 2 - 8;
        for (int i = 0; i < 4; i++) fill(px + i * 3, py + i * 2, 3, 16 - i * 4, pal::gold);
        if (!hint.empty()) hint_ = hint;
    }
    float sc = 3;
    while (sc > 1.5f && textWidth(label, sc) > w - 56) sc -= 0.25f;
    text(x + w / 2 + (focused ? 8 : 0), y + h / 2 - 4 * sc, label, sc, enabled ? (focused ? pal::white : pal::parchment) : pal::dim, CENTER);
    return act;
}

bool Ui::option(const std::string& label, const std::string& value, float x, float y, float w) {
    bool hot, focused;
    float h = 52;
    nextWidget(x, y, w, h, true, hot, focused);
    bool act = false;
    if (hot && click_) { act = true; optDir_ = mx_ < x + w * 0.5f ? -1 : 1; if (mx_ >= x + w * 0.5f) optDir_ = 1; }
    if (focused && (key_ == SDLK_RIGHT || key_ == SDLK_RETURN || key_ == SDLK_SPACE)) { act = true; optDir_ = 1; }
    if (focused && key_ == SDLK_LEFT) { act = true; optDir_ = -1; }
    fill(x, y, w, h, focused ? Color{70, 52, 120} : Color{36, 28, 66});
    frame(x, y, w, h, focused ? pal::gold : pal::panelEdge, 2);
    std::string v = "< " + value + " >";
    float vw = textWidth(v, 3);
    float sc = 3;                                            // l'etichetta si adatta allo spazio a sinistra del valore
    while (sc > 1.5f && textWidth(label, sc) > w - vw - 64) sc -= 0.25f;
    text(x + 20, y + h / 2 - 4 * sc, label, sc, pal::parchment, LEFT);
    text(x + w - 20, y + 14, v, 3, focused ? pal::gold : pal::accent, RIGHT);
    return act;
}
