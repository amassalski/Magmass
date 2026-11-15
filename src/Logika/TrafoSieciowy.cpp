#include "TrafoSieciowy.h"

#include <algorithm>
#include <cmath>

namespace TrafoSieciowy {

namespace {
const double kSeria[] = {0.05, 0.06, 0.07, 0.08, 0.09, 0.10, 0.11, 0.12, 0.13, 0.14, 0.15, 0.16, 0.17, 0.18, 0.19, 0.20, 0.21, 0.23, 0.25,
                         0.27, 0.29, 0.31, 0.33, 0.35, 0.38, 0.41, 0.44, 0.47, 0.51, 0.55, 0.59, 0.64, 0.69, 0.74, 0.80, 0.86, 0.93, 1.00,
                         1.08, 1.16, 1.25, 1.35, 1.45, 1.56, 1.68, 1.81, 1.95, 2.10, 2.26, 2.44};
const double kPi = 3.14159265358979323846;
}

Typowe typowe(double p) {
    if (p <= 10) return {6000, 0.65, 2.75};
    if (p <= 30) return {7000, 0.75, 2.5};
    if (p <= 50) return {8000, 0.825, 2.25};
    if (p <= 100) return {8750, 0.875, 2.25};
    return {11000, 0.9, 2.0};
}

double drutNormalny(double d) {
    if (d > 2.44) return std::round(d * 100.0) / 100.0;
    double naj = kSeria[0];
    for (double s : kSeria) if (std::abs(s - d) <= std::abs(naj - d) + 1e-12) naj = s;   // najbliższa, przy remisie większa
    return naj;
}

double drutWEmalii(double d) {
    return d + (d <= 0.3 ? 0.02 : d <= 0.8 ? 0.04 : d <= 1.1 ? 0.05 : 0.06);
}

namespace {
WynikUzwojenia ukladaj(int zwoje, double pradA, double gestosc, double yMm, double beta, double gamma) {
    WynikUzwojenia u;
    u.zwoje = zwoje;
    u.pradA = pradA;
    u.drutObliczonyMm = std::sqrt(4.0 * pradA / (kPi * gestosc));
    u.drutMm = drutNormalny(u.drutObliczonyMm);
    u.drutEmaliaMm = drutWEmalii(u.drutMm);
    u.zwojeWWarstwie = std::max(1, static_cast<int>(std::floor(yMm / u.drutEmaliaMm)));
    u.warstwy = (zwoje + u.zwojeWWarstwie - 1) / u.zwojeWWarstwie;
    u.gruboscMm = u.warstwy * (u.drutEmaliaMm + beta) + gamma;
    return u;
}
}

std::optional<Wynik> oblicz(const Wejscie& w) {
    const Dlawik::Rdzen& r = w.rdzen;
    for (double v : {r.a, r.b, r.c, r.h, w.napiecieSieciV, w.bGs, w.sprawnosc, w.gestoscAmm2, w.kOkna, w.kFe})
        if (!std::isfinite(v) || !(v > 0)) return std::nullopt;
    if (w.sprawnosc > 1 || w.wtorne.empty()) return std::nullopt;
    for (const Uzwojenie& u : w.wtorne)
        if (!std::isfinite(u.napiecieV) || !std::isfinite(u.pradA) || !(u.napiecieV > 0) || !(u.pradA > 0)) return std::nullopt;

    Wynik o;
    for (const Uzwojenie& u : w.wtorne) o.mocWtornaVA += u.napiecieV * u.pradA;
    o.mocPierwotnaVA = o.mocWtornaVA / w.sprawnosc;                      // [6]
    o.pradPierwotnyA = 1.1 * o.mocPierwotnaVA / w.napiecieSieciV;        // [7]
    o.przekrojZelazaCm2 = r.a * r.b * w.kFe / 100.0;
    o.oknoCm2 = r.c * r.h / 100.0;
    o.iloczynPosiadanyCm4 = o.przekrojZelazaCm2 * o.oknoCm2;
    // [10]: q_o·q_z = 10⁴/B · P_II (1+η) / (2 η k_m j)
    o.iloczynWymaganyCm4 = 1e4 / w.bGs * o.mocWtornaVA * (1 + w.sprawnosc) / (2 * w.sprawnosc * w.kOkna * w.gestoscAmm2);
    o.srDlugoscZwojuCm = w.srDlugoscZwojuCm > 0 ? w.srDlugoscZwojuCm : (2.0 * (r.a + r.b) + 0.8 * kPi * r.c) / 10.0;
    const double bq = w.bGs * o.przekrojZelazaCm2, spadek = o.srDlugoscZwojuCm * w.gestoscAmm2 * 100.0;
    if (!(bq - spadek > 0)) return std::nullopt;
    if (!(w.marginesWarstwyMm >= 0)) return std::nullopt;
    const double y = r.h - 2.0 * w.sciankaMm - 2.0 * w.marginesWarstwyMm;   // [19] z marginesem na brzegach warstwy
    if (!(y > 0)) return std::nullopt;
    const int z1 = static_cast<int>(std::lround(w.napiecieSieciV * 1e6 / (2.0 * (bq + spadek))));       // [15]
    o.pierwotne = ukladaj(z1, o.pradPierwotnyA, w.gestoscAmm2, y, w.przekladkaWarstwMm, w.przekladkaUzwojenMm);
    double suma = o.pierwotne.gruboscMm;
    for (const Uzwojenie& u : w.wtorne) {
        const int zn = static_cast<int>(std::lround(u.napiecieV * 1e6 / (2.0 * (bq - spadek))));       // [17]
        o.wtorne.push_back(ukladaj(std::max(1, zn), u.pradA, w.gestoscAmm2, y, w.przekladkaWarstwMm, w.przekladkaUzwojenMm));
        suma += o.wtorne.back().gruboscMm;
    }
    o.sumaGruboscMm = suma;
    o.zapasMm = r.c - (suma + w.sciankaMm);                                // [25]
    o.miesciSie = o.zapasMm >= 0;
    return o;
}

} // namespace TrafoSieciowy
