#include "SchematBarwy.h"

#include <algorithm>
#include <map>
#include <set>

namespace SchematBarwy {

using namespace KalkulatoryWzmacniaczy;

namespace {
constexpr double ROZSTAW_SZYN = 120, WYS_WIERSZA = 50, MARGINES = 40, DLUGOSC_SYMBOLU = 44, GORA = 46;

std::string nazwaDomyslna(const Skladnik& s) {
    switch (s.typ) {
        case Typ::R: return "R" + std::to_string(s.numer);
        case Typ::C: return "C" + std::to_string(s.numer);
        case Typ::L: return "L" + std::to_string(s.numer);
        case Typ::PotLo: return "P" + std::to_string(s.numer) + "a";
        case Typ::PotHi: return "P" + std::to_string(s.numer) + "b";
    }
    return {};
}
Rodzaj rodzajSymbolu(Typ t) { return t == Typ::C ? Rodzaj::Kondensator : t == Typ::L ? Rodzaj::Cewka : Rodzaj::Rezystor; }
}  // namespace

Rysunek zbuduj(const UkladBarwy& u, const std::function<std::string(const Skladnik&)>& etykieta) {
    Rysunek r;
    r.czynny = u.opWe != 0;

    // elementy: szeregowe (oba węzły != 0) w wierszach, bocznikowe (jeden węzeł = masa) zwisają w dół
    std::vector<const Skladnik*> wiersze, boczniki;
    for (const Skladnik& s : u.skladniki) {
        if (s.w1 == s.w2) continue;   // zwarty element nie ma co rysować
        (s.w1 == 0 || s.w2 == 0 ? boczniki : wiersze).push_back(&s);
    }

    // kolejność szyn: wejście (2) pierwsze, potem według przeszukiwania wszerz, wyjście (3) ostatnie; masa osobno
    std::map<int, std::set<int>> sasiedzi;
    std::set<int> wezly = {2, 3};
    for (const Skladnik* s : wiersze) {
        sasiedzi[s->w1].insert(s->w2);
        sasiedzi[s->w2].insert(s->w1);
        wezly.insert(s->w1); wezly.insert(s->w2);
    }
    for (const Skladnik* s : boczniki) wezly.insert(s->w1 == 0 ? s->w2 : s->w1);
    if (u.opWe) wezly.insert(u.opWe);
    std::vector<int> kolejnosc;
    std::set<int> odwiedzone;
    auto bfs = [&](int start) {
        std::vector<int> kolejka = {start};
        odwiedzone.insert(start);
        for (size_t i = 0; i < kolejka.size(); ++i) {
            if (kolejka[i] != 3) kolejnosc.push_back(kolejka[i]);
            for (int n : sasiedzi[kolejka[i]]) if (n != 0 && !odwiedzone.count(n)) { odwiedzone.insert(n); kolejka.push_back(n); }
        }
    };
    bfs(2);
    for (int w : wezly) if (w != 0 && w != 3 && !odwiedzone.count(w)) bfs(w);
    kolejnosc.push_back(3);
    std::map<int, double> xSzyny;
    for (size_t i = 0; i < kolejnosc.size(); ++i) xSzyny[kolejnosc[i]] = MARGINES + i * ROZSTAW_SZYN;

    // wiersze: elementy szeregowe, potem (jeśli czynny) wzmacniacz
    const int nWierszy = static_cast<int>(wiersze.size()) + (r.czynny ? 1 : 0);
    const double yPierwszy = GORA + 20;
    auto yWiersza = [&](int i) { return yPierwszy + i * WYS_WIERSZA; };
    const double yBocznik = yWiersza(std::max(nWierszy, 1)) + 10;
    const double yMasy = yBocznik + 6 + DLUGOSC_SYMBOLU + 8 + 24;

    std::map<int, double> yMin, yMax;
    std::map<int, int> polaczen;
    auto szyna = [&](int w, double y) {
        if (!yMin.count(w)) { yMin[w] = y; yMax[w] = y; }
        yMin[w] = std::min(yMin[w], y); yMax[w] = std::max(yMax[w], y);
        ++polaczen[w];
    };
    struct Kropka { int w; double y; };
    std::vector<Kropka> kropki;
    auto tekst = [&](double x, double y, const std::string& t, bool lewo = false) { r.prymy.push_back({Rodzaj::Tekst, x, y, lewo ? 1.0 : 0.0, 0, t}); };
    auto linia = [&](double x1, double y1, double x2, double y2) { r.prymy.push_back({Rodzaj::Linia, x1, y1, x2, y2, {}}); };
    auto nazwa = [&](const Skladnik& s) { std::string t = etykieta ? etykieta(s) : std::string(); return t.empty() ? nazwaDomyslna(s) : t; };

    for (size_t i = 0; i < wiersze.size(); ++i) {
        const Skladnik& s = *wiersze[i];
        const double y = yWiersza(static_cast<int>(i));
        double xa = xSzyny[s.w1], xb = xSzyny[s.w2];
        if (xa > xb) std::swap(xa, xb);
        // symbol w pierwszej przerwie za lewą szyną, żeby nie leżał na liniach szyn pośrednich
        const double sr = xa + std::min(ROZSTAW_SZYN, xb - xa) / 2, p1 = sr - DLUGOSC_SYMBOLU / 2, p2 = sr + DLUGOSC_SYMBOLU / 2;
        linia(xa, y, p1, y);
        linia(p2, y, xb, y);
        r.prymy.push_back({rodzajSymbolu(s.typ), p1, y, p2, y, {}});
        tekst(sr, y - 24, nazwa(s));
        szyna(s.w1, y); szyna(s.w2, y);
        kropki.push_back({s.w1, y}); kropki.push_back({s.w2, y});
        ++r.elementow;
    }
    if (r.czynny) {
        const double y = yWiersza(static_cast<int>(wiersze.size()));
        const double xa = xSzyny[u.opWe], xb = xSzyny[3];
        const double lewy = std::min(xa, xb), prawy = std::max(xa, xb), sr = lewy + std::min(ROZSTAW_SZYN, prawy - lewy) / 2;
        // trójkąt: wejście odwracające od strony szyny wejściowej, wyjście na szynie 3
        linia(xa, y, sr - 24, y);
        linia(sr + 24, y, xb, y);
        r.prymy.push_back({Rodzaj::Wzmacniacz, sr - 24, y, sr + 24, y, {}});
        tekst(sr, y - 38, "wzmacniacz operacyjny");
        szyna(u.opWe, y); szyna(3, y);
        kropki.push_back({u.opWe, y}); kropki.push_back({3, y});
        if (u.opPlus == 0) {
            // wejście nieodwracające na masie
            linia(sr - 24, y + 11, sr - 34, y + 11);
            linia(sr - 34, y + 11, sr - 34, y + 26);
            r.prymy.push_back({Rodzaj::Masa, sr - 34, y + 26, 0, 0, {}});
        } else {
            // wejście nieodwracające na szynie opPlus (linia poziomo pod wzmacniaczem)
            const double xp = xSzyny[u.opPlus];
            linia(sr - 24, y + 11, xp, y + 11);
            szyna(u.opPlus, y + 11);
            kropki.push_back({u.opPlus, y + 11});
        }
    }

    // boczniki: pionowe symbole od szyny do masy, kilka na jednej szynie obok siebie
    std::map<int, int> licznikBocznikow;
    for (const Skladnik* s : boczniki) {
        const int w = s->w1 == 0 ? s->w2 : s->w1;
        const int k = licznikBocznikow[w]++;
        const double x = xSzyny[w] + k * 70;
        if (k > 0) linia(xSzyny[w], yBocznik, x, yBocznik);
        const double yS = yBocznik + 6, yK = yS + DLUGOSC_SYMBOLU + 8;
        linia(x, yBocznik, x, yS);
        r.prymy.push_back({rodzajSymbolu(s->typ), x, yS, x, yS + DLUGOSC_SYMBOLU, {}});
        linia(x, yS + DLUGOSC_SYMBOLU, x, yK);
        r.prymy.push_back({Rodzaj::Masa, x, yK, 0, 0, {}});
        tekst(x + 8, yS + 12, nazwa(*s), true);
        szyna(w, yBocznik);
        if (k == 0) kropki.push_back({w, yBocznik});
        ++r.elementow;
    }

    // zaciski wejścia i wyjścia
    {
        const double x2 = xSzyny[2], x3 = xSzyny[3];
        szyna(2, GORA); szyna(3, GORA);
        r.prymy.push_back({Rodzaj::Zacisk, x2, GORA, 0, 0, {}});
        r.prymy.push_back({Rodzaj::Zacisk, x3, GORA, 0, 0, {}});
        tekst(x2, GORA - 22, "we");
        tekst(x3, GORA - 22, "wy");
    }
    for (const auto& [w, x] : xSzyny) if (yMin.count(w) && yMax[w] > yMin[w]) linia(x, yMin[w], x, yMax[w]);
    for (const Kropka& k : kropki) if (polaczen[k.w] >= 3) r.prymy.push_back({Rodzaj::Kropka, xSzyny[k.w], k.y, 0, 0, {}});

    r.szerokosc = MARGINES * 2 + (kolejnosc.size() - 1) * ROZSTAW_SZYN + 30;
    for (const Prym& p : r.prymy) r.szerokosc = std::max(r.szerokosc, std::max(p.x1, p.x2) + (p.rodzaj == Rodzaj::Tekst ? 60.0 : 20.0));
    r.wysokosc = yMasy + 30;
    return r;
}

}  // namespace SchematBarwy
