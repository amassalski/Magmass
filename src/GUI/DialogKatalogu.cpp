#include "DialogKatalogu.h"
#include "../Logika/OpisyParametrow.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"
#include "../Narzedzia/Indeksy.h"
#include "ZdjeciaElementow.h"

namespace {
std::string Tekst(const wxTextCtrl* t) { return t->GetValue().Strip(wxString::both).utf8_string(); }
}

// ---------------------------------------------------------------------------
DialogKatalogu::DialogKatalogu(wxWindow* parent, const PozycjaKatalogu& pozycja, const std::vector<std::string>& rodziny)
    : wxDialog(parent, wxID_ANY, pozycja.id == 0 ? TR("Nowa pozycja katalogu") : TR("Pozycja katalogu"),
               wxDefaultPosition, wxSize(640, 640), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      pozycja_(pozycja) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    g->AddGrowableCol(1);
    auto pole = [&](const wxString& et, wxTextCtrl*& tc, const std::string& wart, const char* klucz, long styl = 0) {
        g->Add(new wxStaticText(this, wxID_ANY, et), 0, wxALIGN_CENTER_VERTICAL);
        tc = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(wart), wxDefaultPosition, wxDefaultSize, styl);
        PomocGUI::Dodaj(tc, klucz);
        g->Add(tc, 1, wxEXPAND);
    };
    pole(TR("Symbol:"), tcSymbol, pozycja.symbol, "katalog.symbol");
    pole(TR("Producent:"), tcProducent, pozycja.producent, "katalog.producent");
    pole(TR("Opis:"), tcOpis, pozycja.opis, "katalog.opis");
    pole(TR("Obudowa:"), tcObudowa, pozycja.obudowa, "katalog.obudowa");
    g->Add(new wxStaticText(this, wxID_ANY, TR("Rodzina:")), 0, wxALIGN_CENTER_VERTICAL);
    cbRodzina = new wxComboBox(this, wxID_ANY, TekstZUTF8(pozycja.rodzina));
    for (const std::string& r : rodziny) cbRodzina->Append(TekstZUTF8(r));
    PomocGUI::Dodaj(cbRodzina, "katalog.rodzina");
    g->Add(cbRodzina, 1, wxEXPAND);
    pole(TR("Datasheet (adres):"), tcDatasheet, pozycja.datasheetUrl, "katalog.datasheet");
    g->Add(new wxStaticText(this, wxID_ANY, TR("Zdjęcie:")), 0, wxALIGN_CENTER_VERTICAL);
    tcZdjecie = new wxTextCtrl(this, wxID_ANY, TekstZUTF8(pozycja.zdjecie));
    tcZdjecie->SetHint(TR("plik obrazu"));
    PomocGUI::Dodaj(tcZdjecie, "katalog.zdjecie");
    g->Add(ZdjeciaElementow::WierszWyboru(this, tcZdjecie, [this] { return tcSymbol->GetValue(); }), 1, wxEXPAND);
    pole(TR("Uwagi:"), tcUwagi, pozycja.uwagi, "katalog.uwagi");
    glowny->Add(g, 0, wxEXPAND | wxALL, 10);

    glowny->Add(new wxStaticText(this, wxID_ANY, TR("Parametry (indeksy: V_CE albo V_{CE(sat)}, potęgi: m^2, 10^-3):")), 0, wxLEFT | wxRIGHT, 10);
    siatka = new wxGrid(this, wxID_ANY);
    siatka->CreateGrid(0, 4);
    siatka->SetColLabelValue(0, TR("Nazwa"));
    siatka->SetColLabelValue(1, TR("Wartość"));
    siatka->SetColLabelValue(2, TR("Źródło"));
    siatka->SetColLabelValue(3, TR("Opis (dymek)"));
    siatka->SetColSize(0, 160); siatka->SetColSize(1, 200); siatka->SetColSize(2, 80); siatka->SetColSize(3, 260);
    siatka->SetRowLabelSize(30);
    siatka->EnableDragRowSize(false);
    PomocGUI::Dodaj(siatka, "katalog.parametry");
    glowny->Add(siatka, 1, wxEXPAND | wxALL, 10);
    for (const ParametrKatalogu& p : pozycja.parametry) {
        siatka->AppendRows(1);
        const int r = siatka->GetNumberRows() - 1;
        siatka->SetCellValue(r, 0, TekstZUTF8(p.nazwa));
        siatka->SetCellValue(r, 1, TekstZUTF8(p.wartosc));
        siatka->SetCellValue(r, 2, TekstZUTF8(p.zrodlo));
        siatka->SetCellValue(r, 3, TekstZUTF8(p.opis));
        siatka->SetReadOnly(r, 2);
    }
    // dymek z opisem parametru po najechaniu na nazwę lub wartość (własny opis albo ze słownika znanych symboli)
    siatka->GetGridWindow()->Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        int x, y;
        siatka->CalcUnscrolledPosition(e.GetX(), e.GetY(), &x, &y);
        const int r = siatka->YToRow(y), c = siatka->XToCol(x);
        wxString tip;
        if (r != wxNOT_FOUND && c != wxNOT_FOUND && c <= 1) tip = OpisParametru(r);
        wxWindow* okno = siatka->GetGridWindow();
        const wxString teraz = okno->GetToolTipText();
        if (tip != teraz) { if (tip.empty()) okno->UnsetToolTip(); else okno->SetToolTip(tip); }
        e.Skip();
    });

    podglad = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 34), wxHW_SCROLLBAR_NEVER | wxBORDER_THEME);
    PomocGUI::Dodaj(podglad, "katalog.parametry_podglad");
    glowny->Add(podglad, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    siatka->Bind(wxEVT_GRID_SELECT_CELL, [this](wxGridEvent& e) { WybierzWiersz(e.GetRow()); e.Skip(); });
    siatka->Bind(wxEVT_GRID_CELL_CHANGED, [this](wxGridEvent& e) { WybierzWiersz(e.GetRow()); e.Skip(); });
    wxBoxSizer* przyciskiParam = new wxBoxSizer(wxHORIZONTAL);
    wxButton* bDodaj = new wxButton(this, wxID_ANY, TR("Dodaj parametr"));
    wxButton* bUsun = new wxButton(this, wxID_ANY, TR("Usuń parametr"));
    przyciskiParam->Add(bDodaj, 0, wxRIGHT, 6);
    przyciskiParam->Add(bUsun, 0);
    glowny->Add(przyciskiParam, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    bDodaj->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        siatka->AppendRows(1);
        const int r = siatka->GetNumberRows() - 1;
        siatka->SetCellValue(r, 2, "reczny");
        siatka->SetReadOnly(r, 2);
        siatka->GoToCell(r, 0);
        siatka->SetFocus();
    });
    bUsun->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        const int r = siatka->GetGridCursorRow();
        if (r >= 0 && r < siatka->GetNumberRows()) siatka->DeleteRows(r, 1);
    });

    glowny->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
    SetSizer(glowny);
    Bind(wxEVT_BUTTON, &DialogKatalogu::OnOK, this, wxID_OK);
    SetMinSize(wxSize(520, 520));
}

