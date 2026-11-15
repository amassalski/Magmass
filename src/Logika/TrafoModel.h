#ifndef TRAFOMODEL_H
#define TRAFOMODEL_H

#include <optional>
#include <vector>

/// Jeden, wspólny model fizyczny transformatora głośnikowego (SE i PP) – pełny schemat zastępczy bez uproszczeń typu „stała μ” czy „−3 dB z Rp/ω”.
///
/// Schemat (wszystko sprowadzone do strony pierwotnej, n = z1/z2):
///   źródło E – Ri – Rp – węzeł A;   węzeł A: Cd ∥ Lp(przyrostowa, z polaryzacją) ∥ Rc(f) ∥ [Lr + n²·Rw + n²·Rg]
/// - Lp = z1²/(lFe/(μ0·μprzyr·S) + lg/(μ0·Ag)); punkt pracy z równania amperozwojów z1·I = H(B)·lFe + B·S·lg/(μ0·Ag)
///   i krzywej magnesowania Fröhlicha-Kennelly'ego B = μ0·μi·H/(1 + μ0·μi·H/Bs)  =>  μprzyr = μi·(1 − B/Bs)²
/// - Ag = (a+lg)(b+lg) (rozpływ pola w szczelinie), jeśli znane wymiary kolumny środkowej
/// - Rc(f) = (ω·z1·S)²/(2·pFe·m·(f/50)^1,5): straty w żelazie P = pFe·m·(f/50)^1,5·B² (histereza + wiry)
/// - pasmo: przedziały -x dB względem pasma środkowego (1 kHz) liczone z pełnej transmitancji zespolonej
/// - indukcja szczytowa z napięcia w węźle A przy częstotliwości dolnej i pełnej mocy
/// Literatura (Langford-Smith, Krize, Witort, Cykin, strona gitarowa) służy tylko do porównania.
namespace TrafoModel {

struct Rdzen {
    double przekrojCm2 = 0;          ///< przekrój czynny żelaza (z uwzględnieniem współczynnika wypełnienia)
    double drogaCm = 0;              ///< średnia droga strumienia w żelazie
    double szerSrodkowaCm = 0;       ///< szerokość kolumny środkowej a (0 = bez rozpływu w szczelinie)
    double pakietCm = 0;             ///< grubość pakietu b
    double muPoczatkowa = 3000;      ///< przenikalność początkowa samego żelaza μi
    double bNasycenieT = 1.8;        ///< indukcja nasycenia w krzywej Fröhlicha Bs
    double szczelinaStykuMm = 0;     ///< dodatkowa szczelina styku E–I; PP i blachy składane na przemian (raz E z jednej, raz z drugiej strony) – brak; SE ze szczeliną – szczelina celowa jest osobno
    double stratyZelazaWkg = 2.5;    ///< straty w żelazie przy 1 T, 50 Hz [W/kg]
    double gestoscKgDm3 = 7.65;
};

struct Uzwojenia {
    double zwojePierwotne = 0, zwojeWtorne = 0;
    double rpOm = 0, rwOm = 0;      ///< rezystancje uzwojeń (DC)
    double lrH = 0;                 ///< indukcyjność rozproszenia sprowadzona do pierwotnego
    double cdF = 0;                 ///< pojemność własna sprowadzona do pierwotnego
    double szczelinaCelowaMm = 0;   ///< szczelina w rdzeniu (SE)
};

struct Zrodlo {
    bool przeciwsobny = false;
    double riOm = 0;                ///< rezystancja wewnętrzna lampy (PP: anoda-anoda)
    double rgOm = 0;                ///< impedancja obciążenia (głośnik, prąd zmienny)
    double mocW = 0;                ///< moc znamionowa
    double pradStalyA = 0;          ///< SE: prąd anodowy; PP: różnica prądów anodowych połówek
    double spadekDb = 3;            ///< wymagany spadek na krańcach pasma
    double fMocyHz = 0;             ///< najniższa częstotliwość, przy której ma być oddana pełna moc (0 = uzyskane fd)
};

struct PunktKrzywej { double fHz, db; };

struct Wynik {
    double przekladnia = 0;
    double raWidzianeOm = 0;        ///< Re(Zwe) w paśmie środkowym
    double sprawnoscMiedz = 0;      ///< n²Rg/(n²(Rg+Rw)+Rp)
    double sprawnoscCalkowita = 0;  ///< z uwzględnieniem strat w żelazie przy fd
    double lpH = 0, lpBezPolaryzacjiH = 0, lpSzczytH = 0;
    double muPrzyrostowa = 0;
    double bStalaT = 0, bZmiennaT = 0, bSumaT = 0;
    double szczelinaCalkowitaMm = 0;
    double amperozwojeNaCm = 0;
    double fdHz = 0, fgHz = 0;      ///< krańce pasma (spadek jak w Zrodlo::spadekDb), 0 = nie osiągnięto
    double rezonansHz = 0;          ///< maksimum charakterystyki powyżej pasma (0 = brak)
    double nadmiarRezonansuDb = 0;
    double spadekDcV = 0, stratyDcW = 0;
    double stratyZelazaW = 0;
    bool nasycenie = false;         ///< B stała + zmienna > 0,9·Bs
    std::vector<PunktKrzywej> krzywa;
};

/// Współczynnik k = √(M²−1), M = amplitudowy spadek na krańcu pasma: dla jednobiegunowego filtru f(−x dB) = f₀·k (górny) lub f₀/k (dolny).
/// 3 dB oznacza punkt połowy mocy (M = √2, 3,01 dB), czyli k = 1; 1 dB → 0,508; 2 dB → 0,765.
double wspolczynnikSpadku(double spadekDb);
/// Dokładny spadek w dB używany do wyszukiwania krańców pasma (3 → 3,0103).
double spadekDokladnyDb(double spadekDb);

std::optional<Wynik> analiza(const Rdzen& r, const Uzwojenia& u, const Zrodlo& z);

/// Szczelina [mm] (bez styku) dająca zadaną indukcyjność przyrostową; 0 gdy już bez szczeliny indukcyjność jest za mała.
std::optional<double> szczelinaDlaIndukcyjnosci(const Rdzen& r, double zwoje, const Zrodlo& z, double lWymaganaH);

/// Najmniejsza liczba zwojów pierwotnych dającą Lp >= lWymaganaH przy zadanej szczelinie (PP lub SE z gotową szczeliną)
std::optional<double> zwojeDlaIndukcyjnosci(const Rdzen& r, double szczelinaCelowaMm, const Zrodlo& z, double lWymaganaH);

/// Pojemność własna sprowadzona do pierwotnego: pojemności międzywarstwowe (energia ½CV², układ „tam i z powrotem”: C_eff = 4/3·(N−1)/N²·C_warstw)
/// oraz pojemność pierwotne–wtórne (w najgorszym przypadku pełne napięcie uzwojenia).
struct SekcjaPojemnosci {
    bool pierwotna = true;
    int warstwy = 0;
    double zwoje = 0;                ///< zwoje sekcji (udział napięcia uzwojenia)
    double dlugoscZwojuCm = 0, wysokoscCm = 0;
    double izolacjaWarstwMm = 0.1;   ///< papier + emalia między warstwami
};
double pojemnoscWlasna(const std::vector<SekcjaPojemnosci>& s, double przekladnia, double izolacjaPSMm, double wagaPS, double epsR = 3.0);

/// Sekcja uzwojenia PP do obliczenia pojemności z rzeczywistych potencjałów warstw (znormalizowane napięcie pierwotne V1 = 1, środek uzwojenia
/// pierwotnego na masie: φ(t) = t/z1 − ½; wtórne jednym końcem na masie: ψ(s) = s/z2).
struct SekcjaPP {
    bool pierwotna = true;
    double zwoje = 0, zwojePrzed = 0;    ///< zwoje sekcji i zwoje tego samego uzwojenia wcześniej w kolejności nawijania
    double warstwy = 0, zwojeWWarstwie = 0, zwojeOstatniejWarstwy = 0;
    double dlugoscZwojuCm = 0, wysokoscCm = 0;
    int komora = 0;
};
/// Pojemność własna sprowadzona do pierwotnego: pojemności międzywarstwowe sekcji (4/3·(N−1)/N² · C_warstw, udział napięcia² sekcji)
/// oraz pojemności między sąsiednimi sekcjami pierwotna–wtórna w tej samej komorze: C·(średni kwadrat różnicy potencjałów warstw
/// zwróconych do siebie; przy nieznanym kierunku nawijania bierzemy gorszy z dwóch przypadków).
double pojemnoscPP(const std::vector<SekcjaPP>& s, double zwojePier, double zwojeWt, double przekladnia, double izolWarstwMm, double izolPSMm, double epsR = 3.0);

/// Parametry materiału i modelu wspólne dla okien SE i PP
struct Parametry {
    double muPoczatkowa = 3000, bNasycenieT = 1.8, szczelinaStykuMm = 0, stratyZelazaWkg = 2.5;
    double spadekDb = 3, epsR = 3.0;
    double asymetriaPPA = 0.005;     ///< PP: różnica prądów anodowych połówek (zalecane ≤ 5 mA)
};

} // namespace TrafoModel

#endif
