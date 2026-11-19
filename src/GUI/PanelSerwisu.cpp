#include "PoleDaty.h"
#include "PanelSerwisu.h"

#include "DialogProjektu.h"
#include "Ikony.h"
#include "UstawieniaOkna.h"
#include "ZdjeciaElementow.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <wx/combobox.h>
#include <wx/dateevt.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/listbox.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/wrapsizer.h>

#include <algorithm>

namespace {

wxString Przytnij(const wxString& s) { return s.Strip(wxString::both); }
std::string Iso(const wxDateTime& d) { return d.IsValid() ? d.FormatISODate().utf8_string() : std::string(); }

wxString NazwaPliku(const std::string& sciezka) { return wxFileName(TekstZUTF8(sciezka)).GetFullName(); }

/// Okno jednej części: nazwa, ilość, numery seryjne zdjętego i zamontowanego elementu, uwagi
class DialogCzesci : public wxDialog {
public:
    DialogCzesci(wxWindow* parent, const CzescSerwisowa& c, LogikaMagazynu& magazyn, const wxString& urzadzenie)
        : wxDialog(parent, wxID_ANY, TR("Część"), wxDefaultPosition, wxSize(520, 430), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), czesc_(c), magazyn_(magazyn) {
        wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
        wxFlexGridSizer* s = new wxFlexGridSizer(2, 6, 8);
        s->AddGrowableCol(1);
        auto dodaj = [&](const wxString& et, wxWindow* p) {
            s->Add(new wxStaticText(this, wxID_ANY, et), 0, wxALIGN_CENTER_VERTICAL);
            s->Add(p, 1, wxEXPAND);
        };
        tcNazwa_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(c.nazwa));
        tcNazwa_->SetHint(TR("np. zasilacz 150 W, moduł SFP, wentylator"));
        dodaj(TR("Nazwa *"), tcNazwa_);
        tcIlosc_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(liczbaNaTekstPolski(c.ilosc, 3, 0)));
        dodaj(TR("Ilość"), tcIlosc_);
        tcStary_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(c.numerStary));
        tcStary_->SetHint(TR("numer seryjny elementu zdjętego"));
        dodaj(TR("Numer zdjętego"), tcStary_);
        tcNowy_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(c.numerNowy));
        tcNowy_->SetHint(TR("numer seryjny elementu zamontowanego"));
        dodaj(TR("Numer zamontowanego"), tcNowy_);
        tcUwagi_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(c.uwagi));
        dodaj(TR("Uwagi"), tcUwagi_);
        g->Add(s, 0, wxEXPAND | wxALL, 10);
        wxStaticBoxSizer* mag = new wxStaticBoxSizer(wxVERTICAL, this, TR("Z magazynu"));
        wxBoxSizer* wm = new wxBoxSizer(wxHORIZONTAL);
        bWybierz_ = new wxButton(this, wxID_ANY, TR("Wybierz z magazynu..."));
        bOdlacz_ = new wxButton(this, wxID_ANY, TR("Odłącz"));
        stMag_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
        wm->Add(bWybierz_, 0, wxRIGHT, 6);
        wm->Add(bOdlacz_, 0, wxRIGHT, 10);
        wm->Add(stMag_, 1, wxALIGN_CENTER_VERTICAL);
        mag->Add(wm, 0, wxEXPAND | wxALL, 4);
        cbPobierz_ = new wxCheckBox(this, wxID_ANY, TR("Zdejmij ilość ze stanu magazynu przy zapisie wpisu"));
        cbPobierz_->SetValue(true);
        mag->Add(cbPobierz_, 0, wxALL, 4);
        g->Add(mag, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        g->AddStretchSpacer(1);
        g->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
        SetSizer(g);
        PomocGUI::Dodaj(bWybierz_, "serwis.czesc_magazyn");
        PomocGUI::Dodaj(cbPobierz_, "serwis.czesc_pobierz");
        urzadzenie_ = urzadzenie;
        bWybierz_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Wybierz(); });
        bOdlacz_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { czesc_.idElementu.reset(); czesc_.symbolElementu.clear(); OdswiezMagazyn(); });
        OdswiezMagazyn();
        PomocGUI::Dodaj(tcStary_, "serwis.czesc_numery");
        PomocGUI::Dodaj(tcNowy_, "serwis.czesc_numery");
        Bind(wxEVT_BUTTON, &DialogCzesci::NaOK, this, wxID_OK);
    }
    const CzescSerwisowa& Wynik() const { return czesc_; }

