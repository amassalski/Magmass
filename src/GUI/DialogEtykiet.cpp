#include "DialogEtykiet.h"
#include "../Narzedzia/Konfiguracja.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"
#include "DialogZebra.h"

#include <wx/filedlg.h>
#include <algorithm>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/sizer.h>

DialogEtykiet::DialogEtykiet(wxWindow* parent, const std::vector<Element>& elementy, long long idWybranego)
    : wxDialog(parent, wxID_ANY, TR("Drukowanie etykiet"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      elementy_(elementy), idWybranego_(idWybranego) {
    wxBoxSizer* glowny = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer* lewa = new wxBoxSizer(wxVERTICAL);
    zebra_ = UstawieniaZebry::wczytaj();
    chSposob = new wxChoice(this, wxID_ANY);
    chSposob->Append(TR("Drukarka systemowa (arkusze i rolki)"));
    chSposob->Append(TR("Zebra ZPL (ZD421 i podobne)"));
    chSposob->SetSelection(zebra_.uzywaj ? 1 : 0);
    PomocGUI::Dodaj(chSposob, "etykiety.sposob");
    lewa->Add(new wxStaticText(this, wxID_ANY, TR("Sposób drukowania")), 0, wxBOTTOM, 2);
    lewa->Add(chSposob, 0, wxEXPAND | wxBOTTOM, 8);
    wxStaticBoxSizer* skad = new wxStaticBoxSizer(wxVERTICAL, this, TR("Które elementy"));
    rbZaznaczony = new wxRadioButton(this, wxID_ANY, TR("Tylko zaznaczony element"), wxDefaultPosition, wxDefaultSize,
                                     wxRB_GROUP);
    rbWszystkie = new wxRadioButton(this, wxID_ANY,
                                    wxString::Format(TR("Wszystkie elementy z listy (%zu)"), elementy_.size()));
    skad->Add(rbZaznaczony, 0, wxALL, 4);
    skad->Add(rbWszystkie, 0, wxALL, 4);
    lewa->Add(skad, 0, wxEXPAND | wxBOTTOM, 8);

    wxFlexGridSizer* siatka = new wxFlexGridSizer(2, 6, 8);
    auto dodaj = [&](const wxString& t, wxWindow* k) {
        siatka->Add(new wxStaticText(this, wxID_ANY, t), 0, wxALIGN_CENTER_VERTICAL);
        siatka->Add(k, 0, wxEXPAND);
    };
    scKopie = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, 1, 500, 1);
    chFormat = new wxChoice(this, wxID_ANY);
    for (const FormatEtykiet& f : DrukujEtykiety::formatyWbudowane()) chFormat->Append(TekstZUTF8(f.nazwa));
    chFormat->SetSelection(0);
    scStart = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, 1, 100, 1);
    scX = new wxSpinCtrlDouble(this, wxID_ANY, "0", wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, -20, 20, 0, 0.5);
    scY = new wxSpinCtrlDouble(this, wxID_ANY, "0", wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, -20, 20, 0, 0.5);
    cbRamki = new wxCheckBox(this, wxID_ANY, TR("Ramki etykiet (wydruk próbny)"));
    dodaj(TR("Kopii każdej etykiety"), scKopie);
    dodaj(TR("Format arkusza"), chFormat);
    dodaj(TR("Zacznij od etykiety nr"), scStart);
    dodaj(TR("Przesunięcie poziome [mm]"), scX);
    dodaj(TR("Przesunięcie pionowe [mm]"), scY);
    lewa->Add(siatka, 0, wxEXPAND | wxBOTTOM, 8);
    lewa->Add(cbRamki, 0, wxBOTTOM, 8);
    lbInfo = new wxStaticText(this, wxID_ANY, wxEmptyString);
    lewa->Add(lbInfo, 0, wxBOTTOM, 10);

    wxBoxSizer* przyciski = new wxBoxSizer(wxVERTICAL);
    wxButton* bPodglad = bPodglad_ = new wxButton(this, wxID_ANY, TR("Podgląd wydruku..."));
    wxButton* bDrukuj = bDrukuj_ = new wxButton(this, wxID_ANY, TR("Drukuj..."));
    wxButton* bPng = bPng_ = new wxButton(this, wxID_ANY, TR("Zapisz jako PNG..."));
    bUstZebry_ = new wxButton(this, wxID_ANY, TR("Ustawienia Zebry..."));
    bZapiszZpl_ = new wxButton(this, wxID_ANY, TR("Zapisz ZPL do pliku..."));
    przyciski->Add(bPodglad, 0, wxEXPAND | wxBOTTOM, 4);
    przyciski->Add(bDrukuj, 0, wxEXPAND | wxBOTTOM, 4);
    przyciski->Add(bPng, 0, wxEXPAND | wxBOTTOM, 4);
    przyciski->Add(bUstZebry_, 0, wxEXPAND | wxBOTTOM, 4);
    przyciski->Add(bZapiszZpl_, 0, wxEXPAND | wxBOTTOM, 10);
    przyciski->Add(new wxButton(this, wxID_CANCEL, TR("Zamknij")), 0, wxEXPAND);
    lewa->Add(przyciski, 0, wxEXPAND);
    glowny->Add(lewa, 0, wxEXPAND | wxALL, 12);

    podglad = new wxStaticBitmap(this, wxID_ANY, wxBitmap(300, 420));
    glowny->Add(podglad, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 12);
    SetSizerAndFit(glowny);
    SetEscapeId(wxID_CANCEL);

    if (idWybranego_ == 0 || std::none_of(elementy_.begin(), elementy_.end(),
                                          [&](const Element& e) { return e.id == idWybranego_; })) {
        rbZaznaczony->Enable(false);
        rbWszystkie->SetValue(true);
    }

    auto odswiez = [this](wxCommandEvent&) { Odswiez(); };
    for (wxControl* c : std::initializer_list<wxControl*>{rbZaznaczony, rbWszystkie, chFormat, cbRamki})
        c->Bind(c == chFormat ? wxEVT_CHOICE : (c == cbRamki ? wxEVT_CHECKBOX : wxEVT_RADIOBUTTON), odswiez);
    for (wxSpinCtrl* s : {scKopie, scStart}) s->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { Odswiez(); });
    for (wxSpinCtrlDouble* s : {scX, scY}) s->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { Odswiez(); });
    bPodglad->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Podglad(); });
    bDrukuj->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Drukuj(); });
    bPng->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ZapiszPNGOknem(); });
    bUstZebry_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { UstawieniaZebryOknem(); });
    bZapiszZpl_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ZapiszZPLOknem(); });
    chSposob->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
        zebra_.uzywaj = chSposob->GetSelection() == 1;
        zebra_.zapisz();
        OdswiezAktywne();
        Odswiez();
    });
    OdswiezAktywne();
    Odswiez();
}

