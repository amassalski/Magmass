#include "DialogKsiazki.h"

#include "DialogUwagHTML.h"
#include "../Import/BibliotekaNarodowa.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Narzedzia/UwagiHTML.h"
#include "../Pomoc/PomocGUI.h"
#include "../Wyszukiwanie/KlientHTTP.h"

#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/filename.h>

namespace {

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

const char* const JEZYKI[] = {"polski", "angielski", "niemiecki", "francuski", "rosyjski", "czeski", "słowacki", "ukraiński",
                              "hiszpański", "włoski", "łaciński"};

}  // namespace

DialogKsiazki::DialogKsiazki(wxWindow* parent, const Ksiazka& k, int liczbaWybranych)
    : wxDialog(parent, wxID_ANY,
               liczbaWybranych > 1 ? wxString::Format(TR("Zmiana zbiorcza: %d książek"), liczbaWybranych)
                                   : (k.id ? TR("Zmień książkę") : TR("Nowa książka")),
               wxDefaultPosition, wxSize(660, 600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      ksiazka_(k), zbiorczo_(liczbaWybranych > 1) {
    wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
    if (zbiorczo_) {
        g->Add(new wxStaticText(this, wxID_ANY,
                                TR("Zaznacz pola, które mają dostać tę samą wartość we wszystkich wybranych książkach. Pola indywidualne są zablokowane.")),
               0, wxALL, 10);
    }
    wxFlexGridSizer* s = new wxFlexGridSizer(2, 6, 8);
    s->AddGrowableCol(1);
    auto dodaj = [&](const wxString& et, auto* p) {
        s->Add(new wxStaticText(this, wxID_ANY, et), 0, wxALIGN_CENTER_VERTICAL);
        s->Add(p, 1, wxEXPAND);
    };
    auto dodajZmienny = [&](wxCheckBox*& cb, const wxString& et, auto* p) {
        cb = new wxCheckBox(this, wxID_ANY, et);
        s->Add(cb, 0, wxALIGN_CENTER_VERTICAL);
        s->Add(p, 1, wxEXPAND);
    };
    tcTytul_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(k.tytul));
    dodaj(TR("Tytuł *"), tcTytul_);
    tcAutor_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(k.autor));
    tcWydawca_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(k.wydawca));
    if (zbiorczo_) {
        dodajZmienny(cbAutor_, TR("Autor"), tcAutor_);
        dodajZmienny(cbWydawca_, TR("Wydawca"), tcWydawca_);
    } else {
        dodaj(TR("Autor"), tcAutor_);
        dodaj(TR("Wydawca"), tcWydawca_);
    }
    tcRok_ = new wxTextCtrl(this, wxID_ANY, k.rok > 0 ? wxString::Format("%d", k.rok) : wxString(), wxDefaultPosition, wxSize(90, -1));
    tcRok_->SetMaxLength(4);
    tcRok_->SetHint(TR("rrrr"));
    tcRok_->SetValidator(wxTextValidator(wxFILTER_DIGITS));
    tcRok_->Bind(wxEVT_TEXT_PASTE, [this](wxClipboardTextEvent&) {
        wxString tekst;
        if (wxTheClipboard->Open()) {
            if (wxTheClipboard->IsSupported(wxDF_UNICODETEXT)) {
                wxTextDataObject d;
                if (wxTheClipboard->GetData(d)) tekst = d.GetText();
            }
            wxTheClipboard->Close();
        }
        WklejRok(tekst);   // bez Skip: sama liczba albo nic
    });
    wxBoxSizer* rokWiersz = new wxBoxSizer(wxHORIZONTAL);
    rokWiersz->Add(tcRok_, 0, wxALIGN_CENTER_VERTICAL);
    rokWiersz->Add(new wxStaticText(this, wxID_ANY, TR("  (puste = nieznany)")), 0, wxALIGN_CENTER_VERTICAL);
    dodaj(TR("Rok wydania"), rokWiersz);
    tcWydanie_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(k.wydanie));
    tcWydanie_->SetHint(TR("np. pierwsze, drugie poprawione, 3rd edition"));
    PomocGUI::Dodaj(tcWydanie_, "ksiazka.wydanie");
    dodaj(TR("Wydanie"), tcWydanie_);
    wxBoxSizer* isbnWiersz = new wxBoxSizer(wxHORIZONTAL);
    tcIsbn_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(k.isbn));
    isbnWiersz->Add(tcIsbn_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    bBN_ = new wxButton(this, wxID_ANY, TR("Pobierz z Biblioteki Narodowej"));
    PomocGUI::Dodaj(bBN_, "ksiazka.bn");
    isbnWiersz->Add(bBN_, 0, wxALIGN_CENTER_VERTICAL);
    dodaj(TR("ISBN"), isbnWiersz);
    cbJezyk_ = new wxComboBox(this, wxID_ANY, TekstZUTF8(k.jezyk), wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN);
    for (const char* j : JEZYKI) cbJezyk_->Append(TekstZUTF8(j));
    PomocGUI::Dodaj(cbJezyk_, "ksiazka.jezyk");
    dodaj(TR("Język"), cbJezyk_);
    tcTagi_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(k.tagi));
    tcTagi_->SetHint(TR("np. wzmacniacze, lampy, zasilacze"));
    if (zbiorczo_) {
        wxBoxSizer* tagi = new wxBoxSizer(wxHORIZONTAL);
        chTagi_ = new wxChoice(this, wxID_ANY);
        chTagi_->Append(TR("dopisz do istniejących"));
        chTagi_->Append(TR("zastąp istniejące"));
        chTagi_->SetSelection(0);
        tagi->Add(tcTagi_, 1, wxRIGHT, 6);
        tagi->Add(chTagi_, 0);
        dodajZmienny(cbTagi_, TR("Tagi"), tagi);
    } else {
        dodaj(TR("Tagi"), tcTagi_);
    }
    wxBoxSizer* plik = new wxBoxSizer(wxHORIZONTAL);
    tcSciezka_ = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(k.sciezka));
    tcSciezka_->SetHint(TR("pusta = książka papierowa"));
    plik->Add(tcSciezka_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    wxButton* przegladaj = new wxButton(this, wxID_ANY, TR("Wybierz..."));
    plik->Add(przegladaj, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    wxButton* odczytaj = new wxButton(this, wxID_ANY, TR("Odczytaj dane z pliku"));
    plik->Add(odczytaj, 0, wxALIGN_CENTER_VERTICAL);
    PomocGUI::Dodaj(odczytaj, "ksiazka.metadane");
    dodaj(TR("Plik (PDF, EPUB)"), plik);
    g->Add(s, 0, wxEXPAND | wxALL, 10);
    wxBoxSizer* uwagiNaglowek = new wxBoxSizer(wxHORIZONTAL);
    uwagiNaglowek->Add(new wxStaticText(this, wxID_ANY, TR("Uwagi (dwuklik w polu otwiera edytor):")), 1, wxALIGN_CENTER_VERTICAL);
    wxButton* bUwagi = new wxButton(this, wxID_ANY, TR("Edytor HTML..."));
    uwagiNaglowek->Add(bUwagi, 0);
    g->Add(uwagiNaglowek, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    podgladUwag_ = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 110), wxHW_SCROLLBAR_AUTO | wxBORDER_SUNKEN);
    PomocGUI::Dodaj(podgladUwag_, "ksiazka.uwagi");
    UstawUwagi(TekstZUTF8(k.uwagi));
    g->Add(podgladUwag_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    g->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
    SetSizer(g);
    PomocGUI::Dodaj(tcSciezka_, "ksiazka.plik");
    PomocGUI::Dodaj(tcTagi_, "ksiazka.tagi");

    przegladaj->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxFileDialog dlg(this, TR("Plik książki"), wxEmptyString, wxEmptyString,
                         TR("Książki (*.pdf;*.epub)|*.pdf;*.epub|Wszystkie pliki|*.*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) return;
        tcSciezka_->SetValue(dlg.GetPath());
        OdczytajZPliku(false);
        if (tcRok_->GetValue().Strip(wxString::both).empty()) {
            const int rok = LogikaKsiazek::rokZNazwyPliku(wxFileName(dlg.GetPath()).GetFullName().utf8_string());
            if (rok > 0) tcRok_->SetValue(wxString::Format("%d", rok));   // rok z początku nazwy pliku
        }
        if (tcTytul_->GetValue().Strip(wxString::both).empty())
            tcTytul_->SetValue(TekstZUTF8(LogikaKsiazek::tytulZNazwyPliku(wxFileName(dlg.GetPath()).GetFullName().utf8_string())));
    });
    odczytaj->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OdczytajZPliku(true); });
    bBN_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PobierzZBN(); });
    bUwagi->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EdytujUwagi(); });
    podgladUwag_->Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent&) { EdytujUwagi(); });
    Bind(wxEVT_BUTTON, &DialogKsiazki::NaOK, this, wxID_OK);

    if (zbiorczo_) {
        // pola indywidualne wyszarzone i zablokowane
        for (wxWindow* w : std::vector<wxWindow*>{tcTytul_, tcRok_, tcIsbn_, cbJezyk_, tcSciezka_, tcWydanie_, podgladUwag_, bBN_, przegladaj, odczytaj, bUwagi})
            w->Enable(false);
        tcAutor_->Bind(wxEVT_TEXT, [this](wxCommandEvent& e) { cbAutor_->SetValue(true); e.Skip(); });
        tcWydawca_->Bind(wxEVT_TEXT, [this](wxCommandEvent& e) { cbWydawca_->SetValue(true); e.Skip(); });
        tcTagi_->Bind(wxEVT_TEXT, [this](wxCommandEvent& e) { cbTagi_->SetValue(true); e.Skip(); });
        tcAutor_->ChangeValue(wxEmptyString);
        tcWydawca_->ChangeValue(wxEmptyString);
        tcTagi_->ChangeValue(wxEmptyString);
        tcTytul_->ChangeValue(wxEmptyString);
        UstawUwagi(wxEmptyString);
        tcWydanie_->ChangeValue(wxEmptyString);
        tcIsbn_->ChangeValue(wxEmptyString);
        tcSciezka_->ChangeValue(wxEmptyString);
        tcRok_->ChangeValue(wxEmptyString);
        cbJezyk_->SetValue(wxEmptyString);
    }
}

