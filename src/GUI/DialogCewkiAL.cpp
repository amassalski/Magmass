#include "KoloryMotywu.h"
#include "DialogCewkiAL.h"
#include "../Logika/CewkaAL.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

namespace {
const char* kNazwy[7] = {"AL", "L", "tol", "sprawdz", "Ae", "I", "Bs"};
const char* kKlucze[8] = {"n", "zwoje", "L", "blad", "zakres", "L_sprawdz", "blad_sprawdz", "B"};
const double kJednostki[3] = {1e-6, 1e-3, 1.0};
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, m)); }
}

DialogCewkiAL::DialogCewkiAL(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Cewka na rdzeniu z AL"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* par = new wxStaticBoxSizer(wxVERTICAL, this, TR("Parametry"));
    wxWindow* wp = par->GetStaticBox();
    wxFlexGridSizer* g = new wxFlexGridSizer(0, 3, 4, 6);
    const char* ep[7] = {"Współczynnik AL [nH/zw²] =", "Żądana indukcyjność L =", "Tolerancja AL [±%] =", "Zwoje do sprawdzenia (opcjonalnie) =",
                         "Przekrój rdzenia Ae [mm²] (opcjonalnie) =", "Prąd szczytowy [A] (opcjonalnie) =", "Dopuszczalna indukcja [T] ="};
    const char* dp[7] = {"7000", "770", "0", "", "", "", "0,3"};
    const char* hk[7] = {"cewka.al", "cewka.l", "cewka.tol", "cewka.sprawdz", "cewka.indukcja", "cewka.indukcja", "cewka.indukcja"};
    for (int k = 0; k < 7; k++) {
        g->Add(new wxStaticText(wp, wxID_ANY, TR(ep[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[k] = new wxTextCtrl(wp, wxID_ANY, dp[k], wxDefaultPosition, wxSize(90, -1));
        PomocGUI::Dodaj(tc_[k], hk[k]);
        g->Add(tc_[k]);
        if (k == 1) {
            chJ_ = new wxChoice(wp, wxID_ANY);
            for (const char* j : {"µH", "mH", "H"}) chJ_->Append(wxString::FromUTF8(j));
            chJ_->SetSelection(1);
            PomocGUI::DodajPole(chJ_, "cewka.jednostka");
            g->Add(chJ_);
        } else {
            g->Add(new wxStaticText(wp, wxID_ANY, ""));
        }
    }
    par->Add(g, 0, wxALL, 6);
    gora->Add(par, 0, wxEXPAND);

    wxStaticBoxSizer* wyn = new wxStaticBoxSizer(wxVERTICAL, this, TR("Wyniki"));
    wxWindow* ww = wyn->GetStaticBox();
    wxFlexGridSizer* gw = new wxFlexGridSizer(0, 2, 4, 8);
    const char* ew[8] = {"Zwoje dokładnie N = √(L/AL) =", "Zwoje (zaokrąglone) =", "Indukcyjność dla tej liczby zwojów [mH] =", "Odchyłka od żądanej [%] =",
                         "Zwoje dla granic tolerancji AL =", "Indukcyjność dla zwojów do sprawdzenia [mH] =", "... odchyłka od żądanej [%] =", "Indukcja szczytowa B [T] ="};
    for (int k = 0; k < kWyn; k++) {
        gw->Add(new wxStaticText(ww, wxID_ANY, TR(ew[k])), 0, wxALIGN_RIGHT);
        lb_[k] = new wxStaticText(ww, wxID_ANY, "", wxDefaultPosition, wxSize(150, -1));
        lb_[k]->SetForegroundColour(k >= 5 && k < 7 ? Kolory::Szary() : Kolory::Wynik());
        PomocGUI::Dodaj(lb_[k], k >= 5 && k < 7 ? "cewka.sprawdz" : "cewka.wyniki");
        gw->Add(lb_[k]);
    }
    wyn->Add(gw, 0, wxALL, 6);
    gora->Add(wyn, 0, wxEXPAND | wxLEFT, 8);
    glowny->Add(gora, 0, wxALL, 8);
    lbUwagi_ = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxSize(640, 60));
    lbUwagi_->SetForegroundColour(Kolory::Ostrzezenie());
    glowny->Add(lbUwagi_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    glowny->Add(new wxButton(this, wxID_CLOSE, TR("Zamknij")), 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 8);
    SetSizerAndFit(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    chJ_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    for (wxTextCtrl* t : tc_) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Przelicz(); });
    Przelicz();
}

void DialogCewkiAL::UstawPole(const wxString& n, const wxString& v) {
    for (int k = 0; k < kPol; k++) if (n == kNazwy[k]) tc_[k]->SetValue(v);
}
void DialogCewkiAL::UstawJednostke(int numer) {
    chJ_->SetSelection(numer);
    Przelicz();
}
wxString DialogCewkiAL::Wynik(const wxString& n) const {
    for (int k = 0; k < kWyn; k++) if (n == kKlucze[k]) return wyn_[k];
    return wxString();
}

void DialogCewkiAL::Przelicz() {
    for (wxString& s : wyn_) s = "";
    for (wxStaticText* l : lb_) l->SetLabel("");
    lbUwagi_->SetLabel("");
    double v[kPol] = {0, 0, 0, 0, 0, 0, 0.3};
    bool ok = true;
    for (int k = 0; k < kPol; k++) {
        const std::string t = tc_[k]->GetValue().utf8_string();
        if (t.empty() && (k == 3 || k == 4 || k == 5)) continue;   // pola opcjonalne
        if (!tekstNaLiczbe(t, v[k])) ok = false;
    }
    CewkaAL::Wejscie w;
    w.alNH = v[0]; w.indukcyjnoscH = v[1] * kJednostki[chJ_->GetSelection()]; w.tolerancjaAlProc = v[2]; w.zwojeSprawdz = v[3];
    w.aeMm2 = v[4]; w.pradA = v[5]; w.bNasyceniaT = v[6];
    const auto o = ok ? CewkaAL::oblicz(w) : std::nullopt;
    if (!o) {
        lbUwagi_->SetLabel(TR("Uwagi: wpisz AL i żądaną indukcyjność (liczby większe od zera); tolerancja poniżej 100%."));
        return;
    }
    wyn_[0] = L(o->zwojeDokladne, 2);
    wyn_[1] = L(o->zwoje, 0);
    wyn_[2] = L(o->indukcyjnoscWykonanaH * 1e3, 3);
    wyn_[3] = L(o->bladProc, 2);
    if (w.tolerancjaAlProc > 0) wyn_[4] = L(o->zwojeMin, 1) + " ... " + L(o->zwojeMax, 1);
    if (w.zwojeSprawdz > 0) { wyn_[5] = L(o->indukcyjnoscSprawdzH * 1e3, 3); wyn_[6] = L(o->bladSprawdzProc, 2); }
    if (o->bPoprawkaT > 0) wyn_[7] = L(o->bPoprawkaT, 3);
    for (int k = 0; k < kWyn; k++) lb_[k]->SetLabel(wyn_[k]);
    wxString uw;
    if (w.zwojeSprawdz > 0 && std::abs(o->bladSprawdzProc) > 1.0)
        uw += wxString::Format(TR("Dla %s zwojów z podanym AL wychodzi %s mH, czyli %s%% względem żądanej indukcyjności; wzór L = AL·N² jest definicją AL, więc różnicę wyjaśnia inna wartość AL lub inna definicja w drugim programie. "),
                               L(w.zwojeSprawdz, 0), L(o->indukcyjnoscSprawdzH * 1e3, 1), L(o->bladSprawdzProc, 1));
    if (o->przekroczonaIndukcja) uw += TR("Indukcja szczytowa przekracza dopuszczalną - AL maleje, a rdzeń się nasyca; weź rdzeń o większym Ae lub ze szczeliną. ");
    uw += TR("AL podaje producent dla małego sygnału bez prądu stałego; przy polaryzacji prądem stałym efektywne AL jest mniejsze (potrzeba więcej zwojów lub szczeliny).");
    lbUwagi_->SetLabel(uw);
}
