#include "TrafoModel.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace TrafoModel {

namespace {
const double kPi = 3.14159265358979323846;
const double kMi0 = 4e-7 * kPi;
const double kEps0 = 8.8541878128e-12;
using C = std::complex<double>;

bool dodatnie(double v) { return std::isfinite(v) && v > 0; }

struct PunktPracy { double bT = 0, muPrzyr = 0, lpH = 0, lpPoczH = 0, lgCalkM = 0; };

double agM2(const Rdzen& r, double lgM) {
    const double s = r.przekrojCm2 * 1e-4;
    if (!(r.szerSrodkowaCm > 0) || !(r.pakietCm > 0)) return s;
    return std::max(s, (r.szerSrodkowaCm * 1e-2 + lgM) * (r.pakietCm * 1e-2 + lgM));
}

/// Punkt pracy rdzenia z szczeliną i polaryzacją: at = amperozwoje stałe [A]
PunktPracy punkt(const Rdzen& r, double zwoje, double at, double szczelinaMm) {
    PunktPracy p;
    const double S = r.przekrojCm2 * 1e-4, l = r.drogaCm * 1e-2;
    const double lg = (szczelinaMm + r.szczelinaStykuMm) * 1e-3;
    const double Ag = agM2(r, lg);
    const double mi = r.muPoczatkowa, bs = r.bNasycenieT;
    auto bilans = [&](double b) {   // amperozwoje potrzebne dla indukcji b minus dostępne
        const double h = b / (kMi0 * mi * (1 - b / bs));
        return h * l + b * S * lg / (kMi0 * Ag) - at;
    };
    double lo = 0, hi = bs * (1 - 1e-12);
    if (at > 0) for (int k = 0; k < 200; k++) { const double m = 0.5 * (lo + hi); (bilans(m) > 0 ? hi : lo) = m; }
    p.bT = at > 0 ? 0.5 * (lo + hi) : 0.0;
    p.muPrzyr = mi * std::pow(1 - p.bT / bs, 2);
    p.lgCalkM = lg;
    auto indukc = [&](double mu) { return zwoje * zwoje / (l / (kMi0 * mu * S) + lg / (kMi0 * Ag)); };
    p.lpH = indukc(p.muPrzyr);
    p.lpPoczH = indukc(mi);
    return p;
}

struct Charakt {
    const Rdzen& r; const Uzwojenia& u; const Zrodlo& z;
    double n, lp, masaKg;
    C zWezlaA(double f) const {      // impedancja węzła A (równoległe gałęzie)
        const double w = 2 * kPi * f;
        const double ws = w * u.zwojePierwotne * r.przekrojCm2 * 1e-4;
        const double rc = ws * ws / (2 * r.stratyZelazaWkg * masaKg * std::pow(f / 50.0, 1.5));
        const C yc = C(0, w * u.cdF), yl = 1.0 / C(0, w * lp), yr = C(1.0 / rc, 0);
        const C ob = C(u.rwOm * n * n + n * n * z.rgOm, w * u.lrH);
        return 1.0 / (yc + yl + yr + 1.0 / ob);
    }
    C transmitancja(double f) const {   // Uwyj'/E
        const C za = zWezlaA(f);
        const double w = 2 * kPi * f;
        const C ob = C(u.rwOm * n * n + n * n * z.rgOm, w * u.lrH);
        const C va = za / (za + C(z.riOm + u.rpOm, 0));
        return va * (n * n * z.rgOm) / ob;
    }
    C wezlaDoE(double f) const { const C za = zWezlaA(f); return za / (za + C(z.riOm + u.rpOm, 0)); }
};
}

double spadekDokladnyDb(double d) { return std::fabs(d - 3.0) < 1e-9 ? 10 * std::log10(2.0) : d; }
double wspolczynnikSpadku(double d) {
    const double m2 = std::fabs(d - 3.0) < 1e-9 ? 2.0 : std::pow(10.0, d / 10.0);
    return std::sqrt(m2 - 1);
}