private:
    CzescSerwisowa czesc_;
    LogikaMagazynu& magazyn_;
    wxString urzadzenie_;
    wxButton *bWybierz_, *bOdlacz_;
    wxStaticText* stMag_;
    wxCheckBox* cbPobierz_;
    wxTextCtrl *tcNazwa_, *tcIlosc_, *tcStary_, *tcNowy_, *tcUwagi_;
    static std::string T(wxTextCtrl* c) { return Przytnij(c->GetValue()).utf8_string(); }
    void OdswiezMagazyn() {
        const bool powiazana = czesc_.idElementu.has_value();
        if (czesc_.pobrano) stMag_->SetLabel(TR("Pobrano ze stanu: ") + TekstZUTF8(czesc_.symbolElementu));
        else if (powiazana) stMag_->SetLabel(TR("Element: ") + TekstZUTF8(czesc_.symbolElementu));
        else stMag_->SetLabel(TR("(część spoza magazynu)"));
        bWybierz_->Enable(!czesc_.pobrano);
        bOdlacz_->Enable(powiazana && !czesc_.pobrano);
        cbPobierz_->Show(powiazana && !czesc_.pobrano);
        tcIlosc_->Enable(!czesc_.pobrano);
        Layout();
    }
    void Wybierz() {
        DialogWyboruElementu dlg(this, magazyn_, Przytnij(tcNazwa_->GetValue()));
        if (dlg.ShowModal() != wxID_OK || dlg.WybraneId() == 0) return;
        const auto el = magazyn_.pobierzElement(dlg.WybraneId());
        if (!el) { wxMessageBox(TekstZUTF8(magazyn_.pobierzBlad()), TR("Serwis"), wxOK | wxICON_WARNING, this); return; }
        const Element& e = *el;
        czesc_.idElementu = e.id;
        czesc_.symbolElementu = e.symbol;
        wxString nazwa = TekstZUTF8(e.symbol);
        if (!e.wartosc.empty()) nazwa += " " + TekstZUTF8(e.wartosc);
        if (!e.obudowa.empty()) nazwa += " " + TekstZUTF8(e.obudowa);
        tcNazwa_->SetValue(nazwa);
        OdswiezMagazyn();
    }
    void NaOK(wxCommandEvent& e) {
        double ilosc = 1;
        if (T(tcNazwa_).empty()) {
            wxMessageBox(TR("Podaj nazwę części."), TR("Serwis"), wxOK | wxICON_WARNING, this);
            return;
        }
        if (!T(tcIlosc_).empty() && (!tekstNaLiczbe(T(tcIlosc_), ilosc) || ilosc <= 0)) {
            wxMessageBox(TR("Ilość musi być liczbą dodatnią."), TR("Serwis"), wxOK | wxICON_WARNING, this);
            return;
        }
        if (czesc_.idElementu && !czesc_.pobrano && cbPobierz_->GetValue() && ilosc != static_cast<int>(ilosc)) {
            wxMessageBox(TR("Część z magazynu musi mieć całkowitą ilość sztuk."), TR("Serwis"), wxOK | wxICON_WARNING, this);
            return;
        }
        czesc_.nazwa = T(tcNazwa_);
        czesc_.ilosc = ilosc;
        czesc_.pobierz = czesc_.idElementu && !czesc_.pobrano && cbPobierz_->GetValue();
        czesc_.numerStary = T(tcStary_);
        czesc_.numerNowy = T(tcNowy_);
        czesc_.uwagi = T(tcUwagi_);
        e.Skip();
    }
};

