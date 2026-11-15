#include "KoloryMotywu.h"
#include "DialogTrafoPP.h"
#include "DialogTrafoPPEtap3.h"
#include "../Logika/Dlawik.h"
#include "DialogDlawika.h"
#include "../Logika/TrafoPP.h"
#include "../Logika/LampyKatalog.h"
#include "../Logika/LogikaKatalogu.h"
#include "../BazaDanych/ManagerBazyDanych.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <cmath>

namespace {
// pola: 0 R0, 1 Raa, 2 P0, 3 Fd, 4 Fg, 5 Ri, 6 I0, 7 B, 8 mi, 9 eta, 10 alfa, 11 beta, 12 sigma, 13 h12(cm; przekładka), 14 S, 15 Fe, 16 Cu, 17 h, 18 a
const char* kNazwy[19] = {"R0", "Raa", "P0", "Fd", "Fg", "Ri", "I0", "B", "mi", "eta", "alfa", "beta", "sigma", "h12", "S", "Fe", "Cu", "h", "a"};
const char* kKlucze[19] = {"Li", "Lr", "n", "iloczyn", "przekroj", "z1", "d1", "z2", "d2", "I1", "I2", "g_L1", "g_V", "g_z1", "g_I1", "g_d1", "g_w4", "g_w8", "g_w16"};
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, m)); }
int Idx(const wxString& n) {
    for (int k = 0; k < 19; k++) if (n == kNazwy[k]) return k;
    return -1;
}
}