bool DialogEtykiet::ZebraAktywna() const { return chSposob->GetSelection() == 1; }

void DialogEtykiet::UstawZebra(bool tak) {
    chSposob->SetSelection(tak ? 1 : 0);
    zebra_.uzywaj = tak;
    OdswiezAktywne();
    Odswiez();
}

void DialogEtykiet::OdswiezAktywne() {
    const bool z = ZebraAktywna();
    chFormat->Enable(!z);
    scStart->Enable(!z);
    scX->Enable(!z);
    scY->Enable(!z);
    cbRamki->Enable(!z);
    bPodglad_->Enable(!z);
    bPng_->Enable(!z);
    bUstZebry_->Enable(z);
    bZapiszZpl_->Enable(z);
    bDrukuj_->SetLabel(z ? TR("Drukuj na Zebrze") : TR("Drukuj..."));
}

std::vector<EtykietyZPL::Etykieta> DialogEtykiet::EtykietyZebry() const {
    std::vector<EtykietyZPL::Etykieta> wynik;
    for (const EtykietaDoDruku& et : Etykiety()) {
        const Element& e = et.element;
        EtykietyZPL::Etykieta z;
        z.symbol = e.symbol;
        std::string druga = e.wartosc;
        if (!e.tolerancja.empty()) druga += (druga.empty() ? "" : " ") + e.tolerancja;
        if (!e.obudowa.empty()) druga += (druga.empty() ? "" : "  ") + e.obudowa;
        z.drugaLinia = druga;
        z.miejsce = e.miejsceTekst();
        z.producent = e.producent;
        z.opis = e.opis;
        z.kod = et.kod;
        z.typ = KodKreskowy::typZNazwy(Konfiguracja::pobierz().pobierzUstawieniaAplikacji().typKoduEAN);
        wynik.push_back(z);
    }
    return wynik;
}

std::string DialogEtykiet::PoleceniaZPL() const { return EtykietyZPL::zplWielu(EtykietyZebry(), zebra_.etykieta); }