/// Okno wpisu serwisowego: dane urządzenia, opis usterki i czynności, lista części, zdjęcia (z kamery lub z pliku)
class DialogEdycjiSerwisu : public wxDialog {
public:
    DialogEdycjiSerwisu(wxWindow* parent, const WpisSerwisowy& w, LogikaSerwisu& logika, LogikaMagazynu& magazyn)
        : wxDialog(parent, wxID_ANY, w.id ? TR("Zmień wpis serwisowy") : TR("Nowy wpis serwisowy"), wxDefaultPosition, wxSize(760, 760),
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
          wpis_(w), magazyn_(magazyn), zdjecia_(w.zdjecia) {
        wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
        wxFlexGridSizer* s = new wxFlexGridSizer(4, 6, 8);
        s->AddGrowableCol(1);
        s->AddGrowableCol(3);
        auto dodaj = [&](const wxString& et, wxWindow* p) {
            s->Add(new wxStaticText(this, wxID_ANY, et), 0, wxALIGN_CENTER_VERTICAL);
            s->Add(p, 1, wxEXPAND);
        };
        dpData_ = NowePoleDaty(this, wxDateTime::Today(), wxDP_DROPDOWN);
        wxDateTime d;
        if (!w.data.empty() && d.ParseISODate(TekstZUTF8(w.data))) dpData_->SetValue(d);
        dodaj(TR("Data serwisu"), dpData_);
        tcZgloszenie_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(w.zgloszenie));
        tcZgloszenie_->SetHint(TR("np. numer w systemie zgłoszeń"));
        dodaj(TR("Nr zgłoszenia"), tcZgloszenie_);
        cbUrzadzenie_ = new wxComboBox(this, wxID_ANY, TekstZUTF8(w.urzadzenie));
        for (const std::string& u : logika.urzadzenia()) cbUrzadzenie_->Append(TekstZUTF8(u));
        dodaj(TR("Urządzenie *"), cbUrzadzenie_);
        tcSeryjny_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(w.numerSeryjny));
        dodaj(TR("Numer seryjny"), tcSeryjny_);
        tcProducent_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(w.producent));
        dodaj(TR("Producent"), tcProducent_);
        tcModel_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(w.model));
        dodaj(TR("Model"), tcModel_);
        cbLokalizacja_ = new wxComboBox(this, wxID_ANY, TekstZUTF8(w.lokalizacja));
        for (const std::string& u : logika.lokalizacje()) cbLokalizacja_->Append(TekstZUTF8(u));
        dodaj(TR("Lokalizacja"), cbLokalizacja_);
        cbTechnik_ = new wxComboBox(this, wxID_ANY, TekstZUTF8(w.technik));
        for (const std::string& u : logika.technicy()) cbTechnik_->Append(TekstZUTF8(u));
        dodaj(TR("Technik"), cbTechnik_);
        chRodzaj_ = new wxChoice(this, wxID_ANY);
        for (const std::string& r : LogikaSerwisu::rodzaje()) chRodzaj_->Append(TekstZUTF8(r));
        chRodzaj_->SetSelection(Indeks(LogikaSerwisu::rodzaje(), w.rodzaj));
        dodaj(TR("Rodzaj"), chRodzaj_);
        chStatus_ = new wxChoice(this, wxID_ANY);
        for (const std::string& r : LogikaSerwisu::statusy()) chStatus_->Append(TekstZUTF8(r));
        chStatus_->SetSelection(Indeks(LogikaSerwisu::statusy(), w.status));
        dodaj(TR("Status"), chStatus_);
        tcCzas_ = new wxTextCtrl(this, wxID_ANY, w.czasMin ? wxString::Format("%d", *w.czasMin) : wxString());
        tcCzas_->SetHint(TR("minuty"));
        dodaj(TR("Czas pracy [min]"), tcCzas_);
        g->Add(s, 0, wxEXPAND | wxALL, 10);

        wxBoxSizer* opisy = new wxBoxSizer(wxHORIZONTAL);
        auto pole = [&](const wxString& et, const std::string& v, wxTextCtrl*& wsk) {
            wxBoxSizer* b = new wxBoxSizer(wxVERTICAL);
            b->Add(new wxStaticText(this, wxID_ANY, et), 0, wxBOTTOM, 2);
            wsk = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(v), wxDefaultPosition, wxSize(-1, 70), wxTE_MULTILINE);
            b->Add(wsk, 1, wxEXPAND);
            opisy->Add(b, 1, wxEXPAND | wxRIGHT, 6);
        };
        pole(TR("Usterka / powód wizyty"), w.usterka, tcUsterka_);
        pole(TR("Wykonane czynności"), w.wykonano, tcWykonano_);
        pole(TR("Uwagi"), w.uwagi, tcUwagi_);
        g->Add(opisy, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

        g->Add(new wxStaticText(this, wxID_ANY, TR("Wymienione i zamontowane części:")), 0, wxLEFT | wxRIGHT | wxTOP, 10);
        czesciWidok_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 110), wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_THEME);
        czesciWidok_->InsertColumn(0, TR("Część"), wxLIST_FORMAT_LEFT, 220);
        czesciWidok_->InsertColumn(1, TR("Ilość"), wxLIST_FORMAT_RIGHT, 60);
        czesciWidok_->InsertColumn(2, TR("Zdjęty nr"), wxLIST_FORMAT_LEFT, 130);
        czesciWidok_->InsertColumn(3, TR("Zamontowany nr"), wxLIST_FORMAT_LEFT, 130);
        czesciWidok_->InsertColumn(4, TR("Uwagi"), wxLIST_FORMAT_LEFT, 150);
        czesciWidok_->InsertColumn(5, TR("Magazyn"), wxLIST_FORMAT_LEFT, 110);
        PomocGUI::Dodaj(czesciWidok_, "serwis.czesci");
        g->Add(czesciWidok_, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
        wxBoxSizer* bc = new wxBoxSizer(wxHORIZONTAL);
        wxButton* bDodajC = new wxButton(this, wxID_ANY, TR("Dodaj część..."));
        wxButton* bZmienC = new wxButton(this, wxID_ANY, TR("Zmień..."));
        wxButton* bUsunC = new wxButton(this, wxID_ANY, TR("Usuń część"));
        for (wxButton* b : {bDodajC, bZmienC, bUsunC}) bc->Add(b, 0, wxRIGHT, 6);
        g->Add(bc, 0, wxALL, 10);

        g->Add(new wxStaticText(this, wxID_ANY, TR("Zdjęcia:")), 0, wxLEFT | wxRIGHT, 10);
        zdjeciaLista_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 70));
        PomocGUI::Dodaj(zdjeciaLista_, "serwis.zdjecia");
        g->Add(zdjeciaLista_, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        wxBoxSizer* bz = new wxBoxSizer(wxHORIZONTAL);
        wxButton* bKamera = new wxButton(this, wxID_ANY, TR("Z kamery..."));
        wxButton* bPlik = new wxButton(this, wxID_ANY, TR("Z pliku..."));
        wxButton* bUsunZ = new wxButton(this, wxID_ANY, TR("Usuń zdjęcie"));
        for (wxButton* b : {bKamera, bPlik, bUsunZ}) bz->Add(b, 0, wxRIGHT, 6);
        g->Add(bz, 0, wxALL, 10);
        g->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
        SetSizer(g);

        PomocGUI::Dodaj(cbLokalizacja_, "serwis.lokalizacja");
        PomocGUI::Dodaj(chRodzaj_, "serwis.rodzaj");
        PomocGUI::Dodaj(chStatus_, "serwis.status");
        PomocGUI::Dodaj(tcZgloszenie_, "serwis.zgloszenie");
        PomocGUI::Dodaj(tcSeryjny_, "serwis.seryjny");

        czesci_ = w.czesci;
        OdswiezCzesci();
        OdswiezZdjecia();
        bDodajC->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            DialogCzesci dlg(this, CzescSerwisowa(), magazyn_, cbUrzadzenie_->GetValue());
            if (dlg.ShowModal() == wxID_OK) { czesci_.push_back(dlg.Wynik()); OdswiezCzesci(); }
        });
        auto zmien = [this](wxCommandEvent&) { ZmienCzesc(); };
        bZmienC->Bind(wxEVT_BUTTON, zmien);
        czesciWidok_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) { ZmienCzesc(); });
        bUsunC->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            const long i = czesciWidok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            if (i >= 0 && static_cast<size_t>(i) < czesci_.size()) { czesci_.erase(czesci_.begin() + i); OdswiezCzesci(); }
        });
        bKamera->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            const wxString p = ZdjeciaElementow::ZKamery(this, cbUrzadzenie_->GetValue());
            if (!p.empty()) DodajZdjecie(p);
        });
        bPlik->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            const wxString p = ZdjeciaElementow::ZPliku(this);
            if (!p.empty()) DodajZdjecie(p);
        });
        bUsunZ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            const int i = zdjeciaLista_->GetSelection();
            if (i == wxNOT_FOUND || static_cast<size_t>(i) >= zdjecia_.size()) return;
            if (zdjecia_[static_cast<size_t>(i)].id) usuniete_.push_back(zdjecia_[static_cast<size_t>(i)].id);
            zdjecia_.erase(zdjecia_.begin() + i);
            OdswiezZdjecia();
        });
        Bind(wxEVT_BUTTON, &DialogEdycjiSerwisu::NaOK, this, wxID_OK);
    }
    const WpisSerwisowy& Wynik() const { return wpis_; }
    const std::vector<ZdjecieSerwisu>& Zdjecia() const { return zdjecia_; }
    const std::vector<long long>& Usuniete() const { return usuniete_; }

