#ifndef ZESTAWIENIEDRUTU_H
#define ZESTAWIENIEDRUTU_H

#include <wx/wx.h>

#include "../Logika/MasaDrutu.h"

/// Zestawienie drutu nawojowego (długość i masa każdej średnicy) w dialogach transformatorów i dławika.
namespace ZestawienieDrutu {

/// Tekst tabeli do pola tylko do odczytu; dla pustego zestawienia pusty napis.
wxString Tekst(const MasaDrutu::Zestawienie& z);

/// Pole tylko do odczytu w ramce "Zestawienie drutu (waga)", dodane do sizera. Zwraca pole tekstowe.
wxTextCtrl* Dodaj(wxWindow* rodzic, wxSizer* sizer, const char* kluczPomocy, int margines = 8);   ///< margines: odstęp ramki od krawędzi sizera (0 = na pełną szerokość)

}  // namespace ZestawienieDrutu

#endif  // ZESTAWIENIEDRUTU_H
