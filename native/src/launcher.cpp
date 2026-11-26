// Schermate del launcher: menu principale, opzioni, salvataggi, informazioni.
#include "app.h"
#include "music.h"
#include <algorithm>
#include <cmath>

static void fileDialogCallback(void* ud, const char* const* files, int) {
    App* app = (App*)ud;
    if (!files || !files[0]) return;
    std::lock_guard<std::mutex> lk(app->pickMx);
    app->picked = files[0];
}

void openTapDialog(App& app) {
    static const SDL_DialogFileFilter filters[] = {{"Nastro C64 (*.tap)", "tap"}, {"Tutti i file", "*"}};
    SDL_ShowOpenFileDialog(fileDialogCallback, &app, app.win, filters, 2, nullptr, false);
}

static void title(App& a, float y) {
    Ui& u = a.ui;
    float wob = (float)std::sin(a.time * 1.3) * 3;
    u.text(Ui::W / 2, y + wob, "AVVENTURA 1", 10, Color{110, 62, 10}, CENTER, false);
    u.text(Ui::W / 2 - 4, y - 4 + wob, "AVVENTURA 1", 10, pal::gold, CENTER, true);
    u.text(Ui::W / 2, y + 96, "~ F A N T A S Y ~", 3, pal::parchment, CENTER);
    u.text(Ui::W / 2, y + 136, "di A. Castellari", 2.5f, pal::gold, CENTER);
}

static void footer(App& a) {
    Ui& u = a.ui;
    u.fill(0, Ui::H - 44, Ui::W, 44, Color{6, 4, 14, 200});
    std::string h = u.hoverHint();
    if (h.empty()) h = "Frecce/mouse per scegliere - Invio per confermare - Esc per tornare";
    const std::string ver = "Recomp v" APP_VERSION;
    float avail = Ui::W - 48 - u.textWidth(ver, 2) - 32;      // spazio libero a sinistra della versione
    float sc = 2;
    while (sc > 1 && u.textWidth(h, sc) > avail) sc -= 0.125f;
    u.text(24, Ui::H - 22 - 4 * sc, h, sc, pal::dim, LEFT, false);
    u.text(Ui::W - 24, Ui::H - 30, ver, 2, pal::dim, RIGHT, false);
}

static void mainMenu(App& a) {
    Ui& u = a.ui;
    title(a, 70);
    float pw = 560, px = (Ui::W - pw) / 2, py = 246;
    u.panel(px, py, pw, 500);
    // stato della copia del gioco
    if (a.haveGame) {
        u.fill(px + 30, py + 28, 16, 16, pal::ok);
        u.text(px + 58, py + 26, "Copia originale pronta", 2.5f, pal::ok, LEFT);
    } else {
        u.fill(px + 30, py + 28, 16, 16, pal::danger);
        u.text(px + 58, py + 26, "Manca la copia del gioco", 2.5f, pal::danger, LEFT);
    }
    float bx = px + 50, bw = pw - 100, by = py + 66, bh = 48, gap = 56;
    if (u.button("GIOCA", bx, by, bw, bh, a.haveGame, "Avvia l'avventura")) a.startGame();
    {
        std::string v = "VERSIONE: " + a.modNames[a.modSel];
        for (auto& ch : v) ch = (char)std::toupper((unsigned char)ch);
        if (u.button(v, bx, by + gap * 1, bw, bh, a.modNames.size() > 1,
                     a.modNames.size() > 1 ? "Originale o Espansione (grafica, musica, nuove avventure)" : "Nessuna mod nella cartella mods")) {
            a.modSel = (a.modSel + 1) % (int)a.modNames.size();
            const std::string& fpath = a.modFiles[a.modSel];
            a.cfg.mod = fpath.empty() ? "" : fpath.substr(fpath.find_last_of("/\\") + 1);
            a.saveConfig();
        }
    }
    if (u.button(a.haveGame ? "CAMBIA COPIA (.TAP)" : "FORNISCI COPIA (.TAP)", bx, by + gap * 2, bw, bh, true,
                 "Scegli il .tap originale (o trascinalo qui)"))
        openTapDialog(a);
    if (u.button("SALVATAGGI", bx, by + gap * 3, bw, bh, true, "Le avventure salvate sul nastro virtuale")) { a.refreshSaves(); a.screen = Screen_::SAVES; u.resetFocus(); }
    if (u.button("OPZIONI", bx, by + gap * 4, bw, bh, true, "Velocita', schermo, effetto CRT")) { a.optionsReturn = Screen_::MAIN; a.screen = Screen_::OPTIONS; u.resetFocus(); }
    if (u.button("INFORMAZIONI", bx, by + gap * 5, bw, bh, true, "Crediti e note legali")) { a.screen = Screen_::ABOUT; u.resetFocus(); }
    if (u.button("ESCI", bx, by + gap * 6, bw, bh, true, "Chiudi il programma")) a.running = false;
    if (!a.status.empty()) {
        float sc = 2;
        while (sc > 1 && u.textWidth(a.status, sc) > pw - 60) sc -= 0.125f;
        u.text(Ui::W / 2, py + 470, a.status, sc, a.statusError ? pal::danger : pal::ok, CENTER);
    }
    if (u.backPressed()) a.running = false;
}