void DialogKatalogu::DodajParametr(const wxString& nazwa, const wxString& wartosc) {
    siatka->AppendRows(1);
    const int r = siatka->GetNumberRows() - 1;
    siatka->SetCellValue(r, 0, nazwa);
    siatka->SetCellValue(r, 1, wartosc);
    siatka->SetCellValue(r, 2, "reczny");
}

wxString DialogKatalogu::OpisParametru(int r) const {
    if (r < 0 || r >= siatka->GetNumberRows()) return wxString();
    ParametrKatalogu p;
    p.nazwa = siatka->GetCellValue(r, 0).Strip(wxString::both).utf8_string();
    p.opis = siatka->GetCellValue(r, 3).Strip(wxString::both).utf8_string();
    return TekstZUTF8(OpisyParametrow::dlaParametru(p));
}

int DialogKatalogu::LiczbaParametrow() const { return siatka->GetNumberRows(); }

void DialogKatalogu::WybierzWiersz(int r) {
    if (r < 0 || r >= siatka->GetNumberRows()) { htmlPodgladu_.clear(); OdswiezPodglad(); return; }
    const std::string n = siatka->GetCellValue(r, 0).utf8_string();
    const std::string w = siatka->GetCellValue(r, 1).utf8_string();
    htmlPodgladu_ = wxString::FromUTF8("<b>" + Indeksy::naHtml(n) + "</b>: " + Indeksy::naHtml(w));
    OdswiezPodglad();
}

