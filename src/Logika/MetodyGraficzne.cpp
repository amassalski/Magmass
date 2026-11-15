#include "MetodyGraficzne.h"

#include <algorithm>
#include <cmath>

namespace MetodyGraficzne {

std::optional<Kalibracja> kalibruj(PunktPx zero, PunktPx naOsiU, double napiecieV, PunktPx naOsiI, double pradMa) {
    if (!(napiecieV > 0) || !(pradMa > 0) || !std::isfinite(napiecieV) || !std::isfinite(pradMa)) return std::nullopt;
    const double dx = naOsiU.x - zero.x, dy = zero.y - naOsiI.y;
    if (!(dx > 0) || !(dy > 0)) return std::nullopt;
    return Kalibracja{zero, napiecieV / dx, pradMa / dy};
}

std::optional<WynikProstej> policz(const Prosta& p) {
    WynikProstej w;
    w.u1PoLewejOdOsi = p.p1.u <= 0;
    w.deltaU = p.p2.u - std::max(p.p1.u, 0.0);
    w.deltaI = p.p1.i - p.p2.i;
    if (!(w.deltaU > 0) || !(w.deltaI > 0)) return std::nullopt;
    w.raA = w.deltaU / w.deltaI;           // V / mA = kΩ
    w.raaB = 4 * w.raA;
    w.mocSE = w.deltaU * w.deltaI / 8.0 / 1000.0;
    w.mocPP = 4 * w.mocSE;
    return w;
}

Prosta przesun(const Prosta& p, Koniec ktory, PunktUI cel, const Blokady& b) {
    Prosta r = p;
    const double ra = (p.p1.i - p.p2.i) > 0 ? (p.p2.u - std::max(p.p1.u, 0.0)) / (p.p1.i - p.p2.i) : 0.0;
    const double dI = p.p1.i - p.p2.i;
    if (ktory == Koniec::Pradowy) {
        r.p1 = cel;
        r.p1.u = std::min(r.p1.u, p.p2.u);
        r.p1.i = std::max(r.p1.i, p.p2.i);
        if (b.amplituda) r.p2.i = r.p1.i - dI;
        if (b.ra && ra > 0) r.p2.u = std::max(r.p1.u, 0.0) + ra * (r.p1.i - r.p2.i);
    } else {
        r.p2 = cel;
        r.p2.u = std::max(r.p2.u, r.p1.u);
        r.p2.i = std::min(r.p2.i, p.p1.i);
        if (b.amplituda) r.p1.i = r.p2.i + dI;
        if (b.ra && ra > 0 && r.p1.i > r.p2.i) {
            const double u1 = r.p2.u - ra * (r.p1.i - r.p2.i);
            if (u1 >= 0 || b.amplituda) r.p1.u = std::max(u1, 0.0);
            else {                                   // wyszłoby na lewo od osi prądu: koniec prądowy zostaje na osi
                r.p1.u = 0;
                r.p1.i = r.p2.i + r.p2.u / ra;       // a jego prąd rośnie, by zachować Ra
            }
        }
    }
    // spójność po blokadach
    r.p2.u = std::max(r.p2.u, r.p1.u);
    return r;
}

std::vector<PunktUI> krzywaPaMax(double paW, double odV, double doV, int n) {
    std::vector<PunktUI> w;
    if (!(paW > 0) || !(odV > 0) || !(doV > odV) || n < 2) return w;
    w.reserve(n);
    for (int k = 0; k < n; k++) {
        const double u = odV + (doV - odV) * k / (n - 1);
        w.push_back({u, paW / u * 1000.0});
    }
    return w;
}

} // namespace MetodyGraficzne
