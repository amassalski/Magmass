#ifndef DIALOGTRAFOSEWYKONANE_H
#define DIALOGTRAFOSEWYKONANE_H

#include "../Logika/TrafoSE.h"
#include "PanelTrafoModel.h"
#include <wx/wx.h>

/// Transformator SE: uzwojenia wykonane (całkowite zwoje, normalne średnice drutu, szczelina), rozmieszczenie w oknie,
/// rezystancje, rozproszenie, sprawność. Wzory fizyczne; wartości z książki pokazane obok jako porównanie.
class DialogTrafoSEWykonane : public wxDialog {
public:
    DialogTrafoSEWykonane(wxWindow* parent, const TrafoSE::Wejscie& w, const TrafoSE::Wynik& p);

    // --- dostęp dla testów
    void UstawPole(const wxString& nazwa, const wxString& wartosc);   ///< "z1","d1","z2","d2","szczelina","przekladka","izolacja","k","rho","fg"
    void UstawUklad(int numer);                                        ///< 0 = P-S, 1 = P/2-S-P/2, 2 = S/2-P-S/2
    wxString Wynik(const wxString& klucz) const;
    wxString Tabela() const { return tcTabela_->GetValue(); }
    wxString Uwagi() const { return lbUwagi_->GetLabel(); }   // komunikaty trafo i modelu (jak w ramce Komunikaty)
    wxString Drut() const { return tcDrut_->GetValue(); }
    PanelTrafoModel* Model() const { return model_; }                  ///< analiza dokładna (wspólny model)

private:
    static constexpr int kPol = 12, kWyn = 20;
    TrafoSE::Wejscie w_;
    TrafoSE::Wynik p_;
    PanelTrafoModel* model_ = nullptr;
    wxChoice* chUklad_ = nullptr;
    wxTextCtrl *tc_[kPol] = {}, *tcTabela_ = nullptr;
    wxStaticText *lb_[kWyn] = {}, *lbUwagi_ = nullptr;
    wxTextCtrl* tcDrut_ = nullptr;
    wxString wyn_[kWyn];
    void Przelicz();
    void PokazUwagi();
    wxString uwagiTrafo_, uwagiModelu_;
};

#endif
