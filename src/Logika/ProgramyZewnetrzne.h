#ifndef PROGRAMYZEWNETRZNE_H
#define PROGRAMYZEWNETRZNE_H

#include <wx/string.h>

#include <functional>
#include <string>
#include <vector>

/// Uruchamianie programów zewnętrznych (ExifTool, ocrmypdf) bez powłoki: argumenty przekazywane osobno,
/// więc spacje i polskie znaki w ścieżkach nie wymagają cudzysłowów.
namespace ProgramyZewnetrzne {

struct Wynik {
    bool uruchomiono = false;   ///< false: nie udało się wystartować programu
    int kod = -1;               ///< kod wyjścia
    bool przerwano = false;     ///< przerwane przez użytkownika
    std::string wyjscie;        ///< stdout i stderr (ucięte do kilku KB)
    bool ok() const { return uruchomiono && !przerwano && kod == 0; }
};

/// Pełna ścieżka programu: @p zUstawien (ścieżka albo nazwa) albo @p nazwa z PATH (także z ".exe"); pusta, gdy nie znaleziono
/// Dopisuje do PATH tego procesu typowe katalogi programów (macOS: Homebrew /opt/homebrew/bin i /usr/local/bin, MacPorts /opt/local/bin;
/// Linux: /usr/local/bin). Aplikacja uruchomiona z Findera ma tylko minimalny PATH, więc bez tego nie widzi exiftool, ocrmypdf ani Tesseracta.
/// Wywołanie wielokrotne jest bezpieczne.
void DopiszTypoweKatalogiDoPATH();

wxString Znajdz(const std::string& nazwa, const std::string& zUstawien = "");

/// Uruchamia program i czeka na zakończenie. W trakcie czekania wywołuje @p postep (np. Pulse okna postępu);
/// gdy zwróci false, proces jest przerywany.
Wynik Uruchom(const wxString& program, const std::vector<std::string>& argumenty, const std::function<bool()>& postep = nullptr);

} // namespace ProgramyZewnetrzne

#endif // PROGRAMYZEWNETRZNE_H