private:
    WpisSerwisowy wpis_;
    LogikaMagazynu& magazyn_;
    std::vector<CzescSerwisowa> czesci_;
    std::vector<ZdjecieSerwisu> zdjecia_;
    std::vector<long long> usuniete_;
    wxTextCtrl *tcZgloszenie_, *tcSeryjny_, *tcProducent_, *tcModel_, *tcCzas_, *tcUsterka_, *tcWykonano_, *tcUwagi_;
    wxComboBox *cbUrzadzenie_, *cbLokalizacja_, *cbTechnik_;
    wxChoice *chRodzaj_, *chStatus_;
    PoleDatyTyp* dpData_;
    wxListCtrl* czesciWidok_;
    wxListBox* zdjeciaLista_;

    static int Indeks(const std::vector<std::string>& v, const std::string& x) {
        const auto it = std::find(v.begin(), v.end(), x);
        return it == v.end() ? 0 : static_cast<int>(it - v.begin());
    }
    static std::string T(wxTextCtrl* c) { return Przytnij(c->GetValue()).utf8_string(); }

    void OdswiezCzesci() {
        czesciWidok_->DeleteAllItems();
        for (size_t i = 0; i < czesci_.size(); ++i) {
            const CzescSerwisowa& c = czesci_[i];
            const long w = czesciWidok_->InsertItem(static_cast<long>(i), TekstZUTF8(c.nazwa));
            czesciWidok_->SetItem(w, 1, TekstZUTF8(liczbaNaTekstPolski(c.ilosc, 3, 0)));
            czesciWidok_->SetItem(w, 2, TekstZUTF8(c.numerStary));
            czesciWidok_->SetItem(w, 3, TekstZUTF8(c.numerNowy));
            czesciWidok_->SetItem(w, 4, TekstZUTF8(c.uwagi));
            czesciWidok_->SetItem(w, 5, c.pobrano ? TR("pobrano ") + TekstZUTF8(c.symbolElementu) : c.pierwszePobranie() ? TR("do pobrania") : c.idElementu ? TR("powiązano") : wxString());
        }
    }
    void OdswiezZdjecia() {
        zdjeciaLista_->Clear();
        for (const ZdjecieSerwisu& z : zdjecia_) zdjeciaLista_->Append(NazwaPliku(z.sciezka));
    }
    void ZmienCzesc() {
        const long i = czesciWidok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (i < 0 || static_cast<size_t>(i) >= czesci_.size()) return;
        DialogCzesci dlg(this, czesci_[static_cast<size_t>(i)], magazyn_, cbUrzadzenie_->GetValue());
        if (dlg.ShowModal() != wxID_OK) return;
        czesci_[static_cast<size_t>(i)] = dlg.Wynik();
        OdswiezCzesci();
    }
    void DodajZdjecie(const wxString& sciezka) {
        ZdjecieSerwisu z;
        z.sciezka = sciezka.utf8_string();
        zdjecia_.push_back(z);
        OdswiezZdjecia();
    }
    void NaOK(wxCommandEvent& e) {
        if (Przytnij(cbUrzadzenie_->GetValue()).empty()) {
            wxMessageBox(TR("Podaj nazwę urządzenia."), TR("Serwis"), wxOK | wxICON_WARNING, this);
            return;
        }
        std::optional<int> czas;
        if (!T(tcCzas_).empty()) {
            double v = 0;
            if (!tekstNaLiczbe(T(tcCzas_), v) || v < 0 || v > 100000) {
                wxMessageBox(TR("Czas pracy musi być liczbą minut (np. 90)."), TR("Serwis"), wxOK | wxICON_WARNING, this);
                return;
            }
            czas = static_cast<int>(v + 0.5);
        }
        wpis_.data = Iso(dpData_->GetValue());
        wpis_.urzadzenie = Przytnij(cbUrzadzenie_->GetValue()).utf8_string();
        wpis_.producent = T(tcProducent_);
        wpis_.model = T(tcModel_);
        wpis_.numerSeryjny = T(tcSeryjny_);
        wpis_.lokalizacja = Przytnij(cbLokalizacja_->GetValue()).utf8_string();
        wpis_.technik = Przytnij(cbTechnik_->GetValue()).utf8_string();
        wpis_.zgloszenie = T(tcZgloszenie_);
        wpis_.rodzaj = LogikaSerwisu::rodzaje()[static_cast<size_t>(std::max(0, chRodzaj_->GetSelection()))];
        wpis_.status = LogikaSerwisu::statusy()[static_cast<size_t>(std::max(0, chStatus_->GetSelection()))];
        wpis_.czasMin = czas;
        wpis_.usterka = T(tcUsterka_);
        wpis_.wykonano = T(tcWykonano_);
        wpis_.uwagi = T(tcUwagi_);
        wpis_.czesci = czesci_;
        e.Skip();
    }
};

}  // namespace

