#ifndef ZDJECIAELEMENTOW_H
#define ZDJECIAELEMENTOW_H

#include "../Narzedzia/Kamera.h"

#include <wx/image.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/window.h>

#include <functional>

/// Zapis i odczyt zdjęć elementów w katalogu zdjęć z ustawień aplikacji
namespace ZdjeciaElementow {

wxImage ZKlatki(const KlatkaRGB& klatka);
/// Katalog zdjęć z ustawień (utworzony, jeśli go nie ma); pusty tekst, gdy nie da się go utworzyć
wxString Katalog();
/// Zapisuje klatkę jako JPEG "<symbol>_RRRRMMDD_GGMMSS.jpg"; zwraca pełną ścieżkę albo pusty tekst
wxString ZapiszKlatke(const KlatkaRGB& klatka, const wxString& symbol);
/// Kopiuje plik do katalogu zdjęć (pod unikalną nazwą); gdy się nie da, zwraca ścieżkę oryginału
wxString SkopiujDoKatalogu(const wxString& sciezka);
/// Ładuje zdjęcie i skaluje do pola o podanym rozmiarze z zachowaniem proporcji (nie powiększa); pusty obraz przy błędzie
wxImage Wczytaj(const wxString& sciezka, wxSize pole);

/// Okno kamery: zwraca ścieżkę zrobionego zdjęcia albo pusty tekst, gdy anulowano. @p nazwaDlaPliku trafia do nazwy pliku.
wxString ZKamery(wxWindow* rodzic, const wxString& nazwaDlaPliku);
/// Okno wyboru pliku obrazu; wybrany plik jest kopiowany do katalogu zdjęć. Pusty tekst, gdy anulowano.
wxString ZPliku(wxWindow* rodzic);

/**
 * Wiersz formularza ze zdjęciem: pole tekstowe @p pole (musi mieć rodzica @p rodzic) i przyciski "Wybierz..." oraz "Z kamery...".
 * @p nazwaDlaKamery daje tekst użyty w nazwie pliku zdjęcia z kamery (np. symbol). Zwraca sizer do wstawienia w formularz.
 */
wxBoxSizer* WierszWyboru(wxWindow* rodzic, wxTextCtrl* pole, const std::function<wxString()>& nazwaDlaKamery);

} // namespace ZdjeciaElementow

#endif // ZDJECIAELEMENTOW_H
