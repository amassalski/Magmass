#ifndef LOGIKAOBUDOW_H
#define LOGIKAOBUDOW_H

#include "../BazaDanych/ManagerBazyDanych.h"
#include "../Import/ImportLBR.h"
#include "../Model/Obudowa.h"
#include "KluczObudowy.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

struct WynikImportuObudow {
    int dodano = 0;          ///< nowe obudowy
    int uzupelniono = 0;     ///< istniejące obudowy, którym dopisano brakujące atrybuty
    int typowych = 0;        ///< zapisane typowe elementy (z zestawów biblioteki)
    std::vector<std::string> bledy;
};

/// Typowy element w obudowie (z zestawu biblioteki Eagle)
struct TypowyElement {
    std::string zestaw;      ///< symbol elementu, np. "BD437" albo "2N2222-TO18"
    std::string opis;
    std::string prefiks;     ///< np. "Q"
};

/**
 * @class LogikaObudow
 * @brief Słownik obudów. Element ma ich wiele (tekst pola "Obudowa" jest rozbijany przez trigger bazy na relację
 *        tbl_elementy_obudowy). Nazwy porównuje się bez wielkości liter i znaków specjalnych ("TO-220" == "to220").
 */
class LogikaObudow {
public:
    explicit LogikaObudow(ManagerBazyDanych& manager) : manager_(manager) {}

    std::string pobierzBlad() const { return blad_; }

    /// Klucz porównania nazw: małe litery i cyfry, bez reszty znaków
    static std::string klucz(const std::string& nazwa);

    bool pobierz(const std::string& tekst, std::vector<Obudowa>& wynik);
    std::optional<Obudowa> pobierzObudowe(long long id);
    /// Obudowa o tej nazwie (porównanie po kluczu)
    std::optional<Obudowa> znajdz(const std::string& nazwa);
    std::vector<std::string> nazwy();
    /// Obudowy z tekstu pola "Obudowa" elementu ("R7.5, R10"), te które są w słowniku
    std::vector<Obudowa> zTekstu(const std::string& tekst);
    /// Tekst pola "Obudowa" z listy nazw: kanoniczny, rozdzielany przecinkami
    static std::string zlozTekst(const std::vector<std::string>& nazwy);
    /// Rozbija tekst pola "Obudowa" na nazwy (przecinek lub średnik)
    static std::vector<std::string> rozbijTekst(const std::string& tekst);

    /// Typowe elementy w obudowie (z zestawów bibliotek Eagle), posortowane po symbolu
    std::vector<TypowyElement> typowe(long long idObudowy);
    /// Podpowiedź obudów dla symbolu elementu: najpierw typowe elementy z bibliotek Eagle (symbol równy albo zaczynający się
    /// od symbolu zestawu, np. "BD437G" -> BD437), potem obudowa z katalogu o tym samym symbolu.
    std::vector<Obudowa> podpowiedzi(const std::string& symbol, size_t maksymalnie = 8);
    /// Przekłada nazwę footprintu z BOM ("Resistor_SMD:R_0805_2012Metric", "SOT-23", "0805") na obudowę ze słownika.
    /// Zwraca tylko wtedy, gdy wynik jest jednoznaczny.
    std::optional<Obudowa> rozwiazStopke(const std::string& stopka);

    std::optional<long long> zapisz(const Obudowa& obudowa);
    bool usun(long long id);

    /// Dodaje obudowy dla tekstów z pola "Obudowa" elementów i pozycji katalogu, których jeszcze nie ma w słowniku
    int uzupelnijZElementow();
    /// Dodaje obudowy (każdy pakiet Eagle to osobna obudowa) z biblioteki Eagle; @p nadpisz zastępuje także wypełnione atrybuty
    bool importuj(const ImportLBR::Biblioteka& biblioteka, bool nadpisz, WynikImportuObudow& wynik);

private:
    ManagerBazyDanych& manager_;
    std::string blad_;
    PolaczenieBazy* polaczenie();
    bool ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem = "");
};

#endif // LOGIKAOBUDOW_H
