#include "KoloryMotywu.h"
#include "DialogAudioLampy.h"
#include "../Logika/Kalkulatory.h"
#include "../Logika/KalkulatoryAnalogowe.h"
#include "PanelSchematu.h"
#include "../Logika/LogikaKatalogu.h"
#include "../BazaDanych/ManagerBazyDanych.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <wx/dcbuffer.h>
#include <wx/tokenzr.h>
#include <wx/arrstr.h>
#include <wx/statline.h>

#include <algorithm>
#include <cctype>
#include <optional>
#include <tuple>
#include <cmath>

namespace {

std::optional<double> Liczba(const wxTextCtrl* tc) {
    double v = 0;
    if (!tekstNaLiczbe(tc->GetValue().utf8_string(), v)) return std::nullopt;
    return v;
}
std::optional<double> Opor(const wxTextCtrl* tc) {
    return SzeregiE::parsujWartosc(tc->GetValue().utf8_string(), RodzajElementu::Rezystor);
}
std::optional<double> Czest(const wxTextCtrl* tc) {
    std::string t = tc->GetValue().utf8_string();
    const size_t p = t.rfind("Hz");
    if (p != std::string::npos && p + 2 == t.size()) t.erase(p);
    double v = 0;
    if (tekstNaLiczbe(t, v) && v >= 0) return v;                    // także 0
    return SzeregiE::parsujWartosc(t, RodzajElementu::Rezystor);   // k, M jak w rezystorach (2k5)
}
/// Pole opcjonalne: puste = 0 ("nie licz"), błędny tekst = nullopt
std::optional<double> OpcjaLiczba(const wxTextCtrl* tc) {
    if (tc->GetValue().Trim().empty()) return 0.0;
    return Liczba(tc);
}
std::optional<double> OpcjaOpor(const wxTextCtrl* tc) {
    if (tc->GetValue().Trim().empty()) return 0.0;
    return Opor(tc);
}
std::optional<double> OpcjaCzest(const wxTextCtrl* tc) {
    if (tc->GetValue().Trim().empty()) return 0.0;
    return Czest(tc);
}
wxString L(double v, int miejsc = 3) { return TekstZUTF8(liczbaNaTekstPolski(v, miejsc, 0)); }
wxString Znak(double v, int miejsc = 2) { return (v > 0 ? "+" : "") + L(v, miejsc); }
wxString Op(double v) { return TekstZUTF8(SzeregiE::formatZJednostka(v, RodzajElementu::Rezystor)); }
wxString Pf(double v) { return TekstZUTF8(SzeregiE::formatZJednostka(v, RodzajElementu::Kondensator)); }

wxChoice* WyborSzeregu(wxWindow* p, Szereg domyslny, bool tylkoDlaKondensatorow = false) {
    wxChoice* c = new wxChoice(p, wxID_ANY);
    (void)tylkoDlaKondensatorow;
    for (Szereg s : {Szereg::E6, Szereg::E12, Szereg::E24, Szereg::E48, Szereg::E96}) c->Append(TekstZUTF8(SzeregiE::nazwa(s)));
    c->SetSelection(static_cast<int>(domyslny));
    return c;
}
Szereg Wybrany(const wxChoice* c) { return static_cast<Szereg>(std::max(0, c->GetSelection())); }

wxTextCtrl* Pole(wxWindow* p, wxSizer* s, const wxString& wartosc, const char* klucz, int szer = 90) {
    wxTextCtrl* t = new wxTextCtrl(p, wxID_ANY, wartosc, wxDefaultPosition, wxSize(szer, -1));
    PomocGUI::Dodaj(t, klucz);
    s->Add(t, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    return t;
}
void Etykieta(wxWindow* p, wxSizer* s, const wxString& tekst) {
    s->Add(new wxStaticText(p, wxID_ANY, tekst), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
}
wxStaticText* Wynik(wxWindow* p, wxSizer* s) {
    wxStaticText* l = new wxStaticText(p, wxID_ANY, wxEmptyString);
    s->Add(l, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    return l;
}

}  // namespace

// ---------------------------------------------------------------- wykres

WykresRIAA::WykresRIAA(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(420, 240)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(320, 200));
    Bind(wxEVT_PAINT, &WykresRIAA::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
}

void WykresRIAA::UstawZakres(double dbMin, double dbMax, int krok) {
    if (dbMax > dbMin && krok > 0) { dbMin_ = dbMin; dbMax_ = dbMax; krok_ = krok; }
    Refresh();
}

void WykresRIAA::Ustaw(const std::vector<double>& f, const std::vector<double>& idealna, const std::vector<double>& siec) {
    f_ = f;
    idealna_ = idealna;
    siec_ = siec;
    Refresh();
}

void WykresRIAA::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    const wxSize rozm = GetClientSize();
    const int lewo = 44, prawo = 10, gora = 10, dol = 26;
    const int w = rozm.x - lewo - prawo, h = rozm.y - gora - dol;
    if (w < 20 || h < 20 || f_.empty()) return;
    const double fMin = f_.front(), fMax = f_.back();
    const double dbMin = dbMin_, dbMax = dbMax_;
    auto x = [&](double f) { return lewo + static_cast<int>(w * std::log10(f / fMin) / std::log10(fMax / fMin)); };
    auto y = [&](double db) { return gora + static_cast<int>(h * (dbMax - db) / (dbMax - dbMin)); };

    dc.SetFont(GetFont().Smaller());
    dc.SetTextForeground(wxColour(80, 80, 80));
    dc.SetPen(wxPen(wxColour(225, 225, 225)));
    for (int db = static_cast<int>(std::ceil(dbMin)); db <= dbMax; ++db) {
        if (db % krok_ != 0) continue;
        dc.DrawLine(lewo, y(db), lewo + w, y(db));
        dc.DrawText(wxString::Format("%+d", db), 6, y(db) - 7);
    }
    for (double dek = std::pow(10.0, std::floor(std::log10(fMin))); dek <= fMax * 1.001; dek *= 10) {
        for (int m = 1; m <= 9; ++m) {
            const double f = dek * m;
            if (f < fMin * 0.999 || f > fMax * 1.001) continue;
            dc.SetPen(wxPen(m == 1 ? wxColour(190, 190, 190) : wxColour(235, 235, 235)));
            dc.DrawLine(x(f), gora, x(f), gora + h);
            if (m == 1 || m == 2 || m == 5) {
                const wxString t = f >= 1000 ? wxString::Format("%gk", f / 1000) : wxString::Format("%g", f);
                dc.DrawText(t, x(f) - 8, gora + h + 6);
            }
        }
    }
    dc.SetPen(wxPen(wxColour(120, 120, 120)));
    dc.DrawLine(lewo, y(0), lewo + w, y(0));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(lewo, gora, w, h);

    auto krzywa = [&](const std::vector<double>& v, const wxColour& kolor, int grubosc) {
        if (v.size() != f_.size()) return;
        dc.SetPen(wxPen(kolor, grubosc));
        for (size_t i = 1; i < v.size(); ++i)
            dc.DrawLine(x(f_[i - 1]), y(std::max(dbMin, std::min(dbMax, v[i - 1]))), x(f_[i]), y(std::max(dbMin, std::min(dbMax, v[i]))));
    };
    krzywa(idealna_, wxColour(30, 90, 200), 2);
    krzywa(siec_, wxColour(210, 80, 0), 1);
}

// ---------------------------------------------------------------- okno

DialogAudioLampy::DialogAudioLampy(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Narzędzia audio i lampowe"), wxDefaultPosition, wxSize(980, 740),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    ksiazka_ = new wxNotebook(this, wxID_ANY);
    ksiazka_->AddPage(StronaUklady(), TR("Układy lampowe"));
    ksiazka_->AddPage(StronaDecybeli(), TR("dB i nepery"));
    ksiazka_->AddPage(StronaRIAA(), TR("Krzywa RIAA"));
    ksiazka_->AddPage(StronaPunktuPracy(), TR("Punkt pracy lampy"));
    ksiazka_->AddPage(StronaKatody(), TR("Polaryzacja katodowa"));
    ksiazka_->AddPage(StronaSprzezenia(), TR("Kondensator sprzęgający"));
    ksiazka_->AddPage(StronaBarwy(), TR("Regulacja barwy"));
    glowny->Add(ksiazka_, 1, wxEXPAND | wxALL, 10);
    glowny->Add(new wxButton(this, wxID_CLOSE, TR("Zamknij")), 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    SetSizer(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    UkladZmieniony();
    PrzeliczDecybele();
    PrzeliczRIAA();
    PrzeliczTabele();
    PrzeliczSiec();
    PrzeliczLampe();
    PrzeliczKatode();
    PrzeliczSprzezenie();
    PrzeliczBarwe();
}

// ---------------------------------------------------------------- schemat układu

SchematUkladu::SchematUkladu(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(330, 340)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(300, 300));
    Bind(wxEVT_PAINT, &SchematUkladu::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
}

void SchematUkladu::Ustaw(const SchematyLampowe::Schemat& s) {
    schemat_ = s;
    Refresh();
}

void SchematUkladu::OnPaint(wxPaintEvent&) {
    using namespace SchematyLampowe;
    wxAutoBufferedPaintDC dc(this);
    const wxColour tlo = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour tekst = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    const bool ciemny = (tlo.Red() + tlo.Green() + tlo.Blue()) < 3 * 128;
    const wxColour kolUzyty = ciemny ? wxColour(110, 170, 255) : wxColour(0, 70, 200);
    const wxColour kolSzary = ciemny ? wxColour(150, 150, 150) : wxColour(120, 120, 120);
    dc.SetBackground(wxBrush(tlo));
    dc.Clear();
    const wxSize sz = GetClientSize();
    const int zapas = 34;   // miejsce na podpis
    const double skala = std::min(sz.x / static_cast<double>(kSzer), (sz.y - zapas) / static_cast<double>(kWys));
    if (skala <= 0) return;
    const double ox = (sz.x - kSzer * skala) / 2;
    auto X = [&](double x) { return static_cast<int>(ox + x * skala + 0.5); };
    auto Y = [&](double y) { return static_cast<int>(y * skala + 0.5); };
    auto D = [&](double v) { return static_cast<int>(v * skala + 0.5); };
    wxFont f = GetFont();
    f.SetPointSize(std::max(7, static_cast<int>(9 * std::min(1.0, skala * 1.1) + 0.5)));
    auto etykieta = [&](double x, double y, const std::string& t, bool uzyty, bool srodkuj = false) {
        if (t.empty()) return;
        wxFont ff = f;
        if (uzyty) ff.SetWeight(wxFONTWEIGHT_BOLD);
        dc.SetFont(ff);
        dc.SetTextForeground(uzyty ? kolUzyty : kolSzary);
        const wxString w = wxString::FromUTF8(t);
        const wxSize ts = dc.GetTextExtent(w);
        dc.DrawText(w, X(x) - (srodkuj ? ts.x / 2 : 0), Y(y) - ts.y / 2);
    };
    for (const ElementRysunku& e : schemat_.elementy) {
        const wxColour kol = e.typ == Typ::Linia ? tekst : (e.uzyty ? kolUzyty : (e.typ == Typ::Tekst ? kolSzary : tekst));
        wxPen pioro(e.kreska ? kolSzary : kol, e.uzyty && e.typ != Typ::Trioda ? 2 : 1, e.kreska ? wxPENSTYLE_SHORT_DASH : wxPENSTYLE_SOLID);
        dc.SetPen(pioro);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        switch (e.typ) {
        case Typ::Linia: dc.DrawLine(X(e.x1), Y(e.y1), X(e.x2), Y(e.y2)); break;
        case Typ::RezystorPion: {
            const double sr = (e.y1 + e.y2) / 2, pol = std::min(11.0, std::abs(e.y2 - e.y1) / 2 - 1);
            dc.DrawLine(X(e.x1), Y(e.y1), X(e.x1), Y(sr - pol));
            dc.DrawLine(X(e.x1), Y(sr + pol), X(e.x1), Y(e.y2));
            dc.DrawRectangle(X(e.x1 - 4), Y(sr - pol), D(8), D(2 * pol));
            etykieta(e.x1 + 8, sr, e.tekst, e.uzyty);
            break;
        }
        case Typ::KondPoz: {
            const double sr = (e.x1 + e.x2) / 2;
            dc.DrawLine(X(e.x1), Y(e.y1), X(sr - 2), Y(e.y1));
            dc.DrawLine(X(sr + 2), Y(e.y1), X(e.x2), Y(e.y1));
            dc.DrawLine(X(sr - 2), Y(e.y1 - 8), X(sr - 2), Y(e.y1 + 8));
            dc.DrawLine(X(sr + 2), Y(e.y1 - 8), X(sr + 2), Y(e.y1 + 8));
            etykieta(sr, e.y1 - 14, e.tekst, e.uzyty, true);
            break;
        }
        case Typ::KondPion: {
            const double sr = (e.y1 + e.y2) / 2;
            dc.DrawLine(X(e.x1), Y(e.y1), X(e.x1), Y(sr - 2));
            dc.DrawLine(X(e.x1), Y(sr + 2), X(e.x1), Y(e.y2));
            dc.DrawLine(X(e.x1 - 8), Y(sr - 2), X(e.x1 + 8), Y(sr - 2));
            dc.DrawLine(X(e.x1 - 8), Y(sr + 2), X(e.x1 + 8), Y(sr + 2));
            etykieta(e.x1 + 10, sr, e.tekst, e.uzyty);
            break;
        }
        case Typ::Trioda: {
            dc.SetPen(wxPen(tekst, 1));
            dc.DrawCircle(X(e.x1), Y(e.y1), D(20));
            dc.DrawLine(X(e.x1 - 8), Y(e.y1 - 10), X(e.x1 + 8), Y(e.y1 - 10));   // anoda
            dc.DrawLine(X(e.x1), Y(e.y1 - 10), X(e.x1), Y(e.y1 - 30));
            dc.SetPen(wxPen(tekst, 1, wxPENSTYLE_SHORT_DASH));
            dc.DrawLine(X(e.x1 - 9), Y(e.y1), X(e.x1 + 9), Y(e.y1));              // siatka
            dc.SetPen(wxPen(tekst, 1));
            dc.DrawLine(X(e.x1 - 9), Y(e.y1), X(e.x1 - 30), Y(e.y1));
            dc.DrawLine(X(e.x1 - 6), Y(e.y1 + 10), X(e.x1 + 6), Y(e.y1 + 10));   // katoda
            dc.DrawLine(X(e.x1), Y(e.y1 + 10), X(e.x1), Y(e.y1 + 30));
            break;
        }
        case Typ::Masa:
            dc.SetPen(wxPen(tekst, 1));
            dc.DrawLine(X(e.x1), Y(e.y1), X(e.x1), Y(e.y1 + 4));
            dc.DrawLine(X(e.x1 - 8), Y(e.y1 + 4), X(e.x1 + 8), Y(e.y1 + 4));
            dc.DrawLine(X(e.x1 - 5), Y(e.y1 + 8), X(e.x1 + 5), Y(e.y1 + 8));
            dc.DrawLine(X(e.x1 - 2), Y(e.y1 + 12), X(e.x1 + 2), Y(e.y1 + 12));
            break;
        case Typ::Zasilanie:
            dc.SetPen(wxPen(tekst, 1));
            dc.DrawLine(X(e.x1 - 8), Y(e.y1), X(e.x1 + 8), Y(e.y1));
            etykieta(e.x1 + 12, e.y1, e.tekst, false);
            break;
        case Typ::Zacisk:
            dc.SetPen(wxPen(tekst, 1));
            dc.DrawCircle(X(e.x1), Y(e.y1), std::max(2, D(3)));
            etykieta(e.x1, e.y1 - 11, e.tekst, false, true);
            break;
        case Typ::Kropka:
            dc.SetPen(wxPen(tekst, 1));
            dc.SetBrush(wxBrush(tekst));
            dc.DrawCircle(X(e.x1), Y(e.y1), std::max(2, D(2)));
            break;
        case Typ::Tekst: etykieta(e.x1, e.y1, e.tekst, e.uzyty); break;
        }
    }
    // podpis i objaśnienie koloru
    dc.SetFont(f);
    dc.SetTextForeground(tekst);
    wxString podpis = wxString::FromUTF8(schemat_.podpis);
    if (!podpis.empty()) {
        podpis = wxGetTranslation(podpis);
        wxString w = podpis;
        int y = static_cast<int>(kWys * skala) + 4;
        // zawijanie do szerokości panelu
        wxString linia, wynik;
        for (const wxString& slowo : wxSplit(w, ' ')) {
            const wxString probna = linia.empty() ? slowo : linia + " " + slowo;
            if (dc.GetTextExtent(probna).x > sz.x - 8 && !linia.empty()) { dc.DrawText(linia, 4, y); y += dc.GetCharHeight(); linia = slowo; }
            else linia = probna;
        }
        dc.DrawText(linia, 4, y);
    }
}

// ---------------------------------------------------------------- układy lampowe

namespace {

struct DefPola {
    const char* etykieta;   // klucz do TR()
    const char* domyslna;
};
struct DefUkladu {
    const char* nazwa;
    std::vector<DefPola> pola;
    std::vector<const char*> wyniki;
};

// Wartości domyślne to przykłady z ECCLab (ECC83 / ECC81 / ECC82 ...)
const std::vector<DefUkladu>& Uklady() {
    static const std::vector<DefUkladu> u = {
        {"Sprzężenie RC",
         {{"Cak [pF]", "0,45"}, {"Cas [pF]", "1,1"}, {"Csk [pF]", "1,6"}, {"Cs [nF]", "220"}, {"Rk [kΩ]", "0,47"},
          {"Ra [kΩ]", "100"}, {"Rs [kΩ]", "470"}, {"Sa [mA/V]", "1,6"}, {"Ka [V/V]", "100"}, {"Rwew [kΩ]", "62,5"}},
         {"fd – częstotliwość dolna -3 dB [Hz]", "fg – częstotliwość górna -3 dB [kHz]", "Wzmocnienie [V/V]",
          "Oporność wyjściowa [kΩ]"}},
        {"Wtórnik anodowy",
         {{"Rs [kΩ]", "470"}, {"Ro [kΩ]", "0,47"}, {"Sa [mA/V]", "5,5"}, {"Ka [V/V]", "60"}, {"Rwew [kΩ]", "11"}},
         {"Oporność wejściowa [kΩ]", "Oporność wyjściowa [kΩ]", "Wzmocnienie [V/V]"}},
        {"Wtórnik katodowy",
         {{"Csk [pF]", "1,6"}, {"Rk [kΩ]", "0,47"}, {"R [kΩ]", "47"}, {"Rs [kΩ]", "470"}, {"Sa [mA/V]", "1,6"},
          {"Ka [V/V]", "100"}, {"Rwew [kΩ]", "62,5"}, {"Ugk [V]", "6"}},
         {"Oporność wejściowa [kΩ]", "Oporność wyjściowa [kΩ]", "Wzmocnienie [V/V]", "fg – częstotliwość górna [MHz]",
          "Maksymalne napięcie wejściowe [V]"}},
        {"Inwerter różnicowy",
         {{"Rk [kΩ]", "11,3"}, {"Ra [kΩ]", "100"}, {"Rs [kΩ]", "470"}, {"Ka [V/V]", "60"}, {"Rwew [kΩ]", "11"}},
         {"Współczynnik asymetrii q", "Średni współczynnik wzmocnienia [V/V]", "Wzmocnienie pierwszej triody [V/V]",
          "Wzmocnienie drugiej triody [V/V]"}},
        {"Mi-wtórnik",
         {{"Ka1 [V/V]", "100"}, {"Rwew1 [kΩ]", "62,5"}, {"Ka2 [V/V]", "100"}, {"Rwew2 [kΩ]", "62,5"},
          {"Rk2 [kΩ]", "2,2"}, {"R0 – obciążenie, 0 = brak [kΩ]", "100"}, {"Cas1 [pF]", "1,6"}, {"Csk1 [pF]", "1,6"},
          {"Cs0 [pF]", "10"}},
         {"Wzmocnienie [V/V]", "Oporność wyjściowa [kΩ]", "Oporność obciążenia anody lampy 1 [kΩ]",
          "Pojemność wejściowa [pF]"}},
        {"SRPP",
         {{"Ka [V/V]", "20"}, {"Rwew [kΩ]", "7,7"}, {"Rk [kΩ]", "1"}, {"RL – obciążenie, 0 = brak [kΩ]", "100"}},
         {"Wzmocnienie bez obciążenia [V/V]", "Wzmocnienie [V/V]", "Oporność wyjściowa [kΩ]",
          "Optymalne obciążenie [kΩ]"}},
        {"Wtórnik White'a",
         {{"Ka1 [V/V]", "20"}, {"Rwew1 [kΩ]", "7,7"}, {"Ka2 [V/V]", "20"}, {"Rwew2 [kΩ]", "7,7"},
          {"RL [kΩ]", "10"}},
         {"Wzmocnienie [V/V]", "Oporność wyjściowa [kΩ]", "Optymalny rezystor anodowy [kΩ]"}},
        {"Kaskodowy wtórnik katodowy",
         {{"Ka1 [V/V]", "60"}, {"Rwew1 [kΩ]", "11"}, {"Ka2 [V/V]", "60"}, {"Rwew2 [kΩ]", "11"}, {"Ra [kΩ]", "10"},
          {"R0 – obciążenie [kΩ]", "100"}},
         {"Wzmocnienie [V/V]", "Wzmocnienie bez obciążenia [V/V]", "Oporność wyjściowa [kΩ]",
          "Oporność wyjściowa, przybliżenie [kΩ]"}},
        {"Superwtórnik",
         {{"Ka [V/V]", "60"}, {"Rwew [kΩ]", "11"}, {"Ra [kΩ]", "10"}, {"Rząd n (1..6)", "1"}},
         {"Wzmocnienie [V/V]", "Oporność wyjściowa [kΩ]", "Wzmocnienie do anody lampy 1 [V/V]"}}};
    return u;
}

}  // namespace

wxPanel* DialogAudioLampy::StronaUklady() {
    wxPanel* p = new wxPanel(ksiazka_);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);
    gora->Add(new wxStaticText(p, wxID_ANY, TR("Układ:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    chUklad_ = new wxChoice(p, wxID_ANY);
    for (const auto& u : Uklady()) chUklad_->Append(TR(u.nazwa));
    chUklad_->SetSelection(0);
    PomocGUI::DodajPole(chUklad_, "audio.uklad");
    gora->Add(chUklad_, 0, wxRIGHT, 16);
    cbAutom_ = new wxCheckBox(p, wxID_ANY, TR("Polaryzacja automatyczna (Rk bez kondensatora)"));
    cbAutom_->SetValue(true);
    PomocGUI::DodajPole(cbAutom_, "audio.uklad_polaryzacja");
    gora->Add(cbAutom_, 0, wxALIGN_CENTER_VERTICAL);
    s->Add(gora, 0, wxALL, 10);
    wxBoxSizer* lampa = new wxBoxSizer(wxHORIZONTAL);
    wxButton* bz = new wxButton(p, wxID_ANY, TR("Lampa z katalogu..."));
    wxButton* bs = new wxButton(p, wxID_ANY, TR("Zapisz lampę do katalogu..."));
    PomocGUI::Dodaj(bz, "audio.uklad_lampa");
    PomocGUI::Dodaj(bs, "audio.uklad_lampa");
    lampa->Add(bz, 0, wxRIGHT, 8);
    lampa->Add(bs, 0);
    s->Add(lampa, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    bz->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnLampaZKatalogu(); });
    bs->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnZapiszLampe(); });
    wxFlexGridSizer* siatka = new wxFlexGridSizer(0, 4, 6, 8);
    for (int i = 0; i < kMaksPol; i++) {
        etPola_[i] = new wxStaticText(p, wxID_ANY, "");
        tcPola_[i] = new wxTextCtrl(p, wxID_ANY, "", wxDefaultPosition, wxSize(90, -1));
        PomocGUI::Dodaj(tcPola_[i], "audio.uklad_pola");
        siatka->Add(etPola_[i], 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        siatka->Add(tcPola_[i], 0);
        tcPola_[i]->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczUklad(); });
    }
    wxBoxSizer* srodek = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* lewy = new wxBoxSizer(wxVERTICAL);
    lewy->Add(siatka, 0, wxLEFT | wxRIGHT, 10);
    lewy->Add(new wxStaticLine(p), 0, wxEXPAND | wxALL, 10);
    wxFlexGridSizer* wyn = new wxFlexGridSizer(0, 2, 4, 12);
    for (int i = 0; i < kMaksWynikow; i++) {
        etWyn_[i] = new wxStaticText(p, wxID_ANY, "");
        lbWyn_[i] = new wxStaticText(p, wxID_ANY, "");
        lbWyn_[i]->SetForegroundColour(Kolory::Wynik());
        wyn->Add(etWyn_[i], 0, wxALIGN_RIGHT);
        wyn->Add(lbWyn_[i], 0);
    }
    lewy->Add(wyn, 0, wxLEFT | wxRIGHT, 10);
    lbUwagi_ = new wxStaticText(p, wxID_ANY, "");
    lbUwagi_->SetForegroundColour(Kolory::Ostrzezenie());
    lewy->Add(lbUwagi_, 0, wxALL, 10);
    srodek->Add(lewy, 1, wxEXPAND);
    schemat_ = new SchematUkladu(p);
    srodek->Add(schemat_, 0, wxEXPAND | wxRIGHT | wxBOTTOM, 10);
    s->Add(srodek, 1, wxEXPAND);
    p->SetSizer(s);
    chUklad_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UkladZmieniony(); });
    cbAutom_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { PrzeliczUklad(); });
    panelUkladu_ = p;
    return p;
}

