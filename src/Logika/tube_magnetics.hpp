// tube_magnetics.hpp
// ============================================================================
// Moduł obliczeń inżynierskich elementów magnetycznych dla techniki lampowej:
//  1. Transformatory sieciowe (zasilające)
//  2. Dławiki filtracyjne zasilaczy (z prądem stałym DC i szczeliną)
//  3. Transformatory głośnikowe Single-Ended (SE)
//  4. Transformatory głośnikowe Push-Pull (PP)
//
// Literatura i źródła teoretyczne:
//  [1] G. S. Cykin, "Transformatory małej częstotliwości", WKiŁ 1968 / PNT
//  [2] G. S. Cykin, "Wzmacniacze elektroniczne", WKiŁ 1970
//  [3] F. Langford-Smith, "Radiotron Designer's Handbook" (RDH4), RCA/AWV 1953, rozdz. 5
//  [4] Morgan Jones, "Valve Amplifiers", 4th ed., Focal Press 2012, rozdz. 5 i 6
//  [5] C. R. Hanna, "Design of Reactances and Transformers Which Carry Direct Current", AIEE 1927
//  [6] Reuben Lee, "Electronic Transformers and Circuits", John Wiley & Sons 1955
//  [7] A. Zawada, "Lampy elektronowe w aplikacjach audio", BTC 2011
//  [8] W. Mikołajczyk, "Transformatory i dławiki w urządzeniach elektronicznych", WNT 1978
//
// UWAGA DOTYCZĄCA JEDNOSTEK (układ SI):
//  - Wymiary geometryczne rdzenia: metry [m] (np. 32 mm = 0.032 m)
//  - Powierzchnie przekrojów: metry kwadratowe [m²] (1 cm² = 1e-4 m²)
//  - Indukcja magnetyczna B: tesle [T] (np. 1.2 T)
//  - Indukcyjność: heny [H]
//  - Rezystancje i impedancje: omy [Ω]
//  - Prądy: ampery [A] (np. 60 mA = 0.060 A)
//  - Napięcia: wolty [V]
//  - Częstotliwości: herce [Hz]
//  - Gęstość prądu J: amper na metr kwadratowy [A/m²] (2.5 A/mm² = 2.5e6 A/m²)
// ============================================================================
#pragma once

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
#include <string>

namespace tube::magnetics {

// Stałe fizyczne
inline constexpr double MU_0 = 4.0 * 3.14159265358979323846e-7; // przenikalność próżni [H/m]
inline constexpr double RHO_CU_20C = 1.72e-8;                    // rezystywność miedzi w 20°C [Ω*m]
inline constexpr double RHO_CU_75C = 2.10e-8;                    // rezystywność miedzi w 75°C (robocza) [Ω*m]

// ============================================================================
// 1. STRUKTURY GEOMETRII RDZENIA I MATERIAŁU
// ============================================================================

/// @brief Geometria i właściwości magnetyczne rdzenia transformatora / dławika
struct CoreGeometry {
    double a;    ///< Szerokość kolumny środkowej / języka [m]
    double b;    ///< Szerokość okna karkasu [m]
    double c;    ///< Wysokość okna karkasu [m]
    double g;    ///< Grubość pakietu blach [m]
    double kFe;  ///< Współczynnik wypełnienia rdzenia żelazem (zwykle 0.90..0.95) [-]
    double lFe;  ///< Średnia długość drogi magnetycznej strumienia [m]

    /// @brief Geometryczne pole przekroju kolumny środkowej [m²]
    [[nodiscard]] constexpr double Sgeom() const { return a * g; }

    /// @brief Czynne pole przekroju magnetycznego żelaza Qfe = Sgeom * kFe [m²]
    [[nodiscard]] constexpr double Qfe() const { return a * g * kFe; }

    /// @brief Geometryczna powierzchnia okna karkasu [m²]
    [[nodiscard]] constexpr double Qo() const { return b * c; }

    /// @brief Objętość czynna rdzenia żelaznego [m³]
    [[nodiscard]] constexpr double Vfe() const { return Qfe() * lFe; }

