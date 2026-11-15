#ifndef KOLORYMOTYWU_H
#define KOLORYMOTYWU_H

#include <wx/settings.h>
#include <wx/colour.h>

/// Kolory tekstu wyników i komunikatów zależne od motywu systemu: w jasnym ciemne odcienie (czytelne na białym tle),
/// w ciemnym jasne (czytelne na ciemnym). Kolor liczony z jasności tła okien, więc działa też bez wxAppearance.
namespace Kolory {

inline bool Ciemny() {
    const wxColour t = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    return (299 * t.Red() + 587 * t.Green() + 114 * t.Blue()) / 1000 < 128;
}
inline wxColour Wynik() { return Ciemny() ? wxColour(120, 185, 255) : wxColour(0, 0, 200); }          ///< wartości obliczone
inline wxColour Ostrzezenie() { return Ciemny() ? wxColour(255, 175, 80) : wxColour(150, 70, 0); }    ///< komunikaty ostrzegawcze
inline wxColour Szary() { return Ciemny() ? wxColour(175, 175, 175) : wxColour(110, 110, 110); }      ///< wartości tylko porównawcze
inline wxColour Zielony() { return Ciemny() ? wxColour(115, 215, 125) : wxColour(0, 110, 0); }        ///< wartości z metody porównawczej / sukces
inline wxColour Blad() { return Ciemny() ? wxColour(255, 115, 115) : wxColour(190, 30, 30); }         ///< błędy

}  // namespace Kolory

#endif
