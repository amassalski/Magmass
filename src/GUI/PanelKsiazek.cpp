#include "PanelKsiazek.h"

#include "DialogPozyczki.h"
#include "DialogKsiazki.h"
#include "../Narzedzia/UwagiHTML.h"
#include "Ikony.h"
#include "UstawieniaOkna.h"
#include "../Logika/KsiazkiCSV.h"
#include "../Import/MetadaneKsiazek.h"
#include "../Logika/StronyPDF.h"
#include "../Logika/StronyKsiazki.h"
#include "../Logika/ProgramyZewnetrzne.h"
#include "OcrPliku.h"
#include "../Narzedzia/PolecenieKsiazek.h"
#include "../Logika/LogikaNarzedzi.h"
#include "../Narzedzia/Konfiguracja.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

#include <wx/config.h>
#include <wx/dirdlg.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/html/htmlwin.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/progdlg.h>
#include <wx/spinctrl.h>
#include <wx/utils.h>
#include <wx/wrapsizer.h>

#include <memory>

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

bool JestPDF(const std::string& sciezka) {
    return wxFileName(wxString::FromUTF8(sciezka)).GetExt().Lower() == "pdf";
}

/// Otwiera plik książki. Dla PDF i strony > 0 próbuje programu, który umie otworzyć stronę:
/// polecenie z ustawień (Ustawienia / Inne, config.ini literatura.polecenie_pdf, np. "AcroRd32.exe /A page=%p \"%f\""; %u = ścieżka jako URL) albo znany czytnik z PATH.
bool OtworzPlik(const std::string& sciezka, int strona) {
    const wxString plik = wxString::FromUTF8(sciezka);
    if (!wxFileName::FileExists(plik)) return false;
    if (JestPDF(sciezka)) {
        // polecenie z ustawień działa też bez numeru strony (wtedy %p = 1), żeby PDF zawsze otwierał ten sam czytnik
        wxString szablon = TekstZUTF8(Konfiguracja::pobierz().pobierz("literatura", "polecenie_pdf", ""));
        if (szablon.empty()) wxConfigBase::Get()->Read("/Ksiazki/PolecenieStrony", &szablon);   // starszy zapis
        if (!szablon.empty()) {
            ProgramyZewnetrzne::DopiszTypoweKatalogiDoPATH();
            const wxString cmd = wxString::FromUTF8(PolecenieKsiazek::wypelnijPolecenieCzytnika(szablon.utf8_string(), sciezka, strona > 0 ? strona : 1));
            if (wxExecute(cmd, wxEXEC_ASYNC) != 0) return true;
        }
    }
    if (strona > 0 && JestPDF(sciezka)) {
        for (const char* program : {"evince", "okular", "zathura", "atril", "xpdf", "mupdf"}) {
            wxString sciezkaProgramu = wxGetenv("PATH");
            wxPathList lista;
            lista.AddEnvList("PATH");
            if (lista.FindAbsoluteValidPath(program).empty()) continue;
            const auto arg = StronyPDF::argumentyOtwarciaNaStronie(program, sciezka, strona);
            if (arg.empty()) continue;
            wxString cmd;
            for (const std::string& a : arg) cmd += (cmd.empty() ? "" : " ") + wxString("\"") + wxString::FromUTF8(a) + "\"";
            if (wxExecute(cmd, wxEXEC_ASYNC) != 0) return true;
        }
    }
    return wxLaunchDefaultApplication(plik);
}

/// Wpisuje do książki dane odczytane z pliku, ale tylko w puste pola
void UzupelnijZPliku(Ksiazka& k, const MetadaneKsiazek::Dane& d) {
    if (k.tytul.empty()) k.tytul = d.tytul;
    if (k.autor.empty()) k.autor = d.autor;
    if (k.wydawca.empty()) k.wydawca = d.wydawca;
    if (k.rok == 0) k.rok = d.rok;
    if (k.wydanie.empty()) k.wydanie = d.wydanie;
    if (k.isbn.empty()) k.isbn = d.isbn;
    if (k.jezyk.empty()) k.jezyk = d.jezyk;
    if (k.tagi.empty()) k.tagi = d.tagi;
    if (k.uwagi.empty()) k.uwagi = d.opis;
}

}  // namespace

