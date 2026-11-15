#ifndef PANELSCHEMATU_H
#define PANELSCHEMATU_H

#include <wx/wx.h>
#include <string>
#include <vector>

/// Prosty schemat do kalkulatorów. Elementy, które kalkulator oblicza, są narysowane kolorem i pogrubione,
/// elementy zadane (wejściowe) zwykłą kreską.
class PanelSchematu : public wxPanel {
public:
    enum class Rodzaj { Dzielnik, LED, FiltrRC, Zasilacz, Tranzystor, Lampa, KondensatorESR, LM317, LM337, CewkaJedno, CewkaWiele, ZasilaczMostek, ZasilaczOdczep };

    PanelSchematu(wxWindow* parent, Rodzaj rodzaj);

    /// Układa schemat z prawej strony treści strony (kalkulator po lewej), ustawia sizer strony; zwraca panel.
    static PanelSchematu* Obok(wxPanel* strona, wxSizer* tresc, Rodzaj rodzaj);
    /// Etykiety elementów wyróżnionych na schemacie (np. "R1", "R2", "Uwy").
    static std::vector<std::string> ElementyLiczone(Rodzaj rodzaj);

    Rodzaj RodzajSchematu() const { return rodzaj_; }
    void UstawRodzaj(Rodzaj r) { rodzaj_ = r; Refresh(); }
    bool Liczony(const std::string& e) const;

private:
    Rodzaj rodzaj_;
    void OnPaint(wxPaintEvent&);
};

#endif  // PANELSCHEMATU_H
