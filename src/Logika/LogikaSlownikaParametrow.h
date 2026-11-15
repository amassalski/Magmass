#ifndef LOGIKASLOWNIKAPARAMETROW_H
#define LOGIKASLOWNIKAPARAMETROW_H

#include "../BazaDanych/ManagerBazyDanych.h"

#include <string>
#include <vector>

/// Wpis słownika opisów parametrów (tbl_slownik_parametrow)
struct WpisSlownikaParametrow {
    long long id = 0;      ///< 0 = nowy
    std::string symbol;    ///< np. "W_amax"
    std::string opis;      ///< np. "Maksymalna dopuszczalna moc strat anody [W]"
};

/**
 * @class LogikaSlownikaParametrow
 * @brief Słownik opisów symboli parametrów katalogu, przechowywany w bazie (wspólny dla wszystkich komputerów).
 *        Symbol porównywany jest bez wielkości liter, spacji, podkreśleń i nawiasów (OpisyParametrow::klucz).
 */
class LogikaSlownikaParametrow {
public:
    explicit LogikaSlownikaParametrow(ManagerBazyDanych& manager) : manager_(manager) {}

    std::string pobierzBlad() const { return blad_; }

    /// Wpisy posortowane po symbolu; @p szukaj filtruje po symbolu i opisie (bez wielkości liter)
    bool pobierz(const std::string& szukaj, std::vector<WpisSlownikaParametrow>& wynik);
    /// Dodaje (id = 0) albo zmienia wpis; zwraca id. Symbol o tym samym kluczu już istnieje = błąd.
    std::optional<long long> zapisz(const WpisSlownikaParametrow& wpis);
    bool usun(long long id);
    /// Dopisuje wbudowane symbole, których w słowniku brakuje (istniejących nie rusza); zwraca liczbę dopisanych (-1 = błąd)
    int przywrocBrakujace();
    /// Wczytuje słownik z bazy do pamięci (OpisyParametrow::ustawZBazy), żeby dymki go używały
    bool wczytajDoPamieci();

private:
    ManagerBazyDanych& manager_;
    std::string blad_;
    PolaczenieBazy* polaczenie();
    bool ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem = "");
};

#endif // LOGIKASLOWNIKAPARAMETROW_H
