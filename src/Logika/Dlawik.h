#ifndef DLAWIK_H
#define DLAWIK_H

#include <optional>
#include <string>

/// Dławik zasilacza z prądem stałym i szczeliną (rdzeń EI), jednostki jak w ECCLab: mm, H, mA, Gs/Oe.
/// Obliczenia wykonuje tube::magnetics::calcFilterChoke (Hanna, RDH4, Cykin).
namespace Dlawik {

struct Rdzen {
    double A = 0, B = 0;           ///< wymiary zewnętrzne [mm]
    double a = 0;                  ///< szerokość kolumny środkowej [mm]
    double b = 0;                  ///< grubość pakietu [mm]
    double c = 0;                  ///< szerokość okna [mm]
    double d = 0;                  ///< szerokość kolumny bocznej [mm]
    double h = 0;                  ///< wysokość okna [mm]
    double blachaMm = 0.5;         ///< grubość blachy (wpływa na współczynnik wypełnienia żelazem)
};

/// Standardowa kształtka EI o kolumnie środkowej a (A = 3a, B = 2,5a, c = d = a/2, h = 1,5a); grubość pakietu @p pakietMm
Rdzen ksztaltkaEI(double aMm, double pakietMm, double blachaMm = 0.5);
/// Kształtka z nazwy ("EI102" -> a = 34 mm); nullopt dla nieznanej
std::optional<Rdzen> ksztaltkaZNazwy(const std::string& nazwa, double pakietMm, double blachaMm = 0.5);

struct Wejscie {
    Rdzen rdzen;
    double indukcyjnoscH = 0;
    double pradMa = 0;
    double mi = 500;               ///< przenikalność magnetyczna żelaza [Gs/Oe]
    double bMaxT = 1.0;            ///< dopuszczalna indukcja od prądu stałego [T]
    double gestoscAmm2 = 2.55;     ///< gęstość prądu [A/mm²]
    double drutMm = 0;             ///< wymuszona średnica gołego drutu; 0 = dobrana z gęstości
};

struct Wynik {
    // rdzeń
    double dlugoscStrumieniaCm = 0, sredniaDlugoscZwojuCm = 0, przekrojCzynnyCm2 = 0, objetoscCzynnaCm3 = 0;
    // dławik
    double zalecanaObjetoscCm3 = 0; ///< "Zalecana objętość rdzenia" jak w ECCLab (Krize [35]: L·I0²·10⁻³/2, L w H, I0 w mA)
    double energiaMJ = 0;          ///< ½ L I² [mJ]
    int zwoje = 0;
    double drutGolyMm = 0, drutEmaliaMm = 0;
    double szczelinaMm = 0;        ///< całkowita szczelina powietrzna: μ0·N·I/B - lFe/μ (fizyczna)
    double przekladkaMm = 0;       ///< grubość przekładki pod kolumny (połowa szczeliny)
    double szczelinaKrizeMm = 0;   ///< szczelina wg Krize'a [38] / ECCLab (bez odejmowania reluktancji żelaza), tylko do porównania
    double opornoscOm = 0, oporNaMetrOm = 0;
    double spadekV = 0;
    double wypelnieniePct = 0;
    bool miesciSie = false;        ///< wypełnienie okna do 75%
    enum class Ostrzezenie { brak, zaMaloWypelnione, zaDuzoWypelnione };
    Ostrzezenie ostrzezenie = Ostrzezenie::brak;   ///< poniżej 50% rdzeń nie jest optymalnie wypełniony, powyżej 75% uzwojenie się nie mieści
    double zwojeNaCm2 = 0;         ///< ile zwojów drutu w emalii mieści się na 1 cm²
    double pradDlaGestosciMa = 0;  ///< prąd dopuszczalny dla zadanej gęstości przy użytym drucie [mA]
    double indukcyjnoscRzeczH = 0;
    double indukcjaT = 0;          ///< indukcja od składowej stałej
};

std::optional<Wynik> oblicz(const Wejscie& w);

} // namespace Dlawik

#endif