DialogKsiazki::~DialogKsiazki() { *zyje_ = false; }

int DialogKsiazki::Rok() const {
    const wxString t = tcRok_->GetValue().Strip(wxString::both);
    if (t.empty()) return 0;
    long r = 0;
    return t.ToLong(&r) ? static_cast<int>(r) : 0;
}

bool DialogKsiazki::WklejRok(const wxString& tekst) {
    const int r = BibliotekaNarodowa::rokZTekstu(tekst.utf8_string());
    if (r <= 0) return false;
    tcRok_->SetValue(wxString::Format("%d", r));
    return true;
}

void DialogKsiazki::UstawDane(const MetadaneKsiazek::Dane& d) {
    if (!d.tytul.empty()) tcTytul_->SetValue(TekstZUTF8(d.tytul));
    if (!d.autor.empty()) tcAutor_->SetValue(TekstZUTF8(d.autor));
    if (!d.wydawca.empty()) tcWydawca_->SetValue(TekstZUTF8(d.wydawca));
    if (d.rok > 0) tcRok_->SetValue(wxString::Format("%d", d.rok));
    if (!d.isbn.empty()) tcIsbn_->SetValue(TekstZUTF8(d.isbn));
    if (!d.jezyk.empty()) cbJezyk_->SetValue(TekstZUTF8(d.jezyk));
    if (!d.tagi.empty() && T(tcTagi_).empty()) tcTagi_->SetValue(TekstZUTF8(d.tagi));
    if (!d.wydanie.empty()) tcWydanie_->SetValue(TekstZUTF8(d.wydanie));
    if (!d.opis.empty() && uwagi_.Strip(wxString::both).empty()) UstawUwagi(TekstZUTF8(d.opis));
}

