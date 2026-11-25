// Macchina virtuale del P-code di Avventura 1: reimplementazione del runtime originale
// ($081F-$1783) che esegue direttamente il programma compilato della copia dell'utente.
// Gli opcode $62-$70 (non usati dall'originale) sono riservati alle estensioni delle mod.
#pragma once
#include "screen.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
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
    virtual void onEnd(const std::string& reason) = 0;     // "end" "reset" "error" "stop"
};

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
    int currentPC() const { return pc_; }
    static std::string fmtNumber(double v);
    static double val(const std::string& s);

private:
    struct Ref { int kind = 0, idx = 0, off = 0; };      // 0 nessuno, 1 variabile, 2 elemento di array
    struct Entry { Value v; Ref r; };
    struct Array { std::string name; bool isStr = false, isInt = false; std::vector<int> dims; std::vector<Value> data; };
    struct Frame { int kind = 0; int ret = 0; int var = 0; double to = 0, step = 1; int body = 0; };   // 1 gosub 2 for 3 init
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
