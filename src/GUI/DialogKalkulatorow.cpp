#include "DialogKalkulatorow.h"
#include <wx/statline.h>
#include "../Logika/Kalkulatory.h"
#include "../Logika/KalkulatoryAnalogowe.h"
#include "../Logika/CewkaPowietrzna.h"
#include "../Logika/StabilizatorRegulowany.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"
#include "PanelSchematu.h"

#include <cmath>
#include <optional>

namespace {

std::optional<double> Napiecie(const wxTextCtrl* tc) {
    double v = 0;
    if (!tekstNaLiczbe(tc->GetValue().utf8_string(), v)) return std::nullopt;
    return v;
}
std::optional<double> Opor(const wxTextCtrl* tc) {
    return SzeregiE::parsujWartosc(tc->GetValue().utf8_string(), RodzajElementu::Rezystor);
}
std::optional<double> Pojemnosc(const wxTextCtrl* tc) {
    return SzeregiE::parsujWartosc(tc->GetValue().utf8_string(), RodzajElementu::Kondensator);
}
/// Częstotliwość: "1k", "159", "2,2M", opcjonalnie z "Hz"
std::optional<double> Czestotliwosc(const wxTextCtrl* tc) {
    std::string t = tc->GetValue().utf8_string();
    for (const char* jedn : {"Hz", "hz", "HZ"}) {
        const size_t p = t.rfind(jedn);
        if (p != std::string::npos && p + 2 == t.size()) t.erase(p);
    }
    auto v = SzeregiE::parsujWartosc(t, RodzajElementu::Rezystor);   // k, M, G jak w rezystorach
    return v;
}

wxString Op(double v) { return TekstZUTF8(SzeregiE::formatZJednostka(v, RodzajElementu::Rezystor)); }
wxString Pf(double v) { return TekstZUTF8(SzeregiE::formatZJednostka(v, RodzajElementu::Kondensator)); }
wxString Liczba(double v, int miejsc = 2) { return TekstZUTF8(liczbaNaTekstPolski(v, miejsc, 0)); }

wxChoice* WyborSzeregu(wxWindow* p, Szereg domyslny) {
    wxChoice* c = new wxChoice(p, wxID_ANY);
    for (Szereg s : {Szereg::E6, Szereg::E12, Szereg::E24, Szereg::E48, Szereg::E96}) c->Append(TekstZUTF8(SzeregiE::nazwa(s)));
    c->SetSelection(static_cast<int>(domyslny));
    return c;
}
Szereg Wybrany(const wxChoice* c) { return static_cast<Szereg>(std::max(0, c->GetSelection())); }

wxTextCtrl* Pole(wxWindow* p, wxFlexGridSizer* g, const wxString& etykieta, const wxString& wartosc, const char* klucz) {
    g->Add(new wxStaticText(p, wxID_ANY, etykieta), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl* t = new wxTextCtrl(p, wxID_ANY, wartosc, wxDefaultPosition, wxSize(130, -1));
    PomocGUI::Dodaj(t, klucz);
    g->Add(t, 0);
    return t;
}

/// Pole opcjonalne: puste = 0 ("nie licz"), błędny tekst = nullopt
std::optional<double> OpcjaOpor(const wxTextCtrl* tc) {
    if (tc->GetValue().Trim().empty()) return 0.0;
    return SzeregiE::parsujWartosc(tc->GetValue().utf8_string(), RodzajElementu::Rezystor);
}
std::optional<double> OpcjaLiczba(const wxTextCtrl* tc) {
    if (tc->GetValue().Trim().empty()) return 0.0;
    double v = 0;
    if (!tekstNaLiczbe(tc->GetValue().utf8_string(), v)) return std::nullopt;
    return v;
}
std::optional<double> OpcjaCzest(const wxTextCtrl* tc) {
    if (tc->GetValue().Trim().empty()) return 0.0;
    std::string t = tc->GetValue().utf8_string();
    const size_t p = t.rfind("Hz");
    if (p != std::string::npos && p + 2 == t.size()) t.erase(p);
    return SzeregiE::parsujWartosc(t, RodzajElementu::Rezystor);
}

wxStaticText* Opis(wxWindow* p, wxSizer* s, const wxString& tekst) {
    wxStaticText* l = new wxStaticText(p, wxID_ANY, tekst);
    l->Wrap(470);
    s->Add(l, 0, wxALL, 10);
    return l;
}

wxStaticText* Wynik(wxWindow* p, wxSizer* s, bool pogrubiony) {
    wxStaticText* l = new wxStaticText(p, wxID_ANY, wxEmptyString);
    if (pogrubiony) l->SetFont(l->GetFont().Scaled(1.2f).Bold());
    s->Add(l, 0, wxALL, 10);
    return l;
}

} // namespace

DialogKalkulatorow::DialogKalkulatorow(wxWindow* parent, bool tylkoCewki)
    : wxDialog(parent, wxID_ANY, tylkoCewki ? TR("Elementy indukcyjne: cewki powietrzne") : TR("Narzędzia: kalkulatory"), wxDefaultPosition,
               tylkoCewki ? wxSize(900, 640) : wxSize(1000, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    ksiazka = new wxNotebook(this, wxID_ANY);
    if (tylkoCewki) {
        ksiazka->AddPage(StronaCewki(false), TR("Cewka jednowarstwowa"));
        ksiazka->AddPage(StronaCewki(true), TR("Cewka wielowarstwowa"));
    } else {
        ksiazka->AddPage(StronaDzielnika(), TR("Dzielnik napięcia"));
        ksiazka->AddPage(StronaLED(), TR("Rezystor do LED"));
        ksiazka->AddPage(StronaRC(), TR("Filtr RC"));
        ksiazka->AddPage(StronaZasilacza(), TR("Zasilacz (trafo)"));
        ksiazka->AddPage(StronaTranzystora(), TR("Tranzystor"));
        ksiazka->AddPage(StronaESR(), TR("ESR kondensatorów"));
        ksiazka->AddPage(StronaLM(), TR("LM317 / LM337"));
    }
    glowny->Add(ksiazka, 1, wxEXPAND | wxALL, 10);
    glowny->Add(new wxButton(this, wxID_CLOSE, TR("Zamknij")), 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    SetSizer(glowny);
    SetEscapeId(wxID_CLOSE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    if (tylkoCewki) {
        PrzeliczCewke(false);
        PrzeliczCewke(true);
        return;
    }
    PrzeliczDzielnik();
    PrzeliczLED();
    PrzeliczRC();
    PrzeliczZasilacz();
    PrzeliczTranzystor();
    PrzeliczESR();
    PrzeliczLM();
}

// ---------------------------------------------------------------- dzielnik
wxPanel* DialogKalkulatorow::StronaDzielnika() {
    wxPanel* p = new wxPanel(ksiazka);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    Opis(p, s, TR("Uwy = Uwe · R2 / (R1 + R2).  Wpisz napięcia, a program dobierze rezystory z wybranego szeregu."));
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    tcUwe = Pole(p, g, TR("Napięcie wejściowe Uwe [V]:"), "5", "kalk.uwe");
    tcUwy = Pole(p, g, TR("Napięcie wyjściowe Uwy [V]:"), "3,3", "kalk.uwy");
    g->Add(new wxStaticText(p, wxID_ANY, TR("Szereg:")), 0, wxALIGN_CENTER_VERTICAL);
    chSzeregD = WyborSzeregu(p, Szereg::E24);
    PomocGUI::Dodaj(chSzeregD, "kalk.szereg");
    g->Add(chSzeregD, 0);
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbDzielnik = Wynik(p, s, true);
    s->Add(new wxStaticLine(p), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    Opis(p, s, TR("Sprawdź własne rezystory (Uwe z góry):"));
    wxFlexGridSizer* g2 = new wxFlexGridSizer(2, 6, 8);
    tcR1 = Pole(p, g2, TR("R1 (do plusa):"), "10k", "kalk.r1");
    tcR2 = Pole(p, g2, TR("R2 (do masy):"), "20k", "kalk.r2");
    s->Add(g2, 0, wxLEFT | wxRIGHT, 10);
    lbDzielnik2 = Wynik(p, s, false);
    PanelSchematu::Obok(p, s, PanelSchematu::Rodzaj::Dzielnik);
    for (wxTextCtrl* t : {tcUwe, tcUwy, tcR1, tcR2}) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczDzielnik(); });
    chSzeregD->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczDzielnik(); });
    return p;
}

void DialogKalkulatorow::UstawDzielnik(const wxString& uwe, const wxString& uwy, const wxString& r1, const wxString& r2) {
    tcUwe->ChangeValue(uwe); tcUwy->ChangeValue(uwy); tcR1->ChangeValue(r1); tcR2->ChangeValue(r2);
    PrzeliczDzielnik();
}

void DialogKalkulatorow::PrzeliczDzielnik() {
    const auto uwe = Napiecie(tcUwe), uwy = Napiecie(tcUwy);
    if (!uwe || !uwy) {
        lbDzielnik->SetLabel(TR("Wpisz napięcia, np. 5 i 3,3."));
    } else if (auto w = Kalkulatory::dobierzDzielnik(*uwe, *uwy, Wybrany(chSzeregD))) {
        lbDzielnik->SetLabel("R1 = " + Op(w->r1) + ",  R2 = " + Op(w->r2) + "\n" + TR("Uwy = ") + Liczba(w->uwy, 3) + " V  (" +
                             TekstZUTF8(std::string(w->bladProc > 0 ? "+" : "") + liczbaNaTekstPolski(w->bladProc, 2, 2)) + " %)\n" + TR("Prąd dzielnika: ") + Liczba(w->pradMA, 3) + " mA");
    } else {
        lbDzielnik->SetLabel(TR("Uwy musi być dodatnie i mniejsze niż Uwe."));
    }
    const auto r1 = Opor(tcR1), r2 = Opor(tcR2);
    if (!uwe || !r1 || !r2) {
        lbDzielnik2->SetLabel(TR("Wpisz Uwe oraz R1 i R2 (np. 10k, 4k7)."));
    } else if (auto u = Kalkulatory::napiecieDzielnika(*uwe, *r1, *r2)) {
        lbDzielnik2->SetLabel(TR("Uwy = ") + Liczba(*u, 3) + " V,  " + TR("prąd ") + Liczba(*uwe / (*r1 + *r2) * 1000.0, 3) + " mA");
    } else {
        lbDzielnik2->SetLabel(TR("R2 musi być większe od zera."));
    }
}

// ---------------------------------------------------------------- LED
wxPanel* DialogKalkulatorow::StronaLED() {
    wxPanel* p = new wxPanel(ksiazka);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    Opis(p, s, TR("R = (Uzas - Uf) / I.  Wybierana jest najbliższa NIE MNIEJSZA wartość z szeregu, żeby nie przekroczyć prądu diody."));
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    tcUzas = Pole(p, g, TR("Napięcie zasilania [V]:"), "5", "kalk.uzas");
    tcUf = Pole(p, g, TR("Napięcie diody Uf [V]:"), "2", "kalk.uf");
    tcI = Pole(p, g, TR("Prąd diody [mA]:"), "20", "kalk.iled");
    g->Add(new wxStaticText(p, wxID_ANY, TR("Szereg:")), 0, wxALIGN_CENTER_VERTICAL);
    chSzeregL = WyborSzeregu(p, Szereg::E24);
    PomocGUI::Dodaj(chSzeregL, "kalk.szereg");
    g->Add(chSzeregL, 0);
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbLED = Wynik(p, s, true);
    Opis(p, s, TR("Typowe Uf: czerwona 1,8-2,2 V, żółta/zielona 2,0-2,4 V, niebieska/biała 3,0-3,4 V."));
    PanelSchematu::Obok(p, s, PanelSchematu::Rodzaj::LED);
    for (wxTextCtrl* t : {tcUzas, tcUf, tcI}) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczLED(); });
    chSzeregL->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczLED(); });
    return p;
}

