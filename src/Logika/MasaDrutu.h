#ifndef MASADRUTU_H
#define MASADRUTU_H

// Zestawienie drutu nawojowego dla zakupu na wagę: długość i masa każdej średnicy z uzwojeń transformatora lub dławika.

#include <string>
#include <vector>

#include "Dlawik.h"
#include "TrafoPP.h"
#include "TrafoSE.h"
#include "TrafoSieciowy.h"

namespace MasaDrutu {

constexpr double kGestoscMiedziGcm3 = 8.96;   ///< gęstość miedzi [g/cm³]
constexpr double kDodatekEmaliiPct = 3.0;     ///< przybliżony dodatek masy lakieru (emalii) do masy miedzi, rząd wielkości
constexpr double kZapasPct = 10.0;            ///< domyślny zapas na odpady, początek i koniec uzwojenia, wyprowadzenia, błędy nawijania

/// Jedno uzwojenie albo sekcja: tyle zwojów drutu o danej średnicy (gołego) o danej średniej długości zwoju
struct Pozycja {
    std::string nazwa;          ///< np. "P", "W1", "P2"
    double drutMm = 0;          ///< średnica drutu gołego
    double zwoje = 0;
    double dlugoscZwojuCm = 0;  ///< średnia długość zwoju
};

struct Wiersz {
    double drutMm = 0;
    double dlugoscM = 0;        ///< długość drutu (bez zapasu)
    double masaCuG = 0;         ///< masa samej miedzi
    double masaZakupG = 0;      ///< masa do kupienia: miedź + emalia + zapas
    std::string uzwojenia;      ///< np. "P1, P2"
};

struct Zestawienie {
    std::vector<Wiersz> wiersze;    ///< od najcieńszego drutu
    double razemCuG = 0, razemZakupG = 0, razemDlugoscM = 0;
    bool puste() const { return wiersze.empty(); }
};

/// Masa miedzi [g] drutu o średnicy d [mm] i długości l [m]: π/4·d²·l·ρ (mm²·m = cm³)
double masaMiedziG(double drutMm, double dlugoscM);

/// Pozycje o tej samej średnicy (do 0,001 mm) łączone w jeden wiersz. Pozycje z drutem lub zwojami <= 0 są pomijane.
Zestawienie zestawienie(const std::vector<Pozycja>& pozycje, double zapasPct = kZapasPct);

// adaptery z wyników obliczeń
std::vector<Pozycja> pozycje(const Dlawik::Wynik& w);
std::vector<Pozycja> pozycje(const TrafoSieciowy::Wynik& w);
std::vector<Pozycja> pozycje(const TrafoSE::WynikWykonane& o, const TrafoSE::Wykonane& u);
std::vector<Pozycja> pozycje(const TrafoPP::WynikWykonane& o, const TrafoPP::Wykonane& u);

}  // namespace MasaDrutu

#endif  // MASADRUTU_H
