#include "PomocGUI.h"
#include "PomocKontekstowa.h"
#include "../BazaDanych/ManagerBazyDanych.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "ObrazyPomocy.h"

#include <wx/filename.h>
#include <wx/fs_mem.h>
#include <wx/wfstream.h>
#include <wx/image.h>
#include <wx/spinctrl.h>
#include <wx/html/htmlwin.h>
#include <wx/mimetype.h>
#include <wx/stdpaths.h>
#include <wx/tooltip.h>
#include <wx/utils.h>

#include <cctype>
#include <memory>
#include <unordered_map>
#include <vector>

namespace PomocGUI {
namespace {

std::unordered_map<const wxWindow*, std::string>& Klucze() {
    static std::unordered_map<const wxWindow*, std::string> m;
    return m;
}

wxString PlikPomocy() {
    const std::string kat = ZnajdzKatalogPomocy();
    if (kat.empty()) return wxString();
    return wxFileName(wxString::FromUTF8(kat), "pomoc_pl.htm").GetFullPath();
}

// Obrazy pomocy są wkompilowane w program; do wxHtmlWindow trafiają przez pamięciowy system plików
// (src="obrazy/x.png" -> src="memory:x.png"), do zewnętrznej przeglądarki przez katalog tymczasowy.
void RejestrujObrazyPomocy() {
    static bool zrobione = false;
    if (zrobione) return;
    zrobione = true;
    wxFileSystem::AddHandler(new wxMemoryFSHandler);
    for (const ObrazPomocy& o : ObrazyPomocy())
        wxMemoryFSHandler::AddFileWithMimeType(wxString::FromUTF8(o.nazwa), o.dane, o.rozmiar, "image/png");
}

wxString TekstDymka(const WpisPomocy& w) {
    wxString t = wxString::FromUTF8(w.opis);
    if (!w.tytul.empty()) t = wxString::FromUTF8(w.tytul) + ":\n" + t;
    return t;
}

// ---------------------------------------------------------------------------
// Przeglądarka pomocy: wxHtmlWindow z własną historią i obsługą odnośników #kotwica
// ---------------------------------------------------------------------------
class Przegladarka : public wxHtmlWindow {
public:
    Przegladarka(wxWindow* rodzic) : wxHtmlWindow(rodzic, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                  wxHW_SCROLLBAR_AUTO | wxBORDER_THEME) {}

    void Idz(const wxString& kotwica, bool zapiszHistorie = true) {
        if (zapiszHistorie) {
            historia_.resize(pozycja_ + 1);
            if (historia_.empty() || historia_.back() != kotwica) {
                historia_.push_back(kotwica);
                pozycja_ = historia_.size() - 1;
            }
        }
        if (kotwica.empty()) Scroll(0, 0);
        else if (!ScrollToAnchor(kotwica)) Scroll(0, 0);
    }
    bool MozeWstecz() const { return pozycja_ > 0; }
    bool MozeDalej() const { return pozycja_ + 1 < historia_.size(); }
    void Wstecz() { if (MozeWstecz()) { --pozycja_; Idz(historia_[pozycja_], false); } }
    void Dalej() { if (MozeDalej()) { ++pozycja_; Idz(historia_[pozycja_], false); } }