std::optional<Wynik> analiza(const Rdzen& r, const Uzwojenia& u, const Zrodlo& z) {
    for (double v : {r.przekrojCm2, r.drogaCm, r.muPoczatkowa, r.bNasycenieT, r.stratyZelazaWkg, r.gestoscKgDm3, u.zwojePierwotne, u.zwojeWtorne, z.riOm, z.rgOm, z.mocW, z.spadekDb})
        if (!dodatnie(v)) return std::nullopt;
    for (double v : {u.rpOm, u.rwOm, u.lrH, u.cdF, u.szczelinaCelowaMm, z.pradStalyA, r.szczelinaStykuMm, r.szerSrodkowaCm, r.pakietCm})
        if (!std::isfinite(v) || v < 0) return std::nullopt;
    Wynik o;
    const double n = u.zwojePierwotne / u.zwojeWtorne;
    o.przekladnia = n;
    const double at = (z.przeciwsobny ? 0.5 : 1.0) * u.zwojePierwotne * z.pradStalyA;
    const PunktPracy p = punkt(r, u.zwojePierwotne, at, u.szczelinaCelowaMm);
    o.lpH = p.lpH; o.lpBezPolaryzacjiH = p.lpPoczH; o.muPrzyrostowa = p.muPrzyr; o.bStalaT = p.bT;
    o.szczelinaCalkowitaMm = u.szczelinaCelowaMm + r.szczelinaStykuMm;
    o.amperozwojeNaCm = at / r.drogaCm;
    const double masa = r.przekrojCm2 * r.drogaCm * 1e-3 * r.gestoscKgDm3;   // cm³ → dm³ · kg/dm³
    Charakt c{r, u, z, n, p.lpH, masa};

    // pasmo środkowe i krzywa
    const double fRef = 1000;
    const double hRef = std::abs(c.transmitancja(fRef));
    {
        const C za = c.zWezlaA(fRef);
        o.raWidzianeOm = (C(u.rpOm, 0) + za).real();
    }
    o.sprawnoscMiedz = n * n * z.rgOm / (n * n * (z.rgOm + u.rwOm) + u.rpOm);
    const double prog = -spadekDokladnyDb(z.spadekDb);
    const int N = 600;
    double fMin = 2, fMax = 3e6;
    std::vector<double> db(N + 1), ff(N + 1);
    for (int k = 0; k <= N; k++) {
        ff[k] = fMin * std::pow(fMax / fMin, static_cast<double>(k) / N);
        db[k] = 20 * std::log10(std::abs(c.transmitancja(ff[k])) / hRef);
        if (k % 6 == 0) o.krzywa.push_back({ff[k], db[k]});
    }
    auto przeciecie = [&](double f1, double f2) {   // bisekcja na przejściu przez prog
        auto g = [&](double f) { return 20 * std::log10(std::abs(c.transmitancja(f)) / hRef) - prog; };
        double a = f1, b = f2;
        for (int i = 0; i < 80; i++) { const double m = std::sqrt(a * b); ((g(m) > 0) == (g(a) > 0) ? a : b) = m; }
        return std::sqrt(a * b);
    };
    int kRef = 0;
    for (int k = 0; k <= N; k++) if (ff[k] <= fRef) kRef = k;
    for (int k = kRef; k > 0; k--) if (db[k - 1] < prog) { o.fdHz = przeciecie(ff[k - 1], ff[k]); break; }
    for (int k = kRef; k < N; k++) if (db[k + 1] < prog) { o.fgHz = przeciecie(ff[k], ff[k + 1]); break; }
    double mx = -1e9, fmx = 0;
    for (int k = kRef; k <= N; k++) if (db[k] > mx) { mx = db[k]; fmx = ff[k]; }
    if (mx > 0.5 && fmx > fRef * 1.5) { o.rezonansHz = fmx; o.nadmiarRezonansuDb = mx; }

    // indukcja zmienna przy fd (lub przy 20 Hz, gdy dolny kraniec nie został osiągnięty) i pełnej mocy
    const double fB = z.fMocyHz > 0 ? z.fMocyHz : (o.fdHz > 0 ? o.fdHz : 20.0);
    const double uWyj = n * std::sqrt(2 * z.mocW * z.rgOm);        // szczyt napięcia na obciążeniu sprowadzony do pierwotnej
    const double e = uWyj / (hRef * 1.0);                          // amplituda źródła dająca moc znamionową w paśmie środkowym
    const double vA = e * std::abs(c.wezlaDoE(fB));
    o.bZmiennaT = vA / (2 * kPi * fB * u.zwojePierwotne * r.przekrojCm2 * 1e-4);
    o.bSumaT = o.bStalaT + o.bZmiennaT;
    o.nasycenie = o.bSumaT > 0.9 * r.bNasycenieT;
    if (o.bSumaT < r.bNasycenieT) {
        const double mus = r.muPoczatkowa * std::pow(1 - o.bSumaT / r.bNasycenieT, 2);
        const double lg = p.lgCalkM;
        o.lpSzczytH = u.zwojePierwotne * u.zwojePierwotne / (r.drogaCm * 1e-2 / (kMi0 * mus * r.przekrojCm2 * 1e-4) + lg / (kMi0 * agM2(r, lg)));
    }
    // straty
    o.stratyZelazaW = r.stratyZelazaWkg * masa * std::pow(fB / 50.0, 1.5) * o.bZmiennaT * o.bZmiennaT;
    const double pWej = z.mocW / o.sprawnoscMiedz;
    o.sprawnoscCalkowita = z.mocW / (pWej + o.stratyZelazaW);
    o.spadekDcV = z.pradStalyA * u.rpOm;
    o.stratyDcW = z.pradStalyA * z.pradStalyA * u.rpOm * (z.przeciwsobny ? 0 : 1);
    return o;
}

