#ifndef DIALOGTRAFOSE_H
#define DIALOGTRAFOSE_H

#include <wx/wx.h>
#include "WymiaryRdzenia.h"

/// Transformator głośnikowy jednotaktowy (SE): jedno okno, parametry po lewej, wyniki po prawej
class DialogTrafoSE : public wxDialog {
public:
    explicit DialogTrafoSE(wxWindow* parent);

    // --- dostęp dla testów
    void UstawSpadek(int db);                                          ///< 1, 2 lub 3 dB na krańcach pasma
    void UstawKsztaltke(const wxString& nazwa);
    wxString PoleTekst(const wxString& nazwa) const;                  ///< bieżący tekst pola wymiaru blachy (nazwy „wym…”)
    void UstawPole(const wxString& nazwa, const wxString& wartosc);   ///< "P","Ra","Ri","Ia","Fd","Rg","eta","pakiet","S","Fe","B","mi","sigma","Cu","h","a", wymiary blachy w mm: "wymA","wymB","wyma","wymc","wymd","wymh"
    wxString Wynik(const wxString& klucz) const;   ///< "Smin","Li","Li_porownanie","n","Ipk","z1","d1","z2","d2","szczelina","Bdc","Bpk","n_porownanie","cy_z1","cy_szczelina","cy_L","cy_Bpk","cy_eta","cy_az","kr_z23","kr_z24","kr_z2","kr_szczelina","kr_przekladka","kr_d1","kr_d1min","kr_d2","kr_iloczyn","wi_q","wi_z1","wi_L1"
    wxString Ostrzezenie() const { return ostrz_->GetLabel(); }
    void UstawA(int numer);   ///< 0 = pentoda bez NFB (A=20), 1 = pentoda z NFB (10), 2 = trioda bez NFB (10), 3 = trioda z NFB (5)
    bool WykonaneDostepne() const { return bDalej_ && bDalej_->IsEnabled(); }
    /// Okno uzwojeń wykonanych dla bieżących danych (nullptr gdy niedostępne); wywołujący usuwa okno
    class DialogTrafoSEWykonane* OtworzWykonane();

private:
    static constexpr int kPol = 16, kWyn = 31;
    class PanelRdzenia* rys_ = nullptr;
    wxChoice *chSpadek_ = nullptr, *chKsztaltka_ = nullptr, *chA_ = nullptr;
    wxTextCtrl* tc_[kPol] = {};
    wxStaticText* lb_[kWyn] = {};
    wxStaticText* ostrz_ = nullptr;
    wxButton* bDalej_ = nullptr;
    void Dalej();
    bool wypelniam_ = false;
    void Przelicz();
    void ZKsztaltki();
    void ZWymiarow();   ///< pola pochodne i rysunek z pól wymiarów rdzenia
    WymiaryRdzenia wym_;
};

#endif
