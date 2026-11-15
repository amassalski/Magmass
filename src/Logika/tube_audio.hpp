// tube_audio.hpp
// ============================================================================
// Moduł obliczeń inżynierskich dla układów lampowych audio (małosygnałowych)
//
// Literatura i źródła teoretyczne:
//  [1] G. S. Cykin, "Wzmacniacze elektroniczne", WKiŁ 1970
//  [2] A. P. Łożnikow, E. K. Sonin, "Wzmacniacze kaskodowe", WKiŁ 1968
//  [3] A. Zawada, "Lampy elektronowe w aplikacjach audio", Wydawnictwo BTC 2011
//  [4] Morgan Jones, "Valve Amplifiers", 4th ed., Focal Press 2012
//  [5] Merlin Blencowe, "Designing High-Fidelity Tube Preamps", 2016
//  [6] John Broskie, Tube CAD Journal (analizy SRPP, WCF, Mu-Follower)
//  [7] F. Langford-Smith, "Radiotron Designer's Handbook" (RDH4), RCA/AWV 1953
//  [8] G. E. Valley, H. Wallman, "Vacuum Tube Amplifiers", MIT Rad Lab Vol. 18, 1948
//  [9] E. L. C. White, British Patent 564,250 (White Cathode Follower, 1940)
// [10] A. D. Blumlein, British Patent 482,740 (Long-Tailed Pair, 1936)
//
// UWAGA DOTYCZĄCA JEDNOSTEK (układ SI):
//  - Rezystancje i impedancje: omy [Ω]
//  - Nachylenia charakterystyk (Sa, Sk, gm): amper na wolt [A/V] (np. 2.5 mA/V = 0.0025 A/V)
//  - Pojemności międzyelektrodowe: farady [F] (np. 2.2 pF = 2.2e-12 F)
//  - Pulsacja (omega): radiany na sekundę [rad/s] (omega = 2 * pi * f)
// ============================================================================
#pragma once

#include <cmath>
#include <limits>
#include <stdexcept>

namespace tube {

// ============================================================================
// 1. PODSTAWOWE STRUKTURY PARAMETRÓW LAMP
// ============================================================================

/// @brief Parametry małosygnałowe triody w punkcie pracy (równanie Barkhausena: mu = Sa * rho)
struct TriodeParams {
    double Ka;   ///< Współczynnik amplifikacji (mu / Ka) [-]
    double rho;  ///< Wewnętrzna oporność anodowa (rp / ra / rho) [Ω]
    double Sa;   ///< Nachylenie charakterystyki anodowej (gm / Sa) [A/V]

    static constexpr TriodeParams fromKaRho(double Ka, double rho) {
        if (rho <= 0.0) throw std::invalid_argument("rho musi być > 0");
        return {Ka, rho, Ka / rho};
    }

    static constexpr TriodeParams fromSaRho(double Sa, double rho) {
        if (rho <= 0.0 || Sa <= 0.0) throw std::invalid_argument("Sa i rho muszą być > 0");
        return {Sa * rho, rho, Sa};
    }
};

/// @brief Parametry małosygnałowe pentody w punkcie pracy
struct PentodeParams {
    double Ka;     ///< Współczynnik amplifikacji względem siatki 1 [-]
    double rho;    ///< Wewnętrzna oporność anodowa (rp / rho) [Ω]
    double Sa;     ///< Nachylenie charakterystyki anodowej względem s1 (gm / Sa) [A/V]
    double Ss2;    ///< Nachylenie siatki drugiej względem s1 (gm_s2) [A/V] (opcjonalne)
    double rho_s2; ///< Oporność wewnętrzna siatki drugiej [Ω] (opcjonalne)

    static constexpr PentodeParams fromKaRho(double Ka, double rho) {
        if (rho <= 0.0) throw std::invalid_argument("rho musi być > 0");
        return {Ka, rho, Ka / rho, 0.0, 0.0};
    }