std::optional<double> szczelinaDlaIndukcyjnosci(const Rdzen& r, double zwoje, const Zrodlo& z, double lWym) {
    if (!dodatnie(zwoje) || !dodatnie(lWym) || !dodatnie(r.przekrojCm2) || !dodatnie(r.drogaCm)) return std::nullopt;
    const double at = (z.przeciwsobny ? 0.5 : 1.0) * zwoje * z.pradStalyA;
    auto l = [&](double mm) { return punkt(r, zwoje, at, mm).lpH; };
    if (l(0) <= lWym) return 0.0;          // już bez szczeliny za mała indukcyjność
    double lo = 0, hi = 20;                // [mm]
    if (l(hi) > lWym) return std::nullopt;
    for (int k = 0; k < 100; k++) { const double m = 0.5 * (lo + hi); (l(m) > lWym ? lo : hi) = m; }
    return 0.5 * (lo + hi);
}

std::optional<double> zwojeDlaIndukcyjnosci(const Rdzen& r, double szczelinaMm, const Zrodlo& z, double lWym) {
    if (!dodatnie(lWym) || !dodatnie(r.przekrojCm2) || !dodatnie(r.drogaCm)) return std::nullopt;
    auto l = [&](double zw) { return punkt(r, zw, (z.przeciwsobny ? 0.5 : 1.0) * zw * z.pradStalyA, szczelinaMm).lpH; };
    double lo = 1, hi = 1e6;
    if (l(hi) < lWym) return std::nullopt;
    for (int k = 0; k < 100; k++) { const double m = 0.5 * (lo + hi); (l(m) > lWym ? hi : lo) = m; }
    return 0.5 * (lo + hi);
}