void DialogKalkulatorow::UstawLED(const wxString& uzas, const wxString& uf, const wxString& pradMA) {
    tcUzas->ChangeValue(uzas); tcUf->ChangeValue(uf); tcI->ChangeValue(pradMA);
    PrzeliczLED();
}

void DialogKalkulatorow::PrzeliczLED() {
    const auto uz = Napiecie(tcUzas), uf = Napiecie(tcUf), i = Napiecie(tcI);
    if (!uz || !uf || !i) { lbLED->SetLabel(TR("Wpisz trzy wartości liczbowe.")); return; }
    if (auto w = Kalkulatory::rezystorLED(*uz, *uf, *i, Wybrany(chSzeregL))) {
        lbLED->SetLabel(TR("Rezystor: ") + Op(w->rZSzeregu) + "  (" + TR("wyliczone ") + Op(w->rIdealny) + ")\n" +
                        TR("Prąd z tym rezystorem: ") + Liczba(w->pradRzeczywistyMA, 2) + " mA\n" +
                        TR("Moc rezystora: ") + Liczba(w->mocRezystoraMW, 1) + " mW,  " + TR("diody: ") + Liczba(w->mocLEDMW, 1) + " mW");
    } else {
        lbLED->SetLabel(TR("Napięcie zasilania musi być większe od Uf, a prąd dodatni."));
    }
}

