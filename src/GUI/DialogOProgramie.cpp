#include "DialogOProgramie.h"

#include "Ikony.h"
#include "../Narzedzia/Tlumaczenia.h"

#include <libpq-fe.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

namespace {

wxString Esc(const wxString& t) {
    wxString w = t;
    w.Replace("&", "&amp;"); w.Replace("<", "&lt;"); w.Replace(">", "&gt;");
    return w;
}

wxString Odnosnik(const char* adres) {
    return wxString::Format("<a href=\"%s\">%s</a>", adres, adres);
}

}  // namespace

wxString DialogOProgramie::TrescOProgramie() {
    wxString h = "<html><body>";
    h += "<h3>Magmass " + wxString::FromUTF8(DM_VERSION) + "</h3>";
    h += "<p>" + Esc(TR("Domowy magazyn części elektronicznych: elementy i stany magazynowe, projekty i listy części, "
                        "katalog z parametrami i datasheetami, literatura z indeksem treści oraz narzędzia i pożyczki.")) + "</p>";
    h += "<p><b>" + Esc(TR("Możliwości")) + "</b></p><ul>";
    for (const wxString& m : {
             TR("magazyn z kategoriami, miejscami (kontener, szuflada, przegroda), stanami minimalnymi i historią zmian"),
             TR("hurtowe dodawanie: szeregi E12, E24, E48, E96 i import z CSV"),
             TR("projekty: import list części z KiCada i Eagle'a, braki, lista zakupów, wydawanie części"),
             TR("katalog elementów z parametrami, pobieraniem datasheetów i odczytem danych z PDF"),
             TR("sklepy i ceny, kody kreskowe i etykiety, raporty"),
             TR("literatura: import z katalogu, dane z Biblioteki Narodowej, indeks i szukanie w treści, OCR, zapis danych do PDF"),
             TR("narzędzia i pożyczki, kalkulatory (transformatory, głośniki, punkty pracy tranzystorów i lamp)"),
             TR("współpraca z Eagle CAD 7.6: eksport list części i import bibliotek")})
        h += "<li>" + Esc(m) + "</li>";
    h += "</ul>";
    h += "<p><b>" + Esc(TR("Autor")) + ":</b> " + Esc(wxString::FromUTF8(Autor())) + "<br>";
    h += "<b>" + Esc(TR("Repozytorium")) + ":</b> " + Odnosnik(Repozytorium()) + "</p>";
    h += "<p><b>" + Esc(TR("Licencja")) + ":</b> " + Esc(TR("GNU General Public License, wersja 3 lub nowsza.")) + " " +
         Esc(TR("Program jest wolnym oprogramowaniem: możesz go rozpowszechniać i zmieniać na warunkach tej licencji. "
                "Program jest udostępniany bez żadnej gwarancji. Pełny tekst licencji jest na zakładce Licencja.")) + "</p>";
    h += "<p>" + Esc(wxString::FromUTF8("© 2026 ") + wxString::FromUTF8(Autor())) + "</p>";
    h += "<p><font size=-1>" + Esc(wxString::Format(TR("Biblioteki: %s, PostgreSQL libpq %d.%d"), wxVERSION_STRING,
                                                    PQlibVersion() / 10000, (PQlibVersion() / 100) % 100)) + "</font></p>";
    return h + "</body></html>";
}

wxString DialogOProgramie::TrescPodziekowan() {
    wxString h = "<html><body><p>" + Esc(TR("Magmass korzysta z pracy wielu projektów. Dziękuję ich autorom:")) + "</p><ul>";
    auto poz = [&](const wxString& nazwa, const wxString& opis) { h += "<li><b>" + Esc(nazwa) + "</b>: " + Esc(opis) + "</li>"; };
    poz("Claude (Anthropic)", TR("asystent AI, współtwórca programu: pisał i testował kod, dokumentację i narzędzia pod kierunkiem autora"));
    poz("wxWidgets", TR("biblioteka interfejsu graficznego (wxWindows Library Licence)"));
    poz("PostgreSQL / libpq", TR("baza danych i biblioteka klienta (PostgreSQL License)"));
    poz("ExifTool (Phil Harvey)", TR("zapis i odczyt metadanych PDF; zewnętrzny program, nie jest częścią Magmass (Artistic License / GPL)"));
    poz("OCRmyPDF (James R. Barlow)", TR("rozpoznawanie tekstu w skanach PDF; zewnętrzny program (MPL-2.0), korzysta z Tesseract OCR (Apache-2.0) i Ghostscript"));
    poz("Poppler (pdftotext)", TR("odczyt tekstu z PDF do indeksu treści (GPL); dołączany do wersji instalacyjnych"));
    poz("OpenCV", TR("opcjonalny odczyt kodów kreskowych z kamery (Apache-2.0)"));
    poz(TR("Biblioteka Narodowa"), TR("dane bibliograficzne pobierane z publicznego API data.bn.org.pl"));
    poz(TR("Ikony"), TR("własny, płaski zestaw ikon programu, narysowany skryptem narzedzia/zbuduj_ikony.py i udostępniany na tej samej licencji GPL"));
    h += "</ul><p>" + Esc(TR("Nazwy programów i znaki towarowe należą do ich właścicieli.")) + "</p>";
    return h + "</body></html>";
}