DialogTrafoPP::DialogTrafoPP(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Transformator głośnikowy PP"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);

    // Etap 1: wymagania
    wxStaticBoxSizer* wym = new wxStaticBoxSizer(wxVERTICAL, this, TR("Etap 1: wymagania"));
    wxWindow* ww = wym->GetStaticBox();
    const char* ew[13] = {"Rezystancja obciążenia R0 [Ω] =", "Rezystancja Raa anoda-anoda [Ω] =", "Moc wyjściowa P0 [W] =", "Częstotliwość dolna Fd [Hz] =",
                          "Częstotliwość górna Fg [Hz] =", "Rezystancja wewn. lampy Ri [Ω] =", "Prąd spoczynkowy I0 [mA] =", "Indukcja B [Gs] =",
                          "Przenikalność rdzenia μ =", "Sprawność η =", "Wsp. wypełnienia rdzenia Fe α =", "Wsp. wypełnienia okna Cu β =", "Gęstość prądu σ [A/mm²] ="};
    const char* dw[13] = {"8", "3400", "40", "40", "20000", "15000", "30", "1000", "500", "0,9", "0,9", "0,3", "2"};
    wxFlexGridSizer* g = new wxFlexGridSizer(0, 2, 4, 6);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("Przeznaczenie")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chTyp_ = new wxChoice(ww, wxID_ANY);
    chTyp_->Append(TR("Hi-Fi"));
    chTyp_->Append(TR("gitarowy"));
    chTyp_->SetSelection(0);
    PomocGUI::Dodaj(chTyp_, "trafopp.typ");
    g->Add(chTyp_, 0, wxEXPAND);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("Spadek na krańcach pasma")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chSpadek_ = new wxChoice(ww, wxID_ANY);
    for (const char* s : {"1 dB", "2 dB", "3 dB"}) chSpadek_->Append(s);
    chSpadek_->SetSelection(2);
    PomocGUI::Dodaj(chSpadek_, "trafopp.spadek");
    g->Add(chSpadek_, 0, wxEXPAND);
    g->Add(new wxStaticText(ww, wxID_ANY, TR("Lampa z katalogu")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chLampa_ = new wxChoice(ww, wxID_ANY);
    PomocGUI::Dodaj(chLampa_, "trafopp.lampa");
    g->Add(chLampa_, 0, wxEXPAND);
    for (int k = 0; k < 13; k++) {
        g->Add(new wxStaticText(ww, wxID_ANY, TR(ew[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[k] = new wxTextCtrl(ww, wxID_ANY, dw[k], wxDefaultPosition, wxSize(ww->GetTextExtent("00000").x + 22, -1));   // węższe pola Etapu 1, szerokość z czcionki
        PomocGUI::Dodaj(tc_[k], k < 7 ? "trafopp.wymagania" : "trafopp.zaawansowane");
        g->Add(tc_[k]);
    }
    wym->Add(g, 0, wxALL, 4);
    wxBoxSizer* lewa = new wxBoxSizer(wxVERTICAL);
    lewa->Add(wym, 0, wxEXPAND);
    gora->Add(lewa, 0, wxEXPAND);

    wxBoxSizer* prawa = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* poz = new wxStaticBoxSizer(wxVERTICAL, this, TR("Pożądane parametry transformatora"));
    wxWindow* wpz = poz->GetStaticBox();
    wxFlexGridSizer* gp = new wxFlexGridSizer(0, 2, 4, 8);
    const char* ep[5] = {"Ind. pierwotna (H, min) =", "Ind. rozproszenia (H, max) =", "Przekładnia Naa:N0 =", "Okno x rdzeń [cm⁴] =", "Przekrój rdzenia około [cm²] ="};
    for (int k = 0; k < 5; k++) {
        gp->Add(new wxStaticText(wpz, wxID_ANY, TR(ep[k])), 0, wxALIGN_RIGHT);
        lb_[k] = new wxStaticText(wpz, wxID_ANY, "", wxDefaultPosition, wxSize(wpz->GetTextExtent("0000000000").x, -1));
        lb_[k]->SetForegroundColour(Kolory::Wynik());
        PomocGUI::Dodaj(lb_[k], "trafopp.pozadane");
        gp->Add(lb_[k]);
    }
    poz->Add(gp, 0, wxALL, 6);
    prawa->Add(poz, 0, wxEXPAND);   // pożądane parametry nad rysunkiem rdzenia

    // rdzeń zdobytego trafa
    wxStaticBoxSizer* rdz = new wxStaticBoxSizer(wxVERTICAL, this, TR("Wymiary zdobytego transformatora"));
    wxWindow* wr = rdz->GetStaticBox();
    wxFlexGridSizer* gr = new wxFlexGridSizer(0, 2, 4, 8);
    gr->Add(new wxStaticText(wr, wxID_ANY, TR("Kształtka EI")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chKsztaltka_ = new wxChoice(wr, wxID_ANY);
    for (const char* n : {"EI66", "EI84", "EI96", "EI102", "EI105", "EI120", "EI150"}) chKsztaltka_->Append(n);
    chKsztaltka_->Append(TR("własna"));
    chKsztaltka_->SetSelection(3);
    PomocGUI::Dodaj(chKsztaltka_, "trafopp.rdzen");
    gr->Add(chKsztaltka_);
    const char* er[6] = {"Grubość pakietu [mm] =", "Przekrój rdzenia S [cm²] =", "Droga strumienia Fe [cm] =", "Długość zwoju Cu [cm] =",
                         "Wysokość uzwojenia h [cm] =", "Suma grubości uzwojeń a [cm] ="};
    for (int k = 0; k < 6; k++) {
        gr->Add(new wxStaticText(wr, wxID_ANY, TR(er[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[13 + k] = new wxTextCtrl(wr, wxID_ANY, k == 0 ? "51" : "", wxDefaultPosition, wxSize(wr->GetTextExtent("0000000").x + 30, -1));
        PomocGUI::Dodaj(tc_[13 + k], "trafopp.rdzen");
        gr->Add(tc_[13 + k]);
    }
    rdz->Add(gr, 0, wxALL, 4);
    {   // rysunek rdzenia pod pożądanymi parametrami
        wxStaticBoxSizer* rd = new wxStaticBoxSizer(wxVERTICAL, this, TR("Rdzeń EI"));
        rys_ = new PanelRdzenia(rd->GetStaticBox());
        rd->Add(rys_, 1, wxEXPAND | wxALL, 6);   // rysunek wypełnia wolne miejsce w kolumnie
        wym_.Zbuduj(rd->GetStaticBox(), rd, "trafopp.wymiary_blachy");   // wymiary blachy: wypełniane kształtką, edytowalne
        prawa->Add(rd, 1, wxEXPAND | wxTOP, 6);
    }
    gora->Add(prawa, 0, wxEXPAND | wxLEFT, 8);

    // Etap 2: wyniki
    wxStaticBoxSizer* e2 = new wxStaticBoxSizer(wxVERTICAL, this, TR("Etap 2: uzwojenia"));
    wxWindow* w2 = e2->GetStaticBox();
    wxFlexGridSizer* g2 = new wxFlexGridSizer(0, 2, 4, 8);
    const char* e2t[6] = {"Zwoje pierwotne =", "Drut pierwotny [mm] =", "Zwoje wtórne =", "Drut wtórny [mm] =", "Prąd pierwotny [A] =", "Prąd wtórny [A] ="};
    for (int k = 0; k < 6; k++) {
        g2->Add(new wxStaticText(w2, wxID_ANY, TR(e2t[k])), 0, wxALIGN_RIGHT);
        lb_[5 + k] = new wxStaticText(w2, wxID_ANY, "", wxDefaultPosition, wxSize(w2->GetTextExtent("00000000000").x, -1));
        lb_[5 + k]->SetForegroundColour(Kolory::Wynik());
        PomocGUI::Dodaj(lb_[5 + k], "trafopp.uzwojenia");
        g2->Add(lb_[5 + k]);
    }
    e2->Add(g2, 0, wxALL, 6);
    wxBoxSizer* kol3 = new wxBoxSizer(wxVERTICAL);
    kol3->Add(rdz, 0, wxEXPAND);   // wymiary zdobytego rdzenia nad Etapem 2
    kol3->Add(e2, 0, wxEXPAND | wxTOP, 6);
    wxStaticBoxSizer* gt = new wxStaticBoxSizer(wxVERTICAL, this, TR("Porównanie: metoda gitarowa (Mn = 1,25)"));
    wxWindow* wg = gt->GetStaticBox();
    wxFlexGridSizer* gg = new wxFlexGridSizer(0, 2, 4, 8);
    const char* egt[8] = {"Indukcyjność L1 [H] =", "Objętość rdzenia Vż [cm³] =", "Zwoje pierwotne =", "Prąd skuteczny I1 [A] =", "Drut pierwotny [mm] =",
                           "Wtórne 4 Ω: zwoje / drut [mm] =", "Wtórne 8 Ω: zwoje / drut [mm] =", "Wtórne 16 Ω: zwoje / drut [mm] ="};
    const char* kgt[8] = {nullptr, nullptr, nullptr, nullptr, nullptr, "trafopp.gitarowa.wtorne_4_om", "trafopp.gitarowa.wtorne_8_om", "trafopp.gitarowa.wtorne_16_om"};
    for (int k = 0; k < 8; k++) {
        gg->Add(new wxStaticText(wg, wxID_ANY, TR(egt[k])), 0, wxALIGN_RIGHT);
        lb_[11 + k] = new wxStaticText(wg, wxID_ANY, "", wxDefaultPosition, wxSize(wg->GetTextExtent("000000000000").x, -1));
        lb_[11 + k]->SetForegroundColour(Kolory::Zielony());
        if (kgt[k]) PomocGUI::DodajPole(lb_[11 + k], kgt[k]);
        else PomocGUI::Dodaj(lb_[11 + k], "trafopp.gitarowa");
        gg->Add(lb_[11 + k]);
    }
    gt->Add(gg, 0, wxALL, 6);
    kol3->Add(gt, 0, wxEXPAND | wxTOP, 6);
    gora->Add(kol3, 0, wxEXPAND | wxLEFT, 8);
    glowny->Add(gora, 0, wxALL, 8);
    wxBoxSizer* przyc = new wxBoxSizer(wxHORIZONTAL);
    bDalej_ = new wxButton(this, wxID_ANY, TR("Dalej: uzwojenia wykonane (Etap 3)"));
    bDalej_->Disable();
    przyc->Add(bDalej_, 0, wxRIGHT, 8);
    przyc->Add(new wxButton(this, wxID_CLOSE, TR("Zamknij")));
    glowny->Add(przyc, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 8);
    bDalej_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Dalej(); });

    SetSizerAndFit(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    chTyp_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UstawTyp(chTyp_->GetSelection()); });
    chSpadek_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    WczytajLampy();
    chLampa_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { ZLampy(); });
    chKsztaltka_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { ZKsztaltki(); });
    for (int k = 0; k < kPol; k++)
        tc_[k]->Bind(wxEVT_TEXT, [this, k](wxCommandEvent&) {
            if (!wypelniam_ && (k == 5 || k == 6) && chLampa_->GetSelection() > 0) chLampa_->SetSelection(0);   // ręczna zmiana Ri lub I0
            if (!wypelniam_ && k >= 14 && chKsztaltka_->GetSelection() != static_cast<int>(chKsztaltka_->GetCount()) - 1)
                chKsztaltka_->SetSelection(chKsztaltka_->GetCount() - 1);
            if (k == 13 && !wypelniam_) { ZKsztaltki(); return; }
            Przelicz();
        });
    for (int k = 0; k < 6; k++)
        wym_.t[k]->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
            if (wypelniam_) return;
            chKsztaltka_->SetSelection(chKsztaltka_->GetCount() - 1);   // własne wymiary
            ZWymiarow();
        });
    ZKsztaltki();
}

// Lampy mocy z katalogu (rodzina „Lampy elektronowe”, parametr Ia0 PP); brakujące przykładowe EL34, 6P14P, EL84, 6V6, 6L6, ECL86, ECL82, EBL21 są dopisywane
void DialogTrafoPP::WczytajLampy() {
    chLampa_->Append(TR("(wpisz ręcznie)"));
    chLampa_->SetSelection(0);
    ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
    if (!m.czyPolaczony()) return;
    LogikaKatalogu kat(m);
    for (const auto& p : LampyKatalog::przykladoweMocy())
        if (auto ist = kat.znajdz(p.symbol)) {   // lampa dodana wcześniej bez Raa PP: dopisz tylko ten parametr
            const auto di = LampyKatalog::zPozycji(*ist);
            const auto dp = LampyKatalog::zPozycji(p);
            if (di && di->ia0Ma && !di->raaPPOm && dp && dp->raaPPOm) {
                ist->parametry.push_back({LampyKatalog::kParRaaPP, liczbaNaTekstPolski(*dp->raaPPOm, 4, 0), "reczny"});
                kat.zapisz(*ist);
            }
        } else kat.zapisz(p);
    FiltrKatalogu f;
    f.rodzina = LampyKatalog::kRodzina;
    std::vector<PozycjaKatalogu> w;
    if (!kat.pobierz(f, w)) return;
    for (const auto& p : w) {
        const auto pe = kat.pobierzPozycje(p.id);
        const auto d = pe ? LampyKatalog::zPozycji(*pe) : std::nullopt;
        if (!d || !d->ia0Ma) continue;   // tylko lampy z prądem spoczynkowym dla PP
        lampySym_.push_back(p.symbol);
        lampyRiOm_.push_back(d->rwew * 1000.0);
        lampyI0Ma_.push_back(*d->ia0Ma);
        lampyRaaOm_.push_back(d->raaPPOm.value_or(0));
        chLampa_->Append(wxString::FromUTF8(p.symbol));   // tylko typ lampy
    }
}

void DialogTrafoPP::ZLampy() {
    const int i = chLampa_->GetSelection() - 1;
    if (i < 0 || i >= static_cast<int>(lampySym_.size())) return;
    wypelniam_ = true;                       // zmiana Ri i I0 z katalogu nie przestawia wyboru lampy na „ręcznie”
    tc_[5]->SetValue(L(lampyRiOm_[i], 0));   // Ri [Ω]
    tc_[6]->SetValue(L(lampyI0Ma_[i], 0));   // I0 [mA]
    if (lampyRaaOm_[i] > 0) tc_[1]->SetValue(L(lampyRaaOm_[i], 0));   // Raa [Ω], gdy lampa ma ją w katalogu
    wypelniam_ = false;
}

bool DialogTrafoPP::UstawLampe(const wxString& symbol) {
    for (size_t i = 0; i < lampySym_.size(); i++)
        if (wxString::FromUTF8(lampySym_[i]) == symbol) {
            chLampa_->SetSelection(static_cast<int>(i) + 1);
            ZLampy();
            return true;
        }
    return false;
}

void DialogTrafoPP::UstawSpadek(int db) {
    chSpadek_->SetSelection(db - 1);
    Przelicz();
}

void DialogTrafoPP::UstawTyp(int numer) {
    chTyp_->SetSelection(numer);
    tc_[3]->SetValue(numer == 0 ? "40" : "50");        // dolna częstotliwość Fd [Hz]: gitarowy 50
    tc_[4]->SetValue(numer == 0 ? "20000" : "12000");  // górna częstotliwość Fg [Hz]: gitarowy 12000
    tc_[7]->SetValue(numer == 0 ? "1000" : "8000");   // gitarowy: 8000 Gs dla ok. 50 W (guitaramplifiers.prv.pl), do korekty przez użytkownika
}

void DialogTrafoPP::ZKsztaltki() {
    double pakiet = 0;
    tekstNaLiczbe(tc_[13]->GetValue().utf8_string(), pakiet);
    if (auto r = Dlawik::ksztaltkaZNazwy(chKsztaltka_->GetStringSelection().utf8_string(), pakiet > 0 ? pakiet : 51)) {
        wypelniam_ = true;
        wym_.Wpisz(*r);   // wymiary kształtki do pól (można je potem poprawić)
        wypelniam_ = false;
    }
    ZWymiarow();
}

void DialogTrafoPP::ZWymiarow() {
    double pakiet = 0;
    tekstNaLiczbe(tc_[13]->GetValue().utf8_string(), pakiet);
    Dlawik::Rdzen r;
    if (wym_.Odczytaj(pakiet, r)) {
        const auto q = WymiaryRdzenia::Licz(r);
        wypelniam_ = true;
        tc_[14]->ChangeValue(L(q.sCm2, 2));    // S
        tc_[15]->ChangeValue(L(q.feCm, 2));    // droga strumienia
        tc_[16]->ChangeValue(L(q.cuCm, 2));    // długość zwoju
        tc_[17]->ChangeValue(L(q.hCm, 2));
        tc_[18]->ChangeValue(L(q.cCm, 2));
        rys_->Ustaw(r.A, r.B, r.a, r.c, r.d, r.h);
        wypelniam_ = false;
    }
    Przelicz();
}

void DialogTrafoPP::UstawKsztaltke(const wxString& nazwa) {
    const int i = chKsztaltka_->FindString(nazwa);
    if (i != wxNOT_FOUND) chKsztaltka_->SetSelection(i);
    ZKsztaltki();
}
void DialogTrafoPP::UstawPole(const wxString& n, const wxString& v) {
    if (n == "pakiet") { tc_[13]->SetValue(v); return; }
    if (const int w = WymiaryRdzenia::Indeks(n); w >= 0) { wym_.t[w]->SetValue(v); return; }
    if (int k = Idx(n); k >= 0 && k != 13) tc_[k]->SetValue(v);
}
wxString DialogTrafoPP::PoleTekst(const wxString& n) const {
    if (const int w = WymiaryRdzenia::Indeks(n); w >= 0) return wym_.t[w]->GetValue();
    const int k = Idx(n);
    return k >= 0 ? tc_[k]->GetValue() : wxString();
}
wxString DialogTrafoPP::WybranaLampa() const {
    const int i = chLampa_->GetSelection() - 1;
    return i >= 0 && i < static_cast<int>(lampySym_.size()) ? wxString::FromUTF8(lampySym_[i]) : wxString();
}
wxString DialogTrafoPP::Wynik(const wxString& n) const {
    for (int k = 0; k < kWyn; k++) if (n == kKlucze[k]) return wyn_[k];
    return wxString();
}

namespace {
bool WczytajPP(const wxTextCtrl* const* tc, int spadekDb, TrafoPP::Wymagania& w, TrafoPP::Rdzen& r) {
    double v[19] = {};
    for (int k = 0; k < 19; k++)
        if (k != 13 && !tekstNaLiczbe(tc[k]->GetValue().utf8_string(), v[k])) return false;
    w.rObciazeniaOm = v[0]; w.raaOm = v[1]; w.mocW = v[2]; w.fdHz = v[3]; w.fgHz = v[4]; w.riOm = v[5]; w.i0Ma = v[6];
    w.bGs = v[7]; w.mi = v[8]; w.eta = v[9]; w.alfaFe = v[10]; w.betaOkna = v[11]; w.sigma = v[12];
    w.spadekDb = spadekDb;
    r.przekrojCm2 = v[14]; r.drogaStrumieniaCm = v[15]; r.dlugoscZwojuCm = v[16]; r.wysokoscUzwojeniaCm = v[17]; r.szerokoscOknaCm = v[18];
    return true;
}
}

DialogTrafoPPEtap3* DialogTrafoPP::OtworzEtap3() {
    TrafoPP::Wymagania w;
    TrafoPP::Rdzen r;
    if (!WczytajPP(tc_, chSpadek_->GetSelection() + 1, w, r)) return nullptr;
    const auto u = TrafoPP::etap2(w, r);
    if (!u) return nullptr;
    return new DialogTrafoPPEtap3(this, w, r, *u);
}

void DialogTrafoPP::Dalej() {
    DialogTrafoPPEtap3* d = OtworzEtap3();
    if (!d) return;
    d->ShowModal();
    d->Destroy();
}

void DialogTrafoPP::Przelicz() {
    double v[kPol] = {};
    bool ok = true;
    for (int k = 0; k < kPol; k++)
        if (k != 13 && !tekstNaLiczbe(tc_[k]->GetValue().utf8_string(), v[k])) ok = false;
    for (wxString& w : wyn_) w = "";
    for (wxStaticText* l : lb_) l->SetLabel("");
    if (bDalej_) bDalej_->Enable(ok);
    if (!ok) return;
    TrafoPP::Wymagania w;
    w.rObciazeniaOm = v[0]; w.raaOm = v[1]; w.mocW = v[2]; w.fdHz = v[3]; w.fgHz = v[4]; w.riOm = v[5]; w.i0Ma = v[6];
    w.bGs = v[7]; w.mi = v[8]; w.eta = v[9]; w.alfaFe = v[10]; w.betaOkna = v[11]; w.sigma = v[12];
    w.spadekDb = chSpadek_->GetSelection() + 1;
    const auto p = TrafoPP::etap1(w);
    if (!p) return;
    const double x1[5] = {p->liH, p->lrH, p->przekladnia, p->iloczynCm4, p->przekrojCm2};
    const int m1[5] = {4, 4, 3, 2, 3};
    for (int k = 0; k < 5; k++) { wyn_[k] = L(x1[k], m1[k]); lb_[k]->SetLabel(wyn_[k]); }
    TrafoPP::Rdzen r;
    r.przekrojCm2 = v[14]; r.drogaStrumieniaCm = v[15]; r.dlugoscZwojuCm = v[16]; r.wysokoscUzwojeniaCm = v[17]; r.szerokoscOknaCm = v[18];
    const auto u = TrafoPP::etap2(w, r);
    if (bDalej_) bDalej_->Enable(u.has_value());
    if (!u) return;
    const double x2[6] = {u->zwojePierwotne, u->drutPierwotnyMm, u->zwojeWtorne, u->drutWtornyMm, u->pradPierwotnyA, u->pradWtornyA};
    const int m2[6] = {4, 3, 4, 4, 4, 3};
    for (int k = 0; k < 6; k++) { wyn_[5 + k] = L(x2[k], m2[k]); lb_[5 + k]->SetLabel(wyn_[5 + k]); }
    if (const auto g = TrafoPP::gitarowe(w, r)) {
        const double xg[5] = {g->l1H, g->objetoscCm3, g->zwoje1, g->i1A, g->d1Mm};
        const int mg[5] = {3, 1, 4, 4, 3};
        for (int k = 0; k < 5; k++) { wyn_[11 + k] = L(xg[k], mg[k]); lb_[11 + k]->SetLabel(wyn_[11 + k]); }
        for (int k = 0; k < 3; k++) {   // wtórne zawsze dla 4, 8 i 16 Ω
            wyn_[16 + k] = L(std::round(g->zwoje2Zl[k]), 0) + " / " + L(g->d2ZlMm[k], 2);
            lb_[16 + k]->SetLabel(wyn_[16 + k]);
        }
    }
}
