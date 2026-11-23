#include "KoloryMotywu.h"
#include "DialogTrafoPPEtap3.h"
#include <wx/gbsizer.h>
#include "ZestawienieDrutu.h"
#include "WymiaryRdzenia.h"
#include "../Logika/TrafoSieciowy.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

namespace {
constexpr int kUkladWlasny = 7;   // numer pozycji „własny (tabela sekcji)” na liście układów (po szablonach P-S ... Mesa Boogie DR)
const char* kNazwy[7] = {"z1", "d1", "z2", "d2", "beta", "k", "rho"};
struct Poz { const char* klucz; const char* etykieta; int miejsca; };
const Poz kPoz[15] = {
    {"li", "Ind. pierwotna wykonana [H] =", 4}, {"raa", "Rezystancja Raa wykonana [Ω] =", 2},
    {"lr", "Ind. rozproszenia, wybrany układ [H] =", 5}, {"lr_nieprz", "... uzwojenia nieprzeplatane [H] =", 5},
    {"lr_przepl", "... pierwotne w dwóch częściach [H] =", 5}, {"rp", "Rez. uzwojenia pierwotnego [Ω] =", 3},
    {"rw", "Rez. uzwojenia wtórnego [Ω] =", 4}, {"eta", "Sprawność wykonanego trafo =", 4},
    {"miejsce", "Miejsce w oknie (szer. x wys.) [cm²] =", 3}, {"zajete", "Uzwojenia zajmą [cm²] =", 3},
    {"lr_ksiazka", "Porównanie: Lr wg książki/ECCLab [H] =", 5}, {"rw_ecclab", "Porównanie: Rw wg ECCLab [Ω] =", 4},
    {"li_zad", "Pożądana ind. pierwotna min [H] =", 4}, {"lr_zad", "Dopuszczalna ind. rozproszenia max [H] =", 4},
    {"k_uzyte", "Użyta poprawka k (brzegi) =", 4}};
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, m)); }
}

