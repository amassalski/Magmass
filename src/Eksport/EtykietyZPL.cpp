#include "EtykietyZPL.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>

namespace EtykietyZPL {

int mmNaKropki(double mm, int dpi) { return static_cast<int>(std::lround(mm * dpi / 25.4)); }

namespace {

/// Znaki UTF-8 po kolei (jako ciągi bajtów)
std::vector<std::string> znaki(const std::string& s) {
    std::vector<std::string> w;
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
        if (i + n > s.size()) n = 1;
        w.push_back(s.substr(i, n));
        i += n;
    }
    return w;
}

std::string laczZnaki(const std::vector<std::string>& z, size_t ile) {
    std::string w;
    for (size_t i = 0; i < ile && i < z.size(); ++i) w += z[i];
    return w;
}

std::string liczba(int n) { return std::to_string(n); }

std::string dopasujDoSzerokosci(const std::string& tekst, int szerokoscKropek, int wysokosc) {
    // czcionka 0 jest proporcjonalna; zakładamy średnio ok. 0,52 wysokości na znak (ostrożnie)
    const int maks = std::max(1, static_cast<int>(szerokoscKropek / (0.52 * std::max(1, wysokosc))));
    const std::vector<std::string> z = znaki(tekst);
    if (static_cast<int>(z.size()) <= maks) return tekst;
    return laczZnaki(z, static_cast<size_t>(std::max(1, maks - 1))) + ".";
}

std::string pierwszaLinia(const std::string& s) { return s.substr(0, s.find('\n')); }

}  // namespace

std::string bezPolskichZnakow(const std::string& utf8) {
    static const std::map<std::string, std::string> mapa = {
        {"ą", "a"}, {"ć", "c"}, {"ę", "e"}, {"ł", "l"}, {"ń", "n"}, {"ó", "o"}, {"ś", "s"}, {"ź", "z"}, {"ż", "z"},
        {"Ą", "A"}, {"Ć", "C"}, {"Ę", "E"}, {"Ł", "L"}, {"Ń", "N"}, {"Ó", "O"}, {"Ś", "S"}, {"Ź", "Z"}, {"Ż", "Z"},
        {"µ", "u"}, {"μ", "u"}, {"Ω", "ohm"}, {"Ω", "ohm"}, {"±", "+/-"}, {"°", "deg"}, {"–", "-"}, {"—", "-"},
        {"…", "..."}, {"„", "\""}, {"”", "\""}, {"“", "\""}};
    std::string w;
    for (const std::string& z : znaki(utf8)) {
        const auto it = mapa.find(z);
        w += it == mapa.end() ? z : it->second;
    }
    return w;
}

std::string zabezpiecz(const std::string& utf8) {
    std::string w;
    char bufor[8];
    for (unsigned char c : utf8) {
        if (c == '^' || c == '~' || c == '_') {
            std::snprintf(bufor, sizeof bufor, "_%02X", c);
            w += bufor;
        } else if (c == '\n' || c == '\r') {
            w += ' ';
        } else {
            w += static_cast<char>(c);
        }
    }
    return w;
}

Uklad zbudujUklad(const Etykieta& e, const Ustawienia& u) {
    Uklad uk;
    const int dpi = std::max(100, u.dpi);
    uk.szer = mmNaKropki(u.szerMm, dpi);
    uk.wys = mmNaKropki(u.wysMm, dpi);
    const int margines = std::max(4, mmNaKropki(2.0, dpi));
    const int szerWew = std::max(10, uk.szer - 2 * margines);
    const int wysWew = std::max(10, uk.wys - 2 * margines);
    auto mm = [&](double v) { return mmNaKropki(v, dpi); };
    auto tekst = [&](const std::string& s) { return u.bezPolskichZnakow ? bezPolskichZnakow(s) : s; };

    // --- kod kreskowy u dołu etykiety (paski + cyfry drukowane przez drukarkę pod spodem)
    const std::vector<bool> moduly = KodKreskowy::moduly(e.typ, e.kod);
    int zajeteDol = 0;
    if (!e.kod.empty() && !moduly.empty()) {
        const int cyfry = mm(3.5);
        int wysKodu = std::min(mm(14), static_cast<int>(wysWew * 0.38));
        wysKodu = std::max(wysKodu, mm(6));
        const int liczbaModulow = static_cast<int>(moduly.size());
        const int cel = std::max(1, mmNaKropki(0.33, dpi));                  // ok. 0,33 mm: pewny odczyt
        const int miesci = std::max(1, szerWew / (liczbaModulow + 20));      // z ciszą po bokach
        const int modul = std::max(1, std::min({cel, miesci, 10}));
        const int szerKodu = liczbaModulow * modul;
        uk.kod.jest = true;
        uk.kod.typ = e.typ;
        uk.kod.tekst = e.kod;
        uk.kod.modul = modul;
        uk.kod.wysokosc = wysKodu;
        // EAN-13 drukuje pierwszą cyfrę na lewo od pasków (ok. 7 modułów)
        const int lewoCyfra = e.typ == KodKreskowy::Typ::EAN13 ? 7 * modul : 0;
        uk.kod.x = margines + std::max(0, (szerWew - szerKodu - lewoCyfra) / 2) + lewoCyfra;
        uk.kod.y = uk.wys - margines - cyfry - wysKodu;
        zajeteDol = wysKodu + cyfry + mm(1);
    }

    // --- teksty od góry
    struct Linia { std::string t; bool gruby; double mmWys; };
    std::vector<Linia> linie;
    linie.push_back({e.symbol, true, 6.0});
    if (!e.drugaLinia.empty()) linie.push_back({e.drugaLinia, false, 3.8});
    if (!e.miejsce.empty()) linie.push_back({e.miejsce, false, 3.8});
    if (!e.producent.empty()) linie.push_back({e.producent, false, 3.4});
    if (!e.opis.empty()) linie.push_back({pierwszaLinia(e.opis), false, 3.4});

    const int dostepne = wysWew - zajeteDol;
    int y = margines;
    for (size_t i = 0; i < linie.size(); ++i) {
        int wys = mm(linie[i].mmWys);
        const std::string t = tekst(linie[i].t);
        if (t.empty()) continue;
        if (i == 0) {  // symbol zmniejszamy, aż się zmieści w całości (do połowy rozmiaru)
            const int min = std::max(mm(3.0), wys / 2);
            while (wys > min && static_cast<int>(znaki(t).size()) * 0.52 * wys > szerWew) wys = static_cast<int>(wys * 0.93);
        }
        if (y + wys > margines + dostepne) break;
        Tekst tk;
        tk.x = margines;
        tk.y = y;
        tk.wysokosc = wys;
        tk.gruby = linie[i].gruby;
        tk.tekst = dopasujDoSzerokosci(t, szerWew, wys);
        uk.teksty.push_back(tk);
        y += static_cast<int>(wys * 1.25);
    }
    return uk;
}

