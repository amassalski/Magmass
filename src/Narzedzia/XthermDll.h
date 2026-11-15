#ifndef XTHERMDLL_H
#define XTHERMDLL_H

// Opcjonalne przeliczanie surowych klatek kamer Xtherm (T3 itp.) na temperatury biblioteką producenta XthermDll.dll.
// Biblioteki ani jej nagłówka NIE dołączamy do programu (brak licencji na redystrybucję): użytkownik wskazuje własną kopię
// (wersja x64 dla programu 64-bitowego), a program ładuje ją w czasie działania. Deklaracje są własne, na podstawie
// publicznie opisanych nazw i parametrów eksportów. Klatki dostarcza nasza obsługa UVC, biblioteka tylko liczy.
// Tylko Windows; gdzie indziej zaladuj() zwraca błąd. Bez zależności od wxWidgets.

#include <memory>
#include <string>
#include <vector>

namespace XthermLib {

struct Wynik {
    bool ok = false;
    float maxC = 0, minC = 0, srodekC = 0;
    int xMax = 0, yMax = 0, xMin = 0, yMin = 0;
    float fpaC = 0, rdzenC = 0;       ///< temperatury detektora i modułu (GetDevData)
    std::vector<float> wszystkie;     ///< temperatury całej klatki szer·wys (tylko tryb pełnoklatkowy)
};

class Biblioteka {
public:
    Biblioteka();
    ~Biblioteka();
    Biblioteka(const Biblioteka&) = delete;
    Biblioteka& operator=(const Biblioteka&) = delete;

    bool zaladuj(const std::string& sciezkaUtf8, std::string& blad);
    bool zaladowana() const;
    void zwolnij();

    /// Klatka surowa: szer·wysKlatki słów 16-bitowych, wysKlatki = wiersze obrazu + 4 wiersze parametrów (np. 384x292).
    /// pelnaKlatka: true = temperatura każdego piksela (tryb surowy 0x8004), false = tylko punkty (tryb 0x8005).
    /// odswiezParametry: wczytuje kalibrację z nagłówka klatki (po starcie i po kalibracji migawką).
    Wynik oblicz(const unsigned char* klatka, size_t bajtow, int szer, int wysKlatki, bool pelnaKlatka, bool odswiezParametry,
                 bool zakresWysoki = false);

private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};

}  // namespace XthermLib

#endif  // XTHERMDLL_H