DialogTrafoPPEtap3::DialogTrafoPPEtap3(wxWindow* parent, const TrafoPP::Wymagania& w, const TrafoPP::Rdzen& r, const TrafoPP::Uzwojenia& u)
    : wxDialog(parent, wxID_ANY, TR("Transformator PP - uzwojenia wykonane (Etap 3)"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
      w_(w), r_(r), u_(u) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* wyk = new wxStaticBoxSizer(wxVERTICAL, this, TR("Uzwojenia wykonane"));
    wxWindow* ww = wyk->GetStaticBox();
    // jedna siatka: etykieta | wykonane | proponowane | sekcje od rdzenia (pole na wysokość czterech wierszy); listy wyboru
    // zaczynają się w kolumnie pól, więc są równo z polami zwojów i drutów
    wxGridBagSizer* g = new wxGridBagSizer(4, 8);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("wykonane")), wxGBPosition(0, 1), wxDefaultSpan, wxALIGN_CENTER);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("proponowane")), wxGBPosition(0, 2), wxDefaultSpan, wxALIGN_CENTER);
    const char* et[4] = {"Zwoje pierwotne =", "Drut pierwotny [mm] =", "Zwoje wtórne =", "Drut wtórny [mm] ="};
    const double prop[4] = {u.zwojePierwotne, u.drutPierwotnyMm, u.zwojeWtorne, u.drutWtornyMm};
    const wxString def[4] = {L(std::round(u.zwojePierwotne), 0), L(TrafoSieciowy::drutNormalny(u.drutPierwotnyMm), 2),
                             L(std::round(u.zwojeWtorne), 0), L(TrafoSieciowy::drutNormalny(u.drutWtornyMm), 2)};
    for (int k = 0; k < 4; k++) {
        g->Add(new wxStaticText(ww, wxID_ANY, TR(et[k])), wxGBPosition(k + 1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[k] = new wxTextCtrl(ww, wxID_ANY, def[k], wxDefaultPosition, wxSize(75, -1));
        PomocGUI::Dodaj(tc_[k], "trafopp.wykonane");
        g->Add(tc_[k], wxGBPosition(k + 1, 1));
        g->Add(new wxStaticText(ww, wxID_ANY, L(prop[k], k % 2 == 0 ? 4 : 3)), wxGBPosition(k + 1, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    }
    wxStaticText* etSekcje = new wxStaticText(ww, wxID_ANY, TR("Sekcje od rdzenia: A pierwotne, W wtórne, | przegroda"));
    etSekcje->Wrap(230);
    g->Add(etSekcje, wxGBPosition(0, 3), wxDefaultSpan, wxALIGN_BOTTOM);
    tcSekcje_ = new wxTextCtrl(ww, wxID_ANY, "", wxDefaultPosition, wxSize(230, -1), wxTE_MULTILINE);
    tcSekcje_->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    PomocGUI::Dodaj(tcSekcje_, "trafopp.sekcje_tabela");
    g->Add(tcSekcje_, wxGBPosition(1, 3), wxGBSpan(4, 1), wxEXPAND);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("Układ uzwojeń")), wxGBPosition(5, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chUklad_ = new wxChoice(ww, wxID_ANY);
    chUklad_->Append(TR("P-S: nieprzeplatane"));
    chUklad_->Append(TR("P/2-S-P/2: pierwotne wokół wtórnego"));
    chUklad_->Append(TR("gitarowy: A 40% W 50% A 20% W 50% A 40%"));
    chUklad_->Append(TR("HiFi: karkas dwukomorowy (A W | A W)"));
    chUklad_->Append(TR("HiFi: dwukomorowy przeplatany (A W A | A W A)"));
    chUklad_->Append(TR("Mesa Boogie DR: odczepy 4, 8 i 16 Ω (10 warstw)"));
    chUklad_->Append(TR("Mesa Boogie DR zmodyfikowany: odczepy 4, 8 i 16 Ω (11 warstw)"));
    chUklad_->Append(TR("własny (tabela sekcji)"));
    chUklad_->SetSelection(0);
    PomocGUI::Dodaj(chUklad_, "trafopp.uklad");
    g->Add(chUklad_, wxGBPosition(5, 1), wxGBSpan(1, 3), wxEXPAND);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("Sekcje wtórne połączone")), wxGBPosition(6, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chWtorne_ = new wxChoice(ww, wxID_ANY);
    chWtorne_->Append(TR("szeregowo"));
    chWtorne_->Append(TR("równolegle (równe sekcje)"));
    chWtorne_->SetSelection(0);
    PomocGUI::Dodaj(chWtorne_, "trafopp.wtorne");
    g->Add(chWtorne_, wxGBPosition(6, 1), wxGBSpan(1, 3), wxEXPAND);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("Odczep wtórnego (DR)")), wxGBPosition(7, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chOdczep_ = new wxChoice(ww, wxID_ANY);
    chOdczep_->Append(TR("4 Ω"));
    chOdczep_->Append(TR("8 Ω"));
    chOdczep_->Append(TR("16 Ω"));
    chOdczep_->SetSelection(1);
    chOdczep_->SetToolTip(TR("Transformator z odczepami (Mesa Boogie DR) ma wszystkie trzy obciążenia naraz; tu wybierasz, dla którego liczysz wyniki. Zestawienie wszystkich trzech jest pod tabelą sekcji."));
    g->Add(chOdczep_, wxGBPosition(7, 1), wxGBSpan(1, 3), wxEXPAND);
    wyk->Add(g, 0, wxALL, 4);
    // parametry modelu: dwie pary pole-etykieta w wierszu
    wxFlexGridSizer* g3 = new wxFlexGridSizer(0, 4, 4, 6);
    auto pole = [&](const wxString& etykieta, wxTextCtrl*& tc, const char* dom, const char* klucz) {
        g3->Add(new wxStaticText(ww, wxID_ANY, etykieta), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc = new wxTextCtrl(ww, wxID_ANY, dom, wxDefaultPosition, wxSize(65, -1));
        PomocGUI::Dodaj(tc, klucz);
        g3->Add(tc);
    };
    pole(TR("Przegroda karkasu [mm] ="), tcPrzegroda_, "2", "trafopp.przegroda");
    pole(TR("Ścianka karkasu [mm] ="), tcScianka_, "1,5", "trafopp.scianka_karkasu");
    pole(TR("Margines warstwy [mm] ="), tcMargines_, "1,5", "trafopp.margines_warstwy");
    pole(TR("Przekładki warstw [mm] ="), tc_[4], "0,05", "trafopp.model");
    pole(TR("Poprawka k (brzegi) ="), tc_[5], "auto", "trafopp.poprawka_k");
    pole(TR("Rezystywność Cu [Ω·mm²/m] ="), tc_[6], "0,0178", "trafopp.model");
    wyk->Add(g3, 0, wxALL, 4);
    // siatka 2x2: w górnym rzędzie uzwojenia wykonane i wyniki (z komunikatami) mają tę samą wysokość,
    // w dolnym zestawienie drutu i rozmieszczenie uzwojeń też
    wxFlexGridSizer* siatka = new wxFlexGridSizer(2, 2, 0, 8);
    siatka->AddGrowableCol(1);
    siatka->AddGrowableRow(0);
    siatka->AddGrowableRow(1);
    siatka->Add(wyk, 1, wxEXPAND);
    wxStaticBoxSizer* wyn = new wxStaticBoxSizer(wxVERTICAL, this, TR("Wyniki"));
    wxWindow* wn = wyn->GetStaticBox();
    wxFlexGridSizer* gw = new wxFlexGridSizer(0, 2, 3, 8);
    for (int k = 0; k < kWyn; k++) {
        gw->Add(new wxStaticText(wn, wxID_ANY, TR(kPoz[k].etykieta)), 0, wxALIGN_RIGHT);
        lb_[k] = new wxStaticText(wn, wxID_ANY, "", wxDefaultPosition, wxSize(64, -1));
        lb_[k]->SetForegroundColour(k >= 10 && k < 12 ? Kolory::Szary() : Kolory::Wynik());
        PomocGUI::Dodaj(lb_[k], k >= 10 && k < 12 ? "trafopp.porownanie" : "trafopp.wyniki3");
        gw->Add(lb_[k]);
    }
    wyn->Add(gw, 0, wxALL, 6);
    // obok wyników: komunikaty (pomarańczowe) i przycisk Powrót
    wxBoxSizer* wynIUwagi = new wxBoxSizer(wxHORIZONTAL);
    wynIUwagi->Add(wyn, 0, wxEXPAND);
    wxBoxSizer* uwagiKol = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* uw = new wxStaticBoxSizer(wxVERTICAL, this, TR("Komunikaty"));
    lbUwagi_ = new wxStaticText(uw->GetStaticBox(), wxID_ANY, "");
    lbUwagi_->SetForegroundColour(Kolory::Ostrzezenie());
    uw->Add(lbUwagi_, 1, wxEXPAND | wxALL, 6);
    uwagiKol->Add(uw, 1, wxEXPAND);
    uwagiKol->Add(new wxButton(this, wxID_CLOSE, TR("Powrót")), 0, wxALIGN_RIGHT | wxTOP, 8);
    wynIUwagi->Add(uwagiKol, 1, wxEXPAND | wxLEFT, 8);
    siatka->Add(wynIUwagi, 1, wxEXPAND);

    wxStaticBoxSizer* tab = new wxStaticBoxSizer(wxVERTICAL, this, TR("Rozmieszczenie uzwojeń w oknie (od rdzenia)"));
    tcTabela_ = new wxTextCtrl(tab->GetStaticBox(), wxID_ANY, "", wxDefaultPosition, wxSize(560, 90), wxTE_MULTILINE | wxTE_READONLY);
    tcTabela_->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    PomocGUI::Dodaj(tcTabela_, "trafopp.sekcje");
    tab->Add(tcTabela_, 1, wxEXPAND | wxALL, 4);
    wxBoxSizer* rz = new wxBoxSizer(wxVERTICAL);
    rz->Add(tab, 1, wxEXPAND | wxTOP, 6);
    siatka->Add(rz, 1, wxEXPAND);   // dolny rząd: rozmieszczenie uzwojeń (lewo), zestawienie drutu (prawo)
    wxBoxSizer* lz = new wxBoxSizer(wxVERTICAL);
    tcDrut_ = ZestawienieDrutu::Dodaj(this, lz, "drut.zestawienie", 0);
    siatka->Add(lz, 1, wxEXPAND);
    gora->Add(siatka, 1, wxEXPAND);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 8);

    model_ = new PanelTrafoModel(this, true);
    model_->naZmiane = [this]() { Przelicz(); };
    model_->UwagiDo([this](const wxString& t) { uwagiModelu_ = t; PokazUwagi(); });   // komunikat o rezonansie idzie do ramki Komunikaty
    glowny->Add(model_, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

    SetSizerAndFit(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    chUklad_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { ZSzablonu(); });
    chWtorne_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    chOdczep_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    tcPrzegroda_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Przelicz(); });
    tcScianka_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Przelicz(); });
    tcMargines_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Przelicz(); });
    for (int k = 0; k < kPol; k++)
        tc_[k]->Bind(wxEVT_TEXT, [this, k](wxCommandEvent&) {
            if (!wypelniam_ && (k == 0 || k == 2) && chUklad_->GetSelection() != kUkladWlasny) ZSzablonu();   // zmiana zwojów: odtwórz tabelę z szablonu
            else Przelicz();
        });
    tcSekcje_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        if (wypelniam_) return;
        std::vector<TrafoPP::SekcjaWej> s;
        std::string blad;
        if (TrafoPP::parsujSekcje(tcSekcje_->GetValue().utf8_string(), s, blad)) {      // suma A i W wyznacza z1, z2
            int a = 0;
            for (const auto& x : s) if (x.pierwotna) a += x.zwoje;
            wypelniam_ = true;
            tc_[0]->ChangeValue(L(a, 0));
            tc_[2]->ChangeValue(L(TrafoPP::zwojeWtorneZSekcji(s, chWtorne_->GetSelection() == 1, 1), 0));
            if (chUklad_->GetSelection() != kUkladWlasny) chUklad_->SetSelection(kUkladWlasny);
            wypelniam_ = false;
        }
        Przelicz();
    });
    ZSzablonu();
}

