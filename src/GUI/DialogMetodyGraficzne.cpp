#include "KoloryMotywu.h"
#include "DialogMetodyGraficzne.h"
#include "../Logika/Znieksztalcenia.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <wx/dcbuffer.h>
#include <wx/filedlg.h>
#include <wx/statline.h>

#include <algorithm>
#include <cmath>

using namespace MetodyGraficzne;

namespace {

wxString L(double v, int miejsc = 2) { return TekstZUTF8(liczbaNaTekstPolski(v, miejsc, miejsc)); }

std::optional<double> Liczba(const wxTextCtrl* tc) {
    double v = 0;
    if (!tekstNaLiczbe(tc->GetValue().utf8_string(), v)) return std::nullopt;
    return v;
}

wxTextCtrl* Pole(wxWindow* p, wxSizer* s, const wxString& wartosc, const char* klucz, int szer = 62) {
    wxTextCtrl* t = new wxTextCtrl(p, wxID_ANY, wartosc, wxDefaultPosition, wxSize(szer, -1));
    PomocGUI::DodajPole(t, klucz);
    s->Add(t, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    return t;
}

}  // namespace

// ================================================================ płótno

PlotCharakterystyk::PlotCharakterystyk(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(820, 640), wxWANTS_CHARS) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(480, 360));
    Bind(wxEVT_PAINT, &PlotCharakterystyk::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Przelicz(); Refresh(); e.Skip(); });
    Bind(wxEVT_LEFT_DCLICK, &PlotCharakterystyk::OnDwuklik, this);
    for (auto ev : {wxEVT_LEFT_DOWN, wxEVT_LEFT_UP, wxEVT_MOTION, wxEVT_LEAVE_WINDOW})
        Bind(ev, &PlotCharakterystyk::OnMysz, this);
}

bool PlotCharakterystyk::UstawObraz(const wxImage& obraz) {
    if (!obraz.IsOk() || obraz.GetWidth() < 20 || obraz.GetHeight() < 20) return false;
    obraz_ = obraz;
    for (bool& u : ustawiony_) u = false;
    kal_.reset();
    maProste_ = false;
    Przelicz();
    Refresh();
    if (przyZmianie) przyZmianie();
    return true;
}

void PlotCharakterystyk::Przelicz() {
    if (!obraz_.IsOk()) return;
    const wxSize sz = GetClientSize();
    if (sz.x < 10 || sz.y < 10) return;
    skala_ = std::min(sz.x / static_cast<double>(obraz_.GetWidth()), sz.y / static_cast<double>(obraz_.GetHeight()));
    const int w = std::max(1, static_cast<int>(obraz_.GetWidth() * skala_)), h = std::max(1, static_cast<int>(obraz_.GetHeight() * skala_));
    offx_ = (sz.x - w) / 2.0;
    offy_ = (sz.y - h) / 2.0;
    if (bmpRozmiar_ != wxSize(w, h) || !bmp_.IsOk()) {
        bmp_ = wxBitmap(obraz_.Scale(w, h, wxIMAGE_QUALITY_BILINEAR));
        bmpRozmiar_ = wxSize(w, h);
    }
}

wxPoint PlotCharakterystyk::NaEkran(PunktPx p) const {
    return wxPoint(static_cast<int>(offx_ + p.x * skala_ + 0.5), static_cast<int>(offy_ + p.y * skala_ + 0.5));
}
PunktPx PlotCharakterystyk::NaObraz(const wxPoint& e) const { return {(e.x - offx_) / skala_, (e.y - offy_) / skala_}; }

void PlotCharakterystyk::UstawPunktKalibracji(int krok, double ix, double iy) {
    if (krok < 0 || krok > 2) return;
    pkt_[krok] = {ix, iy};
    ustawiony_[krok] = true;
    Refresh();
    if (przyZmianie) przyZmianie();
}