    void OnLinkClicked(const wxHtmlLinkInfo& link) override {
        const wxString href = link.GetHref();
        if (href.StartsWith("#")) {
            Idz(href.Mid(1));
            if (zmiana_) zmiana_();
        } else if (href.StartsWith("http://") || href.StartsWith("https://")) {
            wxLaunchDefaultBrowser(href);
        }
    }
    std::function<void()> zmiana_;

private:
    std::vector<wxString> historia_{wxString()};
    size_t pozycja_ = 0;
};

class OknoPomocy : public wxFrame {
public:
    OknoPomocy() : wxFrame(nullptr, wxID_ANY, TR("Magmass: pomoc"), wxDefaultPosition, wxSize(900, 650)) {
        wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* pasek = new wxBoxSizer(wxHORIZONTAL);
        bWstecz_ = new wxButton(this, wxID_ANY, TR("&Wstecz"));
        bDalej_ = new wxButton(this, wxID_ANY, TR("&Dalej"));
        wxButton* bSpis = new wxButton(this, wxID_ANY, TR("&Spis treści"));
        wxButton* bPrzegl = new wxButton(this, wxID_ANY, TR("Otwórz w &przeglądarce"));
        for (wxButton* b : {bWstecz_, bDalej_, bSpis}) pasek->Add(b, 0, wxRIGHT, 6);
        pasek->AddStretchSpacer(1);
        pasek->Add(bPrzegl, 0);
        g->Add(pasek, 0, wxEXPAND | wxALL, 6);

        html_ = new Przegladarka(this);
        g->Add(html_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
        SetSizer(g);

        html_->zmiana_ = [this] { Odswiez(); };
        bWstecz_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { html_->Wstecz(); Odswiez(); });
        bDalej_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { html_->Dalej(); Odswiez(); });
        bSpis->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { html_->Idz(wxString()); Odswiez(); });
        bPrzegl->Bind(wxEVT_BUTTON, [](wxCommandEvent&) { OtworzWPrzegladarce(); });
        Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
            if (e.GetKeyCode() == WXK_ESCAPE) Close(); else e.Skip();
        });
    }

    bool Zaladuj() {
        const wxString plik = PlikPomocy();
        if (plik.empty()) return false;
        wxString tresc;
        {
            wxFile f(plik);
            if (!f.IsOpened() || !f.ReadAll(&tresc, wxConvUTF8)) return false;
        }
        RejestrujObrazyPomocy();
        tresc.Replace("src=\"obrazy/", "src=\"memory:");
        tresc.Replace("src=obrazy/", "src=memory:");
        html_->SetPage(tresc);
        Odswiez();
        return true;
    }
    void Idz(const wxString& kotwica) { html_->Idz(kotwica); Odswiez(); }

private:
    void Odswiez() {
        bWstecz_->Enable(html_->MozeWstecz());
        bDalej_->Enable(html_->MozeDalej());
    }
    Przegladarka* html_ = nullptr;
    wxButton *bWstecz_ = nullptr, *bDalej_ = nullptr;
};

OknoPomocy* g_okno = nullptr;

// Okno z opisem pola (F1)
void PokazOpisPola(wxWindow* rodzic, const WpisPomocy& w) {
    wxDialog dlg(rodzic, wxID_ANY, TR("Pomoc: ") + wxString::FromUTF8(w.tytul), wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE);
    wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
    wxStaticText* opis = new wxStaticText(&dlg, wxID_ANY, wxString::FromUTF8(w.opis));
    opis->Wrap(420);
    g->Add(opis, 0, wxALL, 14);
    wxBoxSizer* przyciski = new wxBoxSizer(wxHORIZONTAL);
    wxButton* wiecej = new wxButton(&dlg, wxID_ANY, TR("Więcej w pomocy"));
    wiecej->Enable(!w.kotwica.empty() && !PlikPomocy().empty());
    wxButton* zamknij = new wxButton(&dlg, wxID_OK, TR("Zamknij"));
    przyciski->Add(wiecej, 0, wxRIGHT, 8);
    przyciski->Add(zamknij, 0);
    g->Add(przyciski, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 14);
    dlg.SetSizerAndFit(g);
    zamknij->SetDefault();
    zamknij->SetFocus();
    bool otworzPomoc = false;
    wiecej->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { otworzPomoc = true; dlg.EndModal(wxID_OK); });
    dlg.CentreOnParent();
    dlg.ShowModal();
    if (otworzPomoc) PokazPomoc(w.kotwica);
}

// Filtr zdarzeń: F1 w dowolnym oknie programu
class FiltrF1 : public wxEventFilter {
public:
    int FilterEvent(wxEvent& e) override {
        if (e.GetEventType() == wxEVT_KEY_DOWN) {
            const wxKeyEvent& k = static_cast<const wxKeyEvent&>(e);
            if (k.GetKeyCode() == WXK_F1 && !k.HasAnyModifiers()) {
                PokazPomocDlaFokusu();
                return Event_Processed;
            }
        }
        return Event_Skip;
    }
};

} // namespace

