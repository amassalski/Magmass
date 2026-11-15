#ifndef NARZEDZIACSV_H
#define NARZEDZIACSV_H

#include "../Model/Narzedzie.h"

#include <string>
#include <vector>

/// Narzędzia z/do CSV. Kolumny: Nazwa (wymagana), Producent, Model, Kategoria, Numer seryjny, Miejsce, Data zakupu (RRRR-MM-DD),
/// Cena, Instrukcja, Uwagi. Rozpoznawane są też nazwy angielskie.
namespace NarzedziaCSV {

struct Wynik {
    std::vector<Narzedzie> narzedzia;
    std::vector<std::string> ostrzezenia;
    std::string blad;
};

Wynik parsuj(const std::string& tekst);
std::string naCSV(const std::vector<Narzedzie>& narzedzia);
std::string szablon();

} // namespace NarzedziaCSV

#endif // NARZEDZIACSV_H
