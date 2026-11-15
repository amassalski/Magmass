#ifndef WYMIARYRDZENIA_H
#define WYMIARYRDZENIA_H

#include <wx/wx.h>
#include "../Logika/Dlawik.h"
#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

/// Poprawka k (efekty brzegowe): puste pole albo „auto” = liczona automatycznie ze współczynnika Rogowskiego (wartość 0), liczba > 0 = ręczna
inline bool OdczytajK(const wxString& t, double& k) {
    const wxString s = t.Strip(wxString::both).Lower();
    if (s.empty() || s == "auto") { k = 0; return true; }
    return tekstNaLiczbe(t.utf8_string(), k) && k > 0;
}

/// Pola wymiarów blachy EI (A, B, a, c, d, h w mm) w formularzach Transformator SE i PP: wybór kształtki je wypełnia,
/// ale można je edytować (nietypowa blaszka). Z wymiarów i grubości pakietu b liczone są pola pochodne
/// (przekrój, droga strumienia, długość zwoju, wysokość i szerokość okna) oraz rysunek rdzenia.
struct WymiaryRdzenia {
    wxTextCtrl* t[6] = {};   ///< A, B, a, c, d, h

    /// Wielkości pochodne z wymiarów [mm] i grubości pakietu b [mm]: S [cm²], droga Fe [cm], długość zwoju Cu [cm], h [cm], c [cm]
    struct Pochodne { double sCm2, feCm, cuCm, hCm, cCm; };
    static Pochodne Licz(const Dlawik::Rdzen& r) {
        const double pi = 3.14159265358979323846;
        return {r.a * r.b / 100.0, 2.0 * (r.h + (r.a + r.d) / 2.0 + r.c) / 10.0, (2.0 * (r.a + r.b) + 0.8 * pi * r.c) / 10.0, r.h / 10.0, r.c / 10.0};
    }
    /// Dodaje siatkę 3 wierszy po dwa pola (A B / a c / d h) do @p sizer; @p rodzic = okno ramki
    void Zbuduj(wxWindow* rodzic, wxSizer* sizer, const char* kluczPomocy) {
        const char* et[6] = {"A [mm] =", "B [mm] =", "a [mm] =", "c [mm] =", "d [mm] =", "h [mm] ="};
        wxFlexGridSizer* g = new wxFlexGridSizer(0, 4, 4, 6);
        for (int k = 0; k < 6; k++) {
            g->Add(new wxStaticText(rodzic, wxID_ANY, et[k]), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
            t[k] = new wxTextCtrl(rodzic, wxID_ANY, "", wxDefaultPosition, wxSize(rodzic->GetTextExtent("00000").x + 24, -1));
            PomocGUI::Dodaj(t[k], kluczPomocy);
            g->Add(t[k]);
        }
        sizer->Add(g, 0, wxALIGN_CENTER | wxALL, 6);
    }
    /// Wpisuje wymiary kształtki (ChangeValue: bez zdarzeń)
    void Wpisz(const Dlawik::Rdzen& r) {
        const double v[6] = {r.A, r.B, r.a, r.c, r.d, r.h};
        for (int k = 0; k < 6; k++) t[k]->ChangeValue(TekstZUTF8(liczbaNaTekstPolski(v[k], 2, 0)));
    }
    /// Odczyt pól; false, gdy któreś nie jest liczbą dodatnią
    bool Odczytaj(double pakietMm, Dlawik::Rdzen& r) const {
        double v[6];
        for (int k = 0; k < 6; k++)
            if (!tekstNaLiczbe(t[k]->GetValue().utf8_string(), v[k]) || !(v[k] > 0)) return false;
        if (!(pakietMm > 0)) return false;
        r.A = v[0]; r.B = v[1]; r.a = v[2]; r.c = v[3]; r.d = v[4]; r.h = v[5]; r.b = pakietMm;
        return true;
    }
    /// Indeks pola po nazwie testowej: "wymA", "wymB", "wyma", "wymc", "wymd", "wymh"; -1 gdy to nie wymiar
    static int Indeks(const wxString& n) {
        static const char* nazwy[6] = {"wymA", "wymB", "wyma", "wymc", "wymd", "wymh"};
        for (int k = 0; k < 6; k++) if (n == nazwy[k]) return k;
        return -1;
    }
};

#endif
