#ifndef DIALOGTRAFOPPETAP3_H
#define DIALOGTRAFOPPETAP3_H

#include "../Logika/TrafoPP.h"
#include "PanelTrafoModel.h"
#include <wx/wx.h>

/// Etap 3 transformatora PP: uzwojenia wykonane (całkowite zwoje, normalne średnice drutu), rozmieszczenie w oknie, rezystancje, rozproszenie
class DialogTrafoPPEtap3 : public wxDialog {
public:
    DialogTrafoPPEtap3(wxWindow* parent, const TrafoPP::Wymagania& w, const TrafoPP::Rdzen& r, const TrafoPP::Uzwojenia& u);

    // --- dostęp dla testów
    void UstawPole(const wxString& nazwa, const wxString& wartosc);    ///< "z1","d1","z2","d2","beta","k","rho"
    void UstawUklad(int numer);                                         ///< szablon: 0 = P-S, 1 = P/2-S-P/2, 2 = gitarowy 3-sekcyjny, 3 = HiFi dwukomorowy, 4 = HiFi dwukomorowy przeplatany, 5 = Mesa Boogie DR (odczepy 4/8/16 Ω), 6 = własny
    void UstawSekcje(const wxString& tekst);                            ///< wpisuje tabelę sekcji (A1 600 / W1 100 / | ...) i aktualizuje zwoje z1, z2
    wxString Sekcje() const { return tcSekcje_->GetValue(); }
    void UstawWtorne(bool rownolegle);
    void UstawOdczep(int numer);                                        ///< transformator z odczepami (DR): 0 = 4 Ω, 1 = 8 Ω, 2 = 16 Ω
    void UstawKarkas(const wxString& siankaMm, const wxString& marginesMm);   ///< ścianka karkasu i margines warstwy [mm]
    void UstawPrzegrode(const wxString& mm);
    wxString Wynik(const wxString& klucz) const;
    PanelTrafoModel* Model() const { return model_; }                  ///< analiza dokładna (wspólny model)
    wxString Tabela() const { return tcTabela_->GetValue(); }
    wxString Uwagi() const { return lbUwagi_->GetLabel(); }
    wxString Drut() const { return tcDrut_->GetValue(); }

private:
    static constexpr int kPol = 7, kWyn = 15;
    TrafoPP::Wymagania w_;
    TrafoPP::Rdzen r_;
    TrafoPP::Uzwojenia u_;
    wxChoice *chUklad_ = nullptr, *chWtorne_ = nullptr, *chOdczep_ = nullptr;
    wxTextCtrl *tcSekcje_ = nullptr, *tcPrzegroda_ = nullptr, *tcScianka_ = nullptr, *tcMargines_ = nullptr;
    bool wypelniam_ = false;
    void ZSzablonu();
    PanelTrafoModel* model_ = nullptr;
    wxTextCtrl *tc_[kPol] = {}, *tcTabela_ = nullptr;
    wxStaticText *lb_[kWyn] = {}, *lbUwagi_ = nullptr;
    wxTextCtrl* tcDrut_ = nullptr;
    wxString wyn_[kWyn];
    void Przelicz();
    void PokazUwagi();                       ///< składa komunikaty transformatora i modelu w ramce „Komunikaty”
    wxString uwagiTrafo_, uwagiModelu_;
};

#endif
