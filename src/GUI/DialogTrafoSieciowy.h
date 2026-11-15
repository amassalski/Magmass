#ifndef DIALOGTRAFOSIECIOWY_H
#define DIALOGTRAFOSIECIOWY_H

#include "DialogDlawika.h"

/// Transformator sieciowy małej mocy (metoda Krize'a), formularz w układzie jak dławik
class DialogTrafoSieciowy : public wxDialog {
public:
    explicit DialogTrafoSieciowy(wxWindow* parent);

    // --- dostęp dla testów
    void UstawKsztaltke(const wxString& nazwa);
    void UstawPole(const wxString& nazwa, const wxString& wartosc);   ///< "A","B","a","b","c","d","h","U1","B_Gs","eta","j","km","kFe","beta","gamma","scianka","margines"
    void UstawUzwojenia(const wxString& tekst) { tcWtorne_->SetValue(tekst); }
    wxString Wynik(const wxString& klucz) const;                       ///< "moc_wtorna","moc_pierwotna","prad_pierwotny","iloczyn_wym","iloczyn_pos","zwoje1","zapas"
    wxString Tabela() const { return tcTabela_->GetValue(); }
    wxString Uwagi() const { return lbUwagi_->GetLabel(); }
    wxString Drut() const { return tcDrut_->GetValue(); }

private:
    static constexpr int kPol = 17;
    PanelRdzenia* rys_ = nullptr;
    wxChoice* chKsztaltka_ = nullptr;
    wxTextCtrl *tc_[kPol] = {}, *tcWtorne_ = nullptr, *tcTabela_ = nullptr;
    wxStaticText *lbWyn_[8] = {}, *lbUwagi_ = nullptr;
    wxTextCtrl* tcDrut_ = nullptr;
    wxString wyn_[8];
    bool wypelniam_ = false;
    void Przelicz();
    void ZKsztaltki();
    void Typowe();
};

#endif
