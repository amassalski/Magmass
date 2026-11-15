#include "KoloryMotywu.h"
#include "DialogDlawika.h"
#include "ZestawienieDrutu.h"
#include "../Logika/Dlawik.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <wx/dcbuffer.h>
#include <algorithm>
#include <cmath>
#include <vector>

PanelRdzenia::PanelRdzenia(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(300, 300)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(300, 250));
    Bind(wxEVT_PAINT, &PanelRdzenia::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
}

void PanelRdzenia::Ustaw(double A, double B, double a, double c, double d, double h) {
    if (A > 0 && B > 0) { A_ = A; B_ = B; a_ = a; c_ = c; d_ = d; h_ = h; }
    Refresh();
}

void PanelRdzenia::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    const wxSize rozmiar = GetClientSize();
    const int dx = 18, dy = 12;                  // przesunięcie rzutu ukośnego (głębokość pakietu b)
    const int x0 = 44, y0 = 16;                  // lewy górny róg frontu rdzenia (miejsce na wymiar B po lewej i tylną krawędź u góry)
    const int dolRezerwa = 84;                   // wymiar A i trzy wiersze objaśnień pod rysunkiem
    const double s = std::max(0.5, std::min((rozmiar.x - x0 - dx - 30) / A_, (rozmiar.y - y0 - dolRezerwa) / B_));
    auto X = [&](double v) { return x0 + static_cast<int>(std::lround(v * s)); };
    auto Y = [&](double v) { return y0 + static_cast<int>(std::lround(v * s)); };
    const wxColour zelazo(190, 200, 215), zelazoBok(165, 175, 192);
    dc.SetPen(wxPen(wxColour(40, 40, 40), 1));
    // ścianka górna i prawa (boki pakietu) wypełnione, z liniami blaszek
    const wxPoint gora[4] = {wxPoint(X(0), Y(0)), wxPoint(X(A_), Y(0)), wxPoint(X(A_) + dx, Y(0) - dy), wxPoint(X(0) + dx, Y(0) - dy)};
    const wxPoint prawa[4] = {wxPoint(X(A_), Y(0)), wxPoint(X(A_) + dx, Y(0) - dy), wxPoint(X(A_) + dx, Y(B_) - dy), wxPoint(X(A_), Y(B_))};
    dc.SetBrush(wxBrush(zelazoBok));
    dc.DrawPolygon(4, gora);
    dc.DrawPolygon(4, prawa);
    dc.SetPen(wxPen(wxColour(120, 130, 150), 1));
    const int ile = 8;                           // linie blaszek
    for (int i = 1; i < ile; i++) {
        const int ox = dx * i / ile, oy = dy * i / ile;
        dc.DrawLine(X(0) + ox, Y(0) - oy, X(A_) + ox, Y(0) - oy);                 // krawędzie blaszek na ściance górnej
        dc.DrawLine(X(A_) + ox, Y(0) - oy, X(A_) + ox, Y(B_) - oy);               // i na ściance prawej
    }
    dc.SetPen(wxPen(wxColour(40, 40, 40), 1));
    dc.SetBrush(wxBrush(zelazo));
    dc.DrawRectangle(X(0), Y(0), X(A_) - X(0), Y(B_) - Y(0));
    dc.DrawLine(X(0), Y(0), X(0) + dx, Y(0) - dy);   // lewy skos
    // okna: jarzmo I u góry grubości (B-h)/2 nie jest znane, więc przyjmujemy d u góry, reszta u dołu
    const double yo = d_;                       // początek okien od góry
    const double xl = d_;                       // lewe okno
    const double xr = A_ - d_ - c_;              // prawe okno
    // okna na uzwojenie w rzucie ukośnym: otwór w froncie, a w środku widoczne ścianki (lewa i dolna) o głębokości pakietu
    auto okno = [&](double x1, double x2) {
        const int ax = X(x1), bx = X(x2), ay = Y(yo), by = Y(yo + h_);
        dc.SetPen(wxPen(wxColour(40, 40, 40), 1));
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(ax, ay, bx - ax, by - ay);
        dc.SetBrush(wxBrush(wxColour(150, 160, 178)));
        dc.SetClippingRegion(ax + 1, ay + 1, bx - ax - 1, by - ay - 1);   // ścianki są za frontem, widać je tylko przez otwór
        const wxPoint lewa[4] = {wxPoint(ax, ay), wxPoint(ax, by), wxPoint(ax + dx, by - dy), wxPoint(ax + dx, ay - dy)};
        const wxPoint dolna[4] = {wxPoint(ax, by), wxPoint(bx, by), wxPoint(bx + dx, by - dy), wxPoint(ax + dx, by - dy)};
        dc.DrawPolygon(4, lewa);
        dc.DrawPolygon(4, dolna);
        dc.SetPen(wxPen(wxColour(110, 120, 140), 1));
        for (int i = 1; i < ile; i++) {            // blaszki na ściankach okna
            const int ox = dx * i / ile, oy = dy * i / ile;
            dc.DrawLine(ax + ox, ay - oy, ax + ox, by - oy);
            dc.DrawLine(ax + ox, by - oy, bx + ox, by - oy);
        }
        dc.DestroyClippingRegion();
    };
    okno(xl, xl + c_);
    okno(xr, xr + c_);
    dc.SetPen(wxPen(wxColour(120, 120, 120), 1, wxPENSTYLE_DOT));
    dc.DrawLine(X(0), Y(yo), X(A_), Y(yo));      // styk E i I (szczelina)
    // opisy
    dc.SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    const wxColour kol(190, 0, 0);
    dc.SetTextForeground(kol);
    dc.SetPen(wxPen(kol, 1));
    // grot: pełny, mały trójkąt, którego czubek leży dokładnie na granicy wymiaru; kierunek (ux, uy) wskazuje od wnętrza na zewnątrz
    auto grot = [&](double px, double py, double ux, double uy) {
        const double dl = 7, szer = 3;
        const wxPoint t[3] = {wxPoint(std::lround(px), std::lround(py)),
                              wxPoint(std::lround(px - ux * dl - uy * szer), std::lround(py - uy * dl + ux * szer)),
                              wxPoint(std::lround(px - ux * dl + uy * szer), std::lround(py - uy * dl - ux * szer))};
        dc.SetBrush(wxBrush(kol));
        dc.DrawPolygon(3, t);
    };
    auto strzH = [&](double xa, double xb, int y, const wxString& t, int ty) {
        dc.DrawLine(X(xa), y, X(xb), y);
        grot(X(xa), y, -1, 0);
        grot(X(xb), y, 1, 0);
        wxSize e = dc.GetTextExtent(t);
        dc.DrawText(t, (X(xa) + X(xb)) / 2 - e.x / 2, ty);
    };
    auto strzV = [&](double ya, double yb, int x, const wxString& t, int tx) {
        dc.DrawLine(x, Y(ya), x, Y(yb));
        grot(x, Y(ya), 0, -1);
        grot(x, Y(yb), 0, 1);
        wxSize e = dc.GetTextExtent(t);
        dc.DrawText(t, tx, (Y(ya) + Y(yb)) / 2 - e.y / 2);
    };
    strzH(0, A_, Y(B_) + 14, "A", Y(B_) + 17);
    strzV(0, B_, X(0) - 14, "B", X(0) - 34);
    {   // wymiary d, a i c na jednej wysokości (w górnej części okien)
        const int yw = Y(yo + h_ * 0.2);
        strzH(0, d_, yw, "d", yw - 18);
        strzH(xl + c_, xr, yw, "a", yw - 18);
        strzH(xr, xr + c_, yw, "c", yw - 18);
    }
    strzV(yo, yo + h_, X(xl + c_ / 2), "h", X(xl + c_ / 2) + 4);
    {   // b: głębokość pakietu, na dole obok A (wymiar wzdłuż osi rzutu)
        const int bx = X(A_) + 10, by = Y(B_) + 14;
        dc.DrawLine(bx, by, bx + dx, by - dy);
        const double dl = std::sqrt(double(dx * dx + dy * dy));
        grot(bx, by, -dx / dl, dy / dl);
        grot(bx + dx, by - dy, dx / dl, -dy / dl);
        dc.DrawText("b", bx + dx + 5, by - dy - 8);
    }
    {   // średnia droga strumienia: wystarczy jedna połówka rdzenia (pętla wokół lewego okna), z zaokrąglonymi rogami
        dc.SetPen(wxPen(wxColour(0, 80, 200), 1, wxPENSTYLE_SHORT_DASH));
        const double yg = d_ / 2.0, yd = yo + h_ + (B_ - yo - h_) / 2.0;
        const double xzl = d_ / 2.0, xs = xl + c_ + (xr - xl - c_) / 2.0;
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRoundedRectangle(X(xzl), Y(yg), X(xs) - X(xzl), Y(yd) - Y(yg), std::max(6.0, 0.35 * d_ * s));
        dc.SetTextForeground(wxColour(0, 80, 200));
        dc.SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD));
        dc.DrawText("l", X(xs) - 14, Y(yd) - 20);
    }
    {   // średnia długość zwoju: pętla wokół kolumny środkowej, w połowie grubości uzwojenia (rzut ukośny jak pakiet)
        const wxColour zielony(0, 130, 60);
        dc.SetPen(wxPen(zielony, 1, wxPENSTYLE_SHORT_DASH));
        const int yq = Y(yo + h_ * 0.62);
        const int xa = X(xl + c_ / 2), xb = X(xr + c_ / 2);
        const wxPoint w[4] = {wxPoint(xa, yq), wxPoint(xb, yq), wxPoint(xb + dx, yq - dy), wxPoint(xa + dx, yq - dy)};
        // zaokrąglone rogi: każdy narożnik zastąpiony łukiem (krzywa Béziera drugiego stopnia)
        std::vector<wxPoint> zwoj;
        for (int i = 0; i < 4; i++) {
            const wxPoint pp = w[(i + 3) % 4], pc = w[i], pn = w[(i + 1) % 4];
            auto udzial = [&](const wxPoint& q) {      // zaokrąglenie ok. 8 px, nie więcej niż 30% krawędzi
                const double dl = std::hypot(double(q.x - pc.x), double(q.y - pc.y));
                return dl > 0 ? std::min(0.3, 8.0 / dl) : 0.0;
            };
            const double rp = udzial(pp), rn = udzial(pn);
            const double ax = pc.x + (pp.x - pc.x) * rp, ay = pc.y + (pp.y - pc.y) * rp;
            const double bx2 = pc.x + (pn.x - pc.x) * rn, by2 = pc.y + (pn.y - pc.y) * rn;
            for (int k = 0; k <= 8; k++) {
                const double t = k / 8.0, u = 1 - t;
                zwoj.push_back(wxPoint(std::lround(u * u * ax + 2 * u * t * pc.x + t * t * bx2),
                                       std::lround(u * u * ay + 2 * u * t * pc.y + t * t * by2)));
            }
        }
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawPolygon(static_cast<int>(zwoj.size()), zwoj.data());
        dc.SetTextForeground(zielony);
        dc.SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD));
        dc.DrawText("lzw", (xa + xb) / 2 - 8, yq + 2);
    }
    dc.SetTextForeground(wxColour(60, 60, 60));
    dc.SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.DrawText(TR("b – grubość pakietu (głębokość)"), 4, Y(B_) + 38);
    dc.SetTextForeground(wxColour(0, 80, 200));
    dc.DrawText(TR("l – średnia droga strumienia (jedna połówka)"), 4, Y(B_) + 52);
    dc.SetTextForeground(wxColour(0, 130, 60));
    dc.DrawText(TR("lzw – średnia długość zwoju"), 4, Y(B_) + 66);
}

