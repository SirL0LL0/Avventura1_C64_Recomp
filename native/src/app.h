// Stato condiviso dell'applicazione (launcher + gioco).
#pragma once
#include "choices.h"
#include "screen.h"
#include "tap.h"
#include "ui.h"
#include "vm.h"
#include <SDL3/SDL.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct Config {
    int speed = 0;              // indice in SPEEDS
    bool crt = true;            // linee di scansione
    bool fullscreen = false;
    bool integerScale = false;
    bool hints = true;          // barra dei suggerimenti tasti
    std::string tapPath;        // ultimo .tap fornito (solo informativo)
};

struct SpeedDef { const char* name; double mult; };
extern const SpeedDef SPEEDS[];
extern const int SPEED_COUNT;

struct SaveInfo { std::string path, name, when; int64_t mtime = 0; };

enum class Screen_ { MAIN, OPTIONS, SAVES, ABOUT, GAME };

struct App;

// Host della VM: collega nastro virtuale e fine partita all'app
struct GameHost : VMHost {
    App* app = nullptr;
    void tapeWrite(const std::string& name, const std::string& data) override;
    void tapeReadRequest(const std::string& name) override;
    void lineInputRequest() override;
    void onEnd(const std::string& reason) override;
};

struct App {
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;
    SDL_Texture* c64tex = nullptr;
    Ui ui;
    Config cfg;
    std::string pref;            // cartella dati utente
    bool running = true;
    double time = 0;

    // copia del gioco fornita dall'utente
    bool haveGame = false;
    GameImage game;
    std::string status;          // messaggio di stato nel launcher
    bool statusError = false;

    // file scelto dal dialogo nativo (thread diverso)
    std::mutex pickMx;
    std::string picked;

    Screen_ screen = Screen_::MAIN;
    Screen_ optionsReturn = Screen_::MAIN;

    // partita
    std::unique_ptr<Screen> c64;
    std::unique_ptr<VM> vm;
    GameHost host;
    bool paused = false;
    int pauseFocus = 0;
    bool tapePick = false;       // dialogo di scelta del nastro da caricare
    bool lineInput = false;
    std::string lineBuf;
    std::string endReason;       // fine partita in attesa di conferma
    std::string toast;
    double toastUntil = 0;
    double budgetCarry = 0;

    // scelte cliccabili riconosciute sullo schermo del gioco
    std::vector<Choice> choices;
    int hoverChoice = -1;        // sotto il mouse
    int selChoice = -1;          // selezionata da tastiera (frecce)
    float gmx = -1, gmy = -1;    // mouse in coordinate logiche
    SDL_Cursor* cursorHand = nullptr;
    SDL_Cursor* cursorArrow = nullptr;
    bool handShown = false;

    // salvataggi
    std::vector<SaveInfo> saves;
    int confirmDelete = -1;

    // operazioni
    void loadConfig();
    void saveConfig();
    void tryLoadTap(const std::string& path);
    bool loadCachedGame();
    void refreshSaves();
    void startGame();
    void stopGame();
    void showToast(const std::string& s, double secs = 3.5) { toast = s; toastUntil = time + secs; }
    void applyVideo();
};

// launcher.cpp
void drawLauncher(App& app);
void openTapDialog(App& app);
// main.cpp
std::string asciiFromPetscii(const std::string& s);
