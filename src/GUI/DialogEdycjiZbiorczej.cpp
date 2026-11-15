#include "DialogEdycjiZbiorczej.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Narzedzia/Liczby.h"

namespace {
const int MAKS = 100000000;
}

DialogEdycjiZbiorczej::DialogEdycjiZbiorczej(wxWindow* parent, LogikaMagazynu& logika, int liczbaElementow)
    : wxDialog(parent, wxID_ANY, TR("Edycja zbiorcza elementów"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    glowny->Add(new wxStaticText(this, wxID_ANY,
                    wxString::Format(TR("Zaznaczone elementy: %d. Zaznacz pola, które mają dostać tę samą wartość;\nreszta danych pozostanie bez zmian."), liczbaElementow)),
                0, wxALL, 12);
    wxFlexGridSizer* g = new wxFlexGridSizer(3, 8, 10);
    g->AddGrowableCol(2);
    auto wiersz = [&](wxCheckBox*& cb, const wxString& etykieta, auto* kontrolka) {
        cb = new wxCheckBox(this, wxID_ANY, etykieta);
        g->Add(cb, 0, wxALIGN_CENTER_VERTICAL);
        g->Add(new wxStaticText(this, wxID_ANY, wxEmptyString), 0);
        g->Add(kontrolka, 1, wxEXPAND);
        cb->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { Odswiez(); });
    };
    chKategoria_ = new wxChoice(this, wxID_ANY);
    chKategoria_->Append(TR("(bez kategorii)"));
    idKategorii_.push_back(0);
    for (const Kategoria& k : logika.kategorie()) {
        chKategoria_->Append(TekstZUTF8(k.pelnaNazwa));
        idKategorii_.push_back(k.id);
    }
    chKategoria_->SetSelection(0);
    wiersz(cbKategoria_, TR("Kategoria"), chKategoria_);
    tcProducent_ = new wxTextCtrl(this, wxID_ANY);
    wiersz(cbProducent_, TR("Producent"), tcProducent_);
    tcTolerancja_ = new wxTextCtrl(this, wxID_ANY);
    tcTolerancja_->SetHint(TR("np. 1%"));
    wiersz(cbTolerancja_, TR("Tolerancja"), tcTolerancja_);
    scIlosc_ = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, MAKS, 0);
    wiersz(cbIlosc_, TR("Ilość (ustaw stan na)"), scIlosc_);
    scMinimum_ = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, MAKS, 0);
    wiersz(cbMinimum_, TR("Stan minimalny"), scMinimum_);
    tcCena_ = new wxTextCtrl(this, wxID_ANY);
    tcCena_->SetHint(TR("np. 0,12 (puste = usuń cenę)"));
    wiersz(cbCena_, TR("Cena jednostkowa"), tcCena_);
    wxBoxSizer* tagi = new wxBoxSizer(wxHORIZONTAL);
    tcTagi_ = new wxTextCtrl(this, wxID_ANY);
    chTagi_ = new wxChoice(this, wxID_ANY);
    chTagi_->Append(TR("dopisz do istniejących"));
    chTagi_->Append(TR("zastąp istniejące"));
    chTagi_->SetSelection(0);
    tagi->Add(tcTagi_, 1, wxRIGHT, 6);
    tagi->Add(chTagi_, 0);
    wiersz(cbTagi_, TR("Tagi"), tagi);
    glowny->Add(g, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    glowny->Add(new wxStaticLine(this), 0, wxEXPAND);
    glowny->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
    SetSizerAndFit(glowny);
    SetMinSize(wxSize(520, -1));
    Bind(wxEVT_BUTTON, &DialogEdycjiZbiorczej::OnOK, this, wxID_OK);
    // zmiana wartości zaznacza pole
    auto zaznacz = [this](wxCheckBox* cb) { return [this, cb](wxCommandEvent& e) { cb->SetValue(true); Odswiez(); e.Skip(); }; };
    chKategoria_->Bind(wxEVT_CHOICE, zaznacz(cbKategoria_));
    tcProducent_->Bind(wxEVT_TEXT, zaznacz(cbProducent_));
    tcTolerancja_->Bind(wxEVT_TEXT, zaznacz(cbTolerancja_));
    tcCena_->Bind(wxEVT_TEXT, zaznacz(cbCena_));
    tcTagi_->Bind(wxEVT_TEXT, zaznacz(cbTagi_));
    scIlosc_->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent& e) { cbIlosc_->SetValue(true); Odswiez(); e.Skip(); });
    scMinimum_->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent& e) { cbMinimum_->SetValue(true); Odswiez(); e.Skip(); });
    Odswiez();
}

void DialogEdycjiZbiorczej::Odswiez() {
    chTagi_->Enable(cbTagi_->GetValue());
}

void DialogEdycjiZbiorczej::UstawProducenta(const wxString& t) { tcProducent_->SetValue(t); }
void DialogEdycjiZbiorczej::UstawIloscMinimalna(int n) { scMinimum_->SetValue(n); cbMinimum_->SetValue(true); }
void DialogEdycjiZbiorczej::UstawTagi(const wxString& t, bool dopisz) {
    tcTagi_->SetValue(t);
    chTagi_->SetSelection(dopisz ? 0 : 1);
    cbTagi_->SetValue(true);
}

ZmianyZbiorcze DialogEdycjiZbiorczej::Zmiany() const {
    auto tekst = [](const wxTextCtrl* t) { return t->GetValue().Strip(wxString::both).utf8_string(); };
    ZmianyZbiorcze z;
    if (cbKategoria_->GetValue()) {
        const int w = chKategoria_->GetSelection();
        std::optional<long long> id;
        if (w > 0 && static_cast<size_t>(w) < idKategorii_.size()) id = idKategorii_[static_cast<size_t>(w)];
        z.kategoria = id;
    }
    if (cbProducent_->GetValue()) z.producent = tekst(tcProducent_);
    if (cbTolerancja_->GetValue()) z.tolerancja = tekst(tcTolerancja_);
    if (cbIlosc_->GetValue()) z.ilosc = scIlosc_->GetValue();
    if (cbMinimum_->GetValue()) z.iloscMinimalna = scMinimum_->GetValue();
    if (cbCena_->GetValue()) {
        std::optional<double> cena;
        double v = 0;
        if (!tekst(tcCena_).empty() && tekstNaLiczbe(tekst(tcCena_), v)) cena = v;
        z.cena = cena;
    }
    if (cbTagi_->GetValue()) { z.tagi = tekst(tcTagi_); z.tagiDopisz = chTagi_->GetSelection() == 0; }
    return z;
}

void DialogEdycjiZbiorczej::OnOK(wxCommandEvent& event) {
    if (Zmiany().pusta()) {
        wxMessageBox(TR("Zaznacz przynajmniej jedno pole do zmiany."), TR("Edycja zbiorcza"), wxOK | wxICON_INFORMATION, this);
        return;
    }
    if (cbCena_->GetValue()) {
        const std::string t = tcCena_->GetValue().Strip(wxString::both).utf8_string();
        double v = 0;
        if (!t.empty() && (!tekstNaLiczbe(t, v) || v < 0)) {
            wxMessageBox(TR("Cena musi być liczbą nieujemną (np. 0,12)."), TR("Edycja zbiorcza"), wxOK | wxICON_WARNING, this);
            return;
        }
    }
    event.Skip();
}
