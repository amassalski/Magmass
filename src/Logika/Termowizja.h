#ifndef TERMOWIZJA_H
#define TERMOWIZJA_H

// Obróbka klatek z kamery termowizyjnej USB (np. InfiRay T3 Pro, P2 Pro): rozpoznanie układu ramki, temperatury
// z surowych danych, statystyki i palety barw. Bez zależności od wxWidgets i OpenCV (testowalne).
//
// Moduły Xtherm (T3 itp., ramka 384x292): dokumentacja producenta opisuje 4 wiersze parametrów na dole ramki, tryb surowy (polecenie
// UVC 0x8004, 14-bitowe odczyty NUC) i YUYV (0x8005); temperatura wymaga fabrycznych parametrów z tych wierszy, temperatury detektora
// i poprawek, więc prosty wzór surowe/dzielnik nie ma zastosowania (dane przykładowe producenta: odczyty 5808...6245 to 36...44 °C).
// Dla takich ramek program używa kalibracji użytkownika albo (opcjonalnie) biblioteki producenta ładowanej z kopii użytkownika (XthermDll.h).
//
// Uwaga o danych: producent nie publikuje protokołu temperatur. Przyjęty układ (obraz w górnej połowie ramki YUYV,
// 16-bitowe dane radiometryczne w dolnej, T[°C] = surowe / 64 − 273,15) jest HIPOTEZĄ opartą na opisach społeczności
// dla kamer tej rodziny. Dzielnik i przesunięcie są parametrami, które trzeba zweryfikować pomiarem
// (np. woda z lodem 0 °C, dłoń ok. 33 °C); program nie traktuje tych wartości jako pewnych.

#include <cstdint>
#include <string>
#include <vector>

