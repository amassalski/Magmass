#include "DialogZebra.h"
#include "../Pomoc/PomocGUI.h"
#include "../Narzedzia/Tlumaczenia.h"

#include <wx/filedlg.h>
#include <wx/sizer.h>

DialogZebra::DialogZebra(wxWindow* parent, const UstawieniaZebry& u)
    : wxDialog(parent, wxID_ANY, TR("Drukarka Zebra (ZPL)"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      baza_(u) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer* polaczenie = new wxStaticBoxSizer(wxVERTICAL, this, TR("Połączenie z drukarką"));
    wxFlexGridSizer* siatka = new wxFlexGridSizer(2, 6, 8);
    siatka->AddGrowableCol(1, 1);

    rbSiec_ = new wxRadioButton(this, wxID_ANY, TR("Sieć (Ethernet / Wi-Fi)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    tcHost_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(u.wysylka.host));
    tcHost_->SetHint(TR("adres IP drukarki, np. 192.168.1.50"));
    scPort_ = new wxSpinCtrl(this, wxID_ANY, wxString::Format("%d", u.wysylka.port), wxDefaultPosition, wxSize(90, -1),
                             wxSP_ARROW_KEYS, 1, 65535, u.wysylka.port);
    wxBoxSizer* hostPort = new wxBoxSizer(wxHORIZONTAL);
    hostPort->Add(tcHost_, 1, wxEXPAND | wxRIGHT, 6);
    hostPort->Add(new wxStaticText(this, wxID_ANY, TR("port")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    hostPort->Add(scPort_, 0);
    siatka->Add(rbSiec_, 0, wxALIGN_CENTER_VERTICAL);
    siatka->Add(hostPort, 1, wxEXPAND);

    rbSystem_ = new wxRadioButton(this, wxID_ANY, TR("USB / drukarka w systemie"));
    cbDrukarka_ = new wxComboBox(this, wxID_ANY, TekstZUTF8(u.wysylka.drukarka));
    for (const std::string& n : WysylkaZPL::listaDrukarek()) cbDrukarka_->Append(TekstZUTF8(n));
    siatka->Add(rbSystem_, 0, wxALIGN_CENTER_VERTICAL);
    siatka->Add(cbDrukarka_, 1, wxEXPAND);

    rbPlik_ = new wxRadioButton(this, wxID_ANY, TR("Tylko zapis do pliku .zpl"));
    tcPlik_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(u.wysylka.plik));
    wxButton* bPlik = new wxButton(this, wxID_ANY, "...", wxDefaultPosition, wxSize(32, -1));
    wxBoxSizer* plikRz = new wxBoxSizer(wxHORIZONTAL);
    plikRz->Add(tcPlik_, 1, wxEXPAND | wxRIGHT, 4);
    plikRz->Add(bPlik, 0);
    siatka->Add(rbPlik_, 0, wxALIGN_CENTER_VERTICAL);
    siatka->Add(plikRz, 1, wxEXPAND);
    polaczenie->Add(siatka, 0, wxEXPAND | wxALL, 6);
    glowny->Add(polaczenie, 0, wxEXPAND | wxALL, 8);

    wxStaticBoxSizer* etykieta = new wxStaticBoxSizer(wxVERTICAL, this, TR("Etykieta"));
    wxFlexGridSizer* s2 = new wxFlexGridSizer(2, 6, 8);
    scSzer_ = new wxSpinCtrl(this, wxID_ANY, wxString::Format("%d", static_cast<int>(u.etykieta.szerMm + 0.5)),
                             wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, 10, 120, static_cast<int>(u.etykieta.szerMm + 0.5));
    scWys_ = new wxSpinCtrl(this, wxID_ANY, wxString::Format("%d", static_cast<int>(u.etykieta.wysMm + 0.5)),
                            wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, 10, 400, static_cast<int>(u.etykieta.wysMm + 0.5));
    chDpi_ = new wxChoice(this, wxID_ANY);
    chDpi_->Append("203 dpi");
    chDpi_->Append("300 dpi");
    chDpi_->Append("600 dpi");
    chDpi_->SetSelection(u.etykieta.dpi == 300 ? 1 : (u.etykieta.dpi == 600 ? 2 : 0));
    auto dodaj = [&](const wxString& t, wxWindow* k) {
        s2->Add(new wxStaticText(this, wxID_ANY, t), 0, wxALIGN_CENTER_VERTICAL);
        s2->Add(k, 0, wxEXPAND);
    };
    dodaj(TR("Szerokość (w poprzek głowicy) [mm]"), scSzer_);
    dodaj(TR("Wysokość (wzdłuż rolki) [mm]"), scWys_);
    dodaj(TR("Rozdzielczość drukarki"), chDpi_);
    etykieta->Add(s2, 0, wxEXPAND | wxALL, 6);
    cbBezPolskich_ = new wxCheckBox(this, wxID_ANY, TR("Zamieniaj polskie litery, µ i Ω na znaki ASCII"));
    cbBezPolskich_->SetValue(u.etykieta.bezPolskichZnakow);
    cbObrot_ = new wxCheckBox(this, wxID_ANY, TR("Obróć wydruk o 180 stopni"));
    cbObrot_->SetValue(u.etykieta.obrot180);
    etykieta->Add(cbBezPolskich_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
    etykieta->Add(cbObrot_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
    glowny->Add(etykieta, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    stan_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    glowny->Add(stan_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxBoxSizer* dol = new wxBoxSizer(wxHORIZONTAL);
    wxButton* bProba = new wxButton(this, wxID_ANY, TR("Etykieta próbna"));
    dol->Add(bProba, 0, wxRIGHT, 12);
    dol->AddStretchSpacer(1);
    dol->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0);
    glowny->Add(dol, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    SetSizerAndFit(glowny);
    SetMinSize(wxSize(560, -1));

    switch (u.wysylka.tryb) {
        case WysylkaZPL::Tryb::System: rbSystem_->SetValue(true); break;
        case WysylkaZPL::Tryb::Plik: rbPlik_->SetValue(true); break;
        default: rbSiec_->SetValue(true);
    }

    PomocGUI::Dodaj(rbSiec_, "zebra.tryb");
    PomocGUI::Dodaj(rbSystem_, "zebra.tryb");
    PomocGUI::Dodaj(rbPlik_, "zebra.tryb");
    PomocGUI::Dodaj(tcHost_, "zebra.host");
    PomocGUI::Dodaj(scPort_, "zebra.port");
    PomocGUI::Dodaj(cbDrukarka_, "zebra.drukarka");
    PomocGUI::Dodaj(tcPlik_, "zebra.plik");
    PomocGUI::Dodaj(scSzer_, "zebra.szerokosc");
    PomocGUI::Dodaj(scWys_, "zebra.wysokosc");
    PomocGUI::Dodaj(chDpi_, "zebra.dpi");
    PomocGUI::Dodaj(cbBezPolskich_, "zebra.polskie");
    PomocGUI::Dodaj(cbObrot_, "zebra.obrot");

    for (wxRadioButton* r : {rbSiec_, rbSystem_, rbPlik_})
        r->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { OdswiezAktywne(); });
    bPlik->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { WybierzPlik(); });
    bProba->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { WyslijProbna(); });
    OdswiezAktywne();
}

void DialogZebra::OdswiezAktywne() {
    tcHost_->Enable(rbSiec_->GetValue());
    scPort_->Enable(rbSiec_->GetValue());
    cbDrukarka_->Enable(rbSystem_->GetValue());
    tcPlik_->Enable(rbPlik_->GetValue());
}

void DialogZebra::UstawTryb(WysylkaZPL::Tryb tryb) {
    rbSiec_->SetValue(tryb == WysylkaZPL::Tryb::Siec);
    rbSystem_->SetValue(tryb == WysylkaZPL::Tryb::System);
    rbPlik_->SetValue(tryb == WysylkaZPL::Tryb::Plik);
    OdswiezAktywne();
}

void DialogZebra::WybierzPlik() {
    wxFileDialog dlg(this, TR("Zapisz polecenia ZPL"), wxEmptyString, "etykiety.zpl", TR("Pliki ZPL (*.zpl)|*.zpl|Wszystkie pliki|*"),
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) tcPlik_->SetValue(dlg.GetPath());
}

UstawieniaZebry DialogZebra::Ustawienia() const {
    UstawieniaZebry u = baza_;
    u.wysylka.tryb = rbSystem_->GetValue() ? WysylkaZPL::Tryb::System
                                           : (rbPlik_->GetValue() ? WysylkaZPL::Tryb::Plik : WysylkaZPL::Tryb::Siec);
    u.wysylka.host = tcHost_->GetValue().Strip(wxString::both).utf8_string();
    u.wysylka.port = scPort_->GetValue();
    u.wysylka.drukarka = cbDrukarka_->GetValue().Strip(wxString::both).utf8_string();
    u.wysylka.plik = tcPlik_->GetValue().Strip(wxString::both).utf8_string();
    u.etykieta.szerMm = scSzer_->GetValue();
    u.etykieta.wysMm = scWys_->GetValue();
    const int s = chDpi_->GetSelection();
    u.etykieta.dpi = s == 1 ? 300 : (s == 2 ? 600 : 203);
    u.etykieta.bezPolskichZnakow = cbBezPolskich_->GetValue();
    u.etykieta.obrot180 = cbObrot_->GetValue();
    return u;
}

bool DialogZebra::WyslijProbna() {
    const UstawieniaZebry u = Ustawienia();
    std::string blad;
    wxBusyCursor zajety;
    if (WysylkaZPL::wyslij(u.wysylka, EtykietyZPL::zplProbny(u.etykieta), &blad)) {
        stan_->SetLabel(u.wysylka.tryb == WysylkaZPL::Tryb::Plik ? TR("✓ Zapisano etykietę próbną do pliku.")
                                                                : TR("✓ Wysłano etykietę próbną."));
        return true;
    }
    stan_->SetLabel(TR("✗ ") + TekstZUTF8(blad));
    Layout();
    return false;
}
