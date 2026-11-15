#ifndef DIALOGKALKULATOROW_H
#define DIALOGKALKULATOROW_H

#include <wx/wx.h>
#include "PanelSchematu.h"
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <initializer_list>
#include <vector>

/// Narzędzia: dzielnik napięcia, rezystor do diody LED, filtr RC, zasilacz z trafa sieciowego, punkt pracy tranzystora,
/// tabela ESR kondensatorów, regulowane stabilizatory LM317/LM337 oraz cewki powietrzne (jedno- i wielowarstwowa). Każda zakładka ma schemat z wyróżnionymi liczonymi elementami.
/// Wyniki liczą się na bieżąco, w trakcie wpisywania. (Trafo głośnikowe: Elementy indukcyjne; lampa: Audio i lampowe.)
class DialogKalkulatorow : public wxDialog {
public:
    /// tylkoCewki = false: kalkulatory (strony 0-6); true: okno "Elementy indukcyjne" z cewkami powietrznymi (strony 7, 8)
    explicit DialogKalkulatorow(wxWindow* parent, bool tylkoCewki = false);

    // dostęp dla testów
    void PokazStrone(int nr) { ksiazka->SetSelection(nr); }
    void UstawDzielnik(const wxString& uwe, const wxString& uwy, const wxString& r1, const wxString& r2);
    void UstawLED(const wxString& uzas, const wxString& uf, const wxString& pradMA);
    void UstawRC(const wxString& r, const wxString& c, const wxString& f);
    wxString OpisDzielnika() const { return lbDzielnik->GetLabel(); }
    wxString SprawdzenieDzielnika() const { return lbDzielnik2->GetLabel(); }
    wxString OpisLED() const { return lbLED->GetLabel(); }
    wxString OpisRC() const { return lbRC->GetLabel(); }
    wxString OpisRCDobor() const { return lbRC2->GetLabel(); }
    /// Strony 3 i 4: zasilacz, tranzystor. Pola w kolejności z formularza; "" zostawia puste.
    void UstawPola(int strona, std::initializer_list<wxString> wartosci);
    /// Strony 6 (LM317/337), 7 (cewka jednowarstwowa), 8 (wielowarstwowa): pola jak w formularzu.
    /// Wynik strony 3, 4, 5 (ESR), 6, 7 albo 8; nr 0 = główny, 1 = dodatkowy (dobór)
    wxString WynikStrony(int strona, int nr = 0) const;
    void UstawProstownik(int nr) { chProst->SetSelection(nr); PrzeliczZasilacz(); }
    void UstawESR(const wxString& c, const wxString& u, const wxString& pradTetnien);
    int LiczbaWierszyESR() const { return tabelaESR->GetItemCount(); }
    wxString KomorkaESR(int wiersz, int kolumna) const;
    /// Rodzaj schematu na stronie zasilacza (zależy od wybranego prostownika)
    PanelSchematu::Rodzaj RodzajSchematuZasilacza() const { return psZas->RodzajSchematu(); }
    /// Strona 6: wybór LM317 (0) / LM337 (1)
    void UstawTypLM(int nr) { chTypLM->SetSelection(nr); PrzeliczLM(); }
    void WybierzStrone(int nr) { ksiazka->SetSelection(nr); }
    int LiczbaStron() const { return static_cast<int>(ksiazka->GetPageCount()); }

private:
    wxNotebook* ksiazka = nullptr;
    wxChoice *chSzeregD = nullptr, *chSzeregL = nullptr;
    wxTextCtrl *tcUwe = nullptr, *tcUwy = nullptr, *tcR1 = nullptr, *tcR2 = nullptr;
    wxTextCtrl *tcUzas = nullptr, *tcUf = nullptr, *tcI = nullptr;
    wxTextCtrl *tcRC_R = nullptr, *tcRC_C = nullptr, *tcRC_f = nullptr;
    wxStaticText *lbDzielnik = nullptr, *lbDzielnik2 = nullptr, *lbLED = nullptr, *lbRC = nullptr, *lbRC2 = nullptr;

    wxChoice *chProst = nullptr, *chSiec = nullptr, *chSzeregT = nullptr;
    std::vector<wxTextCtrl*> poleZas, poleTR, poleESR;
    wxListCtrl* tabelaESR = nullptr;
    PanelSchematu* psZas = nullptr;
    wxChoice *chTypLM = nullptr, *chSzeregLM = nullptr;
    std::vector<wxTextCtrl*> poleLM, poleC1, poleC2;
    wxStaticText *lbLM = nullptr, *lbLM2 = nullptr, *lbC1 = nullptr, *lbC1b = nullptr, *lbC2 = nullptr, *lbC2b = nullptr;
    wxStaticText *lbZas = nullptr, *lbZas2 = nullptr, *lbTR = nullptr, *lbTR2 = nullptr, *lbESR = nullptr;

    wxPanel* StronaZasilacza();
    wxPanel* StronaESR();
    wxPanel* StronaTranzystora();
    void PrzeliczZasilacz();
    void PrzeliczESR();
    void PrzeliczTranzystor();
    wxPanel* StronaLM();
    wxPanel* StronaCewki(bool wielowarstwowa);
    void PrzeliczLM();
    void PrzeliczCewke(bool wielowarstwowa);
    wxPanel* StronaDzielnika();
    wxPanel* StronaLED();
    wxPanel* StronaRC();
    void PrzeliczDzielnik();
    void PrzeliczLED();
    void PrzeliczRC();
};

#endif // DIALOGKALKULATOROW_H
