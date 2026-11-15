#ifndef DIALOGCEWKIAL_H
#define DIALOGCEWKIAL_H

#include <wx/wx.h>

/// Cewka na rdzeniu z AL (ferryt, toroid): liczba zwojów dla żądanej indukcyjności i odwrotnie
class DialogCewkiAL : public wxDialog {
public:
    explicit DialogCewkiAL(wxWindow* parent);

    // --- dostęp dla testów
    void UstawPole(const wxString& nazwa, const wxString& wartosc);   ///< "AL","L","tol","sprawdz","Ae","I","Bs"
    void UstawJednostke(int numer);                                    ///< 0 = µH, 1 = mH, 2 = H
    wxString Wynik(const wxString& klucz) const;                       ///< "n","zwoje","L","blad","zakres","L_sprawdz","blad_sprawdz","B"
    wxString Uwagi() const { return lbUwagi_->GetLabel(); }

private:
    static constexpr int kPol = 7, kWyn = 8;
    wxTextCtrl* tc_[kPol] = {};
    wxChoice* chJ_ = nullptr;
    wxStaticText *lb_[kWyn] = {}, *lbUwagi_ = nullptr;
    wxString wyn_[kWyn];
    void Przelicz();
};

#endif
