#include "WysylkaZPL.h"

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/sstream.h>
#include <wx/stdpaths.h>
#include <wx/socket.h>
#include <wx/utils.h>

#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <winspool.h>
#endif

namespace WysylkaZPL {

namespace {

wxString Tekst(const std::string& s) { return wxString::FromUTF8(s.c_str()); }

void ustawBlad(std::string* blad, const wxString& tekst) {
    if (blad) *blad = tekst.utf8_string();
}

}  // namespace

std::string nazwaTrybu(Tryb t) {
    switch (t) {
        case Tryb::System: return "system";
        case Tryb::Plik: return "plik";
        default: return "siec";
    }
}

Tryb trybZNazwy(const std::string& s) {
    if (s == "system") return Tryb::System;
    if (s == "plik") return Tryb::Plik;
    return Tryb::Siec;
}

bool doPliku(const std::string& sciezka, const std::string& dane, std::string* blad) {
    if (sciezka.empty()) {
        ustawBlad(blad, wxString::FromUTF8("Nie podano pliku do zapisu."));
        return false;
    }
    wxLogNull cicho;
    wxFFile f(Tekst(sciezka), "wb");
    if (!f.IsOpened() || f.Write(dane.data(), dane.size()) != dane.size()) {
        ustawBlad(blad, wxString::FromUTF8("Nie można zapisać pliku: ") + Tekst(sciezka));
        return false;
    }
    return true;
}

bool przezSiec(const std::string& host, int port, const std::string& dane, int limitSekund, std::string* blad) {
    if (host.empty()) {
        ustawBlad(blad, wxString::FromUTF8("Nie podano adresu drukarki."));
        return false;
    }
    if (port < 1 || port > 65535) {
        ustawBlad(blad, wxString::FromUTF8("Nieprawidłowy numer portu."));
        return false;
    }
    wxLogNull cicho;
    wxIPV4address adres;
    if (!adres.Hostname(Tekst(host))) {
        ustawBlad(blad, wxString::FromUTF8("Nie można odnaleźć drukarki: ") + Tekst(host));
        return false;
    }
    adres.Service(static_cast<unsigned short>(port));
    wxSocketClient gniazdo(wxSOCKET_BLOCK | wxSOCKET_WAITALL);
    gniazdo.SetTimeout(std::max(1, limitSekund));
    if (!gniazdo.Connect(adres, true)) {
        ustawBlad(blad, wxString::Format(wxString::FromUTF8("Nie można połączyć się z drukarką %s:%d. Sprawdź adres, "
                                                           "kabel sieciowy i port (domyślnie 9100)."),
                                         Tekst(host), port));
        return false;
    }
    size_t wyslano = 0;
    while (wyslano < dane.size()) {
        const size_t ile = std::min<size_t>(4096, dane.size() - wyslano);
        gniazdo.Write(dane.data() + wyslano, ile);
        if (gniazdo.Error() || gniazdo.LastWriteCount() == 0) {
            ustawBlad(blad, wxString::FromUTF8("Połączenie z drukarką zostało przerwane podczas wysyłania."));
            return false;
        }
        wyslano += gniazdo.LastWriteCount();
    }
    gniazdo.Close();
    return true;
}

bool przezSystem(const std::string& drukarka, const std::string& dane, std::string* blad) {
    if (drukarka.empty()) {
        ustawBlad(blad, wxString::FromUTF8("Nie wybrano drukarki."));
        return false;
    }
#ifdef _WIN32
    const std::wstring nazwa = Tekst(drukarka).ToStdWstring();
    HANDLE h = nullptr;
    if (!OpenPrinterW(const_cast<LPWSTR>(nazwa.c_str()), &h, nullptr)) {
        ustawBlad(blad, wxString::FromUTF8("Nie można otworzyć drukarki: ") + Tekst(drukarka));
        return false;
    }
    wchar_t nazwaDok[] = L"Magmass ZPL";
    wchar_t typ[] = L"RAW";
    DOC_INFO_1W di;
    di.pDocName = nazwaDok;
    di.pOutputFile = nullptr;
    di.pDatatype = typ;
    bool ok = false;
    if (StartDocPrinterW(h, 1, reinterpret_cast<LPBYTE>(&di)) != 0) {
        if (StartPagePrinter(h)) {
            DWORD zapisano = 0;
            ok = WritePrinter(h, const_cast<char*>(dane.data()), static_cast<DWORD>(dane.size()), &zapisano) &&
                 zapisano == dane.size();
            EndPagePrinter(h);
        }
        EndDocPrinter(h);
    }
    ClosePrinter(h);
    if (!ok) ustawBlad(blad, wxString::FromUTF8("Wysyłanie do drukarki nie powiodło się: ") + Tekst(drukarka));
    return ok;
#else
    // urządzenie USB widziane jako plik (Linux: /dev/usb/lp0) piszemy wprost
    if (drukarka.rfind("/dev/", 0) == 0) {
        if (!wxFileName::Exists(Tekst(drukarka), wxFILE_EXISTS_ANY)) {  // nie tworzymy zwykłych plików w /dev
            ustawBlad(blad, wxString::FromUTF8("Nie ma urządzenia: ") + Tekst(drukarka) +
                                wxString::FromUTF8(". Sprawdź, czy drukarka jest włączona i podłączona."));
            return false;
        }
        return doPliku(drukarka, dane, blad);
    }
    if (drukarka.find('"') != std::string::npos) {
        ustawBlad(blad, wxString::FromUTF8("Nazwa drukarki zawiera niedozwolony znak."));
        return false;
    }
    // kolejka CUPS: surowe dane bez filtrów
    const wxString tmp = wxFileName::CreateTempFileName("magmass_zpl");
    if (tmp.empty() || !doPliku(tmp.utf8_string(), dane, blad)) return false;
    wxString polecenie = "lp -d \"" + Tekst(drukarka) + "\" -o raw \"" + tmp + "\"";
    wxArrayString wyj, bledy;
    const long kod = wxExecute(polecenie, wyj, bledy, wxEXEC_SYNC | wxEXEC_NODISABLE);
    wxLogNull cicho;
    wxRemoveFile(tmp);
    if (kod != 0) {
        wxString opis = bledy.empty() ? wxString() : bledy[0];
        ustawBlad(blad, wxString::FromUTF8("Drukowanie przez system nie powiodło się (polecenie lp)") +
                            (opis.empty() ? wxString() : ": " + opis));
        return false;
    }
    return true;
#endif
}

bool wyslij(const Ustawienia& u, const std::string& dane, std::string* blad) {
    switch (u.tryb) {
        case Tryb::System: return przezSystem(u.drukarka, dane, blad);
        case Tryb::Plik: return doPliku(u.plik, dane, blad);
        default: return przezSiec(u.host, u.port, dane, u.limitSekund, blad);
    }
}

std::vector<std::string> listaDrukarek() {
    std::vector<std::string> wynik;
#ifdef _WIN32
    DWORD potrzeba = 0, liczba = 0;
    EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 4, nullptr, 0, &potrzeba, &liczba);
    if (potrzeba == 0) return wynik;
    std::vector<BYTE> bufor(potrzeba);
    if (!EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 4, bufor.data(), potrzeba, &potrzeba,
                       &liczba))
        return wynik;
    const PRINTER_INFO_4W* p = reinterpret_cast<const PRINTER_INFO_4W*>(bufor.data());
    for (DWORD i = 0; i < liczba; ++i) wynik.push_back(wxString(p[i].pPrinterName).utf8_string());
#else
    wxLogNull cicho;
    wxArrayString wyj, bledy;
    if (wxExecute("lpstat -e", wyj, bledy, wxEXEC_SYNC | wxEXEC_NODISABLE) == 0) {
        for (const wxString& l : wyj) {
            const wxString n = l.Strip(wxString::both);
            if (!n.empty()) wynik.push_back(n.utf8_string());
        }
    }
#endif
    std::sort(wynik.begin(), wynik.end());
    wynik.erase(std::unique(wynik.begin(), wynik.end()), wynik.end());
    return wynik;
}

}  // namespace WysylkaZPL
