#ifndef CEWKAPOWIETRZNA_H
#define CEWKAPOWIETRZNA_H

#include <optional>

/**
 * Cewka powietrzna (bez rdzenia): jednowarstwowa (solenoid) i wielowarstwowa (walcowa, warstwa na warstwie).
 *
 * Model główny jest wyprowadzony z fizyki, nie z wzoru empirycznego: cewka to N okrągłych zwojów leżących na siatce
 * (warstwy × zwoje w warstwie), a indukcyjność to suma indukcyjności własnych i wzajemnych wszystkich par zwojów.
 * Indukcyjność wzajemna dwóch współosiowych pierścieni (promienie a, b, odstęp z, k² = 4ab/((a+b)²+z²)):
 *   M = μ0·√(ab)·[(2/k − k)·K(k) − (2/k)·E(k)]      (K, E: zupełne całki eliptyczne)
 * Własna indukcyjność pierścienia z drutu o promieniu ρ (równomierny prąd): L = μ0·a·(ln(8a/ρ) − 7/4).
 * Wzory Wheelera (jedno- i wielowarstwowy) są podawane tylko do porównania.
 * Wymiary w mm, indukcyjności w µH, jeśli nie napisano inaczej.
 */
namespace CewkaPowietrzna {

constexpr double kRezystywnoscCuOhmM = 1.72e-8;   // miedź, 20 °C
constexpr long kMaxObliczen = 3000000;            // powyżej tej liczby składników sumy model dokładny zostaje zastąpiony wzorem Wheelera

/// Zupełne całki eliptyczne pierwszego i drugiego rodzaju (średnia arytmetyczno-geometryczna), 0 <= k < 1
double calkaK(double k);
double calkaE(double k);
/// Indukcyjność wzajemna dwóch współosiowych pierścieni [H]; promienie i odstęp w metrach
double indukcyjnoscWzajemnaPierscieni(double a, double b, double z);
/// Indukcyjność własna pierścienia o promieniu a z drutu o promieniu rDrutu [H] (metry)
double indukcyjnoscWlasnaPierscienia(double a, double rDrutu);

struct Parametry {
    double srednicaKarkasuMm = 0;   ///< średnica wewnętrzna uzwojenia (karkasu)
    double drutMm = 0;              ///< średnica drutu bez izolacji
    double drutZIzolacjaMm = 0;     ///< średnica z izolacją; 0 = drut × 1,06
    double skokMm = 0;              ///< odstęp między osiami sąsiednich zwojów; 0 = średnica z izolacją (zwoje obok siebie)
    int zwojeNaWarstwe = 0;
    int warstwy = 1;
};

struct Wynik {
    double indukcyjnoscUH = 0;          ///< model dokładny (albo Wheeler, gdy dokladny == false)
    bool dokladny = true;
    double wheelerUH = 0;               ///< wzór Wheelera: do porównania
    int zwojeRazem = 0;
    double dlugoscUzwojeniaMm = 0;      ///< b: długość uzwojenia wzdłuż osi
    double gruboscUzwojeniaMm = 0;      ///< c: grubość uzwojenia (promieniowo)
    double sredniPromienMm = 0;         ///< a: średni promień uzwojenia
    double dlugoscDrutuM = 0;
    double opor20Ohm = 0;
    double masaMiedziG = 0;
};

/// std::nullopt, gdy dane są nieprawidłowe (wymiary <= 0, zwoje < 1, warstwy < 1, skok albo średnica z izolacją mniejsze od średnicy drutu / z izolacją)
std::optional<Wynik> oblicz(const Parametry& p);

/// Wzór Wheelera dla cewki jednowarstwowej, D = średnia średnica uzwojenia [mm], l = długość [mm]: L[µH] = D²N² / (25,4·(18D + 40l))
double wheelerJednowarstwowaUH(double sredniaSrednicaMm, double dlugoscMm, double zwoje);
/// Wzór Wheelera dla cewki wielowarstwowej: L[µH] = 0,8·a²N² / (25,4·(6a + 9b + 10c)), a = średni promień, b = długość, c = grubość [mm]
double wheelerWielowarstwowaUH(double sredniPromienMm, double dlugoscMm, double gruboscMm, double zwoje);

struct Dobor {
    int zwoje = 0;                ///< jednowarstwowa: zwoje; wielowarstwowa: liczba warstw
    double indukcyjnoscUH = 0;    ///< dla wyznaczonej liczby
    double poprzedniaUH = 0;      ///< dla liczby o jeden mniejszej (0, gdy to 1)
};

/// Najmniejsza liczba zwojów cewki jednowarstwowej (p.zwojeNaWarstwe jest ignorowane), której L >= docelowa. nullopt: dane złe albo > 20000 zwojów.
std::optional<Dobor> zwojeDlaIndukcyjnosci(Parametry p, double docelowaUH);
/// Najmniejsza liczba warstw cewki wielowarstwowej (p.warstwy ignorowane), której L >= docelowa. nullopt: dane złe albo > 100 warstw.
std::optional<Dobor> warstwyDlaIndukcyjnosci(Parametry p, double docelowaUH);

}  // namespace CewkaPowietrzna

#endif  // CEWKAPOWIETRZNA_H
