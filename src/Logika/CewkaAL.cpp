#include "CewkaAL.h"

#include <cmath>

namespace CewkaAL {

std::optional<Wynik> oblicz(const Wejscie& w) {
    for (double v : {w.alNH, w.indukcyjnoscH})
        if (!std::isfinite(v) || !(v > 0)) return std::nullopt;
    for (double v : {w.tolerancjaAlProc, w.zwojeSprawdz, w.aeMm2, w.pradA, w.bNasyceniaT})
        if (!std::isfinite(v) || v < 0) return std::nullopt;
    if (w.tolerancjaAlProc >= 100) return std::nullopt;
    Wynik o;
    const double al = w.alNH * 1e-9;
    o.zwojeDokladne = std::sqrt(w.indukcyjnoscH / al);
    o.zwoje = static_cast<int>(std::lround(o.zwojeDokladne));
    o.indukcyjnoscWykonanaH = al * static_cast<double>(o.zwoje) * o.zwoje;
    o.bladProc = (o.indukcyjnoscWykonanaH - w.indukcyjnoscH) / w.indukcyjnoscH * 100.0;
    const double t = w.tolerancjaAlProc / 100.0;
    o.zwojeMin = std::sqrt(w.indukcyjnoscH / (al * (1 + t)));   // AL większe -> mniej zwojów
    o.zwojeMax = std::sqrt(w.indukcyjnoscH / (al * (1 - t)));
    if (w.zwojeSprawdz > 0) {
        o.indukcyjnoscSprawdzH = al * w.zwojeSprawdz * w.zwojeSprawdz;
        o.bladSprawdzProc = (o.indukcyjnoscSprawdzH - w.indukcyjnoscH) / w.indukcyjnoscH * 100.0;
    }
    if (w.aeMm2 > 0 && w.pradA > 0 && o.zwoje > 0) {
        o.bPoprawkaT = o.indukcyjnoscWykonanaH * w.pradA / (o.zwoje * w.aeMm2 * 1e-6);
        o.przekroczonaIndukcja = w.bNasyceniaT > 0 && o.bPoprawkaT > w.bNasyceniaT;
    }
    return o;
}

} // namespace CewkaAL