bool DialogEtykiet::DrukujZebra(bool cicho) {
    const std::string dane = PoleceniaZPL();
    if (dane.empty()) return false;
    std::string blad;
    {
        wxBusyCursor zajety;
        if (WysylkaZPL::wyslij(zebra_.wysylka, dane, &blad)) {
            lbInfo->SetLabel(zebra_.wysylka.tryb == WysylkaZPL::Tryb::Plik ? TR("✓ Zapisano polecenia ZPL do pliku.")
                                                                           : TR("✓ Wysłano do drukarki."));
            return true;
        }
    }
    lbInfo->SetLabel(TR("✗ ") + TekstZUTF8(blad));
    if (!cicho) wxMessageBox(TekstZUTF8(blad), TR("Etykiety: drukarka Zebra"), wxOK | wxICON_ERROR, this);
    return false;
}

void DialogEtykiet::UstawieniaZebryOknem() {
    DialogZebra dlg(this, zebra_);
    if (dlg.ShowModal() != wxID_OK) return;
    const bool uzywaj = zebra_.uzywaj;
    zebra_ = dlg.Ustawienia();
    zebra_.uzywaj = uzywaj;
    zebra_.zapisz();
    Odswiez();
}

void DialogEtykiet::ZapiszZPLOknem() {
    const std::string dane = PoleceniaZPL();
    if (dane.empty()) return;
    wxFileDialog dlg(this, TR("Zapisz polecenia ZPL"), wxEmptyString, "etykiety.zpl", TR("Pliki ZPL (*.zpl)|*.zpl"),
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    std::string blad;
    if (WysylkaZPL::doPliku(dlg.GetPath().utf8_string(), dane, &blad))
        lbInfo->SetLabel(TR("✓ Zapisano plik ZPL."));
    else
        wxMessageBox(TekstZUTF8(blad), TR("Etykiety"), wxOK | wxICON_ERROR, this);
}

void DialogEtykiet::WybierzTylkoZaznaczony(bool tak) {
    if (tak && rbZaznaczony->IsEnabled()) rbZaznaczony->SetValue(true);
    else rbWszystkie->SetValue(true);
    Odswiez();
}

std::vector<EtykietaDoDruku> DialogEtykiet::Etykiety() const {
    const UstawieniaBazowe us = Konfiguracja::pobierz().pobierzUstawieniaAplikacji();
    const KodKreskowy::Typ typ = KodKreskowy::typZNazwy(us.typKoduEAN);
    std::vector<EtykietaDoDruku> wynik;
    for (const Element& e : elementy_) {
        if (rbZaznaczony->GetValue() && e.id != idWybranego_) continue;
        EtykietaDoDruku et;
        et.element = e;
        et.kod = KodKreskowy::tekstKoduElementu(typ == KodKreskowy::Typ::EAN8 ? typ : KodKreskowy::Typ::EAN13, e.id,
                                                us.prefixEAN);
        for (int i = 0; i < scKopie->GetValue(); ++i) wynik.push_back(et);
    }
    return wynik;
}

UstawieniaDruku DialogEtykiet::Ustawienia() const {
    UstawieniaDruku u;
    const int f = chFormat->GetSelection();
    const auto& formaty = DrukujEtykiety::formatyWbudowane();
    u.format = formaty[static_cast<size_t>(std::max(0, std::min(f, static_cast<int>(formaty.size()) - 1)))];
    u.typKodu = KodKreskowy::typZNazwy(Konfiguracja::pobierz().pobierzUstawieniaAplikacji().typKoduEAN);
    u.pominiete = std::max(0, scStart->GetValue() - 1);
    u.przesuniecieX = scX->GetValue();
    u.przesuniecieY = scY->GetValue();
    u.ramki = cbRamki->GetValue();
    return u;
}

void DialogEtykiet::Odswiez() {
    const UstawieniaDruku u = Ustawienia();
    scStart->SetRange(1, std::max(1, u.format.naStrone()));
    const auto et = Etykiety();
    if (ZebraAktywna()) {
        lbInfo->SetLabel(wxString::Format(TR("Etykiet: %zu (%.0f x %.0f mm, %d dpi)"), et.size(), zebra_.etykieta.szerMm,
                                          zebra_.etykieta.wysMm, zebra_.etykieta.dpi));
        wxBitmap bmp;
        const auto ez = EtykietyZebry();
        const EtykietyZPL::Etykieta z = ez.empty() ? EtykietyZPL::Etykieta() : ez[0];
        bmp = RenderujUkladZPL(EtykietyZPL::zbudujUklad(z, zebra_.etykieta));
        wxImage img = bmp.ConvertToImage();
        const double s = std::min(300.0 / img.GetWidth(), 420.0 / img.GetHeight());
        img = img.Scale(std::max(1, static_cast<int>(img.GetWidth() * s)), std::max(1, static_cast<int>(img.GetHeight() * s)),
                        wxIMAGE_QUALITY_BILINEAR);
        podglad->SetBitmap(wxBitmap(img));
        podglad->SetMinSize(wxSize(300, 420));
        podglad->Refresh();
        Layout();
        return;
    }
    const int stron = DrukujEtykiety::liczbaStron(u.format, u.pominiete, static_cast<int>(et.size()));
    lbInfo->SetLabel(wxString::Format(TR("Etykiet: %zu, arkuszy: %d"), et.size(), stron));
    wxBitmap bmp = RenderujStrone(et, u, 0, 60);
    // dopasuj do pola podglądu (maks. 300 x 420)
    wxImage img = bmp.ConvertToImage();
    const double s = std::min(300.0 / img.GetWidth(), 420.0 / img.GetHeight());
    if (s < 1.0 || s > 1.0) img = img.Scale(std::max(1, static_cast<int>(img.GetWidth() * s)), std::max(1, static_cast<int>(img.GetHeight() * s)), wxIMAGE_QUALITY_BILINEAR);
    podglad->SetBitmap(wxBitmap(img));
    podglad->SetMinSize(wxSize(300, 420));
    podglad->Refresh();
    Layout();
}

int DialogEtykiet::ZapiszPNG(const wxString& sciezkaBazowa, double dpi) const {
    const UstawieniaDruku u = Ustawienia();
    const auto et = Etykiety();
    const int stron = DrukujEtykiety::liczbaStron(u.format, u.pominiete, static_cast<int>(et.size()));
    wxFileName nazwa(sciezkaBazowa);
    int zapisane = 0;
    for (int s = 0; s < stron; ++s) {
        wxFileName f = nazwa;
        if (stron > 1) f.SetName(nazwa.GetName() + wxString::Format("_%d", s + 1));
        f.SetExt("png");
        const wxImage img = RenderujStrone(et, u, s, dpi).ConvertToImage();
        if (img.SaveFile(f.GetFullPath(), wxBITMAP_TYPE_PNG)) ++zapisane;
    }
    return zapisane;
}

void DialogEtykiet::ZapiszPNGOknem() {
    if (Etykiety().empty()) return;
    wxFileDialog dlg(this, TR("Zapisz etykiety jako PNG"), wxEmptyString, "etykiety.png", TR("Obrazy PNG (*.png)|*.png"),
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    const int n = ZapiszPNG(dlg.GetPath());
    wxMessageBox(wxString::Format(TR("Zapisano obrazów: %d"), n), TR("Etykiety"), wxOK | wxICON_INFORMATION, this);
}

void DialogEtykiet::Podglad() {
    const auto et = Etykiety();
    if (et.empty()) return;
    wxPrintDialogData dane;
    wxPrintPreview* pp = new wxPrintPreview(new WydrukEtykiet(et, Ustawienia()), new WydrukEtykiet(et, Ustawienia()), &dane);
    if (!pp->IsOk()) {
        delete pp;
        wxMessageBox(TR("Nie można utworzyć podglądu wydruku (brak skonfigurowanej drukarki?)."), TR("Etykiety"),
                     wxOK | wxICON_WARNING, this);
        return;
    }
    wxPreviewFrame* ramka = new wxPreviewFrame(pp, this, TR("Podgląd etykiet"), wxDefaultPosition, wxSize(800, 900));
    ramka->Centre(wxBOTH);
    ramka->InitializeWithModality(wxPreviewFrame_AppModal);
    ramka->Show();
}

void DialogEtykiet::Drukuj() {
    if (ZebraAktywna()) {
        DrukujZebra();
        return;
    }
    const auto et = Etykiety();
    if (et.empty()) return;
    wxPrintDialogData dane;
    wxPrinter drukarka(&dane);
    WydrukEtykiet wydruk(et, Ustawienia());
    if (!drukarka.Print(this, &wydruk, true) && wxPrinter::GetLastError() == wxPRINTER_ERROR) {
        wxMessageBox(TR("Drukowanie nie powiodło się. Sprawdź ustawienia drukarki."), TR("Etykiety"),
                     wxOK | wxICON_ERROR, this);
    }
}