std::string ZnajdzKatalogPomocy() {
    wxString srodowisko;
    if (wxGetEnv("DM_POMOC", &srodowisko) && wxFileExists(wxFileName(srodowisko, "pomoc_pl.htm").GetFullPath()))
        return srodowisko.utf8_string();   // jawne wskazanie katalogu (testy, instalacje niestandardowe)
    const wxString exe = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    const wxString biezacy = wxGetCwd();
    auto wyzej = [](const wxString& k) {
        wxFileName fn(k, wxEmptyString);
        fn.RemoveLastDir();
        return fn.GetPath();
    };
    for (const wxString& baza : {exe, wxStandardPaths::Get().GetResourcesDir(), wyzej(exe), biezacy, wyzej(biezacy)}) {
        const wxFileName kat(baza + wxFileName::GetPathSeparator() + "pomoc", wxEmptyString);
        if (wxFileExists(wxFileName(kat.GetPath(), "pomoc_pl.htm").GetFullPath()))
            return kat.GetPath().utf8_string();
    }
    return std::string();
}

void WczytajOpisyZPliku() {
    const std::string kat = ZnajdzKatalogPomocy();
    if (kat.empty()) return;
    PomocKontekstowa::pobierz().wczytajZPliku(wxFileName(wxString::FromUTF8(kat), "pola.txt").GetFullPath().utf8_string());
}

void WczytajOpisyZBazy() {
    PomocKontekstowa::pobierz().wczytajZBazy(ManagerBazyDanych::pobierz());
}

std::string KluczPola(const wxWindow* kontrolka, const std::string& grupa) {
    // opis pola z etykiety dostają tylko pola wprowadzania i etykiety wyników; przyciski, suwaki i listy mają klucz grupy
    const bool obslugiwana = dynamic_cast<const wxTextCtrl*>(kontrolka) || dynamic_cast<const wxChoice*>(kontrolka) ||
                             dynamic_cast<const wxComboBox*>(kontrolka) || dynamic_cast<const wxCheckBox*>(kontrolka) ||
                             dynamic_cast<const wxRadioButton*>(kontrolka) || dynamic_cast<const wxSpinCtrl*>(kontrolka) ||
                             dynamic_cast<const wxStaticText*>(kontrolka);
    if (!kontrolka || !obslugiwana) return grupa;
    wxString et;
    if (auto* cb = dynamic_cast<const wxCheckBox*>(kontrolka)) et = cb->GetLabel();
    else if (auto* rb = dynamic_cast<const wxRadioButton*>(kontrolka)) et = rb->GetLabel();
    else if (const wxWindow* rodzic = kontrolka->GetParent()) {
        const wxStaticText* poprz = nullptr;
        for (const wxWindow* r : rodzic->GetChildren()) {
            if (r == kontrolka) break;
            if (auto* st = dynamic_cast<const wxStaticText*>(r)) poprz = st;
        }
        if (poprz) et = poprz->GetLabel();
    }
    std::string tekst = et.utf8_string();
    auto it = OryginalyTlumaczen().find(tekst);
    if (it != OryginalyTlumaczen().end()) tekst = it->second;
    // bez jednostek [..] i wyjaśnień w nawiasach okrągłych; zostają tylko numery typu "(23)"
    std::string czysty;
    for (size_t i = 0; i < tekst.size(); i++) {
        const char c = tekst[i];
        if (c == '[') { const size_t k = tekst.find(']', i); if (k == std::string::npos) break; i = k; continue; }
        if (c == '(') {
            const size_t k = tekst.find(')', i);
            if (k == std::string::npos) break;
            const std::string wnetrze = tekst.substr(i + 1, k - i - 1);
            bool numer = !wnetrze.empty();
            for (char w : wnetrze) if (!std::isdigit(static_cast<unsigned char>(w))) numer = false;
            if (numer) czysty += " " + wnetrze;
            i = k;
            continue;
        }
        czysty += c;
    }
    static const char* pl[][2] = {{"ą", "a"}, {"ć", "c"}, {"ę", "e"}, {"ł", "l"}, {"ń", "n"}, {"ó", "o"}, {"ś", "s"}, {"ź", "z"}, {"ż", "z"},
                                  {"Ą", "a"}, {"Ć", "c"}, {"Ę", "e"}, {"Ł", "l"}, {"Ń", "n"}, {"Ó", "o"}, {"Ś", "s"}, {"Ź", "z"}, {"Ż", "z"},
                                  {"μ", "mi"}, {"Δ", "d"}, {"Ω", "om"}, {"Σ", "s"}, {"²", "2"}, {"³", "3"}};
    std::string slug;
    for (size_t i = 0; i < czysty.size();) {
        bool zamieniono = false;
        for (auto& z : pl) {
            const size_t n = std::char_traits<char>::length(z[0]);
            if (czysty.compare(i, n, z[0]) == 0) { slug += z[1]; i += n; zamieniono = true; break; }
        }
        if (zamieniono) continue;
        const unsigned char c = czysty[i++];
        if (c < 128 && std::isalnum(c)) slug += static_cast<char>(std::tolower(c));
        else if (c < 128 && !slug.empty() && slug.back() != '_') slug += '_';
    }
    if (slug.size() > 40) slug.resize(40);
    while (!slug.empty() && slug.back() == '_') slug.pop_back();
    // pojedyncza duża litera (wymiary A i a rdzenia) różni się od małej
    if (slug.size() == 1 && std::isupper(static_cast<unsigned char>(czysty.find_first_not_of(" ") == std::string::npos ? ' ' : czysty[czysty.find_first_not_of(" ")])))
        slug = "wielkie_" + slug;
    return slug.empty() ? grupa : grupa + "." + slug;
}