namespace Termowizja {

/// Surowa klatka z kamery: bajty tak, jak je dał sterownik (np. YUYV: kanaly = 2, bajtyNaKanal = 1)
struct KlatkaSurowa {
    int szer = 0;            ///< szerokość w pikselach
    int wys = 0;             ///< wysokość w wierszach
    int kanaly = 0;
    int bajtyNaKanal = 1;
    std::string opisTypu;    ///< np. "CV_8UC2"
    std::vector<unsigned char> dane;
    int bajtyNaPiksel() const { return kanaly * bajtyNaKanal; }
    size_t bajtyWiersza() const { return static_cast<size_t>(szer) * bajtyNaPiksel(); }
    bool pusta() const { return szer <= 0 || wys <= 0 || dane.size() < bajtyWiersza() * wys; }
};

enum class Uklad {
    TylkoObraz,    ///< tylko obraz widzialny dla oka (bez temperatur)
    ObrazIDane,    ///< górna połowa: obraz, dolna: 16-bitowe dane radiometryczne (np. ramka 256x384)
    ObrazZNaglowkiem,  ///< 8-bitowy obraz w jasności Y (chrominancja stała 0x80) i 4 wiersze nagłówka na dole (np. 384x292 = 384x288 + 4)
    Surowe16       ///< jak wyżej, ale wiersze obrazu niosą surowe 16-bitowe odczyty z sensora (po przełączeniu kamery w tryb surowy); bez kalibracji do °C
};

struct RozpoznanyUklad {
    Uklad uklad = Uklad::TylkoObraz;
    int obrazSzer = 0, obrazWys = 0;
    std::string opis;
};

/// Rozpoznaje układ na podstawie rozmiaru: ramka 2 bajty na piksel o wysokości 2·(3/4·szer) (np. 256x384: obraz
/// 256x192 i dane 256x192) to ObrazIDane; wszystko inne to TylkoObraz.
/// Ramka 2 bajty na piksel o wysokości szer·3/4 + 4 (np. 384x292) ma 4 wiersze nagłówka: gdy bajty chrominancji obrazu są wszędzie 0x80,
/// to ObrazZNaglowkiem (zwykły obraz szary), inaczej Surowe16.
RozpoznanyUklad rozpoznaj(const KlatkaSurowa& k);

/// Opis 4 wierszy nagłówka (układy ObrazZNaglowkiem i Surowe16): słowa 16-bitowe pierwszego wiersza i liczby zmiennoprzecinkowe ostatniego.
/// Znaczenie pól nie jest opublikowane przez producenta, to tylko zrzut do analizy.
std::string opisNaglowka(const KlatkaSurowa& k, const RozpoznanyUklad& u);

/// Surowe 16-bitowe odczyty (little endian), szer·wysObrazu liczb: z dolnej połowy ramki (ObrazIDane) albo z wierszy obrazu (Surowe16).
/// Puste, gdy układ ich nie ma.
std::vector<uint16_t> daneRadiometryczne(const KlatkaSurowa& k, const RozpoznanyUklad& u);

/// T[°C] = surowe / dzielnik − offsetK. 0 przy dzielniku <= 0.
double naCelsjusza(uint16_t surowe, double dzielnik = 64.0, double offsetK = 273.15);

struct Statystyka {
    bool ok = false;
    double minC = 0, maxC = 0, sredniaC = 0, srodekC = 0;
    int xMin = 0, yMin = 0, xMax = 0, yMax = 0;
};
Statystyka statystyki(const std::vector<uint16_t>& dane, int szer, int wys, double dzielnik = 64.0, double offsetK = 273.15);

enum class Paleta { Zelazo, Teczowa, Szara, SzaraOdwrocona };
/// Kolor palety dla wartości znormalizowanej 0...1 (poza zakresem obcinane)
void kolorPalety(Paleta p, double t01, unsigned char rgb[3]);

/// Obraz RGB (3 bajty na piksel) z danych radiometrycznych, skala od najmniejszej do największej wartości w klatce
std::vector<unsigned char> obrazZDanych(const std::vector<uint16_t>& dane, int szer, int wys, Paleta p);
/// Obraz RGB z jasności obrazu YUYV (bajty Y) albo RGB/BGR: gdy nie ma danych radiometrycznych
std::vector<unsigned char> obrazZJasnosci(const KlatkaSurowa& k, const RozpoznanyUklad& u, Paleta p);

/// Raport diagnostyczny: rozmiar, typ, układ, pierwsze bajty, zakres wartości, temperatury przy podanych parametrach
std::string raportDiagnostyczny(const KlatkaSurowa& k, double dzielnik = 64.0, double offsetK = 273.15);

/// Punkt kalibracji użytkownika: surowy odczyt sensora i znana temperatura tego miejsca
struct PunktKalibracji { double surowe = 0, temperaturaC = 0; };

/**
 * Kalibracja użytkownika surowych odczytów (kamery HT-301/T3 Pro w trybie surowym): wzór producenta nie jest publiczny, więc
 * temperaturę wyznacza się z punktów odniesienia (np. woda z lodem 0 °C, wrząca woda). Przy dwóch punktach to prosta, przy
 * większej liczbie łamana przez punkty (poza zakresem przedłużana ostatnim odcinkiem). Wynik jest orientacyjny: odczyt sensora
 * zależy też od jego temperatury, więc kalibrację warto powtórzyć po rozgrzaniu kamery.
 */
class KalibracjaUzytkownika {
public:
    /// Dodaje punkt; punkt o tym samym odczycie (±0,5) zastępuje poprzedni
    void dodaj(double surowe, double temperaturaC);
    void wyczysc() { p_.clear(); }
    size_t liczba() const { return p_.size(); }
    bool gotowa() const { return p_.size() >= 2; }
    const std::vector<PunktKalibracji>& punkty() const { return p_; }
    /// °C dla odczytu; 0 gdy kalibracja nie jest gotowa
    double naCelsjusza(double surowe) const;
private:
    std::vector<PunktKalibracji> p_;   // posortowane rosnąco po odczycie
};

/// Średnia odczytów w kwadracie o boku 2·promien+1 wokół (cx, cy)
double sredniaSurowych(const std::vector<uint16_t>& d, int szer, int wys, int cx, int cy, int promien);
/// Jak statystyki(), ale temperatury z kalibracji użytkownika
Statystyka statystykiKalibrowane(const std::vector<uint16_t>& d, int szer, int wys, const KalibracjaUzytkownika& k);

/// Klatka testowa trybu surowego 384x292: 288 wierszy odczytów 14-bitowych (gradient i gorący punkt) i 4 wiersze nagłówka
KlatkaSurowa klatkaTestowaSurowa(long numer = 0);

/// Klatka testowa 256x384 YUYV (obraz + dane) z gradientem 20...80 °C i gorącym punktem: do testów bez kamery
KlatkaSurowa klatkaTestowa(long numer = 0);

}  // namespace Termowizja

#endif  // TERMOWIZJA_H
