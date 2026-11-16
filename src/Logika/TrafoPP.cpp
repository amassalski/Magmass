#include "TrafoPP.h"
#include "Rogowski.h"
#include "TrafoSieciowy.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <map>
#include <set>
#include <sstream>

namespace TrafoPP {

namespace {
const double kPi = 3.14159265358979323846;
bool ok(const Wymagania& w) {
    for (double v : {w.rObciazeniaOm, w.raaOm, w.mocW, w.fdHz, w.fgHz, w.riOm, w.i0Ma, w.bGs, w.mi, w.eta, w.alfaFe, w.betaOkna, w.sigma, w.spadekDb})
        if (!std::isfinite(v) || !(v > 0)) return false;
    return w.eta <= 1 && w.alfaFe <= 1 && w.betaOkna <= 1;
}
double przekladnia(const Wymagania& w) { return std::sqrt(w.raaOm * w.eta / w.rObciazeniaOm); }
}

std::optional<Pozadane> etap1(const Wymagania& w) {
    if (!ok(w)) return std::nullopt;
    Pozadane p;
    const double k = TrafoModel::wspolczynnikSpadku(w.spadekDb);
    p.liH = (w.raaOm * w.riOm / (w.raaOm + w.riOm)) / (2 * kPi * w.fdHz * k);
    p.lrH = k * (w.raaOm + w.riOm) / (2 * kPi * w.fgHz);
    p.przekladnia = przekladnia(w);
    p.iloczynCm4 = w.mocW * 1e6 / (4.44 * w.fdHz * w.alfaFe * w.bGs * w.betaOkna * w.sigma);
    p.przekrojCm2 = std::sqrt(p.iloczynCm4);
    return p;
}

std::optional<Uzwojenia> etap2(const Wymagania& w, const Rdzen& r) {
    const auto p = etap1(w);
    if (!p) return std::nullopt;
    for (double v : {r.przekrojCm2, r.drogaStrumieniaCm}) if (!std::isfinite(v) || !(v > 0)) return std::nullopt;
    Uzwojenia u;
    const double mi0 = 4e-7 * kPi;
    u.zwojePierwotne = std::sqrt(p->liH * r.drogaStrumieniaCm * 1e-2 / (mi0 * w.mi * r.przekrojCm2 * 1e-4 * w.alfaFe));
    u.zwojeWtorne = u.zwojePierwotne / p->przekladnia;
    u.pradWtornyA = std::sqrt(w.mocW / w.rObciazeniaOm);
    u.pradPierwotnyA = u.pradWtornyA / p->przekladnia + w.i0Ma * 1e-3;
    u.drutWtornyMm = std::sqrt(4 * u.pradWtornyA / (kPi * w.sigma));
    u.drutPierwotnyMm = std::sqrt(4 * u.pradPierwotnyA / (kPi * w.sigma));
    return u;
}

std::optional<Gitarowe> gitarowe(const Wymagania& w, const Rdzen& r, double mn, bool klasaA) {
    if (!ok(w) || !std::isfinite(mn) || !(mn > 1)) return std::nullopt;
    for (double v : {r.przekrojCm2, r.drogaStrumieniaCm}) if (!std::isfinite(v) || !(v > 0)) return std::nullopt;
    Gitarowe g;
    g.mn = mn;
    const double om = 2 * kPi * w.fdHz;
    g.l1H = w.riOm / (om * (1 + w.riOm / w.raaOm) * std::sqrt(mn * mn - 1));
    g.um1V = std::sqrt(2 * w.mocW * w.raaOm);
    g.objetoscCm3 = 0.4 * kPi * w.mi * g.um1V * g.um1V / (om * om * g.l1H * w.bGs * w.bGs) * 1e8;
    g.zwoje1 = 1e4 * std::sqrt(g.l1H * r.drogaStrumieniaCm / (0.4 * kPi * w.mi * r.przekrojCm2 * w.alfaFe));
    g.imaxA = std::sqrt(2 * w.mocW / w.raaOm);
    const double i0 = w.i0Ma * 1e-3;
    g.i1A = std::sqrt(i0 * i0 + (klasaA ? 0.125 : 0.25) * g.imaxA * g.imaxA);
    g.d1Mm = std::sqrt(4 * (g.i1A / w.sigma) / kPi);
    const double n = std::sqrt(w.rObciazeniaOm / (w.eta * w.raaOm));   // z2/z1
    g.zwoje2 = g.zwoje1 * n;
    g.d2Mm = g.d1Mm / std::sqrt(n);
    const double zl[3] = {4, 8, 16};
    for (int k = 0; k < 3; k++) {
        const double nk = std::sqrt(zl[k] / (w.eta * w.raaOm));
        g.zwoje2Zl[k] = g.zwoje1 * nk;
        g.d2ZlMm[k] = g.d1Mm / std::sqrt(nk);
    }
    return g;
}

namespace {
struct Odcinek {   // fragment przekroju okna: uzwojenie albo izolacja
    double gruboscM;
    double f0, f1;          ///< siłomotoryczna na początku i końcu odcinka (w jednostkach z1·I1)
    double xSrM;            ///< odległość środka od powierzchni rdzenia
    double hM;              ///< wysokość komory, w której leży odcinek
};
}

namespace {
std::optional<WynikWykonane> etap3Wewn(const Wymagania& w, const Rdzen& r, const Wykonane& u, bool porownanie) {
    const auto p = etap1(w);
    if (!p) return std::nullopt;
    for (double v : {r.przekrojCm2, r.drogaStrumieniaCm, r.dlugoscZwojuCm, r.wysokoscUzwojeniaCm, r.szerokoscOknaCm, u.drutPierwotnyMm,
                     u.drutWtornyMm, u.rezystywnosc})
        if (!std::isfinite(v) || !(v > 0)) return std::nullopt;
    if (!std::isfinite(u.kEfektyBrzegowe) || u.kEfektyBrzegowe < 0) return std::nullopt;
    if (u.zwojePierwotne < 2 || u.zwojeWtorne < 1 || u.przekladkaWarstwMm < 0) return std::nullopt;

    const double mi0 = 4e-7 * kPi;
    WynikWykonane o;
    o.liH = mi0 * w.mi * r.przekrojCm2 * 1e-4 * w.alfaFe * u.zwojePierwotne * u.zwojePierwotne / (r.drogaStrumieniaCm * 1e-2);
    o.raaOm = std::pow(static_cast<double>(u.zwojePierwotne) / u.zwojeWtorne, 2) * w.rObciazeniaOm / w.eta;

    const double hMm = r.wysokoscUzwojeniaCm * 10.0;
    const double dEm1 = TrafoSieciowy::drutWEmalii(u.drutPierwotnyMm), dEm2 = TrafoSieciowy::drutWEmalii(u.drutWtornyMm);
    const double gapMm = w.przekladkaCm * 10.0;                    // izolacja między uzwojeniami

    const bool tabela = !u.sekcjeWej.empty();
    int komor = 1;
    if (tabela) for (const SekcjaWej& s : u.sekcjeWej) komor = std::max(komor, s.komora + 1);
    if (u.przegrodaMm < 0 || u.sciankaKarkasuMm < 0 || u.marginesWarstwyMm < 0) return std::nullopt;
    const double hOknoMm = hMm - 2.0 * u.sciankaKarkasuMm;                  // wnętrze karkasu: między policzkami
    const double hKomMm = (hOknoMm - (komor - 1) * u.przegrodaMm) / komor;  // jedna komora (przegrody zabierają wysokość)
    const double hUzytMm = hKomMm - 2.0 * u.marginesWarstwyMm;              // długość warstwy: bez marginesów na brzegach
    if (!(hUzytMm > std::max(dEm1, dEm2))) return std::nullopt;

    auto sekcja = [&](bool pierwotna, double zwoje) {
        SekcjaUzwojenia s;
        s.pierwotna = pierwotna;
        s.zwoje = static_cast<int>(std::lround(zwoje));
        const double dEm = pierwotna ? dEm1 : dEm2;
        s.zwojeWWarstwie = std::max(1, static_cast<int>(std::floor(hUzytMm / dEm)));
        s.warstwy = (s.zwoje + s.zwojeWWarstwie - 1) / s.zwojeWWarstwie;
        s.zwojeOstatniejWarstwy = s.zwoje - s.zwojeWWarstwie * (s.warstwy - 1);
        s.gruboscMm = s.warstwy * (dEm + u.przekladkaWarstwMm);
        s.nieparzysteWarstwy = s.warstwy % 2 == 1;
        s.wysokoscKomoryMm = hUzytMm;   // wysokość uzwojenia (długość warstwy) w komorze
        return s;
    };
    std::vector<int> komoraSekcji;
    std::map<int, int> grZwoje, grLiczba;   // grupy wtórnych: zwoje jednej sekcji i liczba sekcji równoległych
    bool grupy = false;
    if (tabela) {
        int sumaA = 0, sumaW = 0, nW = 0;
        bool rowneW = true;
        int pierwszaW = 0, kolejnoscA = 0, bezGrupy = 0;
        std::set<int> uzyteKolejnosci;
        for (const SekcjaWej& s : u.sekcjeWej) {
            if (s.zwoje < 1 || s.komora < 0) return std::nullopt;
            if (s.pierwotna) { sumaA += s.zwoje; if (s.kolejnosc > 0) { kolejnoscA++; uzyteKolejnosci.insert(s.kolejnosc); } }
            else {
                sumaW += s.zwoje;
                if (nW++ == 0) pierwszaW = s.zwoje; else if (s.zwoje != pierwszaW) rowneW = false;
                if (s.grupa != 0) grupy = true; else bezGrupy++;
                if (s.grupa > 0) {
                    auto it = grZwoje.find(s.grupa);
                    if (it == grZwoje.end()) grZwoje[s.grupa] = s.zwoje; else if (it->second != s.zwoje) return std::nullopt;   // równolegle tylko równe sekcje
                    grLiczba[s.grupa]++;
                }
            }
        }
        if (sumaA != u.zwojePierwotne || nW == 0) return std::nullopt;
        if (kolejnoscA != 0) {   // kolejność elektryczna: wszystkie sekcje A, różne numery
            int nA = 0;
            for (const SekcjaWej& s : u.sekcjeWej) if (s.pierwotna) nA++;
            if (kolejnoscA != nA || static_cast<int>(uzyteKolejnosci.size()) != nA) return std::nullopt;
        }
        if (grupy) {
            int sumaGrup = 0;
            for (const auto& g : grZwoje) sumaGrup += g.second;
            if (bezGrupy != 0 || grZwoje.empty() || sumaGrup != u.zwojeWtorne) return std::nullopt;
        } else if (u.wtorneRownolegle ? (!rowneW || pierwszaW != u.zwojeWtorne) : (sumaW != u.zwojeWtorne)) return std::nullopt;
        double przedW = 0;
        for (const SekcjaWej& sw : u.sekcjeWej) {
            SekcjaUzwojenia s = sekcja(sw.pierwotna, sw.zwoje);
            s.komora = sw.komora;
            if (sw.pierwotna) {
                if (kolejnoscA > 0) { double p = 0; for (const SekcjaWej& t : u.sekcjeWej) if (t.pierwotna && t.kolejnosc < sw.kolejnosc) p += t.zwoje; s.zwojePrzed = p; }
                else { double p = 0; for (const SekcjaWej& t : u.sekcjeWej) { if (&t == &sw) break; if (t.pierwotna) p += t.zwoje; } s.zwojePrzed = p; }
            } else if (grupy) {
                double p = 0;
                if (sw.grupa > 0) for (const auto& g : grZwoje) if (g.first < sw.grupa) p += g.second;   // grupy połączone szeregowo w kolejności numerów
                s.zwojePrzed = p;
            } else {
                s.zwojePrzed = u.wtorneRownolegle ? 0.0 : przedW;
                przedW += sw.zwoje;
            }
            o.sekcje.push_back(s);
            komoraSekcji.push_back(sw.komora);
        }
    } else {
        if (u.pierwotnePodzielone) { o.sekcje = {sekcja(true, u.zwojePierwotne / 2.0), sekcja(false, u.zwojeWtorne), sekcja(true, u.zwojePierwotne / 2.0)}; }
        else { o.sekcje = {sekcja(true, u.zwojePierwotne), sekcja(false, u.zwojeWtorne)}; }
        o.sekcje[0].zwojePrzed = 0;
        if (o.sekcje.size() == 3) o.sekcje[2].zwojePrzed = o.sekcje[0].zwoje;
        komoraSekcji.assign(o.sekcje.size(), 0);
    }
    int nW = 0;
    for (const SekcjaUzwojenia& s : o.sekcje) if (!s.pierwotna) nW++;

    // położenie sekcji: od rdzenia (w każdej komorze od nowa), z izolacją gapMm między sekcjami
    std::vector<Odcinek> odc;
    double x = 0, F = 0;
    double sumaGrubosci = 0, maxGrubosc = 0, zajeteMm2 = 0;
    for (size_t k = 0; k < o.sekcje.size(); k++) {
        SekcjaUzwojenia& s = o.sekcje[k];
        const bool nowaKomora = k == 0 || komoraSekcji[k] != komoraSekcji[k - 1];
        if (nowaKomora && k > 0) {
            if (std::fabs(F) > 0.03) o.niezrownowazone = true;
            maxGrubosc = std::max(maxGrubosc, x);
            zajeteMm2 += x * hUzytMm;
            x = 0; F = 0;
        } else if (k > 0) {                                         // izolacja
            odc.push_back({gapMm * 1e-3, F, F, (x + gapMm / 2.0) * 1e-3, hUzytMm * 1e-3});
            x += gapMm;
        }
        double ulamek = s.pierwotna ? s.zwoje / static_cast<double>(u.zwojePierwotne) : 1.0;   // udział w amperozwojach całkowitych z1·I1
        if (!s.pierwotna && tabela) {
            if (grupy) {
                const int g = u.sekcjeWej[k].grupa;
                ulamek = g > 0 ? grZwoje[g] / static_cast<double>(u.zwojeWtorne) / grLiczba[g] : 0.0;   // nieczynna: bez prądu; w grupie prąd dzieli się równo
            } else ulamek = u.wtorneRownolegle ? 1.0 / nW : s.zwoje / static_cast<double>(u.zwojeWtorne);
        }
        const double F1 = s.pierwotna ? F + ulamek : F - ulamek;    // pierwotne podnosi, wtórne cofa siłomotoryczną
        odc.push_back({s.gruboscMm * 1e-3, F, F1, (x + s.gruboscMm / 2.0) * 1e-3, hUzytMm * 1e-3});
        s.dlugoscZwojuCm = r.dlugoscZwojuCm + 2.0 * kPi * (x + s.gruboscMm / 2.0) * 0.1;   // obwód rdzenia + 2π·odległość środka sekcji
        x += s.gruboscMm;
        F = F1;
    }
    if (std::fabs(F) > 0.03) o.niezrownowazone = true;
    maxGrubosc = std::max(maxGrubosc, x);
    zajeteMm2 += x * hUzytMm;
    sumaGrubosci = maxGrubosc;
    o.zajeteCm2 = zajeteMm2 * 0.01;
    const double cUzytCm = r.szerokoscOknaCm - u.sciankaKarkasuMm * 0.1;      // ścianka karkasu od strony rdzenia
    o.miejsceCm2 = cUzytCm * (hOknoMm - (komor - 1) * u.przegrodaMm) * 0.1;
    o.miesciSie = sumaGrubosci * 0.1 <= cUzytCm;

    // rezystancje: suma po sekcjach
    const double q1 = kPi * u.drutPierwotnyMm * u.drutPierwotnyMm / 4.0, q2 = kPi * u.drutWtornyMm * u.drutWtornyMm / 4.0;
    double gW = 0;                                                  // przewodność wtórnych przy połączeniu równoległym
    std::map<int, double> gGrupy;                                   // przewodność każdej grupy równoległej
    for (size_t k = 0; k < o.sekcje.size(); k++) {
        const SekcjaUzwojenia& s = o.sekcje[k];
        const double dlugoscM = s.zwoje * s.dlugoscZwojuCm * 0.01;
        const double rs = u.rezystywnosc * dlugoscM / (s.pierwotna ? q1 : q2);
        if (s.pierwotna) o.rpOm += rs;
        else if (grupy) { if (u.sekcjeWej[k].grupa > 0) gGrupy[u.sekcjeWej[k].grupa] += 1.0 / rs; }   // sekcje nieczynne nie liczą się do rezystancji
        else if (u.wtorneRownolegle && tabela) gW += 1.0 / rs;
        else o.rwOm += rs;
    }
    if (grupy) { for (const auto& g : gGrupy) if (g.second > 0) o.rwOm += 1.0 / g.second; }
    else if (u.wtorneRownolegle && tabela && gW > 0) o.rwOm = 1.0 / gW;
    // sprawność: straty w miedzi, w klasie B każda połowa pierwotnego przewodzi przez pół okresu (I1² Rp / 2)
    const double i2 = std::sqrt(w.mocW / w.rObciazeniaOm);
    const double i1 = i2 * u.zwojeWtorne / u.zwojePierwotne + w.i0Ma * 1e-3;
    o.sprawnosc = w.mocW / (w.mocW + i2 * i2 * o.rwOm + i1 * i1 * o.rpOm / 2.0);

    // rozproszenie: L = μ0·k·z1²/h · ∫ l(x)·F(x)² dx, F liniowo w uzwojeniach, stała w izolacji (jednowymiarowy model pola)
    // poprawka brzegowa: ręczna (>0) albo współczynnik Rogowskiego z zastępczej odległości uzwojeń δ i długości warstwy
    auto kEfekt = [&](const std::vector<Odcinek>& v) {
        if (u.kEfektyBrzegowe > 0) return u.kEfektyBrzegowe;
        double delta = 0;
        for (const Odcinek& d : v) delta += d.gruboscM * (d.f0 * d.f0 + d.f0 * d.f1 + d.f1 * d.f1) / 3.0;
        return wspolczynnikRogowskiego(delta, hUzytMm * 1e-3);
    };
    auto rozproszenie = [&](const std::vector<Odcinek>& v) {
        double s = 0;
        for (const Odcinek& d : v) {
            const double calka = d.gruboscM * (d.f0 * d.f0 + d.f0 * d.f1 + d.f1 * d.f1) / 3.0;
            s += (r.dlugoscZwojuCm * 0.01 + 2.0 * kPi * d.xSrM) * calka / d.hM;
        }
        return mi0 * kEfekt(v) * static_cast<double>(u.zwojePierwotne) * u.zwojePierwotne * s;
    };
    o.lrH = rozproszenie(odc);
    o.kUzyte = kEfekt(odc);
    if (porownanie) {   // oba układy porównawczo
        Wykonane a = u, b = u;
        a.sekcjeWej.clear(); b.sekcjeWej.clear(); a.wtorneRownolegle = b.wtorneRownolegle = false; a.przegrodaMm = b.przegrodaMm = 0;
        a.zwojeWtorne = b.zwojeWtorne = u.wtorneRownolegle ? u.zwojeWtorne : u.zwojeWtorne;
        a.pierwotnePodzielone = false; b.pierwotnePodzielone = true;
        for (const Wykonane* ptr : {&a, &b}) {
            const auto t = etap3Wewn(w, r, *ptr, false);
            (ptr->pierwotnePodzielone ? o.lrPrzeplatanyH : o.lrNieprzeplatanyH) = t ? t->lrH : 0;
        }
    }
    // porównanie z książką: L = 4π·k·z1²·l/(10⁹·h)·(δ + (Δ1+Δ2)/3), Δ1+Δ2 = szerokość okna a (jak w ECCLab), wymiary w cm
    o.lrKsiazkaH = 4.0 * kPi * (u.kEfektyBrzegowe > 0 ? u.kEfektyBrzegowe : 0.9) *   // wzór książkowy z własnym k (0,9 przy trybie automatycznym)
                    u.zwojePierwotne * u.zwojePierwotne * r.dlugoscZwojuCm /
                   (1e9 * r.wysokoscUzwojeniaCm) * (w.przekladkaCm + r.szerokoscOknaCm / 3.0);
    o.rwEcclabOm = u.rezystywnosc * u.zwojeWtorne * (r.dlugoscZwojuCm + 1.4 * kPi * r.szerokoscOknaCm) * 0.01 / q2;
    return o;
}
}   // namespace

std::optional<WynikWykonane> etap3(const Wymagania& w, const Rdzen& r, const Wykonane& u) { return etap3Wewn(w, r, u, true); }

std::vector<SekcjaWej> presetSekcji(int numer, int z1, int z2) {
    auto A = [](int z, int k = 0) { return SekcjaWej{true, z, k}; };
    auto W = [](int z, int k = 0) { return SekcjaWej{false, z, k}; };
    std::vector<SekcjaWej> v;
    switch (numer) {
    case 1: v = {A(z1 / 2), W(z2), A(z1 - z1 / 2)}; break;
    case 2: {
        const int a1 = static_cast<int>(std::lround(0.4 * z1)), a2 = static_cast<int>(std::lround(0.2 * z1)), w1 = z2 / 2;
        v = {A(a1), W(w1), A(a2), W(z2 - w1), A(z1 - a1 - a2)};
        break;
    }
    case 3: v = {A(z1 / 2), W(z2 / 2), A(z1 - z1 / 2, 1), W(z2 - z2 / 2, 1)}; break;
    case 4: {
        const int q = z1 / 4, h = z1 / 2;
        v = {A(q), W(z2 / 2), A(h - q), A(q, 1), W(z2 - z2 / 2, 1), A(z1 - h - q, 1)};
        break;
    }
    case 5: case 6: case 7: {   // Mesa Boogie DR: 10 warstw od rdzenia, wtórne 2∥5∥8 (4 Ω), 3∥9 (+8 Ω), 6 (+16 Ω)
        // pierwotne 318/530/530/318 z 1696 zwojów, połączone elektrycznie 1-7-(środek)-4-10; wtórne w stosunku 78 : 33 : 78
        const int a1 = static_cast<int>(std::lround(z1 * 318.0 / 1696.0)), a2 = static_cast<int>(std::lround(z1 * 530.0 / 1696.0));
        const int a7 = a2, a10 = z1 - a1 - 2 * a2;       // warstwa 7 = 530/1696, warstwa 10 = reszta
        const int b = numer == 5 ? z2 : numer == 6 ? static_cast<int>(std::lround(z2 * 78.0 / 111.0)) : static_cast<int>(std::lround(z2 / 2.0));
        // dopinka czynna dopełnia odczep do z2 (suma grup = z2); dopinka nieczynna ma wymiar z rysunku w tej samej skali
        const int c = numer == 6 ? std::max(1, z2 - b) : std::max(1, static_cast<int>(std::lround(b * 33.0 / 78.0)));
        const int d = numer == 7 ? std::max(1, z2 - b) : b;
        // grupy: 1 = sekcje 4 Ω (2, 5, 8); 2 = dopinka 8 Ω (3, 9) albo dopinka 16 Ω (6); reszta nieczynna
        const int gC = numer == 6 ? 2 : -1, gD = numer == 7 ? 2 : -1;
        auto Wg = [](int z, int g) { SekcjaWej s{false, z, 0}; s.grupa = g; return s; };
        auto Ae = [](int z, int e) { SekcjaWej s{true, z, 0}; s.kolejnosc = e; return s; };
        v = {Ae(a1, 1), Wg(b, 1), Wg(c, gC), Ae(a2, 3), Wg(b, 1), Wg(d, gD), Ae(a7, 2), Wg(b, 1), Wg(c, gC), Ae(a10, 4)};
        break;
    }
    default: v = {A(z1), W(z2)}; break;
    }
    return v;
}

int zwojeWtorneZSekcji(const std::vector<SekcjaWej>& sekcje, bool rownolegle) {
    int suma = 0, pierwsza = 0, n = 0;
    std::map<int, int> grupy;
    bool czyGrupy = false;
    for (const SekcjaWej& s : sekcje) {
        if (s.pierwotna) continue;
        if (s.grupa != 0) czyGrupy = true;
        if (s.grupa > 0 && !grupy.count(s.grupa)) grupy[s.grupa] = s.zwoje;
        suma += s.zwoje;
        if (n++ == 0) pierwsza = s.zwoje;
    }
    if (czyGrupy) { int g = 0; for (const auto& e : grupy) g += e.second; return g; }
    return rownolegle ? pierwsza : suma;
}

bool parsujSekcje(const std::string& tekst, std::vector<SekcjaWej>& wynik, std::string& blad) {
    wynik.clear();
    blad.clear();
    int komora = 0;
    std::string t = tekst;
    for (char& ch : t) if (ch == ';' || ch == '\r') ch = '\n';
    std::istringstream in(t);
    std::string wiersz;
    int nr = 0;
    while (std::getline(in, wiersz)) {
        nr++;
        size_t a = wiersz.find_first_not_of(" \t");
        if (a == std::string::npos) continue;
        wiersz = wiersz.substr(a);
        if (wiersz[0] == '#') continue;
        if (wiersz[0] == '|' || wiersz.rfind("---", 0) == 0) {
            if (wynik.empty() || (nr > 0 && wynik.back().komora != komora)) { blad = "Pusta komora przed przegrodą w wierszu " + std::to_string(nr); return false; }
            komora++;
            continue;
        }
        const char litera = static_cast<char>(std::toupper(static_cast<unsigned char>(wiersz[0])));
        bool pierwotna;
        if (litera == 'A' || litera == 'P') pierwotna = true;
        else if (litera == 'W' || litera == 'S') pierwotna = false;
        else { blad = "Wiersz " + std::to_string(nr) + ": pierwsza litera to A (pierwotne) lub W (wtórne)"; return false; }
        size_t p = 1;
        while (p < wiersz.size() && std::isdigit(static_cast<unsigned char>(wiersz[p]))) p++;      // numer sekcji (opcjonalny)
        while (p < wiersz.size() && (wiersz[p] == ' ' || wiersz[p] == '\t' || wiersz[p] == ':' || wiersz[p] == '=')) p++;
        if (p >= wiersz.size()) { blad = "Wiersz " + std::to_string(nr) + ": brak liczby zwojów"; return false; }
        size_t k = p;
        while (k < wiersz.size() && std::isdigit(static_cast<unsigned char>(wiersz[k]))) k++;
        if (k == p) { blad = "Wiersz " + std::to_string(nr) + ": liczba zwojów musi być całkowita"; return false; }
        const long z = std::stol(wiersz.substr(p, k - p));
        if (z < 1 || z > 1000000) { blad = "Wiersz " + std::to_string(nr) + ": liczba zwojów poza zakresem"; return false; }
        SekcjaWej sek{pierwotna, static_cast<int>(z), komora};
        // opcjonalne dopiski: g<N> (grupa wtórnych), x (sekcja nieczynna), e<N> (kolejność elektryczna sekcji A)
        std::istringstream reszta(wiersz.substr(k));
        std::string tok;
        while (reszta >> tok) {
            const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(tok[0])));
            const std::string cyfry = tok.substr(1);
            const bool liczba = !cyfry.empty() && cyfry.size() < 6 && cyfry.find_first_not_of("0123456789") == std::string::npos && std::stol(cyfry) > 0;
            if (!pierwotna && c == 'g' && liczba) sek.grupa = static_cast<int>(std::stol(cyfry));
            else if (!pierwotna && c == 'x' && tok.size() == 1) sek.grupa = -1;
            else if (pierwotna && c == 'e' && liczba) sek.kolejnosc = static_cast<int>(std::stol(cyfry));
            else { blad = "Wiersz " + std::to_string(nr) + ": po liczbie zwojów dozwolone tylko g<N> (grupa wtórnych), x (nieczynna) albo e<N> (kolejność sekcji A)"; return false; }
        }
        wynik.push_back(sek);
    }
    if (wynik.empty()) { blad = "Brak sekcji"; return false; }
    if (wynik.back().komora != komora) { blad = "Pusta ostatnia komora"; return false; }
    return true;
}

