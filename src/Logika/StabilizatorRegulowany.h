#ifndef STABILIZATORREGULOWANY_H
#define STABILIZATORREGULOWANY_H

#include <optional>

/**
 * Regulowane stabilizatory trójkońcówkowe LM317 (dodatni) i LM337 (ujemny).
 * Układ: rezystor R1 między wyjściem a końcówką ADJ, R2 od ADJ do masy. Stabilizator utrzymuje między wyjściem a ADJ napięcie
 * odniesienia Vref (≈ 1,25 V), więc przez R1 płynie Vref/R1. Do węzła ADJ dochodzi jeszcze prąd końcówki ADJ (Iadj, rzędu 50 µA),
 * który płynie przez R2 razem z prądem R1 (bilans prądów w węźle ADJ):
 *     |Uwy| = Vref + R2·(Vref/R1 + Iadj) = Vref·(1 + R2/R1) + Iadj·R2
 * Wzór jest taki sam dla obu układów; LM337 daje napięcie ujemne. Wartości Vref, Iadj i minimalnego prądu obciążenia zależą od producenta
 * (p. karta katalogowa), dlatego są parametrami.
 */
namespace StabilizatorRegulowany {

enum class Typ { LM317, LM337 };

struct Parametry {
    double vRef = 1.25;      ///< napięcie odniesienia [V] (wartość bezwzględna; zakres z kart: 1,20 ... 1,30 V)
    double iAdjA = 50e-6;    ///< prąd końcówki ADJ [A] (typ. 50 µA, maks. 100 µA)
    double iMinA = 3.5e-3;   ///< minimalny prąd obciążenia [A]: LM317 typ. 3,5 mA, LM337 typ. 2,5 mA
};
Parametry typowe(Typ t);

/// Napięcie wyjściowe [V] z znakiem (LM337 ujemne). Opory w omach, R1 > 0, R2 >= 0.
double napiecieWyjsciowe(Typ t, double r1, double r2, const Parametry& p);

struct Zakres { double min = 0, typ = 0, maks = 0; };   ///< |Uwy| przy Vref 1,20 ... 1,30 V i Iadj 0 ... 100 µA
Zakres rozrzutNapiecia(double r1, double r2, const Parametry& p);

/// R2 [Ω] dla zadanego |Uwy|; nullopt, gdy |Uwy| <= Vref albo R1 <= 0
std::optional<double> r2DlaNapiecia(double uWy, double r1, const Parametry& p);

struct Moc {
    double wejscieMinV = 0;   ///< najmniejsze napięcie wejściowe: |Uwy| + 3 V (różnica we/wy, jak w warunkach pomiarowych kart)
    double stratyW = 0;       ///< (|Uwe| − |Uwy|)·Iobc (pomijalnie mały prąd ADJ)
    double pradDzielnikaA = 0;///< Vref/R1: wlicza się do minimalnego prądu obciążenia
    bool pradMinimalnyOk = true;
};
Moc bilans(double uWeAbs, double uWyAbs, double iObcA, double r1, const Parametry& p);

}  // namespace StabilizatorRegulowany

#endif  // STABILIZATORREGULOWANY_H