    /// @brief Przybliżona masa rdzenia stalowego [kg] (gęstość stali 7650 kg/m³)
    [[nodiscard]] constexpr double massKg() const { return Vfe() * 7650.0; }

    /// @brief Przybliżona długość średniego zwoju dla karkasu [m]
    [[nodiscard]] constexpr double meanTurnLength() const {
        return 2.0 * (a + g) + 3.14159265358979323846 * b * 0.5;
    }

    // --- FABRYKI STANDARDOWYCH KSZTAŁTEK EI (wg PN/DIN 41302) ---

    /// @brief Kształtka EI 66 (a=22mm, b=11mm, c=33mm), grubość pakietu g [m]
    static constexpr CoreGeometry makeEI66(double gPackMeters, double kFe = 0.92) {
        return {0.022, 0.011, 0.033, gPackMeters, kFe, 0.132};
    }

    /// @brief Kształtka EI 84 (a=28mm, b=14mm, c=42mm), grubość pakietu g [m]
    static constexpr CoreGeometry makeEI84(double gPackMeters, double kFe = 0.93) {
        return {0.028, 0.014, 0.042, gPackMeters, kFe, 0.168};
    }

    /// @brief Kształtka EI 96 (a=32mm, b=16mm, c=48mm), grubość pakietu g [m]
    static constexpr CoreGeometry makeEI96(double gPackMeters, double kFe = 0.93) {
        return {0.032, 0.016, 0.048, gPackMeters, kFe, 0.192};
    }

    /// @brief Kształtka EI 105 (a=35mm, b=17.5mm, c=52.5mm), grubość pakietu g [m]
    static constexpr CoreGeometry makeEI105(double gPackMeters, double kFe = 0.94) {
        return {0.035, 0.0175, 0.0525, gPackMeters, kFe, 0.210};
    }

    /// @brief Kształtka EI 120 (a=40mm, b=20mm, c=60mm), grubość pakietu g [m]
    static constexpr CoreGeometry makeEI120(double gPackMeters, double kFe = 0.94) {
        return {0.040, 0.020, 0.060, gPackMeters, kFe, 0.240};
    }

    /// @brief Kształtka EI 150 (a=50mm, b=25mm, c=75mm), grubość pakietu g [m]
    static constexpr CoreGeometry makeEI150(double gPackMeters, double kFe = 0.95) {
        return {0.050, 0.025, 0.075, gPackMeters, kFe, 0.300};
    }
};

/// @brief Pomocnik do obliczania średnicy i przekroju drutu nawojowego
struct WireCalc {
    /// @brief Przekrój miedzi drutu qCu dla zadanego prądu skutecznego I i gęstości J [m²]
    [[nodiscard]] static constexpr double copperArea(double Irms, double J_A_per_m2) {
        return Irms / J_A_per_m2;
    }

    /// @brief Średnica gołego drutu nawojowego [m]
    [[nodiscard]] static inline double bareDiameter(double Irms, double J_A_per_m2) {
        const double q = copperArea(Irms, J_A_per_m2);
        return 2.0 * std::sqrt(q / 3.14159265358979323846);
    }

    /// @brief Przybliżona średnica drutu z emalią (izolacja podwójna Grade 2) [m]
    [[nodiscard]] static inline double enamelledDiameter(double d_bare) {
        if (d_bare <= 0.0002) return d_bare + 0.03e-3;
        if (d_bare <= 0.0005) return d_bare + 0.04e-3;
        if (d_bare <= 0.0010) return d_bare + 0.05e-3;
        return d_bare + 0.07e-3;
    }