bool PlotCharakterystyk::Skaluj(double napiecieV, double pradMa) {
    if (!ustawiony_[0] || !ustawiony_[1] || !ustawiony_[2]) return false;
    auto k = kalibruj(pkt_[0], pkt_[1], napiecieV, pkt_[2], pradMa);
    if (!k) { kal_.reset(); Refresh(); return false; }
    const bool nowa = !kal_.has_value();
    kal_ = k;
    if (!maProste_ || nowa) {
        const double umax = (obraz_.GetWidth() - k->zero.x) * k->voltNaPx, imax = k->zero.y * k->mANaPx;
        prosta_ = {{0, 0.6 * imax}, {0.6 * umax, 0}};
        maProste_ = true;
    }
    Refresh();
    return true;
}

void PlotCharakterystyk::UstawKoniec(Koniec k, PunktUI cel) {
    if (!maProste_) return;
    prosta_ = przesun(prosta_, k, cel, blok_);
    Refresh();
    if (przyZmianie) przyZmianie();
}

size_t PlotCharakterystyk::PunktowPaMax() const {
    if (!kal_ || !(paMax_ > 0)) return 0;
    const double umax = (obraz_.GetWidth() - kal_->zero.x) * kal_->voltNaPx;
    return krzywaPaMax(paMax_, std::max(0.5, umax / 400), umax, 200).size();
}

void PlotCharakterystyk::OnMysz(wxMouseEvent& e) {
    if (!obraz_.IsOk()) return;
    const PunktPx px = NaObraz(e.GetPosition());
    if (e.GetEventType() == wxEVT_LEAVE_WINDOW) {
        if (przyRuchuKursora) przyRuchuKursora(std::nullopt);
        return;
    }
    if (kal_ && przyRuchuKursora) przyRuchuKursora(kal_->wartosc(px));
    if (e.GetEventType() == wxEVT_LEFT_DOWN) {
        if (!wyniki_) {
            UstawPunktKalibracji(aktywny_, px.x, px.y);
            return;
        }
        if (kal_ && maProste_) {
            przeciagany_ = -1;
            double najblizszy = 12;   // piksele ekranu
            for (int k = 0; k < 2; k++) {
                const wxPoint ep = NaEkran(kal_->piksel(k == 0 ? prosta_.p1 : prosta_.p2));
                const double d = std::hypot(ep.x - e.GetX(), ep.y - e.GetY());
                if (d < najblizszy) { najblizszy = d; przeciagany_ = k; }
            }
            if (przeciagany_ >= 0) CaptureMouse();
        }
    } else if (e.GetEventType() == wxEVT_MOTION && przeciagany_ >= 0 && kal_) {
        UstawKoniec(przeciagany_ == 0 ? Koniec::Pradowy : Koniec::Napieciowy, kal_->wartosc(px));
    } else if (e.GetEventType() == wxEVT_LEFT_UP && przeciagany_ >= 0) {
        przeciagany_ = -1;
        if (HasCapture()) ReleaseMouse();
    }
}

void PlotCharakterystyk::OnDwuklik(wxMouseEvent& e) {
    if (!wyniki_ || !kal_ || !maProste_ || !przyDwukliku) return;
    przyDwukliku(kal_->wartosc(NaObraz(e.GetPosition())));
}

