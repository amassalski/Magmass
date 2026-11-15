#include "KoloryMotywu.h"
#include "DialogZnieksztalcen.h"
#include "../Logika/Znieksztalcenia.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <cmath>

namespace {
const char* kPola[12] = {"imax", "i1", "i0", "i2", "imin", "a", "b", "c", "d", "e", "f", "g"};
const char* kEtyk[12] = {"Ia max [mA]", "Ia1 [mA]", "Ia0 [mA]", "Ia2 [mA]", "Ia min [mA]", "a", "b", "c", "d", "e", "f", "g"};
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, 0)); }
int Indeks(const wxString& n) {
    for (int k = 0; k < 12; k++) if (n == kPola[k]) return k;
    return -1;
}
}

DialogZnieksztalcen::DialogZnieksztalcen(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Zniekształcenia harmoniczne"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(new wxStaticText(this, wxID_ANY, TR("Metoda pięciu rzędnych: odczytaj z prostej obciążenia prądy dla wysterowania +Um, +0,5 Um, 0, -0,5 Um, -Um albo zmierz odcinki a..g.")), 0, wxALL, 10);
    wxBoxSizer* tryb = new wxBoxSizer(wxHORIZONTAL);
    rbPrady_ = new wxRadioButton(this, wxID_ANY, TR("Oblicz dla prądów"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    rbOdcinki_ = new wxRadioButton(this, wxID_ANY, TR("Oblicz dla odcinków"));
    PomocGUI::Dodaj(rbPrady_, "znieksz.tryb");
    PomocGUI::Dodaj(rbOdcinki_, "znieksz.tryb");
    tryb->Add(rbPrady_, 0, wxRIGHT, 12);
    tryb->Add(rbOdcinki_);
    s->Add(tryb, 0, wxLEFT | wxRIGHT, 10);
    wxFlexGridSizer* g = new wxFlexGridSizer(0, 4, 6, 8);
    const char* domyslne[12] = {"93", "67,5", "41", "20", "7", "32,55", "21,9", "30,1", "16,3", "16,6", "18,9", "8,25"};
    for (int k = 0; k < 12; k++) {
        g->Add(new wxStaticText(this, wxID_ANY, TR(kEtyk[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[k] = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(domyslne[k]), wxDefaultPosition, wxSize(90, -1));
        PomocGUI::Dodaj(tc_[k], k < 5 ? "znieksz.prady" : "znieksz.odcinki");
        g->Add(tc_[k]);
        tc_[k]->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Przelicz(); });
    }
    s->Add(g, 0, wxALL, 10);
    wxFlexGridSizer* w = new wxFlexGridSizer(0, 3, 4, 14);
    const char* nz[4] = {"h [%]", "h2 [%]", "h3 [%]", "h4 [%]"};
    for (int k = 0; k < 4; k++) {
        w->Add(new wxStaticText(this, wxID_ANY, nz[k]), 0, wxALIGN_RIGHT);
        lbProc_[k] = new wxStaticText(this, wxID_ANY, "");
        lbProc_[k]->SetForegroundColour(Kolory::Wynik());
        lbDb_[k] = new wxStaticText(this, wxID_ANY, "");
        lbDb_[k]->SetForegroundColour(Kolory::Wynik());
        w->Add(lbProc_[k]);
        w->Add(lbDb_[k]);
        PomocGUI::Dodaj(lbProc_[k], "znieksz.wyniki");
    }
    s->Add(w, 0, wxLEFT | wxRIGHT, 10);
    lbUwagi_ = new wxStaticText(this, wxID_ANY, "");
    lbUwagi_->SetForegroundColour(Kolory::Ostrzezenie());
    s->Add(lbUwagi_, 0, wxALL, 10);
    wxBoxSizer* b = new wxBoxSizer(wxHORIZONTAL);
    wxButton* cz = new wxButton(this, wxID_ANY, TR("Wyczyść dane"));
    cz->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Wyczysc(); });
    b->Add(cz, 0, wxRIGHT, 8);
    b->Add(new wxButton(this, wxID_CLOSE, TR("Zamknij")));
    s->Add(b, 0, wxALIGN_RIGHT | wxALL, 10);
    // po prawej rysunek poglądowy i opis tego, co mierzyć (zmienia się z wybraną opcją)
    wxStaticBoxSizer* rys = new wxStaticBoxSizer(wxVERTICAL, this, TR("Co odczytać z wykresu"));
    wykres_ = new PanelWykresuZnieksztalcen(rys->GetStaticBox());
    opisWykresu_ = new wxStaticText(rys->GetStaticBox(), wxID_ANY, "");
    opisWykresu_->SetMinSize(wxSize(380, 150));
    rys->Add(wykres_, 1, wxEXPAND | wxALL, 4);
    rys->Add(opisWykresu_, 0, wxEXPAND | wxALL, 6);
    wxBoxSizer* calosc = new wxBoxSizer(wxHORIZONTAL);
    calosc->Add(s, 0);
    calosc->Add(rys, 1, wxEXPAND | wxALL, 10);
    SetSizerAndFit(calosc);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    for (wxRadioButton* r : {rbPrady_, rbOdcinki_}) r->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { Przelicz(); });
    Przelicz();
}