PanelSerwisu::PanelSerwisu(wxWindow* parent, LogikaSerwisu& logika, LogikaMagazynu& magazyn) : wxPanel(parent), logika_(logika), magazyn_(magazyn) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxWrapSizer* gora = new wxWrapSizer(wxHORIZONTAL);
    szukaj_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(260, -1));
    szukaj_->SetDescriptiveText(TR("Szukaj urządzenia, numeru, części..."));
    PomocGUI::Dodaj(szukaj_, "serwis.szukaj");
    gora->Add(szukaj_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    chRodzaj_ = new wxChoice(this, wxID_ANY);
    chRodzaj_->Append(TR("wszystkie rodzaje"));
    for (const std::string& r : LogikaSerwisu::rodzaje()) chRodzaj_->Append(TekstZUTF8(r));
    chRodzaj_->SetSelection(0);
    gora->Add(chRodzaj_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    chStatus_ = new wxChoice(this, wxID_ANY);
    chStatus_->Append(TR("wszystkie statusy"));
    for (const std::string& r : LogikaSerwisu::statusy()) chStatus_->Append(TekstZUTF8(r));
    chStatus_->SetSelection(0);
    gora->Add(chStatus_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    gora->Add(new wxStaticText(this, wxID_ANY, TR("od:")), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    dpOd_ = NowePoleDaty(this, wxDefaultDateTime, wxDP_DROPDOWN | wxDP_ALLOWNONE);
    dpOd_->SetValue(wxDateTime());
    gora->Add(dpOd_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    gora->Add(new wxStaticText(this, wxID_ANY, TR("do:")), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    dpDo_ = NowePoleDaty(this, wxDefaultDateTime, wxDP_DROPDOWN | wxDP_ALLOWNONE);
    dpDo_->SetValue(wxDateTime());
    gora->Add(dpDo_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    PomocGUI::Dodaj(dpOd_, "serwis.zakres");
    PomocGUI::Dodaj(dpDo_, "serwis.zakres");
    licznik_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    gora->Add(licznik_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 4);

    wxSplitterWindow* podzial = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);
    widok_ = new wxListCtrl(podzial, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    const struct { const char* t; int s; } kol[] = {{"Data", 85}, {"Urządzenie", 190}, {"Lokalizacja", 130}, {"Rodzaj", 85}, {"Status", 120}, {"Technik", 90}, {"Części", 55}, {"Zdjęcia", 60}};
    for (int i = 0; i < K_LICZBA; ++i) widok_->InsertColumn(i, TR(kol[i].t), (i >= K_CZESCI) ? wxLIST_FORMAT_RIGHT : wxLIST_FORMAT_LEFT, kol[i].s);
    PomocGUI::Dodaj(widok_, "serwis.lista");

    wxPanel* prawy = new wxPanel(podzial);
    wxBoxSizer* ps = new wxBoxSizer(wxVERTICAL);
    naglowek_ = new wxStaticText(prawy, wxID_ANY, TR("Wybierz wpis z listy"));
    wxFont f = naglowek_->GetFont(); f.MakeBold(); f.SetPointSize(f.GetPointSize() + 2);
    naglowek_->SetFont(f);
    ps->Add(naglowek_, 0, wxALL, 8);
    opis_ = new wxStaticText(prawy, wxID_ANY, wxEmptyString);
    ps->Add(opis_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    ps->Add(new wxStaticText(prawy, wxID_ANY, TR("Części:")), 0, wxLEFT | wxRIGHT, 8);
    czesciWidok_ = new wxListCtrl(prawy, wxID_ANY, wxDefaultPosition, wxSize(-1, 100), wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_THEME);
    czesciWidok_->InsertColumn(0, TR("Część"), wxLIST_FORMAT_LEFT, 170);
    czesciWidok_->InsertColumn(1, TR("Ilość"), wxLIST_FORMAT_RIGHT, 50);
    czesciWidok_->InsertColumn(2, TR("Zdjęty nr"), wxLIST_FORMAT_LEFT, 100);
    czesciWidok_->InsertColumn(3, TR("Zamontowany nr"), wxLIST_FORMAT_LEFT, 110);
    czesciWidok_->InsertColumn(4, TR("Magazyn"), wxLIST_FORMAT_LEFT, 90);
    ps->Add(czesciWidok_, 0, wxEXPAND | wxALL, 8);
    ps->Add(new wxStaticText(prawy, wxID_ANY, TR("Zdjęcia (kliknij, by otworzyć; prawy przycisk: usuń):")), 0, wxLEFT | wxRIGHT, 8);
    zdjeciaPanel_ = new wxScrolledWindow(prawy, wxID_ANY, wxDefaultPosition, wxSize(-1, 130), wxVSCROLL | wxBORDER_THEME);
    zdjeciaPanel_->SetScrollRate(0, 10);
    zdjeciaPanel_->SetSizer(new wxWrapSizer(wxHORIZONTAL));
    ps->Add(zdjeciaPanel_, 1, wxEXPAND | wxALL, 8);
    UstawieniaOkna::WczytajKolumny(widok_, "Serwis");
    Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& e) {
        if (e.GetEventObject() == this) UstawieniaOkna::ZapiszKolumny(widok_, "Serwis");
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
    Ikony::DlaPrzycisku(przycisk(ID_S_NOWY, TR("Nowy wpis...")), "nowy");
    wxButton* bEdytuj = przycisk(ID_S_EDYTUJ, TR("Edytuj..."));
    Ikony::DlaPrzycisku(bEdytuj, "edytuj");
    wxButton* bDuplikuj = przycisk(ID_S_DUPLIKUJ, TR("Kolejny serwis tego urządzenia..."));
    PomocGUI::Dodaj(bDuplikuj, "serwis.duplikuj");
    wxButton* bKamera = przycisk(ID_S_KAMERA, TR("Zdjęcie z kamery..."));
    wxButton* bPlik = przycisk(ID_S_PLIK, TR("Zdjęcie z pliku..."));
    wxButton* bHistoria = przycisk(ID_S_HISTORIA, TR("Historia urządzenia"));
    PomocGUI::Dodaj(bHistoria, "serwis.historia");
    wxButton* bUsun = przycisk(ID_S_USUN, TR("Usuń"));
    Ikony::DlaPrzycisku(bUsun, "usun");
    przycisk(ID_S_EKSPORT, TR("Eksport CSV..."));
    glowny->Add(przyciski, 0, wxEXPAND | wxALL, 4);
    SetSizer(glowny);

    for (wxButton* b : {bEdytuj, bDuplikuj, bKamera, bPlik, bHistoria, bUsun})
        b->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(Wybrany() != nullptr); });

    auto odswiez = [this](wxCommandEvent&) { if (!odswiezanie_) Odswiez(); };
    szukaj_->Bind(wxEVT_TEXT, odswiez);
    chRodzaj_->Bind(wxEVT_CHOICE, odswiez);
    chStatus_->Bind(wxEVT_CHOICE, odswiez);
    dpOd_->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) { if (!odswiezanie_) Odswiez(); });
    dpDo_->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) { if (!odswiezanie_) Odswiez(); });
    widok_->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent&) { PokazSzczegoly(); });
    widok_->Bind(wxEVT_LIST_ITEM_DESELECTED, [this](wxListEvent&) { PokazSzczegoly(); });
    widok_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) { Edytuj(); });
    widok_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, [this](wxListEvent& e) { MenuKontekstowe(e.GetIndex()); });
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Nowy(); }, ID_S_NOWY);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Edytuj(); }, ID_S_EDYTUJ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Duplikuj(); }, ID_S_DUPLIKUJ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DodajZdjecieZKamery(); }, ID_S_KAMERA);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DodajZdjecieZPliku(); }, ID_S_PLIK);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { HistoriaUrzadzenia(); }, ID_S_HISTORIA);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Usun(); }, ID_S_USUN);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EksportujCSV(); }, ID_S_EKSPORT);
}

void PanelSerwisu::Blad(const wxString& t) {
    Komunikat(TR("✗ ") + t);
    wxMessageBox(t, TR("Serwis"), wxOK | wxICON_WARNING, this);
}

const WpisSerwisowy* PanelSerwisu::Wybrany() const {
    const long i = widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (i < 0 || static_cast<size_t>(i) >= lista_.size()) return nullptr;
    return &lista_[static_cast<size_t>(i)];
}

