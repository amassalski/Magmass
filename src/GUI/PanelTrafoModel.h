#ifndef PANELTRAFOMODEL_H
#define PANELTRAFOMODEL_H

#include "../Logika/TrafoModel.h"
#include <functional>
#include <optional>
#include <wx/wx.h>

/// Wspólny panel „Analiza dokładna” (jeden model fizyczny dla SE i PP): parametry materiału rdzenia, wyniki schematu zastępczego i wykres pasma.
class PanelTrafoModel : public wxPanel {
public:
    PanelTrafoModel(wxWindow* parent, bool przeciwsobny);
    std::function<void()> naZmiane;                                     ///< wywoływane po zmianie parametru
    bool Parametry(TrafoModel::Parametry& p) const;                     ///< false gdy pole nie jest liczbą
    void Pokaz(const std::optional<TrafoModel::Wynik>& w, double spadekDb = 3);   ///< spadekDb tylko do narysowania linii progu
    // --- dostęp dla testów
    void UstawPole(const wxString& nazwa, const wxString& wartosc);     ///< "mu","bs","styk","straty","eps","asym"
    wxString Wynik(const wxString& klucz) const;                        ///< "lp","lp0","mu","bdc","bac","bsuma","fd","fg","rez","eta","etacu","ra","pfe","lpk"
    wxString Uwagi() const { return naUwagi ? uwagi_ : lbUwagi_->GetLabel(); }
    /// Komunikaty modelu zamiast pod panelem trafiają do @p f (np. do ramki „Komunikaty” formularza); wewnętrzna etykieta jest ukrywana.
    void UwagiDo(std::function<void(const wxString&)> f);
    size_t PunktowKrzywej() const { return krzywa_.size(); }

private:
    std::function<void(const wxString&)> naUwagi;
    wxString uwagi_;
    static constexpr int kPol = 6, kWyn = 14;
    bool pp_;
    wxTextCtrl* tc_[kPol] = {};
    wxStaticText *lb_[kWyn] = {}, *lbUwagi_ = nullptr;
    wxString wyn_[kWyn];
    std::vector<TrafoModel::PunktKrzywej> krzywa_;
    wxPanel* wykres_ = nullptr;
    mutable double spadekDb_ = 3;
    void Rysuj(wxPaintEvent&);
    void UstawUwagi(const wxString& t);
};

#endif
