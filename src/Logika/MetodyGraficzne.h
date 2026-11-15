#ifndef METODYGRAFICZNE_H
#define METODYGRAFICZNE_H

#include <optional>
#include <vector>

/// Metody graficzne na charakterystykach anodowych lampy (bez GUI): skalowanie obrazu, prosta obciążenia,
/// oporność obciążenia klasy A i B, moce SE i PP, krzywa mocy admisyjnej anody Pa max.
/// Napięcia w woltach, prądy w mA, rezystancje w kΩ, moce w W. Współrzędne obrazu w pikselach (y rośnie w dół).
namespace MetodyGraficzne {

struct PunktPx { double x = 0, y = 0; };
struct PunktUI { double u = 0, i = 0; };   ///< V, mA

/// Skalowanie liniowe obrazu: punkt (0 V, 0 mA) w pikselach oraz wartości na piksel na obu osiach
struct Kalibracja {
    PunktPx zero;
    double voltNaPx = 0;
    double mANaPx = 0;

    PunktUI wartosc(PunktPx p) const { return {(p.x - zero.x) * voltNaPx, (zero.y - p.y) * mANaPx}; }
    PunktPx piksel(PunktUI v) const { return {zero.x + v.u / voltNaPx, zero.y - v.i / mANaPx}; }
};

/// Z punktu (0,0), punktu na osi napięcia o znanym napięciu i punktu na osi prądu o znanym prądzie.
/// nullopt, gdy punkt napięcia nie leży na prawo, a punkt prądu nad zerem albo wartości są <= 0.
std::optional<Kalibracja> kalibruj(PunktPx zero, PunktPx naOsiU, double napiecieV, PunktPx naOsiI, double pradMa);

/// Prosta obciążenia od punktu na osi prądu (P1, górny lewy koniec, czerwony) do punktu na osi napięcia (P2, zielony)
struct Prosta {
    PunktUI p1;   ///< koniec „prądowy”: u1 = napięcie, i1 = największy prąd
    PunktUI p2;   ///< koniec „napięciowy”: u2 = największe napięcie, i2 = prąd
};

struct WynikProstej {
    double deltaU = 0, deltaI = 0;   ///< U2 - max(U1, 0) [V]; I1 - I2 [mA]
    bool u1PoLewejOdOsi = false;     ///< koniec prądowy leży na osi albo na lewo od niej (napięcie nieokreślone)
    double raA = 0;                  ///< Ra klasy A = ΔU / ΔI [kΩ]
    double raaB = 0;                 ///< Raa klasy B = 4 Ra [kΩ]
    double mocSE = 0;                ///< ΔU ΔI / 8 [W]
    double mocPP = 0;                ///< 4 · moc SE [W]
};
/// nullopt, gdy ΔU <= 0 albo ΔI <= 0
std::optional<WynikProstej> policz(const Prosta& p);

/// Który koniec jest przesuwany
enum class Koniec { Pradowy, Napieciowy };
struct Blokady { bool ra = false; bool amplituda = false; };

/// Przesuwa koniec prostej na @p cel z ograniczeniem: napięcie końca napięciowego nie może być mniejsze niż końca
/// prądowego, a prąd końca napięciowego nie większy niż końca prądowego. Blokada Ra utrzymuje nachylenie
/// (drugi koniec przesuwa się wzdłuż napięcia), blokada amplitudy utrzymuje ΔI (drugi koniec przesuwa się w prądzie).
Prosta przesun(const Prosta& p, Koniec ktory, PunktUI cel, const Blokady& b);

/// Krzywa mocy admisyjnej Ia = Pa / Ua [mA] w punktach napięcia od @p odV do @p doV (@p n >= 2 punktów)
std::vector<PunktUI> krzywaPaMax(double paW, double odV, double doV, int n);

} // namespace MetodyGraficzne

#endif // METODYGRAFICZNE_H