std::string formatujSekcje(const std::vector<SekcjaWej>& v) {
    std::string t;
    int a = 0, w = 0, komora = 0;
    for (const SekcjaWej& s : v) {
        if (s.komora != komora) { t += "|\n"; komora = s.komora; }
        t += (s.pierwotna ? "A" + std::to_string(++a) : "W" + std::to_string(++w)) + " " + std::to_string(s.zwoje);
        if (!s.pierwotna && s.grupa > 0) t += " g" + std::to_string(s.grupa);
        if (!s.pierwotna && s.grupa < 0) t += " x";
        if (s.pierwotna && s.kolejnosc > 0) t += " e" + std::to_string(s.kolejnosc);
        t += "\n";
    }
    return t;
}

std::optional<TrafoModel::Wynik> analizaDokladna(const Wymagania& w, const Rdzen& r, const Wykonane& u, const WynikWykonane& o, const TrafoModel::Parametry& p) {
    TrafoModel::Rdzen rd;
    rd.przekrojCm2 = r.przekrojCm2 * w.alfaFe; rd.drogaCm = r.drogaStrumieniaCm;
    rd.muPoczatkowa = p.muPoczatkowa; rd.bNasycenieT = p.bNasycenieT; rd.szczelinaStykuMm = p.szczelinaStykuMm; rd.stratyZelazaWkg = p.stratyZelazaWkg;
    TrafoModel::Uzwojenia x;
    x.zwojePierwotne = u.zwojePierwotne; x.zwojeWtorne = u.zwojeWtorne; x.rpOm = o.rpOm; x.rwOm = o.rwOm; x.lrH = o.lrH;
    std::vector<TrafoModel::SekcjaPP> sp;
    const double emalia1 = TrafoSieciowy::drutWEmalii(u.drutPierwotnyMm) - u.drutPierwotnyMm;
    for (const auto& s : o.sekcje) {
        TrafoModel::SekcjaPP q;
        q.pierwotna = s.pierwotna; q.zwoje = s.zwoje; q.zwojePrzed = s.zwojePrzed; q.warstwy = s.warstwy; q.zwojeWWarstwie = s.zwojeWWarstwie;
        q.zwojeOstatniejWarstwy = s.zwojeOstatniejWarstwy; q.dlugoscZwojuCm = s.dlugoscZwojuCm;
        q.wysokoscCm = (s.wysokoscKomoryMm > 0 ? s.wysokoscKomoryMm : r.wysokoscUzwojeniaCm * 10.0) * 0.1; q.komora = s.komora;
        sp.push_back(q);
    }
    x.cdF = TrafoModel::pojemnoscPP(sp, u.zwojePierwotne, u.zwojeWtorne, static_cast<double>(u.zwojePierwotne) / u.zwojeWtorne,
                                    u.przekladkaWarstwMm + emalia1, w.przekladkaCm * 10.0, p.epsR);
    TrafoModel::Zrodlo z;
    z.przeciwsobny = true; z.riOm = w.riOm; z.rgOm = w.rObciazeniaOm; z.mocW = w.mocW; z.pradStalyA = p.asymetriaPPA; z.spadekDb = w.spadekDb; z.fMocyHz = w.fdHz;
    return TrafoModel::analiza(rd, x, z);
}

} // namespace TrafoPP
