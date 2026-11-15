#include "KoloryMotywu.h"
#include "PanelTrafoModel.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <cmath>
#include <wx/dcbuffer.h>

namespace {
const char* kNazwy[6] = {"mu", "bs", "styk", "straty", "eps", "asym"};
struct Poz { const char* klucz; const char* etykieta; int miejsca; };
const Poz kPoz[14] = {
    {"lp", "Indukcyjność przyrostowa Lp [H] =", 3}, {"lp0", "... bez polaryzacji [H] =", 3}, {"mu", "Przenikalność przyrostowa μ =", 0},
    {"bdc", "Indukcja stała [T] =", 3}, {"bac", "Indukcja zmienna, pełna moc przy Fd [T] =", 3}, {"bsuma", "Indukcja łączna [T] =", 3},
    {"lpk", "Lp w szczycie sygnału [H] =", 3}, {"fd", "Dolna częstotliwość graniczna [Hz] =", 1}, {"fg", "Górna częstotliwość graniczna [Hz] =", 0},
    {"rez", "Rezonans powyżej pasma [Hz] =", 0}, {"ra", "Ra widziane z lampy [Ω] =", 1}, {"etacu", "Sprawność (miedź) =", 4},
    {"eta", "Sprawność całkowita (miedź + żelazo) =", 4}, {"pfe", "Straty w żelazie przy Fd [W] =", 3}};
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, m)); }
}

