#ifndef POLEDATY_H
#define POLEDATY_H

#include <wx/datectrl.h>   // wxDP_ALLOWNONE, wxDP_DROPDOWN
#include <wx/dateevt.h>    // wxEVT_DATE_CHANGED
#include <wx/datetime.h>
#include <wx/panel.h>

class wxButton;
class wxTextCtrl;

/// Pole daty Magmass: pole tekstowe (RRRR-MM-DD albo DD.MM.RRRR) z przyciskiem otwierającym kalendarz.
/// Takie samo na Windows, macOS i Linuksie. Natywne pola dat nie przyjmują ciemnego motywu (zostają białe),
/// a kalendarz wxWidgets rysujemy tu w kolorach motywu systemu.
///
/// Zachowuje się jak wxDatePickerCtrl: GetValue/SetValue, styl wxDP_ALLOWNONE (pusta data = brak daty)
/// i zdarzenie wxEVT_DATE_CHANGED (wxDateEvent).
class PoleDaty : public wxPanel {
public:
    PoleDaty(wxWindow* rodzic, const wxDateTime& data, long styl);

    wxDateTime GetValue() const { return data_; }
    /// Ustawia datę bez zdarzenia; nieprawidłowa data = brak (tylko przy wxDP_ALLOWNONE, inaczej dziś).
    void SetValue(const wxDateTime& data);

    /// Do testów: to samo co wybranie daty w kalendarzu (wywołuje zdarzenie).
    void Wybierz(const wxDateTime& data);

private:
    void PokazKalendarz();
    void PrzeczytajTekst();
    void Odswiez();
    void Powiadom();

    wxTextCtrl* tekst_ = nullptr;
    wxButton* przycisk_ = nullptr;
    wxDateTime data_;
    bool dozwolonyBrak_ = false;
};

using PoleDatyTyp = PoleDaty;

inline PoleDatyTyp* NowePoleDaty(wxWindow* rodzic, const wxDateTime& data, long styl) {
    return new PoleDaty(rodzic, data, styl);
}

#endif