void PanelSerwisu::Wybierz(long long id) {
    for (size_t i = 0; i < lista_.size(); ++i)
        if (lista_[i].id == id) {
            widok_->SetItemState(static_cast<long>(i), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            widok_->EnsureVisible(static_cast<long>(i));
            return;
        }
}

void PanelSerwisu::Szukaj(const std::string& tekst) { szukaj_->ChangeValue(TekstZUTF8(tekst)); Odswiez(); }

void PanelSerwisu::UstawFiltr(const std::string& rodzaj, const std::string& status) {
    auto ustaw = [](wxChoice* ch, const std::string& v) {
        const int i = v.empty() ? 0 : ch->FindString(TekstZUTF8(v));
        ch->SetSelection(i == wxNOT_FOUND ? 0 : i);
    };
    ustaw(chRodzaj_, rodzaj);
    ustaw(chStatus_, status);
    Odswiez();
}

FiltrSerwisu PanelSerwisu::Filtr() const {
    FiltrSerwisu f;
    f.tekst = Przytnij(szukaj_->GetValue()).utf8_string();
    if (chRodzaj_->GetSelection() > 0) f.rodzaj = LogikaSerwisu::rodzaje()[static_cast<size_t>(chRodzaj_->GetSelection() - 1)];
    if (chStatus_->GetSelection() > 0) f.status = LogikaSerwisu::statusy()[static_cast<size_t>(chStatus_->GetSelection() - 1)];
    f.od = Iso(dpOd_->GetValue());
    f.doDaty = Iso(dpDo_->GetValue());
    return f;
}

void PanelSerwisu::Wyczysc() {
    lista_.clear();
    szczegoly_.reset();
    widok_->DeleteAllItems();
    PokazSzczegoly();
    licznik_->SetLabel(wxEmptyString);
}

void PanelSerwisu::Odswiez() {
    odswiezanie_ = true;
    const long long zachowaj = Wybrany() ? Wybrany()->id : 0;
    widok_->DeleteAllItems();
    if (!logika_.pobierz(Filtr(), lista_)) {
        Status(TR("✗ Serwis: ") + TekstZUTF8(logika_.pobierzBlad()));
        lista_.clear();
    }
    for (size_t i = 0; i < lista_.size(); ++i) {
        const WpisSerwisowy& e = lista_[i];
        const long w = widok_->InsertItem(static_cast<long>(i), TekstZUTF8(e.data));
        widok_->SetItem(w, K_URZADZENIE, TekstZUTF8(e.urzadzenie));
        widok_->SetItem(w, K_LOKALIZACJA, TekstZUTF8(e.lokalizacja));
        widok_->SetItem(w, K_RODZAJ, TekstZUTF8(e.rodzaj));
        widok_->SetItem(w, K_STATUS, TekstZUTF8(e.status));
        widok_->SetItem(w, K_TECHNIK, TekstZUTF8(e.technik));
        widok_->SetItem(w, K_CZESCI, e.liczbaCzesci ? wxString::Format("%d", e.liczbaCzesci) : wxString());
        widok_->SetItem(w, K_ZDJECIA, e.liczbaZdjec ? wxString::Format("%d", e.liczbaZdjec) : wxString());
        if (e.status == "oczekuje na części") widok_->SetItemTextColour(w, wxColour(200, 60, 0));
        else if (e.status == "w trakcie") widok_->SetItemTextColour(w, wxColour(30, 100, 190));
    }
    licznik_->SetLabel(wxString::Format(TR("Wpisów: %zu"), lista_.size()));
    odswiezanie_ = false;
    if (zachowaj) Wybierz(zachowaj);
    PokazSzczegoly();
}

void PanelSerwisu::PokazSzczegoly() {
    czesciWidok_->DeleteAllItems();
    zdjeciaPanel_->DestroyChildren();
    zdjeciaPanel_->GetSizer()->Clear(false);
    liczbaMiniatur_ = 0;
    szczegoly_.reset();
    const WpisSerwisowy* sel = Wybrany();
    if (!sel) {
        naglowek_->SetLabel(TR("Wybierz wpis z listy"));
        opis_->SetLabel(wxEmptyString);
        opis_->GetParent()->Layout();
        return;
    }
    szczegoly_ = logika_.pobierzWpis(sel->id);
    if (!szczegoly_) {
        naglowek_->SetLabel(TekstZUTF8(sel->urzadzenie));
        opis_->SetLabel(TekstZUTF8(logika_.pobierzBlad()));
        return;
    }
    const WpisSerwisowy& w = *szczegoly_;
    naglowek_->SetLabel(TekstZUTF8(w.urzadzenie) + " - " + TekstZUTF8(w.data));
    wxString t;
    auto dodaj = [&](const wxString& et, const std::string& v) { if (!v.empty()) t += et + ": " + TekstZUTF8(v) + "\n"; };
    dodaj(TR("Rodzaj"), w.rodzaj);
    dodaj(TR("Status"), w.status);
    dodaj(TR("Producent"), w.producent);
    dodaj(TR("Model"), w.model);
    dodaj(TR("Numer seryjny"), w.numerSeryjny);
    dodaj(TR("Lokalizacja"), w.lokalizacja);
    dodaj(TR("Technik"), w.technik);
    dodaj(TR("Nr zgłoszenia"), w.zgloszenie);
    if (w.czasMin) t += TR("Czas pracy: ") + wxString::Format(TR("%d min"), *w.czasMin) + "\n";
    dodaj(TR("Usterka"), w.usterka);
    dodaj(TR("Wykonano"), w.wykonano);
    dodaj(TR("Uwagi"), w.uwagi);
    opis_->SetLabel(t);
    opis_->Wrap(std::max(200, opis_->GetParent()->GetClientSize().GetWidth() - 20));
    for (size_t i = 0; i < w.czesci.size(); ++i) {
        const CzescSerwisowa& c = w.czesci[i];
        const long r = czesciWidok_->InsertItem(static_cast<long>(i), TekstZUTF8(c.nazwa));
        czesciWidok_->SetItem(r, 1, TekstZUTF8(liczbaNaTekstPolski(c.ilosc, 3, 0)));
        czesciWidok_->SetItem(r, 2, TekstZUTF8(c.numerStary));
        czesciWidok_->SetItem(r, 3, TekstZUTF8(c.numerNowy));
        czesciWidok_->SetItem(r, 4, c.pobrano ? TR("pobrano ") + TekstZUTF8(c.symbolElementu) : c.idElementu ? TekstZUTF8(c.symbolElementu) : wxString());
    }
    for (size_t i = 0; i < w.zdjecia.size(); ++i) {
        const ZdjecieSerwisu& z = w.zdjecia[i];
        const wxString sciezka = TekstZUTF8(z.sciezka);
        const wxImage obraz = ZdjeciaElementow::Wczytaj(sciezka, wxSize(150, 110));
        wxWindow* el;
        if (obraz.IsOk()) {
            ++liczbaMiniatur_;
            el = new wxStaticBitmap(zdjeciaPanel_, wxID_ANY, wxBitmap(obraz));
        } else {
            el = new wxStaticText(zdjeciaPanel_, wxID_ANY, TR("(brak pliku)\n") + NazwaPliku(z.sciezka));
        }
        el->SetToolTip(sciezka + (z.opis.empty() ? wxString() : "\n" + TekstZUTF8(z.opis)));
        el->SetCursor(wxCursor(wxCURSOR_HAND));
        el->Bind(wxEVT_LEFT_UP, [sciezka](wxMouseEvent&) {
            if (wxFileExists(sciezka)) wxLaunchDefaultApplication(sciezka);
        });
        el->Bind(wxEVT_RIGHT_UP, [this, i](wxMouseEvent&) { MenuZdjecia(i); });
        zdjeciaPanel_->GetSizer()->Add(el, 0, wxALL, 4);
    }
    zdjeciaPanel_->FitInside();
    zdjeciaPanel_->GetSizer()->Layout();
    opis_->GetParent()->Layout();
}

void PanelSerwisu::MenuZdjecia(size_t numer) {
    wxMenu menu;
    menu.Append(ID_S_USUN_ZDJECIE, TR("Usuń to zdjęcie z wpisu"));
    menu.Bind(wxEVT_MENU, [this, numer](wxCommandEvent&) { CallAfter([this, numer] { UsunZdjecie(numer); }); }, ID_S_USUN_ZDJECIE);
    PopupMenu(&menu);
}

void PanelSerwisu::MenuKontekstowe(long element) {
    if (element < 0 || static_cast<size_t>(element) >= lista_.size()) return;
    widok_->SetItemState(element, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    wxMenu menu;
    menu.Append(ID_S_EDYTUJ, TR("Edytuj..."));
    menu.Append(ID_S_DUPLIKUJ, TR("Kolejny serwis tego urządzenia..."));
    menu.AppendSeparator();
    menu.Append(ID_S_KAMERA, TR("Dołącz zdjęcie z kamery..."));
    menu.Append(ID_S_PLIK, TR("Dołącz zdjęcie z pliku..."));
    menu.Append(ID_S_HISTORIA, TR("Historia tego urządzenia"));
    menu.AppendSeparator();
    menu.Append(ID_S_USUN, TR("Usuń"));
    menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
        const int id = e.GetId();
        CallAfter([this, id] {
            switch (id) {
            case ID_S_EDYTUJ: Edytuj(); break;
            case ID_S_DUPLIKUJ: Duplikuj(); break;
            case ID_S_KAMERA: DodajZdjecieZKamery(); break;
            case ID_S_PLIK: DodajZdjecieZPliku(); break;
            case ID_S_HISTORIA: HistoriaUrzadzenia(); break;
            case ID_S_USUN: Usun(); break;
            }
        });
    });
    PopupMenu(&menu);
}

