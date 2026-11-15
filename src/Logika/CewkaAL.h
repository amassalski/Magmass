#ifndef CEWKAAL_H
#define CEWKAAL_H

#include <optional>

/// Cewka na rdzeniu z podanym współczynnikiem AL (rdzenie ferrytowe, proszkowe, toroidy): L = AL·N².
/// AL podaje się w nH/zw² (tak jak w katalogach). Zwoje: N = √(L/AL) – wprost z definicji AL, bez poprawek.
namespace CewkaAL {

struct Wejscie {
    double alNH = 0;            ///< AL [nH/zw²]
    double indukcyjnoscH = 0;   ///< żądana indukcyjność [H]
    double tolerancjaAlProc = 0;///< tolerancja AL [±%] (np. 25 dla ferrytów bez szczeliny)
    double zwojeSprawdz = 0;    ///< opcjonalnie: liczba zwojów do sprawdzenia (np. z innego programu); 0 = brak
    double aeMm2 = 0;           ///< opcjonalnie: przekrój czynny rdzenia Ae [mm²] (do indukcji)
    double pradA = 0;           ///< opcjonalnie: prąd szczytowy [A]
    double bNasyceniaT = 0.3;   ///< indukcja, powyżej której ostrzegamy (ferryty ok. 0,3 T)
};

struct Wynik {
    double zwojeDokladne = 0;   ///< N = √(L/AL)
    int zwoje = 0;              ///< zaokrąglone do całkowitej liczby
    double indukcyjnoscWykonanaH = 0;   ///< AL·N² dla zaokrąglonej liczby zwojów
    double bladProc = 0;        ///< (L_wyk − L)/L
    double zwojeMin = 0, zwojeMax = 0;  ///< zakres zwojów, gdy AL leży w granicach tolerancji (dla zadanej indukcyjności)
    double indukcyjnoscSprawdzH = 0, bladSprawdzProc = 0;   ///< dla zwojeSprawdz
    double bPoprawkaT = 0;      ///< B = L·I/(N·Ae) dla zaokrąglonej liczby zwojów [T]; 0 gdy brak Ae/I
    bool przekroczonaIndukcja = false;
};

std::optional<Wynik> oblicz(const Wejscie& w);

} // namespace CewkaAL

#endif
