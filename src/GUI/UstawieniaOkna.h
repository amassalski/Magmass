#ifndef USTAWIENIAOKNA_H
#define USTAWIENIAOKNA_H

#include <wx/string.h>

class wxListCtrl;
class wxFrame;
class wxSplitterWindow;

/**
 * Ustawienia wyglądu okien zapamiętywane osobno dla każdego użytkownika systemu
 * (rejestr HKCU w Windows, plik w katalogu domowym w macOS/Linux) - nie we wspólnym config.ini.
 * Zmienna środowiskowa DM_BEZ_USTAWIEN=1 wyłącza odczyt i zapis (testy).
 */
namespace UstawieniaOkna {

bool Wlaczone();

void ZapiszKolumny(wxListCtrl* lista, const wxString& klucz);
void WczytajKolumny(wxListCtrl* lista, const wxString& klucz);

void ZapiszPodzial(wxSplitterWindow* podzial, const wxString& klucz);
void WczytajPodzial(wxSplitterWindow* podzial, const wxString& klucz);

void ZapiszRamke(wxFrame* ramka, const wxString& klucz);
void WczytajRamke(wxFrame* ramka, const wxString& klucz);

int WczytajInt(const wxString& klucz, int domyslna);
void ZapiszInt(const wxString& klucz, int wartosc);

}  // namespace UstawieniaOkna

#endif
