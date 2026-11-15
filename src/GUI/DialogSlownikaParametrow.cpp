#include "DialogSlownikaParametrow.h"

#include "UstawieniaOkna.h"
#include "../Narzedzia/Tlumaczenia.h"

namespace {

class DialogEdycjiWpisu : public wxDialog {
public:
    DialogEdycjiWpisu(wxWindow* parent, const WpisSlownikaParametrow& w, LogikaSlownikaParametrow& logika)
        : wxDialog(parent, wxID_ANY, w.id ? TR("Zmień wpis słownika") : TR("Nowy wpis słownika"), wxDefaultPosition, wxSize(560, 300),
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
          wpis_(w), logika_(logika) {
        wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
        wxFlexGridSizer* siatka = new wxFlexGridSizer(2, 6, 8);
        siatka->AddGrowableCol(1);
        siatka->AddGrowableRow(1);
        siatka->Add(new wxStaticText(this, wxID_ANY, TR("Symbol (np. W_amax):")), 0, wxALIGN_CENTER_VERTICAL);
        tcSymbol_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(w.symbol));
        siatka->Add(tcSymbol_, 1, wxEXPAND);
        siatka->Add(new wxStaticText(this, wxID_ANY, TR("Opis (dymek):")), 0, wxTOP, 4);
        tcOpis_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(w.opis), wxDefaultPosition, wxSize(-1, 100), wxTE_MULTILINE);
        siatka->Add(tcOpis_, 1, wxEXPAND);
        glowny->Add(siatka, 1, wxEXPAND | wxALL, 10);
        glowny->Add(new wxStaticText(this, wxID_ANY, TR("Wielkość liter, spacje, podkreślenia i nawiasy w symbolu się nie liczą: V_CE(sat) i vce sat to ten sam symbol.")),
                    0, wxLEFT | wxRIGHT, 10);
        glowny->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
        SetSizer(glowny);
        Bind(wxEVT_BUTTON, &DialogEdycjiWpisu::NaOK, this, wxID_OK);
    }
    const WpisSlownikaParametrow& Wynik() const { return wpis_; }

private:
    WpisSlownikaParametrow wpis_;
    LogikaSlownikaParametrow& logika_;
    wxTextCtrl *tcSymbol_, *tcOpis_;

    void NaOK(wxCommandEvent& event) {
        wpis_.symbol = tcSymbol_->GetValue().Strip(wxString::both).utf8_string();
        wpis_.opis = tcOpis_->GetValue().Strip(wxString::both).utf8_string();
        for (char& c : wpis_.opis) if (c == '\n' || c == '\r') c = ' ';
        auto id = logika_.zapisz(wpis_);
        if (!id) {
            wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Słownik parametrów"), wxOK | wxICON_WARNING, this);
            return;
        }
        wpis_.id = *id;
        event.Skip();
    }
};

} // namespace

