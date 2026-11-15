#ifndef DIALOGMETODYGRAFICZNE_H
#define DIALOGMETODYGRAFICZNE_H

#include "../Logika/MetodyGraficzne.h"

#include <wx/simplebook.h>
#include <wx/tglbtn.h>
#include <wx/wx.h>

#include <functional>
#include <optional>

/// Płótno z obrazem charakterystyki anodowej: kalibracja, prosta obciążenia, siatka kontrolna i krzywa Pa max
class PlotCharakterystyk : public wxPanel {
public:
    explicit PlotCharakterystyk(wxWindow* parent);

    bool UstawObraz(const wxImage& obraz);
    bool MaObraz() const { return obraz_.IsOk(); }
    wxSize RozmiarObrazu() const { return obraz_.IsOk() ? wxSize(obraz_.GetWidth(), obraz_.GetHeight()) : wxSize(0, 0); }

    /// krok: 0 = punkt (0,0), 1 = punkt na osi napięcia, 2 = punkt na osi prądu
    void UstawPunktKalibracji(int krok, double ix, double iy);
    bool PunktUstawiony(int krok) const { return ustawiony_[krok]; }
    void UstawAktywnyKrok(int krok) { aktywny_ = krok; }
    /// Przelicza skalę z punktów i wartości; nullopt, gdy dane niepełne lub błędne
    bool Skaluj(double napiecieV, double pradMa);
    const std::optional<MetodyGraficzne::Kalibracja>& Skala() const { return kal_; }

    void UstawTrybWynikow(bool wyniki) { wyniki_ = wyniki; Refresh(); }
    const MetodyGraficzne::Prosta& Prosta() const { return prosta_; }
    bool MaProste() const { return maProste_; }
    void UstawKoniec(MetodyGraficzne::Koniec k, MetodyGraficzne::PunktUI cel);
    void UstawBlokady(bool ra, bool amplituda) { blok_ = {ra, amplituda}; }
    void UstawSiatke(double krokPradMa, double krokNapV) { siatkaI_ = krokPradMa; siatkaU_ = krokNapV; Refresh(); }
    void UstawPaMax(double paW) { paMax_ = paW; Refresh(); }
    double PaMax() const { return paMax_; }
    size_t PunktowPaMax() const;

    std::function<void(MetodyGraficzne::PunktUI)> przyDwukliku;          ///< dwuklik w trybie wyników (wartość w miejscu kliknięcia)
    std::function<void()> przyZmianie;                                  ///< zmiana punktów, prostej albo skali
    std::function<void(std::optional<MetodyGraficzne::PunktUI>)> przyRuchuKursora;

private:
    wxImage obraz_;
    wxBitmap bmp_;
    wxSize bmpRozmiar_;
    double skala_ = 1, offx_ = 0, offy_ = 0;
    MetodyGraficzne::PunktPx pkt_[3];
    bool ustawiony_[3] = {false, false, false};
    int aktywny_ = 0;
    bool wyniki_ = false;
    std::optional<MetodyGraficzne::Kalibracja> kal_;
    MetodyGraficzne::Prosta prosta_;
    bool maProste_ = false;
    MetodyGraficzne::Blokady blok_;
    double siatkaI_ = 0, siatkaU_ = 0, paMax_ = 0;
    int przeciagany_ = -1;   // 0 koniec prądowy, 1 napięciowy

    wxPoint NaEkran(MetodyGraficzne::PunktPx p) const;
    MetodyGraficzne::PunktPx NaObraz(const wxPoint& e) const;
    void Przelicz();
    void OnPaint(wxPaintEvent&);
    void OnMysz(wxMouseEvent&);
    void OnDwuklik(wxMouseEvent&);
};

/// Metody graficzne (wzorowane na ECCLab): obraz charakterystyki anodowej, skalowanie, prosta obciążenia,
/// oporność Ra klasy A i B, moce SE i PP oraz kontrolna krzywa mocy admisyjnej anody
class DialogMetodyGraficzne : public wxDialog {
public:
    explicit DialogMetodyGraficzne(wxWindow* parent);

    // --- dostęp dla testów
    bool UstawObraz(const wxImage& o) { return plot_->UstawObraz(o); }
    void UstawPunktKalibracji(int krok, double ix, double iy) { plot_->UstawPunktKalibracji(krok, ix, iy); Odswiez(); }
    bool Skaluj(const wxString& napiecie, const wxString& prad);
    wxString OpisSkali() const { return lbSkala_->GetLabel(); }
    bool Skalibrowany() const { return plot_->Skala().has_value(); }
    void PokazStrone(int nr);
    void UstawKonieczPradu(double u, double i);
    void UstawKoniecNapiecia(double u, double i);
    void UstawBlokady(bool ra, bool amplituda);
    void UstawPaMax(const wxString& w);
    void UstawSiatke(const wxString& i, const wxString& u);
    size_t PunktowPaMax() const { return plot_->PunktowPaMax(); }
    /// Dwuklik na prostej obciążenia przy napięciu @p u: zapisuje prąd prostej w bieżącym punkcie Ia1/Ia0/Ia2 i przechodzi do następnego
    void Dwuklik(double u);
    wxString Wynik(const wxString& nazwa) const;   ///< także "ia1","ia0","ia2","zn_h","zn_h2","zn_h3","zn_h4"; poprzednie: "ra", "raa", "moc_se", "moc_pp", "du", "di", "u1", "i1", "u2", "i2"
    const MetodyGraficzne::Prosta& Prosta() const { return plot_->Prosta(); }

private:
    PlotCharakterystyk* plot_ = nullptr;
    wxSimplebook* ksiazka_ = nullptr;
    wxToggleButton *bSkalowanie_ = nullptr, *bWyniki_ = nullptr;
    wxToggleButton* krokBtn_[3] = {};
    wxTextCtrl *tcU_ = nullptr, *tcI_ = nullptr, *tcSiatkaI_ = nullptr, *tcSiatkaU_ = nullptr, *tcPa_ = nullptr;
    wxCheckBox *cbRa_ = nullptr, *cbAmp_ = nullptr, *cbSiatkaI_ = nullptr, *cbSiatkaU_ = nullptr, *cbPa_ = nullptr;
    wxStaticText *lbSkala_ = nullptr, *lbKursor_ = nullptr, *lbKrok_[3] = {};
    wxStaticText *lbP1_ = nullptr, *lbP2_ = nullptr, *lbDu_ = nullptr, *lbDi_ = nullptr, *lbRa_ = nullptr, *lbRaa_ = nullptr,
                 *lbSE_ = nullptr, *lbPP_ = nullptr;
    wxString wyniki_[10];
    wxRadioButton* rbPkt_[3] = {};
    wxStaticText *lbPkt_[3] = {}, *lbZn_[4] = {};
    double pktI_[3] = {0, 0, 0};
    bool pktUstawiony_[3] = {false, false, false};
    wxString zn_[7];
    int aktPkt_ = 0;
    void PrzeliczZnieksztalcenia();

    wxPanel* StronaSkalowania(wxWindow* p);
    wxPanel* StronaWynikow(wxWindow* p);
    void Odswiez();
    void WczytajObraz();
    void PrzeliczSkale();
    void ZastosujNakladki();
};

#endif  // DIALOGMETODYGRAFICZNE_H