void DialogKatalogu::OdswiezPodglad() {
    if (podglad) podglad->SetPage("<html><body>" + htmlPodgladu_ + "</body></html>");
}

PozycjaKatalogu DialogKatalogu::Pobierz() const {
    PozycjaKatalogu k = pozycja_;
    k.symbol = Tekst(tcSymbol);
    k.producent = Tekst(tcProducent);
    k.opis = Tekst(tcOpis);
    k.obudowa = Tekst(tcObudowa);
    k.rodzina = cbRodzina->GetValue().Strip(wxString::both).utf8_string();
    k.datasheetUrl = Tekst(tcDatasheet);
    k.uwagi = Tekst(tcUwagi);
    k.zdjecie = Tekst(tcZdjecie);
    k.parametry.clear();
    for (int r = 0; r < siatka->GetNumberRows(); ++r) {
        const std::string n = siatka->GetCellValue(r, 0).Strip(wxString::both).utf8_string();
        const std::string w = siatka->GetCellValue(r, 1).Strip(wxString::both).utf8_string();
        std::string z = siatka->GetCellValue(r, 2).utf8_string();
        if (n.empty() || w.empty()) continue;
        k.parametry.push_back({n, w, z.empty() ? "reczny" : z, siatka->GetCellValue(r, 3).Strip(wxString::both).utf8_string()});
    }
    return k;
}

void DialogKatalogu::OnOK(wxCommandEvent& event) {
    siatka->SaveEditControlValue();
    if (Tekst(tcSymbol).empty()) {
        wxMessageBox(TR("Podaj symbol elementu."), TR("Katalog"), wxOK | wxICON_WARNING, this);
        tcSymbol->SetFocus();
        return;
    }
    event.Skip();
}

// ---------------------------------------------------------------------------
DialogDoMagazynu::DialogDoMagazynu(wxWindow* parent, LogikaMagazynu& magazyn, const wxString& symbol)
    : wxDialog(parent, wxID_ANY, TR("Dodaj do magazynu: ") + symbol, wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 6, 8);
    g->AddGrowableCol(1);
    g->Add(new wxStaticText(this, wxID_ANY, TR("Ilość:")), 0, wxALIGN_CENTER_VERTICAL);
    scIlosc = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxSize(120, -1), wxSP_ARROW_KEYS, 0, 1000000, 0);
    g->Add(scIlosc, 0);
    g->Add(new wxStaticText(this, wxID_ANY, TR("Miejsce:")), 0, wxALIGN_CENTER_VERTICAL);
    tcMiejsce = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(280, -1));
    tcMiejsce->SetHint(TR("np. Kontener 1 szufladka A1/1"));
    PomocGUI::Dodaj(tcMiejsce, "szereg.miejsce");
    g->Add(tcMiejsce, 1, wxEXPAND);
    g->Add(new wxStaticText(this, wxID_ANY, TR("Kategoria:")), 0, wxALIGN_CENTER_VERTICAL);
    chKategoria = new wxChoice(this, wxID_ANY);
    chKategoria->Append(TR("(bez kategorii)"));
    idKategorii_.push_back(0);
    if (magazyn.kategorie().empty()) magazyn.odswiezKategorie();
    for (const Kategoria& k : magazyn.kategorie()) {
        chKategoria->Append(TekstZUTF8(k.pelnaNazwa));
        idKategorii_.push_back(k.id);
    }
    chKategoria->SetSelection(0);
    g->Add(chKategoria, 1, wxEXPAND);
    glowny->Add(g, 0, wxEXPAND | wxALL, 12);
    glowny->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(glowny);
}

