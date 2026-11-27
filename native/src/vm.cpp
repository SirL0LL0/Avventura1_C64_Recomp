#include "vm.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <stdexcept>

namespace {

struct VMError { const char* code; };

uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::string nameOf(uint8_t b0, uint8_t b1) {
    std::string s;
    s += (char)(b0 & 0x7F);
    if (b1 & 0x7F) s += (char)(b1 & 0x7F);
    if ((b0 & 0x80) && (b1 & 0x80)) s += '%';
    else if (b1 & 0x80) s += '$';
    return s;
}

double cbmFloat(const uint8_t* f) {
    if (f[0] == 0) return 0;
    double mant = ((double)((f[1] | 0x80)) * 16777216.0 + f[2] * 65536.0 + f[3] * 256.0 + f[4]) / 4294967296.0;
    double v = std::ldexp(mant, f[0] - 128);
    return (f[1] & 0x80) ? -v : v;
}

// AVV_SEED=n rende ripetibili le partite (test)
std::mt19937& rng() {
    static std::mt19937 r{std::getenv("AVV_SEED") ? (unsigned)std::atoi(std::getenv("AVV_SEED")) : std::random_device{}()};
    return r;
}

} // namespace

// ------------------------------------------------------------------ formato numeri (FOUT)
std::string VM::fmtNumber(double v) {
    if (!std::isfinite(v)) throw VMError{"OVERFLOW"};
    if (v == 0) return " 0";
    double a = std::fabs(v);
    char buf[64];
    std::string s;
    if (a >= 1e9 || a < 0.01) {
        std::snprintf(buf, sizeof buf, "%.8e", a);
        std::string t = buf;
        size_t e = t.find('e');
        std::string mant = t.substr(0, e), ex = t.substr(e + 1);
        while (mant.size() > 1 && mant.back() == '0') mant.pop_back();
        if (mant.back() == '.') mant.pop_back();
        int x = std::atoi(ex.c_str());
        std::snprintf(buf, sizeof buf, "%sE%c%02d", mant.c_str(), x < 0 ? '-' : '+', std::abs(x));
        s = buf;
    } else {
        std::snprintf(buf, sizeof buf, "%.9g", a);
        s = buf;
        if (s.size() > 1 && s[0] == '0' && s[1] == '.') s = s.substr(1);
    }
    return (v < 0 ? "-" : " ") + s;
}

double VM::val(const std::string& in) {
    std::string s;
    for (char c : in) if (c != ' ') s += c;
    size_t i = 0;
    std::string t;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) t += s[i++];
    bool dig = false, dot = false;
    while (i < s.size() && ((s[i] >= '0' && s[i] <= '9') || (s[i] == '.' && !dot))) { if (s[i] == '.') dot = true; else dig = true; t += s[i++]; }
    if (!dig) return 0;
    if (i < s.size() && (s[i] == 'E' || s[i] == 'e')) {
        size_t j = i + 1; std::string ex = "e";
        if (j < s.size() && (s[j] == '+' || s[j] == '-')) ex += s[j++];
        bool ed = false;
        while (j < s.size() && s[j] >= '0' && s[j] <= '9') { ex += s[j++]; ed = true; }
        if (ed) t += ex;
    }
    return std::strtod(t.c_str(), nullptr);
}

// ------------------------------------------------------------------ setup
VM::VM(const std::vector<uint8_t>& mem, Screen& scr, VMHost& host) : mem_(mem), scr_(scr), host_(host) {
    auto w = [&](int a) { return m(a) | m(a + 1) << 8; };
    int vartab = w(0x1784), arytab = w(0x1786);
    initPC_ = w(0x178A); datPtr_ = w(0x178C); startPC_ = w(0x178E);
    for (int a = vartab; a + 7 <= arytab; a += 7) varNames_.push_back(nameOf(m(a), m(a + 1)));
    vars_.resize(varNames_.size());
    // pre-scansione dei DIM per conoscere il numero di indici di ogni array
    scanCode(initPC_, datPtr_);
    scanCode(startPC_, vartab - 1);
    vartabEnd_ = arytab;
    pc_ = startPC_;
    startMs_ = nowMs();
}

// ------------------------------------------------------------------ mod
uint32_t gameFingerprint(const std::vector<uint8_t>& mem, int end) {
    uint32_t h = 0x811C9DC5u;
    for (int a = 0x0801; a < end; a++) { h ^= mem[a]; h *= 0x01000193u; }
    return h;
}

