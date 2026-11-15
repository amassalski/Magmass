#include "DialogUstawien.h"
#include <wx/filename.h>
#include <wx/filedlg.h>
#include "../Narzedzia/Tlumaczenia.h"
#include "../BazaDanych/ManagerBazyDanych.h"
#include "../Narzedzia/Konfiguracja.h"
#include "../Logika/Aktualizacje.h"
#include "DialogPolaczenia.h"
#include "../Pomoc/PomocGUI.h"
#include <wx/sizer.h>
#include <wx/intl.h>
#include <wx/dirdlg.h>
#include <wx/font.h>
#include <wx/notebook.h>

wxBEGIN_EVENT_TABLE(DialogUstawien, wxDialog)
wxEND_EVENT_TABLE()

DialogUstawien::DialogUstawien(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Ustawienia"),
               wxDefaultPosition, wxSize(550, 500),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    
    UtworzKontrole();
    UlozKontrole();
    WczytajUstawienia();
    Center();
}

void DialogUstawien::UtworzKontrole() {
    // === BAZA DANYCH ===
    stOpisBaza = new wxStaticText(this, wxID_ANY, wxEmptyString);
    btnPolaczenie = new wxButton(this, wxID_ANY, TR("&Ustawienia połączenia..."));

    // === ŚCIEŻKI ===
    tcSciezkaObrazki = new wxTextCtrl(this, wxID_ANY);
    btnBrowseObrazki = new wxButton(this, wxID_ANY, TR("..."), wxDefaultPosition, wxSize(40, -1));
    
    tcSciezkaPDF = new wxTextCtrl(this, wxID_ANY);
    btnBrowsePDF = new wxButton(this, wxID_ANY, TR("..."), wxDefaultPosition, wxSize(40, -1));
    
    // === KOD EAN ===
    choiceTypEAN = new wxChoice(this, wxID_ANY);
    choiceTypEAN->Append(TR("EAN13"));
    choiceTypEAN->Append(TR("EAN8"));
    choiceTypEAN->Append(TR("Code128"));
    choiceTypEAN->SetSelection(0);
    
    scPrefixEAN = new wxSpinCtrl(this, wxID_ANY, "20", wxDefaultPosition, wxSize(100, -1),
                                 wxSP_ARROW_KEYS, 0, 99, 20);
    
    // === CZCIONKA ===
    choiceFonty = new wxChoice(this, wxID_ANY);
    choiceFonty->Append(TR("Arial"));
    choiceFonty->Append(TR("Times New Roman"));
    choiceFonty->Append(TR("Courier New"));
    choiceFonty->Append(TR("Verdana"));
    choiceFonty->SetSelection(0);
    
    scRozmiarFontu = new wxSpinCtrl(this, wxID_ANY, "10", wxDefaultPosition, wxSize(100, -1),
                                    wxSP_ARROW_KEYS, 8, 72, 10);
    
    cbPogrubienie = new wxCheckBox(this, wxID_ANY, TR("Pogrubienie"));
    cbKursywa = new wxCheckBox(this, wxID_ANY, TR("Kursywa"));
    
    stPrzeglad = new wxStaticText(this, wxID_ANY, TR("Przykład tekstu"));
    stPrzeglad->SetMinSize(wxSize(200, 40));
    
    // === JĘZYK I LITERATURA ===
    choiceJezyk = new wxChoice(this, wxID_ANY);
    choiceJezyk->Append(TR("Według systemu"));
    choiceJezyk->Append("Polski");
    choiceJezyk->Append("English");
    choiceJezyk->SetSelection(0);
    tcCzytnikPDF = new wxTextCtrl(this, wxID_ANY);
    tcCzytnikPDF->SetHint(TR("puste = automatycznie, np. okular --page %p \"%f\""));
    tcExifTool = new wxTextCtrl(this, wxID_ANY);
    tcExifTool->SetHint(TR("puste = exiftool z PATH"));
    tcOcrmypdf = new wxTextCtrl(this, wxID_ANY);
    tcOcrmypdf->SetHint(TR("puste = ocrmypdf z PATH"));
    tcOcrJezyki = new wxTextCtrl(this, wxID_ANY);
    tcOcrJezyki->SetHint(TR("pol+eng"));
    tcMouserKlucz = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    tcMouserKlucz->SetHint(TR("klucz z maila od Mousera (xxxxxxxx-xxxx-...)"));

    // === PRZYCISKI ===
    btnOK = new wxButton(this, wxID_OK, TR("&OK"));
    btnAnuluj = new wxButton(this, wxID_CANCEL, TR("&Anuluj"));
    
    // === EVENT HANDLERY ===
    Bind(wxEVT_BUTTON, &DialogUstawien::OnUstawieniaPolaczenia, this, btnPolaczenie->GetId());
    Bind(wxEVT_BUTTON, &DialogUstawien::OnBrowseObrazki, this, btnBrowseObrazki->GetId());
    Bind(wxEVT_BUTTON, &DialogUstawien::OnBrowsePDF, this, btnBrowsePDF->GetId());
    Bind(wxEVT_CHOICE, &DialogUstawien::OnFontChanged, this, choiceFonty->GetId());
    Bind(wxEVT_SPINCTRL, &DialogUstawien::OnFontChanged, this, scRozmiarFontu->GetId());
    Bind(wxEVT_CHECKBOX, &DialogUstawien::OnFontChanged, this, cbPogrubienie->GetId());
    Bind(wxEVT_CHECKBOX, &DialogUstawien::OnFontChanged, this, cbKursywa->GetId());
    Bind(wxEVT_BUTTON, &DialogUstawien::OnOK, this, wxID_OK);
    Bind(wxEVT_BUTTON, &DialogUstawien::OnAnuluj, this, wxID_CANCEL);
}

