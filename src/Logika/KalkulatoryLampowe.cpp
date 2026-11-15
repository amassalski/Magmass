#include "KalkulatoryLampowe.h"
#include "tube_audio.hpp"

#include <cmath>
#include <limits>

namespace KalkulatoryLampowe {
namespace {

constexpr double kPi = 3.14159265358979323846;
double rown(double a, double b) { return a * b / (a + b); }
bool dodatnie(double v) { return std::isfinite(v) && v > 0; }
bool nieujemne(double v) { return std::isfinite(v) && v >= 0; }
bool lampaOk(const Trioda& l) { return dodatnie(l.sa) && dodatnie(l.ka) && dodatnie(l.rwew); }

} // namespace

std::optional<WynikRC> sprzezenieRC(const WejscieRC& w) {
    if (!lampaOk(w.lampa) || !dodatnie(w.ra) || !dodatnie(w.rs) || !dodatnie(w.csNf) || !nieujemne(w.rk) ||
        !nieujemne(w.cak) || !nieujemne(w.cas) || !nieujemne(w.csk))
        return std::nullopt;
    const double mu = w.lampa.ka, rw = w.lampa.rwew;
    const double rk = w.automatyczna ? w.rk : 0.0;
    const double rp = rown(w.ra, w.rs);
    WynikRC r;
    r.ku = mu * rp / (rw + rp + (mu + 1) * rk);
    const double rzr = rw + (mu + 1) * rk;                       // oporność lampy widziana z anody
    r.rwyk = 1.0 / (1.0 / w.ra + 1.0 / w.rs + 1.0 / rzr);
    const double rzrod = rown(w.ra, rzr);
    r.fdHz = 1.0 / (2 * kPi * w.csNf * 1e-9 * (rzrod + w.rs) * 1e3);
    // pojemność zastępcza na wyjściu: Cak + Csk + Cas (K + 2), oporność r.rwyk (kΩ·pF = ns)
    const double cwyj = w.cak + w.csk + w.cas * (r.ku + 2.0);
    r.fgKHz = 1e6 / (2 * kPi * r.rwyk * cwyj);
    r.fgPrzyblizona = w.automatyczna && rk > 0;
    return r;
}

std::optional<WynikWtornikaKatodowego> wtornikKatodowy(const WejscieWtornikaKatodowego& w) {
    if (!lampaOk(w.lampa) || !dodatnie(w.rk + w.r) || !dodatnie(w.rs) || !dodatnie(w.csk) || !nieujemne(w.rk) ||
        !nieujemne(w.r) || !nieujemne(w.ugk))
        return std::nullopt;
    const double mu = w.lampa.ka, rw = w.lampa.rwew, sa = w.lampa.sa;
    const double rl = w.rk + w.r;
    const double rwPrim = mu / sa;   // oporność wewnętrzna z nachylenia (mu = Sa·Rw')
    WynikWtornikaKatodowego r;
    r.ku = mu * rl / (rw + (mu + 1) * rl);
    r.rwe = w.rs / (1.0 - mu * w.r / (rwPrim + (mu + 1) * rl));
    r.rwyk = 1.0 / sa;
    r.umax = w.ugk / (1.0 - r.ku);
    // biegun na katodzie: Csk w równoległym połączeniu 1/Sa i Rk + R; kΩ i pF dają ns, stąd 1e3 → MHz
    const double rb = rown(1.0 / sa, rl);
    r.fgMHz = 1e3 / (2 * kPi * rb * w.csk);
    return r;
}

std::optional<WynikWtornikaAnodowego> wtornikAnodowy(const WejscieWtornikaAnodowego& w) {
    if (!lampaOk(w.lampa) || !dodatnie(w.rs) || !dodatnie(w.ro)) return std::nullopt;
    const double mu = w.lampa.ka, rw = w.lampa.rwew;
    const double zg = rw / (mu + 1);   // oporność górnej triody widziana od strony katody
    WynikWtornikaAnodowego r;
    r.rwe = w.rs;
    r.rwyk = rown(zg, rw);
    const double z = rown(zg, w.ro);
    r.ku = mu * z / (rw + z);
    return r;
}

std::optional<WynikInwertera> inwerterRoznicowy(const WejscieInwertera& w) {
    if (!dodatnie(w.lampa.ka) || !dodatnie(w.lampa.rwew) || !dodatnie(w.rk) || !dodatnie(w.ra) || !dodatnie(w.rs))
        return std::nullopt;
    const double ra = rown(w.ra, w.rs);   // anoda widzi Ra równolegle z Rs
    const auto L = tube::TriodeParams::fromKaRho(w.lampa.ka, w.lampa.rwew * 1e3);
    const auto t = tube::calcLTPTriode(L, L, ra * 1e3, ra * 1e3, w.rk * 1e3);
    WynikInwertera r;
    r.ku1 = t.ku1;
    r.ku2 = t.ku2;
    r.q = t.q;
    r.kuSr = 0.5 * (t.ku1 + t.ku2);
    return r;
}


namespace {
bool lampaOk2(const Trioda& l) { return dodatnie(l.ka) && dodatnie(l.rwew); }
tube::TriodeParams T(const Trioda& l) { return tube::TriodeParams::fromKaRho(l.ka, l.rwew * 1e3); }
}

std::optional<WynikMuWtornika> muWtornik(const WejscieMuWtornika& w) {
    if (!lampaOk2(w.l1) || !lampaOk2(w.l2) || !nieujemne(w.rk2) || !nieujemne(w.r0) || !nieujemne(w.cas1) ||
        !nieujemne(w.csk1) || !nieujemne(w.cs0) || w.l1.ka <= 1 || w.l2.ka <= 1)
        return std::nullopt;
    const auto r = tube::calcMuFollower(T(w.l1), T(w.l2), w.rk2 * 1e3, w.cs0 * 1e-12, w.csk1 * 1e-12, w.cas1 * 1e-12,
                                        w.r0 > 0 ? w.r0 * 1e3 : std::numeric_limits<double>::infinity());
    return WynikMuWtornika{r.ku, r.Rwy / 1e3, r.Raktywne / 1e3, r.Cin * 1e12};
}

std::optional<WynikSRPP> srpp(const WejscieSRPP& w) {
    if (!lampaOk2(w.lampa) || w.lampa.ka <= 1 || !nieujemne(w.rk) || !nieujemne(w.rl)) return std::nullopt;
    const auto L = T(w.lampa);
    const auto r = tube::calcSRPP(L, L, w.rk * 1e3, w.rl > 0 ? w.rl * 1e3 : std::numeric_limits<double>::infinity());
    return WynikSRPP{r.ku0, r.ku, r.Rwy / 1e3, r.RLopt / 1e3};
}

std::optional<WynikWhite> wtornikWhite(const WejscieWhite& w) {
    if (!lampaOk2(w.l1) || !lampaOk2(w.l2) || !dodatnie(w.rl)) return std::nullopt;
    const auto r = tube::calcWhiteCathodeFollower(T(w.l1), T(w.l2), w.rl * 1e3);
    return WynikWhite{r.ku, r.Rwy / 1e3, r.RaOpt / 1e3};
}

std::optional<WynikKaskodowego> wtornikKaskodowy(const WejscieKaskodowego& w) {
    if (!lampaOk2(w.l1) || !lampaOk2(w.l2) || !dodatnie(w.ra) || !dodatnie(w.r0)) return std::nullopt;
    const auto r = tube::calcKaskodeFollower(T(w.l1), T(w.l2), w.ra * 1e3, w.r0 * 1e3);
    return WynikKaskodowego{r.ku, r.ku0, r.Rwy / 1e3, r.RwyApprox / 1e3};
}

std::optional<WynikSuper> superwtornik(const WejscieSuper& w) {
    if (!lampaOk2(w.lampa) || !dodatnie(w.ra) || w.n < 1 || w.n > 6) return std::nullopt;
    const auto r = tube::calcSuperFollower(T(w.lampa), w.ra * 1e3, w.n);
    return WynikSuper{r.ku1, r.Rwy / 1e3, r.ku3};
}

} // namespace KalkulatoryLampowe
