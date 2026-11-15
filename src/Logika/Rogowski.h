#ifndef ROGOWSKI_H
#define ROGOWSKI_H

#include <cmath>

/// Współczynnik Rogowskiego: poprawka indukcyjności rozproszenia na rozchodzenie się pola przy końcach uzwojenia (efekty brzegowe).
/// Model jednowymiarowy zakłada pole równoległe do warstw na całej wysokości h; w rzeczywistości pole się rozszerza,
/// a rozproszenie maleje: K = 1 − (δ/(π·h))·(1 − e^(−π·h/δ)), gdzie δ to zastępcza odległość uzwojeń
/// (Σ grubość·(F0² + F0·F1 + F1²)/3 z modelu pola), h – długość warstwy (ta sama jednostka). K → 1 dla δ << h.
inline double wspolczynnikRogowskiego(double delta, double h) {
    if (!(delta > 0) || !(h > 0)) return 1.0;
    const double a = 3.14159265358979323846 * h / delta;
    return 1.0 - (1.0 - std::exp(-a)) / a;
}

#endif
