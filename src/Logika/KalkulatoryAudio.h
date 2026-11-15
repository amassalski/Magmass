#ifndef KALKULATORYAUDIO_H
#define KALKULATORYAUDIO_H

#include "SzeregiE.h"

#include <complex>
#include <optional>
#include <vector>

/// ECCkalkulator: decybele i nepery, krzywa korekcji RIAA oraz dobór elementów RC sieci korekcyjnej (bez GUI i bazy).
namespace KalkulatoryAudio {

constexpr double kDbNaNeper = 8.685889638065037;   ///< 20 / ln(10): 1 Np = 8,686 dB (dla napięć)

// ---------------------------------------------------------------- decybele i nepery
double dbZNeperow(double neper);
double neperyZDb(double db);
/// 20 log10(u1 / u2); nullopt, gdy któreś napięcie <= 0
std::optional<double> dbNapiec(double u1, double u2);
/// 10 log10(p1 / p2); nullopt, gdy któraś moc <= 0
std::optional<double> dbMocy(double p1, double p2);
std::optional<double> neperyNapiec(double u1, double u2);   ///< ln(u1 / u2)
std::optional<double> neperyMocy(double p1, double p2);      ///< 0,5 ln(p1 / p2)
/// Miara liniowa -> dB (stosunek napięć albo mocy); nullopt, gdy miara <= 0
std::optional<double> dbZMiaryLiniowej(double miara, bool moc);
/// dB -> miara liniowa (10^(dB/20) albo 10^(dB/10))
double miaraLiniowaZDb(double db, bool moc);

// ---------------------------------------------------------------- krzywa RIAA
/// Stałe czasowe [s]: 3180 µs, 318 µs, 75 µs; IEC dodaje filtr górnoprzepustowy 7950 µs (ok. 20 Hz)
constexpr double kTau1 = 3180e-6;
constexpr double kTau2 = 318e-6;
constexpr double kTau3 = 75e-6;
constexpr double kTau4 = 7950e-6;

/// Transmitancja korektora odtwarzania (H = (1 + s t2) / ((1 + s t1)(1 + s t3)), z IEC razem z s t4 / (1 + s t4)),
/// znormalizowana tak, aby przy 1 kHz miała 0 dB. Dla nagrywania: odwrotność (bez członu IEC).
std::complex<double> odpowiedzRIAA(double fHz, bool odtwarzanie = true, bool iec = false);
double tlumienieRIAAdB(double fHz, bool odtwarzanie = true, bool iec = false);

struct PunktRIAA {
    double fHz = 0;
    double db = 0;
    double faza = 0;   ///< stopnie
};
/// Tabela od @p odHz do @p doHz co @p interwalHz (liniowo); najwyżej @p maks punktów. Puste dla błędnych danych.
std::vector<PunktRIAA> tabelaRIAA(double odHz, double doHz, double interwalHz, bool odtwarzanie = true, bool iec = false,
                                  size_t maks = 5000);
/// Częstotliwości co najwyżej @p naDekade punktów na dekadę (do wykresu), od @p odHz do @p doHz włącznie
std::vector<double> czestotliwosciLogarytmiczne(double odHz, double doHz, int naDekade);

// ---------------------------------------------------------------- bierna sieć korekcyjna RIAA (odtwarzanie)
/// Dwie sekcje rozdzielone stopniem wzmacniającym:
///  A: szereg R1 || C1, bocznik R2 || C2 (półka 3180/318 µs, tłumienie 20 dB),
///  B: szereg R3, bocznik C3 (75 µs).
/// Sekcja A zakłada małą impedancję źródła i dużą obciążenia, sekcja B także.
struct SiecRIAA {
    double r1 = 0, c1 = 0, r2 = 0, c2 = 0;   ///< Ω, F
    double r3 = 0, c3 = 0;
};
/// Dobiera elementy dla zadanych rezystorów R2 i R3 (idealne wartości, bez zaokrąglenia). nullopt dla R <= 0.
std::optional<SiecRIAA> projektujSiecRIAA(double r2Om, double r3Om);
/// Zaokrągla do szeregów (rezystory @p r, kondensatory @p c)
SiecRIAA zaokraglijSiec(const SiecRIAA& s, Szereg r, Szereg c);
/// Transmitancja obu sekcji razem (bez normalizacji)
std::complex<double> odpowiedzSieci(const SiecRIAA& s, double fHz);
/// Największa różnica [dB] między siecią a idealną krzywą RIAA (po wyrównaniu obu przy 1 kHz) w paśmie 20 Hz..20 kHz
double maksOdchylkaSieci(const SiecRIAA& s, double odHz = 20, double doHz = 20000);
/// Tłumienie sieci przy 1 kHz [dB] (ujemne): o tyle wzmacniacz musi dodać wzmocnienia
double tlumienieSieciPrzy1kHz(const SiecRIAA& s);

} // namespace KalkulatoryAudio

#endif // KALKULATORYAUDIO_H
