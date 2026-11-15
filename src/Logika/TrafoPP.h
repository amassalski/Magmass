#ifndef TRAFOPP_H
#define TRAFOPP_H

#include <optional>
#include <string>
#include <vector>
#include "TrafoModel.h"

/// Transformator głośnikowy przeciwsobny (PP), obliczenia w etapach jak w programach typu Laboratorium2005.
/// Etap 1: pożądane parametry z wymagań; Etap 2: uzwojenia na zdobytym rdzeniu.
namespace TrafoPP {

struct Wymagania {
    double rObciazeniaOm = 8, raaOm = 3400, mocW = 40, fdHz = 40, fgHz = 20000, riOm = 15000, i0Ma = 30;
    double bGs = 1000;       ///< indukcja (Hi-Fi 1000 Gs)
    double mi = 500;         ///< przenikalność rdzenia
    double eta = 0.9;        ///< sprawność transformatora
    double alfaFe = 0.9;     ///< współczynnik wypełnienia rdzenia żelazem
    double betaOkna = 0.3;   ///< współczynnik wypełnienia okna miedzią
    double sigma = 2;        ///< gęstość prądu [A/mm²]
    double spadekDb = 3;     ///< dopuszczalny spadek charakterystyki na krańcach pasma: 1, 2 lub 3 dB (3 = punkt połowy mocy)
    double przekladkaCm = 0.05;
};

struct Pozadane {
    double liH = 0;          ///< indukcyjność pierwotna minimalna
    double lrH = 0;          ///< indukcyjność rozproszenia maksymalna
    double przekladnia = 0;  ///< Naa:N0
    double iloczynCm4 = 0;   ///< okno x rdzeń
    double przekrojCm2 = 0;  ///< przekrój rdzenia około
};
std::optional<Pozadane> etap1(const Wymagania& w);

struct Rdzen {
    double przekrojCm2 = 0, drogaStrumieniaCm = 0, dlugoscZwojuCm = 0, wysokoscUzwojeniaCm = 0, szerokoscOknaCm = 0;
};

struct Uzwojenia {
    double zwojePierwotne = 0, drutPierwotnyMm = 0, zwojeWtorne = 0, drutWtornyMm = 0;
    double pradPierwotnyA = 0, pradWtornyA = 0;
};
std::optional<Uzwojenia> etap2(const Wymagania& w, const Rdzen& r);

/// Porównanie: metoda uproszczona ze strony o wzmacniaczach gitarowych (guitaramplifiers.prv.pl, wzory z obrazków sprawdzone niezależnie).
/// L1 = Ri/(2π·fd·(1+Ri/Raa)·√(Mn²−1)); Vz = 0,4π·μ·Um1²/((2π·fd)²·L1·Bm²)·10⁸ [cm³]; z1 = 10⁴·√(L1·lż/(0,4π·μ·qż));
/// I1 = √(I0² + 0,25·Imax²) (AB) lub √(I0² + 0,125·Imax²) (A); Imax = √(2P/Raa); z2 = z1·√(R0/(η·Raa)). Mn = 1,25 (spadek 2 dB).
struct Gitarowe {
    double mn = 1.25;
    double l1H = 0, um1V = 0, objetoscCm3 = 0, zwoje1 = 0, imaxA = 0, i1A = 0, d1Mm = 0, zwoje2 = 0, d2Mm = 0;
    /// wtórne dla stałych impedancji 4, 8 i 16 Ω (niezależnie od wpisanego obciążenia): zwoje i średnica drutu
    double zwoje2Zl[3] = {0, 0, 0}, d2ZlMm[3] = {0, 0, 0};
};
std::optional<Gitarowe> gitarowe(const Wymagania& w, const Rdzen& r, double mn = 1.25, bool klasaA = false);

/// Etap 3: uzwojenia wykonane (całkowite zwoje, normalne średnice drutu) i ich rozmieszczenie w oknie.
/// Wzory fizyczne: rozproszenie z całki energii pola (jednowymiarowy rozkład siłomotorycznej w oknie), długość zwoju z geometrii,
/// sprawność ze strat w miedzi. Książka (Gierasimow/Krize) i ECCLab służą tylko do porównania.
/// Jedna sekcja uzwojenia z tabeli użytkownika (od rdzenia na zewnątrz): A = pierwotne (anoda), W = wtórne.
struct SekcjaWej {
    bool pierwotna = true;
    int zwoje = 0;
    int komora = 0;          ///< numer komory karkasu (0, 1, ...); przegroda karkasu dzieli okno na komory
};
/// Gotowe układy sekcji dla z1 i z2 zwojów: 0 = P-S, 1 = P/2-S-P/2, 2 = gitarowy 3-sekcyjny (A 40% / W 50% / A 20% / W 50% / A 40%),
/// 3 = HiFi dwukomorowy (komora 1: A½ W½, komora 2: A½ W½), 4 = HiFi dwukomorowy przeplatany (każda komora: A¼ W½ A¼)
std::vector<SekcjaWej> presetSekcji(int numer, int z1, int z2);
/// Tekst „A1 600 / W1 100 / | / A2 300 ...” -> sekcje. Wiersz „|” (albo „---”) zaczyna nową komorę. A lub P = pierwotne, W lub S = wtórne.
bool parsujSekcje(const std::string& tekst, std::vector<SekcjaWej>& sekcje, std::string& blad);
/// Sekcje -> tekst w formacie jak wyżej (A1, W1, A2, ... i „|” między komorami)
std::string formatujSekcje(const std::vector<SekcjaWej>& sekcje);

struct Wykonane {
    int zwojePierwotne = 0, zwojeWtorne = 0;
    double drutPierwotnyMm = 0, drutWtornyMm = 0;       ///< średnice gołego drutu (np. normalne 0,3 i 1,3)
    bool pierwotnePodzielone = false;                   ///< true: pierwotne w dwóch połowach po obu stronach wtórnego (3 sekcje)
    double przekladkaWarstwMm = 0.05;                   ///< papier między warstwami
    double kEfektyBrzegowe = 0;                         ///< poprawka na efekty brzegowe pola rozproszenia: 0 = automatycznie (współczynnik Rogowskiego z geometrii), >0 = wartość ręczna (1,0 = model idealny)
    double rezystywnosc = 0.0178;                       ///< miedź [Ω·mm²/m]
    std::vector<SekcjaWej> sekcjeWej;                   ///< jeśli niepuste: układ sekcji z tabeli (zamiast pierwotnePodzielone); suma A = z1, suma W = z2
    double sciankaKarkasuMm = 1.5;                      ///< grubość policzka karkasu z każdej strony okna (wysokość uzwojenia = h − 2·ścianka) i od strony rdzenia
    double marginesWarstwyMm = 1.5;                     ///< wolny pas na brzegu każdej warstwy (drut się nie zsuwa); 0 = warstwa na całej wysokości komory
    double przegrodaMm = 2.0;                           ///< grubość przegrody karkasu między komorami (zabiera wysokość okna)
    bool wtorneRownolegle = false;                      ///< sekcje wtórne połączone równolegle (wszystkie o tej samej liczbie zwojów = z2)
};

struct SekcjaUzwojenia {
    bool pierwotna = true;
    int zwoje = 0;
    int zwojeWWarstwie = 0, warstwy = 0;
    double gruboscMm = 0;                               ///< razem z przekładkami między warstwami
    double dlugoscZwojuCm = 0;                          ///< średnia długość zwoju sekcji
    bool nieparzysteWarstwy = false;
    int komora = 0;
    double zwojePrzed = 0;                              ///< zwoje tego samego uzwojenia (A albo W) wcześniej w tabeli
    int zwojeOstatniejWarstwy = 0;
    double wysokoscKomoryMm = 0;
};

struct WynikWykonane {
    double liH = 0, raaOm = 0;                          ///< indukcyjność pierwotna i Raa wykonanego transformatora
    std::vector<SekcjaUzwojenia> sekcje;                ///< od rdzenia na zewnątrz
    double zajeteCm2 = 0, miejsceCm2 = 0;               ///< pole zajęte przez uzwojenia i pole okna (szerokość x wysokość)
    bool miesciSie = false;
    bool niezrownowazone = false;                       ///< któraś komora nie ma równych amperozwojów A i W (strumień rozproszenia nie zamyka się w komorze)
    double rpOm = 0, rwOm = 0;                          ///< rezystancje uzwojeń (model fizyczny)
    double lrH = 0;                                     ///< rozproszenie dla wybranego układu (model fizyczny)
    double lrNieprzeplatanyH = 0, lrPrzeplatanyH = 0;   ///< to samo dla obu układów
    double sprawnosc = 0;                               ///< P0 / (P0 + straty w miedzi)
    // porównanie z literaturą i ECCLab
    double kUzyte = 1;                                  ///< poprawka brzegowa użyta do Lr (ręczna albo z współczynnika Rogowskiego)
    double lrKsiazkaH = 0;                              ///< wzór z książki: nieprzeplatane, Δ1+Δ2 = szerokość okna a, k jak wyżej
    double rwEcclabOm = 0;                              ///< ECCLab: długość zwoju wtórnego Cu + 1,4π·a
};
std::optional<WynikWykonane> etap3(const Wymagania& w, const Rdzen& r, const Wykonane& u);

/// Analiza dokładna wykonanego transformatora PP jednym wspólnym modelem (TrafoModel).
std::optional<TrafoModel::Wynik> analizaDokladna(const Wymagania& w, const Rdzen& r, const Wykonane& u, const WynikWykonane& o, const TrafoModel::Parametry& p);

} // namespace TrafoPP

#endif
