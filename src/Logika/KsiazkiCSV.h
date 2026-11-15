#ifndef KSIAZKICSV_H
#define KSIAZKICSV_H

#include "../Model/Ksiazka.h"

#include <string>
#include <vector>

/**
 * Książki z/do CSV. Kolumny: Tytuł (wymagana), Autor, Wydawca, Rok, ISBN, Język, Tagi, Ścieżka (plik PDF/EPUB), Uwagi.
 * Rozpoznawane są też nazwy angielskie (Title, Author, Publisher, Year, Language, Path, Notes).
 */
namespace KsiazkiCSV {

struct Wynik {
    std::vector<Ksiazka> ksiazki;
    std::vector<std::string> ostrzezenia;   ///< np. wiersze bez tytułu
    std::string blad;                       ///< niepusty = pliku nie da się wczytać
};

Wynik parsuj(const std::string& tekst);
std::string naCSV(const std::vector<Ksiazka>& ksiazki);
std::string szablon();

} // namespace KsiazkiCSV

#endif // KSIAZKICSV_H
