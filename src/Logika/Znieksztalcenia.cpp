#include "Znieksztalcenia.h"

#include <cmath>
#include <limits>

namespace Znieksztalcenia {

double Harmoniczne::dB(double p) { return p > 0 ? 20 * std::log10(p / 100.0) : -std::numeric_limits<double>::infinity(); }

namespace {
void wypelnij(Harmoniczne& r) {
    r.h2 = std::abs(r.i2 / r.i1) * 100;
    r.h3 = std::abs(r.i3 / r.i1) * 100;
    r.h4 = std::abs(r.i4 / r.i1) * 100;
    r.h = std::sqrt(r.h2 * r.h2 + r.h3 * r.h3 + r.h4 * r.h4);
}
}

std::optional<Harmoniczne> zPradow(double m, double p1, double p0, double p2, double n) {
    for (double v : {m, p1, p0, p2, n})
        if (!std::isfinite(v)) return std::nullopt;
    Harmoniczne r;
    r.i0 = (m + 2 * p1 + 2 * p2 + n) / 6.0;
    r.i1 = (m + p1 - p2 - n) / 3.0;
    r.i2 = (m - 2 * p0 + n) / 4.0;
    r.i3 = (m - 2 * p1 + 2 * p2 - n) / 6.0;
    r.i4 = (m - 4 * p1 + 6 * p0 - 4 * p2 + n) / 12.0;
    if (!(std::abs(r.i1) > 1e-12)) return std::nullopt;
    wypelnij(r);
    return r;
}

std::optional<Harmoniczne> zOdcinkow(double a, double b, double c, double d, double e, double f, double g) {
    for (double v : {a, b, c, d, e, f, g})
        if (!std::isfinite(v)) return std::nullopt;
    Harmoniczne r;
    r.i1 = (a + b + c) / 3.0;                 // w jednostkach długości odcinka
    r.i2 = (a - b) / 4.0;
    r.i3 = (a + b - 2 * c) / 6.0;
    r.i4 = (d - 3 * e + 3 * f - g) / 12.0;
    if (!(std::abs(r.i1) > 1e-12)) return std::nullopt;
    wypelnij(r);
    return r;
}

} // namespace Znieksztalcenia