PanelKsiazek::PanelKsiazek(wxWindow* parent, LogikaKsiazek& logika) : wxPanel(parent), logika_(logika) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxWrapSizer* gora = new wxWrapSizer(wxHORIZONTAL);
    szukaj_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(260, -1));
    szukaj_->SetDescriptiveText(TR("Szukaj książki (tytuł, autor, tagi)..."));
    PomocGUI::Dodaj(szukaj_, "ksiazki.szukaj");
    gora->Add(szukaj_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    szukajTresci_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(300, -1), wxTE_PROCESS_ENTER);
    szukajTresci_->SetDescriptiveText(TR("Szukaj w treści książek (Enter)..."));
    szukajTresci_->ShowSearchButton(true);
    PomocGUI::Dodaj(szukajTresci_, "ksiazki.szukaj_tresci");
    gora->Add(szukajTresci_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    cbZaznaczona_ = new wxCheckBox(this, wxID_ANY, TR("tylko w zaznaczonej książce"));
    gora->Add(cbZaznaczona_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    licznik_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    gora->Add(licznik_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 4);

    wxSplitterWindow* podzial = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);
    widok_ = new wxListCtrl(podzial, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
    const struct { const char* t; int s; } kol[] = {{"Tytuł", 280}, {"Autor", 170}, {"Rok", 50}, {"Wydawca", 130}, {"Plik", 50}, {"Indeks (str.)", 80}, {"Pożyczona", 150}};
    for (int i = 0; i < K_LICZBA; ++i)
        widok_->InsertColumn(i, TR(kol[i].t), (i == K_ROK || i == K_STRONY) ? wxLIST_FORMAT_RIGHT : wxLIST_FORMAT_LEFT, kol[i].s);
    PomocGUI::Dodaj(widok_, "ksiazki.lista");

    wxPanel* prawy = new wxPanel(podzial);
    wxBoxSizer* ps = new wxBoxSizer(wxVERTICAL);
    naglowek_ = new wxStaticText(prawy, wxID_ANY, TR("Wybierz książkę z listy"));
    wxFont f = naglowek_->GetFont(); f.MakeBold(); f.SetPointSize(f.GetPointSize() + 2);
    naglowek_->SetFont(f);
    ps->Add(naglowek_, 0, wxALL, 8);
    szczegoly_ = new wxStaticText(prawy, wxID_ANY, wxEmptyString);
    ps->Add(szczegoly_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    ps->Add(new wxStaticText(prawy, wxID_ANY, TR("Wyniki szukania w treści (dwuklik otwiera książkę na stronie):")), 0, wxLEFT | wxRIGHT, 8);
    widok_->Bind(wxEVT_LIST_COL_CLICK, [this](wxListEvent& e) {
        sort_.Klik(e.GetColumn());
        Odswiez();
    });
    wyniki_ = new wxListCtrl(prawy, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_THEME);
    wyniki_->InsertColumn(0, TR("Książka"), wxLIST_FORMAT_LEFT, 170);
    wyniki_->InsertColumn(1, TR("Str."), wxLIST_FORMAT_RIGHT, 45);
    wyniki_->InsertColumn(2, TR("Fragment"), wxLIST_FORMAT_LEFT, 420);
    PomocGUI::Dodaj(wyniki_, "ksiazki.wyniki");
    ps->Add(wyniki_, 1, wxEXPAND | wxALL, 8);
    UstawieniaOkna::WczytajKolumny(widok_, "Ksiazki");
    UstawieniaOkna::WczytajKolumny(wyniki_, "KsiazkiWyniki");
    Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& e) {
        if (e.GetEventObject() == this) {
            UstawieniaOkna::ZapiszKolumny(widok_, "Ksiazki");
            UstawieniaOkna::ZapiszKolumny(wyniki_, "KsiazkiWyniki");
        }
        e.Skip();
    });
    prawy->SetSizer(ps);
    podzial->SplitVertically(widok_, prawy, -480);
    podzial->SetMinimumPaneSize(200);
    podzial->SetSashGravity(1.0);
    glowny->Add(podzial, 1, wxEXPAND | wxLEFT | wxRIGHT, 6);

    wxWrapSizer* przyciski = new wxWrapSizer(wxHORIZONTAL);
    auto przycisk = [&](int id, const wxString& t) {
        wxButton* b = new wxButton(this, id, t);
        przyciski->Add(b, 0, wxALL, 3);
        return b;
    };
    Ikony::DlaPrzycisku(przycisk(ID_KS_NOWA, TR("Nowa...")), "nowy");
    wxButton* bEdytuj = przycisk(ID_KS_EDYTUJ, TR("Edytuj..."));
    Ikony::DlaPrzycisku(bEdytuj, "edytuj");
    wxButton* bUsun = przycisk(ID_KS_USUN, TR("Usuń"));
    Ikony::DlaPrzycisku(bUsun, "usun");
    wxButton* bOtworz = przycisk(ID_KS_OTWORZ, TR("Otwórz plik"));
    wxButton* bPozycz = przycisk(ID_KS_POZYCZ, TR("Pożycz..."));
    wxButton* bZwroc = przycisk(ID_KS_ZWROC, TR("Zwrot"));
    PomocGUI::Dodaj(bPozycz, "ksiazki.pozycz");
    wxButton* bIndeks = przycisk(ID_KS_INDEKSUJ, TR("Indeksuj treść"));
    PomocGUI::Dodaj(bIndeks, "ksiazki.indeksuj");
    wxButton* bWszystkie = przycisk(ID_KS_INDEKSUJ_WSZYSTKIE, TR("Indeksuj wszystkie"));
    wxButton* bMeta = przycisk(ID_KS_ZAPISZ_META, TR("Zapisz dane do PDF"));
    PomocGUI::Dodaj(bMeta, "ksiazki.zapisz_meta");
    wxButton* bOcr = przycisk(ID_KS_OCR, TR("OCR..."));
    PomocGUI::Dodaj(bOcr, "ksiazki.ocr");
    przycisk(ID_KS_KATALOG, TR("Dodaj z katalogu na dysku..."));
    przycisk(ID_KS_IMPORT, TR("Import CSV..."));
    przycisk(ID_KS_EKSPORT, TR("Eksport CSV..."));
    przycisk(ID_KS_SZABLON, TR("Szablon CSV..."));
    glowny->Add(przyciski, 0, wxEXPAND | wxALL, 4);
    SetSizer(glowny);
    (void)bWszystkie;

    for (wxButton* b : {bEdytuj, bUsun})
        b->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(Wybrana() != nullptr); });
    // akcje na jednej książce: tylko przy dokładnie jednym zaznaczeniu
    auto jedna = [this] { return LiczbaZaznaczonych() == 1 ? Wybrana() : nullptr; };
    bPozycz->Bind(wxEVT_UPDATE_UI, [jedna](wxUpdateUIEvent& e) { const Ksiazka* k = jedna(); e.Enable(k && !k->pozyczona); });
    bZwroc->Bind(wxEVT_UPDATE_UI, [jedna](wxUpdateUIEvent& e) { const Ksiazka* k = jedna(); e.Enable(k && k->pozyczona); });
    bOtworz->Bind(wxEVT_UPDATE_UI, [jedna](wxUpdateUIEvent& e) { const Ksiazka* k = jedna(); e.Enable(k && !k->sciezka.empty()); });
    bIndeks->Bind(wxEVT_UPDATE_UI, [jedna](wxUpdateUIEvent& e) { const Ksiazka* k = jedna(); e.Enable(k && !k->sciezka.empty() && StronyKsiazki::czyMoznaIndeksowac(k->sciezka)); });
    for (wxButton* b : {bMeta, bOcr})   // wymagają zaznaczonych książek z istniejącym plikiem PDF
        b->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Enable(!WybraneZPdf().empty()); });

    szukaj_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { if (!odswiezanie_) Odswiez(); });
    szukajTresci_->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { NaWyszukiwanieTresci(); });
    szukajTresci_->Bind(wxEVT_SEARCH, [this](wxCommandEvent&) { NaWyszukiwanieTresci(); });
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
    widok_->Bind(wxEVT_LIST_KEY_DOWN, [this](wxListEvent& e) {
        if (e.GetKeyCode() == WXK_DELETE) UsunZaznaczone();
        else if ((e.GetKeyCode() == 'A') && wxGetKeyState(WXK_CONTROL)) ZaznaczWszystkie();
        else e.Skip();
    });
    wyniki_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& e) {
        const long i = e.GetIndex();
        if (i < 0 || static_cast<size_t>(i) >= trafienia_.size()) return;
        const TrafienieKsiazki& t = trafienia_[static_cast<size_t>(i)];
        if (t.sciezka.empty() || !OtworzPlik(t.sciezka, t.strona))
            wxMessageBox(TR("Nie można otworzyć pliku książki (sprawdź ścieżkę)."), TR("Literatura"), wxOK | wxICON_WARNING, this);
        else
            Status(wxString::Format(TR("Otwarto: %s, strona %d"), TekstZUTF8(t.tytul), t.strona));
    });
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Nowa(); }, ID_KS_NOWA);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Edytuj(); }, ID_KS_EDYTUJ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Usun(); }, ID_KS_USUN);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Otworz(); }, ID_KS_OTWORZ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Pozycz(); }, ID_KS_POZYCZ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Zwroc(); }, ID_KS_ZWROC);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ZapiszMetadaneDoPliku(); }, ID_KS_ZAPISZ_META);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Ocr(); }, ID_KS_OCR);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Indeksuj(); }, ID_KS_INDEKSUJ);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { IndeksujWszystkie(); }, ID_KS_INDEKSUJ_WSZYSTKIE);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DodajZKatalogu(); }, ID_KS_KATALOG);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ImportujCSV(); }, ID_KS_IMPORT);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EksportujCSV(); }, ID_KS_EKSPORT);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ZapiszSzablon(); }, ID_KS_SZABLON);
}

