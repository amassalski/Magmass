#include "DialogDatasheetMouser.h"

#include "../Narzedzia/Tlumaczenia.h"
#include "../Wyszukiwanie/KlientHTTP.h"

DialogDatasheetMouser::DialogDatasheetMouser(wxWindow* parent, const wxString& fraza, const std::string& klucz)
    : wxDialog(parent, wxID_ANY, TR("Pobierz datasheet z Mousera"), wxDefaultPosition, wxSize(820, 480), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      klucz_(klucz) {
    wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
    g->Add(new wxStaticText(this, wxID_ANY, TR("Wpisz symbol albo numer części (np. LM358) i wybierz właściwą pozycję. Program pobierze jej datasheet.")),
           0, wxALL, 10);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);
    fraza_ = new wxTextCtrl(this, wxID_ANY, fraza, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    szukaj_ = new wxButton(this, wxID_ANY, TR("Szukaj"));
    gora->Add(fraza_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    gora->Add(szukaj_, 0, wxALIGN_CENTER_VERTICAL);
    g->Add(gora, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

    lista_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    lista_->InsertColumn(0, TR("Numer producenta"), wxLIST_FORMAT_LEFT, 150);
    lista_->InsertColumn(1, TR("Producent"), wxLIST_FORMAT_LEFT, 150);
    lista_->InsertColumn(2, TR("Opis"), wxLIST_FORMAT_LEFT, 300);
    lista_->InsertColumn(3, TR("Obudowa"), wxLIST_FORMAT_LEFT, 100);
    lista_->InsertColumn(4, TR("Datasheet"), wxLIST_FORMAT_LEFT, 70);
    g->Add(lista_, 1, wxEXPAND | wxALL, 10);
    stan_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    g->Add(stan_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxBoxSizer* przyciski = new wxBoxSizer(wxHORIZONTAL);
    wxButton* recznie = new wxButton(this, wxID_ANY, TR("Wpisz adres ręcznie..."));
    ok_ = new wxButton(this, wxID_OK, TR("Pobierz datasheet"));
    ok_->SetDefault();
    ok_->Enable(false);
    przyciski->Add(recznie, 0, wxRIGHT, 6);
    przyciski->AddStretchSpacer();
    przyciski->Add(ok_, 0, wxRIGHT, 6);
    przyciski->Add(new wxButton(this, wxID_CANCEL, TR("Anuluj")), 0);
    g->Add(przyciski, 0, wxEXPAND | wxALL, 10);
    SetSizer(g);
    SetEscapeId(wxID_CANCEL);

    szukaj_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Szukaj(); });
    fraza_->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { Szukaj(); });
    recznie->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_EDIT); });
    lista_->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent&) { ok_->Enable(Wybrana() != nullptr); });
    lista_->Bind(wxEVT_LIST_ITEM_DESELECTED, [this](wxListEvent&) { ok_->Enable(Wybrana() != nullptr); });
    lista_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) { if (Wybrana()) EndModal(wxID_OK); });
    CentreOnParent();
}

DialogDatasheetMouser::~DialogDatasheetMouser() { *zyje_ = false; }

const Mouser::Czesc* DialogDatasheetMouser::Wybrana() const {
    const long i = lista_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (i < 0 || static_cast<size_t>(i) >= czesci_.size()) return nullptr;
    return &czesci_[static_cast<size_t>(i)];
}

bool DialogDatasheetMouser::Wybierz(int indeks) {
    if (indeks < 0 || indeks >= LiczbaWynikow()) return false;
    lista_->SetItemState(indeks, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
    ok_->Enable(true);
    return true;
}

void DialogDatasheetMouser::PokazBlad(const wxString& tekst) {
    stan_->SetLabel(TR("✗ ") + tekst);
    Layout();
}

void DialogDatasheetMouser::Szukaj() {
    const std::string fraza = fraza_->GetValue().Strip(wxString::both).utf8_string();
    if (fraza.empty()) { PokazBlad(TR("Wpisz symbol albo numer części.")); return; }
    if (!Mouser::czyKluczPoprawny(klucz_)) { PokazBlad(TR("Klucz API Mouser w Ustawieniach (zakładka Inne) ma nieprawidłowy format.")); return; }
    szukaj_->Enable(false);
    stan_->SetLabel(TR("Szukam w Mouserze..."));
    std::weak_ptr<bool> zyje = zyje_;
    KlientHTTP::wyslijJSON(Mouser::adresZapytania(klucz_), Mouser::cialoZapytania(fraza), [this, zyje](const WynikHTTP& w) {
        auto z = zyje.lock();
        if (!z || !*z) return;   // okno już zamknięte
        szukaj_->Enable(true);
        if (!w.ok) {
            if (w.kod == 403 || w.kod == 401) PokazBlad(TR("Mouser odrzucił zapytanie: sprawdź klucz API w Ustawieniach."));
            else if (w.kod == 429) PokazBlad(TR("Za dużo zapytań do Mousera (limit: 30 na minutę, 1000 na dobę). Spróbuj za chwilę."));
            else PokazBlad(TR("Nie udało się połączyć z Mouserem: ") + wxString::FromUTF8(w.blad));
            return;
        }
        ZastosujOdpowiedz(w.tresc);
    });
}

int DialogDatasheetMouser::ZastosujOdpowiedz(const std::string& json) {
    std::string blad;
    std::vector<Mouser::Czesc> nowe;
    lista_->DeleteAllItems();
    czesci_.clear();
    ok_->Enable(false);
    if (!Mouser::zOdpowiedzi(json, nowe, blad)) { PokazBlad(wxString::FromUTF8(blad)); return -1; }
    czesci_ = std::move(nowe);
    for (size_t i = 0; i < czesci_.size(); ++i) {
        const Mouser::Czesc& c = czesci_[i];
        const long w = lista_->InsertItem(static_cast<long>(i), wxString::FromUTF8(c.numerProducenta.empty() ? c.numerMouser : c.numerProducenta));
        lista_->SetItem(w, 1, wxString::FromUTF8(c.producent));
        lista_->SetItem(w, 2, wxString::FromUTF8(c.opis));
        lista_->SetItem(w, 3, wxString::FromUTF8(c.obudowa()));
        lista_->SetItem(w, 4, c.adresDatasheetu.empty() ? TR("brak") : TR("jest"));
    }
    stan_->SetLabel(czesci_.empty() ? TR("Mouser nie znalazł takiej części. Spróbuj podać dokładniejszy numer.")
                                    : wxString::Format(TR("Znaleziono: %zu. Wybierz pozycję i kliknij Pobierz datasheet."), czesci_.size()));
    Layout();
    return LiczbaWynikow();
}
