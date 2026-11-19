#ifndef POLEDATY_H
#define POLEDATY_H

#include <wx/datectrl.h>
#ifdef __WXMSW__
#include <wx/generic/datectrl.h>
#endif

/// Pole daty. Na Windows natywne pole daty nie przyjmuje ciemnego motywu (zostaje białe), więc tam używamy wersji
/// rysowanej przez wxWidgets (kolory z motywu); na pozostałych systemach pole natywne.
#ifdef __WXMSW__
using PoleDatyTyp = wxDatePickerCtrlGeneric;
#else
using PoleDatyTyp = wxDatePickerCtrl;
#endif

inline PoleDatyTyp* NowePoleDaty(wxWindow* rodzic, const wxDateTime& data, long styl) {
#ifdef __WXMSW__
    return new wxDatePickerCtrlGeneric(rodzic, wxID_ANY, data, wxDefaultPosition, wxDefaultSize, styl);
#else
    return new wxDatePickerCtrl(rodzic, wxID_ANY, data, wxDefaultPosition, wxDefaultSize, styl);
#endif
}

#endif