// ---------------------------------------------------------------- RC
wxPanel* DialogKalkulatorow::StronaRC() {
    wxPanel* p = new wxPanel(ksiazka);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    Opis(p, s, TR("Filtr RC pierwszego rzędu: fc = 1 / (2π · R · C)."));
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    tcRC_R = Pole(p, g, TR("Rezystancja R:"), "10k", "kalk.rc_r");
    tcRC_C = Pole(p, g, TR("Pojemność C:"), "100n", "kalk.rc_c");
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbRC = Wynik(p, s, true);
    s->Add(new wxStaticLine(p), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    Opis(p, s, TR("Dobierz element dla zadanej częstotliwości granicznej:"));
    wxFlexGridSizer* g2 = new wxFlexGridSizer(2, 6, 8);
    tcRC_f = Pole(p, g2, TR("Częstotliwość fc [Hz]:"), "1k", "kalk.rc_f");
    s->Add(g2, 0, wxLEFT | wxRIGHT, 10);
    lbRC2 = Wynik(p, s, false);
    PanelSchematu::Obok(p, s, PanelSchematu::Rodzaj::FiltrRC);
    for (wxTextCtrl* t : {tcRC_R, tcRC_C, tcRC_f}) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczRC(); });
    return p;
}

void DialogKalkulatorow::UstawRC(const wxString& r, const wxString& c, const wxString& f) {
    tcRC_R->ChangeValue(r); tcRC_C->ChangeValue(c); tcRC_f->ChangeValue(f);
    PrzeliczRC();
}

void DialogKalkulatorow::PrzeliczRC() {
    const auto r = Opor(tcRC_R), c = Pojemnosc(tcRC_C), f = Czestotliwosc(tcRC_f);
    if (r && c) {
        const double fc = Kalkulatory::czestotliwoscGranicznaRC(*r, *c);
        lbRC->SetLabel(TR("fc = ") + TekstZUTF8(Kalkulatory::formatCzestotliwosci(fc)) + TR(",  τ = ") +
                       TekstZUTF8(Kalkulatory::formatCzasu(Kalkulatory::stalaCzasowaRC(*r, *c))));
    } else {
        lbRC->SetLabel(TR("Wpisz R i C (np. 10k, 100n)."));
    }
    if (!f) { lbRC2->SetLabel(TR("Wpisz częstotliwość (np. 1k, 440).")); return; }
    wxString t;
    if (c) {
        const double rr = Kalkulatory::rDlaFiltru(*f, *c);
        t += TR("Dla C = ") + Pf(*c) + ":  R = " + Op(rr) + "  (" + TR("z szeregu E24: ") + Op(Kalkulatory::najblizsza(rr, Szereg::E24)) + ")";
    }
    if (r) {
        const double cc = Kalkulatory::cDlaFiltru(*f, *r);
        t += (t.empty() ? "" : "\n") + TR("Dla R = ") + Op(*r) + ":  C = " + Pf(cc) + "  (" + TR("z szeregu E12: ") + Pf(Kalkulatory::najblizsza(cc, Szereg::E12)) + ")";
    }
    lbRC2->SetLabel(t);
}

