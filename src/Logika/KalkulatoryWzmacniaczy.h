#ifndef KALKULATORYWZMACNIACZY_H
#define KALKULATORYWZMACNIACZY_H

#include "SzeregiE.h"

#include <complex>
#include <optional>
#include <string>
#include <vector>

/// Kalkulatory wzmacniaczy lampowych: polaryzacja katodowa z kondensatorem katodowym, kondensator sprzęgający między stopniami
/// oraz regulacja barwy typu TMB (Fender, Marshall) liczona z układu węzłowego. Bez GUI i bazy.
namespace KalkulatoryWzmacniaczy {

// ---------------------------------------------------------------- polaryzacja katodowa (automatyczna)
struct WejscieKatody {
    double iaMA = 0;     ///< prąd anodowy [mA]
    double ig2MA = 0;    ///< prąd siatki ekranującej [mA] (0 dla triody); płynie przez ten sam Rk
    double ukV = 0;      ///< wymagane napięcie polaryzacji (spadek na Rk) [V]
    double gmMAV = 0;    ///< konduktancja wzajemna [mA/V]; 0 = bez doboru Ck z gm
    double fdHz = 0;     ///< żądana dolna częstotliwość graniczna [Hz]; 0 = bez doboru Ck
    Szereg szeregR = Szereg::E24;
    Szereg szeregC = Szereg::E12;
};

struct WynikKatody {
    double ikMA = 0;           ///< prąd katody: Ia + Ig2
    double rkIdealny = 0;      ///< Ω: Uk / Ik
    double rkSzereg = 0;       ///< Ω: najbliższa wartość z szeregu
    double ukRzeczywiste = 0;  ///< V: Ik · Rk z szeregu
    double mocMW = 0;          ///< mW: Uk² / Rk (rezystor z szeregu)
    double mocZalecanaMW = 0;  ///< mW: 2-krotny zapas
    bool maCk = false;         ///< czy policzono kondensator katodowy
    bool ckPotrzebny = true;   ///< false, gdy Rk i gm są tak małe, że spadek 3 dB nigdy nie nastąpi
    double ckUF = 0;           ///< µF: spadek wzmocnienia o 3 dB (w stosunku do pełnego zblokowania) przy fd
    double ckSzeregUF = 0;     ///< µF: najbliższa NIE MNIEJSZA z szeregu
    double ckReguleUF = 0;     ///< µF: reguła praktyczna, reaktancja Ck = Rk / 10 przy fd
    double napiecieCkV = 0;    ///< V: zalecane napięcie pracy kondensatora (>= 2 · Uk, najmniej 16 V)
};
/// nullopt dla Ia + Ig2 <= 0 albo Uk <= 0 albo ujemnych danych
std::optional<WynikKatody> polaryzacjaKatodowa(const WejscieKatody& w);

/// Częstotliwość -3 dB (względem pełnego zblokowania Rk) zestawu Rk ‖ Ck przy konduktancji gm: wynik z równania
/// |(1 + jx) / (1 + K + jx)| = 1/√2, x = 2π f Rk Ck, K = gm·Rk, czyli x = √((1+K)² − 2). 0, gdy K < √2 − 1 (spadek nigdy nie sięga 3 dB).
double fKatodyHz(double rkOm, double ckF, double gmSiemens);

// ---------------------------------------------------------------- kondensator sprzęgający
struct WejscieSprzezenia {
    double raKOhm = 0;     ///< obciążenie anodowe poprzedniego stopnia [kΩ]
    double rwewKOhm = 0;   ///< rezystancja wewnętrzna lampy poprzedniego stopnia [kΩ] (0 = pomiń, wtedy Rwyj = Ra)
    double rgKOhm = 0;     ///< rezystor siatki następnego stopnia [kΩ]
    double csNF = 0;       ///< kondensator sprzęgający [nF]; 0 = tylko dobór z fd
    double fdHz = 0;       ///< żądana dolna częstotliwość graniczna całego układu -3 dB [Hz]; 0 = tylko policz z Cs
    int sekcje = 1;        ///< ile jednakowych sekcji RC w kaskadzie (1..8), np. kilka stopni sprzężonych RC
    Szereg szeregC = Szereg::E12;
};
struct WynikSprzezenia {
    double rcalkKOhm = 0;      ///< (Ra ‖ ra) + Rg
    bool maF = false;          ///< policzono f z Cs
    double fSekcjiHz = 0;      ///< 1 / (2π (Rwyj + Rg) Cs)
    double fCalkowitaHz = 0;   ///< częstotliwość -3 dB sekcji połączonych: f / √(2^(1/n) − 1)
    bool maCs = false;         ///< policzono Cs z fd
    double csPotrzebneNF = 0;  ///< nF, dla fd
    double csSzeregNF = 0;     ///< nF: najbliższa NIE MNIEJSZA z szeregu
    double spadekPrzyFdDb = 0; ///< dB (ujemne): tłumienie całkowite przy fd dla policzonego Cs z szeregu
};
/// nullopt dla Rg <= 0 albo Ra <= 0 albo ujemnych danych albo sekcji spoza 1..8
std::optional<WynikSprzezenia> kondensatorSprzegajacy(const WejscieSprzezenia& w);

// ---------------------------------------------------------------- regulacja barwy (bierne układy)
/// Charakterystyka potencjometru. Udział rezystancji f(x) dla położenia osi x (0..1): liniowa f = x, pozostałe
/// f = a·b^x − a, b = (1/m − 1)², a = 1/(b − 1), gdzie m = f(0,5) to udział w połowie obrotu:
/// log (m = 0,12), log A (m = 0,25), odwrotna log (m = 0,88).
enum class Taper { Liniowy, Log, LogA, OdwrotnyLog };
double udzialRezystancji(double x, Taper t);
double polowaTaperu(Taper t);   ///< m z powyższego wzoru

/// Element netlisty: węzeł 0 to masa, węzeł 2 wejście (za rezystancją źródła), węzeł 3 wyjście.
/// Potencjometr N składa się z dwóch rezystorów: PotLo (R·f) i PotHi (R·(1 − f)), połączonych w węźle suwaka.
enum class Typ { R, C, L, PotLo, PotHi };
struct Skladnik {
    Typ typ = Typ::R;
    int numer = 0;   ///< numer R1.., C1.., L1.. albo potencjometru 1..3
    int w1 = 0, w2 = 0;
};
struct PotUkladu {
    std::string nazwa;      ///< "Bas", "Środek", "Wysokie"...; pusta = potencjometru nie ma
    double calkowita = 0;   ///< Ω
    Taper taper = Taper::Liniowy;
};
struct UkladBarwy {
    std::string nazwa;
    std::vector<Skladnik> skladniki;
    double r[10] = {};  ///< R1..R9 [Ω] (indeks 0 nieużywany)
    double c[8] = {};   ///< C1..C7 [F]
    double l[3] = {};   ///< L1, L2 [H]
    double rzr = 0;     ///< Ω: rezystancja źródła (R98), 0 = idealne źródło
    double robc = 0;    ///< Ω: obciążenie wyjścia (R99), 0 = bez obciążenia
    PotUkladu pot[4];   ///< indeksy 1..3
    /// Układ czynny: węzeł wejścia odwracającego idealnego wzmacniacza operacyjnego (0 = układ bierny). Wyjście wzmacniacza
    /// to węzeł 3. Wzmacniacz jest idealny: napięcie wejścia odwracającego równa się napięciu wejścia nieodwracającego
    /// (opPlus; 0 = masa), więc obciążenie wyjścia (robc) nie wpływa na wynik.
    int opWe = 0;
    int opPlus = 0;   ///< węzeł wejścia nieodwracającego (0 = masa)
    std::string opis;   ///< krótki opis układu (źródło, zastosowanie)
    bool uzywa(Typ t, int numer) const;
    bool uzywaPot(int numer) const;
};
/// Układy wzorcowe: Fender Bassman 5F6-A (wartości typowe), bierne układy z katalogu projektu tonestack
/// (Fender, Marshall, Vox, James, Baxandall, Bandmaster, Bench, Voigt, Bone Ray, Tube Driver, Marshall 18 W, Big Muff, Moonlight)
/// oraz układy czynne z idealnym wzmacniaczem operacyjnym (Baxandall ze sprzężeniem zwrotnym).
const std::vector<UkladBarwy>& ukladyBarwy();

struct NastawyBarwy {
    double poz[4] = {0, 0.5, 0.5, 0.5};   ///< położenia osi potencjometrów 1..3 (0..1), indeks 0 nieużywany
};

/// Stosunek napięcia wyjściowego (węzeł 3) do napięcia źródła, z równań węzłowych; 0 dla błędnych danych
/// (R, C, L <= 0 w użytym elemencie, potencjometr użyty bez rezystancji, położenia poza 0..1, f <= 0).
std::complex<double> odpowiedzUkladu(const UkladBarwy& u, const NastawyBarwy& n, double fHz);
double poziomUkladuDb(const UkladBarwy& u, const NastawyBarwy& n, double fHz);

struct ParametryUkladu {
    double minimumDb = 0;     ///< najgłębsze wycięcie w paśmie 100 Hz do 3 kHz
    double fMinimumHz = 0;
    double poziom100Hz = 0, poziom400Hz = 0, poziom1kHz = 0, poziom4kHz = 0, poziom10kHz = 0;   ///< dB
    double maksimumDb = 0;    ///< najwyższy poziom w paśmie 20 Hz do 20 kHz
};
ParametryUkladu parametryUkladu(const UkladBarwy& u, const NastawyBarwy& n);

} // namespace KalkulatoryWzmacniaczy

#endif // KALKULATORYWZMACNIACZY_H