void DialogTrafoPPEtap3::UstawPole(const wxString& n, const wxString& v) {
    for (int k = 0; k < kPol; k++) if (n == kNazwy[k]) tc_[k]->SetValue(v);
}
void DialogTrafoPPEtap3::ZSzablonu() {
    const int numer = chUklad_->GetSelection();
    if (numer != kUkladWlasny) {
        double z1 = 0, z2 = 0;
        tekstNaLiczbe(tc_[0]->GetValue().utf8_string(), z1);
        tekstNaLiczbe(tc_[2]->GetValue().utf8_string(), z2);
        wypelniam_ = true;
        tcSekcje_->ChangeValue(wxString::FromUTF8(TrafoPP::formatujSekcje(TrafoPP::presetSekcji(numer, static_cast<int>(std::lround(z1)), static_cast<int>(std::lround(z2))))));
        wypelniam_ = false;
        if (numer == 5 || numer == 6) {   // DR: odczep najbliższy obciążeniu z Etapu 1 (z2 w tabeli = zwoje odczepu 4 Ω)
            int najblizszy = 0;
            for (int k = 1; k < 3; k++)
                if (std::fabs(TrafoPP::kImpedancjeOdczepow[k] - w_.rObciazeniaOm) < std::fabs(TrafoPP::kImpedancjeOdczepow[najblizszy] - w_.rObciazeniaOm)) najblizszy = k;
            chOdczep_->SetSelection(najblizszy);
        }
    }
    Przelicz();
}
void DialogTrafoPPEtap3::UstawOdczep(int numer) {
    chOdczep_->SetSelection(numer);
    Przelicz();
}
void DialogTrafoPPEtap3::UstawUklad(int numer) {
    chUklad_->SetSelection(numer);
    ZSzablonu();
}
void DialogTrafoPPEtap3::UstawSekcje(const wxString& tekst) { tcSekcje_->SetValue(tekst); }
void DialogTrafoPPEtap3::UstawWtorne(bool rownolegle) {
    chWtorne_->SetSelection(rownolegle ? 1 : 0);
    Przelicz();
}
void DialogTrafoPPEtap3::UstawKarkas(const wxString& s, const wxString& m) { tcScianka_->SetValue(s); tcMargines_->SetValue(m); }
void DialogTrafoPPEtap3::UstawPrzegrode(const wxString& mm) { tcPrzegroda_->SetValue(mm); }
wxString DialogTrafoPPEtap3::Wynik(const wxString& n) const {
    for (int k = 0; k < kWyn; k++) if (n == kPoz[k].klucz) return wyn_[k];
    return wxString();
}

