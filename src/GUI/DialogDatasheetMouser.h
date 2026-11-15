#ifndef DIALOGDATASHEETMOUSER_H
#define DIALOGDATASHEETMOUSER_H

#include "../Wyszukiwanie/Mouser.h"

#include <wx/wx.h>
#include <wx/listctrl.h>

#include <memory>
#include <vector>

/**
 * @class DialogDatasheetMouser
 * @brief Wyszukiwanie części w Mouser Search API: pole z symbolem, lista trafień (numer producenta, producent, opis, obudowa, datasheet)
 *        i wybór pozycji, której datasheet ma zostać pobrany. Zamyka się kodem wxID_OK (wybrano), wxID_EDIT (użytkownik chce wpisać adres ręcznie)
 *        albo wxID_CANCEL.
 */
class DialogDatasheetMouser : public wxDialog {
public:
    DialogDatasheetMouser(wxWindow* parent, const wxString& fraza, const std::string& klucz);
    ~DialogDatasheetMouser() override;

    /// Wysyła zapytanie do Mousera (asynchronicznie); wynik trafia do ZastosujOdpowiedz
    void Szukaj();
    /// Rozbiera odpowiedź JSON i wypełnia listę. Zwraca liczbę trafień (-1 = błąd, komunikat jest pokazany w oknie).
    int ZastosujOdpowiedz(const std::string& json);

    int LiczbaWynikow() const { return static_cast<int>(czesci_.size()); }
    const Mouser::Czesc* Wybrana() const;
    bool Wybierz(int indeks);
    wxString Stan() const { return stan_->GetLabel(); }
    wxTextCtrl* PoleSzukania() const { return fraza_; }

private:
    std::string klucz_;
    std::vector<Mouser::Czesc> czesci_;
    wxTextCtrl* fraza_ = nullptr;
    wxButton* szukaj_ = nullptr;
    wxListCtrl* lista_ = nullptr;
    wxStaticText* stan_ = nullptr;
    wxButton* ok_ = nullptr;
    std::shared_ptr<bool> zyje_ = std::make_shared<bool>(true);

    void PokazBlad(const wxString& tekst);
};

#endif // DIALOGDATASHEETMOUSER_H