namespace {
// pola: 0 A, 1 B, 2 a, 3 b, 4 c, 5 d, 6 h, 7 L, 8 I, 9 mi, 10 B_T, 11 J, 12 drut
const char* kNazwy[13] = {"A", "B", "a", "b", "c", "d", "h", "L", "I", "mi", "B_T", "J", "drut"};
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, m)); }
int Idx(const wxString& n) {
    for (int k = 0; k < 13; k++) if (n == kNazwy[k]) return k;
    return -1;
}
using W = Dlawik::Wynik;
struct Poz { const char* klucz; const char* etykieta; int miejsca; double (*f)(const W&); int grupa; };   // grupa: 0 wyniki, 1 rdzeń, 2 uzwojenie
const Poz kPoz[] = {
    {"zalecana", "Zalecana objętość rdzenia [cm³] =", 3, [](const W& o) { return o.zalecanaObjetoscCm3; }, 0},
    {"zwoje", "Liczba zwojów =", 0, [](const W& o) { return double(o.zwoje); }, 0},
    {"drut", "Średnica drutu [mm] =", 2, [](const W& o) { return o.drutGolyMm; }, 0},
    {"szczelina", "Szczelina [mm] =", 3, [](const W& o) { return o.szczelinaMm; }, 0},
    {"przekladka", "Przekładka pod kolumny [mm] =", 3, [](const W& o) { return o.przekladkaMm; }, 0},
    {"szczelina_ecclab", "Szczelina bez reluktancji żelaza [mm] =", 3, [](const W& o) { return o.szczelinaKrizeMm; }, 0},
    {"indukcyjnosc", "Indukcyjność uzyskana [H] =", 3, [](const W& o) { return o.indukcyjnoscRzeczH; }, 0},
    {"rezystancja", "Oporność uzwojenia [om] =", 3, [](const W& o) { return o.opornoscOm; }, 0},
    {"spadek", "Spadek napięcia [V] =", 3, [](const W& o) { return o.spadekV; }, 0},
    {"wypelnienie", "Wypełnienie okna (c x h) w [%] =", 3, [](const W& o) { return o.wypelnieniePct; }, 0},
    {"lfe", "Długość linii strumienia magnetycznego [cm] =", 2, [](const W& o) { return o.dlugoscStrumieniaCm; }, 1},
    {"lzw", "Średnia długość zwoju [cm] =", 2, [](const W& o) { return o.sredniaDlugoscZwojuCm; }, 1},
    {"qfe", "Przekrój czynny rdzenia [cm²] =", 2, [](const W& o) { return o.przekrojCzynnyCm2; }, 1},
    {"vfe", "Objętość czynna rdzenia [cm³] =", 2, [](const W& o) { return o.objetoscCzynnaCm3; }, 1},
    {"drut_gol", "Średnica drutu bez emalii [mm] =", 2, [](const W& o) { return o.drutGolyMm; }, 2},
    {"drut_em", "Średnica drutu w emalii [mm] =", 3, [](const W& o) { return o.drutEmaliaMm; }, 2},
    {"opor_m", "Oporność [om/m] =", 4, [](const W& o) { return o.oporNaMetrOm; }, 2},
    {"zwoje_cm2", "Liczba zwojów mieszcząca się na 1 cm² =", 0, [](const W& o) { return o.zwojeNaCm2; }, 2},
    {"prad", "Prąd dla gęstości [mA] =", 0, [](const W& o) { return o.pradDlaGestosciMa; }, 2},
};
constexpr int kLiczbaPoz = sizeof(kPoz) / sizeof(kPoz[0]);
}

