#include "Indeksy.h"

#include <map>
#include <vector>

namespace Indeksy {
namespace {

struct Fragment {
    enum Rodzaj { Zwykly, Dolny, Gorny } rodzaj = Zwykly;
    std::string tekst;
};

bool czyAlnum(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (static_cast<unsigned char>(c) >= 128);
}

std::vector<Fragment> rozbij(const std::string& t) {
    std::vector<Fragment> wynik;
    std::string zwykly;
    auto zamknijZwykly = [&] {
        if (!zwykly.empty()) { wynik.push_back({Fragment::Zwykly, zwykly}); zwykly.clear(); }
    };
    size_t i = 0;
    while (i < t.size()) {
        const char c = t[i];
        if (c == '\\' && i + 1 < t.size() && (t[i + 1] == '_' || t[i + 1] == '^' || t[i + 1] == '\\')) {
            zwykly += t[i + 1];
            i += 2;
            continue;
        }
        if (c == '_' || c == '^') {
            const Fragment::Rodzaj rodzaj = c == '_' ? Fragment::Dolny : Fragment::Gorny;
            size_t j = i + 1;
            std::string tresc;
            if (j < t.size() && t[j] == '{') {
                const size_t koniec = t.find('}', j + 1);
                if (koniec != std::string::npos) {
                    tresc = t.substr(j + 1, koniec - j - 1);
                    j = koniec + 1;
                } else {
                    zwykly += c; ++i; continue;
                }
            } else {
                if (rodzaj == Fragment::Gorny && j < t.size() && (t[j] == '-' || t[j] == '+')) { tresc += t[j]; ++j; }
                while (j < t.size() && czyAlnum(t[j])) { tresc += t[j]; ++j; }
                if (tresc.empty() || tresc == "-" || tresc == "+") { zwykly += c; ++i; continue; }
            }
            zamknijZwykly();
            wynik.push_back({rodzaj, tresc});
            i = j;
            continue;
        }
        zwykly += c;
        ++i;
    }
    zamknijZwykly();
    return wynik;
}

std::string html(const std::string& s) {
    std::string w;
    for (char c : s) {
        switch (c) {
            case '&': w += "&amp;"; break;
            case '<': w += "&lt;"; break;
            case '>': w += "&gt;"; break;
            case '"': w += "&quot;"; break;
            default: w += c;
        }
    }
    return w;
}

const std::map<char, const char*>& dolne() {
    static const std::map<char, const char*> m = {
        {'0', "\xE2\x82\x80"}, {'1', "\xE2\x82\x81"}, {'2', "\xE2\x82\x82"}, {'3', "\xE2\x82\x83"}, {'4', "\xE2\x82\x84"},
        {'5', "\xE2\x82\x85"}, {'6', "\xE2\x82\x86"}, {'7', "\xE2\x82\x87"}, {'8', "\xE2\x82\x88"}, {'9', "\xE2\x82\x89"},
        {'+', "\xE2\x82\x8A"}, {'-', "\xE2\x82\x8B"}, {'(', "\xE2\x82\x8D"}, {')', "\xE2\x82\x8E"},
        {'a', "\xE2\x82\x90"}, {'e', "\xE2\x82\x91"}, {'h', "\xE2\x82\x95"}, {'i', "\xE1\xB5\xA2"}, {'j', "\xE2\xB1\xBC"},
        {'k', "\xE2\x82\x96"}, {'l', "\xE2\x82\x97"}, {'m', "\xE2\x82\x98"}, {'n', "\xE2\x82\x99"}, {'o', "\xE2\x82\x92"},
        {'p', "\xE2\x82\x9A"}, {'r', "\xE1\xB5\xA3"}, {'s', "\xE2\x82\x9B"}, {'t', "\xE2\x82\x9C"}, {'u', "\xE1\xB5\xA4"},
        {'v', "\xE1\xB5\xA5"}, {'x', "\xE2\x82\x93"}};
    return m;
}

const std::map<char, const char*>& gorne() {
    static const std::map<char, const char*> m = {
        {'0', "\xE2\x81\xB0"}, {'1', "\xC2\xB9"}, {'2', "\xC2\xB2"}, {'3', "\xC2\xB3"}, {'4', "\xE2\x81\xB4"},
        {'5', "\xE2\x81\xB5"}, {'6', "\xE2\x81\xB6"}, {'7', "\xE2\x81\xB7"}, {'8', "\xE2\x81\xB8"}, {'9', "\xE2\x81\xB9"},
        {'+', "\xE2\x81\xBA"}, {'-', "\xE2\x81\xBB"}, {'(', "\xE2\x81\xBD"}, {')', "\xE2\x81\xBE"}, {'n', "\xE2\x81\xBF"},
        {'i', "\xE2\x81\xB1"}};
    return m;
}

/// Zamiana na znaki Unicode; pusty wynik, gdy któryś znak nie ma odpowiednika
std::string naUnicode(const std::string& s, bool dolny) {
    const auto& mapa = dolny ? dolne() : gorne();
    std::string w;
    for (char c : s) {
        char klucz = c;
        if (dolny && c >= 'A' && c <= 'Z') klucz = static_cast<char>(c - 'A' + 'a');
        auto it = mapa.find(klucz);
        if (it == mapa.end()) return std::string();
        w += it->second;
    }
    return w;
}

} // namespace

std::string naHtml(const std::string& tekst) {
    std::string w;
    for (const Fragment& f : rozbij(tekst)) {
        switch (f.rodzaj) {
            case Fragment::Zwykly: w += html(f.tekst); break;
            case Fragment::Dolny: w += "<sub>" + html(f.tekst) + "</sub>"; break;
            case Fragment::Gorny: w += "<sup>" + html(f.tekst) + "</sup>"; break;
        }
    }
    return w;
}

std::string naTekst(const std::string& tekst) {
    std::string w;
    for (const Fragment& f : rozbij(tekst)) {
        if (f.rodzaj == Fragment::Zwykly) { w += f.tekst; continue; }
        const std::string u = naUnicode(f.tekst, f.rodzaj == Fragment::Dolny);
        w += u.empty() ? f.tekst : u;
    }
    return w;
}

bool maIndeksy(const std::string& tekst) {
    for (const Fragment& f : rozbij(tekst)) if (f.rodzaj != Fragment::Zwykly) return true;
    return false;
}

} // namespace Indeksy
