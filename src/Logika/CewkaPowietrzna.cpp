#include "CewkaPowietrzna.h"

#include <cmath>

#include "MasaDrutu.h"

namespace CewkaPowietrzna {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kMi0 = 4e-7 * kPi;

double agm(double a, double b) {
    for (int i = 0; i < 40 && std::fabs(a - b) > 1e-16 * a; ++i) {
        const double an = 0.5 * (a + b);
        b = std::sqrt(a * b);
        a = an;
    }
    return a;
}

bool poprawne(const Parametry& p, double& dz, double& skok) {
    if (!(p.srednicaKarkasuMm > 0) || !(p.drutMm > 0) || p.zwojeNaWarstwe < 1 || p.warstwy < 1) return false;
    dz = p.drutZIzolacjaMm > 0 ? p.drutZIzolacjaMm : 1.06 * p.drutMm;
    if (dz < p.drutMm) return false;
    skok = p.skokMm > 0 ? p.skokMm : dz;
    if (skok < p.drutMm) return false;
    return true;
}
}  // namespace

double calkaK(double k) {
    return kPi / (2.0 * agm(1.0, std::sqrt(1.0 - k * k)));
}

double calkaE(double k) {
    double a = 1.0, b = std::sqrt(1.0 - k * k), c = k;
    double suma = 0.5 * c * c;      // 2^(n-1)·c_n², n = 0
    double pot = 0.5;
    for (int n = 1; n < 40; ++n) {
        const double an = 0.5 * (a + b);
        c = 0.5 * (a - b);
        b = std::sqrt(a * b);
        a = an;
        pot *= 2.0;
        suma += pot * c * c;
        if (std::fabs(c) < 1e-17) break;
    }
    return calkaK(k) * (1.0 - suma);
}

double indukcyjnoscWzajemnaPierscieni(double a, double b, double z) {
    const double k2 = 4.0 * a * b / ((a + b) * (a + b) + z * z);
    const double k = std::sqrt(k2);
    if (k < 1e-9) return 0.0;
    return kMi0 * std::sqrt(a * b) * ((2.0 / k - k) * calkaK(k) - (2.0 / k) * calkaE(k));
}

double indukcyjnoscWlasnaPierscienia(double a, double rDrutu) {
    return kMi0 * a * (std::log(8.0 * a / rDrutu) - 1.75);
}

double wheelerJednowarstwowaUH(double D, double l, double N) {
    const double den = 25.4 * (18.0 * D + 40.0 * l);
    return den > 0 ? D * D * N * N / den : 0.0;
}

double wheelerWielowarstwowaUH(double a, double b, double c, double N) {
    const double den = 25.4 * (6.0 * a + 9.0 * b + 10.0 * c);
    return den > 0 ? 0.8 * a * a * N * N / den : 0.0;
}

std::optional<Wynik> oblicz(const Parametry& p) {
    double dz = 0, skok = 0;
    if (!poprawne(p, dz, skok)) return std::nullopt;
    const int t = p.zwojeNaWarstwe, n = p.warstwy;
    Wynik w;
    w.zwojeRazem = t * n;
    w.dlugoscUzwojeniaMm = t * skok;
    w.gruboscUzwojeniaMm = n * dz;
    w.sredniPromienMm = p.srednicaKarkasuMm / 2.0 + n * dz / 2.0;
    double dlugosc = 0;
    for (int a = 0; a < n; ++a) dlugosc += 2.0 * kPi * ((p.srednicaKarkasuMm + dz) / 2.0 + a * dz) * 1e-3 * t;
    w.dlugoscDrutuM = dlugosc;
    w.opor20Ohm = kRezystywnoscCuOhmM * dlugosc / (kPi / 4.0 * p.drutMm * p.drutMm * 1e-6);
    w.masaMiedziG = MasaDrutu::masaMiedziG(p.drutMm, dlugosc);
    w.wheelerUH = n == 1 ? wheelerJednowarstwowaUH(p.srednicaKarkasuMm + dz, w.dlugoscUzwojeniaMm, t)
                         : wheelerWielowarstwowaUH(w.sredniPromienMm, w.dlugoscUzwojeniaMm, w.gruboscUzwojeniaMm, w.zwojeRazem);
    const double skladniki = 0.5 * n * (n + 1.0) * t;
    if (skladniki > static_cast<double>(kMaxObliczen)) {
        w.indukcyjnoscUH = w.wheelerUH;
        w.dokladny = false;
        return w;
    }
    const double rDrutu = p.drutMm * 0.5e-3;
    double L = 0;
    for (int a = 0; a < n; ++a) {
        const double ra = ((p.srednicaKarkasuMm + dz) / 2.0 + a * dz) * 1e-3;
        for (int b = a; b < n; ++b) {
            const double rb = ((p.srednicaKarkasuMm + dz) / 2.0 + b * dz) * 1e-3;
            const double waga = a == b ? 1.0 : 2.0;
            for (int d = 0; d < t; ++d) {
                const double ile = d == 0 ? t : 2.0 * (t - d);
                const double m = (a == b && d == 0) ? indukcyjnoscWlasnaPierscienia(ra, rDrutu)
                                                     : indukcyjnoscWzajemnaPierscieni(ra, rb, d * skok * 1e-3);
                L += waga * ile * m;
            }
        }
    }
    w.indukcyjnoscUH = L * 1e6;
    return w;
}

std::optional<Dobor> zwojeDlaIndukcyjnosci(Parametry p, double docelowaUH) {
    p.warstwy = 1;
    auto L = [&](int n) -> double { p.zwojeNaWarstwe = n; auto w = oblicz(p); return w ? w->indukcyjnoscUH : -1.0; };
    if (!(docelowaUH > 0) || L(1) < 0) return std::nullopt;
    int hi = 1;
    while (L(hi) < docelowaUH) { hi *= 2; if (hi > 20000) { hi = 20000; if (L(hi) < docelowaUH) return std::nullopt; break; } }
    int lo = hi / 2;   // L(lo) < cel (albo lo = 0)
    while (hi - lo > 1) { const int m = (lo + hi) / 2; if (L(m) >= docelowaUH) hi = m; else lo = m; }
    return Dobor{hi, L(hi), hi > 1 ? L(hi - 1) : 0.0};
}

std::optional<Dobor> warstwyDlaIndukcyjnosci(Parametry p, double docelowaUH) {
    auto L = [&](int n) -> double { p.warstwy = n; auto w = oblicz(p); return w ? w->indukcyjnoscUH : -1.0; };
    if (!(docelowaUH > 0) || L(1) < 0) return std::nullopt;
    for (int n = 1; n <= 100; ++n) {
        const double l = L(n);
        if (l >= docelowaUH) return Dobor{n, l, n > 1 ? L(n - 1) : 0.0};
    }
    return std::nullopt;
}

}  // namespace CewkaPowietrzna
