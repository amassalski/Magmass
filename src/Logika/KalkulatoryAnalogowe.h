#ifndef KALKULATORYANALOGOWE_H
#define KALKULATORYANALOGOWE_H

#include "SzeregiE.h"

#include <optional>
#include <string>

/// Kalkulatory zasilaczy, transformatorów oraz punktów pracy tranzystora i lampy (bez GUI i bazy).
/// Jednostki: V, A, Ω, F, Hz, W (chyba że w nazwie pola jest mA, mW itp.).
namespace Kalkulatory {

// ---------------------------------------------------------------- transformator sieciowy / zasilacz
enum class Prostownik { Mostek, DwiePolowki, Jednopolowkowy };

struct WynikZasilacza {
    double uSkutecznaWtorna = 0;   ///< V: napięcie uzwojenia wtórnego (całego dla mostka, każdej połówki dla odczepu)
    double uSzczytowa = 0;         ///< V: napięcie szczytowe na kondensatorze (po spadku na diodach)
    double uSrednia = 0;           ///< V: średnie napięcie wyjściowe (szczyt - połowa tętnień)
    double tetnieniaPP = 0;        ///< V: tętnienia międzyszczytowe
    double czestTetnien = 0;       ///< Hz
    double pradWtornyA = 0;        ///< A: skuteczny prąd uzwojenia wtórnego (przybliżenie dla filtra pojemnościowego)
    double mocTrafoVA = 0;         ///< VA: zalecana moc transformatora
    double mocDCW = 0;             ///< W: moc oddawana na wyjściu
    double pradSkokowyDiodyA = 0;  ///< A: orientacyjny szczytowy prąd diody (zakres: rząd wielkości, zależy od rezystancji uzwojeń)
};
/// Dla zadanego napięcia wtórnego, prądu obciążenia, kondensatora filtrującego C i sieci @p fSieci.
/// @p spadekDiodyV spadek na jednej diodzie (mostek: dwie diody szeregowo). nullopt dla błędnych danych.
std::optional<WynikZasilacza> zasilaczZTrafa(double uWtorneV, double pradA, double cF, Prostownik p,
                                            double fSieci = 50.0, double spadekDiodyV = 0.7);

/// Odwrotnie: jakie napięcie wtórne potrzebne, aby po wyprostowaniu i odjęciu tętnień dostać @p uWyjsciaV (średnie).
std::optional<double> napiecieWtorneDlaWyjscia(double uWyjsciaV, double tetnieniaPPV, Prostownik p, double spadekDiodyV = 0.7);

/// Kondensator filtrujący dla dopuszczalnych tętnień: C = I / (k f Upp), k = 1 (jednopołówkowy) lub 2.
double kondensatorFiltra(double pradA, double tetnieniaPPV, Prostownik p, double fSieci = 50.0);

// ---------------------------------------------------------------- ESR kondensatorów elektrolitycznych
/// Typowy współczynnik strat tg δ (przy 120 Hz) aluminiowego kondensatora elektrolitycznego ogólnego przeznaczenia
/// w zależności od napięcia znamionowego [V]; interpolacja liniowa między wartościami katalogowymi (rząd wielkości, nie gwarancja).
double tgDeltaTypowy(double napiecieV);
/// ESR [Ω] z tg δ: ESR = tgδ / (2π f C), C w faradach, f w Hz (domyślnie 120 Hz). 0 dla błędnych danych.
double esrZTgDelta(double tgDelta, double pojemnoscF, double fHz = 120.0);
/// Typowy ESR [Ω] kondensatora o pojemności @p pojemnoscF i napięciu @p napiecieV: esrZTgDelta(tgDeltaTypowy(U), C, f).
double esrTypowy(double pojemnoscF, double napiecieV, double fHz = 120.0);
/// Moc strat [W] od prądu tętnień: P = I² · ESR.
double mocStratESR(double pradTetnienA, double esrOm);

// ---------------------------------------------------------------- tranzystor bipolarny NPN: punkt pracy (dzielnik bazowy + RE)
enum class StanTranzystora { Aktywny, Nasycenie, Zatkany };

struct WynikTranzystora {
    double ibUA = 0;               ///< µA
    double icMA = 0;               ///< mA
    double vceV = 0;               ///< V
    double veV = 0;                ///< V na emiterze
    double vbV = 0;                ///< V na bazie
    double mocTranzystoraMW = 0;   ///< Vce * Ic
    StanTranzystora stan = StanTranzystora::Aktywny;
    double wzmocnienieNapiecioweOE = 0;   ///< przybliżone, z RE całkowicie zblokowanym kondensatorem: gm * RC (ujemne dla odwrócenia fazy nie uwzględniamy)
};
/// Punkt pracy. @p r1 do plusa, @p r2 do masy, @p rc w kolektorze, @p re w emiterze, @p beta wzmocnienie prądowe.
/// R1 = 0 oznacza brak dzielnika do plusa (baza z samego R2 nie ma sensu, więc nullopt). nullopt dla błędnych danych.
std::optional<WynikTranzystora> punktPracyTranzystora(double vcc, double r1, double r2, double rc, double re,
                                                      double beta, double vbe = 0.65);

struct ProjektTranzystora {
    double rc = 0, re = 0, r1 = 0, r2 = 0;   ///< Ω (po dopasowaniu do szeregu)
    WynikTranzystora wynik;
};
/// Dobiera wartości dla zadanego prądu kolektora [mA]: VE = 10 % Vcc, VCE = Vcc/2, prąd dzielnika ~ 10 Ib.
std::optional<ProjektTranzystora> dobierzTranzystor(double vcc, double icMA, double beta, Szereg s = Szereg::E24, double vbe = 0.65);

// ---------------------------------------------------------------- lampa (trioda / pentoda): polaryzacja katodowa i linia obciążenia
struct DaneLampy {
    double bPlus = 250;            ///< V: napięcie zasilania anodowego
    double ra = 0;                 ///< Ω: opór obciążenia anodowego (lub impedancja transformatora)
    double iaMA = 1;               ///< mA: prąd anody w spoczynku
    double ugV = -2;               ///< V: napięcie siatki sterującej względem katody (ujemne)
    double ig2MA = 0;              ///< mA: prąd siatki ekranującej (0 dla triody)
    double ug2V = 0;               ///< V: napięcie ekranu względem katody (pentoda), 0 = pomiń
    double mi = 0;                 ///< współczynnik amplifikacji (0 = pomiń wzmocnienie)
    double rWewnAnodowaOm = 0;     ///< Ω: rezystancja wewnętrzna lampy ra
    double paMaxW = 0;             ///< W: dopuszczalna moc anody (0 = nie sprawdzaj)
    double fDolnaHz = 0;           ///< Hz: do doboru kondensatora katodowego (0 = pomiń)
};
struct WynikLampy {
    double uAnodyV = 0;            ///< V między anodą a katodą
    double uKatodyV = 0;           ///< V na katodzie względem masy = |Ug| (siatka na masie)
    double rKatodyOm = 0;          ///< Ω: |Ug| / (Ia + Ig2)
    double mocRKatodyW = 0;        ///< W
    double mocAnodyW = 0;          ///< W: Ua * Ia
    bool przekroczonaMocAnody = false;
    double rEkranuOm = 0;          ///< Ω: (B+ - Ug2 - Uk) / Ig2, 0 gdy nie dotyczy
    double wzmocnienie = 0;        ///< mi * Ra / (ra + Ra)
    double ckF = 0;                ///< F: kondensator katodowy dla dolnej częstotliwości
    double iaMaxMA = 0;            ///< mA: prąd zwarcia linii obciążenia (Ua = 0): B+ / Ra
};
/// nullopt, gdy napięcie na lampie byłoby <= 0 lub dane są błędne.
std::optional<WynikLampy> punktPracyLampy(const DaneLampy& d);

} // namespace Kalkulatory

#endif // KALKULATORYANALOGOWE_H
