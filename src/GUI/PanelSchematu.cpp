#include "PanelSchematu.h"
#include "../Narzedzia/Tlumaczenia.h"

#include <wx/dcbuffer.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr double kSzer = 240.0, kWys = 200.0;   // wymiary schematu w jednostkach rysunku

/// Narzędzia rysowania w jednostkach schematu, skalowane do rozmiaru panelu.
struct Rysownik {
    wxDC& dc;
    double sk, ox, oy;
    wxColour kolZwykly, kolLiczony;
    int grubZ, grubL;

    int X(double x) const { return static_cast<int>(std::lround(ox + x * sk)); }
    int Y(double y) const { return static_cast<int>(std::lround(oy + y * sk)); }
    void Pioro(bool liczony) {
        dc.SetPen(wxPen(liczony ? kolLiczony : kolZwykly, liczony ? grubL : grubZ));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetTextForeground(liczony ? kolLiczony : kolZwykly);
    }
    void Linia(double x1, double y1, double x2, double y2, bool l = false) { Pioro(l); dc.DrawLine(X(x1), Y(y1), X(x2), Y(y2)); }
    void Wezel(double x, double y) {
        Pioro(false);
        dc.SetBrush(wxBrush(kolZwykly));
        dc.DrawCircle(X(x), Y(y), std::max(2, static_cast<int>(2.5 * sk)));
    }
    void Tekst(const wxString& t, double x, double y, bool l = false) {
        wxFont f = dc.GetFont();
        f.SetPixelSize(wxSize(0, std::max(9, static_cast<int>(11 * sk))));
        f.SetWeight(l ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
        dc.SetFont(f);
        Pioro(l);
        dc.DrawText(t, X(x), Y(y));
    }
    void Masa(double x, double y) {
        Linia(x, y, x, y + 6);
        Linia(x - 9, y + 6, x + 9, y + 6);
        Linia(x - 6, y + 9, x + 6, y + 9);
        Linia(x - 3, y + 12, x + 3, y + 12);
    }
    /// Rezystor: prostokąt 12x26 (pionowo) albo 26x12 (poziomo), środek (cx,cy); etykieta obok.
    void Rezystor(double cx, double cy, bool pion, const wxString& n, bool l) {
        Pioro(l);
        const double a = pion ? 6 : 13, b = pion ? 13 : 6;
        dc.DrawRectangle(X(cx - a), Y(cy - b), static_cast<int>(2 * a * sk), static_cast<int>(2 * b * sk));
        if (pion) Tekst(n, cx + 10, cy - 7, l); else Tekst(n, cx - 8, cy - 22, l);
    }
    /// Kondensator: dwie okładki. pion = przewody pionowe (okładki poziome).
    void Kondensator(double cx, double cy, bool pion, const wxString& n, bool l) {
        Pioro(l);
        if (pion) {
            dc.DrawLine(X(cx - 10), Y(cy - 4), X(cx + 10), Y(cy - 4));
            dc.DrawLine(X(cx - 10), Y(cy + 4), X(cx + 10), Y(cy + 4));
            Tekst(n, cx + 14, cy - 7, l);
        } else {
            dc.DrawLine(X(cx - 4), Y(cy - 10), X(cx - 4), Y(cy + 10));
            dc.DrawLine(X(cx + 4), Y(cy - 10), X(cx + 4), Y(cy + 10));
            Tekst(n, cx - 8, cy - 26, l);
        }
    }
    /// Dioda w dół (anoda u góry), środek (cx,cy), wysokość 18.
    void DiodaPion(double cx, double cy, bool led) {
        Pioro(false);
        wxPoint tr[3] = {wxPoint(X(cx - 8), Y(cy - 9)), wxPoint(X(cx + 8), Y(cy - 9)), wxPoint(X(cx), Y(cy + 9))};
        dc.DrawPolygon(3, tr);
        dc.DrawLine(X(cx - 8), Y(cy + 9), X(cx + 8), Y(cy + 9));
        if (led) {
            dc.DrawLine(X(cx + 12), Y(cy - 8), X(cx + 20), Y(cy - 16));
            dc.DrawLine(X(cx + 14), Y(cy), X(cx + 22), Y(cy - 8));
        }
    }
    /// Dioda w prawo (anoda z lewej), środek (cx,cy).
    void DiodaPoz(double cx, double cy) {
        Pioro(false);
        wxPoint tr[3] = {wxPoint(X(cx - 9), Y(cy - 8)), wxPoint(X(cx - 9), Y(cy + 8)), wxPoint(X(cx + 9), Y(cy))};
        dc.DrawPolygon(3, tr);
        dc.DrawLine(X(cx + 9), Y(cy - 8), X(cx + 9), Y(cy + 8));
    }
    /// Dioda między dwoma punktami (anoda w (ax,ay), katoda w (kx,ky)): trójkąt i kreska katody na środku odcinka, końce to przewody.
    void DiodaMiedzy(double ax, double ay, double kx, double ky) {
        Pioro(false);
        const double dlug = std::hypot(kx - ax, ky - ay);
        const double ux = (kx - ax) / dlug, uy = (ky - ay) / dlug, nx = -uy, ny = ux;
        const double mx = (ax + kx) / 2, my = (ay + ky) / 2, pol = 7.0, szer = 7.0;
        const double bx = mx - ux * pol, by = my - uy * pol, tx = mx + ux * pol, ty = my + uy * pol;
        dc.DrawLine(X(ax), Y(ay), X(bx), Y(by));
        dc.DrawLine(X(tx), Y(ty), X(kx), Y(ky));
        wxPoint tr[3] = {wxPoint(X(bx + nx * szer), Y(by + ny * szer)), wxPoint(X(bx - nx * szer), Y(by - ny * szer)), wxPoint(X(tx), Y(ty))};
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawPolygon(3, tr);
        dc.DrawLine(X(tx + nx * szer), Y(ty + ny * szer), X(tx - nx * szer), Y(ty - ny * szer));
    }
    /// Linia wymiarowa ze strzałkami na obu końcach (poziomo lub pionowo) i kreskami pomocniczymi.
    void Wymiar(double x1, double y1, double x2, double y2) {
        Pioro(false);
        dc.DrawLine(X(x1), Y(y1), X(x2), Y(y2));
        const double dl = std::hypot(x2 - x1, y2 - y1);
        if (dl < 1) return;
        const double ux = (x2 - x1) / dl, uy = (y2 - y1) / dl, nx = -uy, ny = ux, g = 5, w = 2.2;
        for (int k = 0; k < 2; ++k) {
            const double px = k ? x2 : x1, py = k ? y2 : y1, s = k ? -1 : 1;
            dc.DrawLine(X(px), Y(py), X(px + s * ux * g + nx * w), Y(py + s * uy * g + ny * w));
            dc.DrawLine(X(px), Y(py), X(px + s * ux * g - nx * w), Y(py + s * uy * g - ny * w));
        }
    }
    /// Przekrój zwoju drutu: miedź (wypełnienie) w izolacji (zewnętrzny okrąg), środek (cx,cy), promień zewnętrzny rz.
    void ZwojPrzekroj(double cx, double cy, double rz, bool l) {
        Pioro(l);
        dc.SetBrush(wxBrush(wxColour(205, 127, 70)));
        const double rm = rz * 0.78;
        dc.DrawCircle(X(cx), Y(cy), std::max(1, static_cast<int>(rm * sk)));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawCircle(X(cx), Y(cy), std::max(2, static_cast<int>(rz * sk)));
    }
    /// Ścianka karkasu w przekroju: prostokąt z kreskowaniem ukośnym.
    void Karkas(double x1, double y1, double x2, double y2) {
        Pioro(false);
        dc.SetBrush(wxBrush(wxColour(170, 170, 170), wxBRUSHSTYLE_BDIAGONAL_HATCH));
        dc.DrawRectangle(X(x1), Y(y1), std::max(1, static_cast<int>((x2 - x1) * sk)), std::max(1, static_cast<int>((y2 - y1) * sk)));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
    }
    /// Oś cewki: linia punkt-kreska.
    void Os(double x1, double y, double x2) {
        dc.SetPen(wxPen(kolZwykly, 1, wxPENSTYLE_DOT_DASH));
        dc.DrawLine(X(x1), Y(y), X(x2), Y(y));
    }
    /// Uzwojenie: cztery półokręgi w pionie od (cx, y0) w dół, wysokość h, wybrzuszenie w lewo.
    void Cewka(double cx, double y0, double h, bool l) {
        Pioro(l);
        const double r = h / 8.0;
        for (int i = 0; i < 4; ++i)
            dc.DrawEllipticArc(X(cx - r), Y(y0 + i * 2 * r), static_cast<int>(2 * r * sk), static_cast<int>(2 * r * sk), 90, 270);
    }
};
}  // namespace