void DialogAudioLampy::UkladZmieniony() {
    const auto& u = Uklady()[chUklad_->GetSelection()];
    for (int i = 0; i < kMaksPol; i++) {
        const bool jest = i < static_cast<int>(u.pola.size());
        etPola_[i]->Show(jest);
        tcPola_[i]->Show(jest);
        roleP_[i].clear();
        if (jest) {
            std::string e = u.pola[i].etykieta;
            e = e.substr(0, e.find(' '));
            while (!e.empty() && std::isdigit(static_cast<unsigned char>(e.back()))) e.pop_back();
            if (e == "Sa" || e == "Ka" || e == "Rwew" || e == "Cak" || e == "Cas" || e == "Csk") roleP_[i] = e;
            etPola_[i]->SetLabel(TR(u.pola[i].etykieta));
            tcPola_[i]->ChangeValue(wxString::FromUTF8(u.pola[i].domyslna));
            PomocGUI::Dodaj(tcPola_[i], "audio.uklad_pola");   // klucz zależy od etykiety, która zmienia się z układem
        }
    }
    for (int i = 0; i < kMaksWynikow; i++) {
        const bool jest = i < static_cast<int>(u.wyniki.size());
        etWyn_[i]->Show(jest);
        lbWyn_[i]->Show(jest);
        if (jest) etWyn_[i]->SetLabel(TR(u.wyniki[i]) + " =");
    }
    cbAutom_->Show(chUklad_->GetSelection() == 0);
    panelUkladu_->Layout();
    schemat_->Ustaw(SchematyLampowe::zbuduj(chUklad_->GetSelection(), cbAutom_->GetValue()));
    PrzeliczUklad();
}

