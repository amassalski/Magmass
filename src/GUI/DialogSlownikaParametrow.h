#ifndef DIALOGSLOWNIKAPARAMETROW_H
#define DIALOGSLOWNIKAPARAMETROW_H

#include "../Logika/LogikaSlownikaParametrow.h"

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/srchctrl.h>

#include <vector>

/// Słownik opisów parametrów katalogu (symbol -> opis pokazywany w dymku), zapisany w bazie danych
class DialogSlownikaParametrow : public wxDialog {
public:
    DialogSlownikaParametrow(wxWindow* parent, LogikaSlownikaParametrow& logika);

    void Odswiez();
    const std::vector<WpisSlownikaParametrow>& Wpisy() const { return wpisy_; }
    /// Operacje z przycisków (publiczne dla testów); zwracają, czy coś zmieniły
    bool Dodaj();
    bool Edytuj();
    bool Usun();
    bool PrzywrocBrakujace();
    void Szukaj(const wxString& tekst);
    void Wybierz(long long id);

private:
    LogikaSlownikaParametrow& logika_;
    std::vector<WpisSlownikaParametrow> wpisy_;
    wxSearchCtrl* szukaj_ = nullptr;
    wxListCtrl* lista_ = nullptr;
    wxStaticText* licznik_ = nullptr;

    const WpisSlownikaParametrow* Wybrany() const;
    std::vector<long long> Zaznaczone() const;
    bool Edycja(WpisSlownikaParametrow wpis);
};

#endif // DIALOGSLOWNIKAPARAMETROW_H
