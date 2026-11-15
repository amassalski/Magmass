#include "DialogPozyczki.h"

#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <wx/dateevt.h>

namespace {
wxString Przytnij(const wxString& s) { return s.Strip(wxString::both); }
std::string Iso(const wxDateTime& d) { return d.IsValid() ? d.FormatISODate().utf8_string() : std::string(); }
}  // namespace

DialogPozyczki::DialogPozyczki(wxWindow* parent, const wxString& tytul, const wxString& komunikatBraku, const wxString& przedmiot,
                               const std::vector<std::string>& osoby)
    : wxDialog(parent, wxID_ANY, tytul, wxDefaultPosition, wxSize(520, 380), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      komunikatBraku_(komunikatBraku) {
    wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
    g->Add(new wxStaticText(this, wxID_ANY, przedmiot), 0, wxALL, 10);
    wxFlexGridSizer* s = new wxFlexGridSizer(2, 6, 8);
    s->AddGrowableCol(1);
    auto dodaj = [&](const wxString& et, wxWindow* p) {
        s->Add(new wxStaticText(this, wxID_ANY, et), 0, wxALIGN_CENTER_VERTICAL);
        s->Add(p, 1, wxEXPAND);
    };
    cbOsoba_ = new wxComboBox(this, wxID_ANY);
    for (const std::string& o : osoby) cbOsoba_->Append(TekstZUTF8(o));
    dodaj(TR("Komu *"), cbOsoba_);
    dpOd_ = new wxDatePickerCtrl(this, wxID_ANY, wxDateTime::Today(), wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN);
    dodaj(TR("Data pożyczenia"), dpOd_);
    dpTermin_ = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN | wxDP_ALLOWNONE);
    dpTermin_->SetValue(wxDateTime());
    dodaj(TR("Termin zwrotu (opcjonalnie)"), dpTermin_);
    g->Add(s, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    g->Add(new wxStaticText(this, wxID_ANY, TR("Uwagi:")), 0, wxLEFT | wxRIGHT | wxTOP, 10);
    tcUwagi_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 70), wxTE_MULTILINE);
    g->Add(tcUwagi_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    g->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
    SetSizer(g);
    PomocGUI::Dodaj(cbOsoba_, "pozyczka.osoba");
    PomocGUI::Dodaj(dpTermin_, "pozyczka.termin");
    Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        if (Przytnij(cbOsoba_->GetValue()).empty()) {
            wxMessageBox(komunikatBraku_, TR("Pożyczki"), wxOK | wxICON_WARNING, this);
            return;
        }
        e.Skip();
    }, wxID_OK);
}

std::string DialogPozyczki::Osoba() const { return Przytnij(cbOsoba_->GetValue()).utf8_string(); }
std::string DialogPozyczki::Od() const { return Iso(dpOd_->GetValue()); }
std::string DialogPozyczki::Termin() const { return Iso(dpTermin_->GetValue()); }
std::string DialogPozyczki::Uwagi() const { return Przytnij(tcUwagi_->GetValue()).utf8_string(); }
