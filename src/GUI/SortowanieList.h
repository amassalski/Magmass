#pragma once
// Sortowanie list po kliknięciu nagłówka kolumny: stan, porównanie tekstów
// (bez rozróżniania wielkości liter, z polskimi znakami i liczbami „naturalnie”:
// R2 < R10) oraz oznaczenie kolumny strzałką w nagłówku.
#include <wx/listctrl.h>
#include <wx/string.h>

#include <algorithm>
#include <string>
#include <vector>

namespace SortowanieList {

struct Stan {
    int kolumna = -1;
    bool rosnaco = true;
    // Kliknięcie tej samej kolumny odwraca kierunek, inna kolumna sortuje rosnąco.
    void Klik(int k) {
        if (k == kolumna) {
            rosnaco = !rosnaco;
        } else {
            kolumna = k;
            rosnaco = true;
        }
    }
};

// Klucz do porównań: małe litery, polskie litery tuż za podstawowymi.
inline std::wstring Klucz(const std::string& utf8) {
    const wxString w = wxString::FromUTF8(utf8.c_str()).Lower();
    std::wstring wynik;
    for (wxString::const_iterator it = w.begin(); it != w.end(); ++it) {
        const wchar_t c = static_cast<wchar_t>(static_cast<wxUniChar>(*it).GetValue());
        switch (c) {
            case 0x0105: wynik += L"a\x01"; break;  // ą
            case 0x0107: wynik += L"c\x01"; break;  // ć
            case 0x0119: wynik += L"e\x01"; break;  // ę
            case 0x0142: wynik += L"l\x01"; break;  // ł
            case 0x0144: wynik += L"n\x01"; break;  // ń
            case 0x00F3: wynik += L"o\x01"; break;  // ó
            case 0x015B: wynik += L"s\x01"; break;  // ś
            case 0x017A: wynik += L"z\x01"; break;  // ź
            case 0x017C: wynik += L"z\x02"; break;  // ż
            default: wynik += c;
        }
    }
    return wynik;
}

// <0, 0, >0 – jak strcmp; ciągi cyfr porównywane jako liczby.
inline int PorownajTekst(const std::string& a, const std::string& b) {
    const std::wstring x = Klucz(a), y = Klucz(b);
    size_t i = 0, j = 0;
    auto cyfra = [](wchar_t c) { return c >= L'0' && c <= L'9'; };
    while (i < x.size() && j < y.size()) {
        if (cyfra(x[i]) && cyfra(y[j])) {
            size_t i2 = i, j2 = j;
            while (i2 < x.size() && x[i2] == L'0') ++i2;
            while (j2 < y.size() && y[j2] == L'0') ++j2;
            size_t ie = i2, je = j2;
            while (ie < x.size() && cyfra(x[ie])) ++ie;
            while (je < y.size() && cyfra(y[je])) ++je;
            if (ie - i2 != je - j2) return (ie - i2 < je - j2) ? -1 : 1;
            const int c = x.compare(i2, ie - i2, y, j2, je - j2);
            if (c != 0) return c < 0 ? -1 : 1;
            i = ie;
            j = je;
        } else {
            if (x[i] != y[j]) return x[i] < y[j] ? -1 : 1;
            ++i;
            ++j;
        }
    }
    if (i < x.size()) return 1;
    if (j < y.size()) return -1;
    return 0;
}

// Sortuje wektor stabilnie według porównania „mniej(a, b, kolumna)”.
template <class T, class Mniej>
void Posortuj(std::vector<T>& v, const Stan& stan, Mniej mniej) {
    if (stan.kolumna < 0) return;
    const int k = stan.kolumna;
    const bool ros = stan.rosnaco;
    std::stable_sort(v.begin(), v.end(), [&](const T& a, const T& b) { return ros ? mniej(a, b, k) : mniej(b, a, k); });
}

// Strzałka w nagłówku posortowanej kolumny (usuwa ją z pozostałych).
inline void OznaczNaglowek(wxListCtrl* lista, const Stan& stan) {
    if (!lista) return;
    const wxString gora = wxString::FromUTF8(" \xE2\x96\xB2"), dol = wxString::FromUTF8(" \xE2\x96\xBC");
    for (int i = 0; i < lista->GetColumnCount(); ++i) {
        wxListItem it;
        it.SetMask(wxLIST_MASK_TEXT);
        if (!lista->GetColumn(i, it)) continue;
        wxString t = it.GetText();
        if (t.EndsWith(gora)) t.RemoveLast(gora.length());
        else if (t.EndsWith(dol)) t.RemoveLast(dol.length());
        if (i == stan.kolumna) t += stan.rosnaco ? gora : dol;
        if (t != it.GetText()) {
            it.SetText(t);
            lista->SetColumn(i, it);
        }
    }
}

}  // namespace SortowanieList