    static constexpr PentodeParams fromSaRho(double Sa, double rho, double Ss2 = 0.0, double rho_s2 = 0.0) {
        if (rho <= 0.0 || Sa <= 0.0) throw std::invalid_argument("Sa i rho muszą być > 0");
        return {Sa * rho, rho, Sa, Ss2, rho_s2};
    }
};

/// @brief Statyczne pojemności międzyelektrodowe lampy
struct TubeCapacitances {
    double Cas;  ///< Siatka–anoda (Cga) [F]
    double Csk;  ///< Siatka–katoda (Cgk) [F]
    double Cak;  ///< Anoda–katoda (Cak) [F]
};

// ============================================================================
// 2. KASKODOWY WTÓRNIK KATODOWY (TRIODOWY)
//    Źródło: Łożnikow/Sonin, rozdz. 3, rys. 49, s. 97; wzory 74–80
// ============================================================================

struct KaskodeFollowerResult {
    double ku;        ///< Wzmocnienie napięciowe pod obciążeniem R0 [-]
    double ku0;       // Wzmocnienie napięciowe jałowe (R0 -> inf) [-]
    double Rwy;       ///< Dokładna oporność wyjściowa stopnia [Ω]
    double RwyApprox; ///< Oporność wyjściowa przybliżona: 1 / (Ka1 * Sa2) [Ω]
};

/// @brief Oblicza parametry kaskodowego wtórnika katodowego
/// @param L1 Parametry dolnej lampy (wtórnik zasadniczy)
/// @param L2 Parametry górnej lampy (kaskoda / dynamiczne obciążenie)
/// @param Ra Rezystor w pętli sprzężenia zwrotnego [Ω]
/// @param R0 Rezystancja obciążenia [Ω]
[[nodiscard]] KaskodeFollowerResult calcKaskodeFollower(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Ra,
    double R0);

// ============================================================================
// 3. SUPERWTÓRNIK (KASKODOWY WTÓRNIK n-TEGO RZĘDU)
//    Źródło: Łożnikow/Sonin, rozdz. 3, s. 126–130; wzory 102–104
// ============================================================================

struct SuperFollowerResult {
    double ku1; ///< Wzmocnienie sygnału od wejścia do wyjścia (katoda L1) [-]
    double Rwy; ///< Wypadkowa oporność wyjściowa układu [Ω]
    double ku3; ///< Wzmocnienie od wejścia do anody L1 [-]
};

/// @brief Oblicza superwtórnik kaskodowy o n stopniach kompensujących (2n+2 lamp)
[[nodiscard]] SuperFollowerResult calcSuperFollower(
    const TriodeParams& L,
    double Ra,
    int n);

// ============================================================================
// 4. WTÓRNIK O WZMOCNIENIU RÓWNYM 1.0
//    Źródło: Łożnikow/Sonin, rozdz. 3, rys. 81a, s. 130–133; wzory 106, 108
// ============================================================================

/// @brief Wylicza krytyczną wartość oporności R2 dzielnika, przy której Rk' dąży do nieskończoności
[[nodiscard]] constexpr double calcR2Critical(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double R1)
{
    return L1.Ka * R1 - 1.0 / L2.Sa;
}

/// @brief Wylicza wartość R2 gwarantującą wzmocnienie wtórnika równe dokładnie 1.0 (Rk' = -rho1).
///        Rozwiązanie ścisłe równania (106) eliminujące błąd zecerski w druku wzoru (109).
[[nodiscard]] constexpr double calcUnityGainFollowerR2(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double R1)
{
    const double R2kr = calcR2Critical(L1, L2, R1);
    return R2kr - (1.0 / L2.Sa) - (R1 * L1.Sa / L2.Sa);
}

// ============================================================================
// 5. POJEMNOŚCI WEJŚCIOWE WTÓRNIKÓW (EFEKT MILLERA)
//    Źródła: RDH4 s. 50; Morgan Jones s. 110, 118; Łożnikow wzory 84, 90–92
// ============================================================================

/// @brief Pojemność wejściowa prostego wtórnika katodowego (anoda do masy AC)
[[nodiscard]] constexpr double calcKaskodeFollowerInputCapacitance(
    const TubeCapacitances& C, double ku, double Cs0)
{
    return C.Cas + C.Csk * (1.0 - ku) + Cs0;
}

/// @brief Pojemność wejściowa wtórnika z kompensacją anodową (bootstrapping anody)
/// @param ku1 Wzmocnienie od siatki L1 do katody L1 (wyjście)
/// @param ku3 Wzmocnienie wtórnika kompensacyjnego L2 do anody L1
[[nodiscard]] constexpr double calcCompensatedInputCapacitance(
    const TubeCapacitances& C, double ku1, double ku3, double Cs0)
{
    const double ku2 = ku1 * ku3;
    return (1.0 - ku2) * C.Cas + (1.0 - ku1) * C.Csk + Cs0;
}

// ============================================================================
// 6. STABILNOŚĆ KASKODY JAKO WZMACNIACZA W.CZ.
//    Źródła: Valley & Wallman (MIT 1948); Łożnikow rozdz. 1, wzory 18 i 20
// ============================================================================

struct StabilityLimits {
    double ku2max; ///< Maksymalne stabilne wzmocnienie stopnia o wspólnej siatce (L2) [-]
    double kumax;  ///< Maksymalne stabilne wzmocnienie całej kaskody [-]
};

/// @brief Sprawdza warunki stabilności wzmacniacza kaskodowego w.cz.
/// @param Cak2 Pojemność anoda–katoda górnej lampy L2 [F]
/// @param Cas1 Pojemność siatka–anoda dolnej lampy L1 [F]
/// @param omega Pulsacja robocza 2 * pi * f [rad/s]
/// @param ks Współczynnik stabilności (zwykle 0.85 .. 0.90) [-]
[[nodiscard]] StabilityLimits calcKaskodeStabilityLimits(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Cak2,
    double Cas1,
    double omega,
    double ks = 0.85);

// ============================================================================
// 7. KASKODOWY WTÓRNIK NA PENTODACH
//    Źródło: Łożnikow/Sonin, rozdz. 3, s. 101–103, wzory 79, 82, 83
// ============================================================================

struct PentodeFollowerResult {
    double ku;  ///< Wzmocnienie pod obciążeniem R0 [-]
    double ku0; ///< Wzmocnienie jałowe [-]
    double Rwy; ///< Oporność wyjściowa z uwzględnieniem sprzężenia przez Ra [Ω]
    double Cin; ///< Pojemność wejściowa dynamiczna [F]
};

[[nodiscard]] PentodeFollowerResult calcPentodeKaskodeFollower(
    const PentodeParams& L1,
    const PentodeParams& L2,
    double Ra,
    double R0,
    double Cs0,
    double Csk1,
    double Cas1);

// ============================================================================
// 8. µ-WTÓRNIK (MU-FOLLOWER)
//    Źródła: Alan Kimmel (Glass Audio 1993); Merlin Blencowe, rozdz. 7
// ============================================================================

struct MuFollowerResult {
    double ku;        ///< Całkowite wzmocnienie napięciowe (ok. 0.85-0.95 mu1) [-]
    double Rwy;       ///< Oporność wyjściowa na katodzie L2 [Ω]
    double Raktywne;  ///< Dynamiczna oporność obciążenia widziana przez anodę L1 [Ω]
    double Cin;       ///< Pojemność wejściowa z pełnym efektem Millera dla układu WK [F]
};

/// @brief Oblicza stopień wzmacniający Mu-Follower (WK ze źródłem prądowym na L2)
[[nodiscard]] MuFollowerResult calcMuFollower(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Rk2,
    double Cs0,
    double Csk1,
    double Cas1,
    double R0 = std::numeric_limits<double>::infinity());

// ============================================================================
// 9. SRPP (SERIES REGULATED PUSH-PULL / TOTEM POLE)
//    Źródła: John Broskie (Tube CAD 1998); Morgan Jones s. 132–138
// ============================================================================

struct SRPPResult {
    double ku0;   ///< Wzmocnienie bez obciążenia (zbliżone do mu / 2) [-]
    double ku;    ///< Wzmocnienie pod obciążeniem RL [-]
    double Rwy;   ///< Oporność wyjściowa [Ω]
    double RLopt; ///< Optymalna rezystancja obciążenia dla symetrycznej pracy push-pull [Ω]
};

/// @brief Oblicza stopień przeciwsobny SRPP
/// @param Rk Rezystor katodowy (Rk1 = Rk2 = Rk) [Ω]
/// @param RL Zewnętrzna rezystancja obciążenia [Ω]
[[nodiscard]] SRPPResult calcSRPP(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Rk,
    double RL = std::numeric_limits<double>::infinity());

// ============================================================================
// 10. WHITE CATHODE FOLLOWER (WCF)
//     Źródła: E. L. C. White (1940); Morgan Jones s. 142–146
// ============================================================================

struct WCFResult {
    double ku;    ///< Wzmocnienie napięciowe (ok. 0.98 .. 0.99) [-]
    double Rwy;   ///< Bardzo niska oporność wyjściowa (zwykle 10 .. 30 Ω) [Ω]
    double RaOpt; ///< Optymalny rezystor w anodzie L1 dla zadanego obciążenia RL [Ω]
};

/// @brief Oblicza parametry wtórnika White'a (wtórnik przeciwsobny z aktywnym pull-down)
[[nodiscard]] WCFResult calcWhiteCathodeFollower(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double RL);

// ============================================================================
// 11. LONG-TAILED PAIR (ODWRACACZ FAZY ZE SPRZĘŻENIEM KATODOWYM)
//     Źródła: A. D. Blumlein (1936); Morgan Jones s. 121–131; Cykin rozdz. 7.4.5
// ============================================================================

struct LTPResult {
    double ku1; ///< Wzmocnienie na wyjściu odwracającym (anoda 1) [-]
    double ku2; ///< Wzmocnienie na wyjściu nieodwracającym (anoda 2) [-]
    double q;   ///< Stosunek asymetrii (ku1 / ku2) [-]
    double Rks; ///< Rezystancja ogona (tail resistor) [Ω]
};

/// @brief Ścisły triodowy model odwracacza fazy LTP (Blumlein / Morgan Jones)
[[nodiscard]] LTPResult calcLTPTriode(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Ra1,
    double Ra2,
    double Rks);

/// @brief Pentodowy model LTP wg Cykina (gdzie oporność wewnętrzna rho >> Ra)
[[nodiscard]] LTPResult calcLTPPentode(
    double Sa,
    double Ra1,
    double Ra2,
    double Rks,
    double Sk);

/// @brief Wymagana wartość Ra2 dla idealnego zrównoważenia faz (ku1 == ku2)
[[nodiscard]] constexpr double calcLTPRa2ForBalance(
    const TriodeParams& L,
    double Ra1,
    double Rks)
{
    if (Rks <= 0.0) throw std::invalid_argument("Rks musi być > 0");
    const double Rprime = L.rho + Ra1;
    return Ra1 * (1.0 + Rprime / ((L.Ka + 1.0) * Rks));
}

/// @brief Wymagana wartość Rks dla zrównoważenia zadanych Ra1 i Ra2 (wymaga Ra2 > Ra1)
[[nodiscard]] double calcLTPRksForBalance(
    const TriodeParams& L,
    double Ra1,
    double Ra2);

struct LTPDiffResult {
    double Ad;   ///< Wzmocnienie różnicowe [-]
    double Acm;  ///< Wzmocnienie współbieżne [-]
    double CMRR; ///< Współczynnik tłumienia sygnału współbieżnego (liniowy) [-]
};

/// @brief Oblicza parametry różnicowe i tłumienie sygnału współbieżnego (CMRR)
[[nodiscard]] LTPDiffResult calcLTPDiff(const LTPResult& r);

} // namespace tube
