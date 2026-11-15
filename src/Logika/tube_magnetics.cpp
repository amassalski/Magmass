// tube_magnetics.cpp
// ============================================================================
// Implementacja modułu obliczeń inżynierskich elementów magnetycznych
// ============================================================================
#include "tube_magnetics.hpp"

namespace tube::magnetics {

// ============================================================================
// 2. TRANSFORMATOR SIECIOWY
// ============================================================================

MainsSecondarySpec makeSecondaryForBridgeRectifier(
    const std::string& name,
    double U_dc,
    double I_dc,
    double V_rect_drop)
{
    if (U_dc <= 0.0 || I_dc <= 0.0) throw std::invalid_argument("U_dc i I_dc muszą być > 0");

    // Dla filtru z wejściem pojemnościowym (prostownik mostkowy Graetza):
    // U_dc = sqrt(2) * U_ac - V_rect_drop pod typowym obciążeniem i tętnieniem ~10-15%:
    // U_ac_rms ≈ (U_dc + V_rect_drop) / 1.32
    // I_ac_rms ≈ 1.7 * I_dc (współczynnik kształtu prądu ładowania kondensatora)
    const double U_ac = (U_dc + V_rect_drop) / 1.32;
    const double I_ac = 1.70 * I_dc;
    return {name, U_ac, I_ac, 1.06}; // 6% kompensacji spadku napięcia na miedzi
}

MainsSecondarySpec makeSecondaryForCenterTappedRectifier(
    const std::string& name,
    double U_dc,
    double I_dc,
    double V_rect_drop)
{
    if (U_dc <= 0.0 || I_dc <= 0.0) throw std::invalid_argument("U_dc i I_dc muszą być > 0");

    // Prostownik dwupołówkowy z odczepem środkowym (Full-Wave CT, np. lampowy 5U4G / GZ34):
    // Każda połówka uzwojenia przewodzi przez pół okresu:
    // U_ac_rms na połówkę ≈ (U_dc + V_rect_drop) / 1.18
    // I_ac_rms na połówkę ≈ 1.20 * I_dc
    const double U_ac_half = (U_dc + V_rect_drop) / 1.18;
    const double I_ac = 1.20 * I_dc;
    return {name, U_ac_half, I_ac, 1.06};
}

MainsTransformerResult calcMainsTransformer(
    const CoreGeometry& core,
    double U_prim,
    double freq,
    double Bmax,
    double J_density,
    const std::vector<MainsSecondarySpec>& secondaries,
    double efficiency,
    double maxFillFactor)
{
    if (U_prim <= 0.0 || freq <= 0.0 || Bmax <= 0.0 || J_density <= 0.0)
        throw std::invalid_argument("Parametry wejściowe transformatora muszą być > 0");
    if (efficiency <= 0.5 || efficiency >= 1.0)
        throw std::invalid_argument("Sprawność musi być w przedziale (0.5, 1.0)");

    // 1. Sumaryczna moc wtórna
    double P_sec_total = 0.0;
    for (const auto& sec : secondaries) {
        P_sec_total += sec.U_rms * sec.I_rms;
    }

    // 2. Moc obliczeniowa rdzenia i moc pierwotna
    const double P_core_calc = P_sec_total / efficiency;
    const double cos_phi = 0.95; // współczynnik mocy transformatora
    const double I_prim = P_core_calc / (U_prim * cos_phi);

    // 3. Liczba zwojów na wolt (prawo Faradaya dla przebiegu sinusoidalnego):
    // U_rms = 4.44 * f * Bmax * Qfe * N  =>  z_v = N / U = 1 / (4.44 * f * Bmax * Qfe)
    const double turnsPerVolt = 1.0 / (4.44 * freq * Bmax * core.Qfe());

    const double l_turn = core.meanTurnLength();

    // 4. Uzwojenie pierwotne
    const int N_prim = static_cast<int>(std::round(U_prim * turnsPerVolt));
    const double q_prim = WireCalc::copperArea(I_prim, J_density);
    const double d_prim_bare = WireCalc::bareDiameter(I_prim, J_density);
    const double d_prim_iso = WireCalc::enamelledDiameter(d_prim_bare);
    const double R_prim = WireCalc::resistance(N_prim * l_turn, q_prim);
    MainsWindingResult primResult{"Pierwotne", N_prim, d_prim_bare, d_prim_iso, q_prim, R_prim, U_prim * I_prim};

    // 5. Uzwojenia wtórne
    std::vector<MainsWindingResult> secResults;
    secResults.reserve(secondaries.size());
    double totalCuArea = N_prim * q_prim;

    for (const auto& sec : secondaries) {
        // Zwoje wtórne z uwzględnieniem kompensacji spadku napięcia na oporności miedzi:
        const int N_sec = static_cast<int>(std::round(sec.U_rms * turnsPerVolt * sec.dropFactor));
        const double q_sec = WireCalc::copperArea(sec.I_rms, J_density);
        const double d_bare = WireCalc::bareDiameter(sec.I_rms, J_density);
        const double d_iso = WireCalc::enamelledDiameter(d_bare);
        const double R_sec = WireCalc::resistance(N_sec * l_turn, q_sec);

        secResults.push_back({sec.name, N_sec, d_bare, d_iso, q_sec, R_sec, sec.U_rms * sec.I_rms});
        totalCuArea += N_sec * q_sec;
    }

    // 6. Weryfikacja okna karkasu
    const double windowFillFactor = totalCuArea / core.Qo();
    const bool fitsInWindow = (windowFillFactor <= maxFillFactor);

    return {P_sec_total, P_core_calc, turnsPerVolt, primResult, secResults, windowFillFactor, fitsInWindow, efficiency};
}

// ============================================================================
// 3. DŁAWIK FILTRACYJNY Z PRĄDEM DC I SZCZELINĄ
// ============================================================================

FilterChokeResult calcFilterChoke(
    const CoreGeometry& core,
    double L_target,
    double I_dc,
    double Bdc_max,
    double J_density,
    double maxFillFactor,
    double mu_r)
{
    if (L_target <= 0.0 || I_dc <= 0.0 || Bdc_max <= 0.0 || mu_r <= 0.0)
        throw std::invalid_argument("Parametry dławika muszą być > 0");

    const double Qfe = core.Qfe();
    const double l_turn = core.meanTurnLength();

    // 1. Zgromadzona energia magnetyczna:
    const double energyJoules = 0.5 * L_target * I_dc * I_dc;

    // 2. Liczba zwojów N z prawa przepływu i indukcyjności dławika ze szczeliną:
    // L = mu0 * Qfe * N² / l_p   oraz   Bdc = mu0 * N * I_dc / l_p
    // Dzieląc L przez Bdc: L / Bdc = (N * Qfe) / I_dc  =>  N = (L * I_dc) / (Bdc * Qfe)
    const int turns = static_cast<int>(std::ceil((L_target * I_dc) / (Bdc_max * Qfe)));

    // 3. Wymagana całkowita reluktancja drogi magnetycznej:
    // l_eff = l_p + (lFe / mu_r) = mu0 * N * I_dc / Bdc
    // Część reluktancji zapewnia samo żelazo (lFe / mu_r), więc fizyczna szczelina powietrzna to:
    const double l_eff_req = (MU_0 * turns * I_dc) / Bdc_max;
    const double iron_gap_equiv = core.lFe / mu_r;
    const double airGapTotal = std::max(0.0, l_eff_req - iron_gap_equiv);

    // Dla kształtki EI przekładka pod wszystkie 3 kolumny tworzy szczelinę podwójną:
    // l_p = 2 * delta  =>  delta = l_p / 2
    const double spacerThickness = airGapTotal * 0.5;

    // Rzeczywista uzyskana indukcyjność:
    const double l_eff_actual = airGapTotal + iron_gap_equiv;
    const double L_actual = (MU_0 * Qfe * turns * turns) / l_eff_actual;

    // 5. Przewód nawojowy i rezystancja uzwojenia
    const double qCu = WireCalc::copperArea(I_dc, J_density);
    const double d_bare = WireCalc::bareDiameter(I_dc, J_density);
    const double d_iso = WireCalc::enamelledDiameter(d_bare);
    const double R_dc = WireCalc::resistance(turns * l_turn, qCu);

    const double V_drop_dc = I_dc * R_dc;
    const double P_dissipated = I_dc * I_dc * R_dc;

    // 6. Weryfikacja okna
    const double windowFillFactor = (turns * qCu) / core.Qo();
    const bool fitsInWindow = (windowFillFactor <= maxFillFactor);

    return {
        L_actual,
        turns,
        airGapTotal,
        spacerThickness,
        d_bare,
        d_iso,
        R_dc,
        V_drop_dc,
        P_dissipated,
        energyJoules,
        windowFillFactor,
        fitsInWindow
    };
}

// ============================================================================
// 4. TRANSFORMATOR GŁOŚNIKOWY SINGLE-ENDED (SE)
// ============================================================================

SETransformerResult calcSETransformer(
    const CoreGeometry& core,
    double Ra,
    double ra,
    double R_spk,
    double P_out,
    double Ia0,
    double f_low,
    double f_high,
    double Bdc,
    double J_density)
{
    if (Ra <= 0.0 || ra <= 0.0 || R_spk <= 0.0 || P_out <= 0.0 || Ia0 <= 0.0)
        throw std::invalid_argument("Parametry transformatora SE muszą być > 0");

    const double Qfe = core.Qfe();
    const double l_turn = core.meanTurnLength();

    // 1. Przekładnia zwojowa n = sqrt((Ra * eta) / R_spk), wstępne eta ≈ 0.88
    const double eta_est = 0.88;
    const double turnsRatio = std::sqrt((Ra * eta_est) / R_spk);

    // 2. Wymagane napięcie zmienne pełnej mocy na uzwojeniu pierwotnym:
    const double U1_max = std::sqrt(2.0 * P_out * Ra); // amplituda szczytowa
    const double U1_rms = std::sqrt(P_out * Ra);

    // 3. Wymagana indukcyjność główna Lp dla dolnej częstotliwości f_low:
    // Wg Cykina i Morgana Jonesa (Valve Amplifiers s. 445):
    // Aby uniknąć eliptycznego zniekształcenia prostej obciążenia na basie (spadek reaktancji magnesującej),
    // reaktancja uzwojenia pierwotnego omega * Lp musi być co najmniej rzędu impedancji obciążenia Ra:
    // Lp_req >= Ra / (2 * pi * f_low)
    const double Lp_req = Ra / (2.0 * 3.14159265358979323846 * f_low);

    // 4. Liczba zwojów pierwotnych N1:
    // a) Kryterium indukcji zmiennej Bac <= 0.60 T przy pełnej mocy na częstotliwości f_low:
    const double Bac_max_target = 0.60;
    const double N1_ac = U1_max / (2.0 * 3.14159265358979323846 * f_low * Bac_max_target * Qfe);

    // b) Kryterium wymaganej indukcyjności Lp_req:
    // Ze wzoru reluktancji ze szczeliną: Lp ≈ mu0 * Qfe * N1² / (lp + lFe/mu_delta),
    // gdzie lp ≈ mu0 * N1 * Ia0 / Bdc.
    // Stąd N1_L = (Lp_req * Ia0) / (Bdc * Qfe) * (1 + (Bdc * lFe) / (mu_delta * mu0 * N1 * Ia0)...)
    // Przybliżenie z uwzględnieniem spłaszczenia pętli histerezy:
    const double mu_delta = 800.0; // przenikalność dynamiczna blachy zorientowanej/krzemowej pod DC
    const double N1_L = (Lp_req * Ia0) / (Bdc * Qfe) * 1.5;

    const int N1 = static_cast<int>(std::ceil(std::max(N1_ac, N1_L)));

    // 5. Całkowita szczelina niemagnetyczna l_p dla składowej stałej Ia0:
    const double airGapTotal = (MU_0 * N1 * Ia0) / Bdc;
    const double spacerThickness = airGapTotal * 0.5;

    // Rzeczywista indukcyjność z uwzględnieniem szczeliny i reluktancji żelaza:
    const double l_eff_actual = airGapTotal + (core.lFe / mu_delta);
    const double L_prim = (MU_0 * Qfe * N1 * N1) / l_eff_actual;
    const double Req = (Ra * ra) / (Ra + ra);
    const double f_low_actual = Req / (2.0 * 3.14159265358979323846 * L_prim);

    // 6. Rzeczywista indukcja zmienna Bac i całkowity szczyt indukcji
    const double Bac_actual = U1_max / (2.0 * 3.14159265358979323846 * f_low * N1 * Qfe);
    const double B_total_peak = Bdc + Bac_actual;

    // 7. Uzwojenie wtórne
    const int N2 = static_cast<int>(std::round(N1 / turnsRatio));

    // 8. Dobór przewodów nawojowych
    // Prąd pierwotny skuteczny: Ia0 (DC) + prąd zmienny AC:
    const double I1_ac_rms = U1_rms / Ra;
    const double I1_rms = std::sqrt(Ia0 * Ia0 + I1_ac_rms * I1_ac_rms);
    const double I2_rms = std::sqrt(P_out / R_spk);

    const double qCu1 = WireCalc::copperArea(I1_rms, J_density);
    const double qCu2 = WireCalc::copperArea(I2_rms, J_density);
    const double d1_bare = WireCalc::bareDiameter(I1_rms, J_density);
    const double d2_bare = WireCalc::bareDiameter(I2_rms, J_density);

    // 9. Rezystancje uzwojeń i rzeczywista sprawność
    const double R1_dc = WireCalc::resistance(N1 * l_turn, qCu1);
    const double R2_dc = WireCalc::resistance(N2 * l_turn, qCu2);

    const double P_loss_cu = (I1_ac_rms * I1_ac_rms * R1_dc) + (I2_rms * I2_rms * R2_dc);
    const double efficiency = P_out / (P_out + P_loss_cu);

    // 10. Górna częstotliwość f_high i indukcyjność rozproszenia Ls
    // Wg Morgana Jonesa i RDH4: f_high = (Ra + ra) / (2 * pi * Ls)
    const double L_leakage_max = (Ra + ra) / (2.0 * 3.14159265358979323846 * f_high);

    // Liczba sekcji interleaving (np. P-S-P = 3 sekcje, 3P-2S = 5 sekcji):
    // Standardowo dla pasma do 30 kHz przy Ra=3-5k wymagane jest min. 3 sekcje (P-S-P), a dla 50 kHz - 5 sekcji:
    int minInterleavingSections = 3;
    if (f_high > 35000.0) minInterleavingSections = 5;

    // 11. Wypełnienie okna
    const double totalCu = N1 * qCu1 + N2 * qCu2;
    const double windowFillFactor = totalCu / core.Qo();
    const bool fitsInWindow = (windowFillFactor <= 0.38);

    return {
        turnsRatio,
        N1,
        N2,
        L_prim,
        f_low_actual,
        airGapTotal,
        spacerThickness,
        d1_bare,
        d2_bare,
        R1_dc,
        R2_dc,
        efficiency,
        Bac_actual,
        Bdc,
        B_total_peak,
        L_leakage_max,
        minInterleavingSections,
        windowFillFactor,
        fitsInWindow
    };
}

// ============================================================================
// 5. TRANSFORMATOR GŁOŚNIKOWY PUSH-PULL (PP)
// ============================================================================

PPTransformerResult calcPPTransformer(
    const CoreGeometry& core,
    double Raa,
    double ra,
    double R_spk,
    double P_out,
    double f_low,
    double f_high,
    double delta_Ia0,
    double ultralinearTapFraction,
    double Bac_design,
    double J_density)
{
    if (Raa <= 0.0 || ra <= 0.0 || R_spk <= 0.0 || P_out <= 0.0)
        throw std::invalid_argument("Parametry transformatora PP muszą być > 0");

    const double Qfe = core.Qfe();
    const double l_turn = core.meanTurnLength();

    // 1. Przekładnia anoda-anoda n = sqrt((Raa * eta) / R_spk)
    const double eta_est = 0.92;
    const double turnsRatio = std::sqrt((Raa * eta_est) / R_spk);

    // 2. Napięcie zmienne skuteczne anoda-anoda przy pełnej mocy:
    const double Uaa_rms = std::sqrt(P_out * Raa);

    // 3. Całkowita liczba zwojów pierwotnych anoda-anoda (prawo Faradaya dla przebiegu AC):
    // N1 = Uaa_rms / (4.44 * f_low * Bac_design * Qfe)
    const int N1_total = static_cast<int>(std::ceil(Uaa_rms / (4.44 * f_low * Bac_design * Qfe)));
    const int N1_half = N1_total / 2;

    // Odczep Ultra-Linear (zwykle 43% uzwojenia połówkowego od odczepu środkowego B+):
    const int N1_ultralinear = static_cast<int>(std::round(N1_half * ultralinearTapFraction));

    // 4. Uzwojenie wtórne
    const int N2 = static_cast<int>(std::round(N1_total / turnsRatio));

    // 5. Mikroszczelina technologiczna zabezpieczająca przed asymetrią lamp (delta_Ia0, np. 5 mA):
    // Dopuszczalna indukcja od asymetrii Bdc_tol ≈ 0.15 T
    const double Bdc_tol = 0.15;
    const double techAirGapTotal = (MU_0 * N1_half * delta_Ia0) / Bdc_tol;
    const double spacerThickness = techAirGapTotal * 0.5;

    // 6. Indukcyjność główna pierwotna Lp (bez szczeliny lub z mikroszczeliną):
    const double mu_r = (techAirGapTotal > 1e-6) ? 1200.0 : 2500.0;
    const double l_eff = techAirGapTotal + (core.lFe / mu_r);
    const double L_prim = (MU_0 * Qfe * N1_total * N1_total) / l_eff;

    // Dolna częstotliwość graniczna (-3 dB):
    // Oporność zastępcza stopnia PP widziana przez całe uzwojenie anoda-anoda: Req = Raa || (2 * ra)
    const double Req_aa = (Raa * 2.0 * ra) / (Raa + 2.0 * ra);
    const double f_low_actual = Req_aa / (2.0 * 3.14159265358979323846 * L_prim);

    // 7. Przewody nawojowe
    // W klasie AB prąd skuteczny w każdej połówce uzwojenia pierwotnego:
    const double I1_rms = std::sqrt(P_out / Raa) * 1.1; // z naddatkiem na prąd spoczynkowy
    const double I2_rms = std::sqrt(P_out / R_spk);

    const double qCu1 = WireCalc::copperArea(I1_rms, J_density);
    const double qCu2 = WireCalc::copperArea(I2_rms, J_density);
    const double d1_bare = WireCalc::bareDiameter(I1_rms, J_density);
    const double d2_bare = WireCalc::bareDiameter(I2_rms, J_density);

    // 8. Rezystancje uzwojeń i sprawność
    const double R1_half_dc = WireCalc::resistance(N1_half * l_turn, qCu1);
    const double R2_dc = WireCalc::resistance(N2 * l_turn, qCu2);

    const double P_loss_cu = (2.0 * I1_rms * I1_rms * R1_half_dc) + (I2_rms * I2_rms * R2_dc);
    const double efficiency = P_out / (P_out + P_loss_cu);

    // 9. Dopuszczalna indukcyjność rozproszenia Ls anoda-anoda:
    // f_high = (Raa + 2 * ra) / (2 * pi * Ls)
    const double L_leakage_max = (Raa + 2.0 * ra) / (2.0 * 3.14159265358979323846 * f_high);

    // Dla PP ze względu na symetrię zaleca się podział min. 4P + 3S lub 5P + 4S:
    const int recommendedSections = (f_high > 40000.0) ? 7 : 5;

    // 10. Wypełnienie okna
    const double totalCu = N1_total * qCu1 + N2 * qCu2;
    const double windowFillFactor = totalCu / core.Qo();
    const bool fitsInWindow = (windowFillFactor <= 0.38);

    return {
        turnsRatio,
        N1_total,
        N1_half,
        N1_ultralinear,
        N2,
        L_prim,
        f_low_actual,
        d1_bare,
        d2_bare,
        R1_half_dc,
        R2_dc,
        efficiency,
        Bac_design,
        techAirGapTotal,
        spacerThickness,
        L_leakage_max,
        recommendedSections,
        windowFillFactor,
        fitsInWindow
    };
}

} // namespace tube::magnetics