DialogSlownikaParametrow::DialogSlownikaParametrow(wxWindow* parent, LogikaSlownikaParametrow& logika)
    : wxDialog(parent, wxID_ANY, TR("Słownik parametrów"), wxDefaultPosition, wxSize(900, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      logika_(logika) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);
    szukaj_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(260, -1));
    szukaj_->ShowCancelButton(true);
    gora->Add(szukaj_, 0, wxRIGHT, 10);
    licznik_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    gora->Add(licznik_, 1, wxALIGN_CENTER_VERTICAL);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 10);
    glowny->Add(new wxStaticText(this, wxID_ANY, TR("Opisy symboli parametrów pokazywane w dymku po najechaniu na parametr w katalogu (gdy parametr nie ma własnego opisu).")),
                0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    lista_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxBORDER_THEME);
    lista_->AppendColumn(TR("Symbol"), wxLIST_FORMAT_LEFT, 150);
    lista_->AppendColumn(TR("Opis"), wxLIST_FORMAT_LEFT, 700);
    glowny->Add(lista_, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

    wxBoxSizer* przyciski = new wxBoxSizer(wxHORIZONTAL);
    wxButton* bDodaj = new wxButton(this, wxID_ANY, TR("Dodaj..."));
    wxButton* bEdytuj = new wxButton(this, wxID_ANY, TR("Edytuj..."));
    wxButton* bUsun = new wxButton(this, wxID_ANY, TR("Usuń"));
    wxButton* bPrzywroc = new wxButton(this, wxID_ANY, TR("Przywróć brakujące wbudowane"));
    bPrzywroc->SetToolTip(TR("Dopisuje wbudowane symbole, których w słowniku brakuje; istniejących wpisów nie zmienia."));
    wxButton* bZamknij = new wxButton(this, wxID_CLOSE, TR("Zamknij"));
    for (wxButton* b : {bDodaj, bEdytuj, bUsun, bPrzywroc}) przyciski->Add(b, 0, wxRIGHT, 6);
    przyciski->AddStretchSpacer(1);
    przyciski->Add(bZamknij, 0);
    glowny->Add(przyciski, 0, wxEXPAND | wxALL, 10);
    SetSizer(glowny);
    SetEscapeId(wxID_CLOSE);

    bDodaj->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Dodaj(); });
    bEdytuj->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Edytuj(); });
    bUsun->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Usun(); });
    bPrzywroc->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PrzywrocBrakujace(); });
    bZamknij->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); });
    bEdytuj->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(Wybrany() != nullptr); });
    bUsun->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(!Zaznaczone().empty()); });
    szukaj_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Odswiez(); });
    lista_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) { Edytuj(); });
    lista_->Bind(wxEVT_LIST_KEY_DOWN, [this](wxListEvent& e) { if (e.GetKeyCode() == WXK_DELETE) Usun(); else e.Skip(); });

    UstawieniaOkna::WczytajKolumny(lista_, "SlownikParametrow");
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) { UstawieniaOkna::ZapiszKolumny(lista_, "SlownikParametrow"); e.Skip(); });
    Odswiez();
}