void DialogAudioLampy::PrzeliczUklad() {
    using namespace KalkulatoryLampowe;
    const int nr = chUklad_->GetSelection();
    if (schemat_ && nr == 0) schemat_->Ustaw(SchematyLampowe::zbuduj(0, cbAutom_->GetValue()));
    const int n = static_cast<int>(Uklady()[nr].pola.size());
    std::vector<double> v(n, 0.0);
    bool ok = true;
    for (int i = 0; i < n; i++) {
        double x = 0;
        if (tekstNaLiczbe(tcPola_[i]->GetValue().utf8_string(), x)) v[i] = x;
        else ok = false;
    }
    for (int i = 0; i < kMaksWynikow; i++) lbWyn_[i]->SetLabel("");
    lbUwagi_->SetLabel("");
    auto pokaz = [&](int i, double x, int cyfr) { lbWyn_[i]->SetLabel(L(x, cyfr)); };
    auto blad = [&]() { lbUwagi_->SetLabel(TR("Uwagi: uzupełnij pola liczbami (rezystancje, pojemności i dane lampy większe od zera).")); };
    if (!ok) { blad(); return; }
    switch (nr) {
    case 0: {
        WejscieRC w{v[0], v[1], v[2], v[3], v[4], v[5], v[6], {v[7], v[8], v[9]}, cbAutom_->GetValue()};
        auto r = sprzezenieRC(w);
        if (!r) { blad(); return; }
        pokaz(0, r->fdHz, 2); pokaz(1, r->fgKHz, 2); pokaz(2, r->ku, 2); pokaz(3, r->rwyk, 2);
        if (r->fgPrzyblizona)
            lbUwagi_->SetLabel(TR("Uwagi: przy polaryzacji automatycznej górna częstotliwość jest przybliżona (ECCLab podaje niższą)."));
        break;
    }
    case 1: {
        auto r = wtornikAnodowy({v[0], v[1], {v[2], v[3], v[4]}});
        if (!r) { blad(); return; }
        pokaz(0, r->rwe, 2); pokaz(1, r->rwyk, 3); pokaz(2, r->ku, 3);
        break;
    }
    case 2: {
        auto r = wtornikKatodowy({v[0], v[1], v[2], v[3], {v[4], v[5], v[6]}, v[7]});
        if (!r) { blad(); return; }
        pokaz(0, r->rwe, 2); pokaz(1, r->rwyk, 3); pokaz(2, r->ku, 2); pokaz(3, r->fgMHz, 2); pokaz(4, r->umax, 3);
        break;
    }
    case 4: {
        auto r = muWtornik({{0, v[0], v[1]}, {0, v[2], v[3]}, v[4], v[5], v[6], v[7], v[8]});
        if (!r) { blad(); return; }
        pokaz(0, r->ku, 3); pokaz(1, r->rwyk, 3); pokaz(2, r->raktywne, 2); pokaz(3, r->cinPf, 2);
        break;
    }
    case 5: {
        auto r = srpp({{0, v[0], v[1]}, v[2], v[3]});
        if (!r) { blad(); return; }
        pokaz(0, r->ku0, 3); pokaz(1, r->ku, 3); pokaz(2, r->rwyk, 3); pokaz(3, r->rlOpt, 3);
        break;
    }
    case 6: {
        auto r = wtornikWhite({{0, v[0], v[1]}, {0, v[2], v[3]}, v[4]});
        if (!r) { blad(); return; }
        pokaz(0, r->ku, 4); pokaz(1, r->rwyk, 4); pokaz(2, r->raOpt, 3);
        break;
    }
    case 7: {
        auto r = wtornikKaskodowy({{0, v[0], v[1]}, {0, v[2], v[3]}, v[4], v[5]});
        if (!r) { blad(); return; }
        pokaz(0, r->ku, 4); pokaz(1, r->ku0, 4); pokaz(2, r->rwyk, 4); pokaz(3, r->rwykPrzybl, 4);
        break;
    }
    case 8: {
        auto r = superwtornik({{0, v[0], v[1]}, v[2], static_cast<int>(v[3])});
        if (!r) { blad(); return; }
        pokaz(0, r->ku1, 5); pokaz(1, r->rwyk, 5); pokaz(2, r->ku3, 5);
        break;
    }
    default: {
        auto r = inwerterRoznicowy({v[0], v[1], v[2], {0, v[3], v[4]}});
        if (!r) { blad(); return; }
        pokaz(0, r->q, 4); pokaz(1, r->kuSr, 3); pokaz(2, r->ku1, 3); pokaz(3, r->ku2, 3);
        break;
    }
    }
}

void DialogAudioLampy::UstawUklad(int nr) {
    chUklad_->SetSelection(nr);
    UkladZmieniony();
}
int DialogAudioLampy::LiczbaPolUkladu() const { return static_cast<int>(Uklady()[chUklad_->GetSelection()].pola.size()); }
void DialogAudioLampy::UstawPoleUkladu(int nr, const wxString& tekst) {
    tcPola_[nr]->SetValue(tekst);
}
wxString DialogAudioLampy::EtykietaPolaUkladu(int nr) const { return etPola_[nr]->GetLabel(); }
void DialogAudioLampy::UstawPolaryzacjeAutomatyczna(bool autom) {
    cbAutom_->SetValue(autom);
    PrzeliczUklad();
}
int DialogAudioLampy::LiczbaWynikowUkladu() const { return static_cast<int>(Uklady()[chUklad_->GetSelection()].wyniki.size()); }
wxString DialogAudioLampy::ElementySchematu() const {
    wxString w;
    for (const auto& e : schemat_->Schemat().uzyte()) w += (w.empty() ? "" : " ") + wxString::FromUTF8(e);
    return w;
}
wxString DialogAudioLampy::WynikUkladu(int nr) const { return lbWyn_[nr]->GetLabel(); }