// ---------------------------------------------------------------- zasilacz z trafa sieciowego
wxPanel* DialogKalkulatorow::StronaZasilacza() {
    wxPanel* p = new wxPanel(ksiazka);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    Opis(p, s, TR("Napięcie szczytowe = Usek·√2 − spadek na diodach. Tętnienia Upp = I / (f·C), gdzie f to 100 Hz (mostek, odczep) albo 50 Hz (jedna połówka)."));
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    g->Add(new wxStaticText(p, wxID_ANY, TR("Prostownik:")), 0, wxALIGN_CENTER_VERTICAL);
    chProst = new wxChoice(p, wxID_ANY);
    chProst->Append(TR("Mostek"));
    chProst->Append(TR("Dwie połówki (uzwojenie z odczepem)"));
    chProst->Append(TR("Jedna połówka"));
    chProst->SetSelection(0);
    PomocGUI::Dodaj(chProst, "kalk.zas_typ");
    g->Add(chProst, 0);
    g->Add(new wxStaticText(p, wxID_ANY, TR("Sieć:")), 0, wxALIGN_CENTER_VERTICAL);
    chSiec = new wxChoice(p, wxID_ANY);
    chSiec->Append("50 Hz");
    chSiec->Append("60 Hz");
    chSiec->SetSelection(0);
    PomocGUI::Dodaj(chSiec, "kalk.zas_siec");
    g->Add(chSiec, 0);
    poleZas.push_back(Pole(p, g, TR("Napięcie wtórne Usek [V]:"), "18", "kalk.zas_u"));
    poleZas.push_back(Pole(p, g, TR("Prąd obciążenia [A]:"), "1", "kalk.zas_i"));
    poleZas.push_back(Pole(p, g, TR("Kondensator C:"), "4700u", "kalk.zas_c"));
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbZas = Wynik(p, s, true);
    s->Add(new wxStaticLine(p), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    Opis(p, s, TR("Odwrotnie: jakie napięcie wtórne i kondensator dla zadanego wyjścia (przy prądzie z góry):"));
    wxFlexGridSizer* g2 = new wxFlexGridSizer(2, 6, 8);
    poleZas.push_back(Pole(p, g2, TR("Potrzebne napięcie wyjściowe [V]:"), "12", "kalk.zas_wy"));
    poleZas.push_back(Pole(p, g2, TR("Dopuszczalne tętnienia Upp [V]:"), "1", "kalk.zas_upp"));
    s->Add(g2, 0, wxLEFT | wxRIGHT, 10);
    lbZas2 = Wynik(p, s, false);
    psZas = PanelSchematu::Obok(p, s, PanelSchematu::Rodzaj::ZasilaczMostek);
    for (wxTextCtrl* t : poleZas) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczZasilacz(); });
    chProst->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczZasilacz(); });
    chSiec->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczZasilacz(); });
    return p;
}

void DialogKalkulatorow::PrzeliczZasilacz() {
    const Kalkulatory::Prostownik pr = static_cast<Kalkulatory::Prostownik>(std::max(0, chProst->GetSelection()));
    if (psZas) psZas->UstawRodzaj(pr == Kalkulatory::Prostownik::Mostek ? PanelSchematu::Rodzaj::ZasilaczMostek
                                  : pr == Kalkulatory::Prostownik::DwiePolowki ? PanelSchematu::Rodzaj::ZasilaczOdczep : PanelSchematu::Rodzaj::Zasilacz);
    const double f = chSiec->GetSelection() == 1 ? 60.0 : 50.0;
    const auto u = Napiecie(poleZas[0]), i = Napiecie(poleZas[1]), c = Pojemnosc(poleZas[2]);
    if (!u || !i || !c) {
        lbZas->SetLabel(TR("Wpisz Usek, prąd i kondensator (np. 18, 1, 4700u)."));
    } else if (auto w = Kalkulatory::zasilaczZTrafa(*u, *i, *c, pr, f)) {
        lbZas->SetLabel(TR("Napięcie na wyjściu: ") + Liczba(w->uSrednia, 1) + " V  (" + TR("szczyt ") + Liczba(w->uSzczytowa, 1) + " V, " +
                        TR("tętnienia ") + Liczba(w->tetnieniaPP, 2) + " Vpp, " + Liczba(w->czestTetnien, 0) + " Hz)\n" +
                        TR("Moc wyjściowa: ") + Liczba(w->mocDCW, 1) + " W\n" +
                        TR("Transformator: co najmniej ") + Liczba(w->mocTrafoVA, 0) + " VA,  " + TR("prąd wtórny ok. ") + Liczba(w->pradWtornyA, 2) + " A\n" +
                        TR("Prąd szczytowy diody: ok. ") + Liczba(w->pradSkokowyDiodyA, 0) + " A");
    } else {
        lbZas->SetLabel(TR("Dane muszą być dodatnie, a napięcie wtórne wystarczająco duże (po odjęciu diod i tętnień)."));
    }
    const auto uw = Napiecie(poleZas[3]), upp = Napiecie(poleZas[4]);
    if (!uw || !upp || !i) {
        lbZas2->SetLabel(TR("Wpisz napięcie wyjściowe i tętnienia (prąd z pola wyżej)."));
    } else if (auto us = Kalkulatory::napiecieWtorneDlaWyjscia(*uw, *upp, pr)) {
        const double cc = Kalkulatory::kondensatorFiltra(*i, *upp, pr, f);
        lbZas2->SetLabel(TR("Usek co najmniej ") + Liczba(*us, 1) + " V,  " + TR("C co najmniej ") + (cc > 0 ? Pf(cc) : wxString("?")) +
                         "  (" + TR("z szeregu E12: ") + (cc > 0 ? Pf(Kalkulatory::najblizszaWieksza(cc, Szereg::E12)) : wxString("?")) + ")");
    } else {
        lbZas2->SetLabel(TR("Napięcie wyjściowe musi być dodatnie."));
    }
}

// ---------------------------------------------------------------- tranzystor
wxPanel* DialogKalkulatorow::StronaTranzystora() {
    wxPanel* p = new wxPanel(ksiazka);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    Opis(p, s, TR("Tranzystor NPN w układzie OE z dzielnikiem bazowym R1/R2 i rezystorem emitera RE. Przybliżenie: Vbe = 0,65 V, β stałe."));
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    poleTR.push_back(Pole(p, g, TR("Zasilanie Vcc [V]:"), "12", "kalk.tr_vcc"));
    poleTR.push_back(Pole(p, g, TR("R1 (do plusa):"), "47k", "kalk.tr_r1"));
    poleTR.push_back(Pole(p, g, TR("R2 (do masy):"), "10k", "kalk.tr_r2"));
    poleTR.push_back(Pole(p, g, TR("RC (kolektor):"), "2k2", "kalk.tr_rc"));
    poleTR.push_back(Pole(p, g, TR("RE (emiter):"), "470", "kalk.tr_re"));
    poleTR.push_back(Pole(p, g, TR("Wzmocnienie β:"), "100", "kalk.tr_beta"));
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbTR = Wynik(p, s, true);
    s->Add(new wxStaticLine(p), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    Opis(p, s, TR("Dobór wartości: VE = 10 % Vcc, VCE = Vcc/2, prąd dzielnika ok. 10·Ib (Vcc i β z pól wyżej)."));
    wxFlexGridSizer* g2 = new wxFlexGridSizer(2, 6, 8);
    poleTR.push_back(Pole(p, g2, TR("Prąd kolektora Ic [mA]:"), "2", "kalk.tr_ic"));
    g2->Add(new wxStaticText(p, wxID_ANY, TR("Szereg:")), 0, wxALIGN_CENTER_VERTICAL);
    chSzeregT = WyborSzeregu(p, Szereg::E24);
    PomocGUI::Dodaj(chSzeregT, "kalk.szereg");
    g2->Add(chSzeregT, 0);
    s->Add(g2, 0, wxLEFT | wxRIGHT, 10);
    lbTR2 = Wynik(p, s, false);
    PanelSchematu::Obok(p, s, PanelSchematu::Rodzaj::Tranzystor);
    for (wxTextCtrl* t : poleTR) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczTranzystor(); });
    chSzeregT->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczTranzystor(); });
    return p;
}

