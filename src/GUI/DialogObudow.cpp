#include "DialogObudow.h"

#include <algorithm>
#include "Ikony.h"
#include "UstawieniaOkna.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <wx/checklst.h>
#include <wx/ffile.h>
#include <wx/spinctrl.h>
#include <wx/filedlg.h>

namespace {

wxString NaTekst(const std::optional<double>& v) { return v ? TekstZUTF8(liczbaNaTekstPolski(*v, 3, 0)) : wxString(); }

wxString Przytnij(const wxString& s) { return s.Strip(wxString::both); }

/// Pole liczby (pusty tekst = brak); false + komunikat przy błędnym zapisie
bool Liczba(wxWindow* okno, const wxString& tekst, const wxString& nazwaPola, std::optional<double>& wynik) {
    const wxString t = Przytnij(tekst);
    wynik.reset();
    if (t.empty()) return true;
    double v = 0;
    if (!tekstNaLiczbe(t.utf8_string(), v) || v < 0) {
        wxMessageBox(wxString::Format(TR("Pole \"%s\" musi być liczbą nieujemną (np. 2,5)."), nazwaPola), TR("Obudowy"),
                     wxOK | wxICON_WARNING, okno);
        return false;
    }
    wynik = v;
    return true;
}

class DialogEdycjiObudowy : public wxDialog {
public:
    DialogEdycjiObudowy(wxWindow* parent, const Obudowa& o)
        : wxDialog(parent, wxID_ANY, o.id ? TR("Zmień obudowę") : TR("Nowa obudowa"), wxDefaultPosition, wxSize(620, 560),
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
          obudowa_(o) {
        wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
        wxFlexGridSizer* siatka = new wxFlexGridSizer(2, 6, 8);
        siatka->AddGrowableCol(1);
        auto dodaj = [&](const wxString& etykieta, wxWindow* pole) {
            siatka->Add(new wxStaticText(this, wxID_ANY, etykieta), 0, wxALIGN_CENTER_VERTICAL);
            siatka->Add(pole, 1, wxEXPAND);
        };
        tcNazwa_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(o.nazwa));
        dodaj(TR("Nazwa (np. R7.5, TO220AH):"), tcNazwa_);
        tcOpis_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(o.opis));
        dodaj(TR("Opis:"), tcOpis_);
        chMontaz_ = new wxChoice(this, wxID_ANY);
        chMontaz_->Append(TR("(nieokreślony)"));
        chMontaz_->Append("THT");
        chMontaz_->Append("SMD");
        chMontaz_->SetSelection(o.montaz == "THT" ? 1 : (o.montaz == "SMD" ? 2 : 0));
        dodaj(TR("Montaż:"), chMontaz_);
        scPiny_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999, o.liczbaPinow);
        dodaj(TR("Liczba wyprowadzeń (0 = nieznana):"), scPiny_);
        tcMoc_ = new wxTextCtrl(this, wxID_ANY, NaTekst(o.mocStratW));
        dodaj(TR("Moc strat [W]:"), tcMoc_);
        tcNapiecie_ = new wxTextCtrl(this, wxID_ANY, NaTekst(o.napiecieMaxV));
        dodaj(TR("Dopuszczalne napięcie [V]:"), tcNapiecie_);
        tcRozstaw_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(o.rozstaw));
        dodaj(TR("Rozstaw wyprowadzeń:"), tcRozstaw_);
        tcRodzina_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(o.rodzina));
        dodaj(TR("Rodzina (np. TO-220):"), tcRodzina_);
        tcWariant_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(o.wariant));
        dodaj(TR("Wariant (np. AH):"), tcWariant_);
        glowny->Add(siatka, 0, wxEXPAND | wxALL, 10);

        glowny->Add(new wxStaticText(this, wxID_ANY, TR("Uwagi:")), 0, wxLEFT | wxRIGHT, 10);
        tcUwagi_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(o.uwagi), wxDefaultPosition, wxSize(-1, 80), wxTE_MULTILINE);
        glowny->Add(tcUwagi_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        glowny->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
        SetSizer(glowny);
        PomocGUI::Dodaj(tcNazwa_, "obudowa.nazwa");
        PomocGUI::Dodaj(tcMoc_, "obudowa.moc");
        PomocGUI::Dodaj(tcNapiecie_, "obudowa.napiecie");
        PomocGUI::Dodaj(tcRodzina_, "obudowa.rodzina");
        PomocGUI::Dodaj(tcWariant_, "obudowa.wariant");
        Bind(wxEVT_BUTTON, &DialogEdycjiObudowy::NaOK, this, wxID_OK);
    }

    const Obudowa& Wynik() const { return obudowa_; }