// ---------------------------------------------------------------- lampy z katalogu

void DialogAudioLampy::ZastosujLampe(const LampyKatalog::DaneLampy& d) {
    const int n = LiczbaPolUkladu();
    for (int i = 0; i < n; i++) {
        const std::string& r = roleP_[i];
        std::optional<double> v;
        if (r == "Sa") v = d.sa;
        else if (r == "Ka") v = d.ka;
        else if (r == "Rwew") v = d.rwew;
        else if (r == "Cak") v = d.cak;
        else if (r == "Cas") v = d.cas;
        else if (r == "Csk") v = d.csk;
        if (v) tcPola_[i]->ChangeValue(L(*v, 4));
    }
    PrzeliczUklad();
}

std::optional<LampyKatalog::DaneLampy> DialogAudioLampy::DaneLampyZPol() const {
    PozycjaKatalogu p;
    const int n = LiczbaPolUkladu();
    for (int i = 0; i < n; i++) {
        const std::string& r = roleP_[i];
        if (r.empty()) continue;
        const char* nazwa = r == "Sa" ? LampyKatalog::kParSa : r == "Ka" ? LampyKatalog::kParKa
                          : r == "Rwew" ? LampyKatalog::kParRwew : r == "Cak" ? LampyKatalog::kParCak
                          : r == "Cas" ? LampyKatalog::kParCas : LampyKatalog::kParCsk;
        if (p.parametr(nazwa)) continue;   // pierwsza lampa układu
        double x = 0;
        if (r == "Sa" || r == "Ka" || r == "Rwew")   // te trzy muszą być poprawnymi liczbami dodatnimi
            if (!tekstNaLiczbe(tcPola_[i]->GetValue().utf8_string(), x) || x <= 0) return std::nullopt;
        p.parametry.push_back({nazwa, tcPola_[i]->GetValue().utf8_string(), "reczny"});
    }
    return LampyKatalog::zPozycji(p);
}

bool DialogAudioLampy::LampaZKatalogu(const wxString& symbol) {
    ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
    if (!m.czyPolaczony()) return false;
    LogikaKatalogu kat(m);
    auto poz = kat.znajdz(symbol.utf8_string());
    if (!poz) return false;
    auto d = LampyKatalog::zPozycji(*poz);
    if (!d) return false;
    ZastosujLampe(*d);
    return true;
}

bool DialogAudioLampy::ZapiszLampeDoKatalogu(const wxString& symbol) {
    ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
    if (!m.czyPolaczony()) return false;
    auto d = DaneLampyZPol();
    wxString sy = symbol;
    sy.Trim().Trim(false);
    const std::string sym = sy.utf8_string();
    if (!d || sym.empty()) return false;
    LogikaKatalogu kat(m);
    PozycjaKatalogu poz = LampyKatalog::pozycja(sym, *d);
    if (auto stara = kat.znajdz(sym)) {
        poz = *stara;                                   // zachowaj opis, producenta, datasheet
        if (poz.rodzina.empty()) poz.rodzina = LampyKatalog::kRodzina;
        std::vector<ParametrKatalogu> inne;
        for (const auto& p : poz.parametry) {
            bool lampowy = false;
            for (const auto& l : LampyKatalog::parametry(*d))
                if (p.nazwa == l.nazwa) lampowy = true;
            if (!lampowy) inne.push_back(p);
        }
        for (const auto& l : LampyKatalog::parametry(*d)) inne.push_back(l);
        poz.parametry = inne;
    }
    return kat.zapisz(poz).has_value();
}

int DialogAudioLampy::DodajLampyPrzykladowe() {
    ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
    if (!m.czyPolaczony()) return -1;
    LogikaKatalogu kat(m);
    int dodano = 0;
    for (const auto& p : LampyKatalog::przykladowe()) {
        if (kat.znajdz(p.symbol)) continue;
        if (!kat.zapisz(p)) return -1;
        dodano++;
    }
    return dodano;
}

std::optional<LampyKatalog::DaneLampy> DialogAudioLampy::WybierzLampeZKatalogu() {
    ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
    if (!m.czyPolaczony()) {
        wxMessageBox(TR("Brak połączenia z bazą danych."), TR("Lampa z katalogu"), wxOK | wxICON_INFORMATION, this);
        return std::nullopt;
    }
    LogikaKatalogu kat(m);
    auto wczytaj = [&] {
        wxArrayString nazwy;
        std::vector<LampyKatalog::DaneLampy> dane;
        FiltrKatalogu f;
        f.rodzina = LampyKatalog::kRodzina;
        std::vector<PozycjaKatalogu> w;
        if (kat.pobierz(f, w))
            for (const auto& p : w) {
                auto pe = kat.pobierzPozycje(p.id);
                auto d = pe ? LampyKatalog::zPozycji(*pe) : std::nullopt;
                if (!d) continue;
                nazwy.Add(wxString::FromUTF8(p.symbol) + "   (Sa " + L(d->sa, 3) + ", Ka " + L(d->ka, 3) + ", Rwew " + L(d->rwew, 3) + ")");
                dane.push_back(*d);
            }
        return std::make_pair(nazwy, dane);
    };
    auto [nazwy, dane] = wczytaj();
    if (nazwy.empty()) {
        if (wxMessageBox(TR("W katalogu nie ma jeszcze lamp (rodzina „Lampy elektronowe”). Dodać lampy przykładowe: ECC81, ECC82, ECC83, ECC85, ECC88?"),
                         TR("Lampa z katalogu"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
            return std::nullopt;
        if (DodajLampyPrzykladowe() < 0) return std::nullopt;
        std::tie(nazwy, dane) = wczytaj();
        if (nazwy.empty()) return std::nullopt;
    }
    wxSingleChoiceDialog dlg(this, TR("Wybierz lampę z katalogu:"), TR("Lampa z katalogu"), nazwy);
    if (dlg.ShowModal() != wxID_OK) return std::nullopt;
    return dane[dlg.GetSelection()];
}

void DialogAudioLampy::OnLampaZKatalogu() {
    if (auto d = WybierzLampeZKatalogu()) ZastosujLampe(*d);
}

void DialogAudioLampy::ZastosujLampeDoPunktuPracy(const LampyKatalog::DaneLampy& d) {
    poleLM_[6]->ChangeValue(L(d.ka, 4));                       // amplifikacja μ = Ka
    poleLM_[7]->ChangeValue(Op(d.rwew * 1000.0));              // ra = Rwew [kΩ]
    if (d.paMaxW) poleLM_[8]->ChangeValue(L(*d.paMaxW, 3));
    PrzeliczLampe();
}

void DialogAudioLampy::OnLampaDlaPunktuPracy() {
    if (auto d = WybierzLampeZKatalogu()) ZastosujLampeDoPunktuPracy(*d);
}

bool DialogAudioLampy::LampaDlaPunktuPracy(const wxString& symbol) {
    ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
    if (!m.czyPolaczony()) return false;
    LogikaKatalogu kat(m);
    auto poz = kat.znajdz(symbol.utf8_string());
    if (!poz) return false;
    auto d = LampyKatalog::zPozycji(*poz);
    if (!d) return false;
    ZastosujLampeDoPunktuPracy(*d);
    return true;
}

void DialogAudioLampy::OnZapiszLampe() {
    if (!ManagerBazyDanych::pobierz().czyPolaczony()) {
        wxMessageBox(TR("Brak połączenia z bazą danych."), TR("Zapisz lampę do katalogu"), wxOK | wxICON_INFORMATION, this);
        return;
    }
    if (!DaneLampyZPol()) {
        wxMessageBox(TR("Uzupełnij Ka i Rwew (albo Sa) lampy liczbami większymi od zera."), TR("Zapisz lampę do katalogu"), wxOK | wxICON_INFORMATION, this);
        return;
    }
    wxTextEntryDialog dlg(this, TR("Symbol lampy (np. ECC83). Istniejąca pozycja dostanie nowe parametry."), TR("Zapisz lampę do katalogu"));
    if (dlg.ShowModal() != wxID_OK) return;
    if (!ZapiszLampeDoKatalogu(dlg.GetValue()))
        wxMessageBox(TR("Nie udało się zapisać lampy w katalogu."), TR("Zapisz lampę do katalogu"), wxOK | wxICON_WARNING, this);
}

wxPanel* DialogAudioLampy::StronaDecybeli() {
    wxPanel* p = new wxPanel(ksiazka_);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    auto wiersz = [&](const wxString& tytul) {
        wxBoxSizer* r = new wxBoxSizer(wxHORIZONTAL);
        s->Add(new wxStaticText(p, wxID_ANY, tytul), 0, wxLEFT | wxRIGHT | wxTOP, 10);
        s->Add(r, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        return r;
    };
    wxBoxSizer* r = wiersz(TR("Zamiana napięć na decybele i nepery (dB = 20 log U1/U2, Np = ln U1/U2)"));
    Etykieta(p, r, "U1"); tcU1_ = Pole(p, r, "100", "audio.napiecia");
    Etykieta(p, r, "U2"); tcU2_ = Pole(p, r, "120", "audio.napiecia");
    lbNapiecia_ = Wynik(p, r);
    r = wiersz(TR("Zamiana mocy na decybele i nepery (dB = 10 log P1/P2, Np = ½ ln P1/P2)"));
    Etykieta(p, r, "P1"); tcP1_ = Pole(p, r, "100", "audio.moce");
    Etykieta(p, r, "P2"); tcP2_ = Pole(p, r, "120", "audio.moce");
    lbMoce_ = Wynik(p, r);
    r = wiersz(TR("Zamiana miary liniowej na decybele"));
    Etykieta(p, r, TR("Miara liniowa")); tcMiara_ = Pole(p, r, "1,000", "audio.miara");
    chMiara_ = new wxChoice(p, wxID_ANY);
    chMiara_->Append(TR("napięcie / prąd"));
    chMiara_->Append(TR("moc"));
    chMiara_->SetSelection(0);
    PomocGUI::DodajPole(chMiara_, "audio.miara.rodzaj");
    r->Add(chMiara_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    lbMiara_ = Wynik(p, r);
    r = wiersz(TR("Zamiana decybeli na miarę liniową"));
    Etykieta(p, r, "dB"); tcDb_ = Pole(p, r, "0", "audio.db");
    chDb_ = new wxChoice(p, wxID_ANY);
    chDb_->Append(TR("napięcie / prąd"));
    chDb_->Append(TR("moc"));
    chDb_->SetSelection(0);
    PomocGUI::DodajPole(chDb_, "audio.db.rodzaj");
    r->Add(chDb_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    lbDb_ = Wynik(p, r);
    r = wiersz(TR("Zamiana neperów na decybele (1 Np = 8,686 dB)"));
    Etykieta(p, r, "Np"); tcNp_ = Pole(p, r, "0", "audio.nepery");
    lbNepery_ = Wynik(p, r);
    r = wiersz(TR("Zamiana decybeli na nepery"));
    Etykieta(p, r, "dB"); tcDbNp_ = Pole(p, r, "0", "audio.nepery");
    lbDbNp_ = Wynik(p, r);
    p->SetSizer(s);
    for (wxTextCtrl* t : {tcU1_, tcU2_, tcP1_, tcP2_, tcMiara_, tcDb_, tcNp_, tcDbNp_})
        t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczDecybele(); });
    for (wxChoice* c : {chMiara_, chDb_}) c->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczDecybele(); });
    return p;
}

void DialogAudioLampy::PrzeliczDecybele() {
    using namespace KalkulatoryAudio;
    const wxString blad = TR("Wpisz liczby większe od zera.");
    auto u1 = Liczba(tcU1_), u2 = Liczba(tcU2_);
    auto d = u1 && u2 ? dbNapiec(*u1, *u2) : std::nullopt;
    lbNapiecia_->SetLabel(d ? L(*d, 3) + " dB     " + L(*neperyNapiec(*u1, *u2), 4) + " Np" : blad);
    auto p1 = Liczba(tcP1_), p2 = Liczba(tcP2_);
    auto dm = p1 && p2 ? dbMocy(*p1, *p2) : std::nullopt;
    lbMoce_->SetLabel(dm ? L(*dm, 3) + " dB     " + L(*neperyMocy(*p1, *p2), 4) + " Np" : blad);
    auto m = Liczba(tcMiara_);
    auto dl = m ? dbZMiaryLiniowej(*m, chMiara_->GetSelection() == 1) : std::nullopt;
    lbMiara_->SetLabel(dl ? L(*dl, 3) + " dB" : blad);
    auto db = Liczba(tcDb_);
    lbDb_->SetLabel(db ? TR("miara liniowa = ") + L(miaraLiniowaZDb(*db, chDb_->GetSelection() == 1), 5) : TR("Wpisz liczbę."));
    auto np = Liczba(tcNp_);
    lbNepery_->SetLabel(np ? L(dbZNeperow(*np), 4) + " dB" : TR("Wpisz liczbę."));
    auto dn = Liczba(tcDbNp_);
    lbDbNp_->SetLabel(dn ? L(neperyZDb(*dn), 5) + " Np" : TR("Wpisz liczbę."));
}

wxPanel* DialogAudioLampy::StronaRIAA() {
    wxPanel* p = new wxPanel(ksiazka_);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* g = new wxBoxSizer(wxHORIZONTAL);
    chRodzaj_ = new wxChoice(p, wxID_ANY);
    chRodzaj_->Append(TR("Odtwarzanie (korektor gramofonowy)"));
    chRodzaj_->Append(TR("Nagrywanie (krzywa tłoczenia)"));
    chRodzaj_->SetSelection(0);
    PomocGUI::DodajPole(chRodzaj_, "audio.riaa_rodzaj");
    g->Add(chRodzaj_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    cbIec_ = new wxCheckBox(p, wxID_ANY, TR("IEC (filtr 20 Hz)"));
    PomocGUI::DodajPole(cbIec_, "audio.riaa_iec");
    g->Add(cbIec_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 14);
    Etykieta(p, g, TR("Częstotliwość [Hz]"));
    tcF_ = Pole(p, g, "1000", "audio.riaa_f");
    s->Add(g, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    lbRiaaPunkt_ = new wxStaticText(p, wxID_ANY, wxEmptyString);
    lbRiaaPunkt_->SetFont(lbRiaaPunkt_->GetFont().Bold());
    s->Add(lbRiaaPunkt_, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 10);

    wxBoxSizer* z = new wxBoxSizer(wxHORIZONTAL);
    Etykieta(p, z, TR("Tabela od [Hz]"));
    tcOd_ = Pole(p, z, "20", "audio.riaa_zakres", 70);
    Etykieta(p, z, TR("do"));
    tcDo_ = Pole(p, z, "20000", "audio.riaa_zakres", 70);
    Etykieta(p, z, TR("co [Hz]"));
    tcInterwal_ = Pole(p, z, "1000", "audio.riaa_zakres", 70);
    lbZakres_ = Wynik(p, z);
    s->Add(z, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxBoxSizer* srodek = new wxBoxSizer(wxHORIZONTAL);
    tabela_ = new wxListCtrl(p, wxID_ANY, wxDefaultPosition, wxSize(240, 220), wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_THEME);
    tabela_->InsertColumn(0, TR("f [Hz]"), wxLIST_FORMAT_RIGHT, 80);
    tabela_->InsertColumn(1, TR("dB"), wxLIST_FORMAT_RIGHT, 70);
    tabela_->InsertColumn(2, TR("faza [°]"), wxLIST_FORMAT_RIGHT, 70);
    PomocGUI::DodajPole(tabela_, "audio.riaa_tabela");
    srodek->Add(tabela_, 0, wxEXPAND | wxRIGHT, 10);
    wykres_ = new WykresRIAA(p);
    srodek->Add(wykres_, 1, wxEXPAND);
    s->Add(srodek, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

    s->Add(new wxStaticLine(p), 0, wxEXPAND | wxALL, 10);
    wxStaticText* opisSieci = new wxStaticText(p, wxID_ANY,
                            TR("Elementy RC biernej sieci korekcyjnej (odtwarzanie): sekcja A = R1 || C1 w szeregu i R2 || C2 do masy "
                               "(3180/318 µs), sekcja B = R3 szeregowo i C3 do masy (75 µs); sekcje rozdziela stopień wzmacniający."));
    opisSieci->Wrap(740);
    s->Add(opisSieci, 0, wxLEFT | wxRIGHT, 10);
    wxBoxSizer* e = new wxBoxSizer(wxHORIZONTAL);
    Etykieta(p, e, "R2");
    tcR2_ = Pole(p, e, "100k", "audio.riaa_siec", 70);
    Etykieta(p, e, "R3");
    tcR3_ = Pole(p, e, "10k", "audio.riaa_siec", 70);
    Etykieta(p, e, TR("rezystory z szeregu"));
    chSzeregR_ = WyborSzeregu(p, Szereg::E96);
    PomocGUI::DodajPole(chSzeregR_, "audio.riaa_siec.szereg_rezystorow");
    e->Add(chSzeregR_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    Etykieta(p, e, TR("kondensatory"));
    chSzeregC_ = WyborSzeregu(p, Szereg::E24);
    PomocGUI::DodajPole(chSzeregC_, "audio.riaa_siec.szereg_kondensatorow");
    e->Add(chSzeregC_, 0, wxALIGN_CENTER_VERTICAL);
    s->Add(e, 0, wxEXPAND | wxALL, 10);
    lbSiec_ = new wxStaticText(p, wxID_ANY, wxEmptyString);
    s->Add(lbSiec_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    p->SetSizer(s);

    for (wxTextCtrl* t : {tcF_}) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczRIAA(); });
    for (wxTextCtrl* t : {tcOd_, tcDo_, tcInterwal_}) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczTabele(); });
    for (wxTextCtrl* t : {tcR2_, tcR3_}) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczSiec(); });
    for (wxChoice* c : {chSzeregR_, chSzeregC_}) c->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczSiec(); });
    chRodzaj_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczRIAA(); PrzeliczTabele(); PrzeliczSiec(); });
    cbIec_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { PrzeliczRIAA(); PrzeliczTabele(); PrzeliczSiec(); });
    return p;
}