namespace {
void Zastosuj(wxWindow* kontrolka, const std::string& klucz, bool szukajPola) {
    if (!kontrolka) return;
    // opis pojedynczego pola ("grupa.etykieta") ma pierwszeństwo przed opisem grupy
    std::string uzyty = klucz;
    const WpisPomocy* w = nullptr;
    if (szukajPola) {
        const std::string pole = KluczPola(kontrolka, klucz);
        if (pole != klucz) w = PomocKontekstowa::pobierz().znajdz(pole);
        if (w) uzyty = pole;
    }
    if (!w) w = PomocKontekstowa::pobierz().znajdz(klucz);
    if (w) kontrolka->SetToolTip(TekstDymka(*w));
    else if (kontrolka->GetToolTip()) kontrolka->UnsetToolTip();
    const bool nowy = Klucze().find(kontrolka) == Klucze().end();
    Klucze()[kontrolka] = uzyty;
    if (nowy) {
        kontrolka->Bind(wxEVT_DESTROY, [kontrolka](wxWindowDestroyEvent& e) {
            if (e.GetEventObject() == kontrolka) Klucze().erase(kontrolka);
            e.Skip();
        });
    }
}
}

void Dodaj(wxWindow* kontrolka, const std::string& klucz) { Zastosuj(kontrolka, klucz, true); }
void DodajPole(wxWindow* kontrolka, const std::string& klucz) { Zastosuj(kontrolka, klucz, false); }

std::string KluczKontrolki(const wxWindow* kontrolka) {
    auto it = Klucze().find(kontrolka);
    return it == Klucze().end() ? std::string() : it->second;
}

void DymekWiersza(wxListCtrl* lista, std::function<wxString(long)> tekstWiersza) {
    if (!lista) return;
    // Zdarzenie ruchu myszy przychodzi do okna wewnętrznego listy (GetMainWindow) na części systemów
    auto ostatni = std::make_shared<long>(-2);
    auto obsluga = [lista, tekstWiersza, ostatni](wxMouseEvent& e) {
        e.Skip();
        wxPoint p = e.GetPosition();
        wxWindow* zrodlo = dynamic_cast<wxWindow*>(e.GetEventObject());
        if (zrodlo && zrodlo != lista) p = lista->ScreenToClient(zrodlo->ClientToScreen(p));
        int flagi = 0;
        const long wiersz = lista->HitTest(p, flagi);
        if (wiersz == *ostatni) return;
        *ostatni = wiersz;
        const wxString t = wiersz >= 0 ? tekstWiersza(wiersz) : wxString();
        lista->UnsetToolTip();
        if (!t.empty()) lista->SetToolTip(t);
    };
    lista->Bind(wxEVT_MOTION, obsluga);
    for (wxWindowList::compatibility_iterator it = lista->GetChildren().GetFirst(); it; it = it->GetNext())
        it->GetData()->Bind(wxEVT_MOTION, obsluga);
    lista->Bind(wxEVT_LEAVE_WINDOW, [ostatni](wxMouseEvent& e) { *ostatni = -2; e.Skip(); });
}