void DialogSlownikaParametrow::Odswiez() {
    std::vector<long long> zachowaj = Zaznaczone();
    lista_->DeleteAllItems();
    if (!logika_.pobierz(szukaj_->GetValue().utf8_string(), wpisy_)) {
        wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Słownik parametrów"), wxOK | wxICON_ERROR, this);
        wpisy_.clear();
    }
    for (size_t i = 0; i < wpisy_.size(); ++i) {
        const long poz = lista_->InsertItem(static_cast<long>(i), TekstZUTF8(wpisy_[i].symbol));
        lista_->SetItem(poz, 1, TekstZUTF8(wpisy_[i].opis));
        for (long long id : zachowaj)
            if (id == wpisy_[i].id) lista_->SetItemState(poz, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
    licznik_->SetLabel(wxString::Format(TR("Wpisów: %d"), static_cast<int>(wpisy_.size())));
}

const WpisSlownikaParametrow* DialogSlownikaParametrow::Wybrany() const {
    const long i = lista_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    return i >= 0 && i < static_cast<long>(wpisy_.size()) ? &wpisy_[i] : nullptr;
}

std::vector<long long> DialogSlownikaParametrow::Zaznaczone() const {
    std::vector<long long> w;
    for (long i = lista_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); i >= 0;
         i = lista_->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
        if (i < static_cast<long>(wpisy_.size())) w.push_back(wpisy_[i].id);
    return w;
}

void DialogSlownikaParametrow::Wybierz(long long id) {
    for (size_t i = 0; i < wpisy_.size(); ++i) {
        const bool tak = wpisy_[i].id == id;
        lista_->SetItemState(static_cast<long>(i), tak ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED);
    }
}

void DialogSlownikaParametrow::Szukaj(const wxString& tekst) {
    szukaj_->ChangeValue(tekst);
    Odswiez();
}

bool DialogSlownikaParametrow::Edycja(WpisSlownikaParametrow wpis) {
    DialogEdycjiWpisu d(this, wpis, logika_);
    if (d.ShowModal() != wxID_OK) return false;
    Odswiez();
    Wybierz(d.Wynik().id);
    return true;
}

bool DialogSlownikaParametrow::Dodaj() { return Edycja(WpisSlownikaParametrow()); }

bool DialogSlownikaParametrow::Edytuj() {
    const WpisSlownikaParametrow* w = Wybrany();
    return w && Edycja(*w);
}

bool DialogSlownikaParametrow::Usun() {
    const std::vector<long long> id = Zaznaczone();
    if (id.empty()) return false;
    const wxString pytanie = id.size() == 1 ? TR("Usunąć zaznaczony wpis ze słownika?")
                                            : wxString::Format(TR("Usunąć zaznaczone wpisy (%d) ze słownika?"), static_cast<int>(id.size()));
    if (wxMessageBox(pytanie, TR("Słownik parametrów"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES) return false;
    for (long long i : id)
        if (!logika_.usun(i)) {
            wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Słownik parametrów"), wxOK | wxICON_ERROR, this);
            break;
        }
    Odswiez();
    return true;
}

bool DialogSlownikaParametrow::PrzywrocBrakujace() {
    const int n = logika_.przywrocBrakujace();
    if (n < 0) {
        wxMessageBox(TekstZUTF8(logika_.pobierzBlad()), TR("Słownik parametrów"), wxOK | wxICON_ERROR, this);
        return false;
    }
    Odswiez();
    wxMessageBox(n == 0 ? TR("Wszystkie wbudowane symbole są już w słowniku.")
                        : wxString::Format(TR("Dopisano brakujące wbudowane symbole: %d."), n),
                 TR("Słownik parametrów"), wxOK | wxICON_INFORMATION, this);
    return n > 0;
}

DialogDoSlownika::DialogDoSlownika(wxWindow* parent, const std::vector<std::string>& symbole)
    : wxDialog(parent, wxID_ANY, TR("Dodaj do słownika parametrów"), wxDefaultPosition, wxSize(760, 420), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
    g->Add(new wxStaticText(this, wxID_ANY,
           TR("Tych symboli nie ma jeszcze w słowniku, więc dymek nie powie, co oznaczają. Wpisz opis przy tych, które chcesz zapamiętać "
              "(zaznaczone wiersze z opisem trafią do słownika; pozostałe zostaną pominięte).")), 0, wxALL, 10);
    siatka_ = new wxGrid(this, wxID_ANY);
    siatka_->CreateGrid(static_cast<int>(symbole.size()), 3);
    siatka_->SetColLabelValue(0, TR("Dodaj"));
    siatka_->SetColLabelValue(1, TR("Symbol"));
    siatka_->SetColLabelValue(2, TR("Opis"));
    siatka_->HideRowLabels();
    siatka_->SetColSize(0, 60);
    siatka_->SetColSize(1, 190);
    siatka_->SetColSize(2, 440);
    for (int i = 0; i < static_cast<int>(symbole.size()); ++i) {
        siatka_->SetCellValue(i, 0, "0");
        siatka_->SetCellRenderer(i, 0, new wxGridCellBoolRenderer);
        siatka_->SetCellEditor(i, 0, new wxGridCellBoolEditor);
        siatka_->SetCellValue(i, 1, TekstZUTF8(symbole[i]));
        siatka_->SetReadOnly(i, 1);
    }
    // wpisanie opisu zaznacza wiersz
    siatka_->Bind(wxEVT_GRID_CELL_CHANGED, [this](wxGridEvent& e) {
        if (e.GetCol() == 2 && !siatka_->GetCellValue(e.GetRow(), 2).Strip(wxString::both).empty())
            siatka_->SetCellValue(e.GetRow(), 0, "1");
        e.Skip();
    });
    g->Add(siatka_, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
    g->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
    SetSizer(g);
}

std::vector<WpisSlownikaParametrow> DialogDoSlownika::Wpisy() const {
    std::vector<WpisSlownikaParametrow> w;
    if (siatka_->IsCellEditControlEnabled()) siatka_->SaveEditControlValue();
    for (int i = 0; i < siatka_->GetNumberRows(); ++i) {
        const wxString opis = siatka_->GetCellValue(i, 2).Strip(wxString::both);
        if (siatka_->GetCellValue(i, 0) != "1" || opis.empty()) continue;
        WpisSlownikaParametrow e;
        e.symbol = siatka_->GetCellValue(i, 1).utf8_string();
        e.opis = opis.utf8_string();
        for (char& c : e.opis) if (c == '\n' || c == '\r') c = ' ';
        w.push_back(e);
    }
    return w;
}

void DialogDoSlownika::UstawOpis(int wiersz, const wxString& opis, bool zaznacz) {
    siatka_->SetCellValue(wiersz, 2, opis);
    siatka_->SetCellValue(wiersz, 0, zaznacz ? "1" : "0");
}
