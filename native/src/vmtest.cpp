// Test senza finestra della VM: vmtest <gioco.tap> [tasti...]  (tasti: caratteri, SPACE, ENTER, F1)
#include "choices.h"
#include "tap.h"
#include "vm.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

struct Host : VMHost {
    std::string saved, savedName;
    VM* vm = nullptr;
    void tapeWrite(const std::string& n, const std::string& d) override { savedName = n; saved = d; }
    void tapeReadRequest(const std::string&) override { pending = true; }
    void onEnd(const std::string& r) override { std::printf("FINE: %s\n", r.c_str()); }
    void ioWrite(int reg, int v) override { std::printf("IO $DE%02X <- %d\n", reg, v); }
    bool pending = false;
};

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("uso: vmtest gioco.tap [tasti...]\n"); return 1; }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> tap((std::istreambuf_iterator<char>(f)), {});
    GameImage g; std::string err;
    if (!loadGameFromTap(tap, g, err)) { std::printf("ERRORE: %s\n", err.c_str()); return 1; }
    Screen scr; Host host;
    VM vm(g.mem, scr, host);
    int argStart = 2;
    if (argc > 3 && std::string(argv[2]) == "--mod") {
        std::ifstream mf(argv[3]);
        std::string txt((std::istreambuf_iterator<char>(mf)), {});
        ModInfo md;
        if (!parseMod(txt, md, err) || md.game != gameFingerprint(g.mem, g.end) || !vm.loadMod(md, err)) { std::printf("MOD: %s\n", err.empty() ? "impronta del gioco diversa" : err.c_str()); return 1; }
        std::printf("mod caricata: %s (%zu byte, %zu agganci)\n", md.name.c_str(), md.code.size(), md.hooks.size());
        argStart = 4;
    }
    host.vm = &vm;
    auto runFor = [&](double u) {
        for (double left = u; left > 0 && vm.state() != VM::HALT; left -= 500) {
            if (vm.state() == VM::WAIT && host.pending) { host.pending = false; vm.tapeProvide(!host.saved.empty(), host.savedName, host.saved); }
            vm.run(500);
        }
    };
    runFor(20000);
    for (int i = argStart; i < argc; i++) {
        std::string k = argv[i];
        if (k.rfind("--set", 0) == 0 && i + 1 < argc) { std::string kv = argv[++i]; auto eq = kv.find('='); vm.setNumber(kv.substr(0, eq), std::atof(kv.c_str() + eq + 1)); continue; }
        uint8_t c = k == "SPACE" ? 32 : k == "ENTER" ? 13 : k == "F1" ? 133 : (uint8_t)k[0];
        vm.pushKey(c);
        runFor(20000);
    }
    for (int y = 0; y < 25; y++) {
        std::string s = "|";
        for (int x = 0; x < 40; x++) {
            int c = scr.chars[y * 40 + x] & 0x7F;
            if (c >= 1 && c <= 26) s += (char)(96 + c);
            else if (c >= 0x41 && c <= 0x5A) s += (char)c;
            else if (c >= 0x20 && c < 0x40) s += (char)c;
            else s += '#';
        }
        std::printf("%s|\n", s.c_str());
    }
    std::printf("err=%s pc=%04X attesa_tasto=%d\n", vm.errorText().c_str(), vm.currentPC(), (int)vm.waitingKey());
    for (const Choice& c : detectChoices(scr))
        std::printf("scelta tasto=$%02X %s celle (%d,%d)-(%d,%d)\n", c.key, c.pad ? "[tastierino]" : "", c.x0, c.y0, c.x1, c.y1);
    return 0;
}