MiejsceMagazynowe DialogDoMagazynu::Miejsce() const {
    return MiejsceMagazynowe::parsuj(tcMiejsce->GetValue().Strip(wxString::both).utf8_string());
}

std::optional<long long> DialogDoMagazynu::WybranaKategoria() const {
    const int i = chKategoria->GetSelection();
    if (i <= 0 || static_cast<size_t>(i) >= idKategorii_.size()) return std::nullopt;
    return idKategorii_[static_cast<size_t>(i)];
}

// ---------------------------------------------------------------------------
DialogWyboruZKatalogu::DialogWyboruZKatalogu(wxWindow* parent, LogikaKatalogu& katalog, const wxString& fraza)
    : wxDialog(parent, wxID_ANY, TR("Wybierz z katalogu"), wxDefaultPosition, wxSize(760, 480),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      katalog_(katalog) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    szukaj_ = new wxSearchCtrl(this, wxID_ANY, fraza);
    szukaj_->SetDescriptiveText(TR("Szukaj: symbol, producent, opis, rodzina..."));
    glowny->Add(szukaj_, 0, wxEXPAND | wxALL, 10);
    widok_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_THEME);
    const struct { const char* t; int s; } kol[] = {{"Symbol", 140}, {"Producent", 120}, {"Opis", 280}, {"Obudowa", 90}, {"W magazynie", 90}};
    for (int i = 0; i < 5; ++i) widok_->InsertColumn(i, TR(kol[i].t), i == 4 ? wxLIST_FORMAT_RIGHT : wxLIST_FORMAT_LEFT, kol[i].s);
    glowny->Add(widok_, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
    glowny->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
    SetSizer(glowny);

    szukaj_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Szukaj(szukaj_->GetValue()); });
    widok_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& e) { if (Wybierz(static_cast<size_t>(e.GetIndex()))) EndModal(wxID_OK); });
    Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) {
        const long i = widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (i >= 0 && Wybierz(static_cast<size_t>(i))) EndModal(wxID_OK); else e.Skip(false);
    }, wxID_OK);
    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
        e.Enable(widok_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) >= 0);
    }, wxID_OK);
    Szukaj(fraza);
}

void DialogWyboruZKatalogu::Szukaj(const wxString& tekst) {
    FiltrKatalogu f;
    f.tekst = tekst.utf8_string();
    f.limit = 500;
    widok_->DeleteAllItems();
    if (!katalog_.pobierz(f, lista_)) return;
    for (size_t i = 0; i < lista_.size(); ++i) {
        const PozycjaKatalogu& k = lista_[i];
        const long r = widok_->InsertItem(static_cast<long>(i), TekstZUTF8(k.symbol));
        widok_->SetItem(r, 1, TekstZUTF8(k.producent));
        widok_->SetItem(r, 2, TekstZUTF8(k.opis));
        widok_->SetItem(r, 3, TekstZUTF8(k.obudowa));
        widok_->SetItem(r, 4, wxString::Format("%d", k.wMagazynie));
    }
    if (!lista_.empty()) widok_->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

bool DialogWyboruZKatalogu::Wybierz(size_t i) {
    if (i >= lista_.size()) return false;
    wybrana_ = katalog_.pobierzPozycje(lista_[i].id);
    return wybrana_.has_value();
}
