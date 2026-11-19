#include "TrafoSE.h"
#include "TrafoSieciowy.h"
#include "Rogowski.h"
#include <algorithm>
#include <cmath>

namespace TrafoSE {

std::optional<Wynik> oblicz(const Wejscie& w) {
    if (w.mocVA <= 0 || w.raOm <= 0 || w.riOm <= 0 || w.iaMa <= 0 || w.fdHz <= 0 || w.rgOm <= 0 || w.eta <= 0 || w.eta > 1 ||
        w.drogaCm <= 0 || w.przekrojCm2 <= 0 || w.bMaxT <= 0 || w.mi < 1 || w.gestoscAmm2 <= 0 || !(w.spadekDb > 0))
        return std::nullopt;
    const double pi = 3.14159265358979323846, mu0 = 4e-7 * pi;
    Wynik r;
    const double ia = w.iaMa / 1000.0;
    r.rownolegleOm = w.raOm * w.riOm / (w.raOm + w.riOm);
    const double kSp = TrafoModel::wspolczynnikSpadku(w.spadekDb);
    r.liH = r.rownolegleOm / (2 * pi * w.fdHz * kSp);
    r.liPorownanieH = r.rownolegleOm / (7.2 * w.fdHz);
    r.przekladnia = std::sqrt(w.eta * w.raOm / w.rgOm);
    r.przekladniaPorownanie = std::sqrt(w.raOm / (w.eta * w.rgOm));
    r.przekrojOrientacyjnyCm2 = 2.5 * std::sqrt(w.mocVA);

    // pierwotne: moc P/η rozwija się na Ra
    const double iacRms = std::sqrt(w.mocVA / (w.eta * w.raOm));
    const double iacSzczyt = std::sqrt(2.0) * iacRms;
    r.pradZmiennyPierwotnyMa = iacSzczyt * 1000.0;
    r.pradSzczytowyMa = (ia + iacSzczyt) * 1000.0;
    r.mocMaxKlasaAVA = ia * ia * w.raOm / 2.0 * w.eta;
    r.przesterowanie = iacSzczyt > ia;
    r.pradPierwotnyRmsA = std::sqrt(ia * ia + iacRms * iacRms);
    r.pradWtornyA = std::sqrt(w.mocVA / w.rgOm);

    // zwoje: strumień sprzężony Λ = L·Iszczyt = z·B·S; w rdzeniu ze szczeliną B = L·I/(z·S)
    const double S = w.przekrojCm2 * 1e-4;
    const double lambda = r.liH * r.pradSzczytowyMa / 1000.0;
    r.zwojePierwotne = static_cast<int>(std::lround(lambda / (w.bMaxT * S)));
    if (r.zwojePierwotne < 1) return std::nullopt;
    r.zwojeWtorne = static_cast<int>(std::lround(r.zwojePierwotne / r.przekladnia));
    r.drutPierwotnyMm = std::sqrt(4.0 * r.pradPierwotnyRmsA / (pi * w.gestoscAmm2));
    r.drutWtornyMm = std::sqrt(4.0 * r.pradWtornyA / (pi * w.gestoscAmm2));

    // szczelina: L = μ0·S·z²/(lg + lFe/μ)
    const double z = r.zwojePierwotne;
    const double lgCalk = mu0 * S * z * z / r.liH - (w.drogaCm * 1e-2) / w.mi;
    r.brakSzczeliny = lgCalk <= 0;
    r.szczelinaMm = r.brakSzczeliny ? 0 : lgCalk * 1000.0;
    r.amperozwojeNaCm = z * ia / w.drogaCm;
    const double lEff = mu0 * S * z * z / r.liH;   // zastępcza droga strumienia
    r.indukcjaStalaT = mu0 * z * ia / lEff;
    r.indukcjaSzczytowaT = mu0 * z * (r.pradSzczytowyMa / 1000.0) / lEff;
    return r;
}

Literatura literatura(const Wejscie& w, const Wynik& p) {
    Literatura l;
    const double L1 = p.liH, I0 = w.iaMa, Q = w.przekrojCm2, ls = w.drogaCm;
    l.krizeZwoje23 = 10.0 * L1 * I0 / Q * (1.0 + std::sqrt(1.0 + 3000.0 * Q * ls / (L1 * I0 * I0)));
    l.krizeZwoje24 = 45.0 * L1 * I0 / Q;
    l.krizeN = std::sqrt(w.rgOm / w.raOm);
    l.krizeZwojeWtorne = l.krizeN * l.krizeZwoje23;
    l.krizeSzczelinaMm = l.krizeZwoje23 * I0 / 800000.0;
    l.krizePrzekladkaMm = l.krizeSzczelinaMm / 2.0;
    l.krizeR1Om = w.raOm / 2.0 * (1.0 - w.eta);
    if (w.dlugoscZwojuCm > 0 && l.krizeR1Om > 0) l.krizeDrutPierwotnyMm = 0.015 * std::sqrt(w.dlugoscZwojuCm * l.krizeZwoje23 / l.krizeR1Om);
    l.krizeDrutMinMm = 0.025 * std::sqrt(I0);
    if (l.krizeDrutPierwotnyMm > 0) l.krizeDrutWtornyMm = l.krizeDrutPierwotnyMm / std::sqrt(l.krizeN);
    l.krizeIloczynWymaganyCm4 = w.wspolczynnikAKrize * w.mocVA;
    if (w.wysokoscOknaCm > 0 && w.szerokoscOknaCm > 0) l.krizeIloczynRdzeniaCm4 = w.wysokoscOknaCm * w.szerokoscOknaCm * Q;
    l.witortQMinCm2 = 3.0 * std::sqrt(w.mocVA);
    l.witortQMaxCm2 = 5.0 * std::sqrt(w.mocVA);
    l.witortZwoje = 9000.0 * std::sqrt(L1 * ls / (500.0 * Q));
    l.witortL1H = w.raOm / (6.28 * w.fdHz);
    const double zob = w.rgOm;
    l.witortR1Om = zob * p.przekladnia * p.przekladnia * (1.0 - w.eta) / 2.0;
    l.witortR2Om = zob * (1.0 - w.eta) / (2.0 * w.eta);
    return l;
}

namespace {
struct Odcinek { double gruboscM, f0, f1, xSrM; };
}

std::optional<WynikWykonane> wykonane(const Wejscie& w, const Wykonane& u) {
    const auto p = oblicz(w);
    if (!p) return std::nullopt;
    for (double v : {w.dlugoscZwojuCm, w.wysokoscOknaCm, w.szerokoscOknaCm, u.drutPierwotnyMm, u.drutWtornyMm, u.rezystywnosc})
        if (!std::isfinite(v) || !(v > 0)) return std::nullopt;
    if (!std::isfinite(u.kEfektyBrzegowe) || u.kEfektyBrzegowe < 0) return std::nullopt;
    if (u.zwojePierwotne < 2 || u.zwojeWtorne < 2 || u.przekladkaWarstwMm < 0 || u.izolacjaUzwojenMm < 0 || u.szczelinaMm < 0 ||
        u.sciankaKarkasuMm < 0 || u.marginesWarstwyMm < 0) return std::nullopt;
    const double pi = 3.14159265358979323846, mu0 = 4e-7 * pi;
    WynikWykonane o;
    const double S = w.przekrojCm2 * 1e-4, ia = w.iaMa / 1000.0;
    const double lEff = u.szczelinaMm * 1e-3 + (w.drogaCm * 1e-2) / w.mi;    // zastępcza droga strumienia
    o.liH = mu0 * S * double(u.zwojePierwotne) * u.zwojePierwotne / lEff;
    o.indukcjaStalaT = mu0 * u.zwojePierwotne * ia / lEff;
    o.indukcjaSzczytowaT = mu0 * u.zwojePierwotne * (p->pradSzczytowyMa / 1000.0) / lEff;
    o.przekladnia = double(u.zwojePierwotne) / u.zwojeWtorne;

    const double hOknoMm = w.wysokoscOknaCm * 10.0 - 2.0 * u.sciankaKarkasuMm;   // wnętrze karkasu
    const double hMm = hOknoMm - 2.0 * u.marginesWarstwyMm;                       // długość warstwy
    const double cUzytCm = w.szerokoscOknaCm - u.sciankaKarkasuMm * 0.1;          // ścianka od strony rdzenia
    if (!(hMm > 0) || !(cUzytCm > 0)) return std::nullopt;
    const double dEm1 = TrafoSieciowy::drutWEmalii(u.drutPierwotnyMm), dEm2 = TrafoSieciowy::drutWEmalii(u.drutWtornyMm);
    auto sekcja = [&](bool pierwotna, int zwoje) {
        SekcjaUzwojenia s;
        s.pierwotna = pierwotna;
        s.zwoje = zwoje;
        const double dEm = pierwotna ? dEm1 : dEm2;
        s.zwojeWWarstwie = std::max(1, static_cast<int>(std::floor(hMm / dEm)));
        s.warstwy = (s.zwoje + s.zwojeWWarstwie - 1) / s.zwojeWWarstwie;
        s.gruboscMm = s.warstwy * (dEm + u.przekladkaWarstwMm);
        s.nieparzysteWarstwy = s.warstwy % 2 == 1;
        return s;
    };
    struct El { bool pierwotna; int zwoje; };
    auto uklad = [&](Wykonane::Uklad k) {
        const int zp = u.zwojePierwotne, zw = u.zwojeWtorne;
        switch (k) {
            case Wykonane::Uklad::pierwotnePodzielone: return std::vector<El>{{true, zp / 2}, {false, zw}, {true, zp - zp / 2}};
            case Wykonane::Uklad::wtornePodzielone: return std::vector<El>{{false, zw / 2}, {true, zp}, {false, zw - zw / 2}};
            default: return std::vector<El>{{true, zp}, {false, zw}};
        }
    };
    // jedna funkcja: układ -> sekcje, geometria, rozproszenie
    struct Skladnik { std::vector<SekcjaUzwojenia> sek; double lr = 0, suma = 0, k = 1; };
    auto policz = [&](Wykonane::Uklad k) {
        Skladnik r;
        std::vector<Odcinek> odc;
        double x = 0, F = 0;
        for (const El& e : uklad(k)) {
            SekcjaUzwojenia s = sekcja(e.pierwotna, e.zwoje);
            if (!r.sek.empty()) {
                odc.push_back({u.izolacjaUzwojenMm * 1e-3, F, F, (x + u.izolacjaUzwojenMm / 2.0) * 1e-3});
                x += u.izolacjaUzwojenMm;
                r.suma += u.izolacjaUzwojenMm;
            }
            const double ulamek = e.pierwotna ? double(e.zwoje) / u.zwojePierwotne : double(e.zwoje) / u.zwojeWtorne;
            const double F1 = e.pierwotna ? F + ulamek : F - ulamek;
            odc.push_back({s.gruboscMm * 1e-3, F, F1, (x + s.gruboscMm / 2.0) * 1e-3});
            s.dlugoscZwojuCm = w.dlugoscZwojuCm + 2.0 * pi * (u.sciankaKarkasuMm + x + s.gruboscMm / 2.0) * 0.1;
            x += s.gruboscMm;
            r.suma += s.gruboscMm;
            F = F1;
            r.sek.push_back(s);
        }
        double c = 0, delta = 0;
        for (const Odcinek& d : odc) delta += d.gruboscM * (d.f0 * d.f0 + d.f0 * d.f1 + d.f1 * d.f1) / 3.0;
        r.k = u.kEfektyBrzegowe > 0 ? u.kEfektyBrzegowe : wspolczynnikRogowskiego(delta, hMm * 1e-3);
        for (const Odcinek& d : odc) c += (w.dlugoscZwojuCm * 0.01 + 2.0 * pi * d.xSrM) * d.gruboscM * (d.f0 * d.f0 + d.f0 * d.f1 + d.f1 * d.f1) / 3.0;
        r.lr = mu0 * r.k * double(u.zwojePierwotne) * u.zwojePierwotne / (hMm * 1e-3) * c;
        return r;
    };
    const Wykonane::Uklad uk[3] = {Wykonane::Uklad::nieprzeplatane, Wykonane::Uklad::pierwotnePodzielone, Wykonane::Uklad::wtornePodzielone};
    Skladnik wybrany;
    for (int k = 0; k < 3; k++) {
        const Skladnik t = policz(uk[k]);
        o.lrUkladH[k] = t.lr;
        if (uk[k] == u.uklad) wybrany = t;
    }
    o.sekcje = wybrany.sek;
    o.lrH = wybrany.lr;
    o.kUzyte = wybrany.k;
    o.zajeteCm2 = wybrany.suma * 0.1 * hMm * 0.1;
    o.miejsceCm2 = cUzytCm * hOknoMm * 0.1;
    o.miesciSie = wybrany.suma * 0.1 <= cUzytCm;

    const double q1 = pi * u.drutPierwotnyMm * u.drutPierwotnyMm / 4.0, q2 = pi * u.drutWtornyMm * u.drutWtornyMm / 4.0;
    for (const SekcjaUzwojenia& s : o.sekcje) {
        const double dl = s.zwoje * s.dlugoscZwojuCm * 0.01;
        (s.pierwotna ? o.rpOm : o.rwOm) += u.rezystywnosc * dl / (s.pierwotna ? q1 : q2);
    }
    o.spadekStalyV = ia * o.rpOm;
    o.stratyStaleW = ia * ia * o.rpOm;
    const double n = o.przekladnia;
    o.raWidzianeOm = o.rpOm + n * n * (w.rgOm + o.rwOm);
    const double rEkw = w.riOm * o.raWidzianeOm / (w.riOm + o.raWidzianeOm);
    const double kSp = TrafoModel::wspolczynnikSpadku(w.spadekDb);
    o.fdWykonanaHz = rEkw / (2 * pi * o.liH * kSp);
    o.fgWykonanaHz = kSp * (w.riOm + o.raWidzianeOm) / (2 * pi * o.lrH);
    o.lrMaxH = kSp * (w.riOm + w.raOm) / (2 * pi * u.fgHz);
    const double i2 = p->pradWtornyA, i1ac = i2 / n;
    o.sprawnosc = w.mocVA / (w.mocVA + i1ac * i1ac * o.rpOm + i2 * i2 * o.rwOm);
    o.szczelinaUjemna = false;
    o.lrKsiazkaH = 4.0 * pi * (u.kEfektyBrzegowe > 0 ? u.kEfektyBrzegowe : 0.9) * double(u.zwojePierwotne) * u.zwojePierwotne * w.dlugoscZwojuCm /
                   (1e9 * w.wysokoscOknaCm) * (u.izolacjaUzwojenMm * 0.1 + w.szerokoscOknaCm / 3.0);
    return o;
}

std::optional<TrafoModel::Wynik> analizaDokladna(const Wejscie& w, const Wykonane& u, const WynikWykonane& o, const TrafoModel::Parametry& p) {
    TrafoModel::Rdzen r;
    r.przekrojCm2 = w.przekrojCm2; r.drogaCm = w.drogaCm;
    r.muPoczatkowa = p.muPoczatkowa; r.bNasycenieT = p.bNasycenieT; r.szczelinaStykuMm = p.szczelinaStykuMm; r.stratyZelazaWkg = p.stratyZelazaWkg;
    TrafoModel::Uzwojenia x;
    x.zwojePierwotne = u.zwojePierwotne; x.zwojeWtorne = u.zwojeWtorne; x.rpOm = o.rpOm; x.rwOm = o.rwOm; x.lrH = o.lrH;
    x.szczelinaCelowaMm = u.szczelinaMm;
    std::vector<TrafoModel::SekcjaPojemnosci> sp;
    const double emalia1 = TrafoSieciowy::drutWEmalii(u.drutPierwotnyMm) - u.drutPierwotnyMm;
    for (const auto& s : o.sekcje) {
        TrafoModel::SekcjaPojemnosci q;
        q.pierwotna = s.pierwotna; q.warstwy = s.warstwy; q.zwoje = s.zwoje; q.dlugoscZwojuCm = s.dlugoscZwojuCm; q.wysokoscCm = (w.wysokoscOknaCm * 10.0 - 2.0 * u.sciankaKarkasuMm - 2.0 * u.marginesWarstwyMm) * 0.1;
        q.izolacjaWarstwMm = u.przekladkaWarstwMm + emalia1;
        sp.push_back(q);
    }
    x.cdF = TrafoModel::pojemnoscWlasna(sp, o.przekladnia, u.izolacjaUzwojenMm, u.uklad == Wykonane::Uklad::nieprzeplatane ? 1.0 : (u.uklad == Wykonane::Uklad::pierwotnePodzielone ? 0.5 : 2.0), p.epsR);
    TrafoModel::Zrodlo z;
    z.przeciwsobny = false; z.riOm = w.riOm; z.rgOm = w.rgOm; z.mocW = w.mocVA; z.pradStalyA = w.iaMa * 1e-3; z.spadekDb = w.spadekDb; z.fMocyHz = w.fdHz;
    return TrafoModel::analiza(r, x, z);
}

} // namespace TrafoSE