void DialogAudioLampy::PrzeliczRIAA() {
    using namespace KalkulatoryAudio;
    const bool odtw = chRodzaj_->GetSelection() == 0, iec = cbIec_->GetValue();
    const auto f = Czest(tcF_);
    if (!f || !(*f > 0)) { lbRiaaPunkt_->SetLabel(TR("Wpisz częstotliwość większą od zera.")); return; }
    const auto h = odpowiedzRIAA(*f, odtw, iec);
    lbRiaaPunkt_->SetLabel(Znak(tlumienieRIAAdB(*f, odtw, iec), 2) + " dB,  " + L(std::arg(h) * 180.0 / 3.14159265358979, 1) +
                           wxString::FromUTF8("\xC2\xB0  ") + TR("(względem 1 kHz)"));
}

void DialogAudioLampy::PrzeliczTabele() {
    using namespace KalkulatoryAudio;
    const bool odtw = chRodzaj_->GetSelection() == 0, iec = cbIec_->GetValue();
    tabela_->DeleteAllItems();
    const auto od = Czest(tcOd_), doo = Czest(tcDo_), iv = Czest(tcInterwal_);
    std::vector<PunktRIAA> t;
    if (od && doo && iv) t = tabelaRIAA(*od, *doo, *iv, odtw, iec, 2000);
    for (size_t i = 0; i < t.size(); ++i) {
        const long w = tabela_->InsertItem(static_cast<long>(i), L(t[i].fHz, 2));
        tabela_->SetItem(w, 1, Znak(t[i].db, 2));
        tabela_->SetItem(w, 2, L(t[i].faza, 1));
    }
    lbZakres_->SetLabel(t.empty() ? TR("Podaj zakres i dodatni interwał.") : wxString::Format(TR("Punktów: %zu"), t.size()));
    // wykres: 20 Hz..20 kHz, 12 punktów na dekadę
    const std::vector<double> f = czestotliwosciLogarytmiczne(20, 20000, 12);
    std::vector<double> idealna;
    for (double x : f) idealna.push_back(tlumienieRIAAdB(x, odtw, iec));
    std::vector<double> siec;
    const auto r2 = Opor(tcR2_), r3 = Opor(tcR3_);
    if (odtw && r2 && r3) {
        if (const auto s = projektujSiecRIAA(*r2, *r3)) {
            const SiecRIAA z = zaokraglijSiec(*s, Wybrany(chSzeregR_), Wybrany(chSzeregC_));
            const double ref = 20.0 * std::log10(std::abs(odpowiedzSieci(z, 1000.0)));
            for (double x : f) siec.push_back(20.0 * std::log10(std::abs(odpowiedzSieci(z, x))) - ref);
        }
    }
    wykres_->Ustaw(f, idealna, siec);
}

void DialogAudioLampy::PrzeliczSiec() {
    using namespace KalkulatoryAudio;
    PrzeliczTabele();
    const auto r2 = Opor(tcR2_), r3 = Opor(tcR3_);
    if (!r2 || !r3 || !(*r2 > 0) || !(*r3 > 0)) { lbSiec_->SetLabel(TR("Wpisz R2 i R3, np. 100k i 10k.")); return; }
    const auto s = projektujSiecRIAA(*r2, *r3);
    if (!s) { lbSiec_->SetLabel(TR("Wpisz R2 i R3 większe od zera.")); return; }
    const SiecRIAA z = zaokraglijSiec(*s, Wybrany(chSzeregR_), Wybrany(chSzeregC_));
    wxString t;
    t += TR("Obliczone:  ") + "R1 = " + Op(s->r1) + ",  C1 = " + Pf(s->c1) + ",  R2 = " + Op(s->r2) + ",  C2 = " + Pf(s->c2) +
         ",  R3 = " + Op(s->r3) + ",  C3 = " + Pf(s->c3) + "\n";
    t += TR("Z szeregów:  ") + "R1 = " + Op(z.r1) + ",  C1 = " + Pf(z.c1) + ",  R2 = " + Op(z.r2) + ",  C2 = " + Pf(z.c2) +
         ",  R3 = " + Op(z.r3) + ",  C3 = " + Pf(z.c3) + "\n";
    t += wxString::Format(TR("Największa odchyłka od krzywej RIAA (20 Hz..20 kHz): %s dB.  Tłumienie sieci przy 1 kHz: %s dB "
                             "(o tyle wzmacniacz musi dodać wzmocnienia)."),
                          L(maksOdchylkaSieci(z), 2), L(tlumienieSieciPrzy1kHz(z), 1));
    lbSiec_->SetLabel(t);
    lbSiec_->Wrap(740);
    lbSiec_->GetParent()->Layout();
}

