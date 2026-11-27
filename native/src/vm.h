// Macchina virtuale del P-code di Avventura 1: reimplementazione del runtime originale
// ($081F-$1783) che esegue direttamente il programma compilato della copia dell'utente.
// Gli opcode $62-$70 (non usati dall'originale) sono riservati alle estensioni delle mod.
#pragma once
#include "screen.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Value {
    bool isStr = false;
    double n = 0;
    std::string s;          // byte PETSCII
    static Value num(double v) { Value x; x.n = v; return x; }
    static Value str(std::string v) { Value x; x.isStr = true; x.s = std::move(v); return x; }
};

class VMHost {
public:
    virtual ~VMHost() = default;
    // nastro virtuale: scrittura immediata; lettura asincrona (l'host chiama VM::tapeProvide)
    virtual void tapeWrite(const std::string& name, const std::string& data) = 0;
    virtual void tapeReadRequest(const std::string& name) = 0;
    virtual void lineInputRequest() {}
    // registri virtuali di espansione $DE00-$DEFF (POKE/PEEK 56832+reg)
    virtual void ioWrite(int /*reg*/, int /*value*/) {}
    virtual int ioRead(int /*reg*/) { return 0; }
    virtual void onEnd(const std::string& reason) = 0;     // "end" "reset" "error" "stop"
};

// Mod caricata accanto al gioco: codice nostro compilato + agganci alle righe originali
struct ModHook { int at; bool gosub; int target; };
struct ModInfo {
    std::string name;
    uint32_t game = 0;               // impronta FNV-1a del programma originale
    int base = 0;
    std::vector<std::string> vars;   // variabili nuove (dopo quelle originali)
    std::vector<ModHook> hooks;
    std::vector<uint8_t> code;
};
bool parseMod(const std::string& text, ModInfo& m, std::string& err);
uint32_t gameFingerprint(const std::vector<uint8_t>& mem, int end);

class VM {
public:
    enum State { RUN, WAIT, HALT };

    VM(const std::vector<uint8_t>& mem, Screen& scr, VMHost& host);
    State run(double budget);              // esegue fino a `budget` unita' di tempo
    State state() const { return st_; }
    void pushKey(uint8_t petscii) { if (keys_.size() < 10) keys_.push_back(petscii); }
    void tapeProvide(bool ok, const std::string& name, const std::string& data);
    void lineProvide(const std::string& line);
    std::string errorText() const { return err_; }
    // vero se l'ultimo GET non ha trovato tasti: il gioco sta aspettando una scelta
    bool waitingKey() const { return st_ == RUN && waitKey_ && keys_.empty(); }
    int currentPC() const { return pc_; }
    bool loadMod(const ModInfo& m, std::string& err);
    // lettura di variabili per nome (pannello, registri)
    bool varString(const std::string& name, std::string& out) const;
    bool varNumber(const std::string& name, double& out) const;
    bool arrayStrings(const std::string& name, std::vector<std::string>& out) const;   // es. "OS$"
    bool setNumber(const std::string& name, double v);   // solo per i test
    bool setString(const std::string& name, const std::string& v);
    static std::string fmtNumber(double v);
    static double val(const std::string& s);

private:
    struct Ref { int kind = 0, idx = 0, off = 0; };      // 0 nessuno, 1 variabile, 2 elemento di array
    struct Entry { Value v; Ref r; };
    struct Array { std::string name; bool isStr = false, isInt = false; std::vector<int> dims; std::vector<Value> data; };
    struct Frame { int kind = 0; int ret = 0; int var = 0; double to = 0, step = 1; int body = 0; bool hook = false; };   // 1 gosub 2 for 3 init
    struct File { int mode = 0; std::string name, buf; size_t pos = 0; };   // 1 lettura 2 scrittura 3 nullo

    std::vector<uint8_t> mem_;
    Screen& scr_;
    VMHost& host_;
    State st_ = RUN;
    int pc_ = 0, initPC_ = 0, datPtr_ = 0, startPC_ = 0, data_ = 0;
    std::vector<std::string> varNames_;
    std::vector<Value> vars_;
    std::map<int, int> arrayNd_;
    std::map<int, Array> arrays_;
    std::vector<Entry> stack_;
    std::vector<Frame> ctl_;
    std::deque<uint8_t> keys_;
    std::map<int, File> files_;
    File* out_ = nullptr;
    File* in_ = nullptr;
    std::map<int, uint8_t> pokes_;
    std::string err_;
    double cost_ = 0;
    bool waitKey_ = false;
    std::vector<int> hookAt_;          // indirizzo -> indice in hooks_ (-1 nessuno)
    std::vector<ModHook> hooks_;
    int skipHook_ = -1;                // ritorno da un aggancio: non riattivarlo
    int vartabEnd_ = 0;
    void scanCode(int from, int to);
    uint64_t startMs_ = 0;
    std::vector<Ref> pendingInput_;     // INPUT da tastiera in attesa

    uint8_t m(int a) const { return mem_[a & 0xFFFF]; }
    void step();
    [[noreturn]] void fail(const char* code);
    Value pop();
    Entry popEntry();
    double popNum();
    std::string popStr();
    int popByte();
    void push(Value v) { push(std::move(v), Ref()); }
    void push(Value v, Ref r);
    void assign(const Ref& r, const Value& v);
    Value& refValue(const Ref& r);
    Ref arrayRef(int n, bool store, Value* storeVal);
    void binop(int op);
    void func(int op);
    void printValue(const Value& v);
    void outStr(const std::string& s);
    void tabZone();
    void clr();
    int peek(int a);
    void poke(int a, int v);
    void readField(File& f, std::string& out);
    bool isStrVar(int i) const { return !varNames_[i].empty() && varNames_[i].back() == '$'; }
    bool isIntVar(int i) const { return !varNames_[i].empty() && varNames_[i].back() == '%'; }
    void end(const std::string& reason) { st_ = HALT; host_.onEnd(reason); }
};
