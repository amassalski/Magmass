#include "MasaDrutu.h"

#include <algorithm>
#include <cmath>

namespace MasaDrutu {

namespace {
constexpr double kPi = 3.14159265358979323846;   // stała M_PI nie jest dostępna w MinGW przy -std=c++17
}

double masaMiedziG(double d, double l) {
    if (!(d > 0) || !(l > 0)) return 0;
    return kPi / 4.0 * d * d * l * kGestoscMiedziGcm3;
}

Zestawienie zestawienie(const std::vector<Pozycja>& pozycje, double zapas) {
    Zestawienie z;
    for (const Pozycja& p : pozycje) {
        if (!(p.drutMm > 0) || !(p.zwoje > 0) || !(p.dlugoscZwojuCm > 0)) continue;
        const double dl = p.zwoje * p.dlugoscZwojuCm / 100.0;
        Wiersz* w = nullptr;
        for (Wiersz& x : z.wiersze) if (std::fabs(x.drutMm - p.drutMm) < 0.0005) { w = &x; break; }
        if (!w) { z.wiersze.push_back(Wiersz()); w = &z.wiersze.back(); w->drutMm = p.drutMm; }
        w->dlugoscM += dl;
        if (!w->uzwojenia.empty()) w->uzwojenia += ", ";
        w->uzwojenia += p.nazwa;
    }
    std::sort(z.wiersze.begin(), z.wiersze.end(), [](const Wiersz& a, const Wiersz& b) { return a.drutMm < b.drutMm; });
    for (Wiersz& w : z.wiersze) {
        w.masaCuG = masaMiedziG(w.drutMm, w.dlugoscM);
        w.masaZakupG = w.masaCuG * (1 + kDodatekEmaliiPct / 100.0) * (1 + zapas / 100.0);
        z.razemCuG += w.masaCuG;
        z.razemZakupG += w.masaZakupG;
        z.razemDlugoscM += w.dlugoscM;
    }
    return z;
}

std::vector<Pozycja> pozycje(const Dlawik::Wynik& w) {
    return {{"L", w.drutGolyMm, static_cast<double>(w.zwoje), w.sredniaDlugoscZwojuCm}};
}

std::vector<Pozycja> pozycje(const TrafoSieciowy::Wynik& w) {
    std::vector<const TrafoSieciowy::WynikUzwojenia*> uz{&w.pierwotne};
    for (const auto& x : w.wtorne) uz.push_back(&x);
    double suma = 0;
    for (const auto* u : uz) suma += u->gruboscMm;
    std::vector<Pozycja> r;
    double przed = 0;
    for (size_t i = 0; i < uz.size(); ++i) {
        // średnia długość zwoju rośnie z odległością od rdzenia: poprawka 2π·(środek warstwy − środek całości), średnia pozostaje srDlugoscZwojuCm
        const double srodek = przed + uz[i]->gruboscMm / 2.0;
        const double dl = w.srDlugoscZwojuCm + 2 * kPi * (srodek - suma / 2.0) / 10.0;
        przed += uz[i]->gruboscMm;
        r.push_back({i == 0 ? "P" : "W" + std::to_string(i), uz[i]->drutMm, static_cast<double>(uz[i]->zwoje), dl > 0 ? dl : w.srDlugoscZwojuCm});
    }
    return r;
}

namespace {
template <class S>
std::vector<Pozycja> zSekcji(const std::vector<S>& sekcje, double drutP, double drutW) {
    std::vector<Pozycja> r;
    int np = 1, nw = 1;
    for (const S& s : sekcje)
        r.push_back({s.pierwotna ? "P" + std::to_string(np++) : "W" + std::to_string(nw++), s.pierwotna ? drutP : drutW,
                     static_cast<double>(s.zwoje), s.dlugoscZwojuCm});
    return r;
}
}  // namespace

std::vector<Pozycja> pozycje(const TrafoSE::WynikWykonane& o, const TrafoSE::Wykonane& u) {
    return zSekcji(o.sekcje, u.drutPierwotnyMm, u.drutWtornyMm);
}

std::vector<Pozycja> pozycje(const TrafoPP::WynikWykonane& o, const TrafoPP::Wykonane& u) {
    return zSekcji(o.sekcje, u.drutPierwotnyMm, u.drutWtornyMm);
}

}  // namespace MasaDrutu
