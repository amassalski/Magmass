#ifndef OBUDOWA_H
#define OBUDOWA_H

#include <optional>
#include <string>
#include <vector>

/// Obudowa ze słownika (tbl_obudowy), np. "R7.5", "TO220AH", "0603"; element może mieć ich wiele (tbl_elementy_obudowy)
struct Obudowa {
    long long id = 0;                    ///< 0 = jeszcze nie zapisana
    std::string nazwa;                   ///< wymagana; tak samo wpisuje się ją w polu "Obudowa" elementu
    std::string opis;
    std::string montaz;                  ///< "THT", "SMD" albo pusty
    int liczbaPinow = 0;                 ///< 0 = nieznana
    std::optional<double> mocStratW;     ///< dopuszczalna moc strat obudowy
    std::optional<double> napiecieMaxV;  ///< dopuszczalne napięcie
    std::string rozstaw;                 ///< np. "2,54 mm"
    std::string uwagi;
    std::string rodzina;                 ///< rodzina z biblioteki Eagle, np. "TO-220" (do grupowania); może być pusta
    std::string wariant;                 ///< oznaczenie wariantu, np. "AH"; może być puste

    int liczbaElementow = 0;             ///< tylko do odczytu: aktywne elementy z tą obudową
};

#endif // OBUDOWA_H
