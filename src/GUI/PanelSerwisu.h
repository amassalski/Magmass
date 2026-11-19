#ifndef PANELSERWISU_H
#define PANELSERWISU_H

#include "../Logika/LogikaMagazynu.h"
#include "../Logika/LogikaSerwisu.h"

#include <wx/wx.h>
#include <wx/choice.h>
#include "PoleDaty.h"
#include <wx/listctrl.h>
#include <wx/scrolwin.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>

#include <functional>
#include <vector>

/**
 * @class PanelSerwisu
 * @brief Zakładka "Serwis": ewidencja serwisowa (co, gdzie i kiedy serwisowano, co wymieniono, kto to zrobił)
 *        ze zdjęciami (także z kamery), filtrami, historią urządzenia i eksportem do CSV.
 */
class PanelSerwisu : public wxPanel {
public:
    PanelSerwisu(wxWindow* parent, LogikaSerwisu& logika, LogikaMagazynu& magazyn);

    void Odswiez();
    void Wyczysc();

    void Nowy();
    void Edytuj();
    void Usun(bool bezPytan = false);
    /// Nowy wpis z danymi urządzenia (i lokalizacji, technika, rodzaju) zaznaczonego wpisu - przy powtarzanych wizytach
    void Duplikuj();
    void DodajZdjecieZKamery();
    void DodajZdjecieZPliku();
    /// Dołącza do zaznaczonego wpisu zdjęcie z podanego pliku; false przy błędzie
    bool DolaczZdjecie(const wxString& sciezka, const wxString& opis = wxEmptyString);
    void UsunZdjecie(size_t numer, bool bezPytan = false);
    /// Pokazuje wszystkie wpisy tego samego urządzenia (po numerze seryjnym, a bez niego po nazwie)
    void HistoriaUrzadzenia();
    bool EksportujCSV(const wxString& sciezka = wxEmptyString);

    void Szukaj(const std::string& tekst);
    void UstawFiltr(const std::string& rodzaj, const std::string& status);
    const WpisSerwisowy* Wybrany() const;
    void Wybierz(long long id);
    const std::vector<WpisSerwisowy>& Lista() const { return lista_; }
    /// Pełne dane zaznaczonego wpisu (części, zdjęcia); puste, gdy nic nie zaznaczono
    const std::optional<WpisSerwisowy>& Szczegoly() const { return szczegoly_; }
    int LiczbaMiniatur() const { return liczbaMiniatur_; }

    std::function<void(const wxString&)> naStatus;
    std::function<void(const wxString&)> naKomunikat;

private:
    enum { ID_S_NOWY = wxID_HIGHEST + 900, ID_S_EDYTUJ, ID_S_USUN, ID_S_DUPLIKUJ, ID_S_KAMERA, ID_S_PLIK, ID_S_HISTORIA, ID_S_EKSPORT, ID_S_USUN_ZDJECIE };
    enum Kol { K_DATA, K_URZADZENIE, K_LOKALIZACJA, K_RODZAJ, K_STATUS, K_TECHNIK, K_CZESCI, K_ZDJECIA, K_LICZBA };

    LogikaSerwisu& logika_;
    LogikaMagazynu& magazyn_;
    std::vector<WpisSerwisowy> lista_;
    std::optional<WpisSerwisowy> szczegoly_;
    wxSearchCtrl* szukaj_ = nullptr;
    wxChoice *chRodzaj_ = nullptr, *chStatus_ = nullptr;
    PoleDatyTyp *dpOd_ = nullptr, *dpDo_ = nullptr;
    wxStaticText* licznik_ = nullptr;
    wxListCtrl* widok_ = nullptr;
    wxListCtrl* czesciWidok_ = nullptr;
    wxStaticText* naglowek_ = nullptr;
    wxStaticText* opis_ = nullptr;
    wxScrolledWindow* zdjeciaPanel_ = nullptr;
    int liczbaMiniatur_ = 0;
    bool odswiezanie_ = false;

    void PokazSzczegoly();
    void MenuKontekstowe(long element);
    void MenuZdjecia(size_t numer);
    void Komunikat(const wxString& t) const { if (naKomunikat) naKomunikat(t); }
    void Status(const wxString& t) const { if (naStatus) naStatus(t); }
    void Blad(const wxString& t);
    bool ZapiszZeZdjeciami(const WpisSerwisowy& w, const std::vector<ZdjecieSerwisu>& zdjecia, const std::vector<long long>& usuniete, long long& id, bool zwroc = false);
    FiltrSerwisu Filtr() const;
};

#endif // PANELSERWISU_H
