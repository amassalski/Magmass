#ifndef MOUSER_H
#define MOUSER_H

#include <string>
#include <utility>
#include <vector>

/**
 * Wyszukiwanie części w Mouser Search API (https://www.mouser.com/api-search/): zapytanie po słowie kluczowym lub numerze części,
 * odpowiedź z adresem datasheetu, producentem, opisem i atrybutami. Bez wxWidgets: sam zapytanie i rozbiór odpowiedzi;
 * wysyła je KlientHTTP::wyslijJSON. Klucz API pochodzi z ustawień użytkownika (config.ini) i nigdy nie trafia do repozytorium.
 */
namespace Mouser {

struct Czesc {
    std::string numerMouser;        ///< np. "595-LM358DR"
    std::string numerProducenta;    ///< MPN, np. "LM358DR"
    std::string producent;
    std::string opis;
    std::string kategoria;
    std::string adresDatasheetu;    ///< pusty, gdy brak
    std::string adresProduktu;
    std::string cyklZycia;          ///< np. "Active"
    std::vector<std::pair<std::string, std::string>> atrybuty;   ///< ProductAttributes: nazwa, wartość

    /// Obudowa z atrybutów ("Package / Case", po polsku "Obudowa"); pusta, gdy brak. Atrybut "Packaging" (taśma, tuba) jest pomijany.
    std::string obudowa() const;
};

/// Adres zapytania: https://api.mouser.com/api/v1/search/keyword?apiKey=...
std::string adresZapytania(const std::string& klucz);
/// Treść JSON zapytania o @p fraza (symbol, numer części albo słowa), do @p limit wyników (Mouser: maks. 50)
std::string cialoZapytania(const std::string& fraza, int limit = 20);
/// Rozbiór odpowiedzi. false = błąd (komunikat po polsku w @p blad), także gdy API zwróciło tablicę Errors.
bool zOdpowiedzi(const std::string& json, std::vector<Czesc>& czesci, std::string& blad);
/// Adres datasheetu z odpowiedzi doprowadzony do postaci https://... ("//host/..." -> "https://host/...", spacje -> %20); pusty, gdy nie da się użyć
std::string poprawAdres(std::string adres);
/// Czy klucz wygląda na prawidłowy (UUID: 8-4-4-4-12 znaków szesnastkowych), żeby nie wysyłać oczywistych pomyłek
bool czyKluczPoprawny(const std::string& klucz);

} // namespace Mouser

#endif // MOUSER_H
