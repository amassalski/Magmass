#ifndef SERWIS_H
#define SERWIS_H

#include <optional>
#include <string>
#include <vector>

/// Część wymieniona lub zamontowana podczas serwisu (tbl_serwis_czesci)
struct CzescSerwisowa {
    long long id = 0;
    std::string nazwa;
    double ilosc = 1;
    std::string numerStary;      ///< numer seryjny elementu zdjętego
    std::string numerNowy;       ///< numer seryjny elementu zamontowanego
    std::string uwagi;
    std::optional<long long> idElementu;   ///< element z magazynu, z którego pochodzi część
    std::string symbolElementu;            ///< tylko do odczytu: symbol elementu z magazynu
    bool pobrano = false;                  ///< ilość została już zdjęta ze stanu magazynu (zapisane w bazie)
    bool pobierz = false;                  ///< prośba: przy zapisie zdjąć ilość ze stanu magazynu (nie zapisywane)
    bool pierwszePobranie() const { return pobierz && !pobrano; }
};

/// Zdjęcie dołączone do wpisu serwisowego (tbl_serwis_zdjecia)
struct ZdjecieSerwisu {
    long long id = 0;
    std::string sciezka;
    std::string opis;
};

/// Wpis ewidencji serwisowej (tbl_serwis). Daty "RRRR-MM-DD".
struct WpisSerwisowy {
    long long id = 0;            ///< 0 = jeszcze nie zapisany
    std::string data;            ///< pusta = dziś
    std::string urzadzenie;      ///< wymagane
    std::string producent;
    std::string model;
    std::string numerSeryjny;
    std::string lokalizacja;
    std::string rodzaj = "naprawa";
    std::string status = "zakończone";
    std::string zgloszenie;
    std::string technik;
    std::optional<int> czasMin;
    std::string usterka;
    std::string wykonano;
    std::string uwagi;
    std::vector<CzescSerwisowa> czesci;
    std::vector<ZdjecieSerwisu> zdjecia;
    // tylko do odczytu na liście
    int liczbaCzesci = 0;
    int liczbaZdjec = 0;
};

#endif // SERWIS_H