double pojemnoscWlasna(const std::vector<SekcjaPojemnosci>& s, double n, double izolPSMm, double wagaPS, double epsR) {
    double c = 0, zp = 0, zw = 0;
    for (const auto& x : s) (x.pierwotna ? zp : zw) += x.zwoje;
    for (size_t i = 0; i < s.size(); i++) {
        const auto& x = s[i];
        if (x.warstwy < 1 || !dodatnie(x.izolacjaWarstwMm)) continue;
        const double cw = kEps0 * epsR * (x.dlugoscZwojuCm * 1e-2) * (x.wysokoscCm * 1e-2) / (x.izolacjaWarstwMm * 1e-3);
        const double nl = x.warstwy;
        const double ceff = nl > 1 ? 4.0 / 3.0 * (nl - 1) / (nl * nl) * cw : 0;
        const double udzial = x.zwoje > 0 ? x.zwoje / (x.pierwotna ? zp : zw) : 1.0;   // sekcja niesie tylko część napięcia uzwojenia: C·udział²
        c += (x.pierwotna ? ceff : ceff / (n * n)) * udzial * udzial;   // wtórne: C·(V2/V1)²
    }
    if (wagaPS > 0 && dodatnie(izolPSMm)) {
        // pierwsza para sąsiadujących sekcji pierwotna–wtórna wyznacza powierzchnię
        for (size_t i = 0; i + 1 < s.size(); i++) if (s[i].pierwotna != s[i + 1].pierwotna) {
            const double a = kEps0 * epsR * (s[i].dlugoscZwojuCm * 1e-2) * (s[i].wysokoscCm * 1e-2) / (izolPSMm * 1e-3);
            c += wagaPS * a;
            break;
        }
    }
    return c;
}

double pojemnoscPP(const std::vector<SekcjaPP>& s, double z1, double z2, double n, double izolWarstwMm, double izolPSMm, double epsR) {
    if (!dodatnie(z1) || !dodatnie(z2) || !dodatnie(izolWarstwMm)) return 0;
    double c = 0;
    for (const auto& x : s) {
        if (x.warstwy < 1) continue;
        const double cw = kEps0 * epsR * (x.dlugoscZwojuCm * 1e-2) * (x.wysokoscCm * 1e-2) / (izolWarstwMm * 1e-3);
        const double nl = x.warstwy;
        const double ceff = nl > 1 ? 4.0 / 3.0 * (nl - 1) / (nl * nl) * cw : 0;
        const double udzial = x.zwoje / (x.pierwotna ? z1 : z2);
        c += (x.pierwotna ? ceff : ceff / (n * n)) * udzial * udzial;
    }
    if (!dodatnie(izolPSMm)) return c;
    auto potP = [&](double t) { return t / z1 - 0.5; };
    auto potS = [&](double t) { return t / z2; };
    for (size_t i = 0; i + 1 < s.size(); i++) {
        const auto &a = s[i], &b = s[i + 1];
        if (a.pierwotna == b.pierwotna || a.komora != b.komora) continue;
        const auto& p = a.pierwotna ? a : b;       // sekcja pierwotna
        const auto& w = a.pierwotna ? b : a;       // sekcja wtórna
        const bool pWewn = a.pierwotna;            // pierwotna bliżej rdzenia -> jej zewnętrzna (ostatnia) warstwa zwraca się do wtórnej
        const bool wWewn = !a.pierwotna;
        auto zakres = [&](const SekcjaPP& q, bool wewnetrzna, bool prim, double& t0, double& t1) {
            const double nl = wewnetrzna ? (q.zwojeOstatniejWarstwy > 0 ? q.zwojeOstatniejWarstwy : q.zwojeWWarstwie) : std::min(q.zwojeWWarstwie, q.zwoje);
            (void)prim;
            if (wewnetrzna) { t1 = q.zwojePrzed + q.zwoje; t0 = t1 - nl; }
            else { t0 = q.zwojePrzed; t1 = t0 + nl; }
        };
        double p0, p1, w0, w1;
        zakres(p, pWewn, true, p0, p1);
        zakres(w, wWewn, false, w0, w1);
        const double fa0 = potP(p0), fa1 = potP(p1), sa0 = potS(w0), sa1 = potS(w1);
        auto msq = [](double a2, double b2) { return (a2 * a2 + a2 * b2 + b2 * b2) / 3.0; };
        const double mSame = msq(fa0 - sa0, fa1 - sa1), mOpp = msq(fa0 - sa1, fa1 - sa0);
        const double area = kEps0 * epsR * (0.5 * (p.dlugoscZwojuCm + w.dlugoscZwojuCm) * 1e-2) * (p.wysokoscCm * 1e-2) / (izolPSMm * 1e-3);
        c += area * std::max(mSame, mOpp);
    }
    return c;
}

} // namespace TrafoModel
