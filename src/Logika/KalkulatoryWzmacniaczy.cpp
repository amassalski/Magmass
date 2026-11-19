#include "KalkulatoryWzmacniaczy.h"

#include "Kalkulatory.h"

#include <algorithm>
#include <cmath>

namespace KalkulatoryWzmacniaczy {

namespace {
constexpr double kPi = 3.14159265358979323846;
using Zesp = std::complex<double>;
}

// ---------------------------------------------------------------- polaryzacja katodowa

double fKatodyHz(double rkOm, double ckF, double gmSiemens) {
    if (!(rkOm > 0 && ckF > 0 && gmSiemens > 0)) return 0;
    const double K = gmSiemens * rkOm;
    const double x2 = (1 + K) * (1 + K) - 2;
    if (x2 <= 0) return 0;
    return std::sqrt(x2) / (2 * kPi * rkOm * ckF);
}

std::optional<WynikKatody> polaryzacjaKatodowa(const WejscieKatody& w) {
    if (w.iaMA < 0 || w.ig2MA < 0 || w.ukV <= 0 || w.gmMAV < 0 || w.fdHz < 0) return std::nullopt;
    WynikKatody r;
    r.ikMA = w.iaMA + w.ig2MA;
    if (!(r.ikMA > 0)) return std::nullopt;
    r.rkIdealny = w.ukV / (r.ikMA * 1e-3);
    r.rkSzereg = Kalkulatory::najblizsza(r.rkIdealny, w.szeregR);
    r.ukRzeczywiste = r.ikMA * 1e-3 * r.rkSzereg;
    r.mocMW = r.ukRzeczywiste * r.ikMA;   // V · mA = mW
    r.mocZalecanaMW = 2 * r.mocMW;
    r.napiecieCkV = std::max(16.0, 2 * r.ukRzeczywiste);
    if (w.fdHz > 0) {
        r.maCk = true;
        r.ckReguleUF = 10.0 / (2 * kPi * w.fdHz * r.rkSzereg) * 1e6;
        if (w.gmMAV > 0) {
            const double K = w.gmMAV * 1e-3 * r.rkSzereg;
            const double x2 = (1 + K) * (1 + K) - 2;
            if (x2 <= 0) {
                r.ckPotrzebny = false;
            } else {
                r.ckUF = std::sqrt(x2) / (2 * kPi * r.rkSzereg * w.fdHz) * 1e6;
                r.ckSzeregUF = Kalkulatory::najblizszaWieksza(r.ckUF, w.szeregC);
            }
        } else {
            r.ckUF = r.ckReguleUF;                                   // bez gm: tylko reguła praktyczna
            r.ckSzeregUF = Kalkulatory::najblizszaWieksza(r.ckUF, w.szeregC);
        }
    }
    return r;
}

// ---------------------------------------------------------------- kondensator sprzęgający

std::optional<WynikSprzezenia> kondensatorSprzegajacy(const WejscieSprzezenia& w) {
    if (w.raKOhm <= 0 || w.rgKOhm <= 0 || w.rwewKOhm < 0 || w.csNF < 0 || w.fdHz < 0 || w.sekcje < 1 || w.sekcje > 8)
        return std::nullopt;
    WynikSprzezenia r;
    const double rwyj = w.rwewKOhm > 0 ? w.raKOhm * w.rwewKOhm / (w.raKOhm + w.rwewKOhm) : w.raKOhm;
    r.rcalkKOhm = rwyj + w.rgKOhm;
    const double wspolczynnik = std::sqrt(std::pow(2.0, 1.0 / w.sekcje) - 1.0);   // f_całk = f_sekcji / wspolczynnik
    if (w.csNF > 0) {
        r.maF = true;
        r.fSekcjiHz = 1.0 / (2 * kPi * r.rcalkKOhm * 1e3 * w.csNF * 1e-9);
        r.fCalkowitaHz = r.fSekcjiHz / wspolczynnik;
    }
    if (w.fdHz > 0) {
        r.maCs = true;
        const double fSekcji = w.fdHz * wspolczynnik;
        r.csPotrzebneNF = 1.0 / (2 * kPi * r.rcalkKOhm * 1e3 * fSekcji) * 1e9;
        r.csSzeregNF = Kalkulatory::najblizszaWieksza(r.csPotrzebneNF, w.szeregC);
        const double fs = 1.0 / (2 * kPi * r.rcalkKOhm * 1e3 * r.csSzeregNF * 1e-9);
        r.spadekPrzyFdDb = -10.0 * w.sekcje * std::log10(1.0 + (fs / w.fdHz) * (fs / w.fdHz));
    }
    return r;
}

// ---------------------------------------------------------------- regulacja barwy

double polowaTaperu(Taper t) {
    switch (t) {
        case Taper::Log: return 0.12;
        case Taper::LogA: return 0.25;
        case Taper::OdwrotnyLog: return 0.88;
        case Taper::Liniowy: break;
    }
    return 0.5;
}

double udzialRezystancji(double x, Taper t) {
    x = std::min(1.0, std::max(0.0, x));
    if (t == Taper::Liniowy) return x;
    const double m = polowaTaperu(t);
    const double b = (1.0 / m - 1.0) * (1.0 / m - 1.0);
    const double a = 1.0 / (b - 1.0);
    return a * std::pow(b, x) - a;
}

bool UkladBarwy::uzywa(Typ t, int numer) const {
    for (const Skladnik& s : skladniki) if (s.typ == t && s.numer == numer) return true;
    return false;
}
bool UkladBarwy::uzywaPot(int numer) const { return uzywa(Typ::PotLo, numer) || uzywa(Typ::PotHi, numer); }

const std::vector<UkladBarwy>& ukladyBarwy() {
    static const std::vector<UkladBarwy> v = [] {
        // układy z katalogu projektu tonestack (GPL-3.0): generowane skryptem narzedzia/zbuduj_uklady_barwy.py
        std::vector<UkladBarwy> w = {
#include "UkladyBarwyDane.inc"
        };
        // Fender Bassman 5F6-A: topologia Fendera, wartości typowe (250 pF, 20 nF, 20 nF, slope 56 k, wysokie 250 k, bas 1 M, środek 25 k)
        UkladBarwy b = w.at(0);
        b.nazwa = "Fender Bassman 5F6-A (wartości typowe)";
        b.r[1] = 56e3; b.c[1] = 250e-12; b.c[2] = 20e-9; b.c[3] = 20e-9;
        b.rzr = 38e3; b.robc = 1e6;
        b.pot[1].calkowita = 1e6; b.pot[2].calkowita = 25e3; b.pot[3].calkowita = 250e3;
        w.insert(w.begin(), b);
        return w;
    }();
    return v;
}

std::complex<double> odpowiedzUkladu(const UkladBarwy& u, const NastawyBarwy& n, double fHz) {
    if (!(fHz > 0 && u.rzr >= 0 && u.robc >= 0)) return 0;
    for (int i = 1; i <= 3; ++i) if (!(n.poz[i] >= 0 && n.poz[i] <= 1)) return 0;
    const Zesp jw(0, 2 * kPi * fHz);

    // numeracja węzłów: 0 = masa, pozostałe z netlisty (wejście 2 i wyjście 3 zawsze)
    std::vector<int> wezly = {2, 3};
    auto dodaj = [&](int w) { if (w != 0 && std::find(wezly.begin(), wezly.end(), w) == wezly.end()) wezly.push_back(w); };
    for (const Skladnik& s : u.skladniki) { dodaj(s.w1); dodaj(s.w2); }
    const int N = static_cast<int>(wezly.size());
    auto idx = [&](int w) { return w == 0 ? -1 : static_cast<int>(std::find(wezly.begin(), wezly.end(), w) - wezly.begin()); };
    std::vector<std::vector<Zesp>> A(N, std::vector<Zesp>(N + 1, Zesp(0)));
    auto admitancja = [&](int w1, int w2, Zesp y) {
        const int a = idx(w1), b = idx(w2);
        if (a >= 0) A[a][a] += y;
        if (b >= 0) A[b][b] += y;
        if (a >= 0 && b >= 0) { A[a][b] -= y; A[b][a] -= y; }
    };
    constexpr double kMinR = 0.01;   // rezystancja zerowa odcinka potencjometru na końcu skali

    for (const Skladnik& s : u.skladniki) {
        if (s.w1 == s.w2) continue;   // odcinek zwarty sam do siebie nie wnosi admitancji (np. suwak połączony z końcem)
        Zesp y;
        switch (s.typ) {
            case Typ::R: {
                if (s.numer < 1 || s.numer > 5 || !(u.r[s.numer] > 0)) return 0;
                y = 1.0 / u.r[s.numer];
                break;
            }
            case Typ::C: {
                if (s.numer < 1 || s.numer > 5 || !(u.c[s.numer] > 0)) return 0;
                y = jw * u.c[s.numer];
                break;
            }
            case Typ::L: {
                if (s.numer < 1 || s.numer > 2 || !(u.l[s.numer] > 0)) return 0;
                y = 1.0 / (jw * u.l[s.numer]);
                break;
            }
            case Typ::PotLo:
            case Typ::PotHi: {
                if (s.numer < 1 || s.numer > 3 || !(u.pot[s.numer].calkowita > 0)) return 0;
                const double f = udzialRezystancji(n.poz[s.numer], u.pot[s.numer].taper);
                const double r = u.pot[s.numer].calkowita * (s.typ == Typ::PotLo ? f : 1.0 - f);
                y = 1.0 / std::max(r, kMinR);
                break;
            }
        }
        admitancja(s.w1, s.w2, y);
    }
    // źródło 1 V z rezystancją szeregową (Norton) na węźle 2 i obciążenie na węźle 3
    const double rzr = std::max(u.rzr, 1e-3);
    A[idx(2)][idx(2)] += 1.0 / rzr;
    A[idx(2)][N] += 1.0 / rzr;
    if (u.robc > 0) A[idx(3)][idx(3)] += 1.0 / u.robc;

    for (int k = 0; k < N; ++k) {   // eliminacja Gaussa z wyborem elementu głównego
        int p = k;
        for (int i = k + 1; i < N; ++i) if (std::abs(A[i][k]) > std::abs(A[p][k])) p = i;
        if (std::abs(A[p][k]) < 1e-30) return 0;
        std::swap(A[k], A[p]);
        for (int i = k + 1; i < N; ++i) {
            const Zesp m = A[i][k] / A[k][k];
            for (int j = k; j <= N; ++j) A[i][j] -= m * A[k][j];
        }
    }
    std::vector<Zesp> v(N);
    for (int i = N - 1; i >= 0; --i) {
        Zesp sum = A[i][N];
        for (int j = i + 1; j < N; ++j) sum -= A[i][j] * v[j];
        v[i] = sum / A[i][i];
    }
    return v[idx(3)];
}

double poziomUkladuDb(const UkladBarwy& u, const NastawyBarwy& n, double fHz) {
    const double m = std::abs(odpowiedzUkladu(u, n, fHz));
    return 20.0 * std::log10(std::max(m, 1e-12));
}

ParametryUkladu parametryUkladu(const UkladBarwy& u, const NastawyBarwy& n) {
    ParametryUkladu p;
    p.poziom100Hz = poziomUkladuDb(u, n, 100);
    p.poziom400Hz = poziomUkladuDb(u, n, 400);
    p.poziom1kHz = poziomUkladuDb(u, n, 1000);
    p.poziom4kHz = poziomUkladuDb(u, n, 4000);
    p.poziom10kHz = poziomUkladuDb(u, n, 10000);
    p.minimumDb = 1e9;
    p.maksimumDb = -1e9;
    for (double f = 20; f <= 20000 * 1.0001; f *= std::pow(10.0, 1.0 / 60)) {
        const double db = poziomUkladuDb(u, n, f);
        p.maksimumDb = std::max(p.maksimumDb, db);
        if (f >= 100 && f <= 3000 && db < p.minimumDb) { p.minimumDb = db; p.fMinimumHz = f; }
    }
    return p;
}

} // namespace KalkulatoryWzmacniaczy
