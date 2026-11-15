#ifndef PANELWYKRESUZNIEKSZTALCEN_H
#define PANELWYKRESUZNIEKSZTALCEN_H

#include <wx/wx.h>

/// Rysunek poglądowy do metody pięciu rzędnych: charakterystyki anodowe z prostą obciążenia i pięcioma punktami.
/// Tryb „prądy” pokazuje, które prądy anodowe odczytać (rzuty punktów na oś Ia), tryb „odcinki” – które odcinki prostej
/// obciążenia zmierzyć (d, e, f, g oraz a = d+e, b = f+g, c = e+f). Wykres jest schematyczny (przykładowa trioda), nie dane użytkownika.
class PanelWykresuZnieksztalcen : public wxPanel {
public:
    explicit PanelWykresuZnieksztalcen(wxWindow* parent);
    void UstawTryb(bool odcinki);
    bool Odcinki() const { return odcinki_; }

private:
    bool odcinki_ = false;
    void OnPaint(wxPaintEvent&);
};

#endif
