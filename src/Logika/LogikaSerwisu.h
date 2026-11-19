#ifndef LOGIKASERWISU_H
#define LOGIKASERWISU_H

#include "../BazaDanych/ManagerBazyDanych.h"
#include "../Model/Serwis.h"

#include <optional>
#include <string>
#include <vector>

struct FiltrSerwisu {
    std::string tekst;       ///< szuka w urządzeniu, producencie, modelu, numerze seryjnym (także części), lokalizacji, zgłoszeniu, techniku, opisach i częściach
    std::string rodzaj;      ///< pusty = wszystkie
    std::string status;      ///< pusty = wszystkie
    std::string od, doDaty;  ///< zakres dat serwisu (włącznie), puste = bez ograniczenia
};

/**
 * @class LogikaSerwisu
 * @brief Ewidencja serwisowa: co, gdzie i kiedy serwisowano, co wymieniono, kto to zrobił, ze zdjęciami.
 */
class LogikaSerwisu {
public:
    explicit LogikaSerwisu(ManagerBazyDanych& manager) : manager_(manager) {}
    std::string pobierzBlad() const { return blad_; }

    /// Dozwolone wartości pól rodzaj i status
    static const std::vector<std::string>& rodzaje();
    static const std::vector<std::string>& statusy();

    /// Lista wpisów (najnowsze pierwsze) z liczbą części i zdjęć; bez list szczegółowych
    bool pobierz(const FiltrSerwisu& filtr, std::vector<WpisSerwisowy>& wynik);
    /// Pełny wpis z częściami i zdjęciami
    std::optional<WpisSerwisowy> pobierzWpis(long long id);
    /// Zapisuje wpis razem z listą części (w jednej transakcji); zdjęcia przez dodajZdjecie/usunZdjecie. Zwraca id.
    /// Części z idElementu i pobierz=true (jeszcze nie pobrane) zdejmują ilość ze stanu magazynu (WYDANIE); brak towaru
    /// przerywa cały zapis. @p zwrocUsuniete: części już pobrane, których nie ma na nowej liście, wracają do magazynu.
    std::optional<long long> zapisz(const WpisSerwisowy& w, bool zwrocUsuniete = false);
    /// Usuwa wpis; @p zwroc: części pobrane z magazynu wracają na stan
    bool usun(long long id, bool zwroc = false);
    /// Części wpisu pobrane z magazynu (do pytania o zwrot przy usuwaniu)
    std::vector<CzescSerwisowa> czesciPobrane(long long idWpisu);
    bool dodajZdjecie(long long idWpisu, const std::string& sciezka, const std::string& opis = "");
    bool usunZdjecie(long long idZdjecia);
    /// Wszystkie wpisy dla urządzenia o danym numerze seryjnym lub nazwie (historia urządzenia), najnowsze pierwsze
    bool historiaUrzadzenia(const std::string& numerSeryjny, const std::string& urzadzenie, std::vector<WpisSerwisowy>& wynik);

    /// Podpowiedzi do formularza
    std::vector<std::string> urzadzenia();
    std::vector<std::string> lokalizacje();
    std::vector<std::string> technicy();

    /// Eksport do CSV (średnik, UTF-8 z BOM-em tworzy wywołujący): jedna linia na część, a wpis bez części ma jedną linię
    static std::string naCSV(const std::vector<WpisSerwisowy>& wpisy);

private:
    ManagerBazyDanych& manager_;
    std::string blad_;
    PolaczenieBazy* polaczenie();
    bool ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem = "");
    std::vector<std::string> unikalne(const char* kolumna, const char* tabela = "tbl_serwis");
};

#endif // LOGIKASERWISU_H
