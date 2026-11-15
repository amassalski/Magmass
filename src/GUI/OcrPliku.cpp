#include "OcrPliku.h"

#include "../Logika/ProgramyZewnetrzne.h"
#include "../Narzedzia/Konfiguracja.h"
#include "../Narzedzia/PolecenieKsiazek.h"
#include "../Narzedzia/Tlumaczenia.h"

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/progdlg.h>

#include <functional>
#include <memory>

namespace OcrPliku {

bool Przygotuj(Ustawienia& ust, wxString& blad) {
    ust.program = ProgramyZewnetrzne::Znajdz("ocrmypdf", Konfiguracja::pobierz().pobierz("literatura", "ocrmypdf", ""));
    if (ust.program.empty()) {
        blad = TR("Nie znaleziono programu ocrmypdf. Zainstaluj go (ocrmypdf.readthedocs.io; wymaga Tesseracta z językiem polskim) albo wskaż jego położenie w Ustawieniach (zakładka Inne).");
        return false;
    }
    ust.jezyki = Konfiguracja::pobierz().pobierz("literatura", "ocr_jezyki", "pol+eng");
    if (!PolecenieKsiazek::poprawneJezykiOcr(ust.jezyki)) {
        blad = TR("Nieprawidłowe języki OCR w ustawieniach. Podaj kody Tesseracta, np. pol+eng.");
        return false;
    }
    return true;
}

bool WybierzTryb(wxWindow* rodzic, size_t liczba, Tryb& tryb) {
    wxMessageDialog dlg(rodzic, wxString::Format(TR("OCR plików PDF: %zu. Dla dużych książek trwa to kilka minut lub dłużej, a strony z tekstem są pomijane.\n\n"
                                                    "Tak: wynik zastępuje plik (oryginał zostaje jako plik.pdf.bak) i indeks treści jest budowany od nowa.\n"
                                                    "Nie: wynik jest zapisywany obok jako plik_ocr.pdf."), liczba),
                        TR("OCR"), wxYES_NO | wxCANCEL | wxICON_QUESTION);
    dlg.SetYesNoLabels(TR("Zastąp oryginał"), TR("Zapisz obok"));
    const int odp = dlg.ShowModal();
    if (odp == wxID_CANCEL) return false;
    tryb = odp == wxID_YES ? Tryb::Zastap : Tryb::Obok;
    return true;
}

Wynik Wykonaj(wxWindow* rodzic, const Ustawienia& ust, const std::string& sciezka, const wxString& tytul, size_t nr, size_t razem,
              bool zastap, bool bezPytan, std::string& blad) {
    wxLogNull bezLogu;   // usuwanie nieistniejących plików wynikowych nie ma pokazywać okien błędów
    const std::string wynik = PolecenieKsiazek::sciezkaWynikuOcr(sciezka);
    std::unique_ptr<wxProgressDialog> okno;
    if (!bezPytan)
        okno.reset(new wxProgressDialog(TR("OCR"), wxString::Format(TR("%s (%zu z %zu)\nTo może potrwać kilka minut..."), tytul, nr, razem),
                                        100, rodzic, wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME));
    const auto w = ProgramyZewnetrzne::Uruchom(ust.program, PolecenieKsiazek::argumentyOcr(sciezka, wynik, ust.jezyki),
                                               okno ? std::function<bool()>([&okno] { return okno->Pulse(); }) : std::function<bool()>());
    okno.reset();
    if (w.przerwano) { wxRemoveFile(wxString::FromUTF8(wynik)); blad = "przerwano"; return Wynik::Przerwano; }
    if (!w.ok() || !wxFileName::FileExists(wxString::FromUTF8(wynik))) {
        blad = w.uruchomiono ? w.wyjscie.substr(0, 300) : std::string("nie można uruchomić programu");
        wxRemoveFile(wxString::FromUTF8(wynik));
        return Wynik::Blad;
    }
    if (zastap) {
        const wxString orig = wxString::FromUTF8(sciezka), kopia = orig + ".bak", nowy = wxString::FromUTF8(wynik);
        if (!wxRenameFile(orig, kopia, true)) { blad = "nie można utworzyć kopii .bak"; wxRemoveFile(nowy); return Wynik::Blad; }
        if (!wxRenameFile(nowy, orig, true)) { wxRenameFile(kopia, orig, true); blad = "nie można podmienić pliku"; return Wynik::Blad; }
    }
    return Wynik::Ok;
}

} // namespace OcrPliku
