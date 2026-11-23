#include "PanelSchematuBarwy.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/settings.h>

#include <cmath>

PanelSchematuBarwy::PanelSchematuBarwy(wxWindow* rodzic) : wxPanel(rodzic, wxID_ANY, wxDefaultPosition, wxSize(360, 260)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(300, 220));
    Bind(wxEVT_PAINT, &PanelSchematuBarwy::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
}

void PanelSchematuBarwy::Ustaw(SchematBarwy::Rysunek rysunek) {
    rysunek_ = std::move(rysunek);
    Refresh();
}

void PanelSchematuBarwy::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    const wxColour tlo = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour kolor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    dc.SetBackground(wxBrush(tlo));
    dc.Clear();
    if (rysunek_.prymy.empty() || rysunek_.szerokosc <= 0) return;

    const wxSize rozm = GetClientSize();
    const double skala = std::min({rozm.x / rysunek_.szerokosc, rozm.y / rysunek_.wysokosc, 1.6});
    const double dx = (rozm.x - rysunek_.szerokosc * skala) / 2, dy = (rozm.y - rysunek_.wysokosc * skala) / 2;
    auto X = [&](double x) { return static_cast<int>(std::lround(dx + x * skala)); };
    auto Y = [&](double y) { return static_cast<int>(std::lround(dy + y * skala)); };

    dc.SetPen(wxPen(kolor, std::max(1, static_cast<int>(std::lround(1.4 * skala)))));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetTextForeground(kolor);
    wxFont f = GetFont();
    f.SetPointSize(std::max(7, static_cast<int>(std::lround(f.GetPointSize() * std::min(1.0, skala + 0.2)))));
    dc.SetFont(f);

    using SchematBarwy::Rodzaj;
    for (const SchematBarwy::Prym& p : rysunek_.prymy) {
        switch (p.rodzaj) {
            case Rodzaj::Linia: dc.DrawLine(X(p.x1), Y(p.y1), X(p.x2), Y(p.y2)); break;
            case Rodzaj::Kropka: {
                dc.SetBrush(wxBrush(kolor));
                dc.DrawCircle(X(p.x1), Y(p.y1), std::max(2, static_cast<int>(std::lround(3 * skala))));
                dc.SetBrush(*wxTRANSPARENT_BRUSH);
                break;
            }
            case Rodzaj::Zacisk: dc.DrawCircle(X(p.x1), Y(p.y1), std::max(2, static_cast<int>(std::lround(3.5 * skala)))); break;
            case Rodzaj::Masa: {
                const int x = X(p.x1), y = Y(p.y1);
                const int w = static_cast<int>(std::lround(10 * skala)), k = std::max(2, static_cast<int>(std::lround(3 * skala)));
                dc.DrawLine(x, y, x, y + k);
                dc.DrawLine(x - w, y + k, x + w, y + k);
                dc.DrawLine(x - w * 2 / 3, y + 2 * k, x + w * 2 / 3, y + 2 * k);
                dc.DrawLine(x - w / 3, y + 3 * k, x + w / 3, y + 3 * k);
                break;
            }
            case Rodzaj::Tekst: {
                const wxString t = wxString::FromUTF8(p.tekst);
                const wxSize s = dc.GetTextExtent(t);
                dc.DrawText(t, p.x2 > 0.5 ? X(p.x1) : X(p.x1) - s.x / 2, Y(p.y1));
                break;
            }
            case Rodzaj::Wzmacniacz: {
                const int xl = X(p.x1), xr = X(p.x2), y = Y(p.y1), h = static_cast<int>(std::lround(22 * skala));
                wxPoint tr[3] = {wxPoint(xl, y - h), wxPoint(xl, y + h), wxPoint(xr, y)};
                dc.DrawPolygon(3, tr);
                dc.DrawText(wxT("−"), xl + 3, y - h / 2 - dc.GetCharHeight() / 2 - 1);
                dc.DrawText(wxT("+"), xl + 3, y + h / 2 - dc.GetCharHeight() / 2 + 1);
                break;
            }
            case Rodzaj::Rezystor:
            case Rodzaj::Kondensator:
            case Rodzaj::Cewka: {
                // symbol od (x1,y1) do (x2,y2); układ lokalny: t wzdłuż osi, n prostopadle
                const double ax = X(p.x1), ay = Y(p.y1), bx = X(p.x2), by = Y(p.y2);
                const double dl = std::hypot(bx - ax, by - ay);
                if (dl < 1) break;
                const double tx = (bx - ax) / dl, ty = (by - ay) / dl, nx = -ty, ny = tx;
                auto P = [&](double t, double n) { return wxPoint(static_cast<int>(std::lround(ax + tx * t + nx * n)), static_cast<int>(std::lround(ay + ty * t + ny * n))); };
                const double h = 6 * skala;
                if (p.rodzaj == Rodzaj::Rezystor) {
                    wxPoint pt[4] = {P(0, -h), P(dl, -h), P(dl, h), P(0, h)};
                    dc.DrawPolygon(4, pt);
                } else if (p.rodzaj == Rodzaj::Kondensator) {
                    const double s = dl / 2, g = 3.5 * skala, w = 9 * skala;
                    dc.DrawLine(P(0, 0), P(s - g, 0));
                    dc.DrawLine(P(s + g, 0), P(dl, 0));
                    dc.DrawLine(P(s - g, -w), P(s - g, w));
                    dc.DrawLine(P(s + g, -w), P(s + g, w));
                } else {
                    std::vector<wxPoint> pts;
                    for (int i = 0; i <= 48; ++i) {
                        const double t = dl * i / 48.0, ph = 4 * M_PI * i / 48.0;
                        pts.push_back(P(t, -std::fabs(std::sin(ph)) * 7 * skala));
                    }
                    dc.DrawLines(static_cast<int>(pts.size()), pts.data());
                }
                break;
            }
        }
    }
}