PanelTrafoModel::PanelTrafoModel(wxWindow* parent, bool pp) : wxPanel(parent), pp_(pp) {
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* box = new wxStaticBoxSizer(wxHORIZONTAL, this, TR("Analiza dokładna (jeden model fizyczny: pełny schemat zastępczy)"));
    wxWindow* w = box->GetStaticBox();
    wxFlexGridSizer* gp = new wxFlexGridSizer(0, 2, 4, 6);
    const char* ep[6] = {"Przenikalność początkowa żelaza μi =", "Indukcja nasycenia Bs [T] =", "Dodatkowa szczelina styku [mm] =", "Straty żelaza 1 T/50 Hz [W/kg] =",
                         "Przenikalność izolacji εr =", "Różnica prądów anodowych PP [mA] ="};
    const char* dp[6] = {"3000", "1,8", "0", "2,5", "3", "5"};
    for (int k = 0; k < (pp_ ? 6 : 5); k++) {
        gp->Add(new wxStaticText(w, wxID_ANY, TR(ep[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[k] = new wxTextCtrl(w, wxID_ANY, dp[k], wxDefaultPosition, wxSize(70, -1));
        PomocGUI::Dodaj(tc_[k], "trafomodel.parametry");
        gp->Add(tc_[k]);
        tc_[k]->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { if (naZmiane) naZmiane(); });
    }
    box->Add(gp, 0, wxALL, 6);
    // wyniki w dwóch kolumnach (kolejność: najpierw cała lewa, potem prawa), żeby panel był niski
    wxFlexGridSizer* gw = new wxFlexGridSizer(0, 4, 3, 6);
    const int polowa = (kWyn + 1) / 2;
    for (int r = 0; r < polowa; r++) {
        for (int kol = 0; kol < 2; kol++) {
            const int k = r + kol * polowa;
            if (k >= kWyn) { gw->AddSpacer(1); gw->AddSpacer(1); continue; }
            gw->Add(new wxStaticText(w, wxID_ANY, TR(kPoz[k].etykieta)), 0, wxALIGN_RIGHT);
            lb_[k] = new wxStaticText(w, wxID_ANY, "", wxDefaultPosition, wxSize(70, -1));
            lb_[k]->SetForegroundColour(Kolory::Wynik());
            PomocGUI::Dodaj(lb_[k], "trafomodel.wyniki");
            gw->Add(lb_[k]);
        }
    }
    box->Add(gw, 0, wxALL, 6);
    wykres_ = new wxPanel(w, wxID_ANY, wxDefaultPosition, wxSize(280, 190));
    wykres_->SetBackgroundStyle(wxBG_STYLE_PAINT);
    PomocGUI::Dodaj(wykres_, "trafomodel.wykres");
    wykres_->Bind(wxEVT_PAINT, &PanelTrafoModel::Rysuj, this);
    box->Add(wykres_, 0, wxALL, 6);
    s->Add(box, 0, wxEXPAND);
    lbUwagi_ = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxSize(820, 36));
    lbUwagi_->SetForegroundColour(Kolory::Ostrzezenie());
    s->Add(lbUwagi_, 0, wxTOP, 4);
    SetSizerAndFit(s);
}

void PanelTrafoModel::UwagiDo(std::function<void(const wxString&)> f) {
    naUwagi = std::move(f);
    lbUwagi_->Hide();
    lbUwagi_->SetMinSize(wxSize(0, 0));
    Layout();
}
void PanelTrafoModel::UstawUwagi(const wxString& t) {
    uwagi_ = t;
    if (naUwagi) naUwagi(t); else lbUwagi_->SetLabel(t);
}

bool PanelTrafoModel::Parametry(TrafoModel::Parametry& p) const {
    double v[kPol] = {0, 0, 0, 0, 0, 5};
    for (int k = 0; k < (pp_ ? 6 : 5); k++) if (!tekstNaLiczbe(tc_[k]->GetValue().utf8_string(), v[k])) return false;
    p.muPoczatkowa = v[0]; p.bNasycenieT = v[1]; p.szczelinaStykuMm = v[2]; p.stratyZelazaWkg = v[3]; p.epsR = v[4];
    p.asymetriaPPA = v[5] * 1e-3;
    return true;
}

void PanelTrafoModel::Pokaz(const std::optional<TrafoModel::Wynik>& o, double spadekDb) {
    spadekDb_ = TrafoModel::spadekDokladnyDb(spadekDb);
    for (wxString& x : wyn_) x = "";
    for (wxStaticText* l : lb_) l->SetLabel("");
    krzywa_.clear();
    UstawUwagi("");
    if (!o) { wykres_->Refresh(); return; }
    const double v[kWyn] = {o->lpH, o->lpBezPolaryzacjiH, o->muPrzyrostowa, o->bStalaT, o->bZmiennaT, o->bSumaT, o->lpSzczytH, o->fdHz, o->fgHz,
                            o->rezonansHz, o->raWidzianeOm, o->sprawnoscMiedz, o->sprawnoscCalkowita, o->stratyZelazaW};
    for (int k = 0; k < kWyn; k++) {
        if (std::string(kPoz[k].klucz) == "rez" && o->rezonansHz <= 0) wyn_[k] = TR("brak");
        else wyn_[k] = L(v[k], kPoz[k].miejsca);
        lb_[k]->SetLabel(wyn_[k]);
    }
    krzywa_ = o->krzywa;
    wxString uw;
    if (o->nasycenie) uw += TR("Indukcja łączna przekracza 0,9·Bs - rdzeń nasyca się przy pełnej mocy na dolnym brzegu pasma. ");
    if (o->fdHz <= 0) uw += TR("Dolny kraniec pasma poza zakresem obliczeń. ");
    if (o->fgHz <= 0) uw += TR("Górny kraniec pasma poza zakresem obliczeń. ");
    if (o->rezonansHz > 0) uw += wxString::Format(TR("Rezonans rozproszenia z pojemnością własną: +%s dB przy %s Hz - grozi niestabilnością pętli sprzężenia zwrotnego. "), L(o->nadmiarRezonansuDb, 1), L(o->rezonansHz, 0));
    UstawUwagi(uw);
    wykres_->Refresh();
}

void PanelTrafoModel::UstawPole(const wxString& n, const wxString& v) {
    for (int k = 0; k < kPol; k++) if (tc_[k] && n == kNazwy[k]) tc_[k]->SetValue(v);
}
wxString PanelTrafoModel::Wynik(const wxString& n) const {
    for (int k = 0; k < kWyn; k++) if (n == kPoz[k].klucz) return wyn_[k];
    return wxString();
}

void PanelTrafoModel::Rysuj(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(wykres_);
    dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    dc.Clear();
    const wxSize sz = wykres_->GetClientSize();
    const int l = 38, t = 8, r = 8, b = 22;
    const int W = sz.x - l - r, H = sz.y - t - b;
    dc.SetPen(wxPen(Kolory::Ciemny() ? wxColour(80, 80, 80) : wxColour(200, 200, 200)));
    const double fmin = std::log10(10.0), fmax = std::log10(1e6), dbMin = -12, dbMax = 6;
    auto X = [&](double f) { return l + (std::log10(f) - fmin) / (fmax - fmin) * W; };
    auto Y = [&](double d) { return t + (dbMax - std::max(dbMin, std::min(dbMax, d))) / (dbMax - dbMin) * H; };
    dc.SetFont(wxFont(7, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    for (int e = 1; e <= 6; e++) { const int x = (int)X(std::pow(10.0, e)); dc.DrawLine(x, t, x, t + H); dc.DrawText(e < 3 ? wxString::Format("%d", (int)std::pow(10.0, e)) : (e < 6 ? wxString::Format("%dk", (int)std::pow(10.0, e - 3)) : wxString("1M")), x - 10, t + H + 3); }
    for (int d = -12; d <= 6; d += 3) { const int y = (int)Y(d); dc.DrawLine(l, y, l + W, y); dc.DrawText(wxString::Format("%d", d), 8, y - 6); }
    dc.SetPen(wxPen(Kolory::Ostrzezenie(), 1, wxPENSTYLE_DOT));
    dc.DrawLine(l, (int)Y(-spadekDb_), l + W, (int)Y(-spadekDb_));
    if (krzywa_.size() < 2) return;
    dc.SetPen(wxPen(Kolory::Wynik(), 2));
    for (size_t i = 1; i < krzywa_.size(); i++) {
        if (krzywa_[i].fHz < 10 || krzywa_[i].fHz > 1e6) continue;
        dc.DrawLine((int)X(krzywa_[i - 1].fHz < 10 ? 10 : krzywa_[i - 1].fHz), (int)Y(krzywa_[i - 1].db), (int)X(krzywa_[i].fHz), (int)Y(krzywa_[i].db));
    }
}