// ---------------------------------------------------------------- dostęp dla testów

void DialogAudioLampy::UstawNapiecia(const wxString& u1, const wxString& u2) {
    tcU1_->ChangeValue(u1); tcU2_->ChangeValue(u2); PrzeliczDecybele();
}
void DialogAudioLampy::UstawMoce(const wxString& p1, const wxString& p2) {
    tcP1_->ChangeValue(p1); tcP2_->ChangeValue(p2); PrzeliczDecybele();
}
void DialogAudioLampy::UstawMiare(const wxString& miara, bool moc) {
    tcMiara_->ChangeValue(miara); chMiara_->SetSelection(moc ? 1 : 0); PrzeliczDecybele();
}
void DialogAudioLampy::UstawDb(const wxString& db, bool moc) {
    tcDb_->ChangeValue(db); chDb_->SetSelection(moc ? 1 : 0); PrzeliczDecybele();
}
void DialogAudioLampy::UstawNepery(const wxString& np) { tcNp_->ChangeValue(np); PrzeliczDecybele(); }
void DialogAudioLampy::UstawDbNaNepery(const wxString& db) { tcDbNp_->ChangeValue(db); PrzeliczDecybele(); }

void DialogAudioLampy::UstawRIAA(int rodzaj, bool iec, const wxString& czestotliwosc) {
    chRodzaj_->SetSelection(rodzaj);
    cbIec_->SetValue(iec);
    tcF_->ChangeValue(czestotliwosc);
    PrzeliczRIAA();
    PrzeliczTabele();
    PrzeliczSiec();
}
void DialogAudioLampy::UstawZakresRIAA(const wxString& od, const wxString& doo, const wxString& interwal) {
    tcOd_->ChangeValue(od); tcDo_->ChangeValue(doo); tcInterwal_->ChangeValue(interwal);
    PrzeliczTabele();
}
wxString DialogAudioLampy::KomorkaRIAA(int wiersz, int kolumna) const {
    if (wiersz < 0 || wiersz >= tabela_->GetItemCount()) return wxString();
    wxListItem it;
    it.SetId(wiersz);
    it.SetColumn(kolumna);
    it.SetMask(wxLIST_MASK_TEXT);
    tabela_->GetItem(it);
    return it.GetText();
}
void DialogAudioLampy::UstawSiecRIAA(const wxString& r2, const wxString& r3, int szeregR, int szeregC) {
    tcR2_->ChangeValue(r2); tcR3_->ChangeValue(r3);
    chSzeregR_->SetSelection(szeregR); chSzeregC_->SetSelection(szeregC);
    PrzeliczSiec();
}

// ---------------------------------------------------------------- punkt pracy lampy (polaryzacja katodowa)
wxPanel* DialogAudioLampy::StronaPunktuPracy() {
    wxPanel* p = new wxPanel(ksiazka_);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxStaticText* opis = new wxStaticText(p, wxID_ANY, TR("Punkt pracy ze stałą polaryzacją katodową (siatka na masie): Rk = |Ug| / (Ia + Ig2), Ua = B+ − Ia·Ra − |Ug|. Napięcie siatki Ug odczytaj z charakterystyk lampy."));
    opis->Wrap(560);
    s->Add(opis, 0, wxALL, 10);
    wxButton* bk = new wxButton(p, wxID_ANY, TR("Lampa z katalogu..."));
    PomocGUI::Dodaj(bk, "audio.lampa_katalog");
    bk->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnLampaDlaPunktuPracy(); });
    s->Add(bk, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    wxFlexGridSizer* g = new wxFlexGridSizer(4, 6, 8);
    auto dodaj = [&](const wxString& etykieta, const wxString& wartosc, const char* klucz) {
        g->Add(new wxStaticText(p, wxID_ANY, etykieta), 0, wxALIGN_CENTER_VERTICAL);
        wxTextCtrl* t = new wxTextCtrl(p, wxID_ANY, wartosc, wxDefaultPosition, wxSize(110, -1));
        PomocGUI::Dodaj(t, klucz);
        g->Add(t, 0);
        poleLM_.push_back(t);
    };
    dodaj(TR("Zasilanie B+ [V]:"), "250", "kalk.lm_b");
    dodaj(TR("Opór anodowy Ra [Ω]:"), "100k", "kalk.lm_ra");
    dodaj(TR("Prąd anody Ia [mA]:"), "0,8", "kalk.lm_ia");
    dodaj(TR("Napięcie siatki Ug [V]:"), "-1,5", "kalk.lm_ug");
    dodaj(TR("Prąd ekranu Ig2 [mA]:"), "", "kalk.lm_ig2");
    dodaj(TR("Napięcie ekranu Ug2 [V]:"), "", "kalk.lm_ug2");
    dodaj(TR("Amplifikacja μ:"), "100", "kalk.lm_mi");
    dodaj(TR("Rezystancja wewn. ra [Ω]:"), "62k5", "kalk.lm_rw");
    dodaj(TR("Moc anody max [W]:"), "1", "kalk.lm_pa");
    dodaj(TR("Dolna częstotliwość [Hz]:"), "20", "kalk.lm_f");
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbLM_ = new wxStaticText(p, wxID_ANY, wxEmptyString);
    lbLM_->SetFont(lbLM_->GetFont().Scaled(1.2f).Bold());
    s->Add(lbLM_, 0, wxALL, 10);
    wxStaticText* uwagi = new wxStaticText(p, wxID_ANY, TR("Pola Ig2 i Ug2 wypełnij tylko dla pentody lub tetrody. μ i ra są potrzebne do wzmocnienia i kondensatora katodowego (np. ECC83: μ = 100, ra = 62,5 k; ECC82: μ = 17, ra = 7,7 k)."));
    uwagi->Wrap(560);
    s->Add(uwagi, 0, wxALL, 10);
    PanelSchematu::Obok(p, s, PanelSchematu::Rodzaj::Lampa);
    for (wxTextCtrl* t : poleLM_) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczLampe(); });
    return p;
}

void DialogAudioLampy::PrzeliczLampe() {
    Kalkulatory::DaneLampy d;
    const auto b = Liczba(poleLM_[0]), ra = Opor(poleLM_[1]), ia = Liczba(poleLM_[2]), ug = Liczba(poleLM_[3]);
    const auto ig2 = OpcjaLiczba(poleLM_[4]), ug2 = OpcjaLiczba(poleLM_[5]), mi = OpcjaLiczba(poleLM_[6]), rw = OpcjaOpor(poleLM_[7]),
               pa = OpcjaLiczba(poleLM_[8]), f = OpcjaCzest(poleLM_[9]);
    if (!b || !ra || !ia || !ug || !ig2 || !ug2 || !mi || !rw || !pa || !f) { lbLM_->SetLabel(TR("Wpisz B+, Ra, Ia i Ug (np. 250, 100k, 0,8, -1,5).")); return; }
    d.bPlus = *b; d.ra = *ra; d.iaMA = *ia; d.ugV = *ug; d.ig2MA = *ig2; d.ug2V = *ug2; d.mi = *mi; d.rWewnAnodowaOm = *rw; d.paMaxW = *pa; d.fDolnaHz = *f;
    if (auto w = Kalkulatory::punktPracyLampy(d)) {
        wxString t = TR("Rk = ") + Op(w->rKatodyOm) + "  (" + L(w->mocRKatodyW * 1000, 1) + " mW)\n" +
                     TR("Ua = ") + L(w->uAnodyV, 1) + " V,  " + TR("moc anody ") + L(w->mocAnodyW, 2) + " W" +
                     (w->przekroczonaMocAnody ? "  " + TR("— PRZEKROCZONA moc dopuszczalna!") : wxString()) + "\n" +
                     TR("Linia obciążenia: od ") + L(d.bPlus, 0) + " V do " + L(w->iaMaxMA, 1) + " mA";
        if (w->rEkranuOm > 0) t += "\n" + TR("Rezystor ekranu Rg2 = ") + Op(w->rEkranuOm);
        if (w->wzmocnienie > 0) t += "\n" + TR("Wzmocnienie napięciowe: ") + L(w->wzmocnienie, 1) + TR(" V/V");
        if (w->ckF > 0) t += "\n" + TR("Kondensator katodowy: co najmniej ") + Pf(w->ckF);
        lbLM_->SetLabel(t);
    } else {
        lbLM_->SetLabel(TR("Niemożliwe: Ug musi być ujemne, a spadek na Ra mniejszy od B+."));
    }
}

void DialogAudioLampy::UstawPolaLampy(std::initializer_list<wxString> wartosci) {
    size_t i = 0;
    for (const wxString& w : wartosci) { if (i < poleLM_.size()) poleLM_[i]->ChangeValue(w); ++i; }
    PrzeliczLampe();
}

// ---------------------------------------------------------------- polaryzacja katodowa, sprzężenie RC, regulacja barwy

namespace {
std::optional<double> Kond(const wxTextCtrl* tc) {
    return SzeregiE::parsujWartosc(tc->GetValue().utf8_string(), RodzajElementu::Kondensator);
}
}

wxPanel* DialogAudioLampy::StronaKatody() {
    wxPanel* p = new wxPanel(ksiazka_);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(new wxStaticText(p, wxID_ANY, TR("Rk = Uk / (Ia + Ig2).  Ck: spadek wzmocnienia o 3 dB przy fd, z równania |(1 + jx) / (1 + gm·Rk + jx)| = 1/√2, czyli x = 2π·fd·Rk·Ck = √((1 + gm·Rk)² − 2).")),
           0, wxALL, 10);
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 10);
    auto pole = [&](const wxString& et, const wxString& wart, const char* klucz) {
        g->Add(new wxStaticText(p, wxID_ANY, et), 0, wxALIGN_CENTER_VERTICAL);
        wxTextCtrl* t = new wxTextCtrl(p, wxID_ANY, wart, wxDefaultPosition, wxSize(110, -1));
        PomocGUI::Dodaj(t, klucz);
        g->Add(t, 0);
        poleKat_.push_back(t);
    };
    pole(TR("Prąd anodowy Ia [mA]:"), "1,2", "audio.kat_ia");
    pole(TR("Prąd siatki ekranującej Ig2 [mA] (trioda: 0):"), "0", "audio.kat_ig2");
    pole(TR("Napięcie polaryzacji Uk [V]:"), "1,8", "audio.kat_uk");
    pole(TR("Konduktancja gm [mA/V] (puste: tylko reguła):"), "1,6", "audio.kat_gm");
    pole(TR("Dolna częstotliwość graniczna fd [Hz] (puste: bez Ck):"), "40", "audio.kat_fd");
    g->Add(new wxStaticText(p, wxID_ANY, TR("Szereg dla Rk:")), 0, wxALIGN_CENTER_VERTICAL);
    chKatR_ = WyborSzeregu(p, Szereg::E24);
    PomocGUI::DodajPole(chKatR_, "audio.kat_szereg");
    g->Add(chKatR_, 0);
    g->Add(new wxStaticText(p, wxID_ANY, TR("Szereg dla Ck:")), 0, wxALIGN_CENTER_VERTICAL);
    chKatC_ = WyborSzeregu(p, Szereg::E12);
    PomocGUI::DodajPole(chKatC_, "audio.kat_szereg");
    g->Add(chKatC_, 0);
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbKatoda_ = new wxStaticText(p, wxID_ANY, wxEmptyString);
    s->Add(lbKatoda_, 0, wxALL, 10);
    p->SetSizer(s);
    for (wxTextCtrl* t : poleKat_) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczKatode(); });
    for (wxChoice* c : {chKatR_, chKatC_}) c->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczKatode(); });
    return p;
}