void DialogZnieksztalcen::UstawTryb(bool odcinki) {
    (odcinki ? rbOdcinki_ : rbPrady_)->SetValue(true);
    Przelicz();
}
void DialogZnieksztalcen::UstawPole(const wxString& n, const wxString& v) {
    if (int k = Indeks(n); k >= 0) tc_[k]->SetValue(v);
}
void DialogZnieksztalcen::Wyczysc() {
    for (wxTextCtrl* t : tc_) t->ChangeValue("");
    Przelicz();
}
wxString DialogZnieksztalcen::Wynik(const wxString& n) const {
    static const char* kl[14] = {"h", "h2", "h3", "h4", "h_db", "h2_db", "h3_db", "h4_db", "i0", "i1", "i2", "i3", "i4", ""};
    for (int k = 0; k < 13; k++) if (n == kl[k]) return wyn_[k];
    return wxString();
}

void DialogZnieksztalcen::Przelicz() {
    using namespace Znieksztalcenia;
    const bool od = rbOdcinki_->GetValue();
    for (int k = 0; k < 12; k++) tc_[k]->Enable(od ? k >= 5 : k < 5);
    wykres_->UstawTryb(od);
    opisWykresu_->SetLabel(od ?
        TR("Na prostej obciążenia zaznacz pięć punktów (przecięcia z charakterystykami dla Ug0 ±Um, Ug0 ±0,5Um i Ug0). Zmierz linijką kolejne odcinki między nimi: d, e, f, g. Odcinki a = d+e, b = f+g i c = e+f są ich sumami; do h2 i h3 wystarczą a, b, c, do h4 potrzebne są d, e, f, g.") :
        TR("Narysuj prostą obciążenia i zaznacz jej przecięcia z charakterystykami dla Ug0 ±Um, Ug0 ±0,5Um i Ug0. Z każdego punktu poprowadź poziomą linię do osi Ia i odczytaj prądy anodowe: Ia max, Ia1, Ia0, Ia2, Ia min [mA]."));
    opisWykresu_->Wrap(370);
    double v[12] = {};
    bool ok = true;
    const int pocz = od ? 5 : 0, kon = od ? 12 : 5;
    for (int k = pocz; k < kon; k++) if (!tekstNaLiczbe(tc_[k]->GetValue().utf8_string(), v[k])) ok = false;
    std::optional<Harmoniczne> r;
    if (ok) r = od ? zOdcinkow(v[5], v[6], v[7], v[8], v[9], v[10], v[11]) : zPradow(v[0], v[1], v[2], v[3], v[4]);
    for (wxString& x : wyn_) x = "";
    for (int k = 0; k < 4; k++) { lbProc_[k]->SetLabel(""); lbDb_[k]->SetLabel(""); }
    if (!r) {
        lbUwagi_->SetLabel(ok ? TR("Uwagi: pierwsza harmoniczna jest równa zero (brak wysterowania).") : TR("Uwagi: uzupełnij pola liczbami."));
        return;
    }
    lbUwagi_->SetLabel("");
    const double p[4] = {r->h, r->h2, r->h3, r->h4};
    for (int k = 0; k < 4; k++) {
        const double db = Harmoniczne::dB(p[k]);
        wyn_[k] = L(p[k], 3);
        wyn_[4 + k] = std::isinf(db) ? wxString("-inf") : L(db, 2);
        lbProc_[k]->SetLabel(wyn_[k]);
        lbDb_[k]->SetLabel(wyn_[4 + k] + " dB");
    }
    wyn_[8] = L(r->i0, 3); wyn_[9] = L(r->i1, 3); wyn_[10] = L(r->i2, 3); wyn_[11] = L(r->i3, 3); wyn_[12] = L(r->i4, 3);
    Layout();
}
