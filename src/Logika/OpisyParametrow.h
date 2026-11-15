#ifndef OPISYPARAMETROW_H
#define OPISYPARAMETROW_H

#include "../Model/Katalog.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

/// Słownik opisów typowych symboli parametrów (lampy, półprzewodniki), np. "W_amax" = maksymalna moc strat anody.
/// Służy jako domyślny opis w dymku, gdy użytkownik nie wpisał własnego. Symbol bez znaczenia dla wielkości liter,
/// spacji, podkreśleń i nawiasów: "V_CE(sat)", "VCE sat" i "v_{ce(sat)}" to ten sam klucz.
namespace OpisyParametrow {

/// Opis ze słownika; pusty, gdy symbolu nie znamy
std::string zeSlownika(const std::string& nazwa);
/// Własny opis parametru, a gdy go nie ma, opis ze słownika (może być pusty)
std::string dlaParametru(const ParametrKatalogu& p);
/// Wbudowany zestaw (symbol, opis): zasiew tabeli słownika w bazie i przycisk "Przywróć brakujące"
const std::vector<std::pair<std::string, std::string>>& wbudowane();
/// Słownik z bazy (symbol -> opis); po jego ustawieniu zeSlownika() korzysta tylko z niego
void ustawZBazy(const std::map<std::string, std::string>& opisy);
void wyczyscZBazy();
/// Symbole z listy, których nie ma w słowniku (bez powtórzeń wg klucza, w kolejności pierwszego wystąpienia)
std::vector<std::string> nieznane(const std::vector<std::string>& nazwy);
/// Klucz porównawczy symbolu (do testów)
std::string klucz(const std::string& nazwa);

} // namespace OpisyParametrow

#endif // OPISYPARAMETROW_H
