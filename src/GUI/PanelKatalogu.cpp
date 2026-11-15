#include "PanelKatalogu.h"
#include "HtmlZDymkami.h"
#include "../Logika/OpisyParametrow.h"
#include "Ikony.h"
#include "UstawieniaOkna.h"
#include "DialogKatalogu.h"
#include "DialogSlownikaParametrow.h"
#include "../Logika/LogikaSlownikaParametrow.h"
#include "DialogDatasheetMouser.h"
#include "ZdjeciaElementow.h"
#include "../Narzedzia/Indeksy.h"
#include "../Logika/KatalogCSV.h"
#include "../Logika/OdczytDatasheetu.h"
#include "../Narzedzia/Konfiguracja.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"
#include "../Wyszukiwanie/KlientHTTP.h"
#include "../Wyszukiwanie/WyszukiwanieDatasheet.h"

#include <wx/datetime.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/log.h>
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
    wxString s;
    wxFileOffset n = f.Length();
    tekst.assign(static_cast<size_t>(n), '\0');
    return f.Read(&tekst[0], tekst.size()) == tekst.size();
}

/// Okno z propozycjami odczytanymi z datasheetu - każdą można odznaczyć
class DialogPropozycji : public wxDialog {
public:
    DialogPropozycji(wxWindow* parent, const OdczytDatasheetu::Wynik& w, const PozycjaKatalogu& poz)
        : wxDialog(parent, wxID_ANY, TR("Parametry odczytane z datasheetu"), wxDefaultPosition, wxSize(860, 560),
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), wynik_(w) {
        wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
        g->Add(new wxStaticText(this, wxID_ANY,
               TR("To tylko propozycje - program odgaduje je z tekstu karty katalogowej i może się mylić.\n"
                  "Zaznacz te, które są poprawne; zapiszą się z oznaczeniem źródła \"datasheet\".\n"
                  "\"Maks.\" = Absolute Maximum Ratings, \"Elektr.\" = Electrical Characteristics. "
                  "Pozycje oznaczone \"zgadywane\" są odznaczone - sprawdź je przed zaznaczeniem.")),
               0, wxALL, 10);
        lista_ = new wxCheckListBox(this, wxID_ANY);
        for (const auto& p : w.parametry) {
            const int i = lista_->Append(TekstZUTF8(p.nazwa) + ": " + TekstZUTF8(p.wartosc) + "    [" + TekstZUTF8(p.fragment) + "]" +
                                         (p.pewny ? wxString() : TR("   (zgadywane)")));
            lista_->Check(static_cast<unsigned>(i), p.pewny);
        }
        g->Add(lista_, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
        cbOpis_ = new wxCheckBox(this, wxID_ANY, TR("Ustaw opis: ") + TekstZUTF8(w.opis));
        cbOpis_->SetValue(!w.opis.empty() && poz.opis.empty());
        cbOpis_->Enable(!w.opis.empty());
        cbObudowa_ = new wxCheckBox(this, wxID_ANY, TR("Ustaw obudowę: ") + TekstZUTF8(w.obudowa));
        cbObudowa_->SetValue(!w.obudowa.empty() && poz.obudowa.empty());
        cbObudowa_->Enable(!w.obudowa.empty());
        cbUwagi_ = new wxCheckBox(this, wxID_ANY, TR("Dopisz do uwag: ") + TekstZUTF8(w.uwagi.size() > 110 ? w.uwagi.substr(0, 110) + "..." : w.uwagi));
        cbUwagi_->SetValue(!w.uwagi.empty() && poz.uwagi.find(w.uwagi) == std::string::npos);
        cbUwagi_->Enable(!w.uwagi.empty());
        g->Add(cbOpis_, 0, wxALL, 10);
        g->Add(cbObudowa_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
        g->Add(cbUwagi_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
        g->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
        SetSizer(g);
    }
    std::vector<ParametrKatalogu> Wybrane() const {
        std::vector<ParametrKatalogu> w;
        for (size_t i = 0; i < wynik_.parametry.size(); ++i)
            if (lista_->IsChecked(static_cast<unsigned>(i)))
                w.push_back({wynik_.parametry[i].nazwa, wynik_.parametry[i].wartosc, "datasheet"});
        return w;
    }
    bool UstawOpis() const { return cbOpis_->GetValue(); }
    bool UstawObudowe() const { return cbObudowa_->GetValue(); }
    bool DopiszUwagi() const { return cbUwagi_->GetValue(); }

private:
    OdczytDatasheetu::Wynik wynik_;
    wxCheckListBox* lista_ = nullptr;
    wxCheckBox *cbOpis_ = nullptr, *cbObudowa_ = nullptr, *cbUwagi_ = nullptr;
};

} // namespace

PanelKatalogu::PanelKatalogu(wxWindow* parent, LogikaMagazynu& magazyn, LogikaKatalogu& katalog)
    : wxPanel(parent), magazyn_(magazyn), katalog_(katalog) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);
    szukaj_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(280, -1));
    szukaj_->SetDescriptiveText(TR("Szukaj w katalogu..."));
    PomocGUI::Dodaj(szukaj_, "katalog.szukaj");
    gora->Add(szukaj_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    gora->Add(new wxStaticText(this, wxID_ANY, TR("Rodzina:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    chRodzina_ = new wxChoice(this, wxID_ANY);
    PomocGUI::Dodaj(chRodzina_, "katalog.filtr_rodziny");
    gora->Add(chRodzina_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    cbMagazyn_ = new wxCheckBox(this, wxID_ANY, TR("tylko te, które mam w magazynie"));
    PomocGUI::Dodaj(cbMagazyn_, "katalog.tylko_magazyn");
    gora->Add(cbMagazyn_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    licznik_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    gora->Add(licznik_, 0, wxALIGN_CENTER_VERTICAL);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 6);

    wxSplitterWindow* podzial = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);
    widok_ = new wxListCtrl(podzial, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
    const struct { const char* t; int s; } kol[] = {{"Symbol", 150}, {"Producent", 120}, {"Opis", 300}, {"Obudowa", 90}, {"Rodzina", 110}, {"W magazynie", 90}};
    for (int i = 0; i < K_LICZBA; ++i)
        widok_->InsertColumn(i, TR(kol[i].t), i == K_MAGAZYN ? wxLIST_FORMAT_RIGHT : wxLIST_FORMAT_LEFT, kol[i].s);
    PomocGUI::Dodaj(widok_, "katalog.lista");
    widok_->Bind(wxEVT_LIST_COL_CLICK, [this](wxListEvent& e) {
        sort_.Klik(e.GetColumn());
        Odswiez();
    });

    wxPanel* prawy = new wxPanel(podzial);
    wxBoxSizer* ps = new wxBoxSizer(wxVERTICAL);
    naglowek_ = new wxStaticText(prawy, wxID_ANY, TR("Wybierz pozycję z listy"));
    wxFont f = naglowek_->GetFont(); f.MakeBold(); f.SetPointSize(f.GetPointSize() + 2);
    naglowek_->SetFont(f);
    ps->Add(naglowek_, 0, wxALL, 8);
    zdjecie_ = new wxStaticBitmap(prawy, wxID_ANY, wxNullBitmap);
    zdjecie_->Hide();
    ps->Add(zdjecie_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    szczegoly_ = new wxStaticText(prawy, wxID_ANY, wxEmptyString);
    ps->Add(szczegoly_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    // parametry jako HTML: nazwy i wartości mogą mieć indeksy dolne i górne (V_CE, m^2)
    parametry_ = new HtmlZDymkami(prawy, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_DEFAULT_STYLE | wxBORDER_THEME);
    PomocGUI::Dodaj(parametry_, "katalog.parametry");
    UstawieniaOkna::WczytajKolumny(widok_, "Katalog");
    Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& e) {
        if (e.GetEventObject() == this) UstawieniaOkna::ZapiszKolumny(widok_, "Katalog");
        e.Skip();
    });
    ps->Add(parametry_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
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
    Ikony::DlaPrzycisku(przycisk(ID_K_NOWA, TR("Nowa...")), "nowy");
    wxButton* bEdytuj = przycisk(ID_K_EDYTUJ, TR("Edytuj..."));
    Ikony::DlaPrzycisku(bEdytuj, "edytuj");
    wxButton* bUsun = przycisk(ID_K_USUN, TR("Usuń"));
    Ikony::DlaPrzycisku(bUsun, "usun");
    wxButton* bMag = przycisk(ID_K_DO_MAGAZYNU, TR("Dodaj do magazynu..."));
    PomocGUI::Dodaj(bMag, "katalog.do_magazynu");
    przycisk(ID_K_IMPORT, TR("Import CSV..."));
    przycisk(ID_K_EKSPORT, TR("Eksport CSV..."));
    przycisk(ID_K_SZABLON, TR("Szablon CSV..."));
    wxButton* bOtworz = przycisk(ID_K_DS_OTWORZ, TR("Otwórz datasheet"));
    wxButton* bPobierz = przycisk(ID_K_DS_POBIERZ, TR("Pobierz datasheet"));
    wxButton* bOdczyt = przycisk(ID_K_DS_ODCZYT, TR("Odczytaj parametry z PDF..."));
    PomocGUI::Dodaj(bOdczyt, "katalog.odczyt_pdf");
    wxButton* bOcr = przycisk(ID_K_DS_OCR, TR("OCR datasheetu..."));
    PomocGUI::Dodaj(bOcr, "katalog.ocr");
    wxButton* bSlownik = przycisk(ID_K_SLOWNIK, TR("Słownik parametrów..."));
    bSlownik->SetToolTip(TR("Opisy symboli parametrów (np. W_amax) pokazywane w dymkach; zapisane w bazie danych."));
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OtworzSlownikParametrow(); }, ID_K_SLOWNIK);
    glowny->Add(przyciski, 0, wxEXPAND | wxALL, 4);
    SetSizer(glowny);

    for (wxButton* b : {bEdytuj, bMag, bOtworz, bPobierz, bOdczyt})
        b->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(Wybrana() != nullptr); });
    bOcr->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(!WybraneZDatasheetem().empty()); });
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OcrDatasheetow(); }, ID_K_DS_OCR);
    bUsun->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(LiczbaZaznaczonych() > 0); });
    widok_->Bind(wxEVT_LIST_KEY_DOWN, [this](wxListEvent& e) {
        if (e.GetKeyCode() == WXK_DELETE) Usun();
        else if (e.GetKeyCode() == 'A' && wxGetKeyState(WXK_CONTROL)) ZaznaczWszystkie();
        else if (e.GetKeyCode() == 'A' && wxGetKeyState(WXK_CONTROL)) widok_->SetItemState(-1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        else e.Skip();
    });

    auto odswiez = [this](wxCommandEvent&) { if (!odswiezanie_) Odswiez(); };
    szukaj_->Bind(wxEVT_TEXT, odswiez);
    chRodzina_->Bind(wxEVT_CHOICE, odswiez);
    cbMagazyn_->Bind(wxEVT_CHECKBOX, odswiez);
    widok_->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent&) { PokazSzczegoly(); });
    widok_->Bind(wxEVT_LIST_ITEM_DESELECTED, [this](wxListEvent&) { PokazSzczegoly(); });
    widok_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) { Edytuj(); });
    widok_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, [this](wxListEvent& e) {
        if (e.GetIndex() >= 0 && !(widok_->GetItemState(e.GetIndex(), wxLIST_STATE_SELECTED) & wxLIST_STATE_SELECTED)) {
            widok_->SetItemState(-1, 0, wxLIST_STATE_SELECTED);   // prawy przycisk na niezaznaczonej pozycji: zaznacz tylko ją
            widok_->SetItemState(e.GetIndex(), wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
        }
        auto menu = MenuKontekstowe();
        if (menu) widok_->PopupMenu(menu.get());
    });
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Nowa(); }, ID_K_NOWA);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Edytuj(); }, ID_K_EDYTUJ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Usun(); }, ID_K_USUN);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DodajDoMagazynu(); }, ID_K_DO_MAGAZYNU);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ImportujCSV(); }, ID_K_IMPORT);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EksportujCSV(); }, ID_K_EKSPORT);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ZapiszSzablon(); }, ID_K_SZABLON);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OtworzDatasheet(); }, ID_K_DS_OTWORZ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PobierzDatasheet(); }, ID_K_DS_POBIERZ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OdczytajDatasheet(); }, ID_K_DS_ODCZYT);
}

