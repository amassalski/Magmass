#ifndef DIALOGPOZYCZKI_H
#define DIALOGPOZYCZKI_H

#include <wx/wx.h>
#include <wx/combobox.h>
#include <wx/datectrl.h>

#include <string>
#include <vector>

/// Okno pożyczki (komu, od kiedy, termin zwrotu, uwagi); wspólne dla narzędzi i książek
class DialogPozyczki : public wxDialog {
public:
    /// @p tytul tytuł okna, @p komunikatBraku tekst ostrzeżenia, gdy nie podano osoby, @p przedmiot nazwa pożyczanej rzeczy
    DialogPozyczki(wxWindow* parent, const wxString& tytul, const wxString& komunikatBraku, const wxString& przedmiot,
                   const std::vector<std::string>& osoby);

    std::string Osoba() const;
    std::string Od() const;
    std::string Termin() const;
    std::string Uwagi() const;

private:
    wxComboBox* cbOsoba_;
    wxDatePickerCtrl *dpOd_, *dpTermin_;
    wxTextCtrl* tcUwagi_;
    wxString komunikatBraku_;
};

#endif // DIALOGPOZYCZKI_H