PanelSchematu::PanelSchematu(wxWindow* parent, Rodzaj rodzaj)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(250, 210)), rodzaj_(rodzaj) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(240, 200));
    Bind(wxEVT_PAINT, &PanelSchematu::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
}

PanelSchematu* PanelSchematu::Obok(wxPanel* strona, wxSizer* tresc, Rodzaj rodzaj) {
    PanelSchematu* ps = new PanelSchematu(strona, rodzaj);
    wxBoxSizer* poz = new wxBoxSizer(wxHORIZONTAL);
    poz->Add(tresc, 1, wxEXPAND);
    poz->Add(ps, 0, wxALL | wxALIGN_TOP, 8);
    strona->SetSizer(poz);
    return ps;
}

std::vector<std::string> PanelSchematu::ElementyLiczone(Rodzaj r) {
    switch (r) {
        case Rodzaj::Dzielnik: return {"R1", "R2", "Uwy"};
        case Rodzaj::LED: return {"R", "I"};
        case Rodzaj::FiltrRC: return {"R", "C", "fc"};
        case Rodzaj::Zasilacz: case Rodzaj::ZasilaczMostek: case Rodzaj::ZasilaczOdczep: return {"Usek", "C", "Uwy", "Upp"};
        case Rodzaj::Tranzystor: return {"R1", "R2", "RC", "RE", "Ic", "Vce"};
        case Rodzaj::Lampa: return {"Rk", "Ck", "Ua"};
        case Rodzaj::KondensatorESR: return {"ESR", "P"};
        case Rodzaj::LM317: case Rodzaj::LM337: return {"R2", "Uwy"};
        case Rodzaj::CewkaJedno: case Rodzaj::CewkaWiele: return {"L"};
    }
    return {};
}