void DialogTrafoPPEtap3::PokazUwagi() {
    wxString t = uwagiTrafo_;
    if (!uwagiModelu_.empty()) t += (t.empty() ? "" : "\n\n") + uwagiModelu_;
    lbUwagi_->SetLabel(t);
    lbUwagi_->Wrap(300);
    Layout();
}

void DialogTrafoPPEtap3::Przelicz() {
    for (wxString& s : wyn_) s = "";
    uwagiTrafo_.clear();
    for (wxStaticText* l : lb_) l->SetLabel("");
    tcTabela_->ChangeValue("");
    if (tcDrut_) tcDrut_->ChangeValue("");
    if (model_) model_->Pokaz(std::nullopt);
    double v[kPol] = {};
    bool ok = true;
    for (int k = 0; k < kPol; k++)
        if (k == 5 ? !OdczytajK(tc_[5]->GetValue(), v[5]) : !tekstNaLiczbe(tc_[k]->GetValue().utf8_string(), v[k])) ok = false;   // k: „auto” = 0
    TrafoPP::Wykonane x;
    x.zwojePierwotne = static_cast<int>(std::lround(v[0])); x.drutPierwotnyMm = v[1];
    x.zwojeWtorne = static_cast<int>(std::lround(v[2])); x.drutWtornyMm = v[3];
    x.przekladkaWarstwMm = v[4]; x.kEfektyBrzegowe = v[5]; x.rezystywnosc = v[6];
    std::string bladSekcji;
    ok = ok && TrafoPP::parsujSekcje(tcSekcje_->GetValue().utf8_string(), x.sekcjeWej, bladSekcji);
    double przegroda = 0;
    ok = ok && tekstNaLiczbe(tcPrzegroda_->GetValue().utf8_string(), przegroda);
    x.przegrodaMm = przegroda;
    ok = ok && tekstNaLiczbe(tcScianka_->GetValue().utf8_string(), x.sciankaKarkasuMm) && tekstNaLiczbe(tcMargines_->GetValue().utf8_string(), x.marginesWarstwyMm);
    x.wtorneRownolegle = chWtorne_->GetSelection() == 1;
    x.odczep = chOdczep_->GetSelection() + 1;
    const bool odczepy = ok && TrafoPP::maOdczepy(x.sekcjeWej);
    chOdczep_->Enable(odczepy);
    chWtorne_->Enable(!odczepy);
    TrafoPP::Wymagania wUz = w_;                                     // przy odczepach: obciążenie wybranego odczepu
    if (odczepy) { wUz.rObciazeniaOm = TrafoPP::kImpedancjeOdczepow[x.odczep - 1]; x = TrafoPP::rozwiazOdczep(x); }
    const auto o = ok ? TrafoPP::etap3(wUz, r_, x) : std::nullopt;
    if (!o) {
        uwagiTrafo_ = (!bladSekcji.empty() ? wxString::FromUTF8(bladSekcji) :
            TR("Uwagi: uzupełnij pola liczbami większymi od zera. Suma zwojów A w tabeli sekcji musi być równa zwojom pierwotnym, suma W wtórnym (przy połączeniu równoległym każda sekcja W ma tyle zwojów, ile wtórne); komory muszą się zmieścić w oknie."));
        PokazUwagi();
        return;
    }
    const auto p = TrafoPP::etap1(w_);
    const double val[15] = {o->liH, o->raaOm, o->lrH, o->lrNieprzeplatanyH, o->lrPrzeplatanyH, o->rpOm, o->rwOm, o->sprawnosc, o->miejsceCm2,
                            o->zajeteCm2, o->lrKsiazkaH, o->rwEcclabOm, p->liH, p->lrH, o->kUzyte};
    for (int k = 0; k < kWyn; k++) { wyn_[k] = L(val[k], kPoz[k].miejsca); lb_[k]->SetLabel(wyn_[k]); }
    wxString t = wxString::Format("%-8s %6s %7s %11s %8s %12s %10s\n", TR("Sekcja"), TR("komora"), TR("zwoje"), TR("zw/warstwa"), TR("warstw"), TR("grubość[mm]"), TR("zwój[cm]"));
    bool nieparzyste = false;
    int na = 0, nw = 0, sekcjiA = 0;
    for (const auto& s : o->sekcje) if (s.pierwotna) sekcjiA++;
    for (const auto& s : o->sekcje) {
        t += wxString::Format("%-8s %6d %7d %11d %8d %12s %10s%s\n", s.pierwotna ? wxString::Format("A%d", ++na) : wxString::Format("W%d", ++nw), s.komora + 1, s.zwoje,
                              s.zwojeWWarstwie, s.warstwy, L(s.gruboscMm, 2), L(s.dlugoscZwojuCm, 2), s.nieparzysteWarstwy ? "  *" : "");
        nieparzyste = nieparzyste || (s.nieparzysteWarstwy && s.pierwotna && sekcjiA > 1);
    }
    if (odczepy) {   // zestawienie wszystkich odczepów, każdy dla własnego obciążenia
        t += wxString("\n") + TR("Odczepy (każdy dla własnego obciążenia):") + "\n";
        t += wxString::Format("%-6s %7s %10s %11s %10s %7s\n", TR("odczep"), TR("zwoje"), TR("Raa[Ω]"), TR("Lr[H]"), TR("Rw[Ω]"), TR("η"));
        for (const auto& od : TrafoPP::odczepy(w_, r_, x))
            t += wxString::Format("%-6s %7d %10s %11s %10s %7s\n", wxString::Format(wxString::FromUTF8("%g \xCE\xA9"), od.omy), od.zwojeWtorne, L(od.raaOm, 1), L(od.lrH, 5), L(od.rwOm, 4), L(od.sprawnosc, 3));
    }
    tcTabela_->ChangeValue(t);
    tcDrut_->ChangeValue(ZestawienieDrutu::Tekst(MasaDrutu::zestawienie(MasaDrutu::pozycje(*o, x))));
    wxString uw;
    if (o->liH < 0.99 * p->liH) uw += TR("Indukcyjność pierwotna jest mniejsza od pożądanej - dodaj zwoje. ");
    if (o->lrH > p->lrH) uw += TR("Indukcyjność rozproszenia przekracza dopuszczalną - przeplataj uzwojenia lub zmniejsz grubość uzwojeń. ");
    if (!o->miesciSie) uw += TR("Uzwojenia nie mieszczą się w oknie. ");
    if (nieparzyste) uw += TR("Podzielona sekcja pierwotna ma nieparzystą liczbę warstw (*) - trudniej połączyć sekcje; zmień liczbę zwojów lub drut. ");
    if (o->niezrownowazone) uw += TR("Którakolwiek komora nie ma równych amperozwojów A i W - strumień rozproszenia nie zamyka się w komorze, wynik rozproszenia jest orientacyjny. ");
    uwagiTrafo_ = uw;
    PokazUwagi();
    TrafoModel::Parametry par;
    if (model_ && model_->Parametry(par)) model_->Pokaz(TrafoPP::analizaDokladna(w_, r_, x, *o, par), w_.spadekDb);
    Layout();
}
