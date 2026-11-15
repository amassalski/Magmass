#ifndef LOGIKANARZEDZI_H
#define LOGIKANARZEDZI_H

#include "../BazaDanych/ManagerBazyDanych.h"
#include "../Model/Narzedzie.h"

#include <optional>
#include <string>
#include <vector>

struct FiltrNarzedzi {
    std::string tekst;
    bool tylkoPozyczone = false;
    bool tylkoPrzeterminowane = false;
};

struct WynikImportuNarzedzi {
    int dodano = 0;
    int pominieto = 0;     ///< to samo narzędzie (nazwa, model i numer seryjny) już jest
    std::vector<std::string> bledy;
};

/**
 * @class LogikaNarzedzi
 * @brief Baza narzędzi użytkownika i rejestr pożyczek (komu, od kiedy, do kiedy, zwrot).
 */
class LogikaNarzedzi {
public:
    explicit LogikaNarzedzi(ManagerBazyDanych& manager) : manager_(manager) {}
    std::string pobierzBlad() const { return blad_; }

    bool pobierz(const FiltrNarzedzi& filtr, std::vector<Narzedzie>& wynik);
    std::optional<Narzedzie> pobierzNarzedzie(long long id);
    std::optional<long long> zapisz(const Narzedzie& n);
    bool usun(long long id);
    bool importuj(const std::vector<Narzedzie>& narzedzia, WynikImportuNarzedzi& wynik);
    std::vector<std::string> kategorie();

    /// Pożycza narzędzie; @p od puste = dziś, @p termin puste = bez terminu. Błąd, gdy jest już pożyczone.
    bool pozycz(long long idNarzedzia, const std::string& osoba, const std::string& od, const std::string& termin, const std::string& uwagi);
    /// Zwrot otwartej pożyczki; @p data pusta = dziś
    bool zwroc(long long idNarzedzia, const std::string& data = "");
    /// Historia pożyczek narzędzia (najnowsze pierwsze); @p idNarzedzia 0 = wszystkich narzędzi
    bool historia(long long idNarzedzia, std::vector<Pozyczka>& wynik);
    /// Otwarte pożyczki po terminie
    bool przeterminowane(std::vector<Pozyczka>& wynik);
    /// Osoby, którym już pożyczano (do podpowiedzi)
    std::vector<std::string> osoby();

    /// Poprawna data "RRRR-MM-DD"
    static bool czyData(const std::string& s);

private:
    ManagerBazyDanych& manager_;
    std::string blad_;
    PolaczenieBazy* polaczenie();
    bool ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem = "");
};

#endif // LOGIKANARZEDZI_H