static void optionsMenu(App& a) {
    Ui& u = a.ui;
    u.text(Ui::W / 2, 60, "OPZIONI", 6, pal::gold, CENTER);
    float pw = 760, px = (Ui::W - pw) / 2, py = 150;
    u.panel(px, py, pw, 560);
    float x = px + 40, w = pw - 80, y = py + 30, g = 58;
    auto bar = [](int v) { std::string s; for (int i = 0; i < 10; i++) s += i < v ? '|' : '.'; return s; };
    if (u.option("Musica", a.cfg.musicVol ? bar(a.cfg.musicVol) : "NO", x, y + g * 5, w)) {
        a.cfg.musicVol = std::clamp(a.cfg.musicVol + u.optionDir(), 0, 10);
        g_music.setVolumes(a.cfg.musicVol / 10.0f, a.cfg.sfxVol / 10.0f);
    }
    if (u.option("Effetti sonori", a.cfg.sfxVol ? bar(a.cfg.sfxVol) : "NO", x, y + g * 6, w)) {
        a.cfg.sfxVol = std::clamp(a.cfg.sfxVol + u.optionDir(), 0, 10);
        g_music.setVolumes(a.cfg.musicVol / 10.0f, a.cfg.sfxVol / 10.0f);
        g_music.sfx("moneta");
    }
    if (u.option("Velocita'", SPEEDS[a.cfg.speed].name, x, y, w)) a.cfg.speed = (a.cfg.speed + u.optionDir() + SPEED_COUNT) % SPEED_COUNT;
    if (u.option("Effetto CRT", a.cfg.crt ? "SI" : "NO", x, y + g, w)) a.cfg.crt = !a.cfg.crt;
    if (u.option("Schermo intero", a.cfg.fullscreen ? "SI" : "NO", x, y + g * 2, w)) { a.cfg.fullscreen = !a.cfg.fullscreen; a.applyVideo(); }
    if (u.option("Scala intera", a.cfg.integerScale ? "SI" : "NO", x, y + g * 3, w)) { a.cfg.integerScale = !a.cfg.integerScale; a.applyVideo(); }
    if (u.option("Suggerimenti tasti", a.cfg.hints ? "SI" : "NO", x, y + g * 4, w)) a.cfg.hints = !a.cfg.hints;
    u.text(px + pw / 2, y + g * 7 + 4, "Originale = tempi simili al C64 vero", 2, pal::dim, CENTER, false);
    if (u.button("INDIETRO", px + pw / 2 - 160, py + 480, 320, 56) || u.backPressed()) {
        a.saveConfig();
        a.screen = a.optionsReturn;
        u.resetFocus();
    }
}