    /// @brief Rezystancja odcinka drutu miedzianego w temperaturze roboczej [Ω]
    [[nodiscard]] static constexpr double resistance(double lengthMeters, double qCuMeters2, double rhoCu = RHO_CU_75C) {
        return rhoCu * lengthMeters / qCuMeters2;
    }
};

// ============================================================================
// 2. TRANSFORMATOR SIECIOWY (POWER TRANSFORMER)
//    Źródła: Cykin rozdz. 11; RDH4 rozdz. 5.2; Morgan Jones rozdz. 5
// ============================================================================

/// @brief Specyfikacja pojedynczego uzwojenia wtórnego transformatora sieciowego
struct MainsSecondarySpec {
    std::string name;  ///< Nazwa (np. "Anodowe HT", "Zarzenie 6.3V", "Bias")
    double U_rms;      ///< Napięcie znamionowe skuteczne pod obciążeniem [V]
    double I_rms;      ///< Prąd znamionowy skuteczny [A]
    double dropFactor; ///< Współczynnik kompensacji spadku napięcia na miedzi (zwykle 1.05..1.08) [-]
};

struct MainsWindingResult {
    std::string name;
    int turns;          ///< Liczba zwojów
    double wireDiaBare; ///< Średnica gołego drutu [m]
    double wireDiaIso;  ///< Średnica drutu w emalii [m]
    double wireAreaCu;  ///< Przekrój miedzi [m²]
    double R_cu;        ///< Przybliżona rezystancja uzwojenia w 75°C [Ω]
    double P_va;        ///< Moc pozorna uzwojenia [VA]
};

struct MainsTransformerResult {
    double P_sec_total;     ///< Sumaryczna moc pozorna strony wtórnej [VA]
    double P_core_calc;     ///< Moc obliczeniowa rdzenia uwzględniająca sprawność [VA]
    double turnsPerVolt;    ///< Liczba zwojów na wolt z_v [zw/V]
    MainsWindingResult primary; ///< Dane uzwojenia pierwotnego
    std::vector<MainsWindingResult> secondaries; ///< Dane uzwojeń wtórnych
    double windowFillFactor;///< Współczynnik wypełnienia okna miedzią k_Cu [-]
    bool fitsInWindow;      ///< Czy uzwojenia zmieszczą się w oknie karkasu (k_Cu <= k_dop)
    double efficiency;      ///< Przyjęta sprawność transformatora [-]
};

/// @brief Oblicza transformator sieciowy na zadanym rdzeniu
/// @param core Geometria wybranego rdzenia
/// @param U_prim Napięcie sieci zasilającej (np. 230 V) [V]
/// @param freq Częstotliwość sieci (np. 50 Hz) [Hz]
/// @param Bmax Maksymalna robocza indukcja magnetyczna (zwykle 1.0..1.2 T dla EI, 1.4..1.5 T dla RGO/toroidu) [T]
/// @param J_density Gęstość prądu w uzwojeniach (zwykle 2.0..2.5 A/mm² = 2.0e6..2.5e6 A/m²) [A/m²]
/// @param secondaries Lista uzwojeń wtórnych
/// @param efficiency Sprawność transformatora (domyślnie 0.88 dla 100-200 VA)
/// @param maxFillFactor Dopuszczalny współczynnik wypełnienia okna karkasu miedzią (zwykle 0.32..0.38)
[[nodiscard]] MainsTransformerResult calcMainsTransformer(
    const CoreGeometry& core,
    double U_prim,
    double freq,
    double Bmax,
    double J_density,
    const std::vector<MainsSecondarySpec>& secondaries,
    double efficiency = 0.88,
    double maxFillFactor = 0.35);

/// @brief Wyprowadza wymagany prąd i napięcie AC uzwojenia anodowego dla prostownika mostkowego Graetza
/// @param U_dc Wymagane napięcie stałe wyjściowe za pierwszym kondensatorem [V]
/// @param I_dc Wymagany prąd stały obciążenia [A]
/// @param V_rect_drop Spadek napięcia na diodach/lampie prostowniczej [V]
[[nodiscard]] MainsSecondarySpec makeSecondaryForBridgeRectifier(
    const std::string& name,
    double U_dc,
    double I_dc,
    double V_rect_drop = 2.0);

/// @brief Wyprowadza uzwojenie anodowe z odczepem środkowym (Full-Wave Center-Tapped)
/// @param U_dc Wymagane napięcie stałe wyjściowe za filtrem [V]
/// @param I_dc Wymagany prąd stały obciążenia [A]
/// @param V_rect_drop Spadek napięcia na lampie prostowniczej (np. 30 V dla 5U4G, 15 V dla GZ34) [V]
[[nodiscard]] MainsSecondarySpec makeSecondaryForCenterTappedRectifier(
    const std::string& name,
    double U_dc,
    double I_dc,
    double V_rect_drop = 25.0);

// ============================================================================
// 3. DŁAWIK FILTRACYJNY Z PRĄDEM DC I SZCZELINĄ (FILTER CHOKE)
//    Źródła: C. R. Hanna (AIEE 1927); RDH4 rozdz. 5.3; Cykin rozdz. 10
// ============================================================================

struct FilterChokeResult {
    double L_actual;      ///< Rzeczywista uzyskana indukcyjność znamionowa [H]
    int turns;            ///< Wymagana liczba zwojów N [-]
    double airGapTotal;   ///< Całkowita szczelina niemagnetyczna w obwodzie l_p [m]
    double spacerThickness;///< Grubość przekładki niemagnetycznej delta = l_p / 2 [m]
    double wireDiaBare;   ///< Średnica gołego drutu nawojowego [m]
    double wireDiaIso;    ///< Średnica drutu w emalii [m]
    double R_dc;          ///< Rezystancja uzwojenia dla prądu stałego w 75°C [Ω]
    double V_drop_dc;     ///< Spadek napięcia stałego na dławiku: I_dc * R_dc [V]
    double P_dissipated;  ///< Moc tracona w miedzi dławika [W]
    double energyJoules;  ///< Zgromadzona energia magnetyczna: 0.5 * L * I_dc² [J]
    double windowFillFactor; ///< Współczynnik wypełnienia okna miedzią [-]
    bool fitsInWindow;    ///< Czy uzwojenie zmieści się w oknie karkasu
};

/// @brief Oblicza dławik zasilacza z prądem DC i niemagnetyczną szczeliną powietrzną
/// @param core Geometria rdzenia
/// @param L_target Wymagana indukcyjność [H] (np. 10 H)
/// @param I_dc Znamionowy prąd stały [A] (np. 0.150 A = 150 mA)
/// @param Bdc_max Maksymalna dopuszczalna indukcja od składowej stałej (zwykle 0.7..0.85 T) [T]
/// @param J_density Gęstość prądu (zwykle 2.0..2.5 A/mm²) [A/m²]
/// @param maxFillFactor Dopuszczalne wypełnienie okna (zwykle 0.35..0.40)
[[nodiscard]] FilterChokeResult calcFilterChoke(
    const CoreGeometry& core,
    double L_target,
    double I_dc,
    double Bdc_max = 0.80,
    double J_density = 2.2e6,
    double maxFillFactor = 0.38,
    double mu_r = 800.0);   ///< względna przenikalność żelaza (w CGS: Gs/Oe) przy podmagnesowaniu

/// @brief Oblicza minimalną krytyczną indukcyjność dławika dla zasilacza z wejściem dławikowym (choke-input)
///        Źródło: Morgan Jones s. 343; RDH4 s. 1182
/// @param U_dc Napięcie stałe zasilacza [V]
/// @param I_dc Minimalny prąd pobierany przez układ [A]
/// @param mainsFreq Częstotliwość sieciowa (np. 50 Hz) [Hz]
[[nodiscard]] constexpr double calcCriticalChokeInductance(
    double U_dc,
    double I_dc,
    double mainsFreq = 50.0)
{
    if (I_dc <= 0.0) throw std::invalid_argument("I_dc musi być > 0");
    const double R_load = U_dc / I_dc;
    // Dla prostownika 2-połówkowego pulsacja tętnień omega_ripple = 2 * pi * (2 * f):
    const double omega_ripple = 4.0 * 3.14159265358979323846 * mainsFreq;
    // Wzór klasyczny L_crit >= R_load / (3 * omega_ripple) lub przybliżony R_load / 942 dla 50Hz:
    return R_load / (1.5 * omega_ripple);
}

// ============================================================================
// 4. TRANSFORMATOR GŁOŚNIKOWY SINGLE-ENDED (SE OUTPUT TRANSFORMER)
//    Źródła: Cykin rozdz. 7 i 8; RDH4 rozdz. 5.4; Zawada rozdz. 5; Morgan Jones rozdz. 6
// ============================================================================

struct SETransformerResult {
    double turnsRatio;     ///< Przekładnia zwojowa n = N1 / N2 [-]
    int N1;                ///< Liczba zwojów uzwojenia pierwotnego [-]
    int N2;                ///< Liczba zwojów uzwojenia wtórnego [-]
    double L_prim;         ///< Indukcyjność główna uzwojenia pierwotnego Lp [H]
    double f_low_actual;   ///< Rzeczywista dolna częstotliwość graniczna (-3 dB) [Hz]
    double airGapTotal;    ///< Całkowita szczelina niemagnetyczna l_p [m]
    double spacerThickness;///< Grubość przekładki pod rdzeń delta = l_p / 2 [m]
    double d1_bare;        ///< Średnica gołego drutu pierwotnego [m]
    double d2_bare;        ///< Średnica gołego drutu wtórnego [m]
    double R1_dc;          ///< Rezystancja uzwojenia pierwotnego w 75°C [Ω]
    double R2_dc;          ///< Rezystancja uzwojenia wtórnego w 75°C [Ω]
    double efficiency;     ///< Sprawność transformatora eta = P_out / P_in [-]
    double Bac_max;        ///< Amplituda indukcji zmiennej przy pełnej mocy na dolnej częstotliwości [T]
    double Bdc;            ///< Indukcja składowej stałej podmagnesowania prądem Ia0 [T]
    double B_total_peak;   ///< Maksymalna szczytowa indukcja w rdzeniu Bdc + Bac [T]
    double L_leakage_max;  ///< Dopuszczalna indukcja rozproszenia Ls dla zadanego f_high [H]
    int minInterleavingSections; ///< Zalecana minimalna liczba sekcji uzwojeń (np. 3 dla P-S-P) [-]
    double windowFillFactor;///< Współczynnik wypełnienia okna karkasu [-]
    bool fitsInWindow;     ///< Czy uzwojenia i izolacje zmieszczą się w oknie karkasu
};

/// @brief Oblicza transformator głośnikowy dla stopnia Single-Ended z podmagnesowaniem DC
/// @param core Geometria rdzenia (musi mieć odpowiednią objętość dla zgromadzenia energii)
/// @param Ra Optymalna impedancja obciążenia anody lampy (np. 3500 Ω dla 300B, 5000 Ω dla EL34) [Ω]
/// @param ra Wewnętrzna oporność dynamiczna lampy w punkcie pracy (np. 800 Ω dla 300B, 2000 Ω dla KT88 trioda) [Ω]
/// @param R_spk Impedancja znamionowa głośnika (np. 4, 8 lub 16 Ω) [Ω]
/// @param P_out Znamionowa moc wyjściowa stopnia [W]
/// @param Ia0 Prąd spoczynkowy anody (składowa stała magnesująca rdzeń) [A]
/// @param f_low Wymagana dolna częstotliwość pasma przy pełnej mocy [Hz] (np. 30 Hz)
/// @param f_high Wymagana górna częstotliwość pasma (-3 dB) [Hz] (np. 30000 Hz)
/// @param Bdc Indukcja składowej stałej (zwykle 0.6..0.75 T) [T]
/// @param J_density Gęstość prądu w miedzi (zwykle 2.2..2.8 A/mm²) [A/m²]
[[nodiscard]] SETransformerResult calcSETransformer(
    const CoreGeometry& core,
    double Ra,
    double ra,
    double R_spk,
    double P_out,
    double Ia0,
    double f_low = 30.0,
    double f_high = 30000.0,
    double Bdc = 0.70,
    double J_density = 2.5e6);

// ============================================================================
// 5. TRANSFORMATOR GŁOŚNIKOWY PUSH-PULL (PP OUTPUT TRANSFORMER)
//    Źródła: Cykin rozdz. 7 i 9; RDH4 rozdz. 5.4; Morgan Jones rozdz. 6
// ============================================================================

struct PPTransformerResult {
    double turnsRatio;       ///< Przekładnia całkowita anoda-anoda do głośnika n = N1 / N2 [-]
    int N1_total;            ///< Całkowita liczba zwojów pierwotnych anoda-anoda (N1a + N1b) [-]
    int N1_half;             ///< Liczba zwojów jednej połówki (od odczepu środkowego B+ do anody) [-]
    int N1_ultralinear;      ///< Liczba zwojów do odczepu Ultra-Linear (jeśli dotyczy) [-]
    int N2;                  ///< Liczba zwojów uzwojenia wtórnego [-]
    double L_prim;           ///< Indukcyjność główna anoda-anoda Lp [H]
    double f_low_actual;     ///< Rzeczywista dolna częstotliwość graniczna (-3 dB) [Hz]
    double d1_bare;          ///< Średnica gołego drutu pierwotnego [m]
    double d2_bare;          ///< Średnica gołego drutu wtórnego [m]
    double R1_half_dc;       ///< Rezystancja jednej połówki uzwojenia pierwotnego w 75°C [Ω]
    double R2_dc;            ///< Rezystancja uzwojenia wtórnego w 75°C [Ω]
    double efficiency;       ///< Sprawność transformatora eta [-]
    double Bac_max;          ///< Amplituda indukcji roboczej przy pełnej mocy na dolnej częstotliwości [T]
    double techAirGapTotal;  ///< Technologiczna mikroszczelina zabezpieczająca przed asymetrią lamp [m]
    double spacerThickness;  ///< Grubość przekładki technologicznej delta = l_p / 2 [m]
    double L_leakage_max;    ///< Dopuszczalna indukcyjność rozproszenia anoda-anoda dla zadanego f_high [H]
    int recommendedSections; ///< Zalecana liczba sekcji dla układu symetrycznego (np. 4P + 3S) [-]
    double windowFillFactor; ///< Współczynnik wypełnienia okna karkasu [-]
    bool fitsInWindow;       ///< Czy uzwojenia zmieszczą się w oknie karkasu
};

/// @brief Oblicza transformator głośnikowy dla stopnia przeciwsobnego Push-Pull
/// @param core Geometria rdzenia
/// @param Raa Impedancja anoda-anoda (np. 6600 Ω dla 2x EL34, 4000 Ω dla 2x 6L6/KT88, 5000 Ω dla 2x 2A3) [Ω]
/// @param ra Wewnętrzna oporność dynamiczna pojedynczej lampy w punkcie pracy [Ω]
/// @param R_spk Impedancja głośnika (np. 4, 8 lub 16 Ω) [Ω]
/// @param P_out Znamionowa moc wyjściowa stopnia [W]
/// @param f_low Dolna częstotliwość graniczna przy pełnej mocy [Hz] (np. 20..30 Hz)
/// @param f_high Górna częstotliwość graniczna (-3 dB) [Hz] (np. 40000..60000 Hz)
/// @param delta_Ia0 Dopuszczalna asymetria prądów stałych obu lamp (np. 0.005 A = 5 mA) [A]
/// @param ultralinearTapFraction Odczep Ultra-Linear (np. 0.43 dla 43% uzwojenia od CT, 0.0 dla czystego pentodowego/triodowego) [-]
/// @param Bac_design Robocza amplituda indukcji przy f_low (zwykle 0.9..1.1 T dla blach krzemowych) [T]
/// @param J_density Gęstość prądu w miedzi (zwykle 2.5..3.2 A/mm²) [A/m²]
[[nodiscard]] PPTransformerResult calcPPTransformer(
    const CoreGeometry& core,
    double Raa,
    double ra,
    double R_spk,
    double P_out,
    double f_low = 25.0,
    double f_high = 45000.0,
    double delta_Ia0 = 0.005,
    double ultralinearTapFraction = 0.43,
    double Bac_design = 1.0,
    double J_density = 2.8e6);

} // namespace tube::magnetics
