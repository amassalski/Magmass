#include "KoloryMotywu.h"
#include "DialogTrafoSE.h"
#include "DialogTrafoSEWykonane.h"
#include "../Logika/tube_magnetics.hpp"
#include <algorithm>
#include "../Logika/Dlawik.h"
#include "DialogDlawika.h"
#include "../Logika/TrafoSE.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

namespace {
// pola: 0 P, 1 Ra, 2 Ri, 3 Ia, 4 Fd, 5 Rg, 6 eta, 7 pakiet, 8 S, 9 Fe, 10 B, 11 mi, 12 sigma, 13 Cu, 14 h, 15 a
const char* kNazwy[16] = {"P", "Ra", "Ri", "Ia", "Fd", "Rg", "eta", "pakiet", "S", "Fe", "B", "mi", "sigma", "Cu", "h", "a"};
const char* kKlucze[31] = {"Smin", "Li", "Li_porownanie", "n", "n_porownanie", "Ipk", "z1", "d1", "z2", "d2", "szczelina", "Bdc", "Bpk",
                           "cy_z1", "cy_szczelina", "cy_L", "cy_Bpk", "cy_eta", "cy_az",
                           "kr_z23", "kr_z24", "kr_z2", "kr_szczelina", "kr_przekladka", "kr_d1", "kr_d1min", "kr_d2", "kr_iloczyn",
                           "wi_q", "wi_z1", "wi_L1"};
const double kAKrize[4] = {20, 10, 10, 5};
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, m)); }
int Idx(const wxString& n) {
    for (int k = 0; k < 16; k++) if (n == kNazwy[k]) return k;
    return -1;
}
}

