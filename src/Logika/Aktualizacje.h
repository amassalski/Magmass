#ifndef AKTUALIZACJE_H
#define AKTUALIZACJE_H

#include <optional>
#include <string>
#include <vector>

/**
 * Sprawdzanie, czy jest nowsza wersja programu. Program pobiera mały plik tekstowy z repozytorium na GitHubie
 * (domyślnie wersja.txt z gałęzi głównej repozytorium) i porównuje numer z własną wersją. Nic nie instaluje.
 *
 * Format pliku wersja.txt:
 *   1.0.1                                   <- pierwsza linia: numer wersji (może być z "v": v1.0.1)
 *   https://github.com/.../releases         <- opcjonalnie: linia zaczynająca się od http - strona pobierania
 *   Co nowego: kilka zdań...                <- pozostałe linie: opis zmian
 */
namespace Aktualizacje {

struct Informacja {
    std::string wersja;           ///< np. "1.0.1"
    std::string adresPobierania;  ///< pusty = użyj adresu domyślnego
    std::string opis;             ///< co nowego (może być pusty)
};

/// Domyślne repozytorium projektu na GitHubie
std::string adresRepozytorium();
/// Adres pliku wersja.txt w głównym katalogu repozytorium (gałąź domyślna, więc nie zależy od jej nazwy).
/// Przyjmuje adres w dowolnej postaci: https://github.com/wlasciciel/repo, z ".git" na końcu, z "/" lub z "/tree/...".
/// Zwraca pusty tekst, gdy to nie jest adres repozytorium na github.com.
std::string adresWersji(const std::string& repozytorium);
/// To samo dla repozytorium domyślnego
std::string adresWersji();
/// Strona, na którą kierujemy użytkownika, gdy plik wersji nie podaje własnej (adres repozytorium bez ".git")
std::string adresStronyPobierania(const std::string& repozytorium);
std::string adresStronyPobierania();

/// Wersja jako liczby: "1.2.10" -> {1,2,10}; nullopt, gdy tekst nie jest numerem wersji
std::optional<std::vector<int>> parsujNumer(const std::string& tekst);
/// -1, 0, 1 (brakujące człony traktowane jak zera: 1.0 == 1.0.0)
int porownaj(const std::vector<int>& a, const std::vector<int>& b);
/// Czy @p dostepna jest nowsza od @p biezaca (false, gdy któryś numer jest nieczytelny)
bool nowsza(const std::string& dostepna, const std::string& biezaca);
/// Czyta treść pliku wersja.txt (z BOM, CRLF, "v" przed numerem); nullopt, gdy pierwsza linia nie jest numerem
std::optional<Informacja> parsujPlikWersji(const std::string& tresc);

} // namespace Aktualizacje

#endif // AKTUALIZACJE_H
