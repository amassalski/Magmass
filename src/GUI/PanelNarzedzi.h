#ifndef PANELNARZEDZI_H
#define PANELNARZEDZI_H

#include "../Logika/LogikaNarzedzi.h"

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>

#include <functional>
#include <vector>

/**
 * @class PanelNarzedzi
 * @brief Zakładka "Narzędzia": baza narzędzi użytkownika i rejestr pożyczek (komu, od kiedy, do kiedy, zwrot)
 *        z historią, filtrem pożyczonych i podświetleniem przeterminowanych.
 */
class PanelNarzedzi : public wxPanel {
public:
    PanelNarzedzi(wxWindow* parent, LogikaNarzedzi& logika);

    void Odswiez();
    void Wyczysc();

    void Nowe();
    void Edytuj();
    void Usun();
    void Pozycz();
    /// Rejestruje zwrot zaznaczonego narzędzia (@p bezPytan: bez potwierdzenia, testy)
    bool Zwroc(bool bezPytan = false);
    int ImportujCSV(const wxString& sciezka = wxEmptyString);
    bool EksportujCSV(const wxString& sciezka = wxEmptyString);
    bool ZapiszSzablon(const wxString& sciezka = wxEmptyString);
    void OtworzInstrukcje();
    /// Ustawia zdjęcie zaznaczonego narzędzia z podanego pliku (zapis w bazie)
    bool DolaczZdjecie(const wxString& sciezka);
    void DolaczZdjecieZKamery();
    void DolaczZdjecieZPliku();
    /// Wpisuje do dziennika przeterminowane pożyczki; zwraca ich liczbę (-1 = błąd)
    int Przypomnij();

    void Szukaj(const std::string& tekst);
    const Narzedzie* Wybrane() const;
    bool MaWybor() const { return Wybrane() != nullptr; }
    void Wybierz(long long id);
    void UstawFiltr(bool tylkoPozyczone, bool tylkoPrzeterminowane);
    const std::vector<Narzedzie>& Lista() const { return lista_; }
    const std::vector<Pozyczka>& Historia() const { return historia_; }
    /// Czy w panelu widać zdjęcie zaznaczonego narzędzia (do testów)
    bool ZdjeciePokazane() const { return zdjeciePokazane_; }

    std::function<void(const wxString&)> naStatus;
    std::function<void(const wxString&)> naKomunikat;

private:
    enum { ID_N_NOWE = wxID_HIGHEST + 800, ID_N_EDYTUJ, ID_N_USUN, ID_N_POZYCZ, ID_N_ZWROC, ID_N_INSTRUKCJA, ID_N_IMPORT, ID_N_EKSPORT, ID_N_SZABLON, ID_N_KAMERA, ID_N_PLIK };
    enum Kol { K_NAZWA, K_MODEL, K_KATEGORIA, K_MIEJSCE, K_STATUS, K_LICZBA };

    LogikaNarzedzi& logika_;
    std::vector<Narzedzie> lista_;
    std::vector<Pozyczka> historia_;
    wxSearchCtrl* szukaj_ = nullptr;
    wxCheckBox* cbPozyczone_ = nullptr;
    wxCheckBox* cbPrzeterminowane_ = nullptr;
    wxListCtrl* widok_ = nullptr;
    wxListCtrl* historiaWidok_ = nullptr;
    wxStaticText* naglowek_ = nullptr;
    wxStaticBitmap* zdjecie_ = nullptr;
    bool zdjeciePokazane_ = false;
    wxStaticText* szczegoly_ = nullptr;
    wxStaticText* licznik_ = nullptr;
    bool odswiezanie_ = false;

    void PokazSzczegoly();
    void MenuKontekstowe(long element);
    void PokazZdjecie(const Narzedzie* n);
    void Komunikat(const wxString& t) const { if (naKomunikat) naKomunikat(t); }
    void Status(const wxString& t) const { if (naStatus) naStatus(t); }
    void Blad(const wxString& t);
};

#endif // PANELNARZEDZI_H
