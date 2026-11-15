#include "ProgramyZewnetrzne.h"
#include "../Import/MetadaneKsiazek.h"

#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/log.h>
#include <wx/sstream.h>
#include <wx/utils.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <memory>

namespace MetadaneKsiazek {
namespace {

/// Początek i koniec pliku (XMP bywa na początku albo na końcu); większe pliki nie są czytane w całości
std::string czytajKrance(const wxString& sciezka) {
    wxLogNull cisza;
    wxFFile f(sciezka, "rb");
    if (!f.IsOpened()) return {};
    const wxFileOffset dl = f.Length();
    const size_t blok = 3u * 1024 * 1024;
    std::string w;
    if (dl <= static_cast<wxFileOffset>(2 * blok)) {
        w.resize(static_cast<size_t>(dl));
        if (f.Read(&w[0], w.size()) != w.size()) w.clear();
        return w;
    }
    w.resize(blok);
    if (f.Read(&w[0], blok) != blok) return {};
    std::string koniec(blok, '\0');
    f.Seek(dl - static_cast<wxFileOffset>(blok));
    if (f.Read(&koniec[0], blok) != blok) return w;
    return w + koniec;
}

std::string wpisZip(const wxString& sciezka, const wxString& nazwa) {
    wxLogNull cisza;
    wxFFileInputStream plik(sciezka);
    if (!plik.IsOk()) return {};
    wxZipInputStream zip(plik);
    std::unique_ptr<wxZipEntry> wpis;
    while (wpis.reset(zip.GetNextEntry()), wpis) {
        if (wpis->GetName(wxPATH_UNIX) != nazwa) continue;
        std::string w;
        char bufor[4096];
        while (!zip.Eof()) {
            zip.Read(bufor, sizeof bufor);
            const size_t n = zip.LastRead();
            if (n == 0) break;
            w.append(bufor, n);
            if (w.size() > 4u * 1024 * 1024) break;
        }
        return w;
    }
    return {};
}

/// Wartość atrybutu full-path pierwszego <rootfile> z META-INF/container.xml
std::string sciezkaOPF(const std::string& container) {
    const size_t r = container.find("rootfile");
    if (r == std::string::npos) return {};
    const size_t a = container.find("full-path", r);
    if (a == std::string::npos) return {};
    const size_t o = container.find_first_of("\"'", a);
    if (o == std::string::npos) return {};
    const size_t k = container.find(container[o], o + 1);
    return k == std::string::npos ? std::string() : container.substr(o + 1, k - o - 1);
}

}  // namespace

Dane odczytaj(const std::string& sciezka) {
    const wxString plik = wxString::FromUTF8(sciezka);
    if (!wxFileName::FileExists(plik)) return {};
    const wxString rozsz = wxFileName(plik).GetExt().Lower();
    if (rozsz == "epub") {
        const std::string opf = sciezkaOPF(wpisZip(plik, "META-INF/container.xml"));
        if (opf.empty()) return {};
        return zOPF(wpisZip(plik, wxString::FromUTF8(opf)));
    }
    if (rozsz != "pdf") return {};
    Dane d = zXMP(wytnijXMP(czytajKrance(plik)));
    // słownik Info (Title, Author, Keywords...) przez pdfinfo; brak programu nie jest błędem
    ProgramyZewnetrzne::DopiszTypoweKatalogiDoPATH();   // macOS: aplikacja z Findera nie ma /opt/local/bin ani /opt/homebrew/bin w PATH
    wxArrayString wyj, bledy;
    if (wxExecute("pdfinfo -enc UTF-8 \"" + plik + "\"", wyj, bledy, wxEXEC_SYNC | wxEXEC_NODISABLE) == 0) {
        std::string tekst;
        for (const wxString& l : wyj) { tekst += l.utf8_string(); tekst += '\n'; }
        d = polacz(d, zInfoPDF(tekst));
    }
    return d;
}

} // namespace MetadaneKsiazek
