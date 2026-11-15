#ifndef NARZEDZIE_H
#define NARZEDZIE_H

#include <optional>
#include <string>

/// Narzędzie użytkownika (tbl_narzedzia). Daty jako "RRRR-MM-DD" (puste = brak).
struct Narzedzie {
    long long id = 0;            ///< 0 = jeszcze nie zapisane
    std::string nazwa;           ///< wymagana
    std::string producent;
    std::string model;
    std::string kategoria;
    std::string numerSeryjny;
    std::string numerInwentarzowy;   ///< własny numer inwentarzowy (np. z naklejki)
    std::string miejsce;
    std::string dataZakupu;
    std::optional<double> cena;
    std::string zdjecie;
    std::string instrukcja;      ///< plik lub adres
    std::string uwagi;

    // tylko do odczytu: otwarta pożyczka
    bool pozyczone = false;
    std::string pozyczonePrzez;
    std::string pozyczoneOd;
    std::string termin;
    bool przeterminowane = false;
};

/// Pożyczka narzędzia albo książki (tbl_pozyczki; dokładnie jedno z idNarzedzia i idKsiazki jest niezerowe)
struct Pozyczka {
    long long id = 0;
    long long idNarzedzia = 0;
    long long idKsiazki = 0;
    std::string nazwaNarzedzia;  ///< tylko do odczytu: nazwa narzędzia albo tytuł książki
    std::string osoba;
    std::string od;
    std::string termin;
    std::string zwrot;           ///< pusty = nadal pożyczone
    std::string uwagi;
    bool przeterminowana = false;
};

#endif // NARZEDZIE_H