void DialogAudioLampy::PrzeliczKatode() {
    using namespace KalkulatoryWzmacniaczy;
    if (poleKat_.size() < 5) return;
    auto ia = Liczba(poleKat_[0]), ig2 = OpcjaLiczba(poleKat_[1]), uk = Liczba(poleKat_[2]), gm = OpcjaLiczba(poleKat_[3]), fd = OpcjaCzest(poleKat_[4]);
    if (!ia || !ig2 || !uk || !gm || !fd) { lbKatoda_->SetLabel(TR("Wpisz Ia i Uk (liczby), pozostałe pola mogą być puste.")); return; }
    WejscieKatody w;
    w.iaMA = *ia; w.ig2MA = *ig2; w.ukV = *uk; w.gmMAV = *gm; w.fdHz = *fd;
    w.szeregR = Wybrany(chKatR_);
    w.szeregC = Wybrany(chKatC_);
    const auto r = polaryzacjaKatodowa(w);
    if (!r) { lbKatoda_->SetLabel(TR("Prąd i napięcie muszą być dodatnie.")); return; }
    wxString t = TR("Prąd katody Ik = ") + L(r->ikMA, 3) + " mA\n";
    t += TR("Rk = ") + Op(r->rkIdealny) + TR("  →  z szeregu ") + Op(r->rkSzereg) + TR("  (Uk = ") + L(r->ukRzeczywiste, 3) + " V)\n";
    t += TR("Moc rezystora ") + L(r->mocMW, 2) + TR(" mW, zalecany z zapasem x2: ") + L(r->mocZalecanaMW, 2) + " mW";
    if (r->maCk) {
        t += "\n\n";
        if (!r->ckPotrzebny)
            t += TR("Przy tak małym gm·Rk spadek wzmocnienia po wyłączeniu Ck nie sięga 3 dB: kondensator katodowy nie jest potrzebny.");
        else {
            t += TR("Ck dla spadku 3 dB przy fd: ") + Pf(r->ckUF * 1e-6) + TR("  →  z szeregu ") + Pf(r->ckSzeregUF * 1e-6) + "\n";
            t += TR("Napięcie pracy Ck co najmniej ") + L(r->napiecieCkV, 0) + " V\n";
            if (r->ckReguleUF > 0 && w.gmMAV > 0) t += TR("Reguła praktyczna (Xc = Rk/10 przy fd): ") + Pf(r->ckReguleUF * 1e-6);
            else t += TR("Bez gm podano tylko regułę praktyczną (Xc = Rk/10 przy fd).");
            if (r->ckSzeregUF > 0 && w.gmMAV > 0) {
                const double f = fKatodyHz(r->rkSzereg, r->ckSzeregUF * 1e-6, w.gmMAV * 1e-3);
                t += "\n" + TR("Z kondensatorem z szeregu częstotliwość -3 dB wynosi ") + L(f, 1) + " Hz";
            }
        }
    }
    lbKatoda_->SetLabel(t);
}

void DialogAudioLampy::UstawPolaKatody(std::initializer_list<wxString> wartosci) {
    size_t i = 0;
    for (const wxString& w : wartosci) if (i < poleKat_.size()) poleKat_[i++]->ChangeValue(w);
    PrzeliczKatode();
}

wxPanel* DialogAudioLampy::StronaSprzezenia() {
    wxPanel* p = new wxPanel(ksiazka_);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(new wxStaticText(p, wxID_ANY, TR("fc = 1 / (2π · (Ra ‖ ra + Rg) · Cs).  Dla n jednakowych sekcji RC w kaskadzie f(-3 dB) = fc / √(2^(1/n) − 1).")), 0, wxALL, 10);
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 10);
    auto pole = [&](const wxString& et, const wxString& wart, const char* klucz) {
        g->Add(new wxStaticText(p, wxID_ANY, et), 0, wxALIGN_CENTER_VERTICAL);
        wxTextCtrl* t = new wxTextCtrl(p, wxID_ANY, wart, wxDefaultPosition, wxSize(110, -1));
        PomocGUI::Dodaj(t, klucz);
        g->Add(t, 0);
        poleSprz_.push_back(t);
    };
    pole(TR("Obciążenie anodowe Ra poprzedniego stopnia:"), "100k", "audio.sprz_ra");
    pole(TR("Rezystancja wewnętrzna lampy ra (puste: pomiń):"), "62k", "audio.sprz_rwew");
    pole(TR("Rezystor siatki następnego stopnia Rg:"), "470k", "audio.sprz_rg");
    pole(TR("Kondensator sprzęgający Cs (np. 22n; puste: tylko dobór):"), "22n", "audio.sprz_cs");
    pole(TR("Żądana dolna częstotliwość fd [Hz] (puste: tylko f z Cs):"), "20", "audio.sprz_fd");
    g->Add(new wxStaticText(p, wxID_ANY, TR("Liczba jednakowych sekcji w kaskadzie:")), 0, wxALIGN_CENTER_VERTICAL);
    chSprzSekcje_ = new wxChoice(p, wxID_ANY);
    for (int i = 1; i <= 8; ++i) chSprzSekcje_->Append(wxString::Format("%d", i));
    chSprzSekcje_->SetSelection(0);
    PomocGUI::DodajPole(chSprzSekcje_, "audio.sprz_sekcje");
    g->Add(chSprzSekcje_, 0);
    g->Add(new wxStaticText(p, wxID_ANY, TR("Szereg dla Cs:")), 0, wxALIGN_CENTER_VERTICAL);
    chSprzC_ = WyborSzeregu(p, Szereg::E12);
    PomocGUI::DodajPole(chSprzC_, "audio.sprz_szereg");
    g->Add(chSprzC_, 0);
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbSprz_ = new wxStaticText(p, wxID_ANY, wxEmptyString);
    s->Add(lbSprz_, 0, wxALL, 10);
    p->SetSizer(s);
    for (wxTextCtrl* t : poleSprz_) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczSprzezenie(); });
    for (wxChoice* c : {chSprzSekcje_, chSprzC_}) c->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczSprzezenie(); });
    return p;
}

void DialogAudioLampy::PrzeliczSprzezenie() {
    using namespace KalkulatoryWzmacniaczy;
    if (poleSprz_.size() < 5) return;
    auto ra = Opor(poleSprz_[0]), rwew = OpcjaOpor(poleSprz_[1]), rg = Opor(poleSprz_[2]), fd = OpcjaCzest(poleSprz_[4]);
    std::optional<double> cs = poleSprz_[3]->GetValue().Trim().empty() ? std::optional<double>(0.0) : Kond(poleSprz_[3]);
    if (!ra || !rwew || !rg || !cs || !fd) { lbSprz_->SetLabel(TR("Wpisz Ra i Rg (np. 100k, 470k); Cs np. 22n.")); return; }
    WejscieSprzezenia w;
    w.raKOhm = *ra / 1e3; w.rwewKOhm = *rwew / 1e3; w.rgKOhm = *rg / 1e3; w.csNF = *cs * 1e9; w.fdHz = *fd;
    w.sekcje = chSprzSekcje_->GetSelection() + 1;
    w.szeregC = Wybrany(chSprzC_);
    const auto r = kondensatorSprzegajacy(w);
    if (!r) { lbSprz_->SetLabel(TR("Ra i Rg muszą być dodatnie.")); return; }
    wxString t = TR("Rezystancja widziana przez Cs: ") + Op(r->rcalkKOhm * 1e3) + "\n";
    if (r->maF) {
        t += TR("Częstotliwość graniczna jednej sekcji: ") + L(r->fSekcjiHz, 2) + " Hz\n";
        if (w.sekcje > 1) t += TR("Częstotliwość -3 dB całej kaskady (") + wxString::Format("%d", w.sekcje) + TR(" sekcji): ") + L(r->fCalkowitaHz, 2) + " Hz\n";
    }
    if (r->maCs) {
        t += "\n" + TR("Cs dla fd = ") + L(w.fdHz, 1) + " Hz: " + Pf(r->csPotrzebneNF * 1e-9) + TR("  →  z szeregu ") + Pf(r->csSzeregNF * 1e-9) + "\n";
        t += TR("Tłumienie całego układu przy fd z tą wartością: ") + L(r->spadekPrzyFdDb, 2) + " dB";
    }
    lbSprz_->SetLabel(t);
}

void DialogAudioLampy::UstawPolaSprzezenia(std::initializer_list<wxString> wartosci, int sekcje) {
    size_t i = 0;
    for (const wxString& w : wartosci) if (i < poleSprz_.size()) poleSprz_[i++]->ChangeValue(w);
    chSprzSekcje_->SetSelection(std::max(0, std::min(7, sekcje - 1)));
    PrzeliczSprzezenie();
}

