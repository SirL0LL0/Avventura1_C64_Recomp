// Pannello laterale dell'Espansione: una "seconda schermata" in stile C64 (18x34 caratteri)
// con l'illustrazione del soggetto corrente (bitmap 128x128), nome, caratteristiche,
// equipaggiamento e le icone dei consumabili, cliccabili.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

class VM;

struct Picture {
    std::string name;
    std::vector<std::string> keys;     // parole che, trovate nel nome, scelgono questa immagine
    std::string title;                 // didascalia
    int w = 0, h = 0;
    std::vector<uint8_t> px;           // colori C64 (0-15), w*h
};

// carica le illustrazioni generate (art_data.h); le icone hanno il nome "icona_..."
void loadPictures(std::vector<Picture>& out);
const Picture* findPicture(const std::vector<Picture>& pics, const std::string& subject);
const Picture* pictureByName(const std::vector<Picture>& pics, const std::string& name);

// consumabili mostrati come icone nella scheda
struct Consumable {
    const Picture* icon = nullptr;
    int count = 0;
    bool usable = false;               // si puo' usare adesso (il gioco aspetta il tasto giusto)
    uint8_t key = 0;                   // tasto PETSCII equivalente (F1, F3)
    const char* keyName = "";
    const char* name = "";
    int px = 0, py = 0;                // posizione nel pannello (pixel)
};

class Panel {
public:
    static const int W = 18, H = 34;
    uint8_t chars[W * H];
    uint8_t colors[W * H];
    int bg = 0;
    Consumable items[2];               // 0 Filtro magico (F1), 1 Pane elfico (F3)

    void clear(int color);
    void text(int x, int y, const std::string& ascii, int color, bool rvs = false);
    void box(int x0, int y0, int x1, int y1, int color);
    void build(VM* vm, const Picture* pic, const std::string& subject);
    void render(uint32_t* dst, int pitch) const;     // W*8 x H*8 pixel

private:
    const Picture* pic_ = nullptr;
};
