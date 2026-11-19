#include "PoleDaty.h"
#include "PanelNarzedzi.h"

#include "DialogPozyczki.h"
#include "Ikony.h"
#include "ZdjeciaElementow.h"
#include "UstawieniaOkna.h"
#include "../Logika/NarzedziaCSV.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <wx/combobox.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/wrapsizer.h>

namespace {

bool ZapiszTekst(const wxString& sciezka, const std::string& tekst) {
    wxLogNull cisza;
    wxFFile f(sciezka, "wb");
    return f.IsOpened() && f.Write(tekst.data(), tekst.size()) == tekst.size();
}

bool WczytajTekst(const wxString& sciezka, std::string& tekst) {
    wxLogNull cisza;
    wxFFile f(sciezka, "rb");
    if (!f.IsOpened()) return false;
    tekst.assign(static_cast<size_t>(f.Length()), '\0');
    return f.Read(&tekst[0], tekst.size()) == tekst.size();
}

wxString Przytnij(const wxString& s) { return s.Strip(wxString::both); }

std::string Iso(const wxDateTime& d) { return d.IsValid() ? d.FormatISODate().utf8_string() : std::string(); }

class DialogEdycjiNarzedzia : public wxDialog {
public:
    DialogEdycjiNarzedzia(wxWindow* parent, const Narzedzie& n, const std::vector<std::string>& kategorie)
        : wxDialog(parent, wxID_ANY, n.id ? TR("Zmień narzędzie") : TR("Nowe narzędzie"), wxDefaultPosition, wxSize(640, 600),
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
          narzedzie_(n) {
        wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
        wxFlexGridSizer* s = new wxFlexGridSizer(2, 6, 8);
        s->AddGrowableCol(1);
        auto dodaj = [&](const wxString& et, auto* p) {
            s->Add(new wxStaticText(this, wxID_ANY, et), 0, wxALIGN_CENTER_VERTICAL);
            s->Add(p, 1, wxEXPAND);
        };
        tcNazwa_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(n.nazwa));
        dodaj(TR("Nazwa *"), tcNazwa_);
        tcProducent_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(n.producent));
        dodaj(TR("Producent"), tcProducent_);
        tcModel_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(n.model));
        dodaj(TR("Model"), tcModel_);
        cbKategoria_ = new wxComboBox(this, wxID_ANY, TekstZUTF8(n.kategoria));
        for (const std::string& k : kategorie) cbKategoria_->Append(TekstZUTF8(k));
        dodaj(TR("Kategoria"), cbKategoria_);
        tcSeryjny_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(n.numerSeryjny));
        dodaj(TR("Numer seryjny"), tcSeryjny_);
        tcInwentarzowy_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(n.numerInwentarzowy));
        dodaj(TR("Numer inwentarzowy"), tcInwentarzowy_);
        tcMiejsce_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(n.miejsce));
        tcMiejsce_->SetHint(TR("np. warsztat, szuflada 3"));
        dodaj(TR("Miejsce"), tcMiejsce_);
        wxBoxSizer* zakup = new wxBoxSizer(wxHORIZONTAL);
        dpZakup_ = NowePoleDaty(this, wxDefaultDateTime, wxDP_DROPDOWN | wxDP_ALLOWNONE);
        wxDateTime d;
        if (!n.dataZakupu.empty() && d.ParseISODate(TekstZUTF8(n.dataZakupu))) dpZakup_->SetValue(d);
        else dpZakup_->SetValue(wxDateTime());
        zakup->Add(dpZakup_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
        zakup->Add(new wxStaticText(this, wxID_ANY, TR("Cena [zł]:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        tcCena_ = new wxTextCtrl(this, wxID_ANY, n.cena ? TekstZUTF8(liczbaNaTekstPolski(*n.cena, 2, 2)) : wxString(), wxDefaultPosition, wxSize(100, -1));
        zakup->Add(tcCena_, 0, wxALIGN_CENTER_VERTICAL);
        dodaj(TR("Data zakupu"), zakup);
        wxBoxSizer* instr = new wxBoxSizer(wxHORIZONTAL);
        tcInstrukcja_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(n.instrukcja));
        tcInstrukcja_->SetHint(TR("plik PDF lub adres strony"));
        instr->Add(tcInstrukcja_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        wxButton* wybierz = new wxButton(this, wxID_ANY, TR("Wybierz..."));
        instr->Add(wybierz, 0, wxALIGN_CENTER_VERTICAL);
        dodaj(TR("Instrukcja"), instr);
        tcZdjecie_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(n.zdjecie));
        tcZdjecie_->SetHint(TR("plik obrazu"));
        dodaj(TR("Zdjęcie"), ZdjeciaElementow::WierszWyboru(this, tcZdjecie_, [this] { return tcNazwa_->GetValue(); }));
        g->Add(s, 0, wxEXPAND | wxALL, 10);
        g->Add(new wxStaticText(this, wxID_ANY, TR("Uwagi:")), 0, wxLEFT | wxRIGHT, 10);
        tcUwagi_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(n.uwagi), wxDefaultPosition, wxSize(-1, 90), wxTE_MULTILINE);
        g->Add(tcUwagi_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        g->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
        SetSizer(g);
        PomocGUI::Dodaj(tcSeryjny_, "narzedzie.seryjny");
        PomocGUI::Dodaj(tcInwentarzowy_, "narzedzie.inwentarzowy");
        PomocGUI::Dodaj(tcInstrukcja_, "narzedzie.instrukcja");
        PomocGUI::Dodaj(tcZdjecie_, "narzedzie.zdjecie");
        wybierz->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            wxFileDialog dlg(this, TR("Instrukcja narzędzia"), wxEmptyString, wxEmptyString, TR("Dokumenty (*.pdf)|*.pdf|Wszystkie pliki|*.*"),
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            if (dlg.ShowModal() == wxID_OK) tcInstrukcja_->SetValue(dlg.GetPath());
        });
        Bind(wxEVT_BUTTON, &DialogEdycjiNarzedzia::NaOK, this, wxID_OK);
    }
    const Narzedzie& Wynik() const { return narzedzie_; }

private:
    Narzedzie narzedzie_;
    wxTextCtrl *tcNazwa_, *tcProducent_, *tcModel_, *tcSeryjny_, *tcInwentarzowy_, *tcMiejsce_, *tcCena_, *tcInstrukcja_, *tcZdjecie_, *tcUwagi_;
    wxComboBox* cbKategoria_;
    PoleDatyTyp* dpZakup_;

    static std::string T(wxTextCtrl* c) { return Przytnij(c->GetValue()).utf8_string(); }
    void NaOK(wxCommandEvent& e) {
        if (T(tcNazwa_).empty()) {
            wxMessageBox(TR("Podaj nazwę narzędzia."), TR("Narzędzia"), wxOK | wxICON_WARNING, this);
            return;
        }
        std::optional<double> cena;
        if (!T(tcCena_).empty()) {
            double v = 0;
            if (!tekstNaLiczbe(T(tcCena_), v) || v < 0) {
                wxMessageBox(TR("Cena musi być liczbą nieujemną (np. 249,90)."), TR("Narzędzia"), wxOK | wxICON_WARNING, this);
                return;
            }
            cena = v;
        }
        narzedzie_.nazwa = T(tcNazwa_);
        narzedzie_.producent = T(tcProducent_);
        narzedzie_.model = T(tcModel_);
        narzedzie_.kategoria = Przytnij(cbKategoria_->GetValue()).utf8_string();
        narzedzie_.numerSeryjny = T(tcSeryjny_);
        narzedzie_.numerInwentarzowy = T(tcInwentarzowy_);
        narzedzie_.miejsce = T(tcMiejsce_);
        narzedzie_.dataZakupu = Iso(dpZakup_->GetValue());
        narzedzie_.cena = cena;
        narzedzie_.instrukcja = T(tcInstrukcja_);
        narzedzie_.zdjecie = T(tcZdjecie_);
        narzedzie_.uwagi = T(tcUwagi_);
        e.Skip();
    }
};

}  // namespace