bool PanelSerwisu::ZapiszZeZdjeciami(const WpisSerwisowy& w, const std::vector<ZdjecieSerwisu>& zdjecia, const std::vector<long long>& usuniete, long long& id, bool zwroc) {
    auto zapisany = logika_.zapisz(w, zwroc);
    if (!zapisany) { Blad(TekstZUTF8(logika_.pobierzBlad())); return false; }
    id = *zapisany;
    for (long long z : usuniete) logika_.usunZdjecie(z);
    for (const ZdjecieSerwisu& z : zdjecia)
        if (z.id == 0 && !logika_.dodajZdjecie(id, z.sciezka, z.opis)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return false; }
    return true;
}

void PanelSerwisu::Nowy() {
    WpisSerwisowy w;
    const auto technicy = logika_.technicy();
    if (!technicy.empty()) w.technik = technicy.front();
    DialogEdycjiSerwisu dlg(this, w, logika_, magazyn_);
    if (dlg.ShowModal() != wxID_OK) return;
    long long id = 0;
    if (!ZapiszZeZdjeciami(dlg.Wynik(), dlg.Zdjecia(), dlg.Usuniete(), id)) return;
    Komunikat(TR("✓ Dodano wpis serwisowy: ") + TekstZUTF8(dlg.Wynik().urzadzenie));
    Odswiez();
    Wybierz(id);
}

void PanelSerwisu::Edytuj() {
    const WpisSerwisowy* sel = Wybrany();
    if (!sel) return;
    auto pelny = logika_.pobierzWpis(sel->id);
    if (!pelny) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    DialogEdycjiSerwisu dlg(this, *pelny, logika_, magazyn_);
    if (dlg.ShowModal() != wxID_OK) return;
    // części pobrane z magazynu, usunięte z listy: pytamy, czy zwrócić je na stan
    wxString usuniete;
    for (const CzescSerwisowa& stara : pelny->czesci) {
        if (!stara.pobrano) continue;
        const auto& nowe = dlg.Wynik().czesci;
        if (std::none_of(nowe.begin(), nowe.end(), [&](const CzescSerwisowa& n) { return n.id == stara.id; }))
            usuniete += "\n  " + TekstZUTF8(stara.nazwa) + " x " + TekstZUTF8(liczbaNaTekstPolski(stara.ilosc, 3, 0));
    }
    bool zwroc = false;
    if (!usuniete.empty())
        zwroc = wxMessageBox(TR("Usunięto części pobrane wcześniej z magazynu:") + usuniete + "\n\n" + TR("Zwrócić je na stan magazynu?"),
                             TR("Serwis"), wxYES_NO | wxICON_QUESTION, this) == wxYES;
    long long id = 0;
    if (!ZapiszZeZdjeciami(dlg.Wynik(), dlg.Zdjecia(), dlg.Usuniete(), id, zwroc)) return;
    Odswiez();
    Wybierz(id);
}