bool parseMod(const std::string& text, ModInfo& m, std::string& err) {
    std::istringstream is(text);
    std::string line;
    bool header = false;
    while (std::getline(is, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t sp = line.find(' ');
        std::string k = line.substr(0, sp), v = sp == std::string::npos ? "" : line.substr(sp + 1);
        if (k == "AVVMOD") { header = v == "1"; continue; }
        if (k == "name") m.name = v;
        else if (k == "game") m.game = (uint32_t)std::strtoul(v.c_str(), nullptr, 16);
        else if (k == "base") m.base = std::atoi(v.c_str());
        else if (k == "var") m.vars.push_back(v);
        else if (k == "hook") {
            std::istringstream hs(v);
            ModHook h; std::string kind;
            hs >> h.at >> kind >> h.target;
            h.gosub = kind == "gosub";
            m.hooks.push_back(h);
        } else if (k == "code") {
            for (size_t i = 0; i + 1 < v.size(); i += 2) m.code.push_back((uint8_t)std::strtoul(v.substr(i, 2).c_str(), nullptr, 16));
        }
    }
    if (!header) { err = "File di mod non valido"; return false; }
    if (m.base < 0x0801 || m.base + (int)m.code.size() > 0xFFF0) { err = "Mod fuori dalla memoria disponibile"; return false; }
    return true;
}

bool VM::loadMod(const ModInfo& md, std::string& err) {
    if (md.base < vartabEnd_) { err = "La mod si sovrappone al gioco"; return false; }
    std::memcpy(&mem_[md.base], md.code.data(), md.code.size());
    for (auto& v : md.vars) { varNames_.push_back(v); Value z; z.isStr = !v.empty() && v.back() == '$'; vars_.push_back(z); }
    hooks_ = md.hooks;
    hookAt_.assign(0x10000, -1);
    for (size_t i = 0; i < hooks_.size(); i++) hookAt_[hooks_[i].at & 0xFFFF] = (int)i;
    scanCode(md.base, md.base + (int)md.code.size());
    return true;
}

bool VM::varString(const std::string& name, std::string& out) const {
    for (size_t i = 0; i < varNames_.size(); i++) if (varNames_[i] == name) { out = vars_[i].s; return true; }
    return false;
}
bool VM::varNumber(const std::string& name, double& out) const {
    for (size_t i = 0; i < varNames_.size(); i++) if (varNames_[i] == name) { out = vars_[i].n; return true; }
    return false;
}

bool VM::arrayStrings(const std::string& name, std::vector<std::string>& out) const {
    for (auto& kv : arrays_)
        if (kv.second.name == name && kv.second.isStr) {
            out.clear();
            for (auto& v : kv.second.data) out.push_back(v.s);
            return true;
        }
    return false;
}

bool VM::setString(const std::string& name, const std::string& v) {
    for (size_t i = 0; i < varNames_.size(); i++) if (varNames_[i] == name) { vars_[i].s = v; return true; }
    return false;
}

bool VM::setNumber(const std::string& name, double v) {
    for (size_t i = 0; i < varNames_.size(); i++) if (varNames_[i] == name) { vars_[i].n = v; return true; }
    return false;
}

void VM::scanCode(int from, int to) {
    for (int a = from; a < to;) {
        uint8_t b = m(a);
        int len = 1;
        if (b >= 0x80) {
            int lo = b & 0x3F;
            if (b == 0xE7) len = 2 + m(a + 1);
            else if (b >= 0xE8 && b <= 0xEF) len = 1 + (b & 7);
            else if (b >= 0xF0 || b == 0xE6) len = 1;
            else if (lo < 0x20) len = 1;
            else if (lo <= 0x22) len = 2;
            else if (lo == 0x23) len = 3;
            else if (lo == 0x24) len = 2;
            else if (lo == 0x25) len = 3;
            else if (lo == 0x26) len = 2;
            else if (lo == 0x27) len = 3;
            else if (lo == 0x28 || lo == 0x29) len = 6;
        } else switch (b) {
            case 0x10: { int slot = m(a + 4) << 8 | m(a + 5); if (slot) arrayNd_[(slot - 1) / 2] = m(a + 1); len = 6; break; }
            case 0x18: len = 2; break;
            case 0x19: case 0x1A: case 0x38: len = 3; break;
            case 0x1B: case 0x1C: len = m(a + 1); break;
            case 0x1F: case 0x5D: case 0x5E: case 0x5F: case 0x60: len = 2; break;
            case 0x37: len = m(a + 5); break;
            case 0x00: return;
        }
        a += len < 1 ? 1 : len;
    }
}

void VM::fail(const char* code) { throw VMError{code}; }

void VM::push(Value v, Ref r) { stack_.push_back({std::move(v), r}); }
VM::Entry VM::popEntry() {
    if (stack_.empty()) fail("SYNTAX");
    Entry e = std::move(stack_.back()); stack_.pop_back(); return e;
}
Value VM::pop() { return popEntry().v; }
double VM::popNum() { Value v = pop(); if (v.isStr) fail("TYPE MISMATCH"); return v.n; }
std::string VM::popStr() { Value v = pop(); if (!v.isStr) fail("TYPE MISMATCH"); return v.s; }
int VM::popByte() { double d = std::floor(popNum()); if (d < 0 || d > 255) fail("ILLEGAL QUANTITY"); return (int)d; }

Value& VM::refValue(const Ref& r) {
    if (r.kind == 1) return vars_[r.idx];
    return arrays_.at(r.idx).data[r.off];
}

void VM::assign(const Ref& r, const Value& v) {
    if (r.kind == 0) fail("SYNTAX");
    bool wantStr, isInt;
    if (r.kind == 1) { wantStr = isStrVar(r.idx); isInt = isIntVar(r.idx); }
    else { auto& a = arrays_.at(r.idx); wantStr = a.isStr; isInt = a.isInt; }
    if (v.isStr != wantStr) fail("TYPE MISMATCH");
    Value& dst = refValue(r);
    dst = v;
    if (!wantStr && isInt) {
        dst.n = std::floor(dst.n);
        if (dst.n < -32768 || dst.n > 32767) fail("ILLEGAL QUANTITY");
    }
    if (!wantStr && r.kind == 1) dst.isStr = false;
}

VM::Ref VM::arrayRef(int n, bool store, Value* storeVal) {
    Value sv;
    if (store) sv = pop();
    auto ndIt = arrayNd_.find(n);
    int nd = ndIt == arrayNd_.end() ? 1 : ndIt->second;
    std::vector<int> idx(nd);
    for (int k = nd - 1; k >= 0; k--) {
        double d = std::floor(popNum());
        if (d < 0 || d > 32767) fail("BAD SUBSCRIPT");
        idx[k] = (int)d;
    }
    auto it = arrays_.find(n);
    if (it == arrays_.end()) fail("BAD SUBSCRIPT");
    Array& a = it->second;
    if ((int)a.dims.size() != nd) fail("BAD SUBSCRIPT");
    int off = 0;
    for (int k = nd - 1; k >= 0; k--) {
        if (idx[k] > a.dims[k]) fail("BAD SUBSCRIPT");
        off = off * (a.dims[k] + 1) + idx[k];
    }
    Ref r{2, n, off};
    if (store && storeVal) *storeVal = sv;
    return r;
}

void VM::clr() {
    for (size_t i = 0; i < vars_.size(); i++) { vars_[i] = Value(); vars_[i].isStr = isStrVar((int)i); }
    arrays_.clear();
    stack_.clear();
    ctl_.clear();
    data_ = datPtr_;
    files_.clear(); out_ = in_ = nullptr;
}

// ------------------------------------------------------------------ memoria simulata
int VM::peek(int a) {
    if (a == 53280) return scr_.border | 0xF0;
    if (a == 53281) return scr_.bg | 0xF0;
    if (a == 646) return scr_.color;
    if (a == 197) return keys_.empty() ? 64 : 0;
    if (a == 198) return (int)keys_.size();
    if (a >= 56832 && a < 57088) return host_.ioRead(a - 56832) & 0xFF;
    if (a >= 1024 && a < 2024) return scr_.chars[a - 1024];
    if (a >= 55296 && a < 56296) return scr_.colors[a - 55296] | 0xF0;
    auto it = pokes_.find(a);
    return it == pokes_.end() ? 0 : it->second;
}
void VM::poke(int a, int v) {
    if (a < 0 || a > 65535) fail("ILLEGAL QUANTITY");
    if (a == 53280) { scr_.border = v & 15; return; }
    if (a == 53281) { scr_.bg = v & 15; return; }
    if (a == 646) { scr_.color = v & 15; return; }
    if (a == 198) { if (v == 0) keys_.clear(); return; }
    if (a >= 56832 && a < 57088) { host_.ioWrite(a - 56832, v); return; }   // registri di espansione $DE00
    if (a >= 1024 && a < 2024) { scr_.chars[a - 1024] = (uint8_t)v; return; }
    if (a >= 55296 && a < 56296) { scr_.colors[a - 55296] = v & 15; return; }
    pokes_[a] = (uint8_t)v;
}

// ------------------------------------------------------------------ output
void VM::outStr(const std::string& s) {
    if (out_) { out_->buf += s; return; }
    for (unsigned char c : s) scr_.out(c);
    cost_ += s.size() * 0.15;
}
void VM::printValue(const Value& v) {
    if (v.isStr) outStr(v.s);
    else { outStr(fmtNumber(v.n)); outStr(out_ ? " " : "\x1D"); }
}
void VM::tabZone() {
    if (out_) { outStr("\x1D"); return; }
    int n = 10 - (scr_.col() % 40) % 10;
    for (int i = 0; i < n; i++) scr_.out(0x1D);
}

void VM::readField(File& f, std::string& out) {
    const std::string& s = f.buf;
    size_t i = f.pos;
    while (i < s.size() && s[i] == ' ') i++;
    out.clear();
    if (i < s.size() && s[i] == '"') {
        i++;
        while (i < s.size() && s[i] != '"') out += s[i++];
        if (i < s.size()) i++;
        while (i < s.size() && s[i] != '\r' && s[i] != ',') i++;
    } else {
        while (i < s.size() && s[i] != '\r' && s[i] != ',') out += s[i++];
    }
    if (i < s.size()) i++;
    f.pos = i;
}

// ------------------------------------------------------------------ operatori e funzioni
void VM::binop(int op) {
    Value b = pop(), a = pop();
    if (a.isStr != b.isStr) fail("TYPE MISMATCH");
    auto T = [](bool c) { return Value::num(c ? -1 : 0); };
    if (a.isStr) {
        switch (op) {
            case 1: push(T(a.s > b.s)); return;
            case 2: push(T(a.s == b.s)); return;
            case 3: push(T(a.s >= b.s)); return;
            case 4: push(T(a.s < b.s)); return;
            case 5: push(T(a.s != b.s)); return;
            case 6: push(T(a.s <= b.s)); return;
            case 7: if (a.s.size() + b.s.size() > 255) fail("STRING TOO LONG"); push(Value::str(a.s + b.s)); return;
        }
        fail("TYPE MISMATCH");
    }
    double x = a.n, y = b.n;
    auto i16 = [&](double v) { v = std::floor(v); if (v < -32768 || v > 32767) fail("ILLEGAL QUANTITY"); return (int)v; };
    switch (op) {
        case 1: push(T(x > y)); return;
        case 2: push(T(x == y)); return;
        case 3: push(T(x >= y)); return;
        case 4: push(T(x < y)); return;
        case 5: push(T(x != y)); return;
        case 6: push(T(x <= y)); return;
        case 7: push(Value::num(x + y)); return;
        case 8: push(Value::num(x - y)); return;
        case 9: push(Value::num(x * y)); return;
        case 10: if (y == 0) fail("DIVISION BY ZERO"); push(Value::num(x / y)); return;
        case 11: push(Value::num(std::pow(x, y))); return;
        case 12: push(Value::num(i16(x) & i16(y))); return;
        case 13: push(Value::num(i16(x) | i16(y))); return;
    }
}

void VM::func(int op) {
    switch (op) {
        case 0x20: { double v = popNum(); push(Value::num(v > 0 ? 1 : v < 0 ? -1 : 0)); return; }
        case 0x21: push(Value::num(std::floor(popNum()))); return;
        case 0x22: push(Value::num(std::fabs(popNum()))); return;
        case 0x23: fail("ILLEGAL QUANTITY");
        case 0x24: pop(); push(Value::num(20000)); return;
        case 0x25: pop(); push(Value::num(scr_.col() % 40)); return;
        case 0x26: { double v = popNum(); if (v < 0) fail("ILLEGAL QUANTITY"); push(Value::num(std::sqrt(v))); return; }
        case 0x27: { popNum(); push(Value::num(std::uniform_real_distribution<double>(0.0, 1.0)(rng()))); return; }
        case 0x28: { double v = popNum(); if (v <= 0) fail("ILLEGAL QUANTITY"); push(Value::num(std::log(v))); return; }
        case 0x29: push(Value::num(std::exp(popNum()))); return;
        case 0x2A: push(Value::num(std::cos(popNum()))); return;
        case 0x2B: push(Value::num(std::sin(popNum()))); return;
        case 0x2C: push(Value::num(std::tan(popNum()))); return;
        case 0x2D: push(Value::num(std::atan(popNum()))); return;
        case 0x2E: push(Value::num(peek((int)std::floor(popNum())))); return;
        case 0x2F: push(Value::num((double)popStr().size())); return;
        case 0x30: push(Value::str(fmtNumber(popNum()))); return;
        case 0x31: push(Value::num(val(popStr()))); return;
        case 0x32: { std::string s = popStr(); if (s.empty()) fail("ILLEGAL QUANTITY"); push(Value::num((uint8_t)s[0])); return; }
        case 0x33: push(Value::str(std::string(1, (char)popByte()))); return;
        case 0x34: { int k = popByte(); std::string s = popStr(); push(Value::str(s.substr(0, k))); return; }
        case 0x35: { int k = popByte(); std::string s = popStr(); push(Value::str(k >= (int)s.size() ? s : s.substr(s.size() - k))); return; }
        case 0x36: {
            int k = popByte(), st = popByte(); std::string s = popStr();
            if (st < 1) fail("ILLEGAL QUANTITY");
            push(Value::str(st - 1 < (int)s.size() ? s.substr(st - 1, k) : std::string()));
            return;
        }
    }
    fail("SYNTAX");
}

// ------------------------------------------------------------------ esecuzione
VM::State VM::run(double budget) {
    if (st_ != RUN) return st_;
    cost_ = 0;
    try {
        while (cost_ < budget && st_ == RUN) step();
    } catch (const VMError& e) {
        err_ = e.code;
        scr_.cr();
        std::string msg = std::string("?") + e.code + "  ERROR";
        for (char c : msg) scr_.out((uint8_t)c);
        scr_.cr();
        st_ = HALT;
        host_.onEnd("error");
    } catch (const std::out_of_range&) {
        err_ = "BAD SUBSCRIPT";
        st_ = HALT;
        host_.onEnd("error");
    }
    return st_;
}

void VM::tapeProvide(bool ok, const std::string& name, const std::string& data) {
    if (st_ != WAIT) return;
    for (auto& kv : files_) if (kv.second.mode == 1 && kv.second.buf.empty() && kv.second.pos == 0) { kv.second.buf = data; kv.second.name = name; }
    if (!ok) { err_ = "FILE NOT FOUND"; st_ = HALT; scr_.cr(); for (char c : std::string("?FILE NOT FOUND  ERROR")) scr_.out((uint8_t)c); scr_.cr(); host_.onEnd("error"); return; }
    st_ = RUN;
}

void VM::lineProvide(const std::string& line) {
    if (st_ != WAIT) return;
    std::vector<std::string> parts;
    std::string cur;
    for (char c : line) { if (c == ',') { parts.push_back(cur); cur.clear(); } else cur += c; }
    parts.push_back(cur);
    for (size_t k = 0; k < pendingInput_.size(); k++) {
        const Ref& r = pendingInput_[k];
        std::string t = k < parts.size() ? parts[k] : "";
        bool s = r.kind == 1 ? isStrVar(r.idx) : arrays_.at(r.idx).isStr;
        assign(r, s ? Value::str(t) : Value::num(val(t)));
    }
    pendingInput_.clear();
    st_ = RUN;
}

void VM::step() {
    // agganci della mod alle righe originali
    if (!hookAt_.empty()) {
        int h = hookAt_[pc_ & 0xFFFF];
        if (h >= 0) {
            if (pc_ == skipHook_) skipHook_ = -1;
            else {
                const ModHook& hk = hooks_[h];
                if (hk.gosub) {
                    if (ctl_.size() > 200) fail("OUT OF MEMORY");
                    Frame f; f.kind = 1; f.ret = pc_; f.hook = true; ctl_.push_back(f);
                }
                pc_ = hk.target;
                cost_ += 0.25;
                return;
            }
        }
    }
    const int a = pc_;
    const uint8_t b = m(a);
    cost_ += 0.25;
    // ------------------------------------------------ operandi
    if (b >= 0x80) {
        const int lo = b & 0x3F;
        const bool store = b & 0x40;
        if (b == 0xE7) { int l = m(a + 1); push(Value::str(std::string((const char*)&mem_[a + 2], l))); pc_ += 2 + l; return; }
        if (b >= 0xE8 && b <= 0xEF) { int l = b & 7; push(Value::str(std::string((const char*)&mem_[a + 1], l))); pc_ += 1 + l; return; }
        if (b >= 0xF0) { push(Value::num(16 + (b & 15))); pc_++; return; }
        if (b == 0xE6) fail("SYNTAX");
        if (lo < 0x24) {
            int idx, len;
            if (lo < 0x20) { idx = lo; len = 1; }
            else if (lo <= 0x22) { idx = (lo & 3) * 256 + m(a + 1); len = 2; }
            else { idx = m(a + 1) * 256 + m(a + 2); len = 3; }
            if (idx >= (int)vars_.size()) fail("SYNTAX");
            Ref r{1, idx, 0};
            if (store) assign(r, pop());
            else { Value v = vars_[idx]; v.isStr = isStrVar(idx); push(v, r); }
            pc_ += len;
            return;
        }
        if (lo == 0x24 || lo == 0x25) {
            int n = lo == 0x24 ? m(a + 1) : m(a + 1) * 256 + m(a + 2);
            Value sv;
            Ref r = arrayRef(n, store, &sv);
            if (store) assign(r, sv);
            else push(refValue(r), r);
            pc_ += lo == 0x24 ? 2 : 3;
            return;
        }
        switch (lo) {
            case 0x26: push(Value::num(m(a + 1))); pc_ += 2; return;
            case 0x27: push(Value::num(m(a + 1) * 256 + m(a + 2))); pc_ += 3; return;
            case 0x28: case 0x29: push(Value::num(cbmFloat(&mem_[a + 1]))); pc_ += 6; return;
            case 0x2A: push(Value::num(3.14159265358979)); pc_++; return;
            case 0x2B: push(Value::num(0)); pc_++; return;
            case 0x2C: push(Value::num((double)((nowMs() - startMs_) * 60 / 1000))); pc_++; return;
            case 0x2E: case 0x2F: {
                uint64_t t = (nowMs() - startMs_) / 1000;
                char buf[16]; std::snprintf(buf, sizeof buf, "%02d%02d%02d", (int)(t / 3600 % 24), (int)(t / 60 % 60), (int)(t % 60));
                push(Value::str(buf)); pc_++; return;
            }
        }
        if (lo >= 0x30) { push(Value::num(lo & 15)); pc_++; return; }
        fail("SYNTAX");
    }
    // ------------------------------------------------ operatori
    if (b >= 1 && b <= 13) { binop(b); pc_++; return; }
    if (b >= 0x20 && b <= 0x36) { func(b); pc_++; return; }
    switch (b) {
        case 0x00: end("end"); return;
        case 0x0E: { double v = popNum(); push(Value::num(-v)); pc_++; return; }
        case 0x0F: { double v = std::floor(popNum()); if (v < -32768 || v > 32767) fail("ILLEGAL QUANTITY"); push(Value::num(~(int)v)); pc_++; return; }
        case 0x10: {
            int nd = m(a + 1), slot = m(a + 4) << 8 | m(a + 5);
            std::vector<int> dims(nd);
            for (int k = nd - 1; k >= 0; k--) { double d = std::floor(popNum()); if (d < 0 || d > 32767) fail("ILLEGAL QUANTITY"); dims[k] = (int)d; }
            if (slot) {
                int n = (slot - 1) / 2;
                if (arrays_.count(n)) fail("REDIM'D ARRAY");
                Array arr;
                arr.name = nameOf(m(a + 2), m(a + 3));
                arr.isStr = arr.name.back() == '$';
                arr.isInt = arr.name.back() == '%';
                arr.dims = dims;
                size_t size = 1;
                for (int d : dims) size *= (size_t)(d + 1);
                Value z; z.isStr = arr.isStr;
                arr.data.assign(size, z);
                arrays_[n] = std::move(arr);
            }
            pc_ += 6;
            return;
        }
        case 0x11: case 0x12: {
            Entry e = popEntry();
            double stepv = b == 0x12 ? popNum() : 1.0;
            double to = popNum();
            if (e.r.kind != 1) fail("SYNTAX");
            for (int i = (int)ctl_.size() - 1; i >= 0; i--) {
                if (ctl_[i].kind != 2) break;
                if (ctl_[i].var == e.r.idx) { ctl_.resize(i); break; }
            }
            if (ctl_.size() > 200) fail("OUT OF MEMORY");
            Frame f; f.kind = 2; f.var = e.r.idx; f.to = to; f.step = stepv; f.body = a + 1;
            ctl_.push_back(f);
            pc_++;
            return;
        }
        case 0x13: case 0x14: {
            int var = -1;
            if (b == 0x14) { Entry e = popEntry(); if (e.r.kind != 1) fail("SYNTAX"); var = e.r.idx; }
            int i = (int)ctl_.size() - 1;
            for (; i >= 0; i--) {
                if (ctl_[i].kind != 2) { i = -1; break; }
                if (var < 0 || ctl_[i].var == var) break;
            }
            if (i < 0) fail("NEXT WITHOUT FOR");
            ctl_.resize(i + 1);
            Frame& f = ctl_[i];
            double v = vars_[f.var].n + f.step;
            vars_[f.var].n = isIntVar(f.var) ? std::floor(v) : v;
            cost_ += 0.75;
            if (f.step >= 0 ? v <= f.to : v >= f.to) { pc_ = f.body; return; }
            ctl_.resize(i);
            pc_++;
            return;
        }
        case 0x15: {
            clr();
            Frame f; f.kind = 3; f.ret = a + 1;
            ctl_.push_back(f);
            pc_ = initPC_;
            return;
        }
        case 0x16: {
            if (ctl_.empty() || ctl_.back().kind != 3) fail("SYNTAX");
            pc_ = ctl_.back().ret; ctl_.pop_back();
            return;
        }
        case 0x17: { int addr = (int)std::floor(popNum()); int v = popByte(); poke(addr, v); pc_++; return; }
        case 0x18: {
            int addr = (int)std::floor(popNum());
            if (addr == 64738) { end("reset"); return; }
            pc_ += 2;
            return;
        }
        case 0x19: pc_ = m(a + 1) << 8 | m(a + 2); return;
        case 0x1A: {
            if (ctl_.size() > 200) fail("OUT OF MEMORY");
            Frame f; f.kind = 1; f.ret = a + 3; ctl_.push_back(f);
            pc_ = m(a + 1) << 8 | m(a + 2);
            return;
        }
        case 0x1B: case 0x1C: {
            int len = m(a + 1), n = (len - 2) / 2;
            double kd = std::floor(popNum());
            if (kd < 0 || kd > 255) fail("ILLEGAL QUANTITY");
            int k = (int)kd;
            if (k >= 1 && k <= n) {
                if (b == 0x1C) { Frame f; f.kind = 1; f.ret = a + len; ctl_.push_back(f); }
                pc_ = m(a + 2 * k) << 8 | m(a + 2 * k + 1);
            } else pc_ += len;
            return;
        }
        case 0x1D: {
            while (!ctl_.empty() && ctl_.back().kind != 1) ctl_.pop_back();
            if (ctl_.empty()) fail("RETURN WITHOUT GOSUB");
            pc_ = ctl_.back().ret;
            if (ctl_.back().hook) skipHook_ = pc_;      // torna alla riga agganciata senza riattivare l'aggancio
            ctl_.pop_back();
            return;
        }
        case 0x1E: data_ = datPtr_; pc_++; return;
        case 0x1F: {
            Value c = pop();
            bool t = c.isStr ? !c.s.empty() : c.n != 0;
            pc_ += t ? 2 : m(a + 1);
            return;
        }
        case 0x3A: pc_++; return;
        case 0x3B: tabZone(); pc_++; return;
        case 0x3C: printValue(pop()); pc_++; return;
        case 0x3D: printValue(pop()); tabZone(); pc_++; return;
        case 0x3E: printValue(pop()); outStr("\r"); pc_++; return;
        case 0x3F: outStr("\r"); pc_++; return;
        case 0x40: case 0x41: {
            int k = popByte();
            if (out_) { if (b == 0x40) outStr(std::string(k, ' ')); }
            else { int n = b == 0x40 ? k : k - scr_.col(); for (int i = 0; i < n; i++) scr_.out(0x1D); }
            pc_++;
            return;
        }
        case 0x42: case 0x44: {
            int n = popByte();
            auto it = files_.find(n);
            if (it == files_.end() || it->second.mode == 1) fail("FILE NOT OPEN");
            out_ = &it->second;
            if (b == 0x44) { outStr("\r"); out_ = nullptr; }
            pc_++;
            return;
        }
        case 0x43: printValue(pop()); outStr("\r"); out_ = nullptr; pc_++; return;
        case 0x45: out_ = nullptr; pc_++; return;
        case 0x46: case 0x47: {
            Entry e = popEntry();
            std::string ch;
            if (in_) { if (in_->pos < in_->buf.size()) ch = in_->buf.substr(in_->pos++, 1); }
            else if (!keys_.empty()) { ch = std::string(1, (char)keys_.front()); keys_.pop_front(); }
            else cost_ += 1.5;                   // attesa tasto
            if (!in_) waitKey_ = ch.empty();
            bool isS = e.r.kind == 1 ? isStrVar(e.r.idx) : arrays_.at(e.r.idx).isStr;
            if (isS) assign(e.r, Value::str(ch));
            else assign(e.r, Value::num(!ch.empty() && ch[0] >= '0' && ch[0] <= '9' ? ch[0] - '0' : 0));
            if (b == 0x47) in_ = nullptr;
            pc_++;
            return;
        }
        case 0x48: {
            int n = popByte();
            auto it = files_.find(n);
            if (it == files_.end() || it->second.mode != 1) fail("FILE NOT OPEN");
            in_ = &it->second;
            pc_++;
            return;
        }
        case 0x49: end("stop"); return;
        case 0x4A: case 0x4B: {
            Entry e = popEntry();
            if (m(data_) >= 0x80) fail("OUT OF DATA");
            int l = m(data_);
            std::string d((const char*)&mem_[data_ + 1], l);
            data_ += 1 + l;
            bool isS = e.r.kind == 1 ? isStrVar(e.r.idx) : arrays_.at(e.r.idx).isStr;
            assign(e.r, isS ? Value::str(d) : Value::num(val(d)));
            pc_++;
            return;
        }
        case 0x4C: pop(); pop(); pc_++; return;
        case 0x4D: pop(); pop(); pop(); pc_++; return;
        case 0x4E: case 0x4F: end("end"); return;
        case 0x50: case 0x59: case 0x5A: case 0x5B: case 0x5C: pc_++; return;
        case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58: {
            Entry e = popEntry();
            bool last = b < 0x55;
            if (in_) {
                std::string f;
                readField(*in_, f);
                bool isS = e.r.kind == 1 ? isStrVar(e.r.idx) : arrays_.at(e.r.idx).isStr;
                assign(e.r, isS ? Value::str(f) : Value::num(val(f)));
                if (last) in_ = nullptr;
                pc_++;
                return;
            }
            pendingInput_.push_back(e.r);
            pc_++;
            if (last) { outStr("? "); st_ = WAIT; host_.lineInputRequest(); }
            return;
        }
        case 0x5D: case 0x5E: case 0x5F: { int np = m(a + 1); for (int i = 0; i < np; i++) pop(); pc_ += 2; return; }
        case 0x60: {
            int np = m(a + 1);
            std::vector<Value> args;
            for (int i = 0; i < np; i++) args.push_back(pop());
            if (args.empty() || args[0].isStr) fail("SYNTAX");
            int n = (int)args[0].n;
            if (files_.count(n)) fail("FILE OPEN");
            int dev = np > 1 ? (int)args[1].n : 1, sa = np > 2 ? (int)args[2].n : 0;
            std::string name = np > 3 && args[3].isStr ? args[3].s : "";
            File f; f.name = name;
            pc_ += 2;
            if (dev != 1 && dev != 8) { f.mode = 3; files_[n] = f; return; }
            if (sa == 0) { f.mode = 1; files_[n] = f; st_ = WAIT; host_.tapeReadRequest(name); }
            else { f.mode = 2; files_[n] = f; }
            return;
        }
        case 0x61: {
            int n = popByte();
            auto it = files_.find(n);
            pc_++;
            if (it == files_.end()) return;
            if (out_ == &it->second) out_ = nullptr;
            if (in_ == &it->second) in_ = nullptr;
            if (it->second.mode == 2) host_.tapeWrite(it->second.name, it->second.buf);
            files_.erase(it);
            return;
        }
    }
    fail("SYNTAX");
}