void PlotCharakterystyk::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
    dc.Clear();
    if (!obraz_.IsOk()) {
        dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
        dc.DrawText(TR("Wczytaj obraz charakterystyki anodowej lampy (przycisk „Wczytaj obraz...”)."), 20, 20);
        return;
    }
    if (!bmp_.IsOk()) Przelicz();
    dc.DrawBitmap(bmp_, static_cast<int>(offx_), static_cast<int>(offy_));
    const wxSize sz = wxSize(obraz_.GetWidth(), obraz_.GetHeight());
    wxFont f = GetFont();
    f.SetPointSize(8);
    dc.SetFont(f);

    // punkty kalibracji
    const wxColour kolKal[3] = {wxColour(0, 90, 220), wxColour(0, 150, 0), wxColour(220, 0, 0)};
    const wxString nazwy[3] = {"(0,0)", "Ua", "Ia"};
    for (int k = 0; k < 3; k++) {
        if (!ustawiony_[k]) continue;
        const wxPoint p = NaEkran(pkt_[k]);
        dc.SetPen(wxPen(kolKal[k], 2));
        dc.DrawLine(p.x - 8, p.y, p.x + 9, p.y);
        dc.DrawLine(p.x, p.y - 8, p.x, p.y + 9);
        dc.SetTextForeground(kolKal[k]);
        dc.DrawText(nazwy[k], p.x + 6, p.y + 4);
    }
    if (!kal_) return;

    const double umax = (sz.x - kal_->zero.x) * kal_->voltNaPx, imax = kal_->zero.y * kal_->mANaPx;
    auto linia = [&](PunktUI a, PunktUI b) {
        const wxPoint pa = NaEkran(kal_->piksel(a)), pb = NaEkran(kal_->piksel(b));
        dc.DrawLine(pa, pb);
    };
    // siatka kontrolna
    if (siatkaI_ > 0 || siatkaU_ > 0) {
        dc.SetPen(wxPen(wxColour(200, 0, 160), 1, wxPENSTYLE_DOT));
        dc.SetTextForeground(wxColour(200, 0, 160));
        if (siatkaI_ > 0 && imax / siatkaI_ < 400)
            for (double i = siatkaI_; i <= imax; i += siatkaI_) {
                linia({0, i}, {umax, i});
                const wxPoint p = NaEkran(kal_->piksel({0, i}));
                dc.DrawText(L(i, 0), p.x + 2, p.y - 12);
            }
        if (siatkaU_ > 0 && umax / siatkaU_ < 400)
            for (double u = siatkaU_; u <= umax; u += siatkaU_) {
                linia({u, 0}, {u, imax});
                const wxPoint p = NaEkran(kal_->piksel({u, 0}));
                dc.DrawText(L(u, 0), p.x + 2, p.y - 14);
            }
    }
    // krzywa Pa max (niebieska)
    if (paMax_ > 0) {
        const auto k = krzywaPaMax(paMax_, std::max(0.5, umax / 400), umax, 200);
        dc.SetPen(wxPen(wxColour(0, 0, 255), 2));
        for (size_t n = 1; n < k.size(); n++) {
            if (k[n - 1].i > imax * 1.02 && k[n].i > imax * 1.02) continue;
            linia(k[n - 1], k[n]);
        }
    }
    // prosta obciążenia
    if (maProste_) {
        dc.SetPen(wxPen(wxColour(0, 0, 0), 1));
        linia(prosta_.p1, prosta_.p2);
        const wxPoint p1 = NaEkran(kal_->piksel(prosta_.p1)), p2 = NaEkran(kal_->piksel(prosta_.p2));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour(220, 0, 0), 2));
        dc.DrawRectangle(p1.x - 5, p1.y - 5, 10, 10);
        dc.SetPen(wxPen(wxColour(0, 150, 0), 2));
        dc.DrawRectangle(p2.x - 5, p2.y - 5, 10, 10);
    }
}

// ================================================================ okno