private:
    Obudowa obudowa_;
    wxTextCtrl *tcNazwa_, *tcOpis_, *tcMoc_, *tcNapiecie_, *tcRozstaw_, *tcRodzina_, *tcWariant_, *tcUwagi_;
    wxChoice* chMontaz_;
    wxSpinCtrl* scPiny_;

    void NaOK(wxCommandEvent& event) {
        if (Przytnij(tcNazwa_->GetValue()).empty()) {
            wxMessageBox(TR("Podaj nazwę obudowy."), TR("Obudowy"), wxOK | wxICON_WARNING, this);
            return;
        }
        std::optional<double> moc, napiecie;
        if (!Liczba(this, tcMoc_->GetValue(), TR("Moc strat"), moc) || !Liczba(this, tcNapiecie_->GetValue(), TR("Dopuszczalne napięcie"), napiecie))
            return;
        obudowa_.nazwa = Przytnij(tcNazwa_->GetValue()).utf8_string();
        obudowa_.opis = Przytnij(tcOpis_->GetValue()).utf8_string();
        obudowa_.montaz = chMontaz_->GetSelection() == 1 ? "THT" : (chMontaz_->GetSelection() == 2 ? "SMD" : "");
        obudowa_.liczbaPinow = scPiny_->GetValue();
        obudowa_.mocStratW = moc;
        obudowa_.napiecieMaxV = napiecie;
        obudowa_.rozstaw = Przytnij(tcRozstaw_->GetValue()).utf8_string();
        obudowa_.uwagi = Przytnij(tcUwagi_->GetValue()).utf8_string();
        obudowa_.rodzina = Przytnij(tcRodzina_->GetValue()).utf8_string();
        obudowa_.wariant = Przytnij(tcWariant_->GetValue()).utf8_string();
        event.Skip();   // zamknij okno z wynikiem OK
    }
};

}  // namespace

DialogObudow::DialogObudow(wxWindow* parent, LogikaObudow& logika)
    : wxDialog(parent, wxID_ANY, TR("Obudowy"), wxDefaultPosition, wxSize(1000, 600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      logika_(logika) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);
    szukaj_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(260, -1));
    szukaj_->ShowCancelButton(true);
    gora->Add(szukaj_, 0, wxRIGHT, 10);
    licznik_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    gora->Add(licznik_, 1, wxALIGN_CENTER_VERTICAL);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 10);

    lista_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxBORDER_THEME);
    lista_->AppendColumn(TR("Obudowa"), wxLIST_FORMAT_LEFT, 130);
    lista_->AppendColumn(TR("Rodzina"), wxLIST_FORMAT_LEFT, 90);
    lista_->AppendColumn(TR("Montaż"), wxLIST_FORMAT_LEFT, 60);
    lista_->AppendColumn(TR("Piny"), wxLIST_FORMAT_RIGHT, 45);
    lista_->AppendColumn(TR("Moc [W]"), wxLIST_FORMAT_RIGHT, 70);
    lista_->AppendColumn(TR("Napięcie [V]"), wxLIST_FORMAT_RIGHT, 90);
    lista_->AppendColumn(TR("Rozstaw"), wxLIST_FORMAT_LEFT, 80);
    lista_->AppendColumn(TR("Elementów"), wxLIST_FORMAT_RIGHT, 80);
    lista_->AppendColumn(TR("Opis"), wxLIST_FORMAT_LEFT, 420);
    glowny->Add(lista_, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
    glowny->Add(new wxStaticText(this, wxID_ANY, TR("Typowe elementy w tej obudowie (z bibliotek Eagle):")), 0, wxLEFT | wxRIGHT | wxTOP, 10);
    typowe_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE | wxTE_READONLY);
    PomocGUI::Dodaj(typowe_, "obudowa.typowe");
    glowny->Add(typowe_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    UstawieniaOkna::WczytajKolumny(lista_, "Obudowy");
    Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& e) {
        if (e.GetEventObject() == this) UstawieniaOkna::ZapiszKolumny(lista_, "Obudowy");
        e.Skip();
    });
    PomocGUI::DymekZKolumn(lista_);

    wxBoxSizer* dol = new wxBoxSizer(wxHORIZONTAL);
    wxButton* bDodaj = new wxButton(this, wxID_ANY, TR("Dodaj..."));
    wxButton* bEdytuj = new wxButton(this, wxID_ANY, TR("Zmień..."));
    wxButton* bUsun = new wxButton(this, wxID_ANY, TR("Usuń"));
    wxButton* bLbr = new wxButton(this, wxID_ANY, TR("Wczytaj bibliotekę Eagle (.lbr)..."));
    wxButton* bElementy = new wxButton(this, wxID_ANY, TR("Uzupełnij z elementów"));
    Ikony::DlaPrzycisku(bDodaj, "nowy");
    Ikony::DlaPrzycisku(bEdytuj, "edytuj");
    Ikony::DlaPrzycisku(bUsun, "usun");
    Ikony::DlaPrzycisku(bLbr, "import");
    for (wxButton* b : {bDodaj, bEdytuj, bUsun, bLbr, bElementy}) dol->Add(b, 0, wxRIGHT, 6);
    dol->AddStretchSpacer();
    dol->Add(new wxButton(this, wxID_CANCEL, TR("Zamknij")));
    glowny->Add(dol, 0, wxEXPAND | wxALL, 10);
    SetSizer(glowny);
    SetEscapeId(wxID_CANCEL);

    bDodaj->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Dodaj(); });
    bEdytuj->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Edytuj(); });
    bUsun->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Usun(); });
    bLbr->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ImportujBiblioteki(); });
    bElementy->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { UzupelnijZElementow(); });
    for (wxButton* b : {bEdytuj, bUsun}) b->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(Wybrana() != nullptr); });
    lista_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) { Edytuj(); });
    lista_->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent& e) { OdswiezTypowe(); e.Skip(); });
    lista_->Bind(wxEVT_LIST_ITEM_DESELECTED, [this](wxListEvent& e) { OdswiezTypowe(); e.Skip(); });
    szukaj_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Odswiez(); });
    Odswiez();
}