void DialogKsiazki::UstawUwagi(const wxString& zrodlo) {
    uwagi_ = zrodlo;
    const std::string html = UwagiHTML::zTekstu(zrodlo.utf8_string());   // zwykły tekst też pokazujemy jako HTML (nowe linie, znaki specjalne)
    podgladUwag_->SetPage(wxString::FromUTF8("<html><body>" + html + "</body></html>"));
}

bool DialogKsiazki::EdytujUwagi() {
    if (zbiorczo_) return false;
    DialogUwagHTML dlg(this, uwagi_, tcTytul_->GetValue());
    if (dlg.ShowModal() != wxID_OK) return false;
    UstawUwagi(dlg.Zrodlo());
    return true;
}

int DialogKsiazki::ZastosujOdpowiedzBN(const std::string& json, bool pytajOWybor) {
    std::vector<MetadaneKsiazek::Dane> wyniki;
    std::string blad;
    if (!BibliotekaNarodowa::zOdpowiedzi(json, wyniki, blad)) {
        wxMessageBox(TR("Nieprawidłowa odpowiedź Biblioteki Narodowej: ") + TekstZUTF8(blad), TR("Literatura"), wxOK | wxICON_WARNING, this);
        return 0;
    }
    if (wyniki.empty()) {
        wxMessageBox(TR("Biblioteka Narodowa nie znalazła takiej książki. Spróbuj podać ISBN albo dokładniejszy tytuł."), TR("Literatura"),
                     wxOK | wxICON_INFORMATION, this);
        return 0;
    }
    size_t wybor = 0;
    if (wyniki.size() > 1 && pytajOWybor) {
        wxArrayString opisy;
        for (const auto& d : wyniki) {
            wxString o = TekstZUTF8(d.tytul);
            if (!d.autor.empty()) o += " — " + TekstZUTF8(d.autor);
            wxString dod;
            if (!d.wydawca.empty()) dod += TekstZUTF8(d.wydawca);
            if (d.rok > 0) dod += (dod.empty() ? "" : ", ") + wxString::Format("%d", d.rok);
            if (!dod.empty()) o += " (" + dod + ")";
            opisy.Add(o);
        }
        wxSingleChoiceDialog dlg(this, TR("Znaleziono kilka pozycji. Wybierz właściwą:"), TR("Biblioteka Narodowa"), opisy);
        if (dlg.ShowModal() != wxID_OK) return static_cast<int>(wyniki.size());
        wybor = static_cast<size_t>(dlg.GetSelection());
    }
    UstawDane(wyniki[wybor]);
    return static_cast<int>(wyniki.size());
}

