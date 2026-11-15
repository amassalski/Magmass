#include "StronyKsiazki.h"
#include "StronyPDF.h"
#include "../Import/TekstEPUB.h"

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <map>
#include <memory>

namespace StronyKsiazki {
namespace {

std::string rozszerzenie(const std::string& sciezka) {
    return wxFileName(wxString::FromUTF8(sciezka)).GetExt().Lower().utf8_string();
}

/// Wczytuje z archiwum EPUB wpisy tekstowe (opf, xhtml, html, xml) do mapy nazwa -> zawartość
std::map<std::string, std::string> wczytajWpisy(const wxString& plik) {
    std::map<std::string, std::string> wynik;
    wxLogNull cisza;
    wxFFileInputStream we(plik);
    if (!we.IsOk()) return wynik;
    wxZipInputStream zip(we);
    std::unique_ptr<wxZipEntry> wpis;
    size_t razem = 0;
    while (wpis.reset(zip.GetNextEntry()), wpis) {
        if (wpis->IsDir()) continue;
        const std::string nazwa = wpis->GetName(wxPATH_UNIX).utf8_string();
        const std::string rozsz = rozszerzenie(nazwa);
        if (rozsz != "xhtml" && rozsz != "html" && rozsz != "htm" && rozsz != "xml" && rozsz != "opf") continue;
        std::string tresc;
        char bufor[8192];
        while (!zip.Eof()) {
            zip.Read(bufor, sizeof bufor);
            const size_t n = zip.LastRead();
            if (n == 0) break;
            tresc.append(bufor, n);
            if (tresc.size() > 20u * 1024 * 1024) break;
        }
        razem += tresc.size();
        wynik[nazwa] = std::move(tresc);
        if (razem > 300u * 1024 * 1024) break;
    }
    return wynik;
}

}  // namespace

bool czyEpub(const std::string& sciezka) { return rozszerzenie(sciezka) == "epub"; }
bool czyMoznaIndeksowac(const std::string& sciezka) { const std::string r = rozszerzenie(sciezka); return r == "pdf" || r == "epub"; }

std::vector<std::string> pobierz(const std::string& sciezka, std::string& blad) {
    blad.clear();
    if (!czyEpub(sciezka)) return StronyPDF::pobierz(sciezka, blad);
    const wxString plik = wxString::FromUTF8(sciezka);
    if (!wxFileName::FileExists(plik)) { blad = "Nie znaleziono pliku: " + sciezka; return {}; }
    const auto wpisy = wczytajWpisy(plik);
    auto kontener = wpisy.find("META-INF/container.xml");
    std::string opfSciezka;
    if (kontener != wpisy.end()) {
        const std::string& c = kontener->second;
        const size_t a = c.find("full-path");
        const size_t o = a == std::string::npos ? a : c.find_first_of("\"'", a);
        if (o != std::string::npos) { const size_t k = c.find(c[o], o + 1); if (k != std::string::npos) opfSciezka = c.substr(o + 1, k - o - 1); }
    }
    auto opf = wpisy.find(opfSciezka);
    if (opfSciezka.empty() || opf == wpisy.end()) { blad = "To nie jest poprawny plik EPUB (brak pliku OPF)."; return {}; }
    const size_t uk = opfSciezka.rfind('/');
    const auto rozdzialy = TekstEPUB::kolejnoscRozdzialow(opf->second, uk == std::string::npos ? std::string() : opfSciezka.substr(0, uk));
    std::vector<std::string> wynik;
    for (const std::string& r : rozdzialy) {
        auto it = wpisy.find(r);
        if (it == wpisy.end()) continue;
        for (std::string& kawalek : TekstEPUB::podzielTekst(TekstEPUB::tekstZHTML(it->second))) wynik.push_back(std::move(kawalek));
    }
    if (wynik.empty()) blad = "W pliku EPUB nie znaleziono tekstu.";
    return wynik;
}

} // namespace StronyKsiazki