wxPanel* DialogAudioLampy::StronaBarwy() {
    wxPanel* p = new wxPanel(ksiazka_);
    panelBarwy_ = p;
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(new wxStaticText(p, wxID_ANY, TR("Bierne układy regulacji barwy liczone z równań węzłowych (topologie i wartości wzorcowe: projekt tonestack, GPL-3.0, wzorowany na Duncan's Tone Stack Calculator). Wykres pokazuje straty układu względem napięcia źródła.")),
           0, wxALL, 10);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);
    gora->Add(new wxStaticText(p, wxID_ANY, TR("Układ:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    chPresetBarwy_ = new wxChoice(p, wxID_ANY);
    for (const auto& u : KalkulatoryWzmacniaczy::ukladyBarwy()) chPresetBarwy_->Append(wxString::FromUTF8(u.nazwa));
    chPresetBarwy_->SetSelection(0);
    PomocGUI::DodajPole(chPresetBarwy_, "audio.barwa_preset");
    gora->Add(chPresetBarwy_, 0, wxALIGN_CENTER_VERTICAL);
    s->Add(gora, 0, wxLEFT | wxRIGHT, 10);

    auto pole = [&](wxStaticText*& et, wxTextCtrl*& tc, const wxString& nazwa, const char* klucz) {
        et = new wxStaticText(p, wxID_ANY, nazwa);
        tc = new wxTextCtrl(p, wxID_ANY, "", wxDefaultPosition, wxSize(70, -1));
        PomocGUI::Dodaj(tc, klucz);
        tc->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczBarwe(); });
    };
    for (int i = 1; i <= 3; ++i) pole(etBR_[i], tcBR_[i], wxString::Format("R%d", i), "audio.barwa_r");
    for (int i = 1; i <= 4; ++i) pole(etBC_[i], tcBC_[i], wxString::Format("C%d", i), "audio.barwa_c");
    for (int i = 1; i <= 2; ++i) pole(etBL_[i], tcBL_[i], wxString::Format("L%d", i), "audio.barwa_l");
    pole(etBRzr_, tcBRzr_, TR("Rźr"), "audio.barwa_zr");
    pole(etBRobc_, tcBRobc_, TR("Robc"), "audio.barwa_obc");
    siatkaBarwy_ = new wxFlexGridSizer(10, 4, 8);
    s->Add(siatkaBarwy_, 0, wxALL, 10);

    siatkaPotBarwy_ = new wxFlexGridSizer(5, 4, 8);
    for (int i = 1; i <= 3; ++i) {
        etPot_[i] = new wxStaticText(p, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1));
        slPot_[i] = new wxSlider(p, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxSize(240, -1));
        PomocGUI::DodajPole(slPot_[i], "audio.barwa_suwaki");
        etPoz_[i] = new wxStaticText(p, wxID_ANY, "5,0", wxDefaultPosition, wxSize(30, -1));
        tcPotR_[i] = new wxTextCtrl(p, wxID_ANY, "", wxDefaultPosition, wxSize(70, -1));
        PomocGUI::Dodaj(tcPotR_[i], "audio.barwa_potr");
        chPotTaper_[i] = new wxChoice(p, wxID_ANY);
        for (const char* t : {"liniowy", "log", "log A", "odwrotny log"}) chPotTaper_[i]->Append(TekstZUTF8(t));
        PomocGUI::DodajPole(chPotTaper_[i], "audio.barwa_taper");
        slPot_[i]->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) { PrzeliczBarwe(); });
        tcPotR_[i]->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczBarwe(); });
        chPotTaper_[i]->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczBarwe(); });
    }
    s->Add(siatkaPotBarwy_, 0, wxLEFT | wxRIGHT, 10);

    lbBarwa_ = new wxStaticText(p, wxID_ANY, wxEmptyString);
    s->Add(lbBarwa_, 0, wxALL, 10);
    wykresBarwy_ = new WykresRIAA(p);
    wykresBarwy_->UstawZakres(-50, 0, 10);
    s->Add(wykresBarwy_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    p->SetSizer(s);

    chPresetBarwy_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { WczytajPresetBarwy(); PrzeliczBarwe(); });
    WczytajPresetBarwy();
    return p;
}

void DialogAudioLampy::WczytajPresetBarwy() {
    using namespace KalkulatoryWzmacniaczy;
    const auto& uk = ukladyBarwy();
    const int nr = chPresetBarwy_->GetSelection();
    if (nr < 0 || nr >= static_cast<int>(uk.size())) return;
    const UkladBarwy& u = uk[nr];
    wczytujeBarwe_ = true;
    auto wpisz = [&](wxTextCtrl* tc, double v, RodzajElementu r) { tc->ChangeValue(v > 0 ? TekstZUTF8(SzeregiE::formatWartosci(v, r)) : wxString("0")); };
    siatkaBarwy_->Clear(false);
    auto pokaz = [&](wxStaticText* et, wxTextCtrl* tc, bool uzyty) {
        et->Show(uzyty); tc->Show(uzyty);
        if (uzyty) { siatkaBarwy_->Add(et, 0, wxALIGN_CENTER_VERTICAL); siatkaBarwy_->Add(tc, 0); }
    };
    for (int i = 1; i <= 3; ++i) { wpisz(tcBR_[i], u.r[i], RodzajElementu::Rezystor); pokaz(etBR_[i], tcBR_[i], u.uzywa(Typ::R, i)); }
    for (int i = 1; i <= 4; ++i) { wpisz(tcBC_[i], u.c[i], RodzajElementu::Kondensator); pokaz(etBC_[i], tcBC_[i], u.uzywa(Typ::C, i)); }
    for (int i = 1; i <= 2; ++i) { wpisz(tcBL_[i], u.l[i], RodzajElementu::Cewka); pokaz(etBL_[i], tcBL_[i], u.uzywa(Typ::L, i)); }
    wpisz(tcBRzr_, u.rzr, RodzajElementu::Rezystor); pokaz(etBRzr_, tcBRzr_, true);
    wpisz(tcBRobc_, u.robc, RodzajElementu::Rezystor); pokaz(etBRobc_, tcBRobc_, true);

    siatkaPotBarwy_->Clear(false);
    for (int i = 1; i <= 3; ++i) {
        const bool uz = u.uzywaPot(i);
        for (wxWindow* w : std::initializer_list<wxWindow*>{etPot_[i], slPot_[i], etPoz_[i], tcPotR_[i], chPotTaper_[i]}) w->Show(uz);
        if (!uz) continue;
        etPot_[i]->SetLabel(wxString::FromUTF8(u.pot[i].nazwa));
        slPot_[i]->SetValue(50);
        wpisz(tcPotR_[i], u.pot[i].calkowita, RodzajElementu::Rezystor);
        chPotTaper_[i]->SetSelection(static_cast<int>(u.pot[i].taper));
        siatkaPotBarwy_->Add(etPot_[i], 0, wxALIGN_CENTER_VERTICAL);
        siatkaPotBarwy_->Add(slPot_[i], 0);
        siatkaPotBarwy_->Add(etPoz_[i], 0, wxALIGN_CENTER_VERTICAL);
        siatkaPotBarwy_->Add(tcPotR_[i], 0);
        siatkaPotBarwy_->Add(chPotTaper_[i], 0);
    }
    wczytujeBarwe_ = false;
    panelBarwy_->Layout();
}

void DialogAudioLampy::PrzeliczBarwe() {
    using namespace KalkulatoryWzmacniaczy;
    if (!wykresBarwy_ || wczytujeBarwe_ || !tcBRzr_) return;
    const auto& uk = ukladyBarwy();
    const int nr = chPresetBarwy_->GetSelection();
    if (nr < 0 || nr >= static_cast<int>(uk.size())) return;
    UkladBarwy u = uk[nr];
    bool blad = false;
    auto pobierz = [&](wxTextCtrl* tc, RodzajElementu r, double& v, bool zeroDozwolone) {
        const wxString t = tc->GetValue().Trim();
        if (zeroDozwolone && (t.empty() || t == "0")) { v = 0; return; }
        auto w = SzeregiE::parsujWartosc(t.utf8_string(), r);
        if (!w) { blad = true; return; }
        v = *w;
    };
    for (int i = 1; i <= 3; ++i) if (u.uzywa(Typ::R, i)) pobierz(tcBR_[i], RodzajElementu::Rezystor, u.r[i], false);
    for (int i = 1; i <= 4; ++i) if (u.uzywa(Typ::C, i)) pobierz(tcBC_[i], RodzajElementu::Kondensator, u.c[i], false);
    for (int i = 1; i <= 2; ++i) if (u.uzywa(Typ::L, i)) pobierz(tcBL_[i], RodzajElementu::Cewka, u.l[i], false);
    pobierz(tcBRzr_, RodzajElementu::Rezystor, u.rzr, true);
    pobierz(tcBRobc_, RodzajElementu::Rezystor, u.robc, true);
    NastawyBarwy n;
    wxString poziomy;
    for (int i = 1; i <= 3; ++i) {
        if (!u.uzywaPot(i)) continue;
        pobierz(tcPotR_[i], RodzajElementu::Rezystor, u.pot[i].calkowita, false);
        u.pot[i].taper = static_cast<Taper>(std::max(0, chPotTaper_[i]->GetSelection()));
        n.poz[i] = slPot_[i]->GetValue() / 100.0;
        etPoz_[i]->SetLabel(L(slPot_[i]->GetValue() / 10.0, 1));
    }
    auto nieDodatnie = [&]() {
        for (const Skladnik& s : u.skladniki) {
            if (s.typ == Typ::R && !(u.r[s.numer] > 0)) return true;
            if (s.typ == Typ::C && !(u.c[s.numer] > 0)) return true;
            if (s.typ == Typ::L && !(u.l[s.numer] > 0)) return true;
            if ((s.typ == Typ::PotLo || s.typ == Typ::PotHi) && !(u.pot[s.numer].calkowita > 0)) return true;
        }
        return false;
    };
    if (blad || nieDodatnie()) {
        lbBarwa_->SetLabel(TR("Wpisz wartości elementów (np. 250k, 22n, 470p, 6H); rezystory, kondensatory i cewki muszą być większe od zera."));
        wykresBarwy_->Ustaw({}, {}, {});
        return;
    }
    const ParametryUkladu ps = parametryUkladu(u, n);
    wxString t = TR("Poziom: 100 Hz ") + Znak(ps.poziom100Hz, 1) + TR(" dB, 400 Hz ") + Znak(ps.poziom400Hz, 1) + TR(" dB, 1 kHz ") + Znak(ps.poziom1kHz, 1) +
                 TR(" dB, 4 kHz ") + Znak(ps.poziom4kHz, 1) + TR(" dB, 10 kHz ") + Znak(ps.poziom10kHz, 1) + " dB\n";
    t += TR("Najgłębsze wycięcie w paśmie 100 Hz do 3 kHz: ") + Znak(ps.minimumDb, 1) + TR(" dB przy ") + L(ps.fMinimumHz, 0) + TR(" Hz; najwyższy poziom: ") + Znak(ps.maksimumDb, 1) + " dB";
    lbBarwa_->SetLabel(t);
    const std::vector<double> f = KalkulatoryAudio::czestotliwosciLogarytmiczne(20, 20000, 40);
    std::vector<double> db;
    for (double x : f) db.push_back(poziomUkladuDb(u, n, x));
    wykresBarwy_->Ustaw(f, db, {});
}

void DialogAudioLampy::UstawBarwe(int uklad, double poz1, double poz2, double poz3) {
    chPresetBarwy_->SetSelection(uklad);
    WczytajPresetBarwy();
    const double p[4] = {0, poz1, poz2, poz3};
    for (int i = 1; i <= 3; ++i) slPot_[i]->SetValue(static_cast<int>(std::lround(p[i] * 100)));
    PrzeliczBarwe();
}

int DialogAudioLampy::LiczbaPotBarwy() const {
    int n = 0;
    for (int i = 1; i <= 3; ++i) if (slPot_[i]->IsShown()) ++n;
    return n;
}

wxString DialogAudioLampy::NazwaPotBarwy(int nr) const {
    return nr >= 1 && nr <= 3 && slPot_[nr]->IsShown() ? etPot_[nr]->GetLabel() : wxString();
}

void DialogAudioLampy::UstawTaperPotBarwy(int nr, int taper) {
    if (nr < 1 || nr > 3) return;
    chPotTaper_[nr]->SetSelection(std::max(0, std::min(3, taper)));
    PrzeliczBarwe();
}
