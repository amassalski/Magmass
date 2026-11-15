#ifndef DIALOGDLAWIKA_H
#define DIALOGDLAWIKA_H

#include <wx/wx.h>

/// Rysunek rdzenia EI z oznaczonymi wymiarami A, B, a, b, c, d, h
class PanelRdzenia : public wxPanel {
public:
    explicit PanelRdzenia(wxWindow* parent);
    void Ustaw(double A, double B, double a, double c, double d, double h);
private:
    double A_ = 102, B_ = 85, a_ = 34, c_ = 17, d_ = 17, h_ = 51;
    void OnPaint(wxPaintEvent&);
};

/// Obliczanie dławika zasilacza z prądem stałym (rdzeń EI ze szczeliną), wzorowane na ECCLab
class DialogDlawika : public wxDialog {
public:
    explicit DialogDlawika(wxWindow* parent);

    // --- dostęp dla testów
    void UstawKsztaltke(const wxString& nazwa);                                   ///< np. "EI102": wypełnia wymiary
    void UstawPole(const wxString& nazwa, const wxString& wartosc);                ///< "A","B","a","b","c","d","h","L","I","mi","B_T","J","drut"
    wxString Wynik(const wxString& nazwa) const;
    wxString Uwagi() const { return lbUwagi_->GetLabel(); }
    wxString Drut() const { return tcDrut_->GetValue(); }
    wxString TrescOstrzezenia() const { return tresc_; }                           ///< tekst okna "Uwaga !!!!" (puste, gdy brak)

private:
    static constexpr int kPol = 13, kWyn = 20;
    PanelRdzenia* rys_ = nullptr;
    wxChoice *chKsztaltka_ = nullptr, *chBlacha_ = nullptr;
    wxTextCtrl* tc_[kPol] = {};
    wxSlider* suwak_ = nullptr;
    wxStaticText* lb_[kWyn] = {};
    wxStaticText* lbUwagi_ = nullptr;
    wxTextCtrl* tcDrut_ = nullptr;
    wxString wyn_[kWyn], tresc_, szczelinaTekst_, bezSzczelinyTekst_;
    bool wypelniam_ = false;
    void Przelicz();
    void ZKsztaltki();
    void Oblicz();
    void Pomoc();
};

#endif
