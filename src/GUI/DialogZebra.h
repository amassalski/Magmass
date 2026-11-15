#ifndef DIALOGZEBRA_H
#define DIALOGZEBRA_H

#include "../Narzedzia/UstawieniaZebry.h"

#include <wx/wx.h>
#include <wx/spinctrl.h>

/// Ustawienia drukarki Zebra (ZPL): sposób połączenia, rozmiar etykiety, rozdzielczość; etykieta próbna
class DialogZebra : public wxDialog {
public:
    DialogZebra(wxWindow* parent, const UstawieniaZebry& ustawienia);

    UstawieniaZebry Ustawienia() const;
    void UstawTryb(WysylkaZPL::Tryb tryb);

    // dla testów
    wxTextCtrl* PoleHosta() { return tcHost_; }
    wxChoice* WyborDpi() { return chDpi_; }
    wxString Stan() const { return stan_->GetLabel(); }
    /// Wysyła etykietę próbną; zwraca powodzenie (komunikat w Stan())
    bool WyslijProbna();

private:
    UstawieniaZebry baza_;
    wxRadioButton* rbSiec_ = nullptr;
    wxRadioButton* rbSystem_ = nullptr;
    wxRadioButton* rbPlik_ = nullptr;
    wxTextCtrl* tcHost_ = nullptr;
    wxSpinCtrl* scPort_ = nullptr;
    wxComboBox* cbDrukarka_ = nullptr;
    wxTextCtrl* tcPlik_ = nullptr;
    wxSpinCtrl* scSzer_ = nullptr;
    wxSpinCtrl* scWys_ = nullptr;
    wxChoice* chDpi_ = nullptr;
    wxCheckBox* cbBezPolskich_ = nullptr;
    wxCheckBox* cbObrot_ = nullptr;
    wxStaticText* stan_ = nullptr;

    void OdswiezAktywne();
    void WybierzPlik();
};

#endif  // DIALOGZEBRA_H