void DialogKalkulatorow::PrzeliczTranzystor() {
    const auto vcc = Napiecie(poleTR[0]), r1 = Opor(poleTR[1]), r2 = Opor(poleTR[2]), rc = Opor(poleTR[3]), re = OpcjaOpor(poleTR[4]), beta = Napiecie(poleTR[5]);
    if (!vcc || !r1 || !r2 || !rc || !re || !beta) {
        lbTR->SetLabel(TR("Wpisz wszystkie wartości (RE może być puste)."));
    } else if (auto w = Kalkulatory::punktPracyTranzystora(*vcc, *r1, *r2, *rc, *re, *beta)) {
        wxString stan = w->stan == Kalkulatory::StanTranzystora::Nasycenie ? TR("NASYCENIE (Vce ≈ 0,2 V)") :
                        w->stan == Kalkulatory::StanTranzystora::Zatkany ? TR("ZATKANY (Vb poniżej 0,65 V)") : TR("praca liniowa");
        lbTR->SetLabel(TR("Ic = ") + Liczba(w->icMA, 2) + " mA,  Ib = " + Liczba(w->ibUA, 1) + wxString::FromUTF8(" \xC2\xB5" "A,  Vce = ") + Liczba(w->vceV, 2) + " V\n" +
                       TR("Vb = ") + Liczba(w->vbV, 2) + " V,  Ve = " + Liczba(w->veV, 2) + " V,  " + TR("moc tranzystora ") + Liczba(w->mocTranzystoraMW, 1) + " mW\n" +
                       TR("Stan: ") + stan + (w->wzmocnienieNapiecioweOE > 0 ? "\n" + TR("Wzmocnienie z zablokowanym RE: ok. ") + Liczba(w->wzmocnienieNapiecioweOE, 0) + TR(" V/V") : wxString()));
    } else {
        lbTR->SetLabel(TR("Wartości muszą być dodatnie (RE może być zerem)."));
    }
    const auto ic = Napiecie(poleTR[6]);
    if (!vcc || !beta || !ic) { lbTR2->SetLabel(TR("Wpisz Vcc, β i prąd kolektora.")); return; }
    if (auto d = Kalkulatory::dobierzTranzystor(*vcc, *ic, *beta, Wybrany(chSzeregT))) {
        lbTR2->SetLabel("R1 = " + Op(d->r1) + ",  R2 = " + Op(d->r2) + ",  RC = " + Op(d->rc) + ",  RE = " + Op(d->re) + "\n" +
                        TR("Daje: Ic = ") + Liczba(d->wynik.icMA, 2) + " mA,  Vce = " + Liczba(d->wynik.vceV, 2) + " V");
    } else {
        lbTR2->SetLabel(TR("Za małe Vcc albo β, albo prąd nie dodatni."));
    }
}

// ---------------------------------------------------------------- ESR kondensatorów elektrolitycznych
namespace {
wxString EsrTekst(double e) {
    if (!(e > 0)) return "-";
    if (e < 1.0) return Liczba(e * 1000.0, e < 0.01 ? 1 : 0) + wxString::FromUTF8(" m\u03a9");
    return Liczba(e, e < 10 ? 2 : 1) + wxString::FromUTF8(" \u03a9");
}
const double kPojemnosciESR[] = {1e-6, 2.2e-6, 4.7e-6, 10e-6, 22e-6, 47e-6, 100e-6, 220e-6, 470e-6, 1000e-6, 2200e-6, 4700e-6, 10000e-6};
const double kNapieciaESR[] = {6.3, 10, 16, 25, 35, 50, 63, 100, 250, 400};
}  // namespace

wxPanel* DialogKalkulatorow::StronaESR() {
    wxPanel* p = new wxPanel(ksiazka);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* lewa = new wxBoxSizer(wxVERTICAL);
    Opis(p, lewa, TR("ESR = tg δ / (2π f C), tg δ zależy od napięcia znamionowego (przy 120 Hz). To orientacyjny rząd wielkości dla kondensatorów aluminiowych ogólnego przeznaczenia; dokładną wartość podaje karta katalogowa (serie Low ESR mają kilka razy mniej)."));
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    poleESR.push_back(Pole(p, g, TR("Pojemność C:"), "1000u", "kalk.esr_c"));
    poleESR.push_back(Pole(p, g, TR("Napięcie znamionowe U [V]:"), "25", "kalk.esr_u"));
    poleESR.push_back(Pole(p, g, TR("Prąd tętnień [A] (opcja):"), "1", "kalk.esr_i"));
    lewa->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbESR = Wynik(p, lewa, true);
    gora->Add(lewa, 1, wxEXPAND);
    gora->Add(new PanelSchematu(p, PanelSchematu::Rodzaj::KondensatorESR), 0, wxALL, 8);
    s->Add(gora, 0, wxEXPAND);
    s->Add(new wxStaticText(p, wxID_ANY, TR("Typowy ESR przy 120 Hz w zależności od pojemności i napięcia znamionowego:")), 0, wxLEFT | wxRIGHT, 10);
    tabelaESR = new wxListCtrl(p, wxID_ANY, wxDefaultPosition, wxSize(-1, 250), wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL);
    PomocGUI::Dodaj(tabelaESR, "kalk.esr_tabela");
    tabelaESR->AppendColumn("C \\ U [V]", wxLIST_FORMAT_LEFT, 80);
    for (double u : kNapieciaESR) tabelaESR->AppendColumn(Liczba(u, u < 10 ? 1 : 0), wxLIST_FORMAT_RIGHT, 66);
    long w = tabelaESR->InsertItem(0, wxString::FromUTF8("tg \u03b4"));
    for (int j = 0; j < 10; ++j) tabelaESR->SetItem(w, j + 1, Liczba(Kalkulatory::tgDeltaTypowy(kNapieciaESR[j]), 2));
    for (double c : kPojemnosciESR) {
        w = tabelaESR->InsertItem(tabelaESR->GetItemCount(), Pf(c));
        for (int j = 0; j < 10; ++j) tabelaESR->SetItem(w, j + 1, EsrTekst(Kalkulatory::esrTypowy(c, kNapieciaESR[j])));
    }
    s->Add(tabelaESR, 1, wxEXPAND | wxALL, 10);
    p->SetSizer(s);
    for (wxTextCtrl* t : poleESR) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczESR(); });
    return p;
}