void DymekZKolumn(wxListCtrl* lista) {
    DymekWiersza(lista, [lista](long wiersz) {
        wxString t;
        for (int k = 0; k < lista->GetColumnCount(); ++k) {
            const wxString v = lista->GetItemText(wiersz, k);
            if (v.empty()) continue;
            wxListItem kol;
            kol.SetMask(wxLIST_MASK_TEXT);
            lista->GetColumn(k, kol);
            if (!t.empty()) t += "\n";
            t += kol.GetText() + ": " + v;
        }
        return t;
    });
}

bool PokazPomoc(const std::string& kotwica) {
    if (PlikPomocy().empty()) {
        wxMessageBox(TR("Nie znaleziono pliku pomocy (katalog „pomoc” obok programu)."), TR("Pomoc"),
                     wxOK | wxICON_INFORMATION);
        return false;
    }
    // Okno utworzone przed oknem modalnym byłoby zablokowane, więc wtedy tworzymy je od nowa
    if (g_okno && !g_okno->IsEnabled()) {
        g_okno->Destroy();
        g_okno = nullptr;
    }
    if (!g_okno) {
        g_okno = new OknoPomocy;
        g_okno->Bind(wxEVT_DESTROY, [](wxWindowDestroyEvent& e) {
            if (e.GetEventObject() == g_okno) g_okno = nullptr;
            e.Skip();
        });
        if (!g_okno->Zaladuj()) {
            g_okno->Destroy();
            g_okno = nullptr;
            wxMessageBox(TR("Nie udało się wczytać pliku pomocy."), TR("Pomoc"), wxOK | wxICON_ERROR);
            return false;
        }
    }
    g_okno->Show();
    g_okno->Raise();
    g_okno->Idz(wxString::FromUTF8(kotwica));
    return true;
}

bool OtworzWPrzegladarce(const std::string& kotwica) {
    const wxString plik = PlikPomocy();
    if (plik.empty()) return false;
    wxString url = wxFileName::FileNameToURL(wxFileName(plik));
    {
        // kopia pomocy z obrazami w katalogu tymczasowym (obrazy są w programie, nie w pliku)
        wxFileName kat(wxFileName::GetTempDir(), wxEmptyString);
        kat.AppendDir("magmass_pomoc");
        kat.AppendDir("obrazy");
        if (wxFileName::Mkdir(kat.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
            for (const ObrazPomocy& o : ObrazyPomocy()) {
                wxFileOutputStream out(wxFileName(kat.GetPath(), wxString::FromUTF8(o.nazwa)).GetFullPath());
                if (out.IsOk()) out.Write(o.dane, o.rozmiar);
            }
            wxFileName cel(kat.GetPath(), "pomoc_pl.htm");
            cel.RemoveLastDir();
            cel.SetFullName("pomoc_pl.htm");
            if (wxCopyFile(plik, cel.GetFullPath(), true)) url = wxFileName::FileNameToURL(cel);
        }
    }
    if (!kotwica.empty()) url += "#" + wxString::FromUTF8(kotwica);
    return wxLaunchDefaultBrowser(url);
}

void PokazPomocDlaFokusu() {
    wxWindow* okno = wxWindow::FindFocus();
    wxWindow* rodzic = okno;
    for (wxWindow* w = okno; w; w = w->GetParent()) {
        const std::string klucz = KluczKontrolki(w);
        if (klucz.empty()) continue;
        if (const WpisPomocy* wpis = PomocKontekstowa::pobierz().znajdz(klucz)) {
            PokazOpisPola(wxGetTopLevelParent(rodzic), *wpis);
            return;
        }
    }
    PokazPomoc();
}

void WlaczKlawiszF1() {
    static bool wlaczone = false;
    if (wlaczone) return;
    wlaczone = true;
    wxEvtHandler::AddFilter(new FiltrF1);   // filtr działa do końca programu
}

} // namespace PomocGUI
