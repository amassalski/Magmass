#include "KoloryMotywu.h"
#include "DialogTrafoSEWykonane.h"
#include "ZestawienieDrutu.h"
#include "WymiaryRdzenia.h"
#include "../Logika/TrafoSieciowy.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <wx/gbsizer.h>
#include <cmath>

namespace {
const char* kNazwy[12] = {"z1", "d1", "z2", "d2", "szczelina", "przekladka", "izolacja", "k", "rho", "fg", "scianka", "margines"};
struct Poz { const char* klucz; const char* etykieta; int miejsca; };
const Poz kPoz[20] = {
    {"li", "Indukcyjność pierwotna wykonana [H] =", 4}, {"fd", "Dolna częstotliwość wykonanego (zadany spadek) [Hz] =", 1},
    {"n", "Przekładnia z1/z2 =", 3}, {"ra", "Ra widziane z lampy [Ω] =", 1},
    {"bdc", "Indukcja od prądu stałego [T] =", 3}, {"bpk", "Indukcja szczytowa [T] =", 3},
    {"rp", "Rezystancja uzwojenia pierwotnego [Ω] =", 3}, {"rw", "Rezystancja uzwojenia wtórnego [Ω] =", 4},
    {"spadek", "Spadek napięcia stałego na pierwotnym [V] =", 2}, {"straty_dc", "Moc tracona od prądu stałego [W] =", 3},
    {"eta", "Sprawność (straty zmienne w miedzi) =", 4},
    {"lr", "Indukcja rozproszenia, wybrany układ [H] =", 5}, {"lr_ukl", "... nieprzeplatane / P/2-S-P/2 / S/2-P-S/2 [H] =", 5},
    {"fg", "Górna częstotliwość wykonanego (zadany spadek) [Hz] =", 0}, {"lr_max", "Dopuszczalna indukcja rozproszenia dla Fg [H] =", 5},
    {"miejsce", "Miejsce na uzwojenia (szerokość x wysokość okna) [cm²] =", 3}, {"zajete", "Uzwojenia zajmą [cm²] =", 3},
    {"lr_ksiazka", "Porównanie: Lr wg książki (Δ = szerokość okna) [H] =", 5}, {"li_zad", "Zadana indukcyjność pierwotna [H] =", 4},
    {"k_uzyte", "Użyta poprawka k (efekty brzegowe) =", 4}};
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, m)); }
}

