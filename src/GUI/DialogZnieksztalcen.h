#ifndef DIALOGZNIEKSZTALCEN_H
#define DIALOGZNIEKSZTALCEN_H

#include <wx/wx.h>
#include "PanelWykresuZnieksztalcen.h"

/// Zniekształcenia harmoniczne metodą graficzną (pięć rzędnych): z prądów w pięciu punktach prostej obciążenia
/// albo z długości odcinków zmierzonych na wykresie
class DialogZnieksztalcen : public wxDialog {
public:
    explicit DialogZnieksztalcen(wxWindow* parent);

    // --- dostęp dla testów
    void UstawTryb(bool odcinki);
    void UstawPole(const wxString& nazwa, const wxString& wartosc);   ///< "imax","i1","i0","i2","imin","a".."g"
    wxString Wynik(const wxString& nazwa) const;                      ///< "h","h2","h3","h4" (%), "h_db","h2_db",...,"i0".."i4"
    bool WykresOdcinki() const { return wykres_->Odcinki(); }          ///< rysunek pokazuje odcinki (true) albo prądy (false)
    wxString OpisWykresu() const { return opisWykresu_->GetLabel(); }
    wxString Uwagi() const { return lbUwagi_->GetLabel(); }
    void Wyczysc();

private:
    wxRadioButton *rbPrady_ = nullptr, *rbOdcinki_ = nullptr;
    wxTextCtrl* tc_[12] = {};
    wxStaticText *lbProc_[4] = {}, *lbDb_[4] = {}, *lbI_[5] = {}, *lbUwagi_ = nullptr;
    wxString wyn_[14];
    PanelWykresuZnieksztalcen* wykres_ = nullptr;
    wxStaticText* opisWykresu_ = nullptr;
    void Przelicz();
};

#endif
