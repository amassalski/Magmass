#include "PanelWykresuZnieksztalcen.h"
#include "KoloryMotywu.h"
#include "../Narzedzia/Tlumaczenia.h"

#include <wx/dcbuffer.h>
#include <algorithm>
#include <cmath>

PanelWykresuZnieksztalcen::PanelWykresuZnieksztalcen(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(430, 330)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(430, 330));
    Bind(wxEVT_PAINT, &PanelWykresuZnieksztalcen::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
}

void PanelWykresuZnieksztalcen::UstawTryb(bool odcinki) {
    odcinki_ = odcinki;
    Refresh();
}

namespace {
// przykładowa trioda: Ia = k·(Ua/μ + Ug)^1.5 [mA], Ug <= 0
double pradAnody(double ua, double ug) {
    const double x = ua / 18.0 + ug;
    return x > 0 ? 2.1 * std::pow(x, 1.5) : 0.0;
}
}

void PanelWykresuZnieksztalcen::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    const wxColour tlo = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW), tekst = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    dc.SetBackground(wxBrush(tlo));
    dc.Clear();
    const wxSize rozm = GetClientSize();
    const int lewy = 46, prawy = 14, gora = 14, dol = 34;
    const int W = rozm.x - lewy - prawy, H = rozm.y - gora - dol;
    if (W < 100 || H < 100) return;
    const double uaMax = 400, iaMax = 130, ub = 350, ra = 3.0;   // oś Ua [V], oś Ia [mA], napięcie zasilania i opór obciążenia [kΩ]
    auto X = [&](double ua) { return lewy + static_cast<int>(std::lround(ua / uaMax * W)); };
    auto Y = [&](double ia) { return gora + H - static_cast<int>(std::lround(ia / iaMax * H)); };
    const wxFont f0 = GetFont();
    wxFont male = f0; male.SetPointSize(std::max(7, f0.GetPointSize() - 1));
    dc.SetFont(male);
    dc.SetTextForeground(tekst);

    // osie ze strzałkami
    dc.SetPen(wxPen(tekst, 1));
    dc.DrawLine(lewy, gora + H, lewy + W, gora + H);
    dc.DrawLine(lewy, gora + H, lewy, gora);
    dc.SetBrush(wxBrush(tekst));
    const wxPoint s1[3] = {wxPoint(lewy + W, gora + H), wxPoint(lewy + W - 8, gora + H - 3), wxPoint(lewy + W - 8, gora + H + 3)};
    const wxPoint s2[3] = {wxPoint(lewy, gora), wxPoint(lewy - 3, gora + 8), wxPoint(lewy + 3, gora + 8)};
    dc.DrawPolygon(3, s1);
    dc.DrawPolygon(3, s2);
    dc.DrawText("Ua [V]", lewy + W - 40, gora + H + 6);
    dc.DrawText("Ia [mA]", 4, gora - 2);

    // charakterystyki anodowe dla Ug0 + {+Um, +0,5Um, 0, -0,5Um, -Um}; Ug0 = -6 V, Um = 6 V
    const double ug[5] = {0.0, -3.0, -6.0, -9.0, -12.0};
    const char* etUg[5] = {"+Um", "+0,5Um", "Ug0", "-0,5Um", "-Um"};
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    for (int k = 0; k < 5; k++) {
        dc.SetPen(wxPen(Kolory::Szary(), k == 2 ? 2 : 1));
        wxPoint poprz;
        bool jest = false;
        for (int px = 0; px <= W; px += 3) {
            const double ua = px * uaMax / W, ia = pradAnody(ua, ug[k]);
            if (ia > iaMax || ia <= 0) { jest = false; continue; }
            const wxPoint p(lewy + px, Y(ia));
            if (jest) dc.DrawLine(poprz, p);
            poprz = p; jest = true;
        }
    }
    // prosta obciążenia Ia = (Ub - Ua)/Ra i jej przecięcia z charakterystykami (bisekcja)
    auto prosta = [&](double ua) { return (ub - ua) / ra; };
    double uaP[5], iaP[5];
    for (int k = 0; k < 5; k++) {
        double lo = 0, hi = ub;
        for (int i = 0; i < 60; i++) {
            const double mid = 0.5 * (lo + hi);
            (pradAnody(mid, ug[k]) > prosta(mid) ? hi : lo) = mid;
        }
        uaP[k] = 0.5 * (lo + hi); iaP[k] = prosta(uaP[k]);
    }
    dc.SetPen(wxPen(Kolory::Wynik(), 2));
    dc.DrawLine(X(uaMax * 0.0 + (ub - iaMax * ra)), Y(iaMax), X(ub), Y(0));   // prosta od Ia = iaMax do Ua = Ub
    dc.SetTextForeground(Kolory::Szary());
    for (int k = 0; k < 5; k++) {   // etykiety charakterystyk przy ich górnym (albo prawym) końcu, naprzemiennie wyżej i niżej
        double ua = uaMax;
        for (double u = 0; u <= uaMax; u += 1) if (pradAnody(u, ug[k]) >= iaMax * 0.97) { ua = u; break; }
        const wxString et = wxString::FromUTF8(etUg[k]);
        const int w = dc.GetTextExtent(et).x;
        const int x = std::min(X(ua) - w / 2, lewy + W - w);
        const int y = std::max(gora, Y(std::min(iaMax, pradAnody(ua, ug[k]))) - 14 - (k % 2) * 12);
        dc.DrawText(et, x, y);
    }
    dc.SetTextForeground(tekst);

    // punkty 1..5 (od największego prądu)
    const char* etI[5] = {"Ia max", "Ia1", "Ia0", "Ia2", "Ia min"};
    const wxColour opis = Kolory::Ostrzezenie();
    if (!odcinki_) {
        // rzuty punktów na oś Ia: tu odczytujemy prądy
        dc.SetPen(wxPen(opis, 1, wxPENSTYLE_SHORT_DASH));
        for (int k = 0; k < 5; k++) dc.DrawLine(lewy, Y(iaP[k]), X(uaP[k]), Y(iaP[k]));
        dc.SetTextForeground(opis);
        for (int k = 0; k < 5; k++) dc.DrawText(wxString::FromUTF8(etI[k]), lewy + 6, Y(iaP[k]) - 15);
    } else {
        // odcinki prostej obciążenia między punktami: d, e, f, g oraz a, b, c (równoległe oznaczenia z odsunięciem)
        const double dx = 1.0, dy = -1.0 / ra;                           // kierunek prostej w układzie danych
        const double dlug = std::sqrt((dx * W / uaMax) * (dx * W / uaMax) + (dy * H / iaMax) * (dy * H / iaMax));
        const double tx = dx * W / uaMax / dlug, ty = -dy * H / iaMax / dlug;   // jednostkowy wektor wzdłuż prostej na ekranie (y w dół)
        const double nx = ty, ny = -tx;                                          // normalna ekranowa: w górę i w prawo od prostej
        auto nawias = [&](int p0, int p1, double odsun, const char* nazwa) {
            const wxPoint a(X(uaP[p0]) + static_cast<int>(nx * odsun), Y(iaP[p0]) + static_cast<int>(ny * odsun));
            const wxPoint b(X(uaP[p1]) + static_cast<int>(nx * odsun), Y(iaP[p1]) + static_cast<int>(ny * odsun));
            dc.SetPen(wxPen(opis, 1));
            dc.DrawLine(a, b);
            for (const wxPoint& p : {a, b}) dc.DrawLine(p.x - static_cast<int>(nx * 4), p.y - static_cast<int>(ny * 4), p.x + static_cast<int>(nx * 4), p.y + static_cast<int>(ny * 4));
            dc.SetPen(wxPen(opis, 1, wxPENSTYLE_DOT));
            for (int p : {p0, p1}) dc.DrawLine(X(uaP[p]), Y(iaP[p]), X(uaP[p]) + static_cast<int>(nx * odsun), Y(iaP[p]) + static_cast<int>(ny * odsun));
            dc.SetTextForeground(opis);
            const wxSize ts = dc.GetTextExtent(wxString::FromUTF8(nazwa));
            dc.DrawText(wxString::FromUTF8(nazwa), (a.x + b.x) / 2 + static_cast<int>(nx * 6) - ts.x / 2, (a.y + b.y) / 2 + static_cast<int>(ny * 6) - ts.y / 2);
        };
        nawias(0, 1, 18, "d"); nawias(1, 2, 18, "e"); nawias(2, 3, 18, "f"); nawias(3, 4, 18, "g");
        nawias(0, 2, 40, "a = d+e"); nawias(2, 4, 40, "b = f+g"); nawias(1, 3, 80, "c = e+f");
    }
    // punkty
    dc.SetBrush(wxBrush(Kolory::Wynik()));
    dc.SetPen(wxPen(Kolory::Wynik(), 1));
    for (int k = 0; k < 5; k++) dc.DrawCircle(X(uaP[k]), Y(iaP[k]), 4);
    dc.SetTextForeground(Kolory::Wynik());
    if (odcinki_) for (int k = 0; k < 5; k++) dc.DrawText(wxString::Format("%d", k + 1), X(uaP[k]) - 14, Y(iaP[k]) + 4);
    dc.SetTextForeground(Kolory::Szary());
    dc.DrawText(TR("wykres poglądowy (przykładowa trioda)"), lewy + 8, gora + H + 6);
}
