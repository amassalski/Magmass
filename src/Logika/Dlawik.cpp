#include "Dlawik.h"
#include "tube_magnetics.hpp"

#include <cmath>
#include <cstdlib>

namespace Dlawik {

Rdzen ksztaltkaEI(double a, double pakiet, double blacha) {
    Rdzen r;
    r.a = a; r.A = 3 * a; r.B = 2.5 * a; r.c = a / 2; r.d = a / 2; r.h = 1.5 * a; r.b = pakiet; r.blachaMm = blacha;
    return r;
}

std::optional<Rdzen> ksztaltkaZNazwy(const std::string& n, double pakiet, double blacha) {
    if (n.size() < 3 || n.compare(0, 2, "EI") != 0) return std::nullopt;
    char* kon = nullptr;
    const double A = std::strtod(n.c_str() + 2, &kon);
    if (*kon != 0 || !(A > 0)) return std::nullopt;
    return ksztaltkaEI(A / 3.0, pakiet, blacha);
}

std::optional<Wynik> oblicz(const Wejscie& w) {
    namespace M = tube::magnetics;
    const Rdzen& r = w.rdzen;
    for (double v : {r.a, r.b, r.c, r.d, r.h, w.indukcyjnoscH, w.pradMa, w.mi, w.bMaxT, w.gestoscAmm2})
        if (!std::isfinite(v) || !(v > 0)) return std::nullopt;
    if (w.drutMm < 0 || !std::isfinite(w.drutMm)) return std::nullopt;
    // współczynnik wypełnienia żelazem zależnie od grubości blachy (0,5 mm: 0,97 wg przykładu ECCLab; 0,35 mm: oszacowanie)
    const double kFe = r.blachaMm <= 0.36 ? 0.93 : r.blachaMm <= 0.55 ? 0.97 : 0.97;
    const double pi = 3.14159265358979323846;
    M::CoreGeometry g{r.a * 1e-3, r.c * 1e-3, r.h * 1e-3, r.b * 1e-3, kFe,
                      2.0 * (r.h + (r.a + r.d) / 2.0 + r.c) * 1e-3};   // strumień wokół jednego okna
    const double J = w.gestoscAmm2 * 1e6, I = w.pradMa * 1e-3;
    M::FilterChokeResult c;
    try {
        c = M::calcFilterChoke(g, w.indukcyjnoscH, I, w.bMaxT, J, 0.38, w.mi);
    } catch (...) {
        return std::nullopt;
    }
    Wynik o;
    o.dlugoscStrumieniaCm = g.lFe * 100;
    // średnia długość zwoju i objętość czynna jak w ECCLab (dopasowane do przykładu: EI102, pakiet 51 mm -> 21,27 cm i 343,12 cm³)
    const double lz = 2.0 * (r.a + r.b) + 0.8 * pi * r.c;   // mm
    o.sredniaDlugoscZwojuCm = lz / 10.0;
    o.przekrojCzynnyCm2 = g.Qfe() * 1e4;
    o.objetoscCzynnaCm3 = (r.A * r.B - 2.0 * r.c * r.h) * r.b * kFe / 1000.0;
    // Krize [35]: Vz >= L·I0²/(1...2)·10⁻³ [cm³] (L w H, I0 w mA); ECCLab przyjmuje dzielnik 2
    o.zalecanaObjetoscCm3 = w.indukcyjnoscH * w.pradMa * w.pradMa * 1e-3 / 2.0;
    o.energiaMJ = c.energyJoules * 1e3;
    o.zwoje = c.turns;
    // szczelina fizyczna: całkowita reluktancja μ0·N·I/B pomniejszona o reluktancję drogi w żelazie lFe/μ (przekładka = połowa, bo EI ma dwie szczeliny)
    o.szczelinaMm = c.airGapTotal * 1e3;
    o.przekladkaMm = c.spacerThickness * 1e3;
    // do porównania: wzór Krize'a [38] / ECCLab, lp = z·I0/8·10⁻⁵ mm (I0 w mA), który reluktancji żelaza nie odejmuje
    o.szczelinaKrizeMm = o.zwoje * w.pradMa / 8.0 * 1e-5 * (1.0 / w.bMaxT);
    o.indukcyjnoscRzeczH = c.L_actual;
    o.indukcjaT = w.bMaxT;
    // uzwojenie: drut zaokrąglany do 0,01 mm, w emalii +0,03 mm, ρ = 1/57 Ω·mm²/m (jak w ECCLab)
    const double d = w.drutMm > 0 ? w.drutMm : std::round(c.wireDiaBare * 1e3 * 100.0) / 100.0;
    const double dEm = d + 0.03;
    const double q = pi * d * d / 4.0;   // mm²
    o.drutGolyMm = d;
    o.drutEmaliaMm = dEm;
    o.oporNaMetrOm = (1.0 / 57.0) / q;
    o.opornoscOm = o.oporNaMetrOm * o.zwoje * lz / 1000.0;
    o.spadekV = I * o.opornoscOm;
    const double gestoscUpakowania = 0.838;   // dopasowane: 475 zwojów/cm² dla drutu 0,42 mm
    const double zwojeCm2 = 100.0 * gestoscUpakowania / (dEm * dEm);
    o.zwojeNaCm2 = std::floor(zwojeCm2);
    o.wypelnieniePct = o.zwoje / (zwojeCm2 * r.c * r.h / 100.0) * 100.0;
    o.pradDlaGestosciMa = std::floor(w.gestoscAmm2 * q * 1000.0);
    o.miesciSie = o.wypelnieniePct <= 75.0;
    o.ostrzezenie = o.wypelnieniePct > 75.0 ? Wynik::Ostrzezenie::zaDuzoWypelnione
                  : o.wypelnieniePct < 50.0 ? Wynik::Ostrzezenie::zaMaloWypelnione : Wynik::Ostrzezenie::brak;
    return o;
}

} // namespace Dlawik