void PanelKsiazek::Blad(const wxString& t) {
    Komunikat(TR("✗ ") + t);
    wxMessageBox(t, TR("Literatura"), wxOK | wxICON_WARNING, this);
}

const Ksiazka* PanelKsiazek::Wybrana() const {
    const long i = widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (i < 0 || static_cast<size_t>(i) >= lista_.size()) return nullptr;
    return &lista_[static_cast<size_t>(i)];
}

std::vector<long long> PanelKsiazek::ZaznaczoneId() const {
    std::vector<long long> idki;
    for (long i = widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); i >= 0;
         i = widok_->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
        if (static_cast<size_t>(i) < lista_.size()) idki.push_back(lista_[static_cast<size_t>(i)].id);
    return idki;
}

int PanelKsiazek::LiczbaZaznaczonych() const { return widok_->GetSelectedItemCount(); }

void PanelKsiazek::ZaznaczWszystkie() {
    for (long i = 0; i < widok_->GetItemCount(); ++i) widok_->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

void PanelKsiazek::ZaznaczZakres(long od, long do_) {
    if (od > do_) std::swap(od, do_);
    for (long i = 0; i < widok_->GetItemCount(); ++i)
        widok_->SetItemState(i, (i >= od && i <= do_) ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED);
}

void PanelKsiazek::Wybierz(long long id) {
    for (size_t i = 0; i < lista_.size(); ++i)
        if (lista_[i].id == id) {
            widok_->SetItemState(-1, 0, wxLIST_STATE_SELECTED);   // zaznaczenie tylko tej książki
            widok_->SetItemState(static_cast<long>(i), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            widok_->EnsureVisible(static_cast<long>(i));
            return;
        }
}

void PanelKsiazek::Szukaj(const std::string& tekst) { szukaj_->ChangeValue(TekstZUTF8(tekst)); Odswiez(); }

void PanelKsiazek::Wyczysc() {
    lista_.clear();
    trafienia_.clear();
    widok_->DeleteAllItems();
    wyniki_->DeleteAllItems();
    PokazSzczegoly();
    licznik_->SetLabel(wxEmptyString);
}

wxString PanelKsiazek::NaglowekKolumny(int kolumna) const {
    wxListItem it;
    it.SetMask(wxLIST_MASK_TEXT);
    widok_->GetColumn(kolumna, it);
    return it.GetText();
}

void PanelKsiazek::Odswiez() {
    odswiezanie_ = true;
    const long long zachowaj = Wybrana() ? Wybrana()->id : 0;
    widok_->DeleteAllItems();
    if (!logika_.pobierz(szukaj_->GetValue().Strip(wxString::both).utf8_string(), lista_)) {
        Status(TR("✗ Literatura: ") + TekstZUTF8(logika_.pobierzBlad()));
        lista_.clear();
    }
    SortowanieList::Posortuj(lista_, sort_, [](const Ksiazka& a, const Ksiazka& b, int kol) {
        auto rodzaj = [](const Ksiazka& k) {
            return k.sciezka.empty() ? std::string() : wxFileName(wxString::FromUTF8(k.sciezka.c_str())).GetExt().utf8_string();
        };
        switch (kol) {
            case K_TYTUL: return SortowanieList::PorownajTekst(a.tytul, b.tytul) < 0;
            case K_AUTOR: return SortowanieList::PorownajTekst(a.autor, b.autor) < 0;
            case K_ROK: return a.rok < b.rok;
            case K_WYDAWCA: return SortowanieList::PorownajTekst(a.wydawca, b.wydawca) < 0;
            case K_PLIK: return SortowanieList::PorownajTekst(rodzaj(a), rodzaj(b)) < 0;
            case K_STRONY: return a.strony < b.strony;
            case K_POZYCZONA:
                return SortowanieList::PorownajTekst(a.pozyczona ? a.pozyczonaPrzez : std::string(),
                                                     b.pozyczona ? b.pozyczonaPrzez : std::string()) < 0;
            default: return false;
        }
    });
    SortowanieList::OznaczNaglowek(widok_, sort_);
    for (size_t i = 0; i < lista_.size(); ++i) {
        const Ksiazka& k = lista_[i];
        const long w = widok_->InsertItem(static_cast<long>(i), TekstZUTF8(k.tytul));
        widok_->SetItem(w, K_AUTOR, TekstZUTF8(k.autor));
        widok_->SetItem(w, K_ROK, k.rok > 0 ? wxString::Format("%d", k.rok) : wxString());
        widok_->SetItem(w, K_WYDAWCA, TekstZUTF8(k.wydawca));
        wxString rodzaj;
        if (!k.sciezka.empty()) rodzaj = wxFileName(TekstZUTF8(k.sciezka)).GetExt().Upper();
        widok_->SetItem(w, K_PLIK, rodzaj);
        widok_->SetItem(w, K_STRONY, k.strony > 0 ? wxString::Format("%d", k.strony) : wxString());
        if (k.pozyczona) {
            wxString p = TekstZUTF8(k.pozyczonaPrzez);
            if (!k.termin.empty()) p += TR(" (do ") + TekstZUTF8(k.termin) + ")";
            widok_->SetItem(w, K_POZYCZONA, p);
            if (k.przeterminowana) widok_->SetItemTextColour(w, wxColour(190, 30, 30));
        }
    }
    licznik_->SetLabel(wxString::Format(TR("Książek: %zu"), lista_.size()));
    odswiezanie_ = false;
    if (zachowaj) Wybierz(zachowaj);
    PokazSzczegoly();
}

void PanelKsiazek::PokazSzczegoly() {
    const Ksiazka* k = Wybrana();
    if (!k) {
        naglowek_->SetLabel(TR("Wybierz książkę z listy"));
        szczegoly_->SetLabel(wxEmptyString);
        szczegoly_->GetParent()->Layout();
        return;
    }
    naglowek_->SetLabel(TekstZUTF8(k->tytul));
    wxString t;
    auto dodaj = [&](const wxString& et, const std::string& v) { if (!v.empty()) t += et + ": " + TekstZUTF8(v) + "\n"; };
    dodaj(TR("Autor"), k->autor);
    dodaj(TR("Wydawca"), k->wydawca);
    if (k->rok > 0) t += TR("Rok") + wxString::Format(": %d\n", k->rok);
    dodaj(TR("Wydanie"), k->wydanie);
    dodaj(TR("ISBN"), k->isbn);
    dodaj(TR("Język"), k->jezyk);
    dodaj(TR("Tagi"), k->tagi);
    if (k->sciezka.empty()) t += TR("Plik: brak (książka papierowa)\n");
    else {
        t += TR("Plik: ") + TekstZUTF8(k->sciezka);
        if (!wxFileName::FileExists(TekstZUTF8(k->sciezka))) t += TR("  (nie znaleziono pliku!)");
        t += "\n";
    }
    if (k->strony > 0) t += wxString::Format(TR("Indeks treści: %d stron (%s)\n"), k->strony, TekstZUTF8(k->zaindeksowano));
    dodaj(TR("Uwagi"), UwagiHTML::naTekst(k->uwagi));
    if (k->pozyczona) {
        t += TR("Pożyczona: ") + TekstZUTF8(k->pozyczonaPrzez) + TR(", od ") + TekstZUTF8(k->pozyczonaOd);
        if (!k->termin.empty()) t += TR(", do ") + TekstZUTF8(k->termin) + (k->przeterminowana ? TR("  (PO TERMINIE)") : wxString());
        t += "\n";
    }
    std::vector<Pozyczka> hist;
    if (logika_.historia(k->id, hist) && !hist.empty()) {
        t += TR("Historia pożyczek:\n");
        for (size_t i = 0; i < hist.size() && i < 8; ++i)
            t += "  " + TekstZUTF8(hist[i].od) + " - " + (hist[i].zwrot.empty() ? TR("nadal") : TekstZUTF8(hist[i].zwrot)) + ": " + TekstZUTF8(hist[i].osoba) + "\n";
    }
    szczegoly_->SetLabel(t);
    szczegoly_->Wrap(std::max(200, szczegoly_->GetParent()->GetClientSize().GetWidth() - 20));
    szczegoly_->GetParent()->Layout();
}

void PanelKsiazek::Nowa() {
    DialogKsiazki dlg(this, Ksiazka());
    if (dlg.ShowModal() != wxID_OK) return;
    auto id = logika_.zapisz(dlg.Wynik());
    if (!id) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    Komunikat(TR("✓ Dodano książkę: ") + TekstZUTF8(dlg.Wynik().tytul));
    Odswiez();
    Wybierz(*id);
}

void PanelKsiazek::Edytuj() {
    if (LiczbaZaznaczonych() > 1) { EdytujZbiorczo(); return; }
    const Ksiazka* w = Wybrana();
    if (!w) return;
    DialogKsiazki dlg(this, *w);
    if (dlg.ShowModal() != wxID_OK) return;
    auto id = logika_.zapisz(dlg.Wynik());
    if (!id) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    Odswiez();
    Wybierz(*id);
}

void PanelKsiazek::EdytujZbiorczo() {
    const std::vector<long long> idki = ZaznaczoneId();
    if (idki.size() < 2) return;
    DialogKsiazki dlg(this, Ksiazka(), static_cast<int>(idki.size()));
    if (dlg.ShowModal() != wxID_OK) return;
    int zmieniono = 0, bledy = 0;
    logika_.zmienZbiorczo(idki, dlg.Zmiany(), &zmieniono, &bledy);
    Komunikat(wxString::Format(TR("✓ Zmieniono książek: %d"), zmieniono) + (bledy ? wxString::Format(TR(" (błędy: %d)"), bledy) : wxString()));
    Odswiez();
    for (long long id : idki) {
        for (size_t i = 0; i < lista_.size(); ++i)
            if (lista_[i].id == id) widok_->SetItemState(static_cast<long>(i), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
}

void PanelKsiazek::Usun() { UsunZaznaczone(); }

std::unique_ptr<wxMenu> PanelKsiazek::MenuKontekstowe() {
    if (LiczbaZaznaczonych() == 0) return nullptr;
    auto menu = std::make_unique<wxMenu>();
    const bool jedna = LiczbaZaznaczonych() == 1;
    bool jestIndeks = false;
    for (long long id : ZaznaczoneId())
        for (const Ksiazka& k : lista_) if (k.id == id && k.strony > 0) jestIndeks = true;
    menu->Append(ID_KS_MENU_USUN, jedna ? TR("Usuń książkę") : TR("Usuń zaznaczone książki"));
    menu->Append(ID_KS_MENU_USUN_INDEKS, TR("Usuń indeks"))->Enable(jestIndeks);
    menu->AppendSeparator();
    const bool zPdf = !WybraneZPdf().empty();
    menu->Append(ID_KS_MENU_META, TR("Zapisz metadane"))->Enable(zPdf);
    menu->Append(ID_KS_MENU_OCR, TR("Wykonaj OCR"))->Enable(zPdf);
    menu->AppendSeparator();
    menu->Append(ID_KS_MENU_INFO, TR("Info"))->Enable(jedna);
    menu->Bind(wxEVT_MENU, [this](wxCommandEvent&) { UsunZaznaczone(); }, ID_KS_MENU_USUN);
    menu->Bind(wxEVT_MENU, [this](wxCommandEvent&) { UsunIndeks(); }, ID_KS_MENU_USUN_INDEKS);
    menu->Bind(wxEVT_MENU, [this](wxCommandEvent&) { ZapiszMetadaneDoPliku(); }, ID_KS_MENU_META);
    menu->Bind(wxEVT_MENU, [this](wxCommandEvent&) { Ocr(); }, ID_KS_MENU_OCR);
    menu->Bind(wxEVT_MENU, [this](wxCommandEvent&) { PokazInfo(); }, ID_KS_MENU_INFO);
    return menu;
}

int PanelKsiazek::UsunIndeks(bool bezPytan) {
    std::vector<long long> idki;
    for (long long id : ZaznaczoneId())
        for (const Ksiazka& k : lista_) if (k.id == id && k.strony > 0) idki.push_back(id);
    if (idki.empty()) {
        if (!bezPytan) wxMessageBox(TR("Zaznaczone książki nie mają indeksu treści."), TR("Literatura"), wxOK | wxICON_INFORMATION, this);
        return 0;
    }
    if (!bezPytan && wxMessageBox(wxString::Format(TR("Usunąć indeks treści (wyszukiwanie w treści) dla książek: %zu? Książki i pliki zostają."), idki.size()),
                                  TR("Literatura"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return 0;
    int n = 0;
    for (long long id : idki) {
        if (logika_.usunIndeks(id)) ++n;
        else { Blad(TekstZUTF8(logika_.pobierzBlad())); break; }
    }
    Odswiez();
    Komunikat(wxString::Format(TR("✓ Usunięto indeks treści: %d"), n));
    return n;
}

wxString PanelKsiazek::TrescInfo() const {
    const Ksiazka* k = Wybrana();
    if (!k) return wxEmptyString;
    auto esc = [](const std::string& s) {
        wxString w = wxString::FromUTF8(s);
        w.Replace("&", "&amp;"); w.Replace("<", "&lt;"); w.Replace(">", "&gt;");
        return w;
    };
    wxString h = "<html><body><h2>" + esc(k->tytul) + "</h2><table cellpadding=\"2\">";
    auto wiersz = [&](const wxString& et, const wxString& v) { if (!v.empty()) h += "<tr><td valign=top><b>" + et + ":</b></td><td>" + v + "</td></tr>"; };
    wiersz(TR("Autor"), esc(k->autor));
    wiersz(TR("Wydawca"), esc(k->wydawca));
    if (k->rok > 0) wiersz(TR("Rok"), wxString::Format("%d", k->rok));
    wiersz(TR("Wydanie"), esc(k->wydanie));
    wiersz(TR("ISBN"), esc(k->isbn));
    wiersz(TR("Język"), esc(k->jezyk));
    wiersz(TR("Tagi"), esc(k->tagi));
    wiersz(TR("Plik"), k->sciezka.empty() ? TR("brak (książka papierowa)") : esc(k->sciezka));
    if (k->strony > 0) wiersz(TR("Indeks treści"), wxString::Format(TR("%d stron (%s)"), k->strony, TekstZUTF8(k->zaindeksowano)));
    if (k->pozyczona) wiersz(TR("Pożyczona"), esc(k->pozyczonaPrzez) + TR(", od ") + TekstZUTF8(k->pozyczonaOd));
    h += "</table>";
    if (!k->uwagi.empty()) h += "<hr><h3>" + TR("Uwagi") + "</h3>" + wxString::FromUTF8(UwagiHTML::zTekstu(k->uwagi));
    return h + "</body></html>";
}

void PanelKsiazek::PokazInfo() {
    if (!Wybrana()) return;
    wxDialog dlg(this, wxID_ANY, TR("Informacje o książce"), wxDefaultPosition, wxSize(560, 480), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
    wxHtmlWindow* html = new wxHtmlWindow(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO | wxBORDER_SUNKEN);
    html->SetPage(TrescInfo());
    g->Add(html, 1, wxEXPAND | wxALL, 8);
    g->Add(dlg.CreateStdDialogButtonSizer(wxCLOSE), 0, wxEXPAND | wxALL, 8);
    dlg.SetSizer(g);
    dlg.SetAffirmativeId(wxID_CLOSE);
    dlg.ShowModal();
}

int PanelKsiazek::UsunZaznaczone(bool bezPytan) {
    const std::vector<long long> idki = ZaznaczoneId();
    if (idki.empty()) return 0;
    if (!bezPytan) {
        const wxString pytanie = idki.size() == 1
            ? wxString::Format(TR("Usunąć książkę \"%s\" z biblioteczki? Plik na dysku zostaje."), TekstZUTF8(Wybrana()->tytul))
            : wxString::Format(TR("Usunąć z biblioteczki zaznaczone książki (%zu)? Pliki na dysku zostają."), idki.size());
        if (wxMessageBox(pytanie, TR("Literatura"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return 0;
    }
    int usunieto = 0;
    for (long long id : idki) {
        if (logika_.usun(id)) ++usunieto;
        else { Blad(TekstZUTF8(logika_.pobierzBlad())); break; }
    }
    Odswiez();
    if (usunieto > 1) Komunikat(wxString::Format(TR("✓ Usunięto książek: %d"), usunieto));
    return usunieto;
}

void PanelKsiazek::Pozycz() {
    const Ksiazka* w = Wybrana();
    if (!w) return;
    if (w->pozyczona) {
        wxMessageBox(wxString::Format(TR("Ta książka jest już pożyczona (%s). Najpierw zarejestruj zwrot."), TekstZUTF8(w->pozyczonaPrzez)),
                     TR("Literatura"), wxOK | wxICON_INFORMATION, this);
        return;
    }
    LogikaNarzedzi osoby(ManagerBazyDanych::pobierz());
    DialogPozyczki dlg(this, TR("Pożycz książkę"), TR("Podaj, komu pożyczasz książkę."), TekstZUTF8(w->tytul), osoby.osoby());
    if (dlg.ShowModal() != wxID_OK) return;
    const long long id = w->id;
    const wxString tytul = TekstZUTF8(w->tytul);
    if (!logika_.pozycz(id, dlg.Osoba(), dlg.Od(), dlg.Termin(), dlg.Uwagi())) { Blad(TekstZUTF8(logika_.pobierzBlad())); return; }
    Komunikat(wxString::Format(TR("✓ Pożyczono \"%s\": %s"), tytul, TekstZUTF8(dlg.Osoba())));
    Odswiez();
    Wybierz(id);
}

bool PanelKsiazek::Zwroc(bool bezPytan) {
    const Ksiazka* w = Wybrana();
    if (!w || !w->pozyczona) return false;
    if (!bezPytan && wxMessageBox(wxString::Format(TR("Zarejestrować zwrot książki \"%s\" od: %s?"), TekstZUTF8(w->tytul), TekstZUTF8(w->pozyczonaPrzez)),
                                  TR("Literatura"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return false;
    const long long id = w->id;
    const wxString tytul = TekstZUTF8(w->tytul);
    if (!logika_.zwroc(id)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return false; }
    Komunikat(TR("✓ Zwrócono: ") + tytul);
    Odswiez();
    Wybierz(id);
    return true;
}

int PanelKsiazek::Przypomnij() {
    std::vector<Pozyczka> przet;
    if (!logika_.przeterminowane(przet)) return -1;
    for (const Pozyczka& p : przet)
        Komunikat(wxString::Format(TR("⚠ Książka \"%s\" miała wrócić %s od: %s"), TekstZUTF8(p.nazwaNarzedzia), TekstZUTF8(p.termin), TekstZUTF8(p.osoba)));
    return static_cast<int>(przet.size());
}

int PanelKsiazek::ImportujCSV(const wxString& sciezka) {
    wxString plik = sciezka;
    if (plik.empty()) {
        wxFileDialog dlg(this, TR("Importuj książki z CSV"), wxEmptyString, wxEmptyString, TR("Pliki CSV (*.csv)|*.csv|Wszystkie pliki|*.*"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) return -1;
        plik = dlg.GetPath();
    }
    std::string tekst;
    if (!WczytajTekst(plik, tekst)) { Blad(TR("Nie można odczytać pliku.")); return -1; }
    const auto w = KsiazkiCSV::parsuj(tekst);
    if (!w.blad.empty()) { Blad(TekstZUTF8(w.blad)); return -1; }
    WynikImportuKsiazek wynik;
    if (!logika_.importuj(w.ksiazki, wynik)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return -1; }
    Odswiez();
    wxString info = wxString::Format(TR("Dodano książek: %d, pominięto (już są): %d."), wynik.dodano, wynik.pominieto);
    for (const std::string& o : w.ostrzezenia) info += "\n" + TekstZUTF8(o);
    for (const std::string& b : wynik.bledy) info += "\n" + TekstZUTF8(b);
    Komunikat(wxString::FromUTF8("✓ ") + info);
    Status(info);
    return wynik.dodano;
}

bool PanelKsiazek::EksportujCSV(const wxString& sciezka) {
    wxString plik = sciezka;
    if (plik.empty()) {
        wxFileDialog dlg(this, TR("Eksportuj książki do CSV"), wxEmptyString, "ksiazki.csv", TR("Pliki CSV (*.csv)|*.csv"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        plik = dlg.GetPath();
    }
    std::vector<Ksiazka> wszystkie;
    if (!logika_.pobierz("", wszystkie)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return false; }
    if (!ZapiszTekst(plik, KsiazkiCSV::naCSV(wszystkie))) { Blad(TR("Nie można zapisać pliku.")); return false; }
    Komunikat(wxString::Format(TR("✓ Wyeksportowano książek: %zu"), wszystkie.size()));
    return true;
}

bool PanelKsiazek::ZapiszSzablon(const wxString& sciezka) {
    wxString plik = sciezka;
    if (plik.empty()) {
        wxFileDialog dlg(this, TR("Zapisz szablon CSV"), wxEmptyString, "szablon_ksiazki.csv", TR("Pliki CSV (*.csv)|*.csv"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK) return false;
        plik = dlg.GetPath();
    }
    if (!ZapiszTekst(plik, KsiazkiCSV::szablon())) { Blad(TR("Nie można zapisać pliku.")); return false; }
    return true;
}

int PanelKsiazek::DodajZKatalogu(const wxString& katalog) {
    wxString dir = katalog;
    if (dir.empty()) {
        wxDirDialog dlg(this, TR("Katalog z książkami (PDF, EPUB; podkatalogi są przeszukiwane)"), wxEmptyString, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) return -1;
        dir = dlg.GetPath();
    }
    std::vector<Ksiazka> nowe;
    {
        wxBusyCursor czekaj;
        for (const std::string& p : StronyPDF::znajdzPlikiKsiazek(dir.utf8_string())) {
            Ksiazka k;
            k.sciezka = p;
            UzupelnijZPliku(k, MetadaneKsiazek::odczytaj(p));   // XMP / Info PDF / OPF; brak danych: tytuł z nazwy pliku
            const std::string nazwaPliku = wxFileName(wxString::FromUTF8(p)).GetFullName().utf8_string();
            if (k.tytul.empty()) k.tytul = LogikaKsiazek::tytulZNazwyPliku(nazwaPliku);
            if (k.rok == 0) k.rok = LogikaKsiazek::rokZNazwyPliku(nazwaPliku);   // rok z początku nazwy pliku
            nowe.push_back(std::move(k));
        }
    }
    WynikImportuKsiazek wynik;
    if (!logika_.importuj(nowe, wynik)) { Blad(TekstZUTF8(logika_.pobierzBlad())); return -1; }
    Odswiez();
    const wxString info = wxString::Format(TR("Znaleziono plików: %zu, dodano książek: %d, pominięto (już są): %d."), nowe.size(), wynik.dodano, wynik.pominieto);
    Komunikat(wxString::FromUTF8("✓ ") + info);
    Status(info);
    return wynik.dodano;
}

bool PanelKsiazek::Otworz(int strona) {
    const Ksiazka* k = Wybrana();
    if (!k || k->sciezka.empty()) return false;
    if (!OtworzPlik(k->sciezka, strona)) {
        wxMessageBox(TR("Nie można otworzyć pliku książki (sprawdź, czy istnieje i czy jest program do jego otwierania)."),
                     TR("Literatura"), wxOK | wxICON_WARNING, this);
        return false;
    }
    return true;
}

std::vector<Ksiazka> PanelKsiazek::WybraneZPdf() const {
    std::vector<Ksiazka> wynik;
    for (long i = widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); i >= 0;
         i = widok_->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) {
        if (static_cast<size_t>(i) >= lista_.size()) continue;
        const Ksiazka& k = lista_[static_cast<size_t>(i)];
        if (k.sciezka.empty()) continue;
        const wxString sc = wxString::FromUTF8(k.sciezka);
        if (!sc.Lower().EndsWith(".pdf") || !wxFileName::FileExists(sc)) continue;
        wynik.push_back(k);
    }
    return wynik;
}

int PanelKsiazek::ZapiszMetadaneDoPliku(bool bezPytan) {
    const std::vector<Ksiazka> ksiazki = WybraneZPdf();
    if (ksiazki.empty()) {
        if (!bezPytan) wxMessageBox(TR("Zaznacz książki, które mają plik PDF (istniejący na dysku)."), TR("Literatura"), wxOK | wxICON_INFORMATION, this);
        return -1;
    }
    const wxString program = ProgramyZewnetrzne::Znajdz("exiftool", Konfiguracja::pobierz().pobierz("literatura", "exiftool", ""));
    if (program.empty()) {
        const wxString t = TR("Nie znaleziono programu ExifTool (exiftool.org). Zainstaluj go albo wskaż jego położenie w Ustawieniach (zakładka Inne).");
        if (bezPytan) Komunikat(TR("✗ ") + t); else Blad(t);
        return -1;
    }
    const bool kopia = Konfiguracja::pobierz().pobierz("literatura", "exiftool_kopia", "1") != "0";
    if (!bezPytan) {
        const wxString pytanie = wxString::Format(TR("Zapisać dane z bazy (tytuł, autor, wydawca, rok, ISBN, język, tagi, uwagi) do plików PDF: %zu?"), ksiazki.size()) +
                                 (kopia ? TR("\n\nExifTool zostawi obok kopię oryginału (plik.pdf_original).") : wxString());
        if (wxMessageBox(pytanie, TR("Zapis danych do PDF"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return -1;
    }
    int zapisano = 0;
    std::string bledy;
    wxLogNull bezLogu;
    {
        wxBusyCursor czekaj;
        for (const Ksiazka& k : ksiazki) {
            const wxString plikArg = wxFileName::CreateTempFileName("dmexif");
            std::string tresc;
            for (const std::string& l : PolecenieKsiazek::liniePlikuExifTool(k, k.sciezka)) tresc += l + "\n";
            if (plikArg.empty() || !ZapiszTekst(plikArg, tresc)) { bledy += k.tytul + ": nie można zapisać pliku tymczasowego\n"; continue; }
            const auto w = ProgramyZewnetrzne::Uruchom(program, PolecenieKsiazek::argumentyExifTool(plikArg.utf8_string(), kopia));
            wxRemoveFile(plikArg);
            if (w.ok()) ++zapisano;
            else bledy += k.tytul + ": " + (w.uruchomiono ? w.wyjscie.substr(0, 300) : std::string("nie można uruchomić programu")) + "\n";
        }
    }
    Komunikat(wxString::Format(TR("✓ Zapisano dane do plików PDF: %d z %zu"), zapisano, ksiazki.size()) + (bledy.empty() ? wxString() : TR(" (są błędy)")));
    Status(wxString::Format(TR("Zapisano dane do plików PDF: %d"), zapisano));
    if (!bledy.empty() && !bezPytan) wxMessageBox(TR("Nie wszystkie pliki zostały zapisane:\n\n") + TekstZUTF8(bledy), TR("Zapis danych do PDF"), wxOK | wxICON_WARNING, this);
    return zapisano;
}

int PanelKsiazek::Ocr(TrybOcr tryb, bool bezPytan) {
    const std::vector<Ksiazka> ksiazki = WybraneZPdf();
    if (ksiazki.empty()) {
        if (!bezPytan) wxMessageBox(TR("Zaznacz książki, które mają plik PDF (istniejący na dysku)."), TR("Literatura"), wxOK | wxICON_INFORMATION, this);
        return -1;
    }
    OcrPliku::Ustawienia ust;
    wxString problem;
    if (!OcrPliku::Przygotuj(ust, problem)) {
        if (bezPytan) Komunikat(TR("✗ ") + problem); else Blad(problem);
        return -1;
    }
    if (tryb == TrybOcr::Pytaj) {
        if (bezPytan) tryb = TrybOcr::ObokOryginalu;
        else {
            OcrPliku::Tryb t = OcrPliku::Tryb::Obok;
            if (!OcrPliku::WybierzTryb(this, ksiazki.size(), t)) return -1;
            tryb = t == OcrPliku::Tryb::Zastap ? TrybOcr::ZastapOryginal : TrybOcr::ObokOryginalu;
        }
    }
    int wykonano = 0;
    std::string bledy;
    std::vector<long long> doIndeksu;
    for (size_t i = 0; i < ksiazki.size(); ++i) {
        const Ksiazka& k = ksiazki[i];
        std::string blad;
        const auto w = OcrPliku::Wykonaj(this, ust, k.sciezka, TekstZUTF8(k.tytul), i + 1, ksiazki.size(), tryb == TrybOcr::ZastapOryginal, bezPytan, blad);
        if (w == OcrPliku::Wynik::Przerwano) { bledy += "przerwano\n"; break; }
        if (w == OcrPliku::Wynik::Blad) { bledy += k.tytul + ": " + blad + "\n"; continue; }
        if (tryb == TrybOcr::ZastapOryginal) doIndeksu.push_back(k.id);
        ++wykonano;
    }
    Odswiez();
    for (long long id : doIndeksu) { Wybierz(id); Indeksuj(true); }
    Komunikat(wxString::Format(TR("✓ OCR wykonano dla plików: %d z %zu"), wykonano, ksiazki.size()) + (bledy.empty() ? wxString() : TR(" (są błędy)")));
    if (!bledy.empty() && !bezPytan) wxMessageBox(TR("Nie wszystkie pliki zostały przetworzone:\n\n") + TekstZUTF8(bledy), TR("OCR"), wxOK | wxICON_WARNING, this);
    return wykonano;
}

int PanelKsiazek::Indeksuj(bool bezPytan) {
    const Ksiazka* k = Wybrana();
    if (!k) return -1;
    if (k->sciezka.empty() || !StronyKsiazki::czyMoznaIndeksowac(k->sciezka)) {
        if (!bezPytan) wxMessageBox(TR("Indeksować można pliki PDF z warstwą tekstową oraz EPUB."), TR("Literatura"), wxOK | wxICON_INFORMATION, this);
        return -1;
    }
    const long long id = k->id;
    const std::string tytul = k->tytul, sciezka = k->sciezka;
    std::string blad;
    std::vector<std::string> strony;
    {
        wxBusyCursor czekaj;
        strony = StronyKsiazki::pobierz(sciezka, blad);
        if (!strony.empty() && !logika_.zapiszIndeks(id, strony)) blad = logika_.pobierzBlad();
    }
    if (!blad.empty()) {
        if (!bezPytan) Blad(TekstZUTF8(blad));
        return -1;
    }
    Odswiez();
    Wybierz(id);
    const auto zapisana = logika_.pobierzKsiazke(id);
    const int n = zapisana ? zapisana->strony : 0;
    Komunikat(wxString::Format(TR("✓ Zaindeksowano \"%s\": %d stron (fragmentów) z tekstem"), TekstZUTF8(tytul), n));
    return n;
}

int PanelKsiazek::IndeksujWszystkie(bool bezPytan) {
    std::vector<Ksiazka> doZrobienia;
    for (const Ksiazka& k : lista_)
        if (k.strony == 0 && !k.sciezka.empty() && StronyKsiazki::czyMoznaIndeksowac(k.sciezka)) doZrobienia.push_back(k);
    if (doZrobienia.empty()) {
        if (!bezPytan) wxMessageBox(TR("Wszystkie książki (PDF, EPUB) z tej listy są już zaindeksowane."), TR("Literatura"), wxOK | wxICON_INFORMATION, this);
        return 0;
    }
    if (!bezPytan && wxMessageBox(wxString::Format(TR("Zaindeksować treść %zu książek? Duże PDF-y mogą potrwać kilka minut."), doZrobienia.size()),
                                  TR("Literatura"), wxYES_NO | wxICON_QUESTION, this) != wxYES) return 0;
    int gotowe = 0;
    wxString problemy;
    std::unique_ptr<wxProgressDialog> postep;
    if (!bezPytan)
        postep.reset(new wxProgressDialog(TR("Indeksowanie"), wxEmptyString, static_cast<int>(doZrobienia.size()), this,
                                          wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_ELAPSED_TIME));
    for (size_t i = 0; i < doZrobienia.size(); ++i) {
        const Ksiazka& k = doZrobienia[i];
        if (postep && !postep->Update(static_cast<int>(i), TekstZUTF8(k.tytul))) break;
        std::string blad;
        auto strony = StronyKsiazki::pobierz(k.sciezka, blad);
        if (blad.empty() && !logika_.zapiszIndeks(k.id, strony)) blad = logika_.pobierzBlad();
        if (blad.empty()) ++gotowe;
        else problemy += "\n" + TekstZUTF8(k.tytul) + ": " + TekstZUTF8(blad);
    }
    postep.reset();
    Odswiez();
    wxString info = wxString::Format(TR("Zaindeksowano książek: %d z %zu."), gotowe, doZrobienia.size());
    Komunikat(wxString::FromUTF8("✓ ") + info + problemy);
    if (!bezPytan && !problemy.empty()) wxMessageBox(info + "\n" + problemy, TR("Literatura"), wxOK | wxICON_WARNING, this);
    return gotowe;
}

int PanelKsiazek::SzukajWTresci(const wxString& tekst, bool tylkoZaznaczona) {
    wyniki_->DeleteAllItems();
    trafienia_.clear();
    long long id = 0;
    if (tylkoZaznaczona && Wybrana()) id = Wybrana()->id;
    if (!logika_.szukajWTresci(tekst.utf8_string(), id, trafienia_)) {
        Blad(TekstZUTF8(logika_.pobierzBlad()));
        return -1;
    }
    for (size_t i = 0; i < trafienia_.size(); ++i) {
        const TrafienieKsiazki& t = trafienia_[i];
        const long w = wyniki_->InsertItem(static_cast<long>(i), TekstZUTF8(t.tytul));
        wyniki_->SetItem(w, 1, wxString::Format("%d", t.strona));
        wyniki_->SetItem(w, 2, TekstZUTF8(t.fragment));
    }
    Status(trafienia_.empty() ? TR("Nie znaleziono w treści zaindeksowanych książek.")
                              : wxString::Format(TR("Trafień w treści: %zu"), trafienia_.size()));
    return static_cast<int>(trafienia_.size());
}

void PanelKsiazek::NaWyszukiwanieTresci() {
    const wxString t = szukajTresci_->GetValue().Strip(wxString::both);
    if (t.empty()) { wyniki_->DeleteAllItems(); trafienia_.clear(); return; }
    SzukajWTresci(t, cbZaznaczona_->GetValue());
}
