#ifndef DIALOGTRAFOPP_H
#define DIALOGTRAFOPP_H

#include <wx/wx.h>
#include "WymiaryRdzenia.h"
#include <string>
#include <vector>

/// Transformator głośnikowy przeciwsobny (PP): Etap 1 (wymagania -> pożądane parametry) i Etap 2 (zdobyty rdzeń -> uzwojenia)
class DialogTrafoPP : public wxDialog {
public:
    explicit DialogTrafoPP(wxWindow* parent);

    // --- dostęp dla testów
    void UstawSpadek(int db);                                          ///< 1, 2 lub 3 dB na krańcach pasma
    void UstawTyp(int numer);                                          ///< 0 = Hi-Fi, 1 = gitarowy
    void UstawKsztaltke(const wxString& nazwa);
    bool UstawLampe(const wxString& symbol);                           ///< lampa z katalogu: Ri i I0 z jej parametrów (false, gdy brak w liście)
    wxString PoleTekst(const wxString& nazwa) const;                   ///< bieżący tekst pola (jak w UstawPole)
    wxString WybranaLampa() const;                                     ///< symbol wybranej lampy z katalogu albo pusty tekst (ręcznie)
    int LiczbaLamp() const { return static_cast<int>(lampySym_.size()); }
    void UstawPole(const wxString& nazwa, const wxString& wartosc);    ///< "R0","Raa","P0","Fd","Fg","Ri","I0","B","mi","eta","alfa","beta","sigma","S","Fe","Cu","h","a","pakiet", wymiary blachy w mm: "wymA","wymB","wyma","wymc","wymd","wymh"
    wxString Wynik(const wxString& klucz) const;                        ///< "Li","Lr","n","iloczyn","przekroj","z1","d1","z2","d2","I1","I2", porównanie gitarowe: "g_L1","g_V","g_z1","g_I1","g_d1","g_w4","g_w8","g_w16" (zwoje / drut)

private:
    static constexpr int kPol = 19, kWyn = 19;
    class PanelRdzenia* rys_ = nullptr;
    wxChoice *chSpadek_ = nullptr, *chTyp_ = nullptr, *chKsztaltka_ = nullptr, *chLampa_ = nullptr;
    std::vector<std::string> lampySym_;
    std::vector<double> lampyRiOm_, lampyI0Ma_, lampyRaaOm_;   // Raa 0 = brak w katalogu
    void WczytajLampy();
    void ZLampy();
    wxTextCtrl* tc_[kPol] = {};
    wxStaticText* lb_[kWyn] = {};
    wxString wyn_[kWyn];
    bool wypelniam_ = false;
    void Przelicz();
    void ZKsztaltki();
    void ZWymiarow();   ///< pola pochodne i rysunek z pól wymiarów rdzenia
    WymiaryRdzenia wym_;
    void Dalej();
    wxButton* bDalej_ = nullptr;
public:
    /// Etap 3 dla bieżących danych (nullptr gdy etapy 1-2 nie dają wyniku); wywołujący usuwa okno
    class DialogTrafoPPEtap3* OtworzEtap3();
    bool Etap3Dostepny() const { return bDalej_ && bDalej_->IsEnabled(); }
};

#endif