bool PanelSchematu::Liczony(const std::string& e) const {
    const auto v = ElementyLiczone(rodzaj_);
    return std::find(v.begin(), v.end(), e) != v.end();
}

void PanelSchematu::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    const wxColour tlo = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour tekst = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    const bool ciemny = (tlo.Red() + tlo.Green() + tlo.Blue()) < 3 * 128;
    dc.SetBackground(wxBrush(tlo));
    dc.Clear();
    const wxSize sz = GetClientSize();
    const double sk = std::max(0.5, std::min(sz.x / kSzer, sz.y / kWys));
    Rysownik r{dc, sk, (sz.x - kSzer * sk) / 2, (sz.y - kWys * sk) / 2, tekst,
               ciemny ? wxColour(110, 170, 255) : wxColour(0, 70, 200), 1, 3};
    auto L = [this](const char* e) { return Liczony(e); };

    switch (rodzaj_) {
    case Rodzaj::Dzielnik:
        r.Tekst("Uwe", 8, 20);
        r.Linia(40, 30, 110, 30);
        r.Linia(110, 30, 110, 55);
        r.Rezystor(110, 68, true, "R1", L("R1"));
        r.Linia(110, 81, 110, 100, L("R1"));
        r.Wezel(110, 100);
        r.Linia(110, 100, 190, 100);
        r.Tekst("Uwy", 196, 92, L("Uwy"));
        r.Linia(110, 100, 110, 112);
        r.Rezystor(110, 125, true, "R2", L("R2"));
        r.Linia(110, 138, 110, 160);
        r.Masa(110, 160);
        break;
    case Rodzaj::LED:
        r.Tekst("Uzas", 8, 20);
        r.Linia(40, 30, 110, 30);
        r.Linia(110, 30, 110, 55);
        r.Rezystor(110, 68, true, "R", L("R"));
        r.Linia(110, 81, 110, 110);
        r.DiodaPion(110, 125, true);
        r.Linia(110, 134, 110, 160);
        r.Masa(110, 160);
        r.Tekst("Uf", 128, 120);
        r.Linia(70, 90, 70, 135, L("I"));
        r.Linia(66, 127, 70, 135, L("I"));
        r.Linia(74, 127, 70, 135, L("I"));
        r.Tekst("I", 56, 105, L("I"));
        break;
    case Rodzaj::FiltrRC:
        r.Tekst("Uwe", 8, 82);
        r.Linia(40, 100, 70, 100);
        r.Rezystor(83, 100, false, "R", L("R"));
        r.Linia(96, 100, 150, 100);
        r.Wezel(150, 100);
        r.Linia(150, 100, 150, 118);
        r.Kondensator(150, 122, true, "C", L("C"));
        r.Linia(150, 126, 150, 160);
        r.Masa(150, 160);
        r.Linia(150, 100, 200, 100);
        r.Tekst("Uwy", 205, 92);
        r.Tekst(wxString::FromUTF8("fc = 1 / 2πRC"), 70, 30, L("fc"));
        break;
    case Rodzaj::Zasilacz:
        r.Cewka(40, 60, 80, L("Usek"));
        r.Tekst("Usek", 6, 90, L("Usek"));
        r.Linia(40, 60, 40, 40);
        r.Linia(40, 40, 82, 40);
        r.DiodaPoz(100, 40);
        r.Linia(109, 40, 150, 40);
        r.Wezel(150, 40);
        r.Linia(150, 40, 150, 82);
        r.Kondensator(150, 86, true, "C", L("C"));
        r.Linia(150, 90, 150, 150);
        r.Wezel(150, 150);
        r.Linia(150, 40, 200, 40);
        r.Linia(200, 40, 200, 60);
        r.Rezystor(200, 75, true, "", false);
        r.Tekst("obc.", 206, 68);
        r.Linia(200, 88, 200, 150);
        r.Linia(40, 140, 40, 150);
        r.Linia(40, 150, 200, 150);
        r.Masa(95, 150);
        r.Tekst("Uwy", 160, 20, L("Uwy"));
        r.Tekst("Upp", 160, 168, L("Upp"));
        r.Tekst(TR("jedna połówka"), 62, 18);
        break;
    case Rodzaj::ZasilaczMostek: {
        r.Tekst("mostek", 80, 6);
        r.Cewka(40, 60, 60, L("Usek"));
        r.Tekst("Usek", 2, 82, L("Usek"));
        r.Linia(40, 60, 40, 48);  r.Linia(40, 48, 52, 48);  r.Linia(52, 48, 52, 90);  r.Linia(52, 90, 65, 90);
        r.Linia(40, 120, 40, 148); r.Linia(40, 148, 145, 148); r.Linia(145, 148, 145, 90); r.Linia(145, 90, 125, 90);
        r.DiodaMiedzy(65, 90, 95, 60);     // L -> góra (DC+)
        r.DiodaMiedzy(125, 90, 95, 60);    // P -> góra
        r.DiodaMiedzy(95, 120, 65, 90);    // dół (DC-) -> L
        r.DiodaMiedzy(95, 120, 125, 90);   // dół -> P
        r.Wezel(65, 90); r.Wezel(125, 90); r.Wezel(95, 60);
        r.Masa(95, 120);
        r.Linia(95, 60, 95, 40);  r.Linia(95, 40, 210, 40);
        r.Wezel(170, 40);
        r.Linia(170, 40, 170, 82);
        r.Kondensator(170, 86, true, "C", L("C"));
        r.Linia(170, 90, 170, 110);
        r.Masa(170, 110);
        r.Linia(210, 40, 210, 62);
        r.Rezystor(210, 75, true, "", false);
        r.Tekst("obc.", 198, 126);
        r.Linia(210, 88, 210, 110);
        r.Masa(210, 110);
        r.Tekst("Uwy", 130, 20, L("Uwy"));
        r.Tekst("Upp", 176, 168, L("Upp"));
        break;
    }
    case Rodzaj::ZasilaczOdczep: {
        r.Tekst(TR("dwie połówki (odczep)"), 60, 6);
        r.Cewka(40, 30, 120, L("Usek"));
        r.Tekst("Usek", 2, 84, L("Usek"));
        r.Linia(40, 90, 56, 90);
        r.Masa(56, 90);
        r.Linia(40, 30, 55, 30);  r.DiodaMiedzy(55, 30, 95, 30);  r.Linia(95, 30, 112, 30);
        r.Linia(40, 150, 55, 150); r.DiodaMiedzy(55, 150, 95, 150); r.Linia(95, 150, 112, 150);
        r.Linia(112, 30, 112, 150);
        r.Wezel(112, 90);
        r.Linia(112, 90, 190, 90);
        r.Wezel(150, 90);
        r.Linia(150, 90, 150, 102);
        r.Kondensator(150, 106, true, "C", L("C"));
        r.Linia(150, 110, 150, 124);
        r.Masa(150, 124);
        r.Linia(190, 90, 190, 96);
        r.Rezystor(190, 109, true, "", false);
        r.Tekst("obc.", 199, 102);
        r.Linia(190, 122, 190, 124);
        r.Masa(190, 124);
        r.Tekst("Uwy", 118, 68, L("Uwy"));
        r.Tekst("Upp", 160, 150, L("Upp"));
        break;
    }
    case Rodzaj::Tranzystor:
        r.Linia(30, 20, 200, 20);
        r.Tekst("Vcc", 204, 12);
        r.Linia(60, 20, 60, 45);
        r.Rezystor(60, 58, true, "R1", L("R1"));
        r.Linia(60, 71, 60, 100);
        r.Wezel(60, 100);
        r.Linia(60, 100, 105, 100);
        r.Linia(60, 100, 60, 125);
        r.Rezystor(60, 138, true, "R2", L("R2"));
        r.Linia(60, 151, 60, 168);
        r.Masa(60, 168);
        r.Linia(105, 86, 105, 114);                       // baza
        r.Linia(105, 94, 150, 74);                        // kolektor
        r.Linia(105, 106, 150, 126);                      // emiter
        r.Linia(150, 74, 150, 20);
        r.Rezystor(150, 45, true, "RC", L("RC"));
        r.Linia(150, 126, 150, 150);
        r.Rezystor(150, 138, true, "RE", L("RE"));
        r.Linia(150, 151, 150, 168);
        r.Masa(150, 168);
        r.Tekst("Ic", 168, 70, L("Ic"));
        r.Tekst("Vce", 168, 100, L("Vce"));
        break;
    case Rodzaj::Lampa:
        r.Linia(40, 14, 190, 14);
        r.Tekst("B+", 194, 6);
        r.Linia(110, 14, 110, 26);
        r.Rezystor(110, 39, true, "Ra", false);
        r.Linia(110, 52, 110, 70);
        r.Wezel(110, 62);
        r.Linia(110, 62, 170, 62);
        r.Tekst("Ua", 174, 54, L("Ua"));
        r.Linia(100, 70, 120, 70);                        // anoda
        r.Pioro(false);
        dc.DrawEllipse(r.X(86), r.Y(60), static_cast<int>(48 * sk), static_cast<int>(56 * sk));
        for (int i = 0; i < 4; ++i) r.Linia(97 + i * 7, 88, 101 + i * 7, 88);   // siatka
        r.Linia(60, 88, 96, 88);
        r.Tekst("Ug", 40, 74);
        r.Linia(100, 106, 120, 106);                      // katoda
        r.Linia(110, 106, 110, 125);
        r.Wezel(110, 125);
        r.Linia(110, 125, 110, 130);
        r.Rezystor(110, 143, true, "Rk", L("Rk"));
        r.Linia(110, 156, 110, 170);
        r.Wezel(110, 170);
        r.Masa(110, 170);
        r.Linia(110, 125, 160, 125);
        r.Linia(160, 125, 160, 140);
        r.Kondensator(160, 144, true, "Ck", L("Ck"));
        r.Linia(160, 148, 160, 170);
        r.Linia(110, 170, 160, 170);
        break;
    case Rodzaj::LM317:
    case Rodzaj::LM337: {
        const bool ujemny = rodzaj_ == Rodzaj::LM337;
        r.Tekst(ujemny ? "-Uwe" : "Uwe", 6, 38);
        r.Linia(30, 55, 70, 55);
        r.Pioro(false);
        dc.DrawRectangle(r.X(70), r.Y(38), static_cast<int>(60 * sk), static_cast<int>(44 * sk));
        r.Tekst(ujemny ? "LM337" : "LM317", 77, 42);
        r.Tekst("IN", 72, 58);
        r.Tekst("OUT", 104, 58);
        r.Tekst("ADJ", 86, 84);
        r.Linia(130, 55, 190, 55);
        r.Wezel(160, 55);
        r.Tekst(ujemny ? "-Uwy" : "Uwy", 196, 47, L("Uwy"));
        r.Linia(160, 55, 160, 72);
        r.Rezystor(160, 85, true, "R1", false);
        r.Linia(160, 98, 160, 115);
        r.Linia(100, 82, 100, 115);
        r.Linia(100, 115, 160, 115);
        r.Wezel(160, 115);
        r.Linia(160, 115, 160, 127);
        r.Rezystor(160, 140, true, "R2", L("R2"));
        r.Linia(160, 153, 160, 168);
        r.Masa(160, 168);
        r.Tekst(wxString::FromUTF8(ujemny ? "|Uwy| = 1,25·(1 + R2/R1)" : "Uwy = 1,25·(1 + R2/R1)"), 10, 6);
        break;
    }
    case Rodzaj::CewkaJedno: {
        // przekrój osiowy solenoidu: karkas (kreskowany) i zwoje drutu w jednej warstwie, nad i pod osią
        const double os = 100, rw = 28, gr = 4, rz = 7, skok = 17;
        const int n = 8;
        const double x0 = 52, x1 = x0 + n * skok;
        r.Os(20, os, 232);
        r.Karkas(x0 - 6, os - rw - gr, x1 + 6, os - rw);
        r.Karkas(x0 - 6, os + rw, x1 + 6, os + rw + gr);
        for (int i = 0; i < n; ++i) {
            const double cx = x0 + skok / 2 + i * skok;
            r.ZwojPrzekroj(cx, os - rw - gr - rz, rz, false);
            r.ZwojPrzekroj(cx, os + rw + gr + rz, rz, false);
        }
        // D: średnica karkasu (powierzchnia, na której leży uzwojenie)
        r.Linia(34, os - rw - gr, 50, os - rw - gr);
        r.Linia(34, os + rw + gr, 50, os + rw + gr);
        r.Wymiar(40, os - rw - gr, 40, os + rw + gr);
        r.Tekst("D", 24, os - 26);
        // długość uzwojenia l = N · skok
        r.Linia(x0, os + rw + gr + 2 * rz + 4, x0, 168);
        r.Linia(x1, os + rw + gr + 2 * rz + 4, x1, 168);
        r.Wymiar(x0, 164, x1, 164);
        r.Tekst(wxString::FromUTF8("l = N·skok"), 88, 168);
        // skok zwojów
        r.Linia(x0 + skok / 2, os - rw - gr - 2 * rz - 3, x0 + skok / 2, 40);
        r.Linia(x0 + skok * 1.5, os - rw - gr - 2 * rz - 3, x0 + skok * 1.5, 40);
        r.Wymiar(x0 + skok / 2, 44, x0 + skok * 1.5, 44);
        r.Tekst(TR("skok"), x0 + skok / 2 - 2, 26);
        r.Tekst("L", 208, 56, L("L"));
        r.Tekst(TR("przekrój osiowy"), 70, 6);
        break;
    }
    case Rodzaj::CewkaWiele: {
        // przekrój osiowy cewki wielowarstwowej: karkas z policzkami, zwoje ułożone warstwa na warstwie
        const double os = 96, rw = 24, gr = 4, rz = 6, szer = 6;
        const int warstw = 3;
        const double xl = 80, xp = xl + szer * 2 * rz + 6, wys = warstw * 2 * rz;
        r.Os(20, os, 232);
        r.Karkas(xl - 8, os - rw - gr, xp + 8, os - rw);
        r.Karkas(xl - 8, os + rw, xp + 8, os + rw + gr);
        r.Karkas(xl - 8, os - rw - gr - wys - 6, xl - 4, os - rw - gr);   // policzki
        r.Karkas(xp + 4, os - rw - gr - wys - 6, xp + 8, os - rw - gr);
        r.Karkas(xl - 8, os + rw + gr, xl - 4, os + rw + gr + wys + 6);
        r.Karkas(xp + 4, os + rw + gr, xp + 8, os + rw + gr + wys + 6);
        for (int j = 0; j < warstw; ++j)
            for (int i = 0; i < szer; ++i) {
                const double cx = xl + 3 + rz + i * 2 * rz, dy = rw + gr + rz + j * 2 * rz;
                r.ZwojPrzekroj(cx, os - dy, rz, false);
                r.ZwojPrzekroj(cx, os + dy, rz, false);
            }
        // D
        r.Linia(46, os - rw - gr, 70, os - rw - gr);
        r.Linia(46, os + rw + gr, 70, os + rw + gr);
        r.Wymiar(54, os - rw - gr, 54, os + rw + gr);
        r.Tekst("D", 40, os - 26);
        // b: długość uzwojenia (szerokość pakietu)
        const double ydol = os + rw + gr + wys + 8;
        r.Linia(xl + 3, ydol - 2, xl + 3, ydol + 12);
        r.Linia(xp - 3, ydol - 2, xp - 3, ydol + 12);
        r.Wymiar(xl + 3, ydol + 8, xp - 3, ydol + 8);
        r.Tekst("b", (xl + xp) / 2 - 3, ydol + 11);
        // c: grubość uzwojenia (liczba warstw)
        const double xc = xp + 22;
        r.Linia(xp + 8, os - rw - gr - wys, xc + 5, os - rw - gr - wys);
        r.Linia(xp + 8, os - rw - gr, xc + 5, os - rw - gr);
        r.Wymiar(xc, os - rw - gr - wys, xc, os - rw - gr);
        r.Tekst("c", xc + 7, os - rw - gr - wys / 2 - 7);
        r.Tekst("L", 208, 56, L("L"));
        r.Tekst(TR("przekrój osiowy"), 70, 6);
        break;
    }
    case Rodzaj::KondensatorESR:
        r.Tekst("model rzeczywisty kondensatora", 28, 20);
        r.Linia(20, 100, 50, 100);
        r.Rezystor(63, 100, false, "ESR", L("ESR"));
        r.Linia(76, 100, 100, 100);
        r.Pioro(false);
        for (int i = 0; i < 4; ++i) dc.DrawEllipticArc(r.X(100 + i * 10), r.Y(92), static_cast<int>(10 * sk), static_cast<int>(16 * sk), 0, 180);
        r.Tekst("ESL", 108, 72);
        r.Linia(140, 100, 160, 100);
        r.Kondensator(164, 100, false, "C", false);
        r.Linia(168, 100, 200, 100);
        r.Tekst(wxString::FromUTF8("P = I²·ESR"), 60, 130, L("P"));
        break;
    }
}
