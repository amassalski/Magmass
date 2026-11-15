#include "KoloryMotywu.h"
#include "DialogTrafoSieciowy.h"
#include "ZestawienieDrutu.h"
#include "../Logika/TrafoSieciowy.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <sstream>

namespace {
const char* kNazwy[17] = {"A", "B", "a", "b", "c", "d", "h", "U1", "B_Gs", "eta", "j", "km", "kFe", "beta", "gamma", "scianka", "margines"};
const char* kKlucze[8] = {"moc_wtorna", "moc_pierwotna", "prad_pierwotny", "iloczyn_wym", "iloczyn_pos", "zwoje1", "zapas", "przekroj"};
const char* kEtWyn[8] = {"Moc wtórna [VA] =", "Moc pierwotna [VA] =", "Prąd pierwotny [A] =", "Wymagane q_o·q_z [cm⁴] =",
                         "Posiadane q_o·q_z [cm⁴] =", "Zwoje uzwojenia pierwotnego =", "Zapas w oknie δ [mm] =", "Przekrój żelaza q_z [cm²] ="};
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, m)); }
int Idx(const wxString& n) {
    for (int k = 0; k < 16; k++) if (n == kNazwy[k]) return k;
    return -1;
}
}

DialogTrafoSieciowy::DialogTrafoSieciowy(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Transformator sieciowy"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* rdz = new wxStaticBoxSizer(wxHORIZONTAL, this, TR("Rdzeń"));
    wxWindow* wr = rdz->GetStaticBox();
    wxFlexGridSizer* g = new wxFlexGridSizer(0, 2, 4, 6);
    const char* et[7] = {"A [mm] =", "B [mm] =", "a [mm] =", "b [mm] =", "c [mm] =", "d [mm] =", "h [mm] ="};
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
    rdz->Add(g, 0, wxALL, 4);
    rys_ = new PanelRdzenia(wr);
    rdz->Add(rys_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 4);   // góra rysunku = góra pola A, dół = dół pola Kształtka
    gora->Add(rdz, 0, wxALIGN_TOP);

    wxStaticBoxSizer* par = new wxStaticBoxSizer(wxVERTICAL, this, TR("Parametry transformatora"));
    wxWindow* wp = par->GetStaticBox();
    wxFlexGridSizer* gp = new wxFlexGridSizer(0, 2, 4, 6);
    const char* ep[10] = {"Napięcie zasilania [V] =", "Indukcja B [Gs] =", "Sprawność η =", "Gęstość prądu j [A/mm²] =", "Wsp. wypełnienia okna Cu =",
                         "Wsp. wypełnienia rdzenia Fe =", "Przekładki między warstwami [mm] =", "Izolacja między uzwojeniami [mm] =", "Ścianka karkasu [mm] =", "Margines warstwy [mm] ="};
    const char* dp[10] = {"230", "8000", "0,85", "2,5", "0,3", "0,9", "0,07", "1", "1,5", "1,5"};
    for (int k = 0; k < 10; k++) {
        gp->Add(new wxStaticText(wp, wxID_ANY, TR(ep[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[7 + k] = new wxTextCtrl(wp, wxID_ANY, dp[k], wxDefaultPosition, wxSize(70, -1));
        PomocGUI::Dodaj(tc_[7 + k], k == 0 ? "trafosiec.zasilanie" : "trafosiec.parametry");
        gp->Add(tc_[7 + k]);
    }
    par->Add(gp, 0, wxALL, 4);
    wxButton* bTyp = new wxButton(wp, wxID_ANY, TR("Wartości typowe wg mocy"));
    par->Add(bTyp, 0, wxALIGN_CENTER | wxBOTTOM, 4);

    wxStaticBoxSizer* wyn = new wxStaticBoxSizer(wxVERTICAL, this, TR("Wyniki obliczeń"));
    wxWindow* ww = wyn->GetStaticBox();
    wxFlexGridSizer* gw = new wxFlexGridSizer(0, 4, 4, 10);   // dwie kolumny par (etykieta, wynik), po cztery wiersze
    wxWindow* komorki[16] = {};
    for (int k = 0; k < 8; k++) {
        wxStaticText* et = new wxStaticText(ww, wxID_ANY, TR(kEtWyn[k]));
        lbWyn_[k] = new wxStaticText(ww, wxID_ANY, "", wxDefaultPosition, wxSize(70, -1));
        lbWyn_[k]->SetForegroundColour(Kolory::Wynik());
        PomocGUI::Dodaj(lbWyn_[k], "trafosiec.wyniki");
        const int w = k % 4, kol = k / 4;
        komorki[(w * 2 + kol) * 2] = et;
        komorki[(w * 2 + kol) * 2 + 1] = lbWyn_[k];
    }
    for (int i = 0; i < 16; i++) {
        if (i % 2 == 0) gw->Add(komorki[i], 0, wxALIGN_RIGHT);
        else gw->Add(komorki[i]);
    }
    gw->AddGrowableCol(0, 1);
    gw->AddGrowableCol(2, 1);
    wyn->Add(gw, 1, wxEXPAND | wxALL, 6);
    gora->Add(par, 1, wxEXPAND | wxLEFT, 8);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 8);
    glowny->Add(wyn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    wxStaticBoxSizer* tab = new wxStaticBoxSizer(wxHORIZONTAL, this, TR("Uzwojenia"));
    wxWindow* wt = tab->GetStaticBox();
    tcTabela_ = new wxTextCtrl(wt, wxID_ANY, "", wxDefaultPosition, wxSize(560, 120), wxTE_MULTILINE | wxTE_READONLY);
    tcTabela_->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    PomocGUI::Dodaj(tcTabela_, "trafosiec.tabela");
    tab->Add(tcTabela_, 1, wxEXPAND | wxALL, 4);
    // uzwojenia wtórne wpisuje się obok układu uzwojeń; przykład jest w dymku
    wxBoxSizer* wt2 = new wxBoxSizer(wxVERTICAL);
    wt2->Add(new wxStaticText(wt, wxID_ANY, TR("Wpisz uzwojenia wtórne:")), 0, wxBOTTOM, 2);
    tcWtorne_ = new wxTextCtrl(wt, wxID_ANY, "300 0,1\n6,3 1,8", wxDefaultPosition, wxSize(240, 100), wxTE_MULTILINE);
    PomocGUI::Dodaj(tcWtorne_, "trafosiec.wtorne");
    wt2->Add(tcWtorne_, 1, wxEXPAND);
    tab->Add(wt2, 0, wxEXPAND | wxALL, 4);
    glowny->Add(tab, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    lbUwagi_ = new wxStaticText(this, wxID_ANY, "");
    lbUwagi_->SetForegroundColour(Kolory::Ostrzezenie());
    glowny->Add(lbUwagi_, 0, wxALL, 8);
    tcDrut_ = ZestawienieDrutu::Dodaj(this, glowny, "drut.zestawienie");
    glowny->Add(new wxButton(this, wxID_CLOSE, TR("Zamknij")), 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 8);

    SetSizerAndFit(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    bTyp->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Typowe(); });
    chKsztaltka_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { ZKsztaltki(); });
    for (int k = 0; k < kPol; k++)
        tc_[k]->Bind(wxEVT_TEXT, [this, k](wxCommandEvent&) {
            if (!wypelniam_ && k < 7 && chKsztaltka_->GetSelection() != static_cast<int>(chKsztaltka_->GetCount()) - 1)
                chKsztaltka_->SetSelection(chKsztaltka_->GetCount() - 1);
            Przelicz();
        });
    tcWtorne_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Przelicz(); });
    tc_[3]->ChangeValue("51");
    ZKsztaltki();
}

void DialogTrafoSieciowy::ZKsztaltki() {
    double pakiet = 0;
    tekstNaLiczbe(tc_[3]->GetValue().utf8_string(), pakiet);
    if (auto r = Dlawik::ksztaltkaZNazwy(chKsztaltka_->GetStringSelection().utf8_string(), pakiet > 0 ? pakiet : 51)) {
        wypelniam_ = true;
        const double v[7] = {r->A, r->B, r->a, r->b, r->c, r->d, r->h};
        for (int k = 0; k < 7; k++) tc_[k]->ChangeValue(L(v[k], 0).BeforeFirst(','));
        wypelniam_ = false;
    }
    Przelicz();
}

void DialogTrafoSieciowy::UstawKsztaltke(const wxString& nazwa) {
    const int i = chKsztaltka_->FindString(nazwa);
    if (i != wxNOT_FOUND) chKsztaltka_->SetSelection(i);
    ZKsztaltki();
}
void DialogTrafoSieciowy::UstawPole(const wxString& n, const wxString& v) {
    if (int k = Idx(n); k >= 0) tc_[k]->SetValue(v);
}
wxString DialogTrafoSieciowy::Wynik(const wxString& n) const {
    for (int k = 0; k < 8; k++) if (n == kKlucze[k]) return wyn_[k];
    return wxString();
}

namespace {
bool Uzwojenia(const wxString& tekst, std::vector<TrafoSieciowy::Uzwojenie>& out) {
    std::istringstream in(tekst.utf8_string());
    std::string linia;
    out.clear();
    while (std::getline(in, linia)) {
        for (char& ch : linia) if (ch == ';' || ch == '\t' || ch == '\r') ch = ' ';
        std::istringstream l(linia);
        std::string a, b;
        if (!(l >> a)) continue;                      // pusty wiersz
        TrafoSieciowy::Uzwojenie u;
        if (!(l >> b) || !tekstNaLiczbe(a, u.napiecieV) || !tekstNaLiczbe(b, u.pradA)) return false;
        out.push_back(u);
    }
    return !out.empty();
}
}

void DialogTrafoSieciowy::Typowe() {
    std::vector<TrafoSieciowy::Uzwojenie> u;
    if (!Uzwojenia(tcWtorne_->GetValue(), u)) return;
    double p = 0;
    for (auto& x : u) p += x.napiecieV * x.pradA;
    const auto t = TrafoSieciowy::typowe(p);
    tc_[8]->SetValue(L(t.bGs, 0));
    tc_[9]->SetValue(L(t.sprawnosc, 3));
    tc_[10]->SetValue(L(t.gestoscAmm2, 2));
}

void DialogTrafoSieciowy::Przelicz() {
    double v[kPol] = {};
    bool ok = true;
    for (int k = 0; k < kPol; k++)
        if (!tekstNaLiczbe(tc_[k]->GetValue().utf8_string(), v[k])) ok = false;
    rys_->Ustaw(v[0], v[1], v[2], v[4], v[5], v[6]);
    for (wxString& w : wyn_) w = "";
    for (wxStaticText* l : lbWyn_) l->SetLabel("");
    tcTabela_->ChangeValue("");
    if (tcDrut_) tcDrut_->ChangeValue("");
    TrafoSieciowy::Wejscie w;
    w.rdzen = {v[0], v[1], v[2], v[3], v[4], v[5], v[6], 0.5};
    w.napiecieSieciV = v[7]; w.bGs = v[8]; w.sprawnosc = v[9]; w.gestoscAmm2 = v[10]; w.kOkna = v[11]; w.kFe = v[12];
    w.przekladkaWarstwMm = v[13]; w.przekladkaUzwojenMm = v[14]; w.sciankaMm = v[15]; w.marginesWarstwyMm = v[16];
    ok = ok && Uzwojenia(tcWtorne_->GetValue(), w.wtorne);
    auto o = ok ? TrafoSieciowy::oblicz(w) : std::nullopt;
    if (!o) {
        lbUwagi_->SetLabel(TR("Uwagi: uzupełnij pola liczbami większymi od zera; uzwojenia wtórne wpisz jako pary: napięcie i prąd."));
        return;
    }
    const double x[8] = {o->mocWtornaVA, o->mocPierwotnaVA, o->pradPierwotnyA, o->iloczynWymaganyCm4, o->iloczynPosiadanyCm4,
                         static_cast<double>(o->pierwotne.zwoje), o->zapasMm, o->przekrojZelazaCm2};
    const int m[8] = {1, 1, 3, 1, 1, 0, 1, 2};
    for (int k = 0; k < 8; k++) { wyn_[k] = L(x[k], m[k]); lbWyn_[k]->SetLabel(wyn_[k]); }
    wxString t = wxString::Format("%-14s %8s %9s %9s %9s %7s %12s\n", TR("Uzwojenie"), TR("zwoje"), TR("drut[mm]"), TR("emalia"), TR("zw/warstwa"),
                                  TR("warstw"), TR("grubość[mm]"));
    auto wiersz = [&](const wxString& nazwa, const TrafoSieciowy::WynikUzwojenia& u) {
        t += wxString::Format("%-14s %8d %9s %9s %9d %7d %12s\n", nazwa, u.zwoje, L(u.drutMm, 2), L(u.drutEmaliaMm, 2), u.zwojeWWarstwie, u.warstwy,
                              L(u.gruboscMm, 1));
    };
    wiersz(TR("pierwotne"), o->pierwotne);
    for (size_t k = 0; k < o->wtorne.size(); k++) wiersz(wxString::Format(TR("wtórne %d"), static_cast<int>(k + 1)), o->wtorne[k]);
    tcTabela_->ChangeValue(t);
    tcDrut_->ChangeValue(ZestawienieDrutu::Tekst(MasaDrutu::zestawienie(MasaDrutu::pozycje(*o))));
    wxString uw;
    if (o->iloczynPosiadanyCm4 < o->iloczynWymaganyCm4) uw = TR("Uwaga: iloczyn q_o·q_z rdzenia jest mniejszy od wymaganego - wybierz większy rdzeń. ");
    uw += o->miesciSie ? (o->zapasMm > 10 ? TR("Uzwojenia mieszczą się w oknie (zapas duży; optymalnie ok. 5 mm).") : TR("Uzwojenia mieszczą się w oknie."))
                       : TR("Uwaga: uzwojenia nie mieszczą się w oknie rdzenia.");
    lbUwagi_->SetLabel(uw);
    Layout();
}
