// Avventura 1 - Recomp: applicazione nativa (SDL3).
// Launcher + finestra di gioco con la VM del P-code originale.
#include "app.h"
#include "music.h"
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

const SpeedDef SPEEDS[] = {{"ORIGINALE", 1.0}, {"VELOCE", 3.0}, {"TURBO", 12.0}, {"ISTANTANEA", 400.0}};
const int SPEED_COUNT = 4;
static const double UNITS_PER_SEC = 2000.0;   // calibrazione: FOR..NEXT vuoto ~ 0,5 ms come sul C64

// ------------------------------------------------------------------ utilita'
std::string asciiFromPetscii(const std::string& s) {
    std::string o;
    for (unsigned char c : s) {
        if (c >= 0x41 && c <= 0x5A) o += (char)(c + 32);
        else if (c >= 0xC1 && c <= 0xDA) o += (char)(c - 0x80);
        else if (c >= 0x20 && c <= 0x40) o += (char)c;
    }
    return o;
}

static std::vector<uint8_t> readFile(const std::string& p) {
    size_t n = 0;
    void* d = SDL_LoadFile(p.c_str(), &n);
    std::vector<uint8_t> v;
    if (d) { v.assign((uint8_t*)d, (uint8_t*)d + n); SDL_free(d); }
    return v;
}
static bool writeFile(const std::string& p, const void* d, size_t n) { return SDL_SaveFile(p.c_str(), d, n); }

// ------------------------------------------------------------------ App
void App::loadConfig() {
    auto v = readFile(pref + "config.ini");
    std::string s(v.begin(), v.end());
    size_t i = 0;
    while (i < s.size()) {
        size_t e = s.find('\n', i);
        if (e == std::string::npos) e = s.size();
        std::string line = s.substr(i, e - i);
        i = e + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), val = line.substr(eq + 1);
        int n = std::atoi(val.c_str());
        if (k == "speed") cfg.speed = std::clamp(n, 0, SPEED_COUNT - 1);
        else if (k == "crt") cfg.crt = n != 0;
        else if (k == "fullscreen") cfg.fullscreen = n != 0;
        else if (k == "integer_scale") cfg.integerScale = n != 0;
        else if (k == "hints") cfg.hints = n != 0;
        else if (k == "tap") cfg.tapPath = val;
        else if (k == "mod") cfg.mod = val;
        else if (k == "music") cfg.musicVol = std::clamp(n, 0, 10);
        else if (k == "sfx") cfg.sfxVol = std::clamp(n, 0, 10);
    }
}

void App::saveConfig() {
    char buf[1200];
    int n = std::snprintf(buf, sizeof buf, "speed=%d\ncrt=%d\nfullscreen=%d\ninteger_scale=%d\nhints=%d\nmusic=%d\nsfx=%d\nmod=%s\ntap=%s\n",
                          cfg.speed, cfg.crt, cfg.fullscreen, cfg.integerScale, cfg.hints, cfg.musicVol, cfg.sfxVol, cfg.mod.c_str(), cfg.tapPath.c_str());
    writeFile(pref + "config.ini", buf, n);
}