void DialogKsiazki::PobierzZBN() {
    const std::string isbn = T(tcIsbn_), tytul = T(tcTytul_), autor = T(tcAutor_);
    if (BibliotekaNarodowa::czystyISBN(isbn).empty() && tytul.empty() && autor.empty()) {
        wxMessageBox(TR("Wpisz ISBN albo tytuł (i ewentualnie autora), a program poszuka książki w Bibliotece Narodowej."), TR("Literatura"),
                     wxOK | wxICON_INFORMATION, this);
        return;
    }
    const std::string url = BibliotekaNarodowa::adresZapytania(isbn, tytul, autor);
    bBN_->Enable(false);
    bBN_->SetLabel(TR("Szukam..."));
    std::weak_ptr<bool> zyje = zyje_;
    KlientHTTP::pobierzStrone(url, [this, zyje](const WynikHTTP& w) {
        auto z = zyje.lock();
        if (!z || !*z) return;   // formularz już zamknięty
        bBN_->Enable(true);
        bBN_->SetLabel(TR("Pobierz z Biblioteki Narodowej"));
        if (!w.ok) {
            wxMessageBox(TR("Nie udało się połączyć z Biblioteką Narodową: ") + TekstZUTF8(w.blad), TR("Literatura"), wxOK | wxICON_WARNING, this);
            return;
        }
        ZastosujOdpowiedzBN(w.tresc);
    });
}