DialogTrafoSEWykonane::DialogTrafoSEWykonane(wxWindow* parent, const TrafoSE::Wejscie& w, const TrafoSE::Wynik& p)
    : wxDialog(parent, wxID_ANY, TR("Transformator SE - uzwojenia wykonane"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE), w_(w), p_(p) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    // siatka 2x2: uzwojenia wykonane i wyniki z komunikatami (górny rząd, równa wysokość), rozmieszczenie uzwojeń i zestawienie drutu (dolny rząd)
    wxFlexGridSizer* siatka = new wxFlexGridSizer(2, 2, 0, 8);
    siatka->AddGrowableCol(1);
    siatka->AddGrowableRow(0);
    siatka->AddGrowableRow(1);

    wxStaticBoxSizer* wyk = new wxStaticBoxSizer(wxVERTICAL, this, TR("Uzwojenia wykonane"));
    wxWindow* ww = wyk->GetStaticBox();
    // jedna siatka: etykieta | pole | proponowane; listy zaczynają się w kolumnie pól, więc wszystkie pola są równo
    wxGridBagSizer* g = new wxGridBagSizer(4, 8);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("wykonane")), wxGBPosition(0, 1), wxDefaultSpan, wxALIGN_CENTER);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("proponowane")), wxGBPosition(0, 2), wxDefaultSpan, wxALIGN_CENTER);
    const char* et[5] = {"Zwoje pierwotne =", "Drut pierwotny [mm] =", "Zwoje wtórne =", "Drut wtórny [mm] =", "Szczelina całkowita [mm] ="};
    const double prop[5] = {double(p.zwojePierwotne), p.drutPierwotnyMm, double(p.zwojeWtorne), p.drutWtornyMm, p.szczelinaMm};
    const int mp[5] = {0, 3, 0, 3, 3};
    const wxString def[5] = {L(p.zwojePierwotne, 0), L(TrafoSieciowy::drutNormalny(p.drutPierwotnyMm), 2), L(p.zwojeWtorne, 0),
                             L(TrafoSieciowy::drutNormalny(p.drutWtornyMm), 2), L(p.szczelinaMm, 3)};
    const int szPola = ww->GetTextExtent("00000000").x + 24;
    for (int k = 0; k < 5; k++) {
        g->Add(new wxStaticText(ww, wxID_ANY, TR(et[k])), wxGBPosition(k + 1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[k] = new wxTextCtrl(ww, wxID_ANY, def[k], wxDefaultPosition, wxSize(szPola, -1));
        PomocGUI::Dodaj(tc_[k], "trafose.wykonane");
        g->Add(tc_[k], wxGBPosition(k + 1, 1));
        g->Add(new wxStaticText(ww, wxID_ANY, L(prop[k], mp[k])), wxGBPosition(k + 1, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    }
    g->Add(new wxStaticText(ww, wxID_ANY, TR("Układ uzwojeń")), wxGBPosition(6, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chUklad_ = new wxChoice(ww, wxID_ANY);
    chUklad_->Append(TR("P-W: nieprzeplatane"));
    chUklad_->Append(TR("P/2-W-P/2: pierwotne wokół wtórnego"));
    chUklad_->Append(TR("W/2-P-W/2: wtórne wokół pierwotnego"));
    chUklad_->SetSelection(0);
    chUklad_->SetMinSize(wxSize(200, -1));   // szerokość z kolumn siatki, nie z najdłuższej pozycji
    PomocGUI::Dodaj(chUklad_, "trafose.uklad");
    g->Add(chUklad_, wxGBPosition(6, 1), wxGBSpan(1, 2), wxEXPAND);
    const char* e2[7] = {"Przekładki między warstwami [mm] =", "Izolacja między uzwojeniami [mm] =", "Poprawka k (efekty brzegowe) =",
                         "Rezystywność miedzi [Ω·mm²/m] =", "Wymagana górna częstotliwość Fg [Hz] =",
                         "Ścianka karkasu [mm] =", "Margines warstwy [mm] ="};
    const char* d2[7] = {"0,05", "0,5", "auto", "0,0178", "20000", "1,5", "1,5"};
    for (int k = 0; k < 7; k++) {
        g->Add(new wxStaticText(ww, wxID_ANY, TR(e2[k])), wxGBPosition(7 + k, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[5 + k] = new wxTextCtrl(ww, wxID_ANY, d2[k], wxDefaultPosition, wxSize(szPola, -1));
        PomocGUI::Dodaj(tc_[5 + k], k >= 5 ? "trafose.karkas" : k == 2 ? "trafose.poprawka_k" : "trafose.model");
        g->Add(tc_[5 + k], wxGBPosition(7 + k, 1));
    }
    wyk->Add(g, 0, wxALL, 4);
    siatka->Add(wyk, 1, wxEXPAND);

    wxStaticBoxSizer* wyn = new wxStaticBoxSizer(wxVERTICAL, this, TR("Wyniki"));
    wxWindow* wn = wyn->GetStaticBox();
    wxFlexGridSizer* gw = new wxFlexGridSizer(0, 2, 3, 8);
    for (int k = 0; k < kWyn; k++) {
        gw->Add(new wxStaticText(wn, wxID_ANY, TR(kPoz[k].etykieta)), 0, wxALIGN_RIGHT);
        lb_[k] = new wxStaticText(wn, wxID_ANY, "", wxDefaultPosition, wxSize(wn->GetTextExtent("0,00000 / 0,00000 / 0,00000").x, -1));
        lb_[k]->SetForegroundColour(k == 17 ? Kolory::Szary() : Kolory::Wynik());
        PomocGUI::Dodaj(lb_[k], k == 17 ? "trafose.porownanie" : "trafose.wyniki3");
        gw->Add(lb_[k]);
    }
    wyn->Add(gw, 0, wxALL, 6);
    // wyniki, pod nimi komunikaty (wysokość wyników jest mniejsza od ramki uzwojeń) i przycisk Powrót
    wxBoxSizer* wynIUwagi = new wxBoxSizer(wxVERTICAL);
    wynIUwagi->Add(wyn, 0, wxEXPAND);
    wxBoxSizer* uwagiKol = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* uw = new wxStaticBoxSizer(wxVERTICAL, this, TR("Komunikaty"));
    lbUwagi_ = new wxStaticText(uw->GetStaticBox(), wxID_ANY, "");
    lbUwagi_->SetForegroundColour(Kolory::Ostrzezenie());
    uw->Add(lbUwagi_, 1, wxEXPAND | wxALL, 6);
    uwagiKol->Add(uw, 1, wxEXPAND);
    uwagiKol->Add(new wxButton(this, wxID_CLOSE, TR("Powrót")), 0, wxALIGN_RIGHT | wxTOP, 8);
    wynIUwagi->Add(uwagiKol, 1, wxEXPAND | wxTOP, 6);
    siatka->Add(wynIUwagi, 1, wxEXPAND);

    wxStaticBoxSizer* tab = new wxStaticBoxSizer(wxVERTICAL, this, TR("Rozmieszczenie uzwojeń w oknie (od rdzenia)"));
    tcTabela_ = new wxTextCtrl(tab->GetStaticBox(), wxID_ANY, "", wxDefaultPosition, wxSize(560, 90), wxTE_MULTILINE | wxTE_READONLY);
    tcTabela_->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    PomocGUI::Dodaj(tcTabela_, "trafose.sekcje");
    tab->Add(tcTabela_, 1, wxEXPAND | wxALL, 4);
    wxBoxSizer* rz = new wxBoxSizer(wxVERTICAL);
    rz->Add(tab, 1, wxEXPAND | wxTOP, 6);
    siatka->Add(rz, 1, wxEXPAND);   // dolny rząd: rozmieszczenie uzwojeń (lewo), zestawienie drutu (prawo)
    wxBoxSizer* lz = new wxBoxSizer(wxVERTICAL);
    tcDrut_ = ZestawienieDrutu::Dodaj(this, lz, "drut.zestawienie", 0);
    siatka->Add(lz, 1, wxEXPAND);
    glowny->Add(siatka, 0, wxEXPAND | wxALL, 8);

    model_ = new PanelTrafoModel(this, false);
    model_->naZmiane = [this]() { Przelicz(); };
    model_->UwagiDo([this](const wxString& t) { uwagiModelu_ = t; PokazUwagi(); });   // komunikat o rezonansie idzie do ramki Komunikaty
    glowny->Add(model_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    SetSizerAndFit(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    chUklad_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    for (wxTextCtrl* t : tc_) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Przelicz(); });
    Przelicz();
}

void DialogTrafoSEWykonane::UstawPole(const wxString& n, const wxString& v) {
    for (int k = 0; k < kPol; k++) if (n == kNazwy[k]) tc_[k]->SetValue(v);
}
void DialogTrafoSEWykonane::UstawUklad(int numer) {
    chUklad_->SetSelection(numer);
    Przelicz();
}
wxString DialogTrafoSEWykonane::Wynik(const wxString& n) const {
    for (int k = 0; k < kWyn; k++) if (n == kPoz[k].klucz) return wyn_[k];
    return wxString();
}

void DialogTrafoSEWykonane::PokazUwagi() {
    wxString t = uwagiTrafo_;
    if (!uwagiModelu_.empty()) t += (t.empty() ? "" : "\n\n") + uwagiModelu_;
    lbUwagi_->SetLabel(t);
    lbUwagi_->Wrap(540);
    Layout();
}

void DialogTrafoSEWykonane::Przelicz() {
    for (wxString& s : wyn_) s = "";
    for (wxStaticText* l : lb_) l->SetLabel("");
    tcTabela_->ChangeValue("");
    if (tcDrut_) tcDrut_->ChangeValue("");
    if (model_) model_->Pokaz(std::nullopt);
    double v[kPol] = {};
    bool ok = true;
    for (int k = 0; k < kPol; k++)
        if (k == 7 ? !OdczytajK(tc_[7]->GetValue(), v[7]) : !tekstNaLiczbe(tc_[k]->GetValue().utf8_string(), v[k])) ok = false;   // k: „auto” = 0
    TrafoSE::Wykonane x;
    x.zwojePierwotne = static_cast<int>(std::lround(v[0])); x.drutPierwotnyMm = v[1];
    x.zwojeWtorne = static_cast<int>(std::lround(v[2])); x.drutWtornyMm = v[3];
    x.szczelinaMm = v[4]; x.przekladkaWarstwMm = v[5]; x.izolacjaUzwojenMm = v[6]; x.kEfektyBrzegowe = v[7]; x.rezystywnosc = v[8]; x.fgHz = v[9]; x.sciankaKarkasuMm = v[10]; x.marginesWarstwyMm = v[11];
    x.uklad = static_cast<TrafoSE::Wykonane::Uklad>(chUklad_->GetSelection());
    const auto o = ok ? TrafoSE::wykonane(w_, x) : std::nullopt;
    if (!o) {
        uwagiTrafo_ = TR("Uwagi: uzupełnij pola liczbami większymi od zera (zwoje całkowite, szczelina może być 0).");
        PokazUwagi();
        return;
    }
    const double val[20] = {o->liH, o->fdWykonanaHz, o->przekladnia, o->raWidzianeOm, o->indukcjaStalaT, o->indukcjaSzczytowaT, o->rpOm, o->rwOm,
                            o->spadekStalyV, o->stratyStaleW, o->sprawnosc, o->lrH, 0, o->fgWykonanaHz, o->lrMaxH, o->miejsceCm2, o->zajeteCm2,
                            o->lrKsiazkaH, p_.liH, o->kUzyte};
    for (int k = 0; k < kWyn; k++) {
        wyn_[k] = k == 12 ? L(o->lrUkladH[0], 5) + " / " + L(o->lrUkladH[1], 5) + " / " + L(o->lrUkladH[2], 5) : L(val[k], kPoz[k].miejsca);
        lb_[k]->SetLabel(wyn_[k]);
    }
    wxString t = wxString::Format("%-10s %7s %11s %8s %12s %10s\n", TR("Sekcja"), TR("zwoje"), TR("zw/warstwa"), TR("warstw"), TR("grubość[mm]"), TR("zwój[cm]"));
    bool nieparzyste = false;
    int np = 1, nw = 1;
    for (const auto& s : o->sekcje) {
        t += wxString::Format("%-10s %7d %11d %8d %12s %10s%s\n", s.pierwotna ? wxString::Format("P%d", np++) : wxString::Format("W%d", nw++), s.zwoje,
                              s.zwojeWWarstwie, s.warstwy, L(s.gruboscMm, 2), L(s.dlugoscZwojuCm, 2), s.nieparzysteWarstwy ? "  *" : "");
        nieparzyste = nieparzyste || (s.nieparzysteWarstwy && x.uklad != TrafoSE::Wykonane::Uklad::nieprzeplatane);
    }
    tcTabela_->ChangeValue(t);
    tcDrut_->ChangeValue(ZestawienieDrutu::Tekst(MasaDrutu::zestawienie(MasaDrutu::pozycje(*o, x))));
    wxString uw;
    if (o->liH < 0.99 * p_.liH) uw += TR("Indukcyjność pierwotna jest mniejsza od zadanej - dodaj zwoje albo zmniejsz szczelinę. ");
    if (o->indukcjaSzczytowaT > 1.05 * w_.bMaxT) uw += TR("Indukcja szczytowa przekracza zadaną - rdzeń może się nasycać; zwiększ szczelinę lub zwoje. ");
    if (o->lrH > o->lrMaxH) uw += TR("Indukcyjność rozproszenia przekracza dopuszczalną dla Fg - przeplataj uzwojenia lub zmniejsz grubość uzwojeń. ");
    if (!o->miesciSie) uw += TR("Uzwojenia nie mieszczą się w oknie. ");
    if (nieparzyste) uw += TR("Podzielona sekcja ma nieparzystą liczbę warstw (*) - trudniej połączyć sekcje; zmień liczbę zwojów lub drut. ");
    uwagiTrafo_ = uw;
    PokazUwagi();
    TrafoModel::Parametry par;
    if (model_ && model_->Parametry(par)) model_->Pokaz(TrafoSE::analizaDokladna(w_, x, *o, par), w_.spadekDb);
    Layout();
}