PanelNarzedzi::PanelNarzedzi(wxWindow* parent, LogikaNarzedzi& logika) : wxPanel(parent), logika_(logika) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxWrapSizer* gora = new wxWrapSizer(wxHORIZONTAL);
    szukaj_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(280, -1));
    szukaj_->SetDescriptiveText(TR("Szukaj narzędzia lub osoby..."));
    PomocGUI::Dodaj(szukaj_, "narzedzia.szukaj");
    gora->Add(szukaj_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    cbPozyczone_ = new wxCheckBox(this, wxID_ANY, TR("tylko pożyczone"));
    PomocGUI::Dodaj(cbPozyczone_, "narzedzia.tylko_pozyczone");
    gora->Add(cbPozyczone_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    cbPrzeterminowane_ = new wxCheckBox(this, wxID_ANY, TR("tylko po terminie"));
    gora->Add(cbPrzeterminowane_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    licznik_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    gora->Add(licznik_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 4);

    wxSplitterWindow* podzial = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);
    widok_ = new wxListCtrl(podzial, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    const struct { const char* t; int s; } kol[] = {{"Narzędzie", 190}, {"Model", 110}, {"Kategoria", 90}, {"Miejsce", 100}, {"Status", 300}};
    for (int i = 0; i < K_LICZBA; ++i) widok_->InsertColumn(i, TR(kol[i].t), wxLIST_FORMAT_LEFT, kol[i].s);
    PomocGUI::Dodaj(widok_, "narzedzia.lista");

    wxPanel* prawy = new wxPanel(podzial);
    wxBoxSizer* ps = new wxBoxSizer(wxVERTICAL);
    naglowek_ = new wxStaticText(prawy, wxID_ANY, TR("Wybierz narzędzie z listy"));
    wxFont f = naglowek_->GetFont(); f.MakeBold(); f.SetPointSize(f.GetPointSize() + 2);
    naglowek_->SetFont(f);
    ps->Add(naglowek_, 0, wxALL, 8);
    zdjecie_ = new wxStaticBitmap(prawy, wxID_ANY, wxNullBitmap);
    zdjecie_->Hide();
    ps->Add(zdjecie_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    szczegoly_ = new wxStaticText(prawy, wxID_ANY, wxEmptyString);
    ps->Add(szczegoly_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    ps->Add(new wxStaticText(prawy, wxID_ANY, TR("Historia pożyczek:")), 0, wxLEFT | wxRIGHT, 8);
    historiaWidok_ = new wxListCtrl(prawy, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_THEME);
    historiaWidok_->InsertColumn(0, TR("Komu"), wxLIST_FORMAT_LEFT, 130);
    historiaWidok_->InsertColumn(1, TR("Od"), wxLIST_FORMAT_LEFT, 85);
    historiaWidok_->InsertColumn(2, TR("Termin"), wxLIST_FORMAT_LEFT, 85);
    historiaWidok_->InsertColumn(3, TR("Zwrot"), wxLIST_FORMAT_LEFT, 85);
    historiaWidok_->InsertColumn(4, TR("Uwagi"), wxLIST_FORMAT_LEFT, 150);
    PomocGUI::Dodaj(historiaWidok_, "narzedzia.historia");
    ps->Add(historiaWidok_, 1, wxEXPAND | wxALL, 8);
    UstawieniaOkna::WczytajKolumny(widok_, "Narzedzia");
    UstawieniaOkna::WczytajKolumny(historiaWidok_, "NarzedziaHistoria");
    Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& e) {
        if (e.GetEventObject() == this) {
            UstawieniaOkna::ZapiszKolumny(widok_, "Narzedzia");
            UstawieniaOkna::ZapiszKolumny(historiaWidok_, "NarzedziaHistoria");
        }
        e.Skip();
    });
    prawy->SetSizer(ps);
    podzial->SplitVertically(widok_, prawy, -430);
    podzial->SetMinimumPaneSize(200);
    podzial->SetSashGravity(1.0);
    glowny->Add(podzial, 1, wxEXPAND | wxLEFT | wxRIGHT, 6);

    wxWrapSizer* przyciski = new wxWrapSizer(wxHORIZONTAL);
    auto przycisk = [&](int id, const wxString& t) {
        wxButton* b = new wxButton(this, id, t);
        przyciski->Add(b, 0, wxALL, 3);
        return b;
    };
    Ikony::DlaPrzycisku(przycisk(ID_N_NOWE, TR("Nowe...")), "nowy");
    wxButton* bEdytuj = przycisk(ID_N_EDYTUJ, TR("Edytuj..."));
    Ikony::DlaPrzycisku(bEdytuj, "edytuj");
    wxButton* bUsun = przycisk(ID_N_USUN, TR("Usuń"));
    Ikony::DlaPrzycisku(bUsun, "usun");
    wxButton* bPozycz = przycisk(ID_N_POZYCZ, TR("Pożycz..."));
    PomocGUI::Dodaj(bPozycz, "narzedzia.pozycz");
    wxButton* bZwroc = przycisk(ID_N_ZWROC, TR("Zwrócono"));
    PomocGUI::Dodaj(bZwroc, "narzedzia.zwroc");
    wxButton* bInstr = przycisk(ID_N_INSTRUKCJA, TR("Otwórz instrukcję"));
    przycisk(ID_N_IMPORT, TR("Import CSV..."));
    przycisk(ID_N_EKSPORT, TR("Eksport CSV..."));
    przycisk(ID_N_SZABLON, TR("Szablon CSV..."));
    glowny->Add(przyciski, 0, wxEXPAND | wxALL, 4);
    SetSizer(glowny);

    for (wxButton* b : {bEdytuj, bUsun})
        b->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(Wybrane() != nullptr); });
    bPozycz->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(Wybrane() && !Wybrane()->pozyczone); });
    bZwroc->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(Wybrane() && Wybrane()->pozyczone); });
    bInstr->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(Wybrane() && !Wybrane()->instrukcja.empty()); });

    auto odswiez = [this](wxCommandEvent&) { if (!odswiezanie_) Odswiez(); };
    szukaj_->Bind(wxEVT_TEXT, odswiez);
    cbPozyczone_->Bind(wxEVT_CHECKBOX, odswiez);
    cbPrzeterminowane_->Bind(wxEVT_CHECKBOX, odswiez);
    widok_->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent&) { PokazSzczegoly(); });
    widok_->Bind(wxEVT_LIST_ITEM_DESELECTED, [this](wxListEvent&) { PokazSzczegoly(); });
    widok_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) { Edytuj(); });
    widok_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, [this](wxListEvent& e) { MenuKontekstowe(e.GetIndex()); });
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Nowe(); }, ID_N_NOWE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Edytuj(); }, ID_N_EDYTUJ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Usun(); }, ID_N_USUN);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Pozycz(); }, ID_N_POZYCZ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Zwroc(); }, ID_N_ZWROC);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OtworzInstrukcje(); }, ID_N_INSTRUKCJA);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ImportujCSV(); }, ID_N_IMPORT);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EksportujCSV(); }, ID_N_EKSPORT);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ZapiszSzablon(); }, ID_N_SZABLON);
}

