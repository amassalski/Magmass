#ifndef SCHEMATBARWY_H
#define SCHEMATBARWY_H

#include "KalkulatoryWzmacniaczy.h"

#include <functional>
#include <string>
#include <vector>

/// Automatyczny rysunek układu regulacji barwy z jego netlisty (bez zależności od wxWidgets, więc da się go testować).
/// Układ jest rysowany jako "szyny" węzłów: każdy węzeł to pionowa linia, każdy element leży poziomo między szynami swoich
/// węzłów, a elementy dołączone do masy zwisają w dół do symbolu masy. Połączenia są zawsze zgodne z netlistą używaną
/// do obliczeń, więc rysunek nie może pokazywać innego układu niż wykres.
namespace SchematBarwy {

enum class Rodzaj {
    Linia,        ///< (x1,y1)-(x2,y2)
    Rezystor,     ///< symbol od (x1,y1) do (x2,y2)
    Kondensator,  ///< jw.
    Cewka,        ///< jw.
    Kropka,       ///< połączenie w (x1,y1)
    Masa,         ///< symbol masy, góra w (x1,y1)
    Wzmacniacz,   ///< trójkąt: wejście odwracające w (x1,y1), wyjście w (x2,y2)
    Tekst,        ///< napis; x1,y1 = środek poziomy i góra; x2 = 1: wyrównanie do lewej
    Zacisk        ///< okrąg zacisku w (x1,y1)
};

struct Prym {
    Rodzaj rodzaj = Rodzaj::Linia;
    double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    std::string tekst;
};

struct Rysunek {
    std::vector<Prym> prymy;
    double szerokosc = 0, wysokosc = 0;
    int elementow = 0;   ///< narysowane R, C, L, części potencjometrów
    bool czynny = false;

    int ile(Rodzaj r) const {
        int n = 0;
        for (const Prym& p : prymy) n += p.rodzaj == r;
        return n;
    }
};

/// etykieta(składnik) daje napis nad symbolem (np. "R1 100k", "Bas 125k"); pusty = nazwa domyślna.
Rysunek zbuduj(const KalkulatoryWzmacniaczy::UkladBarwy& u,
               const std::function<std::string(const KalkulatoryWzmacniaczy::Skladnik&)>& etykieta = nullptr);

}  // namespace SchematBarwy

#endif
