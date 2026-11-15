#include "ZestawienieDrutu.h"

#include "../Narzedzia/Liczby.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"

namespace ZestawienieDrutu {

namespace {
wxString L(double v, int m) { return TekstZUTF8(liczbaNaTekstPolski(v, m, m)); }
}

wxString Tekst(const MasaDrutu::Zestawienie& z) {
    if (z.puste()) return wxString();
    wxString t = wxString::Format("%-9s %9s %11s %13s  %s\n", TR("drut[mm]"), TR("długość[m]"), TR("miedź[g]"), TR("do zakupu[g]"), TR("uzwojenia"));
    for (const auto& w : z.wiersze)
        t += wxString::Format("%-9s %9s %11s %13s  %s\n", L(w.drutMm, 2), L(w.dlugoscM, 1), L(w.masaCuG, 1), L(w.masaZakupG, 0), wxString::FromUTF8(w.uzwojenia));
    t += wxString::Format("%-9s %9s %11s %13s\n", TR("razem"), L(z.razemDlugoscM, 1), L(z.razemCuG, 1), L(z.razemZakupG, 0));
    t += wxString::Format(TR("Do zakupu = miedź + ok. %.0f%% lakieru + %.0f%% zapasu (wartości orientacyjne)."), MasaDrutu::kDodatekEmaliiPct, MasaDrutu::kZapasPct);
    return t;
}

wxTextCtrl* Dodaj(wxWindow* rodzic, wxSizer* sizer, const char* kluczPomocy, int margines) {
    auto* ramka = new wxStaticBoxSizer(wxVERTICAL, rodzic, TR("Zestawienie drutu (waga)"));
    auto* tc = new wxTextCtrl(ramka->GetStaticBox(), wxID_ANY, "", wxDefaultPosition, wxSize(-1, 105), wxTE_MULTILINE | wxTE_READONLY);
    tc->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    PomocGUI::Dodaj(tc, kluczPomocy);
    ramka->Add(tc, 1, wxEXPAND | wxALL, 4);
    if (margines == 0) sizer->Add(ramka, 1, wxEXPAND | wxTOP, 6);   // na pełną szerokość sąsiedniej ramki
    else sizer->Add(ramka, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, margines);
    return tc;
}

}  // namespace ZestawienieDrutu