DialogMetodyGraficzne::DialogMetodyGraficzne(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Metody graficzne"), wxDefaultPosition, wxSize(1180, 780),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX) {
    wxBoxSizer* glowny = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* lewy = new wxBoxSizer(wxVERTICAL);

    wxButton* wczytaj = new wxButton(this, wxID_ANY, TR("Wczytaj obraz..."));
    PomocGUI::Dodaj(wczytaj, "grafika.obraz");
    wczytaj->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { WczytajObraz(); });
    lewy->Add(wczytaj, 0, wxEXPAND | wxALL, 6);

    wxStaticBoxSizer* opcje = new wxStaticBoxSizer(wxVERTICAL, this, TR("Opcje wyświetlania"));
    bSkalowanie_ = new wxToggleButton(this, wxID_ANY, TR("Skalowanie charakterystyk"));
    bWyniki_ = new wxToggleButton(this, wxID_ANY, TR("Wyniki i wyznaczanie Ra"));
    PomocGUI::Dodaj(bSkalowanie_, "grafika.tryb");
    PomocGUI::Dodaj(bWyniki_, "grafika.tryb");
    opcje->Add(bSkalowanie_, 0, wxEXPAND | wxALL, 3);
    opcje->Add(bWyniki_, 0, wxEXPAND | wxALL, 3);
    lewy->Add(opcje, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
    bSkalowanie_->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent&) { PokazStrone(0); });
    bWyniki_->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent&) { PokazStrone(1); });

    ksiazka_ = new wxSimplebook(this);
    ksiazka_->AddPage(StronaSkalowania(ksiazka_), "");
    ksiazka_->AddPage(StronaWynikow(ksiazka_), "");
    lewy->Add(ksiazka_, 1, wxEXPAND | wxALL, 6);
    lewy->Add(new wxButton(this, wxID_CLOSE, TR("Zamknij")), 0, wxALIGN_RIGHT | wxALL, 6);
    glowny->Add(lewy, 0, wxEXPAND);

    plot_ = new PlotCharakterystyk(this);
    glowny->Add(plot_, 1, wxEXPAND | wxALL, 6);
    SetSizer(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    plot_->przyZmianie = [this] { Odswiez(); };
    plot_->przyDwukliku = [this](PunktUI w) { Dwuklik(w.u); };
    plot_->przyRuchuKursora = [this](std::optional<PunktUI> w) {
        lbKursor_->SetLabel(w ? "U = " + (w->u >= 0 ? L(w->u, 1) : wxString("----")) + " V     I = " + L(w->i, 1) + " mA" : wxString());
    };
    PokazStrone(0);
    Odswiez();
}

wxPanel* DialogMetodyGraficzne::StronaSkalowania(wxWindow* rodzic) {
    wxPanel* p = new wxPanel(rodzic);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxStaticText* opis = new wxStaticText(p, wxID_ANY,
        TR("Wybierz krok i kliknij na obrazie: 1) punkt (0,0), czyli przecięcie osi; 2) punkt na osi napięcia o znanej wartości; "
           "3) punkt na osi prądu o znanej wartości. Wpisz te wartości i naciśnij „Skaluj”."));
    opis->Wrap(250);
    s->Add(opis, 0, wxALL, 6);
    const char* tytuly[3] = {"1. Punkt (0,0)", "2. Punkt na osi Ua", "3. Punkt na osi Ia"};
    for (int k = 0; k < 3; k++) {
        wxBoxSizer* r = new wxBoxSizer(wxHORIZONTAL);
        krokBtn_[k] = new wxToggleButton(p, wxID_ANY, TR(tytuly[k]), wxDefaultPosition, wxSize(140, -1));
        PomocGUI::DodajPole(krokBtn_[k], std::string("grafika.krok.") + std::to_string(k + 1));
        r->Add(krokBtn_[k], 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        if (k == 1) { tcU_ = Pole(p, r, "300", "grafika.wartosc.napiecie"); r->Add(new wxStaticText(p, wxID_ANY, "V"), 0, wxALIGN_CENTER_VERTICAL); }
        if (k == 2) { tcI_ = Pole(p, r, "300", "grafika.wartosc.prad"); r->Add(new wxStaticText(p, wxID_ANY, "mA"), 0, wxALIGN_CENTER_VERTICAL); }
        s->Add(r, 0, wxLEFT | wxRIGHT | wxTOP, 6);
        lbKrok_[k] = new wxStaticText(p, wxID_ANY, "");
        s->Add(lbKrok_[k], 0, wxLEFT, 12);
        krokBtn_[k]->Bind(wxEVT_TOGGLEBUTTON, [this, k](wxCommandEvent&) {
            for (int j = 0; j < 3; j++) krokBtn_[j]->SetValue(j == k);
            plot_->UstawAktywnyKrok(k);
        });
    }
    krokBtn_[0]->SetValue(true);
    wxButton* sk = new wxButton(p, wxID_ANY, TR("Skaluj"));
    PomocGUI::Dodaj(sk, "grafika.skaluj");
    sk->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PrzeliczSkale(); });
    s->Add(sk, 0, wxALL, 6);
    lbSkala_ = new wxStaticText(p, wxID_ANY, "");
    lbSkala_->Wrap(250);
    s->Add(lbSkala_, 0, wxALL, 6);
    p->SetSizer(s);
    return p;
}

