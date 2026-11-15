// tube_audio.cpp
// ============================================================================
// Implementacja modułu obliczeń inżynierskich dla układów lampowych audio
// ============================================================================
#include "tube_audio.hpp"

namespace tube {

// ============================================================================
// 2. KASKODOWY WTÓRNIK KATODOWY (TRIODOWY)
// ============================================================================

KaskodeFollowerResult calcKaskodeFollower(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Ra,
    double R0)
{
    if (R0 <= 0.0) throw std::invalid_argument("R0 musi być > 0");

    const double R1 = Ra + L1.rho;
    const double R2 = L2.rho + L2.Ka * Ra;

    const double denomWithoutR0 = (1.0 + L1.Ka) * R2 + R1;
    const double denom = R0 * denomWithoutR0 + L2.rho * R1;

    const double ku = (L1.Ka * R2 * R0) / denom;
    const double ku0 = (L1.Ka * R2) / denomWithoutR0;
    const double Rwy = (L2.rho * R1) / denomWithoutR0;
    const double RwyApprox = L2.rho / (L1.Ka * L2.Ka);

    return {ku, ku0, Rwy, RwyApprox};
}

// ============================================================================
// 3. SUPERWTÓRNIK (KASKODOWY WTÓRNIK n-TEGO RZĘDU)
// ============================================================================

SuperFollowerResult calcSuperFollower(
    const TriodeParams& L,
    double Ra,
    int n)
{
    if (n < 1) throw std::invalid_argument("Rząd n musi być >= 1");

    const double KaPow = std::pow(L.Ka, n + 1);
    const double bracket = 1.0 + (Ra + L.rho) / (L.Ka * Ra + L.rho);
    const double ku1 = 1.0 / (1.0 + bracket / KaPow);
    const double Rwy = (L.rho * (Ra + L.rho)) / (KaPow * (L.Ka * Ra + L.rho));
    const double ku3 = 1.0 / (1.0 + (L.Ka * L.Ka * Ra + L.rho) / (KaPow * (L.Ka * Ra + L.rho)));

    return {ku1, Rwy, ku3};
}

// ============================================================================
// 6. STABILNOŚĆ KASKODY JAKO WZMACNIACZA W.CZ.
// ============================================================================

StabilityLimits calcKaskodeStabilityLimits(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Cak2,
    double Cas1,
    double omega,
    double ks)
{
    if (omega <= 0.0) throw std::invalid_argument("omega musi być > 0");
    if (Cak2 <= 0.0 || Cas1 <= 0.0) throw std::invalid_argument("Pojemności muszą być > 0");
    if (ks <= 0.0 || ks >= 1.0) throw std::invalid_argument("ks musi należeć do przedziału (0, 1)");

    const double ku2max = (2.0 * L2.Sa * (1.0 - ks)) / (omega * Cak2);
    const double kumax = std::sqrt((2.0 * L1.Sa * (1.0 + L2.Ka) * (1.0 - ks)) / (omega * Cas1));

    return {ku2max, kumax};
}

// ============================================================================
// 7. KASKODOWY WTÓRNIK NA PENTODACH
// ============================================================================

PentodeFollowerResult calcPentodeKaskodeFollower(
    const PentodeParams& L1,
    const PentodeParams& L2,
    double Ra,
    double R0,
    double Cs0,
    double Csk1,
    double Cas1)
{
    if (R0 <= 0.0) throw std::invalid_argument("R0 musi być > 0");

    const double denomInner = L1.Sa * (L2.rho + L2.Ka * Ra);
    const double ku0 = 1.0 / (1.0 + 1.0 / denomInner);
    const double Rwy = 1.0 / (L1.Sa * (1.0 + L2.Sa * Ra));
    const double ku = ku0 * R0 / (R0 + Rwy);
    const double Cin = Cas1 + Csk1 * (1.0 - ku) + Cs0;

    return {ku, ku0, Rwy, Cin};
}

// ============================================================================
// 8. µ-WTÓRNIK (MU-FOLLOWER)
// ============================================================================

MuFollowerResult calcMuFollower(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Rk2,
    double Cs0,
    double Csk1,
    double Cas1,
    double R0)
{
    if (L1.Ka <= 1.0 || L2.Ka <= 1.0)
        throw std::invalid_argument("Ka musi być > 1");

    const double Raktywne = L2.rho + (L2.Ka + 1.0) * Rk2;
    const double ku_anoda = (L1.Ka * Raktywne) / (L1.rho + Raktywne);

    const double ku_cf = ((L2.Ka + 1.0) * Rk2) / (L2.rho + (L2.Ka + 1.0) * Rk2);
    const double ku0 = ku_anoda * ku_cf;

    const double Rwy = (L2.rho + Rk2) / (L2.Ka + 1.0);
    const double ku = std::isinf(R0) ? ku0 : ku0 * R0 / (R0 + Rwy);
    const double Cin = Csk1 + Cas1 * (1.0 + ku_anoda) + Cs0;

    return {ku, Rwy, Raktywne, Cin};
}

// ============================================================================
// 9. SRPP (SERIES REGULATED PUSH-PULL)
// ============================================================================

SRPPResult calcSRPP(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Rk,
    double RL)
{
    (void)L2; // Dla typowego SRPP obie lampy są identyczne
    if (L1.Ka <= 1.0) throw std::invalid_argument("Ka musi być > 1");
    if (Rk < 0.0) throw std::invalid_argument("Rk musi być >= 0");

    const double mu = L1.Ka;
    const double rp = L1.rho;

    const double RLopt = (rp + (mu + 1.0) * Rk) / (mu - 1.0);
    const double Rwy = (rp * (rp + (mu + 1.0) * Rk)) / (2.0 * rp + (mu + 1.0) * Rk);
    const double ku0 = (mu * (mu * Rk + rp)) / (2.0 * rp + (mu + 1.0) * Rk);
    const double ku = std::isinf(RL) ? ku0 : (ku0 * RL) / (RL + Rwy);

    return {ku0, ku, Rwy, RLopt};
}

// ============================================================================
// 10. WHITE CATHODE FOLLOWER (WCF)
// ============================================================================

WCFResult calcWhiteCathodeFollower(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double RL)
{
    if (RL <= 0.0) throw std::invalid_argument("RL musi być > 0");

    const double RaOpt = (L2.rho + 2.0 * RL) / L2.Ka;
    const double Rwy = (L1.rho * (L2.rho + RaOpt)) / 
                       (L1.rho + L2.rho + L1.Ka * L2.Ka * RaOpt);
    const double ku = RL / (RL + Rwy);

    return {ku, Rwy, RaOpt};
}

// ============================================================
// 11. LONG-TAILED PAIR (ODWRACACZ FAZY)
// ============================================================

LTPResult calcLTPTriode(
    const TriodeParams& L1,
    const TriodeParams& L2,
    double Ra1,
    double Ra2,
    double Rks)
{
    if (Rks <= 0.0) throw std::invalid_argument("Rks musi być > 0");
    if (Ra1 <= 0.0 || Ra2 <= 0.0) throw std::invalid_argument("Ra1 i Ra2 muszą być > 0");

    const double R1prime = L1.rho + Ra1;
    const double R2prime = L2.rho + Ra2;

    const double gmk1 = (L1.Ka + 1.0) / R1prime;
    const double gmk2 = (L2.Ka + 1.0) / R2prime;

    const double denom = 1.0 / Rks + gmk1 + gmk2;
    // Prąd wpływający do katody lampy 1 przy v_s1 = 1, v_s2 = 0: (Ka1 - (Ka1 + 1) vk) / R1'
    // (w pierwotnej wersji w liczniku stał (Ka1 + 1) / R1', co zawyżało vk; poprawione i sprawdzone
    //  na przykładach ECCLab 7.2 oraz ścisłym rozwiązaniem węzłowym)
    const double vk = (L1.Ka / R1prime) / denom;

    const double i1 = (L1.Ka * (1.0 - vk) - vk) / R1prime;
    const double i2 = (L2.Ka * (0.0 - vk) - vk) / R2prime;

    const double ku1 = std::abs(i1 * Ra1);
    const double ku2 = std::abs(i2 * Ra2);
    const double q = (ku2 > 1e-12) ? ku1 / ku2 : std::numeric_limits<double>::infinity();

    return {ku1, ku2, q, Rks};
}

LTPResult calcLTPPentode(
    double Sa,
    double Ra1,
    double Ra2,
    double Rks,
    double Sk)
{
    if (Sk <= 0.0 || Rks <= 0.0)
        throw std::invalid_argument("Sk i Rks muszą być > 0");
    if (Ra1 <= 0.0 || Ra2 <= 0.0)
        throw std::invalid_argument("Ra1 i Ra2 muszą być > 0");

    const double denom = 1.0 + 2.0 * Sk * Rks;
    const double ku1 = Sa * Ra1 * (1.0 + Sk * Rks) / denom;
    const double ku2 = Sa * Ra2 * (Sk * Rks) / denom;
    const double q = (1.0 + Sk * Rks) / (Sk * Rks);
    return {ku1, ku2, q, Rks};
}

double calcLTPRksForBalance(
    const TriodeParams& L,
    double Ra1,
    double Ra2)
{
    if (Ra2 <= Ra1)
        throw std::invalid_argument("Zrównoważenie LTP przy wejściu na siatkę 1 wymaga Ra2 > Ra1");
    const double Rprime = L.rho + Ra1;
    return (Ra1 * Rprime) / ((L.Ka + 1.0) * (Ra2 - Ra1));
}

LTPDiffResult calcLTPDiff(const LTPResult& r) {
    const double Ad = 0.5 * (r.ku1 + r.ku2);
    const double Acm = std::abs(r.ku1 - r.ku2);
    const double CMRR = (Acm > 1e-12) ? Ad / Acm : std::numeric_limits<double>::infinity();
    return {Ad, Acm, CMRR};
}

} // namespace tube