void DialogKsiazki::OdczytajZPliku(bool komunikat) {
    const std::string sciezka = T(tcSciezka_);
    if (sciezka.empty()) return;
    MetadaneKsiazek::Dane d;
    {
        wxBusyCursor czekaj;
        d = MetadaneKsiazek::odczytaj(sciezka);
    }
    if (d.pusta()) {
        if (komunikat) wxMessageBox(TR("W pliku nie znaleziono metadanych (XMP, Info PDF, OPF) albo nie można go odczytać."), TR("Literatura"), wxOK | wxICON_INFORMATION, this);
        return;
    }
    Ksiazka k;
    k.tytul = T(tcTytul_); k.autor = T(tcAutor_); k.wydawca = T(tcWydawca_); k.rok = Rok(); k.isbn = T(tcIsbn_);
    k.jezyk = cbJezyk_->GetValue().Strip(wxString::both).utf8_string(); k.tagi = T(tcTagi_); k.uwagi = uwagi_.Strip(wxString::both).utf8_string(); k.wydanie = T(tcWydanie_);
    UzupelnijZPliku(k, d);
    tcTytul_->SetValue(TekstZUTF8(k.tytul)); tcAutor_->SetValue(TekstZUTF8(k.autor)); tcWydawca_->SetValue(TekstZUTF8(k.wydawca));
    tcRok_->SetValue(k.rok > 0 ? wxString::Format("%d", k.rok) : wxString()); tcIsbn_->SetValue(TekstZUTF8(k.isbn));
    cbJezyk_->SetValue(TekstZUTF8(k.jezyk));
    tcTagi_->SetValue(TekstZUTF8(k.tagi)); UstawUwagi(TekstZUTF8(k.uwagi)); tcWydanie_->SetValue(TekstZUTF8(k.wydanie));
}

ZmianyZbiorczeKsiazek DialogKsiazki::Zmiany() const {
    ZmianyZbiorczeKsiazek z;
    if (!zbiorczo_) return z;
    if (cbAutor_->GetValue()) z.autor = T(tcAutor_);
    if (cbWydawca_->GetValue()) z.wydawca = T(tcWydawca_);
    if (cbTagi_->GetValue()) { z.tagi = T(tcTagi_); z.tagiDopisz = chTagi_->GetSelection() == 0; }
    return z;
}

void DialogKsiazki::NaOK(wxCommandEvent& e) {
    if (zbiorczo_) {
        if (Zmiany().pusta()) {
            wxMessageBox(TR("Zaznacz przynajmniej jedno pole do zmiany (Autor, Wydawca lub Tagi)."), TR("Literatura"), wxOK | wxICON_WARNING, this);
            return;
        }
        e.Skip();
        return;
    }
    if (T(tcTytul_).empty()) {
        wxMessageBox(TR("Podaj tytuł książki."), TR("Literatura"), wxOK | wxICON_WARNING, this);
        return;
    }
    ksiazka_.tytul = T(tcTytul_);
    ksiazka_.autor = T(tcAutor_);
    ksiazka_.wydawca = T(tcWydawca_);
    ksiazka_.rok = Rok();
    ksiazka_.isbn = T(tcIsbn_);
    ksiazka_.jezyk = cbJezyk_->GetValue().Strip(wxString::both).utf8_string();
    ksiazka_.tagi = T(tcTagi_);
    ksiazka_.sciezka = T(tcSciezka_);
    ksiazka_.uwagi = uwagi_.Strip(wxString::both).utf8_string();
    ksiazka_.wydanie = T(tcWydanie_);
    e.Skip();
}