void PanelNarzedzi::Blad(const wxString& t) {
    Komunikat(TR("✗ ") + t);
    wxMessageBox(t, TR("Narzędzia"), wxOK | wxICON_WARNING, this);
}

const Narzedzie* PanelNarzedzi::Wybrane() const {
    const long i = widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (i < 0 || static_cast<size_t>(i) >= lista_.size()) return nullptr;
    return &lista_[static_cast<size_t>(i)];
}

void PanelNarzedzi::Wybierz(long long id) {
    for (size_t i = 0; i < lista_.size(); ++i)
        if (lista_[i].id == id) {
            widok_->SetItemState(static_cast<long>(i), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            widok_->EnsureVisible(static_cast<long>(i));
            return;
        }
}

void PanelNarzedzi::Szukaj(const std::string& tekst) { szukaj_->ChangeValue(TekstZUTF8(tekst)); Odswiez(); }

void PanelNarzedzi::UstawFiltr(bool tylkoPozyczone, bool tylkoPrzeterminowane) {
    cbPozyczone_->SetValue(tylkoPozyczone);
    cbPrzeterminowane_->SetValue(tylkoPrzeterminowane);
    Odswiez();
}

void PanelNarzedzi::Wyczysc() {
    lista_.clear();
    historia_.clear();
    widok_->DeleteAllItems();
    historiaWidok_->DeleteAllItems();
    PokazSzczegoly();
    licznik_->SetLabel(wxEmptyString);
}

void PanelNarzedzi::Odswiez() {
    odswiezanie_ = true;
    const long long zachowaj = Wybrane() ? Wybrane()->id : 0;
    widok_->DeleteAllItems();
    FiltrNarzedzi f;
    f.tekst = Przytnij(szukaj_->GetValue()).utf8_string();
    f.tylkoPozyczone = cbPozyczone_->GetValue();
    f.tylkoPrzeterminowane = cbPrzeterminowane_->GetValue();
    if (!logika_.pobierz(f, lista_)) {
        Status(TR("✗ Narzędzia: ") + TekstZUTF8(logika_.pobierzBlad()));
        lista_.clear();
    }
    for (size_t i = 0; i < lista_.size(); ++i) {
        const Narzedzie& n = lista_[i];
        const long w = widok_->InsertItem(static_cast<long>(i), TekstZUTF8(n.nazwa));
        widok_->SetItem(w, K_MODEL, TekstZUTF8(n.producent.empty() ? n.model : (n.model.empty() ? n.producent : n.producent + " " + n.model)));
        widok_->SetItem(w, K_KATEGORIA, TekstZUTF8(n.kategoria));
        widok_->SetItem(w, K_MIEJSCE, TekstZUTF8(n.miejsce));
        wxString status = TR("u mnie");
        if (n.pozyczone) {
            status = (n.przeterminowane ? TR("PO TERMINIE: ") : TR("pożyczone: ")) + TekstZUTF8(n.pozyczonePrzez) +
                     (n.przeterminowane ? TR(" (do ") + TekstZUTF8(n.termin) : TR(" (od ") + TekstZUTF8(n.pozyczoneOd)) + ")";
        }
        widok_->SetItem(w, K_STATUS, status);
        if (n.przeterminowane) widok_->SetItemTextColour(w, wxColour(200, 60, 0));
        else if (n.pozyczone) widok_->SetItemTextColour(w, wxColour(30, 100, 190));
    }
    licznik_->SetLabel(wxString::Format(TR("Narzędzi: %zu"), lista_.size()));
    odswiezanie_ = false;
    if (zachowaj) Wybierz(zachowaj);
    PokazSzczegoly();
}

void PanelNarzedzi::PokazZdjecie(const Narzedzie* n) {
    wxImage obraz;
    if (n && !n->zdjecie.empty()) obraz = ZdjeciaElementow::Wczytaj(TekstZUTF8(n->zdjecie), wxSize(260, 170));
    zdjeciePokazane_ = obraz.IsOk();
    if (zdjeciePokazane_) zdjecie_->SetBitmap(wxBitmap(obraz));
    zdjecie_->Show(zdjeciePokazane_);
    zdjecie_->GetParent()->Layout();
}

void PanelNarzedzi::PokazSzczegoly() {
    historiaWidok_->DeleteAllItems();
    historia_.clear();
    const Narzedzie* n = Wybrane();
    PokazZdjecie(n);
    if (!n) {
        naglowek_->SetLabel(TR("Wybierz narzędzie z listy"));
        szczegoly_->SetLabel(wxEmptyString);
        szczegoly_->GetParent()->Layout();
        return;
    }
    naglowek_->SetLabel(TekstZUTF8(n->nazwa));
    wxString t;
    auto dodaj = [&](const wxString& et, const std::string& v) { if (!v.empty()) t += et + ": " + TekstZUTF8(v) + "\n"; };
    dodaj(TR("Producent"), n->producent);
    dodaj(TR("Model"), n->model);
    dodaj(TR("Kategoria"), n->kategoria);
    dodaj(TR("Numer seryjny"), n->numerSeryjny);
    dodaj(TR("Numer inwentarzowy"), n->numerInwentarzowy);
    dodaj(TR("Miejsce"), n->miejsce);
    dodaj(TR("Kupiono"), n->dataZakupu);
    if (n->cena) t += TR("Cena: ") + TekstZUTF8(liczbaNaTekstPolski(*n->cena, 2, 2)) + wxString::FromUTF8(" zł\n");
    dodaj(TR("Instrukcja"), n->instrukcja);
    dodaj(TR("Uwagi"), n->uwagi);
    if (n->pozyczone) {
        t += TR("POŻYCZONE: ") + TekstZUTF8(n->pozyczonePrzez) + TR(", od ") + TekstZUTF8(n->pozyczoneOd);
        if (!n->termin.empty()) t += TR(", termin zwrotu ") + TekstZUTF8(n->termin) + (n->przeterminowane ? TR(" (minął!)") : wxString());
        t += "\n";
    }
    szczegoly_->SetLabel(t);
    szczegoly_->Wrap(std::max(200, szczegoly_->GetParent()->GetClientSize().GetWidth() - 20));
    szczegoly_->GetParent()->Layout();
    if (logika_.historia(n->id, historia_)) {
        for (size_t i = 0; i < historia_.size(); ++i) {
            const Pozyczka& p = historia_[i];
            const long w = historiaWidok_->InsertItem(static_cast<long>(i), TekstZUTF8(p.osoba));
            historiaWidok_->SetItem(w, 1, TekstZUTF8(p.od));
            historiaWidok_->SetItem(w, 2, TekstZUTF8(p.termin));
            historiaWidok_->SetItem(w, 3, p.zwrot.empty() ? TR("nie zwrócono") : TekstZUTF8(p.zwrot));
            historiaWidok_->SetItem(w, 4, TekstZUTF8(p.uwagi));
            if (p.przeterminowana) historiaWidok_->SetItemTextColour(w, wxColour(200, 60, 0));
        }
    }
    Layout();
}

void PanelNarzedzi::Nowe() {
    DialogEdycjiNarzedzia dlg(this, Narzedzie(), logika_.kategorie());
    if (dlg.ShowModal() != wxID_OK) return;
    auto id = logika_.zapisz(dlg.Wynik());
    if (!id) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    Komunikat(TR("✓ Dodano narzędzie: ") + TekstZUTF8(dlg.Wynik().nazwa));
    Odswiez();
    Wybierz(*id);
}

void PanelNarzedzi::Edytuj() {
    const Narzedzie* w = Wybrane();
    if (!w) return;
    DialogEdycjiNarzedzia dlg(this, *w, logika_.kategorie());
    if (dlg.ShowModal() != wxID_OK) return;
    auto id = logika_.zapisz(dlg.Wynik());
    if (!id) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    Odswiez();
    Wybierz(*id);
}

void PanelNarzedzi::Usun() {
    const Narzedzie* w = Wybrane();
    if (!w) return;
    wxString pytanie = wxString::Format(TR("Usunąć narzędzie \"%s\" razem z historią pożyczek?"), TekstZUTF8(w->nazwa));
    if (w->pozyczone) pytanie = wxString::Format(TR("Narzędzie \"%s\" jest pożyczone (%s). Usunąć je razem z historią pożyczek?"), TekstZUTF8(w->nazwa), TekstZUTF8(w->pozyczonePrzez));
    if (wxMessageBox(pytanie, TR("Narzędzia"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return;
    if (!logika_.usun(w->id)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    Odswiez();
}

void PanelNarzedzi::Pozycz() {
    const Narzedzie* w = Wybrane();
    if (!w) return;
    if (w->pozyczone) {
        wxMessageBox(wxString::Format(TR("To narzędzie jest już pożyczone (%s). Najpierw zarejestruj zwrot."), TekstZUTF8(w->pozyczonePrzez)),
                     TR("Narzędzia"), wxOK | wxICON_INFORMATION, this);
        return;
    }
    DialogPozyczki dlg(this, TR("Pożycz narzędzie"), TR("Komu pożyczasz narzędzie?"), TekstZUTF8(w->nazwa), logika_.osoby());
    if (dlg.ShowModal() != wxID_OK) return;
    const long long id = w->id;
    if (!logika_.pozycz(id, dlg.Osoba(), dlg.Od(), dlg.Termin(), dlg.Uwagi())) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    Komunikat(wxString::Format(TR("✓ Pożyczono \"%s\": %s"), TekstZUTF8(w->nazwa), TekstZUTF8(dlg.Osoba())));
    Odswiez();
    Wybierz(id);
}

bool PanelNarzedzi::Zwroc(bool bezPytan) {
    const Narzedzie* w = Wybrane();
    if (!w || !w->pozyczone) return false;
    if (!bezPytan && wxMessageBox(wxString::Format(TR("Zarejestrować zwrot narzędzia \"%s\" od: %s?"), TekstZUTF8(w->nazwa), TekstZUTF8(w->pozyczonePrzez)),
                                  TR("Narzędzia"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return false;
    const long long id = w->id;
    const wxString nazwa = TekstZUTF8(w->nazwa);
    if (!logika_.zwroc(id)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return false; }
    Komunikat(TR("✓ Zwrócono: ") + nazwa);
    Odswiez();
    Wybierz(id);
    return true;
}

int PanelNarzedzi::ImportujCSV(const wxString& sciezka) {
    wxString plik = sciezka;
    if (plik.empty()) {
        wxFileDialog dlg(this, TR("Importuj narzędzia z CSV"), wxEmptyString, wxEmptyString, TR("Pliki CSV (*.csv)|*.csv|Wszystkie pliki|*.*"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) return -1;
        plik = dlg.GetPath();
    }
    std::string tekst;
    if (!WczytajTekst(plik, tekst)) { Blad(TR("Nie można odczytać pliku.")); return -1; }
    const auto w = NarzedziaCSV::parsuj(tekst);
    if (!w.blad.empty()) { Blad(TekstZUTF8(w.blad)); return -1; }
    WynikImportuNarzedzi wynik;
    if (!logika_.importuj(w.narzedzia, wynik)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return -1; }
    Odswiez();
    wxString info = wxString::Format(TR("Dodano narzędzi: %d, pominięto (już są): %d."), wynik.dodano, wynik.pominieto);
    for (const std::string& o : w.ostrzezenia) info += "\n" + TekstZUTF8(o);
    for (const std::string& b : wynik.bledy) info += "\n" + TekstZUTF8(b);
    Komunikat(wxString::FromUTF8("✓ ") + info);
    Status(info);
    return wynik.dodano;
}

bool PanelNarzedzi::EksportujCSV(const wxString& sciezka) {
    wxString plik = sciezka;
    if (plik.empty()) {
        wxFileDialog dlg(this, TR("Eksportuj narzędzia do CSV"), wxEmptyString, "narzedzia.csv", TR("Pliki CSV (*.csv)|*.csv"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        plik = dlg.GetPath();
    }
    std::vector<Narzedzie> wszystkie;
    if (!logika_.pobierz(FiltrNarzedzi(), wszystkie)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return false; }
    if (!ZapiszTekst(plik, NarzedziaCSV::naCSV(wszystkie))) { Blad(TR("Nie można zapisać pliku.")); return false; }
    Komunikat(wxString::Format(TR("✓ Wyeksportowano narzędzi: %zu"), wszystkie.size()));
    return true;
}

bool PanelNarzedzi::ZapiszSzablon(const wxString& sciezka) {
    wxString plik = sciezka;
    if (plik.empty()) {
        wxFileDialog dlg(this, TR("Zapisz szablon CSV"), wxEmptyString, "szablon_narzedzia.csv", TR("Pliki CSV (*.csv)|*.csv"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        plik = dlg.GetPath();
    }
    if (!ZapiszTekst(plik, NarzedziaCSV::szablon())) { Blad(TR("Nie można zapisać pliku.")); return false; }
    return true;
}

void PanelNarzedzi::OtworzInstrukcje() {
    const Narzedzie* w = Wybrane();
    if (!w || w->instrukcja.empty()) return;
    const wxString cel = TekstZUTF8(w->instrukcja);
    const bool adres = cel.StartsWith("http://") || cel.StartsWith("https://");
    if (adres ? !wxLaunchDefaultBrowser(cel) : (!wxFileName::FileExists(cel) || !wxLaunchDefaultApplication(cel)))
        wxMessageBox(TR("Nie można otworzyć instrukcji (sprawdź ścieżkę lub adres)."), TR("Narzędzia"), wxOK | wxICON_WARNING, this);
}

int PanelNarzedzi::Przypomnij() {
    std::vector<Pozyczka> przet;
    if (!logika_.przeterminowane(przet)) return -1;
    for (const Pozyczka& p : przet)
        Komunikat(wxString::Format(TR("⚠ Narzędzie \"%s\" miało wrócić %s od: %s"), TekstZUTF8(p.nazwaNarzedzia), TekstZUTF8(p.termin), TekstZUTF8(p.osoba)));
    return static_cast<int>(przet.size());
}

void PanelNarzedzi::MenuKontekstowe(long element) {
    if (element < 0 || static_cast<size_t>(element) >= lista_.size()) return;
    widok_->SetItemState(element, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    const Narzedzie* n = Wybrane();
    if (!n) return;
    wxMenu menu;
    if (n->pozyczone) menu.Append(ID_N_ZWROC, TR("Zwrócono"));
    else menu.Append(ID_N_POZYCZ, TR("Wypożycz..."));
    menu.AppendSeparator();
    menu.Append(ID_N_KAMERA, TR("Dołącz zdjęcie z kamery..."));
    menu.Append(ID_N_PLIK, TR("Dołącz zdjęcie z pliku..."));
    menu.Append(ID_N_EDYTUJ, TR("Edytuj..."));
    menu.AppendSeparator();
    menu.Append(ID_N_USUN, TR("Usuń"));
    menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
        const int id = e.GetId();
        CallAfter([this, id] {
            switch (id) {
            case ID_N_ZWROC: Zwroc(); break;
            case ID_N_POZYCZ: Pozycz(); break;
            case ID_N_KAMERA: DolaczZdjecieZKamery(); break;
            case ID_N_PLIK: DolaczZdjecieZPliku(); break;
            case ID_N_EDYTUJ: Edytuj(); break;
            case ID_N_USUN: Usun(); break;
            }
        });
    });
    PopupMenu(&menu);
}

bool PanelNarzedzi::DolaczZdjecie(const wxString& sciezka) {
    const Narzedzie* w = Wybrane();
    if (!w || sciezka.empty()) return false;
    Narzedzie n = *w;
    n.zdjecie = sciezka.utf8_string();
    if (!logika_.zapisz(n)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return false; }
    Komunikat(TR("✓ Zapisano zdjęcie narzędzia: ") + TekstZUTF8(n.nazwa));
    Odswiez();
    Wybierz(n.id);
    return true;
}

void PanelNarzedzi::DolaczZdjecieZKamery() {
    const Narzedzie* w = Wybrane();
    if (!w) return;
    const wxString p = ZdjeciaElementow::ZKamery(this, TekstZUTF8(w->nazwa));
    if (!p.empty()) DolaczZdjecie(p);
}

void PanelNarzedzi::DolaczZdjecieZPliku() {
    if (!Wybrane()) return;
    const wxString p = ZdjeciaElementow::ZPliku(this);
    if (!p.empty()) DolaczZdjecie(p);
}
