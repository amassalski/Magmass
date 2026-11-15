#ifndef LOGIKAKSIAZEK_H
#define LOGIKAKSIAZEK_H

#include "../BazaDanych/ManagerBazyDanych.h"
#include "../Model/Ksiazka.h"
#include "../Model/Narzedzie.h"

#include <optional>
#include <string>
#include <vector>

struct WynikImportuKsiazek {
    int dodano = 0;
    int pominieto = 0;                    ///< książki, które już są (ta sama ścieżka albo tytuł i autor)
    std::vector<std::string> bledy;
};

/// Wspólna zmiana wybranych pól wielu książek (nullopt = pole bez zmian)
struct ZmianyZbiorczeKsiazek {
    std::optional<std::string> autor;
    std::optional<std::string> wydawca;
    std::optional<std::string> tagi;
    bool tagiDopisz = true;          ///< true: dopisz do istniejących tagów, false: zastąp
    bool pusta() const { return !autor && !wydawca && !tagi; }
};

/**
 * @class LogikaKsiazek
 * @brief Biblioteczka: książki (papierowe i elektroniczne), import z CSV i z katalogu na dysku,
 *        indeks pełnotekstowy stron PDF w bazie i wyszukiwanie w treści.
 */
class LogikaKsiazek {
public:
    explicit LogikaKsiazek(ManagerBazyDanych& manager) : manager_(manager) {}
    std::string pobierzBlad() const { return blad_; }

    /// Książki pasujące do tekstu (tytuł, autor, wydawca, ISBN, tagi, uwagi; bez rozróżniania wielkości liter)
    bool pobierz(const std::string& tekst, std::vector<Ksiazka>& wynik);
    std::optional<Ksiazka> pobierzKsiazke(long long id);
    std::optional<long long> zapisz(const Ksiazka& k);
    bool usun(long long id);
    /// Stosuje @p zmiany do wszystkich książek z @p idki; @p zmieniono = liczba zapisanych, @p bledy = liczba błędów
    bool zmienZbiorczo(const std::vector<long long>& idki, const ZmianyZbiorczeKsiazek& zmiany, int* zmieniono = nullptr, int* bledy = nullptr);

    /// Pożycza książkę (papierową albo plik); @p od puste = dziś, @p termin puste = bez terminu. Błąd, gdy jest już pożyczona.
    bool pozycz(long long idKsiazki, const std::string& osoba, const std::string& od, const std::string& termin, const std::string& uwagi);
    /// Zwrot otwartej pożyczki; @p data pusta = dziś
    bool zwroc(long long idKsiazki, const std::string& data = "");
    /// Historia pożyczek książki (najnowsze pierwsze); @p idKsiazki 0 = wszystkich książek
    bool historia(long long idKsiazki, std::vector<Pozyczka>& wynik);
    /// Otwarte pożyczki po terminie
    bool przeterminowane(std::vector<Pozyczka>& wynik);

    /// Dodaje książki (CSV, skan katalogu); istniejące (ta sama ścieżka albo tytuł+autor) pomija
    bool importuj(const std::vector<Ksiazka>& ksiazki, WynikImportuKsiazek& wynik);

    /// Zapisuje tekst stron (strony[0] = strona 1) jako indeks książki; zastępuje poprzedni. Puste strony pomija.
    bool zapiszIndeks(long long id, const std::vector<std::string>& strony);
    bool usunIndeks(long long id);

    /// Wyszukuje w treści zaindeksowanych książek (wszystkie słowa na jednej stronie); @p idKsiazki 0 = wszystkie
    bool szukajWTresci(const std::string& tekst, long long idKsiazki, std::vector<TrafienieKsiazki>& wynik, int limit = 200);

    /// Tytuł z nazwy pliku: bez rozszerzenia, "_" i "." zamienione na spacje
    /// Rok wydania z początku nazwy pliku ("1964 ABC radioamatora.pdf" -> 1964); 0, gdy nazwa nie zaczyna się rokiem
    static int rokZNazwyPliku(const std::string& nazwa);
    static std::string tytulZNazwyPliku(const std::string& nazwaPliku);

private:
    ManagerBazyDanych& manager_;
    std::string blad_;
    PolaczenieBazy* polaczenie();
    bool ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem = "");
};

#endif // LOGIKAKSIAZEK_H