DialogTrafoSE::DialogTrafoSE(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Transformator głośnikowy SE"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer* lewa = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* par = new wxStaticBoxSizer(wxVERTICAL, this, TR("Parametry"));
    wxWindow* wp = par->GetStaticBox();
    const char* ep[6] = {"Przewidywana moc wyjściowa [VA] =", "Opór anodowy lampy Ra [Ω] =", "Opór wewnętrzny lampy Ri [Ω] =",
                         "Prąd anodowy Ia [mA] =", "Najniższe pasmo przenoszenia Fd [Hz] =", "Opór głośnika Rg [Ω] ="};
    const char* dp[6] = {"10", "2000", "15000", "85", "50", "4"};
    wxFlexGridSizer* g = new wxFlexGridSizer(0, 2, 4, 6);
    for (int k = 0; k < 6; k++) {
        g->Add(new wxStaticText(wp, wxID_ANY, TR(ep[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[k] = new wxTextCtrl(wp, wxID_ANY, dp[k], wxDefaultPosition, wxSize(75, -1));
        PomocGUI::Dodaj(tc_[k], "trafose.parametry");
        g->Add(tc_[k]);
    }
    g->Add(new wxStaticText(wp, wxID_ANY, TR("Spadek na krańcach pasma")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chSpadek_ = new wxChoice(wp, wxID_ANY);
    for (const char* s : {"1 dB", "2 dB", "3 dB"}) chSpadek_->Append(s);
    chSpadek_->SetSelection(2);
    PomocGUI::Dodaj(chSpadek_, "trafose.spadek");
    g->Add(chSpadek_);
    par->Add(g, 0, wxALL, 4);
    lewa->Add(par, 0, wxEXPAND);

    wxStaticBoxSizer* rdz = new wxStaticBoxSizer(wxVERTICAL, this, TR("Rdzeń i sprawność"));
    wxWindow* wr = rdz->GetStaticBox();
    wxFlexGridSizer* gr = new wxFlexGridSizer(0, 2, 4, 6);
    gr->Add(new wxStaticText(wr, wxID_ANY, TR("Sprawność transformatora η =")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    tc_[6] = new wxTextCtrl(wr, wxID_ANY, "0,85", wxDefaultPosition, wxSize(75, -1));
    PomocGUI::Dodaj(tc_[6], "trafose.rdzen");
    gr->Add(tc_[6]);
    gr->Add(new wxStaticText(wr, wxID_ANY, TR("Kształtka EI")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chKsztaltka_ = new wxChoice(wr, wxID_ANY);
    for (const char* n : {"EI66", "EI84", "EI96", "EI102", "EI105", "EI120", "EI150"}) chKsztaltka_->Append(n);
    chKsztaltka_->Append(TR("własna"));
    chKsztaltka_->SetSelection(3);   // EI102
    PomocGUI::Dodaj(chKsztaltka_, "trafose.rdzen");
    gr->Add(chKsztaltka_);
    const char* er[6] = {"Grubość pakietu [mm] =", "Przekrój rdzenia [cm²] =", "Średnia droga magnetyczna [cm] =", "Długość zwoju przy rdzeniu [cm] =",
                         "Wysokość uzwojenia (okna) [cm] =", "Szerokość okna [cm] ="};
    const char* dr[6] = {"51", "10,5", "17", "", "", ""};
    const int ind[6] = {7, 8, 9, 13, 14, 15};
    for (int k = 0; k < 6; k++) {
        gr->Add(new wxStaticText(wr, wxID_ANY, TR(er[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[ind[k]] = new wxTextCtrl(wr, wxID_ANY, dr[k], wxDefaultPosition, wxSize(75, -1));
        PomocGUI::Dodaj(tc_[ind[k]], "trafose.rdzen");
        gr->Add(tc_[ind[k]]);
    }
    gr->Add(new wxStaticText(wr, wxID_ANY, TR("Lampa i układ (Krize, współczynnik A)")), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    chA_ = new wxChoice(wr, wxID_ANY);
    chA_->Append(TR("pentoda bez NFB (A=20)"));
    chA_->Append(TR("pentoda z NFB (A=10)"));
    chA_->Append(TR("trioda bez NFB (A=10)"));
    chA_->Append(TR("trioda z NFB (A=5)"));
    chA_->SetSelection(0);
    PomocGUI::Dodaj(chA_, "trafose.krize");
    gr->Add(chA_);
    rdz->Add(gr, 0, wxALL, 4);
    lewa->Add(rdz, 0, wxEXPAND | wxTOP, 6);

    wxStaticBoxSizer* zaaw = new wxStaticBoxSizer(wxVERTICAL, this, TR("Model fizyczny"));
    wxWindow* wz = zaaw->GetStaticBox();
    wxFlexGridSizer* gz = new wxFlexGridSizer(0, 2, 4, 6);
    const char* ez[3] = {"Indukcja szczytowa B [T] =", "Przenikalność żelaza μ =", "Gęstość prądu σ [A/mm²] ="};
    const char* dz[3] = {"1", "1000", "2,5"};
    for (int k = 0; k < 3; k++) {
        gz->Add(new wxStaticText(wz, wxID_ANY, TR(ez[k])), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        tc_[10 + k] = new wxTextCtrl(wz, wxID_ANY, dz[k], wxDefaultPosition, wxSize(75, -1));
        PomocGUI::Dodaj(tc_[10 + k], "trafose.model");
        gz->Add(tc_[10 + k]);
    }
    zaaw->Add(gz, 0, wxALL, 4);
    lewa->Add(zaaw, 0, wxEXPAND | wxTOP, 6);
    gora->Add(lewa, 0, wxEXPAND);

    wxStaticBoxSizer* wyn = new wxStaticBoxSizer(wxVERTICAL, this, TR("Wyniki"));
    wxWindow* ww = wyn->GetStaticBox();
    wxFlexGridSizer* gw = new wxFlexGridSizer(0, 2, 4, 8);
    const char* ew[31] = {"Przekrój rdzenia orientacyjnie [cm²] =", "Indukcyjność pierwotna Li [H] =", "Li wg programu porównawczego [H] =",
                          "Przekładnia n = √(η·Ra/Rg) =", "n wg programu porównawczego √(Ra/(η·Rg)) =", "Prąd szczytowy [mA] =", "Zwoje pierwotne =",
                          "Drut pierwotny [mm] =", "Zwoje wtórne =", "Drut wtórny [mm] =", "Szczelina całkowita [mm] =", "Indukcja od prądu stałego [T] =",
                          "Indukcja szczytowa [T] =", "Zwoje pierwotne =", "Szczelina [mm] =", "Indukcyjność pierwotna [H] =", "Indukcja szczytowa Bdc+Bac [T] =",
                          "Sprawność =", "Podmagnesowanie Ia·z1/l [A·zw./cm] =",
                          "Zwoje pierwotne, wzór (23) =", "Zwoje pierwotne, wzór (24) =", "Zwoje wtórne w2 = n·w1 (n = √(Rg/Ra)) =", "Szczelina l3 = w1·I0/8·10⁻⁵ [mm] =",
                          "Przekładka (rdzeń płaszczowy) [mm] =", "Drut pierwotny (26) [mm] =", "Drut pierwotny min (27) [mm] =", "Drut wtórny d1/√n [mm] =",
                          "Q0·Qs: rdzeń / wymagane A·P [cm⁴] =", "Przekrój Q = (3÷5)·√P [cm²] =", "Zwoje n1 = 9000·√(L1·lr/(500·Q)) =", "L1 ≥ Ra/(6,28·fd) [H] ="};
    for (int k = 0; k < 13; k++) {
        gw->Add(new wxStaticText(ww, wxID_ANY, TR(ew[k])), 0, wxALIGN_RIGHT);
        lb_[k] = new wxStaticText(ww, wxID_ANY, "", wxDefaultPosition, wxSize(80, -1));
        lb_[k]->SetForegroundColour(k == 2 || k == 4 ? Kolory::Szary() : Kolory::Wynik());
        PomocGUI::Dodaj(lb_[k], k == 2 || k == 4 ? "trafose.porownanie" : "trafose.wyniki");
        gw->Add(lb_[k]);
    }
    wyn->Add(gw, 0, wxALL, 6);
    wxBoxSizer* por = new wxBoxSizer(wxVERTICAL);
    struct Ramka { const char* tytul; int od, do_; const char* pomoc; };
    const Ramka ramki[3] = {{"Porównanie: moduł Cykina (tube_magnetics)", 13, 19, "trafose.cykin"},
                            {"Porównanie: Krize (transformator z podmagnesowaniem)", 19, 28, "trafose.krize"},
                            {"Porównanie: Witort (1972)", 28, 31, "trafose.witort"}};
    for (const Ramka& r : ramki) {
        wxStaticBoxSizer* ra = new wxStaticBoxSizer(wxVERTICAL, this, TR(r.tytul));
        wxFlexGridSizer* gc = new wxFlexGridSizer(0, 2, 4, 8);
        for (int k = r.od; k < r.do_; k++) {
            gc->Add(new wxStaticText(ra->GetStaticBox(), wxID_ANY, TR(ew[k])), 0, wxALIGN_RIGHT);
            lb_[k] = new wxStaticText(ra->GetStaticBox(), wxID_ANY, "", wxDefaultPosition, wxSize(110, -1));
            lb_[k]->SetForegroundColour(Kolory::Szary());
            PomocGUI::Dodaj(lb_[k], r.pomoc);
            gc->Add(lb_[k]);
        }
        ra->Add(gc, 0, wxALL, 4);
        por->Add(ra, 0, wxEXPAND | wxBOTTOM, 6);
    }
    wxBoxSizer* srodek = new wxBoxSizer(wxVERTICAL);
    srodek->Add(wyn, 0, wxEXPAND);
    {   // rysunek rdzenia pod ramką Wyniki
        wxStaticBoxSizer* rd = new wxStaticBoxSizer(wxVERTICAL, this, TR("Rdzeń EI"));
        rys_ = new PanelRdzenia(rd->GetStaticBox());
        rd->Add(rys_, 0, wxALL, 6);
        wym_.Zbuduj(rd->GetStaticBox(), rd, "trafose.wymiary_blachy");   // wymiary blachy: wypełniane kształtką, edytowalne
        srodek->Add(rd, 0, wxEXPAND | wxTOP, 6);
    }
    gora->Add(srodek, 0, wxEXPAND | wxLEFT, 8);
    ostrz_ = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxSize(390, 150));   // komunikat pod ramką Porównanie: Witort
    ostrz_->SetForegroundColour(Kolory::Ostrzezenie());
    por->Add(ostrz_, 0, wxTOP, 6);
    gora->Add(por, 0, wxEXPAND | wxLEFT, 8);
    glowny->Add(gora, 0, wxALL, 8);
    wxBoxSizer* przyc = new wxBoxSizer(wxHORIZONTAL);
    bDalej_ = new wxButton(this, wxID_ANY, TR("Dalej: uzwojenia wykonane"));
    bDalej_->Disable();
    PomocGUI::Dodaj(bDalej_, "trafose.wykonane");
    przyc->Add(bDalej_, 0, wxRIGHT, 8);
    przyc->Add(new wxButton(this, wxID_CLOSE, TR("Zamknij")));
    glowny->Add(przyc, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 8);
    bDalej_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Dalej(); });

    SetSizerAndFit(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    chKsztaltka_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { ZKsztaltki(); });
    chA_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    chSpadek_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    for (int k = 0; k < kPol; k++)
        tc_[k]->Bind(wxEVT_TEXT, [this, k](wxCommandEvent&) {
            if (!wypelniam_ && (k == 8 || k == 9 || k >= 13) && chKsztaltka_->GetSelection() != static_cast<int>(chKsztaltka_->GetCount()) - 1)
                chKsztaltka_->SetSelection(chKsztaltka_->GetCount() - 1);
            if (k == 7 && !wypelniam_) { ZKsztaltki(); return; }
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

void DialogTrafoSE::ZKsztaltki() {
    double pakiet = 0;
    tekstNaLiczbe(tc_[7]->GetValue().utf8_string(), pakiet);
    if (auto r = Dlawik::ksztaltkaZNazwy(chKsztaltka_->GetStringSelection().utf8_string(), pakiet > 0 ? pakiet : 51)) {
        wypelniam_ = true;
        wym_.Wpisz(*r);   // wymiary kształtki do pól (można je potem poprawić)
        wypelniam_ = false;
    }
    ZWymiarow();
}

void DialogTrafoSE::ZWymiarow() {
    double pakiet = 0;
    tekstNaLiczbe(tc_[7]->GetValue().utf8_string(), pakiet);
    Dlawik::Rdzen r;
    if (wym_.Odczytaj(pakiet, r)) {
        const auto q = WymiaryRdzenia::Licz(r);
        wypelniam_ = true;
        tc_[8]->ChangeValue(L(q.sCm2, 2));    // przekrój
        tc_[9]->ChangeValue(L(q.feCm, 2));    // droga strumienia
        tc_[13]->ChangeValue(L(q.cuCm, 2));   // długość zwoju
        tc_[14]->ChangeValue(L(q.hCm, 2));
        tc_[15]->ChangeValue(L(q.cCm, 2));
        rys_->Ustaw(r.A, r.B, r.a, r.c, r.d, r.h);
        wypelniam_ = false;
    }
    Przelicz();
}

void DialogTrafoSE::UstawSpadek(int db) {
    chSpadek_->SetSelection(db - 1);
    Przelicz();
}

void DialogTrafoSE::UstawA(int numer) {
    chA_->SetSelection(numer);
    Przelicz();
}

void DialogTrafoSE::UstawKsztaltke(const wxString& nazwa) {
    const int i = chKsztaltka_->FindString(nazwa);
    if (i != wxNOT_FOUND) chKsztaltka_->SetSelection(i);
    ZKsztaltki();
}
wxString DialogTrafoSE::PoleTekst(const wxString& n) const {
    if (const int w = WymiaryRdzenia::Indeks(n); w >= 0) return wym_.t[w]->GetValue();
    const int k = Idx(n);
    return k >= 0 ? tc_[k]->GetValue() : wxString();
}
void DialogTrafoSE::UstawPole(const wxString& n, const wxString& v) {
    if (const int w = WymiaryRdzenia::Indeks(n); w >= 0) { wym_.t[w]->SetValue(v); return; }
    if (int k = Idx(n); k >= 0) tc_[k]->SetValue(v);
}
wxString DialogTrafoSE::Wynik(const wxString& n) const {
    for (int k = 0; k < kWyn; k++) if (n == kKlucze[k]) return lb_[k]->GetLabel();
    return wxString();
}

namespace {
bool Wczytaj(const wxTextCtrl* const* tc, int spadekDb, TrafoSE::Wejscie& w, bool zOknem) {
    double v[16] = {};
    for (int k = 0; k < 16; k++) {
        if (k == 7) continue;
        if (k >= 13 && !zOknem) continue;
        if (!tekstNaLiczbe(tc[k]->GetValue().utf8_string(), v[k])) return false;
    }
    w.spadekDb = spadekDb;
    w.mocVA = v[0]; w.raOm = v[1]; w.riOm = v[2]; w.iaMa = v[3]; w.fdHz = v[4]; w.rgOm = v[5]; w.eta = v[6];
    w.przekrojCm2 = v[8]; w.drogaCm = v[9]; w.bMaxT = v[10]; w.mi = v[11]; w.gestoscAmm2 = v[12];
    if (zOknem) { w.dlugoscZwojuCm = v[13]; w.wysokoscOknaCm = v[14]; w.szerokoscOknaCm = v[15]; }
    return true;
}
}

DialogTrafoSEWykonane* DialogTrafoSE::OtworzWykonane() {
    TrafoSE::Wejscie w;
    if (!Wczytaj(tc_, chSpadek_->GetSelection() + 1, w, true)) return nullptr;
    const auto p = TrafoSE::oblicz(w);
    if (!p) return nullptr;
    return new DialogTrafoSEWykonane(this, w, *p);
}

void DialogTrafoSE::Dalej() {
    DialogTrafoSEWykonane* d = OtworzWykonane();
    if (!d) return;
    d->ShowModal();
    d->Destroy();
}

void DialogTrafoSE::Przelicz() {
    for (wxStaticText* l : lb_) l->SetLabel("");
    ostrz_->SetLabel("");
    TrafoSE::Wejscie w;
    if (bDalej_) bDalej_->Disable();
    if (!Wczytaj(tc_, chSpadek_->GetSelection() + 1, w, false)) return;
    const auto r = TrafoSE::oblicz(w);
    if (!r) return;
    TrafoSE::Wejscie we = w;
    const bool okno = Wczytaj(tc_, chSpadek_->GetSelection() + 1, we, true) && we.dlugoscZwojuCm > 0 && we.wysokoscOknaCm > 0 && we.szerokoscOknaCm > 0;
    if (bDalej_) bDalej_->Enable(okno);
    const double x[13] = {r->przekrojOrientacyjnyCm2, r->liH, r->liPorownanieH, r->przekladnia, r->przekladniaPorownanie, r->pradSzczytowyMa,
                          double(r->zwojePierwotne), r->drutPierwotnyMm, double(r->zwojeWtorne), r->drutWtornyMm, r->szczelinaMm, r->indukcjaStalaT,
                          r->indukcjaSzczytowaT};
    const int m[13] = {2, 4, 4, 3, 3, 1, 0, 3, 0, 3, 3, 3, 3};
    lb_[18]->SetLabel(L(r->amperozwojeNaCm, 2));
    for (int k = 0; k < 13; k++) lb_[k]->SetLabel(L(x[k], m[k]));
    // porównanie z modułem Cykina (tube_magnetics::calcSETransformer): wymiary rdzenia z pól, Bdc = 0,7 T wg modułu
    double pakiet = 0;
    if (okno && tekstNaLiczbe(tc_[7]->GetValue().utf8_string(), pakiet) && pakiet > 0) {
        try {
            tube::magnetics::CoreGeometry c{w.przekrojCm2 * 1e-4 / (pakiet * 1e-3), we.szerokoscOknaCm * 1e-2, we.wysokoscOknaCm * 1e-2,
                                            pakiet * 1e-3, 1.0, w.drogaCm * 1e-2};
            const auto cy = tube::magnetics::calcSETransformer(c, w.raOm, w.riOm, w.rgOm, w.mocVA, w.iaMa * 1e-3, w.fdHz, 20000.0, 0.7,
                                                               w.gestoscAmm2 * 1e6);
            lb_[13]->SetLabel(L(cy.N1, 0));
            lb_[14]->SetLabel(L(cy.airGapTotal * 1e3, 3));
            lb_[15]->SetLabel(L(cy.L_prim, 4));
            lb_[16]->SetLabel(L(cy.B_total_peak, 3));
            lb_[17]->SetLabel(L(cy.efficiency, 3));
        } catch (...) {}
    }
    {   // porównanie: Krize i Witort
        TrafoSE::Wejscie wl = we;
        wl.wspolczynnikAKrize = kAKrize[std::max(0, chA_->GetSelection())];
        const auto lt = TrafoSE::literatura(wl, *r);
        lb_[19]->SetLabel(L(lt.krizeZwoje23, 0));
        lb_[20]->SetLabel(L(lt.krizeZwoje24, 0));
        lb_[21]->SetLabel(L(lt.krizeZwojeWtorne, 0));
        lb_[22]->SetLabel(L(lt.krizeSzczelinaMm, 3));
        lb_[23]->SetLabel(L(lt.krizePrzekladkaMm, 3));
        if (lt.krizeDrutPierwotnyMm > 0) {
            lb_[24]->SetLabel(L(lt.krizeDrutPierwotnyMm, 3));
            lb_[26]->SetLabel(L(lt.krizeDrutWtornyMm, 3));
        }
        lb_[25]->SetLabel(L(lt.krizeDrutMinMm, 3));
        lb_[27]->SetLabel(lt.krizeIloczynRdzeniaCm4 > 0 ? L(lt.krizeIloczynRdzeniaCm4, 1) + " / " + L(lt.krizeIloczynWymaganyCm4, 1) : L(lt.krizeIloczynWymaganyCm4, 1));
        lb_[28]->SetLabel(L(lt.witortQMinCm2, 1) + " - " + L(lt.witortQMaxCm2, 1));
        lb_[29]->SetLabel(L(lt.witortZwoje, 0));
        lb_[30]->SetLabel(L(lt.witortL1H, 4));
    }
    wxString u;
    if (r->przesterowanie)
        u += TR("Uwaga: żądana moc jest większa niż lampa odda w klasie A przy tym prądzie anodowym (maks. ok. ") + L(r->mocMaxKlasaAVA, 1) + TR(" VA). ");
    if (r->brakSzczeliny) u += TR("Model liniowy nie wymaga szczeliny (żelazo samo daje potrzebną indukcyjność), ale przy podmagnesowaniu przenikalność przyrostowa spada: zostaw szczelinę co najmniej 0,05 mm. ");
    if (r->amperozwojeNaCm > 2) u += TR("Podmagnesowanie przekracza 2 A·zw./cm: wg Cykina (XI.3) rdzeń musi mieć szczelinę. ");
    if (w.przekrojCm2 < r->przekrojOrientacyjnyCm2) u += TR("Przekrój rdzenia jest mniejszy od orientacyjnego. ");
    ostrz_->SetLabel(u);
    ostrz_->Wrap(330);
}