void DialogKalkulatorow::PrzeliczESR() {
    const auto c = Pojemnosc(poleESR[0]), u = Napiecie(poleESR[1]), i = OpcjaLiczba(poleESR[2]);
    if (!c || !u || !i || !(*c > 0) || !(*u > 0)) { lbESR->SetLabel(TR("Wpisz C i napięcie (np. 1000u, 25).")); return; }
    const double tg = Kalkulatory::tgDeltaTypowy(*u);
    const double esr = Kalkulatory::esrTypowy(*c, *u);
    wxString t = TR("tg δ ≈ ") + Liczba(tg, 2) + wxString::FromUTF8(",  ESR \u2248 ") + EsrTekst(esr) + TR(" (120 Hz)");
    if (*i > 0) t += "\n" + TR("Moc strat w kondensatorze: ") + Liczba(Kalkulatory::mocStratESR(*i, esr) * 1000.0, 1) + " mW";
    lbESR->SetLabel(t);
}

void DialogKalkulatorow::UstawESR(const wxString& c, const wxString& u, const wxString& i) {
    poleESR[0]->ChangeValue(c); poleESR[1]->ChangeValue(u); poleESR[2]->ChangeValue(i);
    PrzeliczESR();
}

wxString DialogKalkulatorow::KomorkaESR(int wiersz, int kolumna) const {
    if (wiersz < 0 || wiersz >= tabelaESR->GetItemCount()) return wxString();
    wxListItem it; it.SetId(wiersz); it.SetColumn(kolumna); it.SetMask(wxLIST_MASK_TEXT);
    return tabelaESR->GetItem(it) ? it.GetText() : wxString();
}

// ---------------------------------------------------------------- LM317 / LM337
wxPanel* DialogKalkulatorow::StronaLM() {
    wxPanel* p = new wxPanel(ksiazka);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    Opis(p, s, TR("Uwy = Vref · (1 + R2/R1) + Iadj · R2.  Przez R1 płynie Vref/R1 (Vref ≈ 1,25 V), a prąd końcówki ADJ (ok. 50 µA) dopływa jeszcze przez R2. LM337 daje to samo napięcie, ale ujemne. Vref i Iadj zależą od producenta - sprawdź kartę katalogową."));
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    g->Add(new wxStaticText(p, wxID_ANY, TR("Stabilizator:")), 0, wxALIGN_CENTER_VERTICAL);
    chTypLM = new wxChoice(p, wxID_ANY);
    chTypLM->Append("LM317 (+)");
    chTypLM->Append("LM337 (-)");
    chTypLM->SetSelection(0);
    PomocGUI::Dodaj(chTypLM, "kalk.lm_typ");
    g->Add(chTypLM, 0);
    poleLM.push_back(Pole(p, g, TR("Napięcie wejściowe |Uwe| [V]:"), "18", "kalk.lm_uwe"));
    poleLM.push_back(Pole(p, g, TR("Prąd obciążenia [mA]:"), "500", "kalk.lm_i"));
    poleLM.push_back(Pole(p, g, TR("R1 (OUT - ADJ):"), "240", "kalk.lm_r1"));
    poleLM.push_back(Pole(p, g, TR("Vref [V]:"), "1,25", "kalk.lm_vref"));
    poleLM.push_back(Pole(p, g, TR("Prąd ADJ [µA]:"), "50", "kalk.lm_iadj"));
    poleLM.push_back(Pole(p, g, TR("R2 (ADJ - masa):"), "2k2", "kalk.lm_r2"));
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    lbLM = Wynik(p, s, true);
    s->Add(new wxStaticLine(p), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    Opis(p, s, TR("Odwrotnie: R2 dla zadanego napięcia wyjściowego (R1, Vref i Iadj z pól wyżej):"));
    wxFlexGridSizer* g2 = new wxFlexGridSizer(2, 6, 8);
    poleLM.push_back(Pole(p, g2, TR("Potrzebne |Uwy| [V]:"), "12", "kalk.lm_cel"));
    g2->Add(new wxStaticText(p, wxID_ANY, TR("Szereg:")), 0, wxALIGN_CENTER_VERTICAL);
    chSzeregLM = WyborSzeregu(p, Szereg::E24);
    PomocGUI::Dodaj(chSzeregLM, "kalk.szereg");
    g2->Add(chSzeregLM, 0);
    s->Add(g2, 0, wxLEFT | wxRIGHT, 10);
    lbLM2 = Wynik(p, s, false);
    PanelSchematu* ps = PanelSchematu::Obok(p, s, PanelSchematu::Rodzaj::LM317);
    for (wxTextCtrl* t : poleLM) t->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { PrzeliczLM(); });
    chSzeregLM->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { PrzeliczLM(); });
    chTypLM->Bind(wxEVT_CHOICE, [this, ps](wxCommandEvent&) {
        ps->UstawRodzaj(chTypLM->GetSelection() == 1 ? PanelSchematu::Rodzaj::LM337 : PanelSchematu::Rodzaj::LM317);
        PrzeliczLM();
    });
    return p;
}