void App::applyVideo() {
    SDL_SetWindowFullscreen(win, cfg.fullscreen);
    SDL_SetRenderLogicalPresentation(ren, Ui::W, Ui::H,
        cfg.integerScale ? SDL_LOGICAL_PRESENTATION_INTEGER_SCALE : SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

bool App::loadCachedGame() {
    auto v = readFile(pref + "gioco.bin");
    if (v.size() < 0x1000 || v.size() > 0x9000) return false;
    game.mem.assign(0x10000, 0);
    std::memcpy(&game.mem[0x0801], v.data(), v.size());
    game.entry = 0x0B41;
    game.end = (uint16_t)(0x0801 + v.size());
    std::string err;
    return validateImage(game, err);
}

void App::tryLoadTap(const std::string& path) {
    auto data = readFile(path);
    if (data.empty()) { status = "Impossibile leggere il file"; statusError = true; return; }
    GameImage g;
    std::string err;
    if (!loadGameFromTap(data, g, err)) { status = err; statusError = true; return; }
    game = g;
    haveGame = true;
    // copia locale dei dati ricavati (solo sul PC dell'utente)
    writeFile(pref + "gioco.bin", &game.mem[0x0801], game.end - 0x0801);
    cfg.tapPath = path;
    saveConfig();
    status = "Copia verificata: pronta per giocare!";
    statusError = false;
}

void App::refreshSaves() {
    saves.clear();
    std::string dir = pref + "saves";
    int n = 0;
    char** list = SDL_GlobDirectory(dir.c_str(), "*.sav", 0, &n);
    for (int i = 0; list && i < n; i++) {
        SaveInfo s;
        s.path = dir + "/" + list[i];
        auto v = readFile(s.path);
        std::string txt(v.begin(), v.end());
        s.name = txt.substr(0, txt.find('\n'));
        SDL_PathInfo info;
        if (SDL_GetPathInfo(s.path.c_str(), &info)) {
            s.mtime = info.modify_time;
            SDL_DateTime dt;
            if (SDL_TimeToDateTime(info.modify_time, &dt, true)) {
                char b[40];
                std::snprintf(b, sizeof b, "%02d/%02d/%04d %02d:%02d", dt.day, dt.month, dt.year, dt.hour, dt.minute);
                s.when = b;
            }
        }
        saves.push_back(s);
    }
    SDL_free(list);
    std::sort(saves.begin(), saves.end(), [](const SaveInfo& a, const SaveInfo& b) { return a.mtime > b.mtime; });
}

void App::scanMods() {
    modFiles.assign(1, ""); modNames.assign(1, "Originale");
    const char* b = SDL_GetBasePath();
    std::string dir = std::string(b ? b : "./") + "mods/";
    int n = 0;
    char** list = SDL_GlobDirectory(dir.c_str(), "*.avm", 0, &n);
    for (int i = 0; list && i < n; i++) {
        auto v = readFile(dir + list[i]);
        std::string txt(v.begin(), v.end()), name = list[i];
        size_t p = txt.find("\nname ");
        if (p != std::string::npos) name = txt.substr(p + 6, txt.find('\n', p + 1) - p - 6);
        modFiles.push_back(dir + list[i]);
        modNames.push_back(name);
    }
    SDL_free(list);
    modSel = 0;
    for (size_t i = 1; i < modFiles.size(); i++) if (!cfg.mod.empty() && modFiles[i].find(cfg.mod) != std::string::npos) modSel = (int)i;
}

void App::startGame() {
    c64 = std::make_unique<Screen>();
    host.app = this;
    vm = std::make_unique<VM>(game.mem, *c64, host);
    panelOn = false; pic = nullptr; subject.clear(); musicScene.clear();
    if (modSel > 0) {
        auto v = readFile(modFiles[modSel]);
        ModInfo md; std::string err;
        std::string txt(v.begin(), v.end());
        if (!parseMod(txt, md, err)) { status = err; statusError = true; vm.reset(); c64.reset(); return; }
        if (md.game != gameFingerprint(game.mem, game.end)) { status = "La mod non e' compatibile con questa copia del gioco"; statusError = true; vm.reset(); c64.reset(); return; }
        if (!vm->loadMod(md, err)) { status = err; statusError = true; vm.reset(); c64.reset(); return; }
        panelOn = true;
    }
    paused = tapePick = lineInput = false;
    endReason.clear();
    endAt = -1;
    budgetCarry = 0;
    screen = Screen_::GAME;
    SDL_StartTextInput(win);
}

void App::stopGame() {
    vm.reset();
    c64.reset();
    SDL_StopTextInput(win);
    choices.clear();
    if (handShown) { SDL_SetCursor(cursorArrow); handShown = false; }
    screen = Screen_::MAIN;
    ui.resetFocus();
}

// ------------------------------------------------------------------ host della VM
void GameHost::tapeWrite(const std::string& name, const std::string& data) {
    std::string clean = asciiFromPetscii(name);
    std::string base;
    for (char c : clean) base += (std::isalnum((unsigned char)c) ? c : '_');
    if (base.empty()) base = "avventura";
    SDL_Time t = 0;
    SDL_GetCurrentTime(&t);
    char fn[256];
    std::snprintf(fn, sizeof fn, "%ssaves/%s_%lld.sav", app->pref.c_str(), base.c_str(), (long long)(t / 1000000000));
    std::string content = (clean.empty() ? "Avventura" : clean) + "\n" + data;
    if (writeFile(fn, content.data(), content.size())) app->showToast("Avventura salvata sul nastro: " + (clean.empty() ? "senza nome" : clean));
    else app->showToast("ERRORE: salvataggio non riuscito");
}

void GameHost::tapeReadRequest(const std::string&) {
    app->refreshSaves();
    app->tapePick = true;
    app->pauseFocus = 0;
    app->ui.resetFocus();
}

void GameHost::lineInputRequest() { app->lineInput = true; app->lineBuf.clear(); }

void GameHost::onEnd(const std::string& reason) { app->endReason = reason; }

// registri di espansione: 0 musica, 1 effetto, 2 immagine (soggetto in MP$)
void GameHost::ioWrite(int reg, int v) {
    static const char* SONG[] = {"", "titolo", "taverna", "viaggio", "battaglia", "citta", "tempio", "eroe", "morte"};
    static const char* SFX[] = {"", "colpo", "ferita", "mancato", "moneta", "pozione", "livello", "clic", "magia", "porta"};
    if (reg == 0) app->musicScene = v >= 0 && v < 9 ? SONG[v] : "";
    else if (reg == 1) { if (v > 0 && v < 10) g_music.sfx(SFX[v]); }
    else if (reg == 2) {
        std::string s;
        if (v == 0 || !app->vm || !app->vm->varString("MP$", s)) { app->pic = nullptr; app->subject.clear(); return; }
        app->subject = asciiFromPetscii(s);
        app->pic = findPicture(app->pictures, app->subject);
    }
}

// ------------------------------------------------------------------ input di gioco
static void gameText(App& a, const char* utf8) {
    bool shift = SDL_GetModState() & SDL_KMOD_SHIFT;
    for (const unsigned char* p = (const unsigned char*)utf8; *p; p++) {
        unsigned char c = *p;
        int pet = -1;
        if (c == 0xC2 && p[1] == 0xA3) { pet = 0x5C; p++; }       // £
        else if (c >= 0x80) continue;
        else if (c >= 'a' && c <= 'z') pet = shift ? c - 'a' + 0xC1 : c - 32;
        else if (c >= 'A' && c <= 'Z') pet = shift ? c - 'A' + 0xC1 : c;
        else if (c >= 0x20 && c <= 0x40) pet = c;
        if (pet < 0) continue;
        if (a.lineInput) { if (a.lineBuf.size() < 80) { a.lineBuf += (char)pet; a.c64->out((uint8_t)pet); } }
        else a.vm->pushKey((uint8_t)pet);
    }
}

static void gameKey(App& a, SDL_Keycode k, SDL_Keymod mod) {
    bool shift = mod & SDL_KMOD_SHIFT;
    int pet = -1;
    switch (k) {
        case SDLK_RETURN: case SDLK_KP_ENTER: pet = 0x0D; break;
        case SDLK_BACKSPACE: case SDLK_DELETE: pet = 0x14; break;
        case SDLK_UP: pet = 0x91; break;
        case SDLK_DOWN: pet = 0x11; break;
        case SDLK_LEFT: pet = 0x9D; break;
        case SDLK_RIGHT: pet = 0x1D; break;
        case SDLK_HOME: pet = shift ? 0x93 : 0x13; break;
        case SDLK_F1: pet = shift ? 0x89 : 0x85; break;
        case SDLK_F2: pet = 0x89; break;
        case SDLK_F3: pet = shift ? 0x8A : 0x86; break;
        case SDLK_F4: pet = 0x8A; break;
        case SDLK_F5: pet = shift ? 0x8B : 0x87; break;
        case SDLK_F6: pet = 0x8B; break;
        case SDLK_F7: pet = shift ? 0x8C : 0x88; break;
        case SDLK_F8: pet = 0x8C; break;
    }
    if (pet < 0) return;
    if (a.lineInput) {
        if (pet == 0x0D) {
            a.lineInput = false;
            a.c64->cr();
            a.vm->lineProvide(a.lineBuf);
        } else if (pet == 0x14 && !a.lineBuf.empty()) { a.lineBuf.pop_back(); a.c64->out(0x14); }
        return;
    }
    a.vm->pushKey((uint8_t)pet);
}

// ------------------------------------------------------------------ scelte cliccabili
static const float C64W = 1152;   // schermo C64 (384x272) x3
static float g_gx = 0;            // ascissa dello schermo C64 (centrato senza pannello)
// cella C64 -> coordinate logiche (bordo 32x36 pixel, scala 3)
static SDL_FRect choiceRect(const Choice& c) {
    if (c.pad) {
        const float bw = 72, gap = 12, total = 9 * bw + 8 * gap;
        return SDL_FRect{g_gx + (C64W - total) / 2 + c.x0 * (bw + gap), 712, bw, 56};
    }
    const float s = 3, cw = 8 * s;
    return SDL_FRect{g_gx + (Screen::BORDER_X * s) + c.x0 * cw, (Screen::BORDER_Y * s) + c.y0 * cw,
                     (c.x1 - c.x0 + 1) * cw, (c.y1 - c.y0 + 1) * cw};
}

// gestione mouse/tastiera delle scelte; ritorna true se l'evento e' stato usato
static bool choiceEvent(App& a, const SDL_Event& e) {
    if (e.type == SDL_EVENT_MOUSE_MOTION) { a.gmx = e.motion.x; a.gmy = e.motion.y; a.selChoice = -1; return true; }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
        a.gmx = e.button.x; a.gmy = e.button.y;
        for (const Choice& c : a.choices) {
            SDL_FRect rc = choiceRect(c);
            if (a.gmx >= rc.x && a.gmx < rc.x + rc.w && a.gmy >= rc.y && a.gmy < rc.y + rc.h) { a.vm->pushKey(c.key); a.choices.clear(); return true; }
        }
        return true;
    }
    if (e.type == SDL_EVENT_KEY_DOWN && !a.choices.empty()) {
        int n = (int)a.choices.size();
        if (e.key.key == SDLK_DOWN || e.key.key == SDLK_TAB) { a.selChoice = (a.selChoice + 1) % n; return true; }
        if (e.key.key == SDLK_UP) { a.selChoice = a.selChoice <= 0 ? n - 1 : a.selChoice - 1; return true; }
        if ((e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) && a.selChoice >= 0) {
            a.vm->pushKey(a.choices[a.selChoice].key); a.selChoice = -1; a.choices.clear(); return true;
        }
    }
    return false;
}

// ------------------------------------------------------------------ disegno partita
static void drawGame(App& a, double dt) {
    Ui& u = a.ui;
    g_gx = a.panelOn ? 0.0f : (Ui::W - C64W) / 2;
    // esecuzione della VM
    if (!a.paused && !a.tapePick && a.endReason.empty() && a.vm && a.vm->state() == VM::RUN) {
        double mult = SPEEDS[a.cfg.speed].mult;
        double budget = UNITS_PER_SEC * mult * std::min(dt, 0.1);
        a.vm->run(budget);
    }
    const bool finished = a.endReason == "end" || a.endReason == "reset" || a.endReason == "stop";
    if (finished && a.endAt < 0) a.endAt = a.time;
    // schermo C64
    void* px; int pitch;
    if (SDL_LockTexture(a.c64tex, nullptr, &px, &pitch)) {
        a.c64->render((uint32_t*)px, pitch / 4, a.lineInput, SDL_GetTicks());
        SDL_UnlockTexture(a.c64tex);
    }
    SDL_FRect dst{g_gx, 0, C64W, (float)Ui::H};
    SDL_RenderTexture(a.ren, a.c64tex, nullptr, &dst);
    if (a.cfg.crt) {
        for (int y = 2; y < Ui::H; y += 3) u.fill(g_gx, (float)y, C64W, 1, Color{0, 0, 0, 80});
        u.frame(g_gx, 0, C64W, Ui::H, Color{0, 0, 0, 90}, 10);
    }
    // pannello laterale PETSCII dell'Espansione
    if (a.panelOn) {
        a.panel.build(a.vm.get(), a.pic, a.subject);
        void* pp; int pitch2;
        if (SDL_LockTexture(a.panelTex, nullptr, &pp, &pitch2)) { a.panel.render((uint32_t*)pp, pitch2 / 4); SDL_UnlockTexture(a.panelTex); }
        SDL_FRect pd{C64W + 8, 0, Panel::W * 24.0f, Panel::H * 24.0f};
        u.fill(C64W, 0, Ui::W - C64W, Ui::H, Color{0, 0, 0});
        SDL_RenderTexture(a.ren, a.panelTex, nullptr, &pd);
        if (a.cfg.crt) for (int y = 2; y < Ui::H; y += 3) u.fill(C64W, (float)y, Ui::W - C64W, 1, Color{0, 0, 0, 80});
    }
    // scelte cliccabili: riconosciute solo mentre il gioco aspetta un tasto
    if (a.vm && a.vm->waitingKey() && !a.paused && !a.tapePick && !a.lineInput && a.endReason.empty()) a.choices = detectChoices(*a.c64);
    else a.choices.clear();
    if (a.selChoice >= (int)a.choices.size()) a.selChoice = -1;
    a.hoverChoice = -1;
    for (size_t i = 0; i < a.choices.size(); i++) {
        SDL_FRect rc = choiceRect(a.choices[i]);
        if (a.gmx >= rc.x && a.gmx < rc.x + rc.w && a.gmy >= rc.y && a.gmy < rc.y + rc.h) a.hoverChoice = (int)i;
    }
    for (size_t i = 0; i < a.choices.size(); i++) {
        const Choice& c = a.choices[i];
        SDL_FRect rc = choiceRect(c);
        bool on = (int)i == a.hoverChoice || (int)i == a.selChoice;
        if (c.pad) {
            u.fill(rc.x + 3, rc.y + 3, rc.w, rc.h, Color{0, 0, 0, 120});
            u.fill(rc.x, rc.y, rc.w, rc.h, on ? Color{92, 62, 150} : Color{44, 34, 82});
            u.frame(rc.x, rc.y, rc.w, rc.h, on ? pal::gold : pal::panelEdge, 2);
            u.text(rc.x + rc.w / 2, rc.y + rc.h / 2 - 12, std::string(1, (char)c.key), 3, on ? pal::white : pal::parchment, CENTER);
            continue;
        }
        float pulse = 0.5f + 0.5f * (float)std::sin(a.time * 5.0);
        if (on) {
            u.fill(rc.x - 4, rc.y - 4, rc.w + 8, rc.h + 8, Color{246, 200, 92, (uint8_t)(40 + 30 * pulse)});
            u.frame(rc.x - 6, rc.y - 6, rc.w + 12, rc.h + 12, pal::gold, 3);
        } else {
            u.frame(rc.x - 4, rc.y - 4, rc.w + 8, rc.h + 8, Color{246, 200, 92, 70}, 1);
        }
    }
    bool wantHand = a.hoverChoice >= 0;
    if (wantHand != a.handShown) { SDL_SetCursor(wantHand ? a.cursorHand : a.cursorArrow); a.handShown = wantHand; }
    // fine partita: la schermata finale resta visibile finche' il giocatore non decide di uscire
    if (finished) {
        float pw = 760, ph = 120, px = g_gx + (C64W - pw) / 2, py = Ui::H - ph - 24;
        u.panel(px, py, pw, ph);
        u.text(g_gx + C64W / 2, py + 26, "FINE DELL'AVVENTURA", 3, pal::gold, CENTER);
        float blink = 0.6f + 0.4f * (float)std::sin(a.time * 4.0);
        u.text(g_gx + C64W / 2, py + 72, "Clicca o premi un tasto per tornare al menu", 2, Color{238, 226, 196, (uint8_t)(255 * blink)}, CENTER, false);
        if (a.time - a.endAt > 0.5 && u.anyInput()) {
            a.stopGame();
            a.status = "Partita terminata. Alla prossima avventura!";
            a.statusError = false;
        }
        return;
    }
    if (a.cfg.hints && !a.paused) {
        u.fill(g_gx, Ui::H - 34, C64W, 34, Color{0, 0, 0, 120});
        const std::string hint = "Clic o tasti per scegliere - Frecce+Invio - F1 bevi - Esc menu";
        const char* sp = SPEEDS[a.cfg.speed].name;
        float avail = C64W - 32 - u.textWidth(sp, 2) - 24, sc = 2;
        while (sc > 1 && u.textWidth(hint, sc) > avail) sc -= 0.125f;
        u.text(g_gx + 16, Ui::H - 17 - 4 * sc, hint, sc, Color{230, 230, 240, 220}, LEFT, false);
        u.text(g_gx + C64W - 16, Ui::H - 25, sp, 2, pal::gold, RIGHT, false);
    }
    if (a.time < a.toastUntil) {
        float w = u.textWidth(a.toast, 2.5f) + 60;
        float x = g_gx + (C64W - w) / 2;
        u.fill(x, 20, w, 56, Color{20, 16, 40, 235});
        u.frame(x, 20, w, 56, pal::gold, 2);
        u.text(g_gx + C64W / 2, 38, a.toast, 2.5f, pal::parchment, CENTER);
    }
    // dialogo: scelta del nastro da caricare
    if (a.tapePick) {
        u.fill(0, 0, Ui::W, Ui::H, Color{0, 0, 0, 150});
        float pw = 820, px = (Ui::W - pw) / 2, py = 150;
        u.panel(px, py, pw, 520);
        u.text(Ui::W / 2, py + 30, "NASTRO VIRTUALE", 4, pal::gold, CENTER);
        u.text(Ui::W / 2, py + 80, "Scegli l'avventura da caricare", 2.5f, pal::parchment, CENTER);
        int k = 0;
        for (auto& s : a.saves) {
            if (k >= 5) break;
            if (u.button(s.name + "  " + s.when, px + 40, py + 130 + k * 64, pw - 80, 54)) {
                auto v = readFile(s.path);
                std::string t(v.begin(), v.end());
                size_t nl = t.find('\n');
                a.tapePick = false;
                a.vm->tapeProvide(true, s.name, nl == std::string::npos ? "" : t.substr(nl + 1));
                return;
            }
            k++;
        }
        if (a.saves.empty()) u.text(Ui::W / 2, py + 200, "Nessun salvataggio sul nastro.", 3, pal::danger, CENTER);
        if (u.button("ANNULLA", px + pw / 2 - 150, py + 450, 300, 50) || u.backPressed()) {
            a.tapePick = false;
            a.vm->tapeProvide(false, "", "");
        }
        return;
    }
    // errore del programma
    if (a.endReason == "error") {
        u.fill(0, 0, Ui::W, Ui::H, Color{0, 0, 0, 140});
        float pw = 760, px = (Ui::W - pw) / 2, py = 270;
        u.panel(px, py, pw, 260);
        u.text(Ui::W / 2, py + 34, "IL PROGRAMMA SI E' FERMATO", 3, pal::danger, CENTER);
        u.text(Ui::W / 2, py + 90, a.vm ? a.vm->errorText() : "", 3, pal::parchment, CENTER);
        if (u.button("TORNA AL MENU", px + pw / 2 - 170, py + 170, 340, 56) || u.backPressed()) { a.stopGame(); return; }
        return;
    }
    // menu di pausa
    if (a.paused) {
        u.fill(0, 0, Ui::W, Ui::H, Color{0, 0, 0, 150});
        float pw = 520, px = (Ui::W - pw) / 2, py = 180;
        u.panel(px, py, pw, 460);
        u.text(Ui::W / 2, py + 30, "PAUSA", 5, pal::gold, CENTER);
        float bx = px + 50, bw = pw - 100;
        if (u.button("RIPRENDI", bx, py + 110, bw, 56) || u.backPressed()) { a.paused = false; return; }
        if (u.button("VELOCITA': " + std::string(SPEEDS[a.cfg.speed].name), bx, py + 180, bw, 56)) { a.cfg.speed = (a.cfg.speed + 1) % SPEED_COUNT; a.saveConfig(); }
        if (u.button(std::string("EFFETTO CRT: ") + (a.cfg.crt ? "SI" : "NO"), bx, py + 250, bw, 56)) { a.cfg.crt = !a.cfg.crt; a.saveConfig(); }
        if (u.button("RICOMINCIA", bx, py + 320, bw, 56, true, "Riavvia il gioco dall'inizio")) { a.startGame(); return; }
        if (u.button("TORNA AL MENU", bx, py + 390 - 10 + 10, bw, 56)) { a.stopGame(); return; }
    }
}

// ------------------------------------------------------------------ icona
static SDL_Surface* makeIcon() {
    SDL_Surface* s = SDL_CreateSurface(32, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return nullptr;
    auto* p = (uint32_t*)s->pixels;
    int pitch = s->pitch / 4;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            uint32_t c = 0xFF2A1F52;
            if (x < 2 || y < 2 || x > 29 || y > 29) c = 0xFFF6C85C;
            // spada stilizzata
            if (x >= 15 && x <= 16 && y >= 4 && y <= 21) c = 0xFFE8E8F0;
            if (y >= 21 && y <= 22 && x >= 10 && x <= 21) c = 0xFFF6C85C;
            if (x >= 15 && x <= 16 && y >= 23 && y <= 27) c = 0xFF8A5A20;
            p[y * pitch + x] = c;
        }
    return s;
}

// ------------------------------------------------------------------ main
static bool pathExists(const std::string& p) { SDL_PathInfo i; return SDL_GetPathInfo(p.c_str(), &i); }

// Versione portable: tutti i dati vanno in "dati\" accanto all'eseguibile.
// Al primo avvio copia i dati di una versione precedente da %APPDATA%, se ci sono.
static std::string portableDataDir(App& a) {
    const char* b = SDL_GetBasePath();
    std::string base = b ? b : "./";
    std::string dir = base + "dati/";
    bool fresh = !pathExists(dir);
    SDL_CreateDirectory((dir + "saves").c_str());
    // controllo di scrittura (es. cartella in Programmi senza permessi)
    std::string probe = dir + ".scrittura";
    if (!writeFile(probe, "ok", 2)) {
        char* pp = SDL_GetPrefPath("Avventura1Recomp", "Avventura1");
        std::string fb = pp ? pp : "./";
        SDL_free(pp);
        SDL_CreateDirectory((fb + "saves").c_str());
        a.status = "Cartella del gioco non scrivibile: dati in AppData";
        a.statusError = true;
        return fb;
    }
    SDL_RemovePath(probe.c_str());
    const char* appdata = SDL_getenv("APPDATA");
    if (fresh && appdata) {
        std::string old = std::string(appdata) + "/Avventura1Recomp/Avventura1/";
        if (pathExists(old)) {
            for (const char* f : {"config.ini", "gioco.bin"})
                if (pathExists(old + f)) SDL_CopyFile((old + f).c_str(), (dir + f).c_str());
            int n = 0;
            char** list = SDL_GlobDirectory((old + "saves").c_str(), "*.sav", 0, &n);
            for (int i = 0; list && i < n; i++)
                SDL_CopyFile((old + "saves/" + list[i]).c_str(), (dir + "saves/" + list[i]).c_str());
            SDL_free(list);
            a.status = "Dati importati dalla versione precedente";
            a.statusError = false;
        }
    }
    return dir;
}

static void saveShot(App& a, const std::string& path) {
    SDL_Surface* s = SDL_RenderReadPixels(a.ren, nullptr);
    if (s) { SDL_SaveBMP(s, path.c_str()); SDL_DestroySurface(s); }
}

int main(int argc, char** argv) {
    std::string shotDir, tapArg;
    for (int i = 1; i + 1 < argc; i++) {
        if (std::strcmp(argv[i], "--shot") == 0) shotDir = argv[i + 1];
        if (std::strcmp(argv[i], "--tap") == 0) tapArg = argv[i + 1];
    }
    if (argc == 2 && argv[1][0] != '-') tapArg = argv[1];      // .tap trascinato sull'eseguibile
    // --wav <cartella>: esporta tutti i brani in WAV (ascolto e test) ed esce
    for (int i = 1; i + 1 < argc; i++)
        if (std::strcmp(argv[i], "--wav") == 0) {
            SDL_Init(0);
            for (const char* s : {"titolo", "taverna", "viaggio", "battaglia", "citta", "tempio", "eroe", "morte"}) {
                float pk = g_music.exportWav(s, 30, std::string(argv[i + 1]) + "/" + s + ".wav");
                std::printf("%-10s picco=%.3f\n", s, pk);
            }
            SDL_Quit();
            return 0;
        }
    int frame = 0;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) { SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Avventura 1", SDL_GetError(), nullptr); return 1; }
    App a;
    a.pref = portableDataDir(a);
    a.loadConfig();

    int ww = Ui::W, wh = Ui::H;
    SDL_Rect usable;
    if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable) && usable.h < Ui::H + 80) { ww = Ui::W * 2 / 3; wh = Ui::H * 2 / 3; }
    a.win = SDL_CreateWindow("Avventura 1 - Fantasy", ww, wh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!a.win) { SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Avventura 1", SDL_GetError(), nullptr); return 1; }
    if (SDL_Surface* icon = makeIcon()) { SDL_SetWindowIcon(a.win, icon); SDL_DestroySurface(icon); }
    a.ren = SDL_CreateRenderer(a.win, nullptr);
    SDL_SetRenderVSync(a.ren, 1);
    a.c64tex = SDL_CreateTexture(a.ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Screen::PIX_W, Screen::PIX_H);
    SDL_SetTextureScaleMode(a.c64tex, SDL_SCALEMODE_NEAREST);
    a.ui.init(a.ren);
    loadPictures(a.pictures);
    a.panelTex = SDL_CreateTexture(a.ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Panel::W * 8, Panel::H * 8);
    SDL_SetTextureScaleMode(a.panelTex, SDL_SCALEMODE_NEAREST);
    a.scanMods();
    a.cursorHand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    a.cursorArrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    a.applyVideo();
    a.haveGame = a.loadCachedGame();
    g_music.open();
    g_music.setVolumes(a.cfg.musicVol / 10.0f, a.cfg.sfxVol / 10.0f);
    if (!tapArg.empty()) a.tryLoadTap(tapArg);

    uint64_t last = SDL_GetTicksNS();
    while (a.running) {
        a.ui.beginFrame();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) a.running = false;
            if (e.type == SDL_EVENT_DROP_FILE && e.drop.data) a.tryLoadTap(e.drop.data);
            SDL_ConvertEventToRenderCoordinates(a.ren, &e);
            bool inGame = a.screen == Screen_::GAME && !a.paused && !a.tapePick && a.endReason.empty();
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_F11) { a.cfg.fullscreen = !a.cfg.fullscreen; a.applyVideo(); a.saveConfig(); continue; }
            if (inGame) {
                if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) { a.paused = true; a.ui.resetFocus(); continue; }
                if (choiceEvent(a, e)) continue;
                if (e.type == SDL_EVENT_TEXT_INPUT) gameText(a, e.text.text);
                if (e.type == SDL_EVENT_KEY_DOWN) gameKey(a, e.key.key, e.key.mod);
                continue;
            }
            a.ui.handleEvent(e);
        }
        {
            std::string p;
            { std::lock_guard<std::mutex> lk(a.pickMx); p.swap(a.picked); }
            if (!p.empty()) a.tryLoadTap(p);
        }
        uint64_t now = SDL_GetTicksNS();
        double dt = (now - last) / 1e9;
        last = now;
        a.time += dt;

        SDL_SetRenderDrawColor(a.ren, 0, 0, 0, 255);
        SDL_RenderClear(a.ren);
        // musica di scena (in partita la sceglie la mod tramite i registri $DE00; di base: viaggio)
        if (a.screen != Screen_::GAME) g_music.play("titolo");
        else if (a.endReason == "error" || a.endReason == "reset") {}
        else if (a.musicScene.empty()) g_music.play("viaggio");
        else g_music.play(a.musicScene);
        if (a.screen == Screen_::GAME) drawGame(a, dt);
        else drawLauncher(a);
        a.ui.endFrame();
        if (!shotDir.empty()) {                  // modalita' cattura schermate (test)
            frame++;
            if (frame == 30) { saveShot(a, shotDir + "/1_menu.bmp"); a.screen = Screen_::OPTIONS; }
            if (frame == 40) { saveShot(a, shotDir + "/2_opzioni.bmp"); a.screen = Screen_::ABOUT; }
            if (frame == 50) { saveShot(a, shotDir + "/2b_info.bmp"); a.screen = Screen_::SAVES; a.refreshSaves(); }
            if (frame == 60) { saveShot(a, shotDir + "/2c_salvataggi.bmp"); a.screen = Screen_::MAIN; if (a.modNames.size() > 1) a.modSel = 1; if (a.haveGame) a.startGame(); }
            if (frame == 300 && a.vm) a.vm->pushKey(' ');
            if (frame == 410 && !a.choices.empty()) {           // mouse sulla seconda scelta
                SDL_FRect rc = choiceRect(a.choices[a.choices.size() > 1 ? 1 : 0]);
                a.gmx = rc.x + rc.w / 2; a.gmy = rc.y + rc.h / 2;
            }
            if (frame == 420) saveShot(a, shotDir + "/3_titolo.bmp");
            if (frame == 430 && a.vm) {                           // clic simulato sulla scelta evidenziata
                SDL_Event ev{}; ev.type = SDL_EVENT_MOUSE_BUTTON_DOWN; ev.button.button = SDL_BUTTON_LEFT;
                ev.button.x = a.gmx; ev.button.y = a.gmy;
                if (!choiceEvent(a, ev) || a.choices.size()) a.vm->pushKey(' ');
            }
            if (frame == 550 && !a.choices.empty()) {
                SDL_FRect rc = choiceRect(a.choices[0]);
                a.gmx = rc.x + rc.w / 2; a.gmy = rc.y + rc.h / 2;
            }
            if (frame == 560) saveShot(a, shotDir + "/4_gioco.bmp");
            if (frame == 565 && a.vm) a.vm->pushKey('1');
            if (frame == 700) saveShot(a, shotDir + "/4b_taverna.bmp");
            if (frame == 705) { a.paused = true; }
            if (frame == 715) { saveShot(a, shotDir + "/5_pausa.bmp"); a.paused = false; a.endReason = "reset"; }
            if (frame == 735) { saveShot(a, shotDir + "/6_fine.bmp"); a.running = false; }
        }
        SDL_RenderPresent(a.ren);
    }
    a.saveConfig();
    g_music.close();
    SDL_DestroyTexture(a.c64tex);
    SDL_DestroyRenderer(a.ren);
    SDL_DestroyWindow(a.win);
    SDL_Quit();
    return 0;
}
