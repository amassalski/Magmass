#ifndef KSIAZKA_H
#define KSIAZKA_H

#include <string>

/// Książka z biblioteczki użytkownika (tbl_ksiazki)
struct Ksiazka {
    long long id = 0;            ///< 0 = jeszcze nie zapisana
    std::string tytul;           ///< wymagany
    std::string autor;
    std::string wydawca;
    int rok = 0;                 ///< 0 = nieznany
    std::string wydanie;         ///< np. "wydanie drugie"
    std::string isbn;
    std::string jezyk;
    std::string tagi;
    std::string uwagi;
    std::string sciezka;         ///< plik PDF/EPUB na dysku (może być pusta: książka papierowa)
    int strony = 0;              ///< liczba zaindeksowanych stron (0 = brak indeksu)
    std::string zaindeksowano;   ///< data indeksowania lub pusty

    // tylko do odczytu: otwarta pożyczka
    bool pozyczona = false;
    std::string pozyczonaPrzez;
    std::string pozyczonaOd;
    std::string termin;
    bool przeterminowana = false;
};

/// Trafienie w treści książki
struct TrafienieKsiazki {
    long long idKsiazki = 0;
    std::string tytul;
    std::string sciezka;
    int strona = 0;
    std::string fragment;
};

#endif // KSIAZKA_H