static void savesMenu(App& a) {
    Ui& u = a.ui;
    u.text(Ui::W / 2, 60, "SALVATAGGI", 6, pal::gold, CENTER);
    float pw = 860, px = (Ui::W - pw) / 2, py = 160;
    u.panel(px, py, pw, 560);
    if (a.saves.empty()) {
        u.text(Ui::W / 2, py + 80, "Nessuna avventura salvata.", 3, pal::parchment, CENTER);
        u.text(Ui::W / 2, py + 130, "In gioco scegli * 9 * per salvare", 2, pal::dim, CENTER, false);
        u.text(Ui::W / 2, py + 160, "sul nastro virtuale.", 2, pal::dim, CENTER, false);
    }
    // elenco scorrevole: rotellina, PagSu/PagGiu o i pulsanti sotto l'elenco
    const int VIS = 6, n = (int)a.saves.size(), maxS = n > VIS ? n - VIS : 0;
    a.saveScroll -= u.wheel();
    if (u.keyPressed(SDLK_PAGEDOWN)) a.saveScroll += VIS;
    if (u.keyPressed(SDLK_PAGEUP)) a.saveScroll -= VIS;
    a.saveScroll = std::max(0, std::min(maxS, a.saveScroll));
    int shown = 0;
    for (size_t i = a.saveScroll; i < a.saves.size() && shown < VIS; i++, shown++) {
        auto& s = a.saves[i];
        float y = py + 30 + shown * 66;
        std::string label = s.name + "   " + s.when;
        if (a.confirmDelete == (int)i) {
            if (u.button("CONFERMA ELIMINAZIONE", px + 30, y, pw - 260, 54, true, "Il salvataggio verra' cancellato")) {
                SDL_RemovePath(s.path.c_str());
                a.confirmDelete = -1;
                a.refreshSaves();
                return;
            }
            if (u.button("NO", px + pw - 210, y, 180, 54)) a.confirmDelete = -1;
        } else {
            u.fill(px + 30, y, pw - 260, 54, Color{36, 28, 66});
            u.frame(px + 30, y, pw - 260, 54, pal::panelEdge, 2);
            u.text(px + 50, y + 15, label, 2.5f, pal::parchment, LEFT);
            if (u.button("ELIMINA", px + pw - 210, y, 180, 54, true, "Cancella questo salvataggio")) a.confirmDelete = (int)i;
        }
    }
    if (n > VIS) {
        char buf[64];
        std::snprintf(buf, sizeof buf, "%d-%d di %d", a.saveScroll + 1, std::min(n, a.saveScroll + VIS), n);
        u.text(Ui::W / 2, py + 438, buf, 2, pal::dim, CENTER, false);
        if (u.button("PIU' RECENTI", px + 30, py + 424, 250, 40, a.saveScroll > 0)) a.saveScroll = std::max(0, a.saveScroll - VIS);
        if (u.button("PIU' VECCHI", px + pw - 280, py + 424, 250, 40, a.saveScroll < maxS)) a.saveScroll = std::min(maxS, a.saveScroll + VIS);
    }
    if (u.button("APRI CARTELLA", px + 60, py + 476, 340, 56, true, "Apre la cartella dei salvataggi")) {
        std::string url = "file:///" + a.pref + "saves";
        SDL_OpenURL(url.c_str());
    }
    if (u.button("INDIETRO", px + pw - 400, py + 476, 340, 56) || u.backPressed()) { a.screen = Screen_::MAIN; a.confirmDelete = -1; u.resetFocus(); }
}

static void aboutMenu(App& a) {
    Ui& u = a.ui;
    u.text(Ui::W / 2, 60, "INFORMAZIONI", 6, pal::gold, CENTER);
    float pw = 900, px = (Ui::W - pw) / 2, py = 160;
    u.panel(px, py, pw, 560);
    const char* lines[] = {
        "Avventura 1 (Fantasy) per Commodore 64",
        "gioco originale di A. Castellari",
        "",
        "Questo programma e' una ricostruzione del motore:",
        "il P-code del gioco viene eseguito da una nuova",
        "macchina virtuale, senza emulare il C64.",
        "",
        "Nessun dato del gioco e' incluso: serve la tua",
        "copia originale su nastro (.tap). Tutto viene",
        "ricavato in locale dal tuo file.",
        "",
        "Font e grafica del launcher: originali del progetto.",
    };
    // scala del testo calcolata sulla riga piu' lunga, cosi' resta dentro il riquadro
    float widest = 0;
    for (const char* l : lines) widest = std::fmax(widest, u.textWidth(l, 1));
    float sc = std::fmin(2.5f, (pw - 80) / widest);
    float y = py + 36;
    for (const char* l : lines) { u.text(px + 40, y, l, sc, l[0] ? pal::parchment : pal::dim, LEFT, false); y += 12 * sc; }
    if (u.button("INDIETRO", px + pw / 2 - 160, py + 476, 320, 56) || u.backPressed()) { a.screen = Screen_::MAIN; u.resetFocus(); }
}

void drawLauncher(App& a) {
    a.ui.background(a.time);
    switch (a.screen) {
        case Screen_::MAIN: mainMenu(a); break;
        case Screen_::OPTIONS: optionsMenu(a); break;
        case Screen_::SAVES: savesMenu(a); break;
        case Screen_::ABOUT: aboutMenu(a); break;
        default: break;
    }
    footer(a);
}
