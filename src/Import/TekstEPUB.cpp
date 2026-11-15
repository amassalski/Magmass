#include "TekstEPUB.h"
#include "MiniXML.h"

#include <algorithm>
#include <cctype>
#include <map>

namespace TekstEPUB {
namespace {

std::string male(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string bezPrefiksu(const std::string& nazwa) {
    const size_t d = nazwa.rfind(':');
    return male(d == std::string::npos ? nazwa : nazwa.substr(d + 1));
}

void dopiszUTF8(std::string& w, unsigned long kod) {
    if (kod < 0x80) w += static_cast<char>(kod);
    else if (kod < 0x800) { w += static_cast<char>(0xC0 | (kod >> 6)); w += static_cast<char>(0x80 | (kod & 0x3F)); }
    else if (kod < 0x10000) { w += static_cast<char>(0xE0 | (kod >> 12)); w += static_cast<char>(0x80 | ((kod >> 6) & 0x3F)); w += static_cast<char>(0x80 | (kod & 0x3F)); }
    else { w += static_cast<char>(0xF0 | (kod >> 18)); w += static_cast<char>(0x80 | ((kod >> 12) & 0x3F)); w += static_cast<char>(0x80 | ((kod >> 6) & 0x3F)); w += static_cast<char>(0x80 | (kod & 0x3F)); }
}

/// Encja zaczynająca się na pozycji i (znak '&'); zwraca długość encji (0 = to nie encja) i dopisuje jej tekst
size_t encja(const std::string& s, size_t i, std::string& wynik) {
    const size_t k = s.find(';', i);
    if (k == std::string::npos || k - i > 10) return 0;
    const std::string nazwa = s.substr(i + 1, k - i - 1);
    static const std::map<std::string, const char*> znane = {
        {"amp", "&"}, {"lt", "<"}, {"gt", ">"}, {"quot", "\""}, {"apos", "'"}, {"nbsp", " "}, {"shy", ""},
        {"ndash", "\xE2\x80\x93"}, {"mdash", "\xE2\x80\x94"}, {"hellip", "\xE2\x80\xA6"}, {"laquo", "\xC2\xAB"}, {"raquo", "\xC2\xBB"},
        {"lsquo", "\xE2\x80\x98"}, {"rsquo", "\xE2\x80\x99"}, {"ldquo", "\xE2\x80\x9C"}, {"rdquo", "\xE2\x80\x9D"}};
    auto it = znane.find(nazwa);
    if (it != znane.end()) { wynik += it->second; return k - i + 1; }
    if (nazwa.size() > 1 && nazwa[0] == '#') {
        const bool hex = nazwa[1] == 'x' || nazwa[1] == 'X';
        const std::string cyfry = nazwa.substr(hex ? 2 : 1);
        if (cyfry.empty()) return 0;
        unsigned long kod = 0;
        for (char c : cyfry) {
            const int v = std::isdigit(static_cast<unsigned char>(c)) ? c - '0' : (hex && std::isxdigit(static_cast<unsigned char>(c)) ? std::tolower(c) - 'a' + 10 : -1);
            if (v < 0) return 0;
            kod = kod * (hex ? 16 : 10) + static_cast<unsigned long>(v);
        }
        if (kod == 0 || kod > 0x10FFFF) return 0;
        dopiszUTF8(wynik, kod);
        return k - i + 1;
    }
    return 0;
}

bool znacznikBlokowy(const std::string& n) {
    static const char* bloki[] = {"p", "div", "br", "h1", "h2", "h3", "h4", "h5", "h6", "li", "tr", "section", "article", "blockquote",
                                  "pre", "table", "ul", "ol", "dd", "dt", "hr", "header", "footer", "title"};
    for (const char* b : bloki) if (n == b) return true;
    return false;
}

std::string odkoduj(const std::string& s) {   // %20 w href
    std::string w;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size() + 0 && std::isxdigit(static_cast<unsigned char>(s[i + 1])) && std::isxdigit(static_cast<unsigned char>(s[i + 2]))) {
            w += static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else w += s[i];
    }
    return w;
}

}  // namespace

std::string tekstZHTML(const std::string& html) {
    std::string w;
    w.reserve(html.size() / 2);
    for (size_t i = 0; i < html.size();) {
        const char c = html[i];
        if (c == '<') {
            if (html.compare(i, 4, "<!--") == 0) {
                const size_t k = html.find("-->", i + 4);
                i = k == std::string::npos ? html.size() : k + 3;
                continue;
            }
            const size_t k = html.find('>', i);
            if (k == std::string::npos) break;
            std::string wnetrze = html.substr(i + 1, k - i - 1);
            const bool zamykajacy = !wnetrze.empty() && wnetrze[0] == '/';
            size_t p = zamykajacy ? 1 : 0;
            size_t e = p;
            while (e < wnetrze.size() && (std::isalnum(static_cast<unsigned char>(wnetrze[e])) || wnetrze[e] == ':' )) ++e;
            const std::string nazwa = bezPrefiksu(wnetrze.substr(p, e - p));
            i = k + 1;
            if (!zamykajacy && (nazwa == "script" || nazwa == "style")) {
                const size_t z = male(html).find("</" + nazwa, i);
                if (z == std::string::npos) break;
                const size_t k2 = html.find('>', z);
                i = k2 == std::string::npos ? html.size() : k2 + 1;
                continue;
            }
            if (znacznikBlokowy(nazwa)) w += '\x01';   // granica akapitu (zwykły \n w źródle to tylko biała spacja)
            continue;
        }
        if (c == '&') {
            const size_t d = encja(html, i, w);
            if (d) { i += d; continue; }
        }
        w += c;
        ++i;
    }
    // białe znaki: spacje i tabulatory zwijamy, puste linie wyrzucamy
    std::string wynik;
    std::string linia;
    auto zakoncz = [&] {
        size_t a = linia.find_first_not_of(' ');
        if (a != std::string::npos) {
            const size_t b = linia.find_last_not_of(' ');
            if (!wynik.empty()) wynik += '\n';
            wynik += linia.substr(a, b - a + 1);
        }
        linia.clear();
    };
    bool spacja = false;
    for (char c : w) {
        if (c == '\x01') { zakoncz(); spacja = false; continue; }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f') { if (!spacja) linia += ' '; spacja = true; continue; }
        linia += c;
        spacja = false;
    }
    zakoncz();
    return wynik;
}

std::string polaczSciezke(const std::string& katalog, const std::string& hrefSurowy) {
    std::string href = hrefSurowy;
    const size_t f = href.find('#');
    if (f != std::string::npos) href.erase(f);
    href = odkoduj(href);
    std::vector<std::string> czesci;
    auto dodaj = [&](const std::string& sciezka) {
        size_t start = 0;
        while (start <= sciezka.size()) {
            size_t k = sciezka.find('/', start);
            if (k == std::string::npos) k = sciezka.size();
            const std::string cz = sciezka.substr(start, k - start);
            if (cz == "..") { if (!czesci.empty()) czesci.pop_back(); }
            else if (!cz.empty() && cz != ".") czesci.push_back(cz);
            start = k + 1;
        }
    };
    if (!href.empty() && href[0] == '/') dodaj(href);
    else { dodaj(katalog); dodaj(href); }
    std::string w;
    for (const std::string& c : czesci) w += (w.empty() ? "" : "/") + c;
    return w;
}

std::vector<std::string> kolejnoscRozdzialow(const std::string& opf, const std::string& katalogOPF) {
    std::vector<std::string> wynik;
    std::string blad;
    auto korzen = MiniXML::parsuj(opf, blad);
    if (!korzen) return wynik;
    std::map<std::string, std::string> manifest;   // id -> href
    std::vector<std::string> spine;
    std::vector<const MiniXML::Wezel*> stos{korzen.get()};
    while (!stos.empty()) {
        const MiniXML::Wezel* w = stos.back();
        stos.pop_back();
        const std::string n = bezPrefiksu(w->nazwa);
        if (n == "item") {
            const std::string typ = male(w->atrybut("media-type"));
            if (typ.find("html") != std::string::npos || typ.find("xml") != std::string::npos)
                manifest[w->atrybut("id")] = w->atrybut("href");
        } else if (n == "itemref") {
            spine.push_back(w->atrybut("idref"));
        }
        for (auto it = w->dzieci.rbegin(); it != w->dzieci.rend(); ++it) stos.push_back(it->get());
    }
    for (const std::string& id : spine) {
        auto it = manifest.find(id);
        if (it != manifest.end() && !it->second.empty()) wynik.push_back(polaczSciezke(katalogOPF, it->second));
    }
    return wynik;
}

std::vector<std::string> podzielTekst(const std::string& tekst, size_t maks) {
    std::vector<std::string> wynik;
    if (maks < 200) maks = 200;
    std::string biezacy;
    auto zrzuc = [&] { if (!biezacy.empty()) { wynik.push_back(biezacy); biezacy.clear(); } };
    size_t start = 0;
    while (start < tekst.size()) {
        size_t k = tekst.find('\n', start);
        if (k == std::string::npos) k = tekst.size();
        std::string akapit = tekst.substr(start, k - start);
        start = k + 1;
        while (akapit.size() > maks) {            // pojedynczy bardzo długi akapit: tniemy na spacji
            size_t c = akapit.rfind(' ', maks);
            if (c == std::string::npos || c < maks / 2) c = maks;
            while (c < akapit.size() && (static_cast<unsigned char>(akapit[c]) & 0xC0) == 0x80) ++c;   // nie w środku znaku UTF-8
            if (!biezacy.empty() && biezacy.size() + c > maks) zrzuc();
            biezacy += (biezacy.empty() ? "" : "\n") + akapit.substr(0, c);
            zrzuc();
            akapit.erase(0, c);
            const size_t b = akapit.find_first_not_of(' ');
            akapit.erase(0, b == std::string::npos ? akapit.size() : b);
        }
        if (akapit.empty()) continue;
        if (!biezacy.empty() && biezacy.size() + akapit.size() + 1 > maks) zrzuc();
        biezacy += (biezacy.empty() ? "" : "\n") + akapit;
    }
    zrzuc();
    return wynik;
}

} // namespace TekstEPUB