std::string zpl(const Etykieta& e, const Ustawienia& u) {
    const Uklad uk = zbudujUklad(e, u);
    std::string s;
    s += "^XA\n^CI28\n^PW" + liczba(uk.szer) + "\n^LL" + liczba(uk.wys) + "\n^LH0,0\n";
    s += u.obrot180 ? "^POI\n" : "^PON\n";
    for (const Tekst& t : uk.teksty) {
        // czcionka 0 (skalowalna); „gruby” uzyskujemy nieco szerszymi znakami
        const int szerZnaku = static_cast<int>(t.wysokosc * (t.gruby ? 0.62 : 0.5));
        s += "^FO" + liczba(t.x) + "," + liczba(t.y) + "^A0N," + liczba(t.wysokosc) + "," + liczba(szerZnaku) +
             "^FH_^FD" + zabezpiecz(t.tekst) + "^FS\n";
    }
    if (uk.kod.jest) {
        const Kod& k = uk.kod;
        std::string polecenie;
        switch (k.typ) {
            case KodKreskowy::Typ::EAN13: polecenie = "^BEN," + liczba(k.wysokosc) + ",Y,N"; break;
            case KodKreskowy::Typ::EAN8: polecenie = "^B8N," + liczba(k.wysokosc) + ",Y,N"; break;
            default: polecenie = "^BCN," + liczba(k.wysokosc) + ",Y,N,N,A"; break;
        }
        s += "^FO" + liczba(k.x) + "," + liczba(k.y) + "^BY" + liczba(k.modul) + polecenie + "^FD" + k.tekst + "^FS\n";
    }
    s += "^PQ" + liczba(std::max(1, e.kopie)) + "\n^XZ\n";
    return s;
}

std::string zplWielu(const std::vector<Etykieta>& etykiety, const Ustawienia& u) {
    std::string s;
    size_t i = 0;
    while (i < etykiety.size()) {
        Etykieta e = etykiety[i];
        int kopie = std::max(1, e.kopie);
        size_t j = i + 1;
        while (j < etykiety.size() && etykiety[j].symbol == e.symbol && etykiety[j].kod == e.kod &&
               etykiety[j].drugaLinia == e.drugaLinia && etykiety[j].miejsce == e.miejsce) {
            kopie += std::max(1, etykiety[j].kopie);
            ++j;
        }
        e.kopie = kopie;
        s += zpl(e, u);
        i = j;
    }
    return s;
}

std::string zplProbny(const Ustawienia& u) {
    const int dpi = std::max(100, u.dpi);
    const int szer = mmNaKropki(u.szerMm, dpi), wys = mmNaKropki(u.wysMm, dpi);
    Etykieta e;
    e.symbol = "TEST Magmass";
    char opis[64];
    std::snprintf(opis, sizeof opis, "%.0f x %.0f mm, %d dpi", u.szerMm, u.wysMm, u.dpi);
    e.drugaLinia = opis;
    e.opis = u.bezPolskichZnakow ? "Zolc gesla jazn" : "Żółć gęślą jaźń";
    e.kod = "2000000000015";
    e.typ = KodKreskowy::Typ::EAN13;
    std::string s = zpl(e, u);
    // ramka na całej etykiecie: wstawiamy przed ^PQ
    const size_t poz = s.rfind("^PQ");
    const std::string ramka = "^FO0,0^GB" + liczba(std::max(1, szer - 1)) + "," + liczba(std::max(1, wys - 1)) + ",2^FS\n";
    if (poz != std::string::npos) s.insert(poz, ramka);
    return s;
}

}  // namespace EtykietyZPL
