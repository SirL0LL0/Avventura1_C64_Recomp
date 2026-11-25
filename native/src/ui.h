// GUI immediate-mode minimale disegnata con il font pixel del progetto.
// Coordinate logiche 1152x816 (= schermo C64 384x272 x3).
#pragma once
#include <SDL3/SDL.h>
#include <string>

struct Color { uint8_t r, g, b, a = 255; };

namespace pal {
const Color gold{246, 200, 92}, goldDark{150, 104, 30}, ink{18, 14, 34}, parchment{238, 226, 196},
    dim{150, 140, 170}, white{255, 255, 255}, accent{120, 196, 255}, danger{235, 105, 90}, ok{120, 220, 140},
    panel{24, 20, 46, 230}, panelEdge{96, 78, 150};
}

enum Align { LEFT, CENTER, RIGHT };

class Ui {
public:
    static const int W = 1152, H = 816;
    SDL_Renderer* r = nullptr;

    bool init(SDL_Renderer* renderer);
    void beginFrame();                       // azzera i widget del fotogramma
    void endFrame();
    void handleEvent(const SDL_Event& e);    // eventi gia' convertiti in coordinate logiche

    // disegno
    void fill(float x, float y, float w, float h, Color c);
    void frame(float x, float y, float w, float h, Color c, float t = 2);
    void panel(float x, float y, float w, float h);
    void text(float x, float y, const std::string& s, float scale, Color c, Align a = LEFT, bool shadow = true);
    float textWidth(const std::string& s, float scale) const { return s.size() * 8 * scale; }
    void background(double t);              // cielo notturno animato

    // widget: ritornano true quando attivati (clic o Invio con il focus)
    bool button(const std::string& label, float x, float y, float w, float h, bool enabled = true, const std::string& hint = "");
    bool option(const std::string& label, const std::string& value, float x, float y, float w);   // cicla con clic/frecce
    int optionDir() const { return optDir_; }
    const std::string& hoverHint() const { return hint_; }

    bool keyPressed(SDL_Keycode k) const { return key_ == k; }
    bool backPressed() const { return key_ == SDLK_ESCAPE; }
    void resetFocus() { focus_ = 0; }

private:
    SDL_Texture* font_ = nullptr;
    float mx_ = -1, my_ = -1;
    bool click_ = false, down_ = false;
    SDL_Keycode key_ = 0;
    int focus_ = 0, count_ = 0, lastCount_ = 0;
    int optDir_ = 0;
    std::string hint_;
    bool mouseMoved_ = false;
    int nextWidget(float x, float y, float w, float h, bool enabled, bool& hot, bool& focused);
};