void DialogUstawien::UlozKontrole() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // === NOTEBOOK (KARTY) ===
    wxNotebook* notebook = new wxNotebook(this, wxID_ANY);

    // --- TAB 1: BAZA DANYCH ---
    wxPanel* panelBaza = new wxPanel(notebook);
    wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer* sboxBaza = new wxStaticBoxSizer(wxVERTICAL, panelBaza, TR("Baza danych"));
    stOpisBaza->Reparent(panelBaza);
    btnPolaczenie->Reparent(panelBaza);
    sboxBaza->Add(stOpisBaza, 0, wxEXPAND | wxALL, 8);
    sboxBaza->Add(btnPolaczenie, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    sizer1->Add(sboxBaza, 0, wxEXPAND | wxALL, 5);
    sizer1->AddStretchSpacer();

    panelBaza->SetSizer(sizer1);
    notebook->AddPage(panelBaza, TR("Baza danych"));

    // --- TAB 2: ŚCIEŻKI ---
    wxPanel* panelSciezki = new wxPanel(notebook);
    wxBoxSizer* sizer2 = new wxBoxSizer(wxVERTICAL);

    for (wxWindow* w : {(wxWindow*)tcSciezkaObrazki, (wxWindow*)btnBrowseObrazki,
                        (wxWindow*)tcSciezkaPDF, (wxWindow*)btnBrowsePDF}) w->Reparent(panelSciezki);
    wxStaticBoxSizer* sboxSciezki = new wxStaticBoxSizer(wxVERTICAL, panelSciezki, TR("Ścieżki"));
    wxFlexGridSizer* gridSciezki = new wxFlexGridSizer(2, 3, 5, 5);
    gridSciezki->AddGrowableCol(1);
    
    gridSciezki->Add(new wxStaticText(panelSciezki, wxID_ANY, TR("Folder obrazków:")), 0, wxALIGN_CENTER_VERTICAL);
    gridSciezki->Add(tcSciezkaObrazki, 1, wxEXPAND);
    gridSciezki->Add(btnBrowseObrazki, 0);
    
    gridSciezki->Add(new wxStaticText(panelSciezki, wxID_ANY, TR("Folder PDF:")), 0, wxALIGN_CENTER_VERTICAL);
    gridSciezki->Add(tcSciezkaPDF, 1, wxEXPAND);
    gridSciezki->Add(btnBrowsePDF, 0);
    
    sboxSciezki->Add(gridSciezki, 0, wxEXPAND | wxALL, 5);
    sizer2->Add(sboxSciezki, 0, wxEXPAND | wxALL, 5);

    // Repozytorium projektu: stąd program sprawdza, czy jest nowsza wersja (gdyby kiedyś się zmieniło, wystarczy poprawić tutaj)
    wxStaticBoxSizer* sboxAkt = new wxStaticBoxSizer(wxVERTICAL, panelSciezki, TR("Aktualizacje"));
    wxFlexGridSizer* gridAkt = new wxFlexGridSizer(2, 5, 5);
    gridAkt->AddGrowableCol(1);
    gridAkt->Add(new wxStaticText(panelSciezki, wxID_ANY, TR("Repozytorium GitHub:")), 0, wxALIGN_CENTER_VERTICAL);
    tcRepozytorium = new wxTextCtrl(panelSciezki, wxID_ANY);
    gridAkt->Add(tcRepozytorium, 1, wxEXPAND);
    sboxAkt->Add(gridAkt, 0, wxEXPAND | wxALL, 5);
    sboxAkt->Add(new wxStaticText(panelSciezki, wxID_ANY, TR("Plik wersja.txt musi leżeć w głównym katalogu repozytorium (repozytorium musi być publiczne).")),
                 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    sizer2->Add(sboxAkt, 0, wxEXPAND | wxALL, 5);
    sizer2->AddStretchSpacer();

    panelSciezki->SetSizer(sizer2);
    notebook->AddPage(panelSciezki, TR("Ścieżki"));

    // --- TAB 3: INNE ---
    wxPanel* panelInne = new wxPanel(notebook);
    wxBoxSizer* sizer3 = new wxBoxSizer(wxVERTICAL);

    for (wxWindow* w : {(wxWindow*)choiceTypEAN, (wxWindow*)scPrefixEAN, (wxWindow*)choiceFonty,
                        (wxWindow*)scRozmiarFontu, (wxWindow*)cbPogrubienie, (wxWindow*)cbKursywa,
                        (wxWindow*)stPrzeglad}) w->Reparent(panelInne);

    // Kod EAN
    wxStaticBoxSizer* sboxEAN = new wxStaticBoxSizer(wxVERTICAL, panelInne, TR("Kod EAN"));
    wxFlexGridSizer* gridEAN = new wxFlexGridSizer(2, 2, 5, 10);
    gridEAN->Add(new wxStaticText(panelInne, wxID_ANY, TR("Typ kodu:")), 0, wxALIGN_CENTER_VERTICAL);
    gridEAN->Add(choiceTypEAN, 0);
    gridEAN->Add(new wxStaticText(panelInne, wxID_ANY, TR("Prefix:")), 0, wxALIGN_CENTER_VERTICAL);
    gridEAN->Add(scPrefixEAN, 0);
    sboxEAN->Add(gridEAN, 0, wxEXPAND | wxALL, 5);
    sizer3->Add(sboxEAN, 0, wxEXPAND | wxALL, 5);

    // Czcionka
    wxStaticBoxSizer* sboxCzcionka = new wxStaticBoxSizer(wxVERTICAL, panelInne, TR("Czcionka"));
    wxFlexGridSizer* gridCzcionka = new wxFlexGridSizer(4, 2, 5, 10);
    
    gridCzcionka->Add(new wxStaticText(panelInne, wxID_ANY, TR("Nazwa:")), 0, wxALIGN_CENTER_VERTICAL);
    gridCzcionka->Add(choiceFonty, 0);
    
    gridCzcionka->Add(new wxStaticText(panelInne, wxID_ANY, TR("Rozmiar:")), 0, wxALIGN_CENTER_VERTICAL);
    gridCzcionka->Add(scRozmiarFontu, 0);
    
    gridCzcionka->Add(new wxStaticText(panelInne, wxID_ANY, TR("Styl:")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* hSizerStyl = new wxBoxSizer(wxHORIZONTAL);
    hSizerStyl->Add(cbPogrubienie, 0, wxRIGHT, 10);
    hSizerStyl->Add(cbKursywa, 0);
    gridCzcionka->Add(hSizerStyl, 0);
    
    gridCzcionka->Add(new wxStaticText(panelInne, wxID_ANY, TR("Podgląd:")), 0, wxALIGN_TOP);
    gridCzcionka->Add(stPrzeglad, 0, wxEXPAND);
    
    sboxCzcionka->Add(gridCzcionka, 0, wxEXPAND | wxALL, 5);
    sizer3->Add(sboxCzcionka, 0, wxEXPAND | wxALL, 5);

    // Język interfejsu i czytnik PDF dla książek
    for (wxWindow* w : {(wxWindow*)choiceJezyk, (wxWindow*)tcCzytnikPDF, (wxWindow*)tcExifTool, (wxWindow*)tcOcrmypdf, (wxWindow*)tcOcrJezyki, (wxWindow*)tcMouserKlucz}) w->Reparent(panelInne);
    // pole ze ścieżką do programu i mały przycisk "..." obok (okno wyboru pliku); dla polecenia czytnika podmienia tylko nazwę programu
    auto poleProgramu = [this, panelInne](wxTextCtrl* tc, bool polecenie) {
        wxBoxSizer* wiersz = new wxBoxSizer(wxHORIZONTAL);
        wiersz->Add(tc, 1, wxEXPAND);
        wxButton* b = new wxButton(panelInne, wxID_ANY, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
        b->SetToolTip(TR("Wskaż program"));
        wiersz->Add(b, 0, wxLEFT | wxEXPAND, 4);
        b->Bind(wxEVT_BUTTON, [this, tc, polecenie](wxCommandEvent&) {
            wxString poczatek = tc->GetValue();
            wxString argumenty;
            if (polecenie) {   // "program" arg arg albo program arg arg
                poczatek.Trim(false);
                size_t koniec = 0;
                if (poczatek.StartsWith("\"")) { koniec = poczatek.find('"', 1); koniec = koniec == wxString::npos ? poczatek.length() : koniec + 1; }
                else { koniec = poczatek.find(' '); if (koniec == wxString::npos) koniec = poczatek.length(); }
                argumenty = poczatek.Mid(koniec);
                poczatek = poczatek.Left(koniec);
                poczatek.Replace("\"", "");
            }
            wxFileDialog fd(this, TR("Wskaż program"), wxFileName(poczatek).GetPath(), wxFileName(poczatek).GetFullName(),
#ifdef __WXMSW__
                            TR("Programy (*.exe;*.bat;*.cmd)|*.exe;*.bat;*.cmd|Wszystkie pliki (*.*)|*.*"),
#else
                            TR("Wszystkie pliki") + "|*",
#endif
                            wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            if (fd.ShowModal() != wxID_OK) return;
            const wxString sc = fd.GetPath();
            if (!polecenie) { tc->SetValue(sc); return; }
            if (argumenty.Trim().empty()) argumenty = " \"%f\"";
            tc->SetValue((sc.Contains(' ') ? "\"" + sc + "\"" : sc) + (argumenty.StartsWith(" ") ? argumenty : " " + argumenty));
        });
        return wiersz;
    };
    wxStaticBoxSizer* sboxJezyk = new wxStaticBoxSizer(wxVERTICAL, panelInne, TR("Język i Literatura"));
    wxFlexGridSizer* gridJezyk = new wxFlexGridSizer(2, 5, 10);
    gridJezyk->AddGrowableCol(1);
    gridJezyk->Add(new wxStaticText(panelInne, wxID_ANY, TR("Język interfejsu:")), 0, wxALIGN_CENTER_VERTICAL);
    gridJezyk->Add(choiceJezyk, 0);
    gridJezyk->Add(new wxStaticText(panelInne, wxID_ANY, TR("Polecenie czytnika PDF:")), 0, wxALIGN_CENTER_VERTICAL);
    gridJezyk->Add(poleProgramu(tcCzytnikPDF, true), 1, wxEXPAND);
    gridJezyk->Add(new wxStaticText(panelInne, wxID_ANY, TR("Program ExifTool:")), 0, wxALIGN_CENTER_VERTICAL);
    gridJezyk->Add(poleProgramu(tcExifTool, false), 1, wxEXPAND);
    gridJezyk->Add(new wxStaticText(panelInne, wxID_ANY, TR("Program ocrmypdf:")), 0, wxALIGN_CENTER_VERTICAL);
    gridJezyk->Add(poleProgramu(tcOcrmypdf, false), 1, wxEXPAND);
    gridJezyk->Add(new wxStaticText(panelInne, wxID_ANY, TR("Języki OCR (Tesseract):")), 0, wxALIGN_CENTER_VERTICAL);
    gridJezyk->Add(tcOcrJezyki, 1, wxEXPAND);
    gridJezyk->Add(new wxStaticText(panelInne, wxID_ANY, TR("Klucz API Mouser (datasheety):")), 0, wxALIGN_CENTER_VERTICAL);
    gridJezyk->Add(tcMouserKlucz, 1, wxEXPAND);
    sboxJezyk->Add(gridJezyk, 0, wxEXPAND | wxALL, 5);
    sboxJezyk->Add(new wxStaticText(panelInne, wxID_ANY, TR("W poleceniu %f to plik, a %p numer strony. Zmiana języka działa po ponownym uruchomieniu programu.")),
                   0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    sizer3->Add(sboxJezyk, 0, wxEXPAND | wxALL, 5);
    sizer3->AddStretchSpacer();

    panelInne->SetSizer(sizer3);
    notebook->AddPage(panelInne, TR("Inne"));
    PomocGUI::Dodaj(tcSciezkaObrazki, "ustawienia.obrazki");
    PomocGUI::Dodaj(tcSciezkaPDF, "ustawienia.pdf");
    PomocGUI::Dodaj(choiceTypEAN, "ustawienia.typ_kodu");
    PomocGUI::Dodaj(scPrefixEAN, "ustawienia.prefiks");
    PomocGUI::Dodaj(choiceFonty, "ustawienia.czcionka");
    PomocGUI::Dodaj(choiceJezyk, "ustawienia.jezyk");
    PomocGUI::Dodaj(tcCzytnikPDF, "ustawienia.czytnik_pdf");
    PomocGUI::Dodaj(tcExifTool, "ustawienia.exiftool");
    PomocGUI::Dodaj(tcOcrmypdf, "ustawienia.ocrmypdf");
    PomocGUI::Dodaj(tcOcrJezyki, "ustawienia.ocr_jezyki");
    PomocGUI::Dodaj(tcMouserKlucz, "ustawienia.mouser_klucz");

    mainSizer->Add(notebook, 1, wxEXPAND | wxALL, 5);

    // === PRZYCISKI ===
    wxBoxSizer* hSizerBtn = new wxBoxSizer(wxHORIZONTAL);
    hSizerBtn->AddStretchSpacer();
    hSizerBtn->Add(btnOK, 0, wxRIGHT, 5);
    hSizerBtn->Add(btnAnuluj, 0);
    mainSizer->Add(hSizerBtn, 0, wxALL | wxEXPAND, 5);

    SetSizer(mainSizer);
}

void DialogUstawien::OdswiezOpisBazy() {
    const UstawieniaBazy& us = ManagerBazyDanych::pobierz().pobierzUstawienia();
    stOpisBaza->SetLabel(
        TR("Serwer: ") + TekstZUTF8(us.host) + wxString::Format(":%d", us.port) + "\n" +
        TR("Baza: ") + TekstZUTF8(us.bazaDanych) + "\n" +
        TR("Użytkownik: ") + TekstZUTF8(us.uzytkownik));
    stOpisBaza->GetParent()->Layout();
}

void DialogUstawien::OnUstawieniaPolaczenia(wxCommandEvent&) {
    DialogPolaczenia dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        zmienionoPolaczenie = true;
        OdswiezOpisBazy();
    }
}

void DialogUstawien::WczytajUstawienia() {
    const UstawieniaBazowe us = Konfiguracja::pobierz().pobierzUstawieniaAplikacji();

    OdswiezOpisBazy();

    tcSciezkaObrazki->SetValue(TekstZUTF8(us.sciezkaObrazki));
    tcSciezkaPDF->SetValue(TekstZUTF8(us.sciezkaPDFDatasheet));
    tcRepozytorium->SetValue(TekstZUTF8(Konfiguracja::pobierz().pobierz("aktualizacje", "repozytorium", Aktualizacje::adresRepozytorium())));

    {
        const std::string j = Konfiguracja::pobierz().pobierz("interfejs", "jezyk", "");
        choiceJezyk->SetSelection(j == "pl" ? 1 : (j == "en" ? 2 : 0));
        tcCzytnikPDF->SetValue(TekstZUTF8(Konfiguracja::pobierz().pobierz("literatura", "polecenie_pdf", "")));
        tcExifTool->SetValue(TekstZUTF8(Konfiguracja::pobierz().pobierz("literatura", "exiftool", "")));
        tcOcrmypdf->SetValue(TekstZUTF8(Konfiguracja::pobierz().pobierz("literatura", "ocrmypdf", "")));
        tcOcrJezyki->SetValue(TekstZUTF8(Konfiguracja::pobierz().pobierz("literatura", "ocr_jezyki", "")));
        tcMouserKlucz->SetValue(TekstZUTF8(Konfiguracja::pobierz().pobierz("datasheet", "mouser_klucz", "")));
    }
    int typ = choiceTypEAN->FindString(TekstZUTF8(us.typKoduEAN));
    choiceTypEAN->SetSelection(typ != wxNOT_FOUND ? typ : 0);
    scPrefixEAN->SetValue(us.prefixEAN);

    // Czcionka z pliku może nie być na liście - dodaj ją
    const wxString nazwaFontu = TekstZUTF8(us.nazwaFontu);
    int font = choiceFonty->FindString(nazwaFontu);
    if (font == wxNOT_FOUND) font = choiceFonty->Append(nazwaFontu);
    choiceFonty->SetSelection(font);
    scRozmiarFontu->SetValue(us.rozmiarFontu);
    cbPogrubienie->SetValue(us.pogrubienie);
    cbKursywa->SetValue(us.kursywa);

    AktualizujPrzeglad();
}

void DialogUstawien::OnBrowseObrazki(wxCommandEvent& event) {
    wxDirDialog dlg(this, TR("Wybierz folder obrazków"), tcSciezkaObrazki->GetValue());
    if (dlg.ShowModal() == wxID_OK) {
        tcSciezkaObrazki->SetValue(dlg.GetPath());
    }
}

void DialogUstawien::OnBrowsePDF(wxCommandEvent& event) {
    wxDirDialog dlg(this, TR("Wybierz folder PDF"), tcSciezkaPDF->GetValue());
    if (dlg.ShowModal() == wxID_OK) {
        tcSciezkaPDF->SetValue(dlg.GetPath());
    }
}

void DialogUstawien::OnFontChanged(wxCommandEvent& event) {
    AktualizujPrzeglad();
}

void DialogUstawien::AktualizujPrzeglad() {
    wxString nazwaFontu = choiceFonty->GetStringSelection();
    int rozmiar = scRozmiarFontu->GetValue();
    wxFontStyle styl = cbKursywa->GetValue() ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL;
    wxFontWeight waga = cbPogrubienie->GetValue() ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL;

    wxFont font(wxFontInfo(rozmiar).Family(wxFONTFAMILY_DEFAULT).Style(styl).Weight(waga).FaceName(nazwaFontu));
    stPrzeglad->SetFont(font);
}

void DialogUstawien::OnOK(wxCommandEvent&) {
    Konfiguracja& kfg = Konfiguracja::pobierz();

    // Zachowaj to, czego to okno nie edytuje (np. progi kolorów)
    UstawieniaBazowe us = kfg.pobierzUstawieniaAplikacji();
    us.sciezkaObrazki = tcSciezkaObrazki->GetValue().utf8_string();
    us.sciezkaPDFDatasheet = tcSciezkaPDF->GetValue().utf8_string();
    us.typKoduEAN = choiceTypEAN->GetStringSelection().utf8_string();
    us.prefixEAN = scPrefixEAN->GetValue();
    us.nazwaFontu = choiceFonty->GetStringSelection().utf8_string();
    us.rozmiarFontu = scRozmiarFontu->GetValue();
    us.pogrubienie = cbPogrubienie->GetValue();
    us.kursywa = cbKursywa->GetValue();

    {
        const std::string repo = tcRepozytorium->GetValue().utf8_string();
        if (!repo.empty() && Aktualizacje::adresWersji(repo).empty()) {
            wxMessageBox(TR("Adres repozytorium musi mieć postać https://github.com/wlasciciel/repozytorium"),
                         TR("Aktualizacje"), wxOK | wxICON_WARNING, this);
            return;
        }
        if (repo.empty() || repo == Aktualizacje::adresRepozytorium()) kfg.usun("aktualizacje", "repozytorium");
        else kfg.ustaw("aktualizacje", "repozytorium", repo);
    }
    {
        const int j = choiceJezyk->GetSelection();
        if (j <= 0) kfg.usun("interfejs", "jezyk"); else kfg.ustaw("interfejs", "jezyk", j == 1 ? "pl" : "en");
        const std::string czytnik = tcCzytnikPDF->GetValue().Strip(wxString::both).utf8_string();
        if (czytnik.empty()) kfg.usun("literatura", "polecenie_pdf"); else kfg.ustaw("literatura", "polecenie_pdf", czytnik);
        const std::pair<const char*, wxTextCtrl*> programy[] = {{"exiftool", tcExifTool}, {"ocrmypdf", tcOcrmypdf}, {"ocr_jezyki", tcOcrJezyki}};
        for (const auto& pr : programy) {
            const std::string v = pr.second->GetValue().Strip(wxString::both).utf8_string();
            if (v.empty()) kfg.usun("literatura", pr.first); else kfg.ustaw("literatura", pr.first, v);
        }
        const std::string klucz = tcMouserKlucz->GetValue().Strip(wxString::both).utf8_string();
        if (klucz.empty()) kfg.usun("datasheet", "mouser_klucz"); else kfg.ustaw("datasheet", "mouser_klucz", klucz);
    }
    kfg.ustawUstawieniaAplikacji(us);
    if (!kfg.zapisz()) {
        wxMessageBox(TR("Nie udało się zapisać ustawień:\n\n") + TekstZUTF8(kfg.ostatniBlad()),
                     TR("Błąd zapisu"), wxOK | wxICON_ERROR, this);
        return; // zostaw okno otwarte
    }

    EndModal(wxID_OK);
}

void DialogUstawien::OnAnuluj(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}
