#include "KalkulatoryAnalogowe.h"
#include "Kalkulatory.h"

#include <algorithm>
#include <cmath>

namespace Kalkulatory {

namespace {
constexpr double PI = 3.14159265358979323846;
int liczbaDiod(Prostownik p) { return p == Prostownik::Mostek ? 2 : 1; }
double krotnoscTetnien(Prostownik p) { return p == Prostownik::Jednopolowkowy ? 1.0 : 2.0; }
}

std::optional<WynikZasilacza> zasilaczZTrafa(double u, double i, double c, Prostownik p, double f, double ud) {
    if (!(u > 0) || !(i > 0) || !(c > 0) || !(f > 0) || ud < 0) return std::nullopt;
    WynikZasilacza w;
    w.uSkutecznaWtorna = u;
    w.uSzczytowa = u * std::sqrt(2.0) - liczbaDiod(p) * ud;
    if (w.uSzczytowa <= 0) return std::nullopt;
    w.czestTetnien = krotnoscTetnien(p) * f;
    w.tetnieniaPP = i / (w.czestTetnien * c);
    w.uSrednia = w.uSzczytowa - w.tetnieniaPP / 2;
    if (w.uSrednia <= 0) return std::nullopt;
    w.mocDCW = w.uSrednia * i;
    // Regula kciuka dla filtra pojemnosciowego: prad skuteczny uzwojenia ok. 1,8 * Idc (mostek, odczep) i ok. 2,5 * Idc (jednopolowkowy);
    // moc transformatora = Usek * Isk.
    const double wsp = p == Prostownik::Jednopolowkowy ? 2.5 : 1.8;
    w.pradWtornyA = wsp * i;
    w.mocTrafoVA = u * w.pradWtornyA;   // odczep: u to napiecie jednej polowki, obie pracuja na przemian
    // Prad szczytowy diody ~ Idc * (1 + pi * sqrt(2 Udc / Upp)): ladowanie kondensatora w krotkim oknie.
    w.pradSkokowyDiodyA = i * (1.0 + PI * std::sqrt(2.0 * w.uSzczytowa / std::max(w.tetnieniaPP, 1e-9)));
    return w;
}

std::optional<double> napiecieWtorneDlaWyjscia(double uWy, double upp, Prostownik p, double ud) {
    if (!(uWy > 0) || upp < 0 || ud < 0) return std::nullopt;
    return (uWy + upp / 2 + liczbaDiod(p) * ud) / std::sqrt(2.0);
}

double kondensatorFiltra(double i, double upp, Prostownik p, double f) {
    if (!(i > 0) || !(upp > 0) || !(f > 0)) return 0;
    return i / (krotnoscTetnien(p) * f * upp);
}

double tgDeltaTypowy(double u) {
    // typowe wartości katalogowe (120 Hz, 20 °C) dla serii ogólnego przeznaczenia
    static const double tab[][2] = {{6.3, 0.22}, {10, 0.19}, {16, 0.16}, {25, 0.14}, {35, 0.12}, {50, 0.10},
                                    {63, 0.09}, {100, 0.08}, {160, 0.10}, {250, 0.15}, {400, 0.20}, {450, 0.20}};
    const int n = static_cast<int>(sizeof(tab) / sizeof(tab[0]));
    if (!(u > 0)) return 0;
    if (u <= tab[0][0]) return tab[0][1];
    if (u >= tab[n - 1][0]) return tab[n - 1][1];
    for (int i = 1; i < n; ++i)
        if (u <= tab[i][0]) return tab[i - 1][1] + (tab[i][1] - tab[i - 1][1]) * (u - tab[i - 1][0]) / (tab[i][0] - tab[i - 1][0]);
    return tab[n - 1][1];
}

double esrZTgDelta(double tg, double c, double f) {
    if (!(tg > 0) || !(c > 0) || !(f > 0)) return 0;
    return tg / (2 * PI * f * c);
}

double esrTypowy(double c, double u, double f) { return esrZTgDelta(tgDeltaTypowy(u), c, f); }

double mocStratESR(double i, double esr) { return (i >= 0 && esr >= 0) ? i * i * esr : 0; }

std::optional<WynikTranzystora> punktPracyTranzystora(double vcc, double r1, double r2, double rc, double re, double beta, double vbe) {
    if (!(vcc > 0) || !(r1 > 0) || !(r2 > 0) || rc < 0 || re < 0 || !(beta > 0)) return std::nullopt;
    const double vth = vcc * r2 / (r1 + r2);
    const double rth = r1 * r2 / (r1 + r2);
    WynikTranzystora w;
    double ib = (vth - vbe) / (rth + (beta + 1) * re);
    double ic = 0;
    if (ib <= 0) {
        w.stan = StanTranzystora::Zatkany;
        ib = 0;
    } else {
        ic = beta * ib;
        const double vce = vcc - ic * rc - (ic + ib) * re;
        if (vce < 0.2) {      // nasycenie: Vce ~ 0,2 V, prad ograniczony obwodem
            w.stan = StanTranzystora::Nasycenie;
            const double rsum = rc + re;
            ic = rsum > 0 ? std::max(0.0, (vcc - 0.2) / rsum) : 0;
            ib = ic / beta;
        }
    }
    w.ibUA = ib * 1e6;
    w.icMA = ic * 1e3;
    w.veV = (ic + ib) * re;
    w.vceV = w.stan == StanTranzystora::Nasycenie ? 0.2 : vcc - ic * rc - w.veV;
    if (w.stan == StanTranzystora::Zatkany) w.vceV = vcc;
    w.vbV = w.stan == StanTranzystora::Zatkany ? vth : w.veV + vbe;
    w.mocTranzystoraMW = w.vceV * ic * 1e3;
    w.wzmocnienieNapiecioweOE = ic > 0 ? (ic / 0.02585) * rc : 0;   // gm = Ic / Vt (25,85 mV w 300 K)
    return w;
}

std::optional<ProjektTranzystora> dobierzTranzystor(double vcc, double icMA, double beta, Szereg s, double vbe) {
    if (!(vcc > 1) || !(icMA > 0) || !(beta > 10)) return std::nullopt;
    const double ic = icMA * 1e-3, ib = ic / beta;
    const double ve = 0.1 * vcc;
    ProjektTranzystora p;
    p.re = najblizsza(ve / (ic + ib), s);
    p.rc = najblizsza((vcc - ve - 0.5 * vcc) / ic, s);
    if (!(p.rc > 0)) return std::nullopt;
    const double vb = ve + vbe, idz = 10 * ib;
    p.r2 = najblizsza(vb / idz, s);
    p.r1 = najblizsza((vcc - vb) / (idz + ib), s);
    auto w = punktPracyTranzystora(vcc, p.r1, p.r2, p.rc, p.re, beta, vbe);
    if (!w) return std::nullopt;
    p.wynik = *w;
    return p;
}

std::optional<WynikLampy> punktPracyLampy(const DaneLampy& d) {
    if (!(d.bPlus > 0) || !(d.ra >= 0) || !(d.iaMA > 0) || d.ugV > 0 || d.ig2MA < 0) return std::nullopt;
    WynikLampy w;
    const double ia = d.iaMA * 1e-3, ig2 = d.ig2MA * 1e-3;
    w.uKatodyV = -d.ugV;
    w.rKatodyOm = w.uKatodyV > 0 ? w.uKatodyV / (ia + ig2) : 0;
    w.mocRKatodyW = w.uKatodyV * (ia + ig2);
    w.uAnodyV = d.bPlus - ia * d.ra - w.uKatodyV;
    if (!(w.uAnodyV > 0)) return std::nullopt;
    w.mocAnodyW = w.uAnodyV * ia;
    w.przekroczonaMocAnody = d.paMaxW > 0 && w.mocAnodyW > d.paMaxW;
    if (d.ig2MA > 0 && d.ug2V > 0) w.rEkranuOm = std::max(0.0, (d.bPlus - d.ug2V - w.uKatodyV) / ig2);
    if (d.mi > 0 && d.rWewnAnodowaOm > 0 && d.ra > 0) w.wzmocnienie = d.mi * d.ra / (d.rWewnAnodowaOm + d.ra);
    if (d.fDolnaHz > 0 && d.mi > 0 && d.rWewnAnodowaOm > 0 && w.rKatodyOm > 0) {
        const double gm = d.mi / d.rWewnAnodowaOm;
        const double rz = 1.0 / (gm + 1.0 / w.rKatodyOm);   // rezystancja widziana z katody
        w.ckF = 1.0 / (2 * PI * d.fDolnaHz * rz);
    }
    w.iaMaxMA = d.ra > 0 ? d.bPlus / d.ra * 1e3 : 0;
    return w;
}

} // namespace Kalkulatory