wxPanel* DialogMetodyGraficzne::StronaWynikow(wxWindow* rodzic) {
    wxPanel* p = new wxPanel(rodzic);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* wyn = new wxStaticBoxSizer(wxVERTICAL, p, TR("Wyniki"));
    wxWindow* w = wyn->GetStaticBox();
    auto lb = [&](const wxString& etykieta, bool pogrubione = false) {
        wxBoxSizer* r = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText* e = new wxStaticText(w, wxID_ANY, etykieta, wxDefaultPosition, wxSize(142, -1));
        wxStaticText* v = new wxStaticText(w, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
        if (pogrubione) { wxFont f = v->GetFont(); f.SetWeight(wxFONTWEIGHT_BOLD); v->SetFont(f); e->SetFont(f); }
        r->Add(e, 0, wxRIGHT, 4);
        r->Add(v, 1);
        wyn->Add(r, 0, wxEXPAND | wxALL, 2);
        return v;
    };
    lbKursor_ = new wxStaticText(w, wxID_ANY, "");
    wyn->Add(lbKursor_, 0, wxALL, 3);
    lbP1_ = lb(TR("Koniec prądowy"));
    lbP2_ = lb(TR("Koniec napięciowy"));
    lbDu_ = lb(wxString::FromUTF8("\xCE\x94Ua [V]"));
    lbDi_ = lb(wxString::FromUTF8("\xCE\x94Ia [mA]"));
    lbRa_ = lb(TR("Ra klasa A [kΩ]"), true);
    lbRaa_ = lb(TR("Raa klasa B [kΩ]"), true);
    lbSE_ = lb(TR("Moc SE klasa A [W]"), true);
    lbPP_ = lb(TR("Moc PP klasa B [W]"), true);
    for (wxStaticText* t : {lbP1_, lbP2_, lbDu_, lbDi_, lbRa_, lbRaa_, lbSE_, lbPP_}) PomocGUI::Dodaj(t, "grafika.wyniki");
    s->Add(wyn, 0, wxEXPAND);

    wxStaticBoxSizer* bl = new wxStaticBoxSizer(wxHORIZONTAL, p, TR("Zablokuj"));
    cbRa_ = new wxCheckBox(bl->GetStaticBox(), wxID_ANY, TR("Wartość Ra"));
    cbAmp_ = new wxCheckBox(bl->GetStaticBox(), wxID_ANY, TR("Amplitudę (ΔIa)"));
    PomocGUI::DodajPole(cbRa_, "grafika.blokady.ra");
    PomocGUI::DodajPole(cbAmp_, "grafika.blokady.amplituda");
    bl->Add(cbRa_, 0, wxALL, 4);
    bl->Add(cbAmp_, 0, wxALL, 4);
    s->Add(bl, 0, wxEXPAND | wxTOP, 6);
    for (wxCheckBox* c : {cbRa_, cbAmp_}) c->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { UstawBlokady(cbRa_->GetValue(), cbAmp_->GetValue()); });

    wxStaticBoxSizer* os = new wxStaticBoxSizer(wxVERTICAL, p, TR("Osie napięcia i natężenia (siatka kontrolna)"));
    wxWindow* wo = os->GetStaticBox();
    wxBoxSizer* r = new wxBoxSizer(wxHORIZONTAL);
    cbSiatkaI_ = new wxCheckBox(wo, wxID_ANY, "Ia");
    tcSiatkaI_ = Pole(wo, r, "50", "grafika.siatka.ia_krok");
    r->Insert(0, cbSiatkaI_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    r->Add(new wxStaticText(wo, wxID_ANY, "mA"), 0, wxALIGN_CENTER_VERTICAL);
    os->Add(r, 0, wxALL, 3);
    r = new wxBoxSizer(wxHORIZONTAL);
    cbSiatkaU_ = new wxCheckBox(wo, wxID_ANY, "Ua");
    tcSiatkaU_ = Pole(wo, r, "60", "grafika.siatka.ua_krok");
    r->Insert(0, cbSiatkaU_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    r->Add(new wxStaticText(wo, wxID_ANY, "V"), 0, wxALIGN_CENTER_VERTICAL);
    os->Add(r, 0, wxALL, 3);
    PomocGUI::DodajPole(cbSiatkaI_, "grafika.siatka.ia");
    PomocGUI::DodajPole(cbSiatkaU_, "grafika.siatka.ua");
    s->Add(os, 0, wxEXPAND | wxTOP, 6);

    wxStaticBoxSizer* pm = new wxStaticBoxSizer(wxHORIZONTAL, p, TR("Moc strat w anodzie Pa max"));
    wxWindow* wp = pm->GetStaticBox();
    cbPa_ = new wxCheckBox(wp, wxID_ANY, TR("Rysuj"));
    PomocGUI::DodajPole(cbPa_, "grafika.pamax");
    pm->Add(cbPa_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    tcPa_ = Pole(wp, pm, "25", "grafika.pamax.moc");
    pm->Add(new wxStaticText(wp, wxID_ANY, "W"), 0, wxALIGN_CENTER_VERTICAL);
    s->Add(pm, 0, wxEXPAND | wxTOP, 6);

    wxStaticBoxSizer* zn = new wxStaticBoxSizer(wxVERTICAL, p, TR("Zniekształcenia (pięć rzędnych)"));
    wxWindow* wz = zn->GetStaticBox();
    zn->Add(new wxStaticText(wz, wxID_ANY, TR("Dwuklik na prostej w miejscu przecięcia z krzywą Ug ustawia kolejny punkt; skrajne punkty to końce prostej.")), 0, wxALL, 3);
    const char* np[3] = {"Ia1", "Ia0", "Ia2"};
    for (int k = 0; k < 3; k++) {
        wxBoxSizer* r = new wxBoxSizer(wxHORIZONTAL);
        rbPkt_[k] = new wxRadioButton(wz, wxID_ANY, np[k], wxDefaultPosition, wxSize(60, -1), k == 0 ? wxRB_GROUP : 0);
        PomocGUI::DodajPole(rbPkt_[k], std::string("grafika.znieksz.") + np[k]);
        lbPkt_[k] = new wxStaticText(wz, wxID_ANY, "----");
        r->Add(rbPkt_[k], 0, wxRIGHT, 6);
        r->Add(lbPkt_[k], 0, wxALIGN_CENTER_VERTICAL);
        zn->Add(r, 0, wxALL, 2);
        rbPkt_[k]->Bind(wxEVT_RADIOBUTTON, [this, k](wxCommandEvent&) { aktPkt_ = k; });
    }
    const char* nzn[4] = {"h [%]", "h2 [%]", "h3 [%]", "h4 [%]"};
    for (int k = 0; k < 4; k++) {
        wxBoxSizer* r = new wxBoxSizer(wxHORIZONTAL);
        r->Add(new wxStaticText(wz, wxID_ANY, nzn[k], wxDefaultPosition, wxSize(60, -1)), 0, wxRIGHT, 6);
        lbZn_[k] = new wxStaticText(wz, wxID_ANY, "----");
        lbZn_[k]->SetForegroundColour(Kolory::Wynik());
        r->Add(lbZn_[k]);
        zn->Add(r, 0, wxALL, 1);
    }
    s->Add(zn, 0, wxEXPAND | wxTOP, 6);
    auto nak = [this](wxCommandEvent&) { ZastosujNakladki(); };
    for (wxCheckBox* c : {cbSiatkaI_, cbSiatkaU_, cbPa_}) c->Bind(wxEVT_CHECKBOX, nak);
    for (wxTextCtrl* t : {tcSiatkaI_, tcSiatkaU_, tcPa_}) t->Bind(wxEVT_TEXT, nak);
    p->SetSizer(s);
    return p;
}

void DialogMetodyGraficzne::PokazStrone(int nr) {
    ksiazka_->SetSelection(nr);
    bSkalowanie_->SetValue(nr == 0);
    bWyniki_->SetValue(nr == 1);
    plot_->UstawTrybWynikow(nr == 1);
    Layout();
}

void DialogMetodyGraficzne::WczytajObraz() {
    wxFileDialog dlg(this, TR("Wybierz obraz charakterystyki"), "", "",
                     TR("Obrazy (*.png;*.jpg;*.jpeg;*.bmp;*.gif)|*.png;*.jpg;*.jpeg;*.bmp;*.gif|Wszystkie pliki|*"),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;
    wxLogNull cicho;
    wxImage obraz;
    if (!obraz.LoadFile(dlg.GetPath()) || !plot_->UstawObraz(obraz))
        wxMessageBox(TR("Nie udało się wczytać obrazu."), TR("Metody graficzne"), wxOK | wxICON_WARNING, this);
    PokazStrone(0);
}

bool DialogMetodyGraficzne::Skaluj(const wxString& napiecie, const wxString& prad) {
    tcU_->SetValue(napiecie);
    tcI_->SetValue(prad);
    PrzeliczSkale();
    return plot_->Skala().has_value();
}

void DialogMetodyGraficzne::PrzeliczSkale() {
    auto u = Liczba(tcU_), i = Liczba(tcI_);
    if (!plot_->MaObraz()) { lbSkala_->SetLabel(TR("Najpierw wczytaj obraz.")); return; }
    if (!u || !i || !plot_->Skaluj(*u, *i)) {
        lbSkala_->SetLabel(TR("Ustaw wszystkie trzy punkty (punkt Ua na prawo od zera, punkt Ia nad zerem) i wpisz wartości większe od zera."));
        lbSkala_->Wrap(250);
        Odswiez();
        return;
    }
    Odswiez();
}

void DialogMetodyGraficzne::Odswiez() {
    static const char* nazwy[3] = {"(0,0)", "Ua", "Ia"};
    (void)nazwy;
    for (int k = 0; k < 3; k++) lbKrok_[k]->SetLabel(plot_->PunktUstawiony(k) ? TR("punkt ustawiony") : TR("brak punktu"));
    if (const auto& k = plot_->Skala()) {
        lbSkala_->SetLabel(TR("Skala: ") + L(k->voltNaPx, 4) + " V/px,  " + L(k->mANaPx, 4) + " mA/px");
        ZastosujNakladki();
    } else if (plot_->MaObraz()) {
        // komunikat o błędach zostaje
    }
    const bool jest = plot_->Skala().has_value() && plot_->MaProste();
    for (wxString& w : wyniki_) w = "";
    wxString p1 = "----", p2 = "----", du = "----", di = "----", ra = "----", raa = "----", se = "----", pp = "----";
    if (jest) {
        const MetodyGraficzne::Prosta& pr = plot_->Prosta();
        p1 = (pr.p1.u > 0 ? "U = " + L(pr.p1.u, 1) + " V, " : wxString("U = ----, ")) + "I = " + L(pr.p1.i, 1) + " mA";
        p2 = "U = " + L(pr.p2.u, 1) + " V, I = " + L(pr.p2.i, 1) + " mA";
        wyniki_[6] = L(pr.p1.u, 3); wyniki_[7] = L(pr.p1.i, 3); wyniki_[8] = L(pr.p2.u, 3); wyniki_[9] = L(pr.p2.i, 3);
        if (auto w = policz(pr)) {
            du = L(w->deltaU, 2); di = L(w->deltaI, 2);
            ra = L(w->raA, 2); raa = L(w->raaB, 2); se = L(w->mocSE, 2); pp = L(w->mocPP, 2);
            wyniki_[0] = du; wyniki_[1] = di; wyniki_[2] = ra; wyniki_[3] = raa; wyniki_[4] = se; wyniki_[5] = pp;
        }
    }
    PrzeliczZnieksztalcenia();
    lbP1_->SetLabel(p1); lbP2_->SetLabel(p2); lbDu_->SetLabel(du); lbDi_->SetLabel(di);
    lbRa_->SetLabel(ra); lbRaa_->SetLabel(raa); lbSE_->SetLabel(se); lbPP_->SetLabel(pp);
}

void DialogMetodyGraficzne::ZastosujNakladki() {
    double i = 0, u = 0, pa = 0;
    if (cbSiatkaI_->GetValue()) if (auto v = Liczba(tcSiatkaI_); v && *v > 0) i = *v;
    if (cbSiatkaU_->GetValue()) if (auto v = Liczba(tcSiatkaU_); v && *v > 0) u = *v;
    if (cbPa_->GetValue()) if (auto v = Liczba(tcPa_); v && *v > 0) pa = *v;
    plot_->UstawSiatke(i, u);
    plot_->UstawPaMax(pa);
}

void DialogMetodyGraficzne::Dwuklik(double u) {
    if (!plot_->MaProste() || !plot_->Skala()) return;
    const MetodyGraficzne::Prosta& pr = plot_->Prosta();
    const double u1 = std::max(pr.p1.u, 0.0);
    if (!(pr.p2.u > u1) || !(pr.p1.i > pr.p2.i)) return;
    const double uu = std::min(std::max(u, u1), pr.p2.u);
    pktI_[aktPkt_] = pr.p1.i - (uu - u1) / (pr.p2.u - u1) * (pr.p1.i - pr.p2.i);
    pktUstawiony_[aktPkt_] = true;
    aktPkt_ = (aktPkt_ + 1) % 3;
    for (int k = 0; k < 3; k++) rbPkt_[k]->SetValue(k == aktPkt_);
    PrzeliczZnieksztalcenia();
}

void DialogMetodyGraficzne::PrzeliczZnieksztalcenia() {
    for (wxString& z : zn_) z = "";
    for (int k = 0; k < 3; k++) lbPkt_[k]->SetLabel(pktUstawiony_[k] ? L(pktI_[k], 1) + " mA" : wxString("----"));
    for (int k = 0; k < 4; k++) lbZn_[k]->SetLabel("----");
    if (!plot_->MaProste() || !plot_->Skala() || !pktUstawiony_[0] || !pktUstawiony_[1] || !pktUstawiony_[2]) return;
    const MetodyGraficzne::Prosta& pr = plot_->Prosta();
    auto r = Znieksztalcenia::zPradow(pr.p1.i, pktI_[0], pktI_[1], pktI_[2], pr.p2.i);
    if (!r) return;
    const double p[4] = {r->h, r->h2, r->h3, r->h4};
    for (int k = 0; k < 4; k++) { zn_[3 + k] = L(p[k], 3); lbZn_[k]->SetLabel(zn_[3 + k]); }
}

void DialogMetodyGraficzne::UstawKonieczPradu(double u, double i) { plot_->UstawKoniec(Koniec::Pradowy, {u, i}); }
void DialogMetodyGraficzne::UstawKoniecNapiecia(double u, double i) { plot_->UstawKoniec(Koniec::Napieciowy, {u, i}); }
void DialogMetodyGraficzne::UstawBlokady(bool ra, bool amplituda) {
    cbRa_->SetValue(ra);
    cbAmp_->SetValue(amplituda);
    plot_->UstawBlokady(ra, amplituda);
}
void DialogMetodyGraficzne::UstawPaMax(const wxString& w) {
    tcPa_->ChangeValue(w);
    cbPa_->SetValue(true);
    ZastosujNakladki();
}
void DialogMetodyGraficzne::UstawSiatke(const wxString& i, const wxString& u) {
    tcSiatkaI_->ChangeValue(i);
    tcSiatkaU_->ChangeValue(u);
    cbSiatkaI_->SetValue(!i.empty());
    cbSiatkaU_->SetValue(!u.empty());
    ZastosujNakladki();
}

wxString DialogMetodyGraficzne::Wynik(const wxString& nazwa) const {
    static const char* zk[7] = {"ia1", "ia0", "ia2", "zn_h", "zn_h2", "zn_h3", "zn_h4"};
    for (int k = 0; k < 3; k++)
        if (nazwa == zk[k]) return pktUstawiony_[k] ? L(pktI_[k], 3) : wxString();
    for (int k = 3; k < 7; k++)
        if (nazwa == zk[k]) return zn_[k];
    static const char* klucze[10] = {"du", "di", "ra", "raa", "moc_se", "moc_pp", "u1", "i1", "u2", "i2"};
    for (int k = 0; k < 10; k++)
        if (nazwa == klucze[k]) return wyniki_[k];
    return wxString();
}
