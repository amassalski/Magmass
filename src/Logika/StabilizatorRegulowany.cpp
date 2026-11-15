#include "StabilizatorRegulowany.h"

#include <algorithm>
#include <cmath>

namespace StabilizatorRegulowany {

Parametry typowe(Typ t) {
    Parametry p;
    p.iMinA = (t == Typ::LM317) ? 3.5e-3 : 2.5e-3;
    return p;
}

double napiecieWyjsciowe(Typ t, double r1, double r2, const Parametry& p) {
    const double u = p.vRef * (1.0 + r2 / r1) + p.iAdjA * r2;
    return t == Typ::LM317 ? u : -u;
}

Zakres rozrzutNapiecia(double r1, double r2, const Parametry& p) {
    Zakres z;
    z.typ = p.vRef * (1.0 + r2 / r1) + p.iAdjA * r2;
    z.min = 1.20 * (1.0 + r2 / r1);
    z.maks = 1.30 * (1.0 + r2 / r1) + 100e-6 * r2;
    return z;
}

std::optional<double> r2DlaNapiecia(double uWy, double r1, const Parametry& p) {
    const double u = std::fabs(uWy);
    if (!(r1 > 0) || !(u > p.vRef)) return std::nullopt;
    return (u - p.vRef) / (p.vRef / r1 + p.iAdjA);
}

Moc bilans(double uWe, double uWy, double iObc, double r1, const Parametry& p) {
    Moc m;
    m.wejscieMinV = uWy + 3.0;
    m.stratyW = std::max(0.0, uWe - uWy) * iObc;
    m.pradDzielnikaA = r1 > 0 ? p.vRef / r1 : 0.0;
    m.pradMinimalnyOk = (iObc + m.pradDzielnikaA) >= p.iMinA;
    return m;
}

}  // namespace StabilizatorRegulowany