DialogDlawika::DialogDlawika(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Dławik zasilacza"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);

    // --- lewa kolumna: wymiary rdzenia i rysunek obok
    wxStaticBoxSizer* rdz = new wxStaticBoxSizer(wxHORIZONTAL, this, TR("Rdzeń"));
    wxWindow* wr = rdz->GetStaticBox();
    wxBoxSizer* pola = new wxBoxSizer(wxVERTICAL);
    const char* et[7] = {"A [mm] =", "B [mm] =", "a [mm] =", "b [mm] =", "c [mm] =", "d [mm] =", "h [mm] ="};
    wxFlexGridSizer* g = new wxFlexGridSizer(0, 2, 4, 6);
    for (int k = 0; k < 7; k++) {
        g->Add(new wxStaticText(wr, wxID_ANY, et[k]), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[k] = new wxTextCtrl(wr, wxID_ANY, "", wxDefaultPosition, wxSize(70, -1));
        PomocGUI::Dodaj(tc_[k], "dlawik.rdzen");
        g->Add(tc_[k]);
    }
    g->Add(new wxStaticText(wr, wxID_ANY, TR("Kształtka")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chKsztaltka_ = new wxChoice(wr, wxID_ANY);
    for (const char* n : {"EI66", "EI84", "EI96", "EI102", "EI105", "EI120", "EI150"}) chKsztaltka_->Append(n);
    chKsztaltka_->Append(TR("własna"));
    chKsztaltka_->SetSelection(3);
    PomocGUI::Dodaj(chKsztaltka_, "dlawik.rdzen");
    g->Add(chKsztaltka_);
    g->Add(new wxStaticText(wr, wxID_ANY, TR("Grubość blachy [mm]")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chBlacha_ = new wxChoice(wr, wxID_ANY);
    for (const char* n : {"0,35", "0,50"}) chBlacha_->Append(n);
    chBlacha_->SetSelection(1);
    PomocGUI::Dodaj(chBlacha_, "dlawik.rdzen");
    g->Add(chBlacha_);
    pola->Add(g, 0, wxALL, 4);
    rdz->Add(pola, 0);
    rys_ = new PanelRdzenia(wr);
    rdz->Add(rys_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 4);   // góra rysunku = góra pola A, dół = dół pola Grubość blachy
    gora->Add(rdz, 1, wxEXPAND);

    // --- prawa kolumna: parametry i wyniki obliczeń
    wxBoxSizer* prawa = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* par = new wxStaticBoxSizer(wxVERTICAL, this, TR("Parametry dławika"));
    wxWindow* wp = par->GetStaticBox();
    const char* ep[6] = {"Indukcyjność w [H] =", "Natężenie prądu [mA] =", "Przenikalność magn. [Gs/Oe] =", "Indukcja od prądu stałego [T] =",
                         "Gęstość prądu [A/mm²] =", "Średnica drutu [mm] (0 = dobierz) ="};
    const char* dp[6] = {"5", "300", "500", "1", "2,55", "0"};
    wxFlexGridSizer* gp = new wxFlexGridSizer(0, 2, 4, 6);
    for (int k = 0; k < 6; k++) {
        gp->Add(new wxStaticText(wp, wxID_ANY, TR(ep[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[7 + k] = new wxTextCtrl(wp, wxID_ANY, dp[k], wxDefaultPosition, wxSize(70, -1));
        PomocGUI::Dodaj(tc_[7 + k], "dlawik.parametry");
        gp->Add(tc_[7 + k]);
    }
    par->Add(gp, 0, wxALL, 4);
    wxButton* bOblicz = new wxButton(wp, wxID_ANY, TR("Oblicz"));
    par->Add(bOblicz, 0, wxALIGN_CENTER | wxBOTTOM, 4);
    prawa->Add(par, 1, wxEXPAND);

    wxStaticBoxSizer* wyn = new wxStaticBoxSizer(wxVERTICAL, this, TR("Wyniki obliczeń"));
    wxFlexGridSizer* gw = new wxFlexGridSizer(0, 4, 4, 10);   // dwie kolumny par (etykieta, wynik), po pięć wierszy
    wxStaticBoxSizer* sRdz = new wxStaticBoxSizer(wxVERTICAL, this, TR("Rdzeń"));
    wxStaticBoxSizer* sUzw = new wxStaticBoxSizer(wxVERTICAL, this, TR("Uzwojenie"));
    wxFlexGridSizer* gr = new wxFlexGridSizer(0, 2, 4, 8);
    wxFlexGridSizer* gu = new wxFlexGridSizer(0, 2, 4, 8);
    std::vector<wxWindow*> komorkiW(20, nullptr);   // komórki siatki wyników (grupa 0) w kolejności wiersz po wierszu
    for (int k = 0; k < kLiczbaPoz; k++) {
        const Poz& p = kPoz[k];
        wxWindow* rodzic = p.grupa == 0 ? wyn->GetStaticBox() : p.grupa == 1 ? sRdz->GetStaticBox() : sUzw->GetStaticBox();
        wxStaticText* et = new wxStaticText(rodzic, wxID_ANY, TR(p.etykieta));
        lb_[k] = new wxStaticText(rodzic, wxID_ANY, "", wxDefaultPosition, wxSize(70, -1));
        lb_[k]->SetForegroundColour(Kolory::Wynik());
        PomocGUI::Dodaj(lb_[k], "dlawik.wyniki");
        if (p.grupa == 0) {
            // pozycje grupy 0 idą kolumnami: 0-4 w lewej kolumnie, 5-9 w prawej
            const int w = k % 5, kol = k / 5;
            komorkiW[(w * 2 + kol) * 2] = et;
            komorkiW[(w * 2 + kol) * 2 + 1] = lb_[k];
        } else {
            wxFlexGridSizer* cel = p.grupa == 1 ? gr : gu;
            cel->Add(et, 0, wxALIGN_RIGHT);
            cel->Add(lb_[k]);
        }
    }
    for (size_t i = 0; i < komorkiW.size(); ++i) {
        if (!komorkiW[i]) continue;
        if (i % 2 == 0) gw->Add(komorkiW[i], 0, wxALIGN_RIGHT);   // etykieta
        else gw->Add(komorkiW[i]);                                 // wynik
    }
    gw->AddGrowableCol(0, 1);
    gw->AddGrowableCol(2, 1);
    wyn->Add(gw, 1, wxEXPAND | wxALL, 6);
    gora->Add(prawa, 0, wxEXPAND | wxLEFT, 8);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 8);
    glowny->Add(wyn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // --- dół: wyniki rdzenia i uzwojenia, przyciski, suwak średnicy drutu
    wxBoxSizer* dol = new wxBoxSizer(wxHORIZONTAL);
    sRdz->Add(gr, 0, wxALL, 6);
    sUzw->Add(gu, 0, wxALL, 6);
    dol->Add(sRdz, 0, wxEXPAND);
    dol->Add(sUzw, 0, wxEXPAND | wxLEFT, 8);
    wxBoxSizer* przyc = new wxBoxSizer(wxVERTICAL);
    wxButton* bPomoc = new wxButton(this, wxID_ANY, TR("Pomoc"));
    przyc->Add(bPomoc, 0, wxEXPAND | wxBOTTOM, 6);
    przyc->Add(new wxButton(this, wxID_CLOSE, TR("Zamknij")), 0, wxEXPAND);
    dol->Add(przyc, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 10);
    glowny->Add(dol, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    wxBoxSizer* sz = new wxBoxSizer(wxHORIZONTAL);
    sz->Add(new wxStaticText(this, wxID_ANY, "0,05 mm"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    suwak_ = new wxSlider(this, wxID_ANY, 39, 5, 200, wxDefaultPosition, wxSize(360, -1));
    PomocGUI::Dodaj(suwak_, "dlawik.suwak");
    sz->Add(suwak_, 1, wxEXPAND);
    sz->Add(new wxStaticText(this, wxID_ANY, "2,00 mm"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    glowny->Add(new wxStaticText(this, wxID_ANY, TR("Średnica drutu nawojowego")), 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 8);
    glowny->Add(sz, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    lbUwagi_ = new wxStaticText(this, wxID_ANY, "");
    lbUwagi_->SetForegroundColour(Kolory::Ostrzezenie());
    glowny->Add(lbUwagi_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    tcDrut_ = ZestawienieDrutu::Dodaj(this, glowny, "drut.zestawienie");

    SetSizerAndFit(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    bOblicz->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Oblicz(); });
    bPomoc->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Pomoc(); });
    suwak_->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) { tc_[12]->SetValue(L(suwak_->GetValue() / 100.0, 2)); });
    chKsztaltka_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { ZKsztaltki(); });
    chBlacha_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    for (int k = 0; k < kPol; k++)
        tc_[k]->Bind(wxEVT_TEXT, [this, k](wxCommandEvent&) {
            if (!wypelniam_ && k < 7 && chKsztaltka_->GetSelection() != static_cast<int>(chKsztaltka_->GetCount()) - 1)
                chKsztaltka_->SetSelection(chKsztaltka_->GetCount() - 1);
            Przelicz();
        });
    tc_[3]->ChangeValue("51");
    ZKsztaltki();
}

void DialogDlawika::ZKsztaltki() {
    const wxString n = chKsztaltka_->GetStringSelection();
    double pakiet = 0;
    tekstNaLiczbe(tc_[3]->GetValue().utf8_string(), pakiet);
    if (auto r = Dlawik::ksztaltkaZNazwy(n.utf8_string(), pakiet > 0 ? pakiet : 51)) {
        wypelniam_ = true;
        const double v[7] = {r->A, r->B, r->a, r->b, r->c, r->d, r->h};
        for (int k = 0; k < 7; k++) tc_[k]->ChangeValue(L(v[k], 0).BeforeFirst(','));
        wypelniam_ = false;
    }
    Przelicz();
}

void DialogDlawika::UstawKsztaltke(const wxString& nazwa) {
    const int i = chKsztaltka_->FindString(nazwa);
    if (i != wxNOT_FOUND) chKsztaltka_->SetSelection(i);
    ZKsztaltki();
}
void DialogDlawika::UstawPole(const wxString& n, const wxString& v) {
    if (int k = Idx(n); k >= 0) tc_[k]->SetValue(v);
}
wxString DialogDlawika::Wynik(const wxString& n) const {
    for (int k = 0; k < kLiczbaPoz; k++) if (n == kPoz[k].klucz) return wyn_[k];
    return wxString();
}

void DialogDlawika::Przelicz() {
    double v[kPol] = {};
    bool ok = true;
    for (int k = 0; k < kPol; k++)
        if (!tekstNaLiczbe(tc_[k]->GetValue().utf8_string(), v[k])) ok = false;
    rys_->Ustaw(v[0], v[1], v[2], v[4], v[5], v[6]);
    for (wxString& w : wyn_) w = "";
    for (wxStaticText* l : lb_) if (l) l->SetLabel("");
    tresc_ = "";
    if (tcDrut_) tcDrut_->ChangeValue("");
    szczelinaTekst_ = "";
    bezSzczelinyTekst_ = "";
    Dlawik::Wejscie w;
    w.rdzen = {v[0], v[1], v[2], v[3], v[4], v[5], v[6], chBlacha_->GetSelection() == 0 ? 0.35 : 0.5};
    w.indukcyjnoscH = v[7]; w.pradMa = v[8]; w.mi = v[9]; w.bMaxT = v[10]; w.gestoscAmm2 = v[11]; w.drutMm = v[12];
    auto o = ok ? Dlawik::oblicz(w) : std::nullopt;
    if (!o) {
        lbUwagi_->SetLabel(TR("Uwagi: uzupełnij pola liczbami większymi od zera (średnica drutu może być 0)."));
        return;
    }
    for (int k = 0; k < kLiczbaPoz; k++) { wyn_[k] = L(kPoz[k].f(*o), kPoz[k].miejsca); lb_[k]->SetLabel(wyn_[k]); }
    tcDrut_->ChangeValue(ZestawienieDrutu::Tekst(MasaDrutu::zestawienie(MasaDrutu::pozycje(*o))));
    suwak_->SetValue(std::clamp(static_cast<int>(std::lround(o->drutGolyMm * 100)), 5, 200));
    {   // indukcja w rdzeniu bez szczeliny: B = μ0·μ·N·I / lFe (μ w Gs/Oe = μr)
        const double bBez = 4e-7 * 3.14159265358979323846 * w.mi * o->zwoje * (w.pradMa * 1e-3) / (o->dlugoscStrumieniaCm * 1e-2);
        bezSzczelinyTekst_ = TR("Dla Twoich danych: indukcja w rdzeniu bez szczeliny wyniosłaby ") + L(bBez, 2) + " T " + TR("przy dopuszczalnej ") + L(w.bMaxT, 2) + " T, " +
                             (bBez > w.bMaxT ? TR("więc szczelina jest potrzebna.") : TR("więc szczelina nie jest potrzebna."));
    }
    szczelinaTekst_ = L(o->szczelinaMm, 3) + " (" + TR("bez reluktancji żelaza") + " " + L(o->szczelinaKrizeMm, 3) + ")";
    if (o->ostrzezenie == Dlawik::Wynik::Ostrzezenie::zaDuzoWypelnione)
        tresc_ = TR("Obliczone uzwojenie zajmuje ponad 75% okna rdzenia. Należy zmniejszyć wypełnienie okna rdzenia poniżej tej wartości. W tym celu powinno się:\n"
                    "- zmniejszyć objętość uzwojenia przez zastosowanie drutu nawojowego o mniejszej średnicy,\n"
                    "- zmniejszyć indukcję dławika, co wpłynie na zmniejszenie liczby zwojów,\n"
                    "- zwiększyć rozmiar kształtki rdzenia.");
    else if (o->ostrzezenie == Dlawik::Wynik::Ostrzezenie::zaMaloWypelnione)
        tresc_ = TR("Obliczone uzwojenie zajmuje mniej niż 50% okna rdzenia, rdzeń nie jest optymalnie wypełniony. Należy zwiększyć wypełnienie okna rdzenia powyżej tej wartości. W tym celu powinno się:\n"
                    "- zwiększyć objętość uzwojenia przez zastosowanie drutu nawojowego o większej średnicy,\n"
                    "- zwiększyć indukcję dławika, co wpłynie na zwiększenie liczby zwojów,\n"
                    "- zmniejszyć rozmiar kształtki rdzenia.");
    lbUwagi_->SetLabel(tresc_.empty() ? wxString() : (o->ostrzezenie == Dlawik::Wynik::Ostrzezenie::zaDuzoWypelnione
                       ? TR("Uwaga: uzwojenie zajmuje ponad 75% okna - nie zmieści się.")
                       : TR("Uwaga: uzwojenie zajmuje mniej niż 50% okna - rdzeń nie jest optymalnie wypełniony.")));
}

void DialogDlawika::Oblicz() {
    Przelicz();
    if (!tresc_.empty()) wxMessageBox(tresc_, TR("Uwaga !!!!"), wxOK | wxICON_WARNING, this);
}

void DialogDlawika::Pomoc() {
    wxDialog dlg(this, wxID_ANY, TR("Dławik - pomoc"));
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxStaticText* t = new wxStaticText(&dlg, wxID_ANY, TR(
        "Obliczone uzwojenie należy sprawdzić pod względem zajmowanego miejsca w oknie rdzenia. Karkas, na którym znajduje się uzwojenie, zmniejsza przekrój okna rdzenia (c x h) o około 25...30%. Optymalne wypełnienie występuje pomiędzy około 50...75%.\n\n"
        "W przypadku gdy obliczone uzwojenie zajmuje ponad 75%, należy zmniejszyć wypełnienie okna rdzenia poniżej tej wartości. W tym celu powinno się:\n"
        "- zmniejszyć objętość uzwojenia przez zastosowanie drutu nawojowego o mniejszej średnicy,\n"
        "- zmniejszyć indukcję dławika, co wpłynie na zmniejszenie liczby zwojów,\n"
        "- zwiększyć rozmiar kształtki rdzenia.\n\n"
        "W przeciwnym wypadku, gdy uzwojenie zajmuje mniej niż 50%, należy zwiększyć wypełnienie okna rdzenia powyżej tej wartości. W tym celu powinno się:\n"
        "- zwiększyć objętość uzwojenia przez zastosowanie drutu nawojowego o większej średnicy,\n"
        "- zwiększyć indukcję dławika, co wpłynie na zwiększenie liczby zwojów,\n"
        "- zmniejszyć rozmiar kształtki rdzenia.\n\n"
        "Chcąc zapobiec nasyceniu dławika spowodowanemu zbyt silnym prądem przepływającym przez uzwojenie, stosuje się szczelinę z materiału niemagnetycznego między kształtkami rdzenia E oraz I. Szczelina nie jest konieczna, gdy indukcja od prądu stałego w rdzeniu bez szczeliny, B = μ0·μ·N·I / lFe, nie przekracza dopuszczalnej indukcji (pole Indukcja max, zwykle 0,7...0,85 T); to znaczy, gdy iloczyn N·I (amperozwoje) jest dość mały."));
    t->Wrap(480);
    s->Add(t, 0, wxALL, 12);
    if (!bezSzczelinyTekst_.empty()) {
        wxStaticText* tb = new wxStaticText(&dlg, wxID_ANY, bezSzczelinyTekst_);
        tb->Wrap(480);
        s->Add(tb, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }
    s->Add(new wxStaticText(&dlg, wxID_ANY, TR("Szczelina [mm] = ") + szczelinaTekst_), 0, wxALIGN_CENTER | wxBOTTOM, 10);
    s->Add(dlg.CreateStdDialogButtonSizer(wxOK), 0, wxALIGN_CENTER | wxBOTTOM, 10);
    dlg.SetSizerAndFit(s);
    dlg.ShowModal();
}