void DialogKalkulatorow::PrzeliczLM() {
    using namespace StabilizatorRegulowany;
    const Typ typ = chTypLM->GetSelection() == 1 ? Typ::LM337 : Typ::LM317;
    const auto uwe = Napiecie(poleLM[0]), iobc = Napiecie(poleLM[1]), r1 = Opor(poleLM[2]), vref = Napiecie(poleLM[3]),
               iadj = Napiecie(poleLM[4]), r2 = OpcjaOpor(poleLM[5]), cel = Napiecie(poleLM[6]);
    const wxString znak = typ == Typ::LM337 ? "-" : "";
    if (!uwe || !iobc || !r1 || !vref || !iadj || !r2 || !(*r1 > 0) || !(*vref > 0)) {
        lbLM->SetLabel(TR("Wpisz wszystkie wartości (R2 może być puste = 0)."));
        lbLM2->SetLabel(wxEmptyString);
        return;
    }
    Parametry par = typowe(typ);
    par.vRef = *vref;
    par.iAdjA = *iadj * 1e-6;
    const double u = std::fabs(napiecieWyjsciowe(typ, *r1, *r2, par));
    const Zakres z = rozrzutNapiecia(*r1, *r2, par);
    const Moc m = bilans(std::fabs(*uwe), u, *iobc * 1e-3, *r1, par);
    wxString t = TR("Uwy = ") + znak + Liczba(u, 2) + " V   (" + TR("z rozrzutu Vref i Iadj: ") + znak + Liczba(z.min, 2) + " ... " + znak + Liczba(z.maks, 2) + " V)\n" +
                 TR("Prąd dzielnika Vref/R1: ") + Liczba(m.pradDzielnikaA * 1000.0, 2) + " mA,  " + TR("moc strat w stabilizatorze: ") + Liczba(m.stratyW, 2) + " W\n" +
                 TR("Napięcie wejściowe musi być co najmniej ") + znak + Liczba(m.wejscieMinV, 1) + " V";
    if (std::fabs(*uwe) < m.wejscieMinV) t += "  (" + TR("za małe: różnica we/wy poniżej 3 V!") + ")";
    if (!m.pradMinimalnyOk) t += "\n" + TR("Uwaga: prąd obciążenia z dzielnikiem jest mniejszy niż minimalny wymagany ") + Liczba(par.iMinA * 1000.0, 1) + " mA.";
    lbLM->SetLabel(t);
    if (!cel) { lbLM2->SetLabel(TR("Wpisz napięcie wyjściowe (np. 12).")); return; }
    if (auto r2c = r2DlaNapiecia(*cel, *r1, par)) {
        const double r2s = Kalkulatory::najblizsza(*r2c, Wybrany(chSzeregLM));
        const double us = std::fabs(napiecieWyjsciowe(typ, *r1, r2s, par));
        lbLM2->SetLabel("R2 = " + Op(*r2c) + "  (" + TR("z szeregu: ") + Op(r2s) + TR(" daje ") + znak + Liczba(us, 2) + " V)");
    } else {
        lbLM2->SetLabel(TR("Napięcie wyjściowe musi być większe niż Vref."));
    }
}