const Obudowa* DialogObudow::Wybrana() const {
    const long i = lista_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (i < 0 || static_cast<size_t>(i) >= obudowy_.size()) return nullptr;
    return &obudowy_[static_cast<size_t>(i)];
}

std::vector<const Obudowa*> DialogObudow::Zaznaczone() const {
    std::vector<const Obudowa*> w;
    for (long i = lista_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); i >= 0;
         i = lista_->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
        if (static_cast<size_t>(i) < obudowy_.size()) w.push_back(&obudowy_[static_cast<size_t>(i)]);
    return w;
}

size_t DialogObudow::LiczbaZaznaczonych() const { return Zaznaczone().size(); }

void DialogObudow::Wybierz(const std::vector<long long>& id) {
    for (size_t i = 0; i < obudowy_.size(); ++i) {
        const bool tak = std::find(id.begin(), id.end(), obudowy_[i].id) != id.end();
        lista_->SetItemState(static_cast<long>(i), tak ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED);
    }
    OdswiezTypowe();
}

void DialogObudow::Wybierz(long long id) {
    for (size_t i = 0; i < obudowy_.size(); ++i)
        if (obudowy_[i].id == id) {
            lista_->SetItemState(static_cast<long>(i), wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                                 wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
            lista_->EnsureVisible(static_cast<long>(i));
        }
}

void DialogObudow::OdswiezTypowe() {
    const Obudowa* o = Wybrana();
    wxString t;
    if (o) {
        for (const TypowyElement& e : logika_.typowe(o->id)) t += (t.empty() ? "" : ", ") + TekstZUTF8(e.zestaw);
    }
    typowe_->ChangeValue(t);
}

void DialogObudow::Odswiez() {
    const long long wybrana = Wybrana() ? Wybrana()->id : 0;
    lista_->DeleteAllItems();
    if (!logika_.pobierz(szukaj_->GetValue().utf8_string(), obudowy_)) {
        wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Obudowy"), wxOK | wxICON_ERROR, this);
        return;
    }
    for (size_t i = 0; i < obudowy_.size(); ++i) {
        const Obudowa& o = obudowy_[i];
        const long w = lista_->InsertItem(static_cast<long>(i), TekstZUTF8(o.nazwa));
        lista_->SetItem(w, 1, TekstZUTF8(o.rodzina));
        lista_->SetItem(w, 2, TekstZUTF8(o.montaz));
        lista_->SetItem(w, 3, o.liczbaPinow > 0 ? wxString::Format("%d", o.liczbaPinow) : wxString());
        lista_->SetItem(w, 4, NaTekst(o.mocStratW));
        lista_->SetItem(w, 5, NaTekst(o.napiecieMaxV));
        lista_->SetItem(w, 6, TekstZUTF8(o.rozstaw));
        lista_->SetItem(w, 7, wxString::Format("%d", o.liczbaElementow));
        lista_->SetItem(w, 8, TekstZUTF8(o.opis));
    }
    licznik_->SetLabel(wxString::Format(TR("Obudów: %zu"), obudowy_.size()));
    if (wybrana) Wybierz(wybrana);
    OdswiezTypowe();
}

bool DialogObudow::Edycja(Obudowa obudowa) {
    DialogEdycjiObudowy dlg(this, obudowa);
    if (dlg.ShowModal() != wxID_OK) return false;
    auto id = logika_.zapisz(dlg.Wynik());
    if (!id) {
        wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Obudowy"), wxOK | wxICON_WARNING, this);
        return false;
    }
    Odswiez();
    Wybierz(*id);
    return true;
}

bool DialogObudow::Dodaj() { return Edycja(Obudowa()); }

bool DialogObudow::Edytuj() {
    if (Zaznaczone().size() > 1) return EdytujZbiorczo();
    const Obudowa* w = Wybrana();
    if (!w) return false;
    auto pelna = logika_.pobierzObudowe(w->id);   
    if (!pelna) {
        wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Obudowy"), wxOK | wxICON_ERROR, this);
        return false;
    }
    return Edycja(*pelna);
}

bool DialogObudow::Usun() {
    const auto zazn = Zaznaczone();
    if (zazn.empty()) return false;
    int uzywane = 0;
    for (const Obudowa* o : zazn) uzywane += o->liczbaElementow;
    wxString pytanie;
    if (zazn.size() == 1)
        pytanie = zazn[0]->liczbaElementow > 0
            ? wxString::Format(TR("Obudowa \"%s\" jest używana w elementach (%d). Zostanie ona usunięta z pola \"Obudowy\" tych elementów. Usunąć?"), TekstZUTF8(zazn[0]->nazwa), zazn[0]->liczbaElementow)
            : wxString::Format(TR("Usunąć obudowę \"%s\"?"), TekstZUTF8(zazn[0]->nazwa));
    else
        pytanie = wxString::Format(TR("Usunąć zaznaczone obudowy (%zu)?"), zazn.size()) +
                  (uzywane > 0 ? "\n" + wxString::Format(TR("Używane w elementach: %d (zostaną usunięte z ich pola \"Obudowy\")."), uzywane) : wxString());
    if (wxMessageBox(pytanie, TR("Obudowy"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return false;
    std::vector<long long> ids;
    for (const Obudowa* o : zazn) ids.push_back(o->id);
    for (long long id : ids)
        if (!logika_.usun(id)) {
            wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Obudowy"), wxOK | wxICON_ERROR, this);
            Odswiez();
            return false;
        }
    Odswiez();
    return true;
}

int DialogObudow::ZmienZaznaczone(const ZmianyZbiorcze& z) {
    std::vector<long long> ids;
    for (const Obudowa* o : Zaznaczone()) ids.push_back(o->id);
    int n = 0;
    for (long long id : ids) {
        auto o = logika_.pobierzObudowe(id);
        if (!o) return -1;
        if (z.rodzina) o->rodzina = *z.rodzina;
        if (z.montaz) o->montaz = *z.montaz;
        if (z.rozstaw) o->rozstaw = *z.rozstaw;
        if (z.mocStratW) o->mocStratW = *z.mocStratW;
        if (z.napiecieMaxV) o->napiecieMaxV = *z.napiecieMaxV;
        if (!logika_.zapisz(*o)) return -1;
        ++n;
    }
    Odswiez();
    Wybierz(ids);
    return n;
}

bool DialogObudow::EdytujZbiorczo() {
    const auto zazn = Zaznaczone();
    if (zazn.size() < 2) return false;
    wxDialog dlg(this, wxID_ANY, wxString::Format(TR("Zmień zaznaczone obudowy (%zu)"), zazn.size()));
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(new wxStaticText(&dlg, wxID_ANY, TR("Zaznacz pola, które mają zostać ustawione we wszystkich zaznaczonych obudowach. Pozostałe atrybuty się nie zmienią.")), 0, wxALL, 10);
    wxFlexGridSizer* g = new wxFlexGridSizer(3, 6, 8);
    wxCheckBox* cb[5]; wxTextCtrl* tc[5]; wxChoice* chM = nullptr;
    const wxString et[5] = {TR("Rodzina:"), TR("Montaż:"), TR("Moc strat [W]:"), TR("Napięcie max [V]:"), TR("Rozstaw:")};
    for (int i = 0; i < 5; ++i) {
        cb[i] = new wxCheckBox(&dlg, wxID_ANY, et[i]);
        g->Add(cb[i], 0, wxALIGN_CENTER_VERTICAL);
        if (i == 1) {
            chM = new wxChoice(&dlg, wxID_ANY);
            chM->Append(TR("(nieokreślony)")); chM->Append("THT"); chM->Append("SMD"); chM->SetSelection(1);
            g->Add(chM, 1, wxEXPAND); tc[i] = nullptr;
        } else {
            tc[i] = new wxTextCtrl(&dlg, wxID_ANY);
            g->Add(tc[i], 1, wxEXPAND);
        }
    }
    g->AddGrowableCol(1);
    s->Add(g, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    s->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxALIGN_RIGHT, 10);
    dlg.SetSizerAndFit(s);
    if (dlg.ShowModal() != wxID_OK) return false;
    ZmianyZbiorcze z;
    if (cb[0]->GetValue()) z.rodzina = tc[0]->GetValue().utf8_string();
    if (cb[1]->GetValue()) { const int sel = chM->GetSelection(); z.montaz = sel == 1 ? "THT" : sel == 2 ? "SMD" : ""; }
    auto liczba = [&](int i) -> std::optional<double> { double v; if (tekstNaLiczbe(tc[i]->GetValue().utf8_string(), v)) return v; return std::nullopt; };
    if (cb[2]->GetValue()) z.mocStratW = liczba(2);
    if (cb[3]->GetValue()) z.napiecieMaxV = liczba(3);
    if (cb[4]->GetValue()) z.rozstaw = tc[4]->GetValue().utf8_string();
    if (!z.czyCos()) return false;
    if (ZmienZaznaczone(z) < 0) {
        wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Obudowy"), wxOK | wxICON_ERROR, this);
        return false;
    }
    return true;
}

int DialogObudow::ImportujBiblioteki(const wxArrayString& pliki) {
    wxArrayString wybrane = pliki;
    if (wybrane.empty()) {
        wxFileDialog dlg(this, TR("Wybierz biblioteki Eagle"), wxEmptyString, wxEmptyString,
                         TR("Biblioteki Eagle (*.lbr)|*.lbr|Wszystkie pliki|*.*"), wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) return 0;
        dlg.GetPaths(wybrane);
    }
    int suma = 0, typowych = 0;
    wxString blednie;
    for (const wxString& plik : wybrane) {
        wxFFile f(plik, "rb");
        std::string bajty;
        if (f.IsOpened() && f.Length() > 0) {
            bajty.resize(static_cast<size_t>(f.Length()));
            if (f.Read(&bajty[0], bajty.size()) != bajty.size()) bajty.clear();
        }
        if (bajty.empty()) { blednie += "\n" + plik + ": " + TR("nie można odczytać pliku"); continue; }
        std::string blad;
        auto biblioteka = ImportLBR::parsuj(bajty, blad);
        if (!biblioteka) { blednie += "\n" + plik + ": " + TekstZUTF8(blad); continue; }
        WynikImportuObudow w;
        if (!logika_.importuj(*biblioteka, false, w)) { blednie += "\n" + plik + ": " + TekstZUTF8(logika_.pobierzBlad()); continue; }
        suma += w.dodano + w.uzupelniono;
        typowych += w.typowych;
        for (const std::string& b : w.bledy) blednie += "\n" + TekstZUTF8(b);
    }
    Odswiez();
    wxString info = wxString::Format(TR("Dodano lub uzupełniono obudów: %d."), suma);
    if (typowych > 0) info += "\n" + wxString::Format(TR("Zapisano typowych elementów: %d."), typowych);
    if (!blednie.empty()) info += TR("\n\nProblemy:") + blednie;
    wxMessageBox(info, TR("Obudowy"), wxOK | (blednie.empty() ? wxICON_INFORMATION : wxICON_WARNING), this);
    return suma;
}

int DialogObudow::UzupelnijZElementow() {
    const int n = logika_.uzupelnijZElementow();
    if (n < 0) {
        wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Obudowy"), wxOK | wxICON_ERROR, this);
        return n;
    }
    Odswiez();
    wxMessageBox(wxString::Format(TR("Dodano obudów z elementów i katalogu: %d."), n), TR("Obudowy"), wxOK | wxICON_INFORMATION, this);
    return n;
}

// ---------------------------------------------------------------------------------------------------------------------
DialogWyboruObudow::DialogWyboruObudow(wxWindow* parent, LogikaObudow& logika, const std::vector<std::string>& zaznaczone)
    : wxDialog(parent, wxID_ANY, TR("Obudowy elementu"), wxDefaultPosition, wxSize(560, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      logika_(logika), wybrane_(zaznaczone) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    glowny->Add(new wxStaticText(this, wxID_ANY, TR("Zaznacz obudowy, w których pasuje ten element (stan magazynowy jest wspólny):")), 0, wxALL, 10);
    szukaj_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    szukaj_->ShowCancelButton(true);
    glowny->Add(szukaj_, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    lista_ = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxBORDER_THEME);
    glowny->Add(lista_, 1, wxEXPAND | wxALL, 10);
    podsumowanie_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    glowny->Add(podsumowanie_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    wxBoxSizer* dol = new wxBoxSizer(wxHORIZONTAL);
    wxButton* bSlownik = new wxButton(this, wxID_ANY, TR("Słownik obudów..."));
    dol->Add(bSlownik, 0, wxRIGHT, 6);
    dol->AddStretchSpacer();
    dol->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0);
    glowny->Add(dol, 0, wxEXPAND | wxALL, 10);
    SetSizer(glowny);
    szukaj_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Odswiez(); });
    lista_->Bind(wxEVT_CHECKLISTBOX, &DialogWyboruObudow::NaZaznaczenie, this);
    bSlownik->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OtworzSlownik(); });
    Odswiez();
}

void DialogWyboruObudow::Odswiez() {
    lista_->Clear();
    if (!logika_.pobierz(szukaj_->GetValue().utf8_string(), widoczne_)) widoczne_.clear();
    for (size_t i = 0; i < widoczne_.size(); ++i) {
        const Obudowa& o = widoczne_[i];
        wxString etykieta = TekstZUTF8(o.nazwa);
        wxString opis = TekstZUTF8(o.opis);
        if (opis.length() > 60) opis = opis.Left(57) + "...";
        if (!opis.empty()) etykieta += "  -  " + opis;
        lista_->Append(etykieta);
        for (const std::string& w : wybrane_)
            if (LogikaObudow::klucz(w) == LogikaObudow::klucz(o.nazwa)) lista_->Check(static_cast<unsigned>(i), true);
    }
    OdswiezPodsumowanie();
}

void DialogWyboruObudow::OdswiezPodsumowanie() {
    podsumowanie_->SetLabel(wxString::Format(TR("Wybrane: %s"), wybrane_.empty() ? wxString("-") : TekstZUTF8(LogikaObudow::zlozTekst(wybrane_))));
}

void DialogWyboruObudow::Zaznacz(const std::string& nazwa, bool tak) {
    const std::string k = LogikaObudow::klucz(nazwa);
    for (auto it = wybrane_.begin(); it != wybrane_.end(); ++it)
        if (LogikaObudow::klucz(*it) == k) { wybrane_.erase(it); break; }
    if (tak) {
        std::string pelna = nazwa;
        for (const Obudowa& o : widoczne_) if (LogikaObudow::klucz(o.nazwa) == k) pelna = o.nazwa;
        wybrane_.push_back(pelna);
    }
    for (size_t i = 0; i < widoczne_.size(); ++i)
        if (LogikaObudow::klucz(widoczne_[i].nazwa) == k) lista_->Check(static_cast<unsigned>(i), tak);
    OdswiezPodsumowanie();
}

void DialogWyboruObudow::Szukaj(const wxString& tekst) { szukaj_->SetValue(tekst); Odswiez(); }

void DialogWyboruObudow::NaZaznaczenie(wxCommandEvent& event) {
    const int i = event.GetInt();
    if (i < 0 || static_cast<size_t>(i) >= widoczne_.size()) return;
    Zaznacz(widoczne_[static_cast<size_t>(i)].nazwa, lista_->IsChecked(static_cast<unsigned>(i)));
}

void DialogWyboruObudow::OtworzSlownik() {
    DialogObudow dlg(this, logika_);
    dlg.ShowModal();
    Odswiez();
}
