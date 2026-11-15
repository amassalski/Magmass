#include "KalkulatoryAudio.h"
#include "Kalkulatory.h"

#include <algorithm>
#include <cmath>

namespace KalkulatoryAudio {

namespace {
const double kPi = 3.14159265358979323846;
using C = std::complex<double>;

C bezNormalizacji(double f, bool odtwarzanie, bool iec) {
    const C s(0.0, 2.0 * kPi * f);
    C h = (1.0 + s * kTau2) / ((1.0 + s * kTau1) * (1.0 + s * kTau3));
    if (!odtwarzanie) return 1.0 / h;
    if (iec) h *= (s * kTau4) / (1.0 + s * kTau4);
    return h;
}
}  // namespace

double dbZNeperow(double neper) { return neper * kDbNaNeper; }
double neperyZDb(double db) { return db / kDbNaNeper; }

std::optional<double> dbNapiec(double u1, double u2) {
    if (!(u1 > 0) || !(u2 > 0)) return std::nullopt;
    return 20.0 * std::log10(u1 / u2);
}
std::optional<double> dbMocy(double p1, double p2) {
    if (!(p1 > 0) || !(p2 > 0)) return std::nullopt;
    return 10.0 * std::log10(p1 / p2);
}
std::optional<double> neperyNapiec(double u1, double u2) {
    if (!(u1 > 0) || !(u2 > 0)) return std::nullopt;
    return std::log(u1 / u2);
}
std::optional<double> neperyMocy(double p1, double p2) {
    if (!(p1 > 0) || !(p2 > 0)) return std::nullopt;
    return 0.5 * std::log(p1 / p2);
}
std::optional<double> dbZMiaryLiniowej(double miara, bool moc) {
    if (!(miara > 0)) return std::nullopt;
    return (moc ? 10.0 : 20.0) * std::log10(miara);
}
double miaraLiniowaZDb(double db, bool moc) { return std::pow(10.0, db / (moc ? 10.0 : 20.0)); }

std::complex<double> odpowiedzRIAA(double f, bool odtwarzanie, bool iec) {
    return bezNormalizacji(f, odtwarzanie, iec) / bezNormalizacji(1000.0, odtwarzanie, iec);
}

double tlumienieRIAAdB(double f, bool odtwarzanie, bool iec) {
    return 20.0 * std::log10(std::abs(odpowiedzRIAA(f, odtwarzanie, iec)));
}

std::vector<PunktRIAA> tabelaRIAA(double od, double doo, double interwal, bool odtwarzanie, bool iec, size_t maks) {
    std::vector<PunktRIAA> w;
    if (!(interwal > 0) || od < 0 || doo < od) return w;
    for (size_t i = 0; i < maks; ++i) {
        const double f = od + interwal * static_cast<double>(i);
        if (f > doo + interwal * 1e-9) break;
        PunktRIAA p;
        p.fHz = f;
        if (f <= 0) {   // 0 Hz: wzmocnienie stałe; RIAA odtwarzania rośnie do ok. +20 dB
            const C h = odtwarzanie ? bezNormalizacji(1e-6, true, iec) / bezNormalizacji(1000.0, true, iec)
                                    : bezNormalizacji(1e-6, false, iec) / bezNormalizacji(1000.0, false, iec);
            p.db = 20.0 * std::log10(std::abs(h));
            p.faza = 0;
        } else {
            const C h = odpowiedzRIAA(f, odtwarzanie, iec);
            p.db = 20.0 * std::log10(std::abs(h));
            p.faza = std::arg(h) * 180.0 / kPi;
        }
        w.push_back(p);
    }
    return w;
}

std::vector<double> czestotliwosciLogarytmiczne(double od, double doo, int naDekade) {
    std::vector<double> w;
    if (!(od > 0) || !(doo >= od) || naDekade < 1) return w;
    const double krok = std::pow(10.0, 1.0 / naDekade);
    for (double f = od; f <= doo * (1 + 1e-9); f *= krok) w.push_back(f);
    if (w.empty() || std::abs(w.back() - doo) > doo * 1e-6) {
        if (w.back() < doo) w.push_back(doo);
    }
    return w;
}

std::optional<SiecRIAA> projektujSiecRIAA(double r2, double r3) {
    if (!(r2 > 0) || !(r3 > 0)) return std::nullopt;
    SiecRIAA s;
    s.r2 = r2;
    s.r1 = r2 * (kTau1 - kTau2) / kTau2;                      // R2 / (R1 + R2) = t2 / t1
    s.c1 = kTau2 / s.r1;                                       // zero: R1 C1 = t2
    s.c2 = kTau1 * (s.r1 + s.r2) / (s.r1 * s.r2) - s.c1;       // biegun: (R1 || R2)(C1 + C2) = t1
    s.r3 = r3;
    s.c3 = kTau3 / r3;
    return s;
}

SiecRIAA zaokraglijSiec(const SiecRIAA& s, Szereg r, Szereg c) {
    SiecRIAA w;
    w.r1 = Kalkulatory::najblizsza(s.r1, r);
    w.r2 = Kalkulatory::najblizsza(s.r2, r);
    w.r3 = Kalkulatory::najblizsza(s.r3, r);
    w.c1 = Kalkulatory::najblizsza(s.c1, c);
    w.c2 = Kalkulatory::najblizsza(s.c2, c);
    w.c3 = Kalkulatory::najblizsza(s.c3, c);
    return w;
}

std::complex<double> odpowiedzSieci(const SiecRIAA& s, double f) {
    const C sj(0.0, 2.0 * kPi * f);
    const C z1 = s.r1 / (1.0 + sj * s.r1 * s.c1);   // szereg: R1 || C1
    const C z2 = s.r2 / (1.0 + sj * s.r2 * s.c2);   // bocznik: R2 || C2
    const C a = z2 / (z1 + z2);
    const C b = 1.0 / (1.0 + sj * s.r3 * s.c3);
    return a * b;
}

double maksOdchylkaSieci(const SiecRIAA& s, double od, double doo) {
    const double ref = 20.0 * std::log10(std::abs(odpowiedzSieci(s, 1000.0)));
    double maks = 0;
    for (double f : czestotliwosciLogarytmiczne(od, doo, 24)) {
        const double db = 20.0 * std::log10(std::abs(odpowiedzSieci(s, f))) - ref;
        maks = std::max(maks, std::abs(db - tlumienieRIAAdB(f, true, false)));
    }
    return maks;
}

double tlumienieSieciPrzy1kHz(const SiecRIAA& s) { return 20.0 * std::log10(std::abs(odpowiedzSieci(s, 1000.0))); }

} // namespace KalkulatoryAudio
