#ifndef TRAFOSIECIOWY_H
#define TRAFOSIECIOWY_H

#include "Dlawik.h"
#include <optional>
#include <vector>

/// Transformator sieciowy małej mocy na rdzeniu EI (płaszczowym) - metoda S. N. Krize, "Obliczanie transformatorów zasilających
/// małej mocy i dławików stosowanych w filtrach" (PWT 1953), rozdz. 5: wzory [7], [10], [15], [17], rozmieszczenie uzwojeń [19]-[25].
namespace TrafoSieciowy {

struct Uzwojenie {
    double napiecieV = 0, pradA = 0;
};

struct Wejscie {
    Dlawik::Rdzen rdzen;               ///< wymiary w mm (a = kolumna środkowa, b = pakiet, c = szerokość okna, h = wysokość okna)
    double napiecieSieciV = 230;
    std::vector<Uzwojenie> wtorne;     ///< dowolna liczba uzwojeń wtórnych
    double bGs = 8000;                 ///< indukcja w rdzeniu [Gs]
    double sprawnosc = 0.85;
    double gestoscAmm2 = 2.5;
    double kOkna = 0.3;                ///< współczynnik wypełnienia okna miedzią (tabl. 6 Krize'a)
    double kFe = 0.9;                  ///< współczynnik wypełnienia rdzenia żelazem
    double przekladkaWarstwMm = 0.07;  ///< β: papier między warstwami
    double przekladkaUzwojenMm = 1.0;  ///< γ: izolacja między uzwojeniami
    double sciankaMm = 1.5;            ///< α: grubość ścianki karkasu
    double marginesWarstwyMm = 1.5;    ///< wolny pas na obu brzegach każdej warstwy (drut się nie zsuwa); 0 = wzór książkowy y = h − 2α
    double srDlugoscZwojuCm = 0;       ///< 0 = z geometrii rdzenia
};

struct Typowe { double bGs, sprawnosc, gestoscAmm2; };
/// Wartości zalecane z tabl. 5 Krize'a zależnie od mocy wtórnej [VA]
Typowe typowe(double mocVA);
/// Najbliższa normalna średnica drutu nawojowego (tabl. 3 Krize'a), jak w książce; przy remisie większa
double drutNormalny(double dMm);
/// Średnica drutu w emalii (PE) dla gołego drutu
double drutWEmalii(double dMm);

struct WynikUzwojenia {
    int zwoje = 0;
    double pradA = 0, drutObliczonyMm = 0, drutMm = 0, drutEmaliaMm = 0;
    int zwojeWWarstwie = 0, warstwy = 0;
    double gruboscMm = 0;
};

struct Wynik {
    double mocWtornaVA = 0, mocPierwotnaVA = 0, pradPierwotnyA = 0;
    double iloczynWymaganyCm4 = 0, iloczynPosiadanyCm4 = 0;   ///< q_o·q_z
    double przekrojZelazaCm2 = 0, oknoCm2 = 0, srDlugoscZwojuCm = 0;
    WynikUzwojenia pierwotne;
    std::vector<WynikUzwojenia> wtorne;
    double sumaGruboscMm = 0, zapasMm = 0;   ///< zapas δ = c - (suma grubości + ścianka); ok. 5 mm jest optymalne
    bool miesciSie = false;
};

std::optional<Wynik> oblicz(const Wejscie& w);

} // namespace TrafoSieciowy

#endif