// ---------------------------------------------------------------- cewki powietrzne
wxPanel* DialogKalkulatorow::StronaCewki(bool wielo) {
    wxPanel* p = new wxPanel(ksiazka);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    Opis(p, s, wielo
        ? TR("Cewka powietrzna wielowarstwowa (warstwa na warstwie). Indukcyjność to suma indukcyjności własnych i wzajemnych wszystkich zwojów (całki eliptyczne); wzór Wheelera jest podany tylko do porównania. Wymiary w milimetrach.")
        : TR("Cewka powietrzna jednowarstwowa (solenoid). Indukcyjność to suma indukcyjności własnych i wzajemnych wszystkich zwojów (całki eliptyczne); wzór Wheelera jest podany tylko do porównania. Wymiary w milimetrach."));
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    auto& v = wielo ? poleC2 : poleC1;
    const char* kl[] = {wielo ? "kalk.cw_dk" : "kalk.c_dk", wielo ? "kalk.cw_d" : "kalk.c_d", wielo ? "kalk.cw_dz" : "kalk.c_dz", wielo ? "kalk.cw_skok" : "kalk.c_skok",
                        wielo ? "kalk.cw_n" : "kalk.c_n", wielo ? "kalk.cw_warstwy" : "kalk.c_cel", "kalk.cw_cel"};
    v.push_back(Pole(p, g, TR("Średnica karkasu D [mm]:"), wielo ? "10" : "10", kl[0]));
    v.push_back(Pole(p, g, TR("Średnica drutu bez izolacji [mm]:"), "0,5", kl[1]));
    v.push_back(Pole(p, g, TR("Z izolacją [mm] (opcja):"), "", kl[2]));
    v.push_back(Pole(p, g, TR("Skok zwojów [mm] (opcja):"), "", kl[3]));
    if (wielo) {
        v.push_back(Pole(p, g, TR("Zwojów w warstwie:"), "40", kl[4]));
        v.push_back(Pole(p, g, TR("Liczba warstw:"), "6", kl[5]));
        v.push_back(Pole(p, g, TR("Docelowa indukcyjność L [µH] (opcja):"), "", kl[6]));
    } else {
        v.push_back(Pole(p, g, TR("Liczba zwojów N:"), "50", kl[4]));
        v.push_back(Pole(p, g, TR("Docelowa indukcyjność L [µH] (opcja):"), "", kl[5]));
    }
    s->Add(g, 0, wxLEFT | wxRIGHT, 10);
    (wielo ? lbC2 : lbC1) = Wynik(p, s, true);
    s->Add(new wxStaticLine(p), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    (wielo ? lbC2b : lbC1b) = Wynik(p, s, false);
    PanelSchematu::Obok(p, s, wielo ? PanelSchematu::Rodzaj::CewkaWiele : PanelSchematu::Rodzaj::CewkaJedno);
    for (wxTextCtrl* t : v) t->Bind(wxEVT_TEXT, [this, wielo](wxCommandEvent&) { PrzeliczCewke(wielo); });
    return p;
}

void DialogKalkulatorow::PrzeliczCewke(bool wielo) {
    auto& v = wielo ? poleC2 : poleC1;
    wxStaticText* lb = wielo ? lbC2 : lbC1;
    wxStaticText* lb2 = wielo ? lbC2b : lbC1b;
    const auto dk = Napiecie(v[0]), d = Napiecie(v[1]), dz = OpcjaLiczba(v[2]), skok = OpcjaLiczba(v[3]), n = Napiecie(v[4]);
    std::optional<double> w = wielo ? Napiecie(v[5]) : std::optional<double>(1.0);
    std::optional<double> cel = OpcjaLiczba(v[wielo ? 6 : 5]);
    lb2->SetLabel(wxEmptyString);
    if (!dk || !d || !dz || !skok || !n || !w || !cel) { lb->SetLabel(TR("Wpisz poprawne liczby (puste pola opcjonalne = domyślne).")); return; }
    CewkaPowietrzna::Parametry p;
    p.srednicaKarkasuMm = *dk; p.drutMm = *d; p.drutZIzolacjaMm = *dz; p.skokMm = *skok;
    p.zwojeNaWarstwe = static_cast<int>(std::lround(*n)); p.warstwy = static_cast<int>(std::lround(*w));
    const auto wy = CewkaPowietrzna::oblicz(p);
    if (!wy) { lb->SetLabel(TR("Dane muszą być dodatnie, a skok i średnica z izolacją nie mniejsze od średnicy drutu.")); return; }
    const wxString uH = wxString::FromUTF8(" \xC2\xB5H");
    wxString t = "L = " + Liczba(wy->indukcyjnoscUH, wy->indukcyjnoscUH < 100 ? 3 : 1) + uH;
    if (!wy->dokladny) t += "  (" + TR("bardzo duża cewka: użyto wzoru Wheelera") + ")";
    else {
        const double roznica = wy->wheelerUH > 0 ? (wy->wheelerUH / wy->indukcyjnoscUH - 1.0) * 100.0 : 0.0;
        t += "\n" + TR("Wheeler (porównanie): ") + Liczba(wy->wheelerUH, wy->wheelerUH < 100 ? 3 : 1) + uH + "  (" + (roznica > 0 ? "+" : "") + Liczba(roznica, 1) + " %)";
    }
    t += "\n" + TR("Uzwojenie: długość ") + Liczba(wy->dlugoscUzwojeniaMm, 1) + " mm" + (wielo ? ", " + TR("grubość ") + Liczba(wy->gruboscUzwojeniaMm, 2) + " mm" : wxString()) +
         ",  " + Liczba(wy->zwojeRazem, 0) + TR(" zwojów") + "\n" +
         TR("Drut: ") + Liczba(wy->dlugoscDrutuM, 2) + " m,  R20 = " + Liczba(wy->opor20Ohm, 3) + wxString::FromUTF8(" \xCE\xA9,  ") + TR("masa Cu ") + Liczba(wy->masaMiedziG, 1) + " g";
    lb->SetLabel(t);
    if (*cel > 0) {
        const auto dob = wielo ? CewkaPowietrzna::warstwyDlaIndukcyjnosci(p, *cel) : CewkaPowietrzna::zwojeDlaIndukcyjnosci(p, *cel);
        if (dob) {
            lb2->SetLabel(TR("Dla L >= ") + Liczba(*cel, 2) + uH + ": " + (wielo ? TR("warstw: ") : TR("zwojów: ")) + Liczba(dob->zwoje, 0) + "  (L = " + Liczba(dob->indukcyjnoscUH, 2) + uH +
                          (dob->zwoje > 1 ? TR(", o jeden mniej: ") + Liczba(dob->poprzedniaUH, 2) + uH : wxString()) + ")");
        } else {
            lb2->SetLabel(TR("Nie da się osiągnąć tej indukcyjności w rozsądnych granicach (do 20000 zwojów / 100 warstw)."));
        }
    }
}

// ---------------------------------------------------------------- dostęp dla testów
void DialogKalkulatorow::UstawPola(int strona, std::initializer_list<wxString> wartosci) {
    std::vector<wxTextCtrl*>* pola = strona == 3 ? &poleZas : strona == 6 ? &poleLM : strona == 7 ? &poleC1 : strona == 8 ? &poleC2 : &poleTR;
    size_t i = 0;
    for (const wxString& w : wartosci) { if (i < pola->size()) (*pola)[i]->ChangeValue(w); ++i; }
    if (strona == 3) PrzeliczZasilacz();
    else if (strona == 6) PrzeliczLM();
    else if (strona == 7) PrzeliczCewke(false);
    else if (strona == 8) PrzeliczCewke(true);
    else PrzeliczTranzystor();
}

wxString DialogKalkulatorow::WynikStrony(int strona, int nr) const {
    if (strona == 3) return (nr == 0 ? lbZas : lbZas2)->GetLabel();
    if (strona == 5) return lbESR->GetLabel();
    if (strona == 6) return (nr == 0 ? lbLM : lbLM2)->GetLabel();
    if (strona == 7) return (nr == 0 ? lbC1 : lbC1b)->GetLabel();
    if (strona == 8) return (nr == 0 ? lbC2 : lbC2b)->GetLabel();
    return (nr == 0 ? lbTR : lbTR2)->GetLabel();
}
