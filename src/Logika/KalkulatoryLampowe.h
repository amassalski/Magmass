#ifndef KALKULATORYLAMPOWE_H
#define KALKULATORYLAMPOWE_H

#include <optional>

/// Układy lampowe małej częstotliwości w jednostkach znanych z ECCLab: rezystancje w kΩ, nachylenie w mA/V,
/// pojemności w pF (Cs w nF), częstotliwości w Hz. Obliczenia małosygnałowe (bez GUI i bazy).
/// Wzory sprawdzone na przykładach z ECCLab 7.2 (testy w tests/test_kalkulatory.cpp).
namespace KalkulatoryLampowe {

/// Dane lampy (trioda): nachylenie Sa [mA/V], współczynnik wzmocnienia Ka [-], oporność wewnętrzna Rwew [kΩ]
struct Trioda {
    double sa = 0, ka = 0, rwew = 0;
};

// ---------------------------------------------------------------- sprzężenie RC
struct WejscieRC {
    double cak = 0, cas = 0, csk = 0;   ///< pF
    double csNf = 0;                    ///< kondensator sprzęgający [nF]
    double rk = 0, ra = 0, rs = 0;      ///< kΩ
    Trioda lampa;
    bool automatyczna = true;           ///< polaryzacja automatyczna (Rk niezablokowany) albo stała
};
struct WynikRC {
    double ku = 0;        ///< wzmocnienie [V/V]
    double rwyk = 0;      ///< oporność wyjściowa [kΩ]
    double fdHz = 0;      ///< dolna częstotliwość graniczna -3 dB
    double fgKHz = 0;     ///< górna częstotliwość graniczna -3 dB [kHz]
    bool fgPrzyblizona = false;   ///< dla polaryzacji automatycznej ECCLab podaje niższą wartość (patrz dokumentacja)
};
std::optional<WynikRC> sprzezenieRC(const WejscieRC& w);

// ---------------------------------------------------------------- wtórnik katodowy
struct WejscieWtornikaKatodowego {
    double csk = 0;                ///< pF
    double rk = 0, r = 0, rs = 0;  ///< kΩ
    Trioda lampa;
    double ugk = 0;                ///< dopuszczalne napięcie siatka–katoda [V]
};
struct WynikWtornikaKatodowego {
    double rwe = 0;       ///< oporność wejściowa [kΩ]
    double rwyk = 0;      ///< oporność wyjściowa [kΩ] (1/Sa)
    double ku = 0;        ///< wzmocnienie [V/V]
    double fgMHz = 0;     ///< górna częstotliwość graniczna [MHz] (ECCLab opisuje tę wartość jako „kHz”)
    double umax = 0;      ///< maksymalne napięcie wejściowe [V]
};
std::optional<WynikWtornikaKatodowego> wtornikKatodowy(const WejscieWtornikaKatodowego& w);

// ---------------------------------------------------------------- wtórnik anodowy (trioda dolna + trioda górna jako obciążenie)
struct WejscieWtornikaAnodowego {
    double rs = 0, ro = 0;   ///< kΩ
    Trioda lampa;
};
struct WynikWtornikaAnodowego {
    double rwe = 0;    ///< = Rs [kΩ]
    double rwyk = 0;   ///< [kΩ]
    double ku = 0;
};
std::optional<WynikWtornikaAnodowego> wtornikAnodowy(const WejscieWtornikaAnodowego& w);

// ---------------------------------------------------------------- inwerter różnicowy (long-tailed pair)
struct WejscieInwertera {
    double rk = 0, ra = 0, rs = 0;   ///< kΩ (Rk to rezystor wspólny katod, Rs to obciążenie każdej anody)
    Trioda lampa;                    ///< obie triody jednakowe (wykorzystane Ka i Rwew)
};
struct WynikInwertera {
    double q = 0;      ///< współczynnik asymetrii ku1/ku2
    double kuSr = 0;   ///< średnie wzmocnienie
    double ku1 = 0, ku2 = 0;
};
std::optional<WynikInwertera> inwerterRoznicowy(const WejscieInwertera& w);


// ---------------------------------------------------------------- układy złożone (moduł tube_audio; kΩ, pF)
struct WejscieMuWtornika {
    Trioda l1, l2;                 ///< l1 – wzmacniająca, l2 – źródło prądowe/wtórnik katodowy
    double rk2 = 0, r0 = 0;        ///< kΩ; r0 = 0 oznacza brak obciążenia
    double cas1 = 0, csk1 = 0, cs0 = 0;   ///< pF
};
struct WynikMuWtornika { double ku = 0, rwyk = 0, raktywne = 0, cinPf = 0; };
std::optional<WynikMuWtornika> muWtornik(const WejscieMuWtornika& w);

struct WejscieSRPP { Trioda lampa; double rk = 0, rl = 0; };   ///< rl = 0: bez obciążenia
struct WynikSRPP { double ku0 = 0, ku = 0, rwyk = 0, rlOpt = 0; };
std::optional<WynikSRPP> srpp(const WejscieSRPP& w);

struct WejscieWhite { Trioda l1, l2; double rl = 0; };
struct WynikWhite { double ku = 0, rwyk = 0, raOpt = 0; };
std::optional<WynikWhite> wtornikWhite(const WejscieWhite& w);

struct WejscieKaskodowego { Trioda l1, l2; double ra = 0, r0 = 0; };
struct WynikKaskodowego { double ku = 0, ku0 = 0, rwyk = 0, rwykPrzybl = 0; };
std::optional<WynikKaskodowego> wtornikKaskodowy(const WejscieKaskodowego& w);

struct WejscieSuper { Trioda lampa; double ra = 0; int n = 1; };
struct WynikSuper { double ku1 = 0, rwyk = 0, ku3 = 0; };
std::optional<WynikSuper> superwtornik(const WejscieSuper& w);

} // namespace KalkulatoryLampowe

#endif // KALKULATORYLAMPOWE_H
