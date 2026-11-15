#ifndef TLUMACZENIA_H
#define TLUMACZENIA_H

#include <wx/intl.h>
#include <wx/string.h>

#include <string>
#include <unordered_map>

// Teksty w kodzie źródłowym są zapisane w UTF-8. Zwykłe _("...") dekoduje
// literał wg strony kodowej systemu (CP1250), co daje krzaczki. TR() zawsze
// traktuje literał jako UTF-8, a dopiero potem szuka tłumaczenia w katalogu.
// Tłumaczenie zapamiętuje też oryginał (polski), bo klucze opisów pól (PomocGUI::Dodaj) powstają
// z oryginalnej etykiety, a nie z tekstu przetłumaczonego - dzięki temu dymki działają w każdym języku.
inline std::unordered_map<std::string, std::string>& OryginalyTlumaczen() {
    static std::unordered_map<std::string, std::string> m;
    return m;
}
inline wxString TlumaczIZapamietaj(const char* utf8) {
    const wxString oryg = wxString::FromUTF8(utf8);
    const wxString tlum = wxGetTranslation(oryg);
    if (tlum != oryg) OryginalyTlumaczen()[tlum.utf8_string()] = oryg.utf8_string();
    return tlum;
}
#define TR(s) TlumaczIZapamietaj(s)

// Tekst z warstwy bazy / konfiguracji (std::string w UTF-8) -> wxString.
// Gdy tekst nie jest poprawnym UTF-8 (np. komunikat libpq w stronie kodowej
// systemu), FromUTF8 zwróciłby pusty napis, więc wracamy do kodowania lokalnego.
inline wxString TekstZUTF8(const std::string& tekst) {
    wxString wynik = wxString::FromUTF8(tekst.c_str(), tekst.size());
    if (wynik.empty() && !tekst.empty()) {
        wynik = wxString(tekst.c_str(), wxConvLibc);
    }
    return wynik;
}

#endif // TLUMACZENIA_H
