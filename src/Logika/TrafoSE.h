#ifndef TRAFOSE_H
#define TRAFOSE_H

#include <optional>
#include <vector>
#include "TrafoModel.h"

/// Transformator głośnikowy jednotaktowy (SE) z prądem stałym lampy i szczeliną w rdzeniu.
/// Wzory fizyczne (rząd równoległy Ra∥Ri, strumień ze składowej stałej i zmiennej, szczelina z reluktancji);
/// wartości z programu porównawczego (zrzut użytkownika) liczone obok jako porównanie.
namespace TrafoSE {

struct Wejscie {
    double mocVA = 10;        ///< przewidywana moc wyjściowa [VA]
    double raOm = 2000;       ///< obciążenie anodowe lampy Ra [Ω]
    double riOm = 15000;      ///< rezystancja wewnętrzna lampy [Ω]
    double iaMa = 85;         ///< prąd anodowy spoczynkowy [mA]
    double fdHz = 50;         ///< najniższa przenoszona częstotliwość [Hz]
    double rgOm = 4;          ///< rezystancja głośnika [Ω]
    double eta = 0.85;        ///< sprawność transformatora
    double drogaCm = 17;      ///< średnia droga magnetyczna [cm]
    double przekrojCm2 = 10.5;///< przekrój rdzenia [cm²]
    double bMaxT = 1.0;       ///< dopuszczalna indukcja szczytowa (składowa stała + zmienna) [T]
    double mi = 1000;         ///< przenikalność względna żelaza w rdzeniu ze szczeliną
    double gestoscAmm2 = 2.5; ///< gęstość prądu w uzwojeniach [A/mm²]
    double spadekDb = 3;      ///< dopuszczalny spadek na krańcach pasma: 1, 2 lub 3 dB (3 = punkt połowy mocy)
    // okno rdzenia (tylko dla uzwojeń wykonanych)
    double dlugoscZwojuCm = 0;      ///< obwód rdzenia pod uzwojeniem (długość zwoju przy rdzeniu) [cm]
    double wysokoscOknaCm = 0;      ///< wysokość uzwojenia (okna) [cm]
    double szerokoscOknaCm = 0;     ///< szerokość okna [cm]
    double wspolczynnikAKrize = 20; ///< A w Q0·Qs = A·P (Krize, tab. 3): 20 pentoda bez NFB, 10 pentoda z NFB lub trioda bez NFB, 5 trioda z NFB
};

struct Wynik {
    double rownolegleOm = 0;       ///< Ra∥Ri
    double liH = 0;                ///< indukcyjność pierwotna: Ra∥Ri/(2π·fd) (spadek -3 dB przy fd)
    double liPorownanieH = 0;      ///< wartość z programu porównawczego: Ra∥Ri/(7,2·fd) (przy fd = 50 Hz: Ra∥Ri/360)
    double przekladnia = 0;        ///< n = √(η·Ra/Rg): Ra widziane z lampy to n²·Rg/η (rezystancje uzwojeń szeregowo z obciążeniem)
    double przekladniaPorownanie = 0; ///< wartość z programu porównawczego: √(Ra/(η·Rg))
    double przekrojOrientacyjnyCm2 = 0; ///< reguła kciuka 2,5·√P (porównawczo)
    double pradZmiennyPierwotnyMa = 0;  ///< wartość szczytowa składowej zmiennej
    double pradSzczytowyMa = 0;    ///< Ia + składowa zmienna
    double mocMaxKlasaAVA = 0;     ///< Ia²·Ra/2·η: moc maksymalna w klasie A przy tym prądzie
    bool przesterowanie = false;   ///< żądana moc większa niż możliwa w klasie A
    int zwojePierwotne = 0, zwojeWtorne = 0;
    double drutPierwotnyMm = 0, drutWtornyMm = 0;
    double pradPierwotnyRmsA = 0, pradWtornyA = 0;
    double szczelinaMm = 0;        ///< szczelina całkowita (fizyczna, z reluktancji żelaza)
    bool brakSzczeliny = false;    ///< szczelina wyszła ≤ 0: rdzeń bez szczeliny wystarcza
    double amperozwojeNaCm = 0;    ///< z1·Ia/lFe [A·zw./cm]: Cykin XI.3 zaleca szczelinę powyżej 1-2
    double indukcjaStalaT = 0;     ///< indukcja od samej składowej stałej
    double indukcjaSzczytowaT = 0;
};

std::optional<Wynik> oblicz(const Wejscie& w);

/// Wzory z literatury, liczone dla tych samych danych tylko do porównania z modelem fizycznym.
/// Krize (Transformatory wyjściowe, transformator z podmagnesowaniem, wzory 22-28) i Witort (Amatorskie wzmacniacze elektroakustyczne, 1972, rozdz. 11).
struct Literatura {
    // Krize
    double krizeZwoje23 = 0;          ///< w1 = 10·L1·I0/Qs·[1 + √(1 + 3000·Qs·ls/(L1·I0²))], I0 w mA
    double krizeZwoje24 = 0;          ///< w1 = 45·L1·I0/Qs
    double krizeN = 0;                ///< n = w2/w1 = √(Rg/Ra) (bez sprawności)
    double krizeZwojeWtorne = 0;      ///< w2 = n·w1 (z (23))
    double krizeSzczelinaMm = 0;      ///< l3 = w1·I0/8·10⁻⁵ mm, I0 w mA (to samo co μ0·N·I/B dla B = 1 T, bez reluktancji żelaza)
    double krizePrzekladkaMm = 0;     ///< dla rdzenia płaszczowego połowa l3
    double krizeR1Om = 0;             ///< r1 = Ra/2·(1 − η)
    double krizeDrutPierwotnyMm = 0;  ///< d1 = 0,015·√(l0·w1/r1), l0 = długość zwoju; 0 gdy brak wymiarów
    double krizeDrutMinMm = 0;        ///< d1 ≥ 0,025·√I0 (I0 w mA): warunek nagrzewania
    double krizeDrutWtornyMm = 0;     ///< d2 = d1/√n
    double krizeIloczynWymaganyCm4 = 0;  ///< Q0·Qs ≥ A·P
    double krizeIloczynRdzeniaCm4 = 0;   ///< Q0·Qs rdzenia (0 gdy brak wymiarów okna)
    // Witort
    double witortQMinCm2 = 0, witortQMaxCm2 = 0;  ///< Q = (3 ÷ 5)·√P
    double witortZwoje = 0;           ///< n1 = 9000·√(L1·lr/(μr·Q)), μr = 500 (rdzeń bez podmagnesowania i bez szczeliny)
    double witortL1H = 0;             ///< L1 ≥ Ra/(6,28·fd) (sama Ra, bez Ri)
    double witortR1Om = 0, witortR2Om = 0;        ///< klasa A: r1 = Zob·p²·(1 − η)/2, r2 = Zob·(1 − η)/(2η)
};
Literatura literatura(const Wejscie& w, const Wynik& p);

/// Uzwojenia wykonane: całkowite zwoje, normalne średnice drutu, szczelina i układ uzwojeń w oknie.
struct Wykonane {
    int zwojePierwotne = 0, zwojeWtorne = 0;
    double drutPierwotnyMm = 0, drutWtornyMm = 0;       ///< średnice gołego drutu
    double szczelinaMm = 0;                             ///< szczelina całkowita (suma szczelin na drodze strumienia)
    double sciankaKarkasuMm = 1.5;                      ///< policzek karkasu z każdej strony okna i od strony rdzenia
    double marginesWarstwyMm = 1.5;                     ///< wolny pas na brzegu każdej warstwy; 0 = warstwa na całej wysokości okna
    enum class Uklad { nieprzeplatane, pierwotnePodzielone, wtornePodzielone };
    Uklad uklad = Uklad::nieprzeplatane;                ///< P-S; P/2-S-P/2; S/2-P-S/2
    double przekladkaWarstwMm = 0.05;                   ///< papier między warstwami
    double izolacjaUzwojenMm = 0.5;                     ///< izolacja między uzwojeniami
    double kEfektyBrzegowe = 0;                         ///< poprawka pola rozproszenia: 0 = automatycznie (współczynnik Rogowskiego), >0 = wartość ręczna (1,0 = model idealny)
    double rezystywnosc = 0.0178;                       ///< miedź [Ω·mm²/m]
    double fgHz = 20000;                                ///< wymagana górna częstotliwość (do oceny rozproszenia)
};

struct SekcjaUzwojenia {
    bool pierwotna = true;
    int zwoje = 0, zwojeWWarstwie = 0, warstwy = 0;
    double gruboscMm = 0, dlugoscZwojuCm = 0;
    bool nieparzysteWarstwy = false;
};

struct WynikWykonane {
    double liH = 0;                 ///< indukcyjność pierwotna wykonana (z szczeliny i zwojów, liniowo)
    double fdWykonanaHz = 0;        ///< dolna częstotliwość -3 dB z rezystancji szeregowej Ri∥(Ra) i Li
    double przekladnia = 0;         ///< z1/z2
    double raWidzianeOm = 0;        ///< Ra widziane z lampy: Rp + n²(Rg + Rw)
    double indukcjaStalaT = 0, indukcjaSzczytowaT = 0;
    std::vector<SekcjaUzwojenia> sekcje;                ///< od rdzenia na zewnątrz
    double zajeteCm2 = 0, miejsceCm2 = 0;
    bool miesciSie = false;
    double rpOm = 0, rwOm = 0;
    double spadekStalyV = 0;        ///< Ia·Rp: spadek napięcia stałego na uzwojeniu pierwotnym
    double stratyStaleW = 0;        ///< Ia²·Rp
    double lrH = 0;                 ///< rozproszenie dla wybranego układu (sprowadzone do pierwotnego)
    double lrUkladH[3] = {0, 0, 0}; ///< to samo dla trzech układów: nieprzeplatane, pierwotne podzielone, wtórne podzielone
    double fgWykonanaHz = 0;        ///< górna częstotliwość -3 dB: (Ri + Ra)/(2π·Lr)
    double lrMaxH = 0;              ///< największe Lr dla żądanego Fg
    double sprawnosc = 0;           ///< P/(P + straty zmienne w miedzi)
    bool szczelinaUjemna = false;
    // porównanie z książką: Lr = 4π·k·z1²·l/(10⁹·h)·(δ + a/3), a = szerokość okna
    double kUzyte = 1;              ///< poprawka brzegowa użyta do Lr wybranego układu
    double lrKsiazkaH = 0;
};
std::optional<WynikWykonane> wykonane(const Wejscie& w, const Wykonane& u);

/// Analiza dokładna wykonanego transformatora jednym wspólnym modelem (TrafoModel): schemat zastępczy z polaryzacją, rozproszeniem,
/// pojemnością własną i stratami w żelazie.
std::optional<TrafoModel::Wynik> analizaDokladna(const Wejscie& w, const Wykonane& u, const WynikWykonane& o, const TrafoModel::Parametry& p);

} // namespace TrafoSE

#endif
