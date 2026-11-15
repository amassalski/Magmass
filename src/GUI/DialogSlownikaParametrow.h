#ifndef DIALOGSLOWNIKAPARAMETROW_H
#define DIALOGSLOWNIKAPARAMETROW_H

#include "../Logika/LogikaSlownikaParametrow.h"

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/srchctrl.h>
#include <wx/grid.h>

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

/// Po odczycie datasheetu: symbole parametrów, których nie ma w słowniku, z miejscem na opis. Zaznaczone wiersze z opisem
/// trafiają do słownika (przycisk OK); puste opisy są pomijane.
class DialogDoSlownika : public wxDialog {
public:
    DialogDoSlownika(wxWindow* parent, const std::vector<std::string>& symbole);
    /// Wpisy do zapisania: zaznaczone i z niepustym opisem
    std::vector<WpisSlownikaParametrow> Wpisy() const;
    /// Dla testów
    void UstawOpis(int wiersz, const wxString& opis, bool zaznacz = true);
    int LiczbaWierszy() const { return siatka_->GetNumberRows(); }

private:
    wxGrid* siatka_ = nullptr;
};

#endif // DIALOGSLOWNIKAPARAMETROW_H