wxString DialogOProgramie::TekstLicencji() {
    const wxString exe = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    const wxString sep = wxFileName::GetPathSeparator();
    const wxString kandydaci[] = {
        exe + sep + "LICENSE", exe + sep + ".." + sep + "Resources" + sep + "LICENSE",
        wxStandardPaths::Get().GetResourcesDir() + sep + "LICENSE", exe + sep + ".." + sep + "LICENSE",
        wxString("LICENSE"), wxString("/usr/share/doc/magmass/LICENSE"), wxString("/usr/share/common-licenses/GPL-3")};
    for (const wxString& k : kandydaci) {
        if (!wxFileName::FileExists(k)) continue;
        wxFFile f(k, "rb");
        wxString tekst;
        if (f.IsOpened() && f.ReadAll(&tekst, wxConvUTF8) && !tekst.empty()) return tekst;
    }
    return TR("Magmass jest wolnym oprogramowaniem na licencji GNU General Public License, wersja 3 lub nowsza. "
              "Nie znaleziono pliku LICENSE z pełnym tekstem; jest on dostępny pod adresem:") + "\n" + wxString::FromUTF8(AdresLicencji());
}

DialogOProgramie::DialogOProgramie(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("O programie Magmass"), wxDefaultPosition, wxSize(640, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);
    const wxIcon ikona = Ikony::IkonaProgramu().GetIcon(wxSize(64, 64), wxIconBundle::FALLBACK_NEAREST_LARGER);
    if (ikona.IsOk()) gora->Add(new wxStaticBitmap(this, wxID_ANY, ikona), 0, wxALL, 10);
    wxStaticText* nazwa = new wxStaticText(this, wxID_ANY, "Magmass");
    wxFont f = nazwa->GetFont();
    f.SetPointSize(f.GetPointSize() + 8);
    f.SetWeight(wxFONTWEIGHT_BOLD);
    nazwa->SetFont(f);
    gora->Add(nazwa, 1, wxALIGN_CENTER_VERTICAL | wxALL, 10);
    g->Add(gora, 0, wxEXPAND);

    karty_ = new wxNotebook(this, wxID_ANY);
    auto strona = [&](const wxString& html) {
        wxHtmlWindow* w = new wxHtmlWindow(karty_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
        w->SetPage(html);
        w->Bind(wxEVT_HTML_LINK_CLICKED, [](wxHtmlLinkEvent& e) { wxLaunchDefaultBrowser(e.GetLinkInfo().GetHref()); });
        return w;
    };
    karty_->AddPage(strona(TrescOProgramie()), TR("O programie"), true);
    karty_->AddPage(strona(TrescPodziekowan()), TR("Podziękowania"));
    wxTextCtrl* lic = new wxTextCtrl(karty_, wxID_ANY, TekstLicencji(), wxDefaultPosition, wxDefaultSize,
                                     wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
    lic->SetFont(wxFont(wxFontInfo(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize()).Family(wxFONTFAMILY_TELETYPE)));
    karty_->AddPage(lic, TR("Licencja"));
    g->Add(karty_, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
    g->Add(CreateStdDialogButtonSizer(wxCLOSE), 0, wxEXPAND | wxALL, 10);
    SetSizer(g);
    SetAffirmativeId(wxID_CLOSE);
    SetEscapeId(wxID_CLOSE);
    CentreOnParent();
}