wxString PanelKatalogu::KatalogDokumentow() const {
    return TekstZUTF8(Konfiguracja::pobierz().pobierzUstawieniaAplikacji().sciezkaPDFDatasheet);
}

void PanelKatalogu::Blad(const wxString& t) {
    Komunikat(TR("✗ ") + t);
    wxMessageBox(t, TR("Katalog"), wxOK | wxICON_WARNING, this);
}

int PanelKatalogu::LiczbaZaznaczonych() const { return widok_->GetSelectedItemCount(); }

void PanelKatalogu::ZaznaczWszystkie() {
    for (long i = 0; i < widok_->GetItemCount(); ++i) widok_->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

const PozycjaKatalogu* PanelKatalogu::Wybrana() const {
    if (widok_->GetSelectedItemCount() != 1) return nullptr;   // przy zaznaczeniu wielu pozycji nie ma jednej wybranej
    const long i = widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (i < 0 || static_cast<size_t>(i) >= lista_.size()) return nullptr;
    return &lista_[static_cast<size_t>(i)];
}

void PanelKatalogu::Wybierz(long long id) {
    for (size_t i = 0; i < lista_.size(); ++i)
        if (lista_[i].id == id) {
            widok_->SetItemState(-1, 0, wxLIST_STATE_SELECTED);   // zaznaczenie tylko tej pozycji
            widok_->SetItemState(static_cast<long>(i), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            widok_->EnsureVisible(static_cast<long>(i));
            return;
        }
}

void PanelKatalogu::PokazPozycje(long long id) {
    odswiezanie_ = true;
    szukaj_->ChangeValue(wxEmptyString);
    chRodzina_->SetSelection(0);
    cbMagazyn_->SetValue(false);
    odswiezanie_ = false;
    Odswiez();
    Wybierz(id);
}

void PanelKatalogu::Szukaj(const std::string& tekst) { szukaj_->ChangeValue(TekstZUTF8(tekst)); Odswiez(); }

void PanelKatalogu::WypelnijRodziny() {
    const wxString zaznaczona = chRodzina_->GetSelection() > 0 ? chRodzina_->GetStringSelection() : wxString();
    rodziny_ = katalog_.rodziny();
    chRodzina_->Clear();
    chRodzina_->Append(TR("(wszystkie)"));
    for (const std::string& r : rodziny_) chRodzina_->Append(TekstZUTF8(r));
    int wybor = 0;
    if (!zaznaczona.empty()) { const int n = chRodzina_->FindString(zaznaczona); if (n != wxNOT_FOUND) wybor = n; }
    chRodzina_->SetSelection(wybor);
}

void PanelKatalogu::Wyczysc() {
    lista_.clear();
    widok_->DeleteAllItems();
    PokazSzczegoly();
    licznik_->SetLabel(wxEmptyString);
}

wxString PanelKatalogu::NaglowekKolumny(int kolumna) const {
    wxListItem it;
    it.SetMask(wxLIST_MASK_TEXT);
    widok_->GetColumn(kolumna, it);
    return it.GetText();
}

void PanelKatalogu::OtworzSlownikParametrow() {
    LogikaSlownikaParametrow sl(ManagerBazyDanych::pobierz());
    {
        DialogSlownikaParametrow d(this, sl);
        d.ShowModal();
    }
    sl.wczytajDoPamieci();
    Odswiez();
}

void PanelKatalogu::Odswiez() {
    { LogikaSlownikaParametrow sl(ManagerBazyDanych::pobierz()); sl.wczytajDoPamieci(); }   // opisy do dymków (wspólne w bazie)
    odswiezanie_ = true;
    const long long zachowaj = Wybrana() ? Wybrana()->id : 0;
    WypelnijRodziny();
    FiltrKatalogu f;
    f.tekst = szukaj_->GetValue().Strip(wxString::both).utf8_string();
    if (chRodzina_->GetSelection() > 0) f.rodzina = chRodzina_->GetStringSelection().utf8_string();
    f.tylkoWMagazynie = cbMagazyn_->GetValue();
    widok_->DeleteAllItems();
    lista_.clear();
    if (!katalog_.pobierz(f, lista_)) {
        Status(TR("✗ Katalog: ") + TekstZUTF8(katalog_.pobierzBlad()));
        lista_.clear();
    }
    SortowanieList::Posortuj(lista_, sort_, [](const PozycjaKatalogu& a, const PozycjaKatalogu& b, int kol) {
        switch (kol) {
            case K_SYMBOL: return SortowanieList::PorownajTekst(a.symbol, b.symbol) < 0;
            case K_PRODUCENT: return SortowanieList::PorownajTekst(a.producent, b.producent) < 0;
            case K_OPIS: return SortowanieList::PorownajTekst(a.opis, b.opis) < 0;
            case K_OBUDOWA: return SortowanieList::PorownajTekst(a.obudowa, b.obudowa) < 0;
            case K_RODZINA: return SortowanieList::PorownajTekst(a.rodzina, b.rodzina) < 0;
            case K_MAGAZYN: return a.wMagazynie < b.wMagazynie;
            default: return false;
        }
    });
    SortowanieList::OznaczNaglowek(widok_, sort_);
    for (size_t i = 0; i < lista_.size(); ++i) {
        const PozycjaKatalogu& k = lista_[i];
        const long r = widok_->InsertItem(static_cast<long>(i), TekstZUTF8(k.symbol));
        widok_->SetItem(r, K_PRODUCENT, TekstZUTF8(k.producent));
        widok_->SetItem(r, K_OPIS, TekstZUTF8(k.opis));
        widok_->SetItem(r, K_OBUDOWA, TekstZUTF8(k.obudowa));
        widok_->SetItem(r, K_RODZINA, TekstZUTF8(k.rodzina));
        widok_->SetItem(r, K_MAGAZYN, k.wMagazynie > 0 ? wxString::Format("%d", k.wMagazynie) : wxString("-"));
        if (k.liczbaElementow > 0) widok_->SetItemTextColour(r, wxColour(0, 110, 0));
    }
    licznik_->SetLabel(wxString::Format(TR("Pozycji: %d"), static_cast<int>(lista_.size())));
    odswiezanie_ = false;
    if (zachowaj) Wybierz(zachowaj);
    if (LiczbaZaznaczonych() == 0 && !lista_.empty()) widok_->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    PokazSzczegoly();
    Layout();
}

void PanelKatalogu::PokazSzczegoly() {
    parametry_->SetPage("<html><body></body></html>");
    zdjeciePokazane_ = false;
    zdjecie_->Hide();
    const PozycjaKatalogu* w = Wybrana();
    if (!w) {
        naglowek_->SetLabel(LiczbaZaznaczonych() > 1 ? wxString::Format(TR("Zaznaczono pozycji: %d"), LiczbaZaznaczonych()) : TR("Wybierz pozycję z listy"));
        szczegoly_->SetLabel(wxEmptyString);
        szczegoly_->GetParent()->Layout();
        return;
    }
    auto pelna = katalog_.pobierzPozycje(w->id);
    const PozycjaKatalogu& k = pelna ? *pelna : *w;
    naglowek_->SetLabel(TekstZUTF8(k.symbol) + (k.producent.empty() ? wxString() : " (" + TekstZUTF8(k.producent) + ")"));
    wxString t;
    if (!k.opis.empty()) t += TekstZUTF8(k.opis) + "\n";
    if (!k.obudowa.empty()) t += TR("Obudowa: ") + TekstZUTF8(k.obudowa) + "\n";
    if (!k.rodzina.empty()) t += TR("Rodzina: ") + TekstZUTF8(k.rodzina) + "\n";
    t += k.wMagazynie > 0 ? wxString::Format(TR("W magazynie: %d szt. (%d poz.)"), k.wMagazynie, k.liczbaElementow)
                          : TR("Nie ma w magazynie.");
    t += "\n";
    if (WyszukiwanieDatasheet::czyPobrany(KatalogDokumentow(), k.symbol)) t += TR("Datasheet: pobrany\n");
    else if (!k.datasheetUrl.empty()) t += TR("Datasheet: ") + TekstZUTF8(k.datasheetUrl) + "\n";
    if (!k.uwagi.empty()) t += TR("Uwagi: ") + TekstZUTF8(k.uwagi);
    szczegoly_->SetLabel(t);
    std::string html = "<html><body><table border=\"0\" cellpadding=\"3\" width=\"100%\">";
    std::vector<wxString> opisy;
    for (const ParametrKatalogu& p : k.parametry) {
        const std::string opis = OpisyParametrow::dlaParametru(p);
        // nazwa z opisem jest łączem "opis:N": po najechaniu pokazuje dymek (patrz HtmlZDymkami)
        const std::string nazwaHtml = opis.empty() ? "<b>" + Indeksy::naHtml(p.nazwa) + "</b>"
                                                   : "<a href=\"opis:" + std::to_string(opisy.size()) + "\"><b>" + Indeksy::naHtml(p.nazwa) + "</b></a>";
        if (!opis.empty()) opisy.push_back(wxString::FromUTF8(opis));
        html += "<tr><td>" + nazwaHtml + "</td><td>" + Indeksy::naHtml(p.wartosc) + "</td><td><font size=\"-1\" color=\"#808080\">" +
                Indeksy::naHtml(p.zrodlo) + "</font></td></tr>";
    }
    html += "</table></body></html>";
    parametry_->UstawOpisy(std::move(opisy));
    htmlParametrow_ = wxString::FromUTF8(html);
    parametry_->SetPage(htmlParametrow_);
    wxImage obraz;
    if (!k.zdjecie.empty()) obraz = ZdjeciaElementow::Wczytaj(TekstZUTF8(k.zdjecie), wxSize(260, 170));
    zdjeciePokazane_ = obraz.IsOk();
    if (zdjeciePokazane_) zdjecie_->SetBitmap(wxBitmap(obraz));
    zdjecie_->Show(zdjeciePokazane_);
    szczegoly_->GetParent()->Layout();
}

void PanelKatalogu::Nowa() {
    if (rodziny_.empty()) rodziny_ = katalog_.rodziny();
    DialogKatalogu dlg(this, PozycjaKatalogu{}, rodziny_);
    if (dlg.ShowModal() != wxID_OK) return;
    const PozycjaKatalogu k = dlg.Pobierz();
    const auto id = katalog_.zapisz(k);
    if (!id) { Blad(TekstZUTF8(katalog_.pobierzBlad())); return; }
    Komunikat(TR("✓ Dodano do katalogu: ") + TekstZUTF8(k.symbol));
    Odswiez();
    Wybierz(*id);
}

void PanelKatalogu::Edytuj() {
    const PozycjaKatalogu* w = Wybrana();
    if (!w) return;
    auto pelna = katalog_.pobierzPozycje(w->id);
    if (!pelna) return;
    DialogKatalogu dlg(this, *pelna, katalog_.rodziny());
    if (dlg.ShowModal() != wxID_OK) return;
    PozycjaKatalogu k = dlg.Pobierz();
    k.id = pelna->id;
    const auto id = katalog_.zapisz(k);
    if (!id) { Blad(TekstZUTF8(katalog_.pobierzBlad())); return; }
    Komunikat(TR("✓ Zapisano: ") + TekstZUTF8(k.symbol));
    Odswiez();
    Wybierz(*id);
}

std::vector<std::pair<PozycjaKatalogu, wxString>> PanelKatalogu::WybraneZDatasheetem() const {
    std::vector<std::pair<PozycjaKatalogu, wxString>> wynik;
    for (long i = widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); i >= 0;
         i = widok_->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) {
        if (static_cast<size_t>(i) >= lista_.size()) continue;
        const PozycjaKatalogu& p = lista_[static_cast<size_t>(i)];
        if (WyszukiwanieDatasheet::czyPobrany(KatalogDokumentow(), p.symbol))
            wynik.emplace_back(p, WyszukiwanieDatasheet::sciezkaPliku(KatalogDokumentow(), p.symbol));
    }
    return wynik;
}

std::unique_ptr<wxMenu> PanelKatalogu::MenuKontekstowe() {
    if (LiczbaZaznaczonych() == 0) return nullptr;
    auto menu = std::make_unique<wxMenu>();
    menu->Append(ID_K_MENU_USUN, LiczbaZaznaczonych() == 1 ? TR("Usuń z katalogu") : TR("Usuń zaznaczone z katalogu"));
    menu->AppendSeparator();
    menu->Append(ID_K_MENU_OCR, TR("Wykonaj OCR datasheetu"))->Enable(!WybraneZDatasheetem().empty());
    menu->Bind(wxEVT_MENU, [this](wxCommandEvent&) { Usun(); }, ID_K_MENU_USUN);
    menu->Bind(wxEVT_MENU, [this](wxCommandEvent&) { OcrDatasheetow(); }, ID_K_MENU_OCR);
    return menu;
}

int PanelKatalogu::OcrDatasheetow(OcrPliku::Tryb tryb, bool bezPytan) {
    const auto cele = WybraneZDatasheetem();
    if (cele.empty()) {
        if (!bezPytan) wxMessageBox(TR("Zaznacz pozycje, które mają pobrany datasheet (plik PDF)."), TR("Datasheet"), wxOK | wxICON_INFORMATION, this);
        return -1;
    }
    OcrPliku::Ustawienia ust;
    wxString problem;
    if (!OcrPliku::Przygotuj(ust, problem)) {
        if (bezPytan) Komunikat(TR("✗ ") + problem); else Blad(problem);
        return -1;
    }
    if (tryb == OcrPliku::Tryb::Pytaj) {
        if (bezPytan) tryb = OcrPliku::Tryb::Obok;
        else if (!OcrPliku::WybierzTryb(this, cele.size(), tryb)) return -1;
    }
    int wykonano = 0;
    std::string bledy;
    for (size_t i = 0; i < cele.size(); ++i) {
        std::string blad;
        const auto w = OcrPliku::Wykonaj(this, ust, cele[i].second.utf8_string(), TekstZUTF8(cele[i].first.symbol), i + 1, cele.size(),
                                         tryb == OcrPliku::Tryb::Zastap, bezPytan, blad);
        if (w == OcrPliku::Wynik::Przerwano) { bledy += "przerwano\n"; break; }
        if (w == OcrPliku::Wynik::Blad) { bledy += cele[i].first.symbol + ": " + blad + "\n"; continue; }
        ++wykonano;
    }
    Komunikat(wxString::Format(TR("✓ OCR datasheetów wykonano dla plików: %d z %zu"), wykonano, cele.size()) + (bledy.empty() ? wxString() : TR(" (są błędy)")) +
              (wykonano > 0 && tryb == OcrPliku::Tryb::Zastap ? TR(". Teraz możesz odczytać parametry z datasheetu.") : wxString()));
    if (!bledy.empty() && !bezPytan) wxMessageBox(TR("Nie wszystkie pliki zostały przetworzone:\n\n") + TekstZUTF8(bledy), TR("OCR"), wxOK | wxICON_WARNING, this);
    PokazSzczegoly();
    return wykonano;
}

int PanelKatalogu::Usun(bool bezPytan) {
    std::vector<std::pair<long long, wxString>> cele;
    for (long i = widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); i >= 0;
         i = widok_->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
        if (static_cast<size_t>(i) < lista_.size()) cele.emplace_back(lista_[static_cast<size_t>(i)].id, TekstZUTF8(lista_[static_cast<size_t>(i)].symbol));
    if (cele.empty()) return 0;
    if (!bezPytan) {
        const wxString pytanie = cele.size() == 1
            ? TR("Usunąć z katalogu \"") + cele[0].second + TR("\"? Elementy w magazynie zostaną nietknięte.")
            : wxString::Format(TR("Usunąć z katalogu zaznaczone pozycje (%zu)? Elementy w magazynie zostaną nietknięte."), cele.size());
        if (wxMessageBox(pytanie, TR("Katalog"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return 0;
    }
    int usunieto = 0;
    for (const auto& c : cele) {
        if (katalog_.usun(c.first)) ++usunieto;
        else { Blad(TekstZUTF8(katalog_.pobierzBlad())); break; }
    }
    Komunikat(wxString::Format(TR("Usunięto z katalogu pozycji: %d"), usunieto));
    Odswiez();
    return usunieto;
}

void PanelKatalogu::DodajDoMagazynu() {
    const PozycjaKatalogu* w = Wybrana();
    if (!w) return;
    const long long id = w->id;
    const wxString symbol = TekstZUTF8(w->symbol);
    DialogDoMagazynu dlg(this, magazyn_, symbol);
    if (dlg.ShowModal() != wxID_OK) return;
    const auto nowy = katalog_.dodajDoMagazynu(magazyn_, id, dlg.Ilosc(), dlg.Miejsce(), dlg.WybranaKategoria());
    if (!nowy) { Blad(TekstZUTF8(katalog_.pobierzBlad())); return; }
    Komunikat(TR("✓ Dodano do magazynu: ") + symbol);
    if (naZmianaMagazynu) naZmianaMagazynu();
    Odswiez();
    Wybierz(id);
}

int PanelKatalogu::ImportujCSV(const wxString& sciezkaWe, TrybKatalogu tryb) {
    wxString sciezka = sciezkaWe;
    if (sciezka.empty()) {
        wxFileDialog dlg(this, TR("Importuj katalog z CSV"), wxEmptyString, wxEmptyString,
                         TR("Pliki CSV (*.csv;*.txt)|*.csv;*.txt|Wszystkie pliki|*.*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) return -1;
        sciezka = dlg.GetPath();
        wxMessageDialog pytanie(this, TR("Co zrobić z pozycjami, które już są w katalogu (ten sam symbol i producent)?"),
                                TR("Import katalogu"), wxYES_NO | wxCANCEL | wxICON_QUESTION);
        pytanie.SetYesNoCancelLabels(TR("Zaktualizuj"), TR("Pomiń"), TR("Anuluj"));
        const int o = pytanie.ShowModal();
        if (o == wxID_CANCEL) return -1;
        tryb = o == wxID_YES ? TrybKatalogu::Aktualizuj : TrybKatalogu::Pomin;
    }
    std::string tekst;
    if (!WczytajTekst(sciezka, tekst)) { Blad(TR("Nie można odczytać pliku.")); return -1; }
    const auto wczytane = KatalogCSV::parsuj(tekst);
    if (!wczytane.blad.empty()) { Blad(TekstZUTF8(wczytane.blad)); return -1; }
    WynikImportuKatalogu w;
    if (!katalog_.importuj(wczytane.pozycje, tryb, w)) { Blad(TekstZUTF8(katalog_.pobierzBlad())); return -1; }
    wxString msg = wxString::Format(TR("Import katalogu: dodano %d, zaktualizowano %d, pominięto %d"), w.dodano, w.zaktualizowano, w.pominieto);
    if (!w.bledy.empty()) msg += wxString::Format(TR(", błędów: %d"), static_cast<int>(w.bledy.size()));
    Komunikat(msg);
    for (size_t i = 0; i < w.bledy.size() && i < 10; ++i) Komunikat("  " + TekstZUTF8(w.bledy[i]));
    Odswiez();
    return w.dodano + w.zaktualizowano;
}

bool PanelKatalogu::EksportujCSV(const wxString& sciezkaWe) {
    wxString sciezka = sciezkaWe;
    if (sciezka.empty()) {
        wxFileDialog dlg(this, TR("Eksport katalogu do CSV"), wxEmptyString, "katalog_" + wxDateTime::Now().Format("%Y%m%d") + ".csv",
                         TR("Pliki CSV (*.csv)|*.csv"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        sciezka = dlg.GetPath();
    }
    FiltrKatalogu f;
    f.limit = 1000000;
    std::vector<PozycjaKatalogu> wszystkie;
    if (!katalog_.pobierz(f, wszystkie)) { Blad(TekstZUTF8(katalog_.pobierzBlad())); return false; }
    for (PozycjaKatalogu& p : wszystkie)
        if (auto pelna = katalog_.pobierzPozycje(p.id)) p.parametry = pelna->parametry;
    if (!ZapiszTekst(sciezka, KatalogCSV::naCSV(wszystkie))) { Blad(TR("Nie udało się zapisać pliku.")); return false; }
    Komunikat(wxString::Format(TR("✓ Wyeksportowano %d pozycji katalogu."), static_cast<int>(wszystkie.size())));
    return true;
}

bool PanelKatalogu::ZapiszSzablon(const wxString& sciezkaWe) {
    wxString sciezka = sciezkaWe;
    if (sciezka.empty()) {
        wxFileDialog dlg(this, TR("Zapisz szablon katalogu"), wxEmptyString, "katalog_szablon.csv",
                         TR("Pliki CSV (*.csv)|*.csv"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        sciezka = dlg.GetPath();
    }
    if (!ZapiszTekst(sciezka, KatalogCSV::szablon())) { Blad(TR("Nie udało się zapisać pliku.")); return false; }
    Komunikat(TR("✓ Zapisano szablon katalogu."));
    return true;
}

void PanelKatalogu::OtworzDatasheet() {
    const PozycjaKatalogu* w = Wybrana();
    if (!w) return;
    wxString plik = WyszukiwanieDatasheet::sciezkaPliku(KatalogDokumentow(), w->symbol);
    if (WyszukiwanieDatasheet::czyPobrany(KatalogDokumentow(), w->symbol)) {
        if (!WyszukiwanieDatasheet::otworz(plik))
            wxMessageBox(TR("Nie znaleziono programu do otwierania plików PDF."), TR("Datasheet"), wxOK | wxICON_WARNING, this);
    } else if (!w->datasheetUrl.empty()) {
        wxLaunchDefaultBrowser(TekstZUTF8(w->datasheetUrl));
    } else {
        wxMessageBox(TR("Ta pozycja nie ma datasheetu. Wpisz adres w edycji pozycji albo użyj \"Pobierz datasheet\"."),
                     TR("Datasheet"), wxOK | wxICON_INFORMATION, this);
    }
}

bool PanelKatalogu::ZastosujWynikMouser(const Mouser::Czesc& c) {
    const PozycjaKatalogu* w = Wybrana();
    if (!w) return false;
    auto p = katalog_.pobierzPozycje(w->id);
    if (!p) return false;
    if (p->producent.empty()) p->producent = c.producent;
    if (p->opis.empty()) p->opis = c.opis;
    if (p->obudowa.empty()) p->obudowa = c.obudowa();
    if (!c.adresDatasheetu.empty()) p->datasheetUrl = c.adresDatasheetu;
    if (!katalog_.zapisz(*p)) { Blad(TekstZUTF8(katalog_.pobierzBlad())); return false; }
    Odswiez();
    Wybierz(p->id);
    return true;
}

void PanelKatalogu::PobierzZAdresu(long long id, const std::string& url, const std::string& symbol, bool otworzPrzyBledzie) {
    Status(TR("Pobieranie datasheetu..."));
    KlientHTTP::pobierzPlik(url, WyszukiwanieDatasheet::sciezkaPliku(KatalogDokumentow(), symbol), [this, id, url, symbol, otworzPrzyBledzie](const WynikHTTP& w) {
        const wxString sciezka = WyszukiwanieDatasheet::sciezkaPliku(KatalogDokumentow(), symbol);
        if (w.ok) {
            if (auto p = katalog_.pobierzPozycje(id)) {
                p->datasheetUrl = url;
                katalog_.zapisz(*p);
            }
            katalog_.ustawDatasheetPlik(id, sciezka.utf8_string());
            Komunikat(TR("✓ Pobrano datasheet: ") + TekstZUTF8(symbol));
        } else {
            Komunikat(TR("✗ Nie udało się pobrać datasheetu: ") + TekstZUTF8(w.blad));
            if (otworzPrzyBledzie) {
                wxLaunchDefaultBrowser(TekstZUTF8(url));
                wxMessageBox(TR("Serwer nie pozwolił pobrać pliku automatycznie, więc otworzyłem datasheet w przeglądarce.\n\n"
                                "Zapisz go w tym miejscu, a program go znajdzie:\n") + sciezka,
                             TR("Datasheet"), wxOK | wxICON_INFORMATION, this);
            }
        }
        Status(wxEmptyString);
        Odswiez();
    });
}

void PanelKatalogu::PobierzDatasheet() {
    const PozycjaKatalogu* w = Wybrana();
    if (!w) return;
    const long long id = w->id;
    const std::string symbol = w->symbol;
    const std::string klucz = Konfiguracja::pobierz().pobierz("datasheet", "mouser_klucz", "");
    if (!klucz.empty()) {
        DialogDatasheetMouser dlg(this, TekstZUTF8(symbol), klucz);
        dlg.Szukaj();
        const int wynik = dlg.ShowModal();
        if (wynik == wxID_OK && dlg.Wybrana()) {
            const Mouser::Czesc czesc = *dlg.Wybrana();
            Wybierz(id);
            if (!ZastosujWynikMouser(czesc)) return;
            if (czesc.adresDatasheetu.empty()) {
                wxMessageBox(TR("Mouser nie podaje datasheetu dla tej części. Dane (producent, opis, obudowa) zostały uzupełnione."),
                             TR("Datasheet"), wxOK | wxICON_INFORMATION, this);
                return;
            }
            PobierzZAdresu(id, czesc.adresDatasheetu, symbol, true);
            return;
        }
        if (wynik != wxID_EDIT) return;   // anulowano; wxID_EDIT = użytkownik chce wpisać adres sam
    }
    wxTextEntryDialog dlg(this, TR("Adres pliku PDF z datasheetem:"), TR("Pobierz datasheet"), TekstZUTF8(w->datasheetUrl));
    if (dlg.ShowModal() != wxID_OK) return;
    const std::string url = dlg.GetValue().Strip(wxString::both).utf8_string();
    if (url.empty()) return;
    PobierzZAdresu(id, url, symbol, false);
}

int PanelKatalogu::OdczytajDatasheet(const wxString& sciezkaWe, bool bezPytania, bool bezOcr) {
    const PozycjaKatalogu* w = Wybrana();
    if (!w) return -1;
    const long long id = w->id;
    wxString sciezka = sciezkaWe;
    if (sciezka.empty()) {
        if (WyszukiwanieDatasheet::czyPobrany(KatalogDokumentow(), w->symbol)) {
            sciezka = WyszukiwanieDatasheet::sciezkaPliku(KatalogDokumentow(), w->symbol);
        } else {
            wxFileDialog dlg(this, TR("Wybierz plik PDF z datasheetem"), KatalogDokumentow(), wxEmptyString,
                             TR("Pliki PDF (*.pdf)|*.pdf"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            if (dlg.ShowModal() != wxID_OK) return -1;
            sciezka = dlg.GetPath();
        }
    }
    std::string blad;
    const std::string tekst = OdczytDatasheetu::tekstZPDF(sciezka.utf8_string(), blad);
    if (tekst.size() < 80 && !bezPytania && !bezOcr && WyszukiwanieDatasheet::czyPobrany(KatalogDokumentow(), w->symbol) &&
        sciezka == WyszukiwanieDatasheet::sciezkaPliku(KatalogDokumentow(), w->symbol)) {
        // plik bez warstwy tekstowej to najczęściej skan: zaproponuj OCR i odczytaj jeszcze raz
        if (wxMessageBox(TR("Ten PDF nie ma warstwy tekstowej (wygląda na skan). Wykonać OCR i odczytać parametry jeszcze raz?\n\nOryginał zostanie zachowany jako plik.pdf.bak."),
                         TR("Datasheet"), wxYES_NO | wxICON_QUESTION, this) == wxYES) {
            if (OcrDatasheetow(OcrPliku::Tryb::Zastap, false) > 0) return OdczytajDatasheet(sciezkaWe, bezPytania, true);
            return -1;
        }
    }
    if (tekst.empty()) {
        if (bezPytania) { Komunikat(TR("✗ Odczyt datasheetu: ") + TekstZUTF8(blad)); return -1; }
        Blad(TR("Nie udało się odczytać tekstu z PDF: ") + TekstZUTF8(blad));
        return -1;
    }
    const OdczytDatasheetu::Wynik wynik = OdczytDatasheetu::wyciagnij(tekst);
    if (wynik.parametry.empty() && wynik.opis.empty() && wynik.obudowa.empty() && wynik.uwagi.empty()) {
        if (!bezPytania)
            wxMessageBox(TR("Nie znalazłem w tym pliku żadnych parametrów, które program umie rozpoznać."),
                         TR("Datasheet"), wxOK | wxICON_INFORMATION, this);
        return 0;
    }
    auto pelna = katalog_.pobierzPozycje(id);
    if (!pelna) return -1;
    std::vector<ParametrKatalogu> wybrane;
    bool opis = false, obudowa = false, uwagi = false;
    if (bezPytania) {
        for (const auto& p : wynik.parametry) if (p.pewny) wybrane.push_back({p.nazwa, p.wartosc, "datasheet"});
        uwagi = pelna->uwagi.empty() && !wynik.uwagi.empty();
        opis = pelna->opis.empty() && !wynik.opis.empty();
        obudowa = pelna->obudowa.empty() && !wynik.obudowa.empty();
    } else {
        DialogPropozycji dlg(this, wynik, *pelna);
        if (dlg.ShowModal() != wxID_OK) return -1;
        wybrane = dlg.Wybrane();
        opis = dlg.UstawOpis();
        obudowa = dlg.UstawObudowe();
        uwagi = dlg.DopiszUwagi();
    }
    if (opis) pelna->opis = wynik.opis;
    if (obudowa) pelna->obudowa = wynik.obudowa;
    if (uwagi) pelna->uwagi = pelna->uwagi.empty() ? wynik.uwagi : pelna->uwagi + "\n" + wynik.uwagi;
    if (opis || obudowa || uwagi) {
        // zapisz tylko pola główne, parametry dopisze dodajParametry (nie kasując ręcznych)
        if (!katalog_.zapisz(*pelna)) { Blad(TekstZUTF8(katalog_.pobierzBlad())); return -1; }
    }
    if (!wybrane.empty() && !katalog_.dodajParametry(id, wybrane, true)) {
        Blad(TekstZUTF8(katalog_.pobierzBlad()));
        return -1;
    }
    Komunikat(wxString::Format(TR("✓ Z datasheetu zapisano parametrów: %d"), static_cast<int>(wybrane.size())));
    Odswiez();
    Wybierz(id);
    return static_cast<int>(wybrane.size());
}
