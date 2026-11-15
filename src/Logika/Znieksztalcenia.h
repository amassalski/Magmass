#ifndef ZNIEKSZTALCENIA_H
#define ZNIEKSZTALCENIA_H

#include <optional>

/// Zniekształcenia nieliniowe metodą pięciu rzędnych (Klin): prądy w pięciu punktach prostej obciążenia
/// odpowiadających wysterowaniu +Um, +0,5Um, 0, -0,5Um, -Um (Ia max, Ia1, Ia0, Ia2, Ia min).
/// Źródła: Cykin, Wzmacniacze elektroniczne 1970, p. 4.2.1; Zawada, Lampy elektronowe w aplikacjach audio 2011, p. 3.9.
namespace Znieksztalcenia {

struct Harmoniczne {
    double i0 = 0;                 ///< składowa stała
    double i1 = 0, i2 = 0, i3 = 0, i4 = 0;   ///< amplitudy harmonicznych prądu
    double h2 = 0, h3 = 0, h4 = 0;           ///< współczynniki zawartości [%] względem pierwszej
    double h = 0;                  ///< łącznie: sqrt(h2²+h3²+h4²) [%]
    static double dB(double procent);        ///< 20 log10(procent/100); -inf dla 0
};

/// Z prądów: imax, i1, i0, i2, imin [mA]. nullopt, gdy pierwsza harmoniczna wynosi 0 (brak wysterowania)
std::optional<Harmoniczne> zPradow(double imax, double i1, double i0, double i2, double imin);
/// Z odcinków prostej obciążenia: a, b, c do h2 i h3, d, e, f, g do h4 (a = d+e, b = f+g, c = e+f dla tych samych punktów)
std::optional<Harmoniczne> zOdcinkow(double a, double b, double c, double d, double e, double f, double g);

} // namespace Znieksztalcenia

#endif
