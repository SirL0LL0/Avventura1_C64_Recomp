// Avventura 1 - Recomp: applicazione nativa (SDL3).
// Launcher + finestra di gioco con la VM del P-code originale.
#include "app.h"
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
    }
}

void App::saveConfig() {
    char buf[1200];
    int n = std::snprintf(buf, sizeof buf, "speed=%d\ncrt=%d\nfullscreen=%d\ninteger_scale=%d\nhints=%d\ntap=%s\n",
                          cfg.speed, cfg.crt, cfg.fullscreen, cfg.integerScale, cfg.hints, cfg.tapPath.c_str());
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

void App::startGame() {
    c64 = std::make_unique<Screen>();
    host.app = this;
    vm = std::make_unique<VM>(game.mem, *c64, host);
    paused = tapePick = lineInput = false;
    endReason.clear();
    budgetCarry = 0;
    screen = Screen_::GAME;
    SDL_StartTextInput(win);
}

void App::stopGame() {
    vm.reset();
    c64.reset();
    SDL_StopTextInput(win);
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

// ------------------------------------------------------------------ disegno partita
static void drawGame(App& a, double dt) {
    Ui& u = a.ui;
    // esecuzione della VM
    if (!a.paused && !a.tapePick && a.endReason.empty() && a.vm && a.vm->state() == VM::RUN) {
        double mult = SPEEDS[a.cfg.speed].mult;
        double budget = UNITS_PER_SEC * mult * std::min(dt, 0.1);
        a.vm->run(budget);
    }
    if (a.endReason == "end" || a.endReason == "reset" || a.endReason == "stop") {
        a.stopGame();
        a.status = "Partita terminata. Alla prossima avventura!";
        a.statusError = false;
        return;
    }
    // schermo C64
    void* px; int pitch;
    if (SDL_LockTexture(a.c64tex, nullptr, &px, &pitch)) {
        a.c64->render((uint32_t*)px, pitch / 4, a.lineInput, SDL_GetTicks());
        SDL_UnlockTexture(a.c64tex);
    }
    SDL_FRect dst{0, 0, (float)Ui::W, (float)Ui::H};
    SDL_RenderTexture(a.ren, a.c64tex, nullptr, &dst);
    if (a.cfg.crt) {
        for (int y = 2; y < Ui::H; y += 3) u.fill(0, (float)y, Ui::W, 1, Color{0, 0, 0, 80});
        u.frame(0, 0, Ui::W, Ui::H, Color{0, 0, 0, 90}, 10);
    }
    if (a.cfg.hints && !a.paused) {
        u.fill(0, Ui::H - 34, Ui::W, 34, Color{0, 0, 0, 120});
        u.text(16, Ui::H - 25, "Tasti numerici per scegliere - F1 bevi il filtro - Esc menu", 2, Color{230, 230, 240, 220}, LEFT, false);
        u.text(Ui::W - 16, Ui::H - 25, SPEEDS[a.cfg.speed].name, 2, pal::gold, RIGHT, false);
    }
    if (a.time < a.toastUntil) {
        float w = u.textWidth(a.toast, 2.5f) + 60;
        float x = (Ui::W - w) / 2;
        u.fill(x, 20, w, 56, Color{20, 16, 40, 235});
        u.frame(x, 20, w, 56, pal::gold, 2);
        u.text(Ui::W / 2, 38, a.toast, 2.5f, pal::parchment, CENTER);
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
    int frame = 0;
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Avventura 1", SDL_GetError(), nullptr); return 1; }
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
    a.applyVideo();
    a.haveGame = a.loadCachedGame();
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
        if (a.screen == Screen_::GAME) drawGame(a, dt);
        else drawLauncher(a);
        a.ui.endFrame();
        if (!shotDir.empty()) {                  // modalita' cattura schermate (test)
            frame++;
            if (frame == 30) { saveShot(a, shotDir + "/1_menu.bmp"); a.screen = Screen_::OPTIONS; }
            if (frame == 40) { saveShot(a, shotDir + "/2_opzioni.bmp"); a.screen = Screen_::ABOUT; }
            if (frame == 50) { saveShot(a, shotDir + "/2b_info.bmp"); a.screen = Screen_::SAVES; a.refreshSaves(); }
            if (frame == 60) { saveShot(a, shotDir + "/2c_salvataggi.bmp"); a.screen = Screen_::MAIN; if (a.haveGame) a.startGame(); }
            if (frame == 300 && a.vm) a.vm->pushKey(' ');
            if (frame == 420) saveShot(a, shotDir + "/3_titolo.bmp");
            if (frame == 430 && a.vm) a.vm->pushKey(' ');
            if (frame == 560) saveShot(a, shotDir + "/4_gioco.bmp");
            if (frame == 570) { a.paused = true; }
            if (frame == 580) { saveShot(a, shotDir + "/5_pausa.bmp"); a.running = false; }
        }
        SDL_RenderPresent(a.ren);
    }
    a.saveConfig();
    SDL_DestroyTexture(a.c64tex);
    SDL_DestroyRenderer(a.ren);
    SDL_DestroyWindow(a.win);
    SDL_Quit();
    return 0;
}