void PanelSerwisu::Duplikuj() {
    const WpisSerwisowy* sel = Wybrany();
    if (!sel) return;
    auto pelny = logika_.pobierzWpis(sel->id);
    if (!pelny) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    WpisSerwisowy w = *pelny;
    w.id = 0;
    w.data.clear();                 // dziś
    w.status = "zakończone";
    w.zgloszenie.clear(); w.czasMin.reset(); w.usterka.clear(); w.wykonano.clear(); w.uwagi.clear();
    w.czesci.clear(); w.zdjecia.clear();
    DialogEdycjiSerwisu dlg(this, w, logika_, magazyn_);
    if (dlg.ShowModal() != wxID_OK) return;
    long long id = 0;
    if (!ZapiszZeZdjeciami(dlg.Wynik(), dlg.Zdjecia(), dlg.Usuniete(), id)) return;
    Komunikat(TR("✓ Dodano wpis serwisowy: ") + TekstZUTF8(dlg.Wynik().urzadzenie));
    Odswiez();
    Wybierz(id);
}

void PanelSerwisu::Usun(bool bezPytan) {
    const WpisSerwisowy* w = Wybrany();
    if (!w) return;
    if (!bezPytan && wxMessageBox(wxString::Format(TR("Usunąć wpis serwisowy \"%s\" z %s razem z listą części i zdjęciami (pliki zdjęć zostają na dysku)?"),
                                                    TekstZUTF8(w->urzadzenie), TekstZUTF8(w->data)),
                                  TR("Serwis"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return;
    bool zwroc = false;
    const auto pobrane = logika_.czesciPobrane(w->id);
    if (!bezPytan && !pobrane.empty()) {
        wxString lista;
        for (const CzescSerwisowa& c : pobrane) lista += "\n  " + TekstZUTF8(c.nazwa) + " x " + TekstZUTF8(liczbaNaTekstPolski(c.ilosc, 3, 0));
        zwroc = wxMessageBox(TR("Ten wpis ma części pobrane z magazynu:") + lista + "\n\n" + TR("Zwrócić je na stan magazynu?"),
                             TR("Serwis"), wxYES_NO | wxICON_QUESTION, this) == wxYES;
    }
    if (!logika_.usun(w->id, zwroc)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    Odswiez();
}

bool PanelSerwisu::DolaczZdjecie(const wxString& sciezka, const wxString& opis) {
    const WpisSerwisowy* w = Wybrany();
    if (!w || sciezka.empty()) return false;
    const long long id = w->id;
    if (!logika_.dodajZdjecie(id, sciezka.utf8_string(), opis.utf8_string())) { Blad(TekstZUTF8(logika_.pobierzBlad())); return false; }
    Komunikat(TR("✓ Dołączono zdjęcie do wpisu serwisowego"));
    Odswiez();
    Wybierz(id);
    return true;
}

void PanelSerwisu::DodajZdjecieZKamery() {
    const WpisSerwisowy* w = Wybrany();
    if (!w) return;
    const wxString p = ZdjeciaElementow::ZKamery(this, TekstZUTF8(w->urzadzenie));
    if (!p.empty()) DolaczZdjecie(p);
}

void PanelSerwisu::DodajZdjecieZPliku() {
    if (!Wybrany()) return;
    const wxString p = ZdjeciaElementow::ZPliku(this);
    if (!p.empty()) DolaczZdjecie(p);
}

void PanelSerwisu::UsunZdjecie(size_t numer, bool bezPytan) {
    if (!szczegoly_ || numer >= szczegoly_->zdjecia.size()) return;
    if (!bezPytan && wxMessageBox(TR("Usunąć to zdjęcie z wpisu (plik zostaje na dysku)?"), TR("Serwis"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return;
    const long long id = szczegoly_->id, zdj = szczegoly_->zdjecia[numer].id;
    if (!logika_.usunZdjecie(zdj)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    Odswiez();
    Wybierz(id);
}

void PanelSerwisu::HistoriaUrzadzenia() {
    const WpisSerwisowy* w = Wybrany();
    if (!w) return;
    // numer seryjny identyfikuje urządzenie jednoznacznie; bez niego szukamy po nazwie
    chRodzaj_->SetSelection(0);
    chStatus_->SetSelection(0);
    dpOd_->SetValue(wxDateTime());
    dpDo_->SetValue(wxDateTime());
    Szukaj(w->numerSeryjny.empty() ? w->urzadzenie : w->numerSeryjny);
    Status(wxString::Format(TR("Historia urządzenia: %zu wpisów (aby wrócić do wszystkich, wyczyść pole szukania)"), lista_.size()));
}

bool PanelSerwisu::EksportujCSV(const wxString& sciezka) {
    wxString plik = sciezka;
    if (plik.empty()) {
        wxFileDialog dlg(this, TR("Eksportuj ewidencję serwisową do CSV"), wxEmptyString, "serwis.csv", TR("Pliki CSV (*.csv)|*.csv"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        plik = dlg.GetPath();
    }
    std::vector<WpisSerwisowy> pelne;
    for (const WpisSerwisowy& e : lista_) {      // eksportuje to, co widać na liście (po filtrach)
        auto p = logika_.pobierzWpis(e.id);
        if (!p) { Blad(TekstZUTF8(logika_.pobierzBlad())); return false; }
        pelne.push_back(*p);
    }
    const std::string tekst = "\xEF\xBB\xBF" + LogikaSerwisu::naCSV(pelne);
    wxLogNull cisza;
    wxFFile f(plik, "wb");
    if (!f.IsOpened() || f.Write(tekst.data(), tekst.size()) != tekst.size()) { Blad(TR("Nie można zapisać pliku.")); return false; }
    Komunikat(wxString::Format(TR("✓ Wyeksportowano wpisów serwisowych: %zu"), pelne.size()));
    return true;
}
