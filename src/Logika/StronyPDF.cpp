#include "StronyPDF.h"
#include "ProgramyZewnetrzne.h"

#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/string.h>
#include <wx/utils.h>

#include <algorithm>

namespace StronyPDF {

std::vector<std::string> pobierz(const std::string& sciezka, std::string& blad) {
    blad.clear();
    std::vector<std::string> strony;
    if (!wxFileName::FileExists(wxString::FromUTF8(sciezka))) { blad = "Nie ma takiego pliku."; return strony; }
    ProgramyZewnetrzne::DopiszTypoweKatalogiDoPATH();   // macOS: aplikacja z Findera nie ma /opt/local/bin ani /opt/homebrew/bin w PATH
    wxArrayString wyj, bledy;
    // bez -layout: kolejność wyrazów w zwykłym tekście jest lepsza do szukania; strony rozdziela znak \f
    wxArrayString argv;
    const wxString polecenie = "pdftotext -enc UTF-8 \"" + wxString::FromUTF8(sciezka) + "\" -";
    const long kod = wxExecute(polecenie, wyj, bledy, wxEXEC_SYNC | wxEXEC_NODISABLE);
    if (kod == -1) { blad = "Nie można uruchomić programu pdftotext (pakiet poppler-utils / poppler; macOS: brew install poppler albo sudo port install poppler)."; return strony; }
    if (kod != 0) { blad = "pdftotext zgłosił błąd - plik może być uszkodzony lub zaszyfrowany."; return strony; }
    std::string biezaca;
    bool cokolwiek = false;
    for (const wxString& l : wyj) {
        const std::string linia = l.utf8_string();
        size_t start = 0;
        while (true) {
            const size_t ff = linia.find('\f', start);
            if (ff == std::string::npos) { biezaca += linia.substr(start); biezaca += '\n'; break; }
            biezaca += linia.substr(start, ff - start);
            strony.push_back(std::move(biezaca));
            biezaca.clear();
            start = ff + 1;
        }
    }
    if (biezaca.find_first_not_of(" \n\t\r") != std::string::npos) strony.push_back(std::move(biezaca));
    for (const std::string& s : strony) if (s.find_first_not_of(" \n\t\r\f") != std::string::npos) { cokolwiek = true; break; }
    if (!cokolwiek) { strony.clear(); blad = "W pliku nie ma warstwy tekstowej (to chyba skan)."; }
    return strony;
}

std::vector<std::string> znajdzPlikiKsiazek(const std::string& katalog) {
    std::vector<std::string> wynik;
    wxArrayString pliki;
    wxDir::GetAllFiles(wxString::FromUTF8(katalog), &pliki, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
    for (const wxString& p : pliki) {
        wxString rozsz = wxFileName(p).GetExt().Lower();
        if (rozsz == "pdf" || rozsz == "epub") wynik.push_back(p.utf8_string());
    }
    std::sort(wynik.begin(), wynik.end());
    return wynik;
}

std::vector<std::string> argumentyOtwarciaNaStronie(const std::string& program, const std::string& sciezka, int strona) {
    const std::string s = std::to_string(strona);
    if (program == "evince" || program == "atril") return {program, "-p", s, sciezka};
    if (program == "okular") return {program, "-p", s, sciezka};
    if (program == "zathura") return {program, "-P", s, sciezka};
    if (program == "xpdf") return {program, sciezka, s};
    if (program == "mupdf") return {program, sciezka, s};
    if (program == "firefox" || program == "chromium" || program == "google-chrome")
        return {program, "file://" + sciezka + "#page=" + s};
    return {};
}

} // namespace StronyPDF
