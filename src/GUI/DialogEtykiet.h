#ifndef DIALOGETYKIET_H
#define DIALOGETYKIET_H

#include "RysowanieEtykiet.h"
#include "../Narzedzia/UstawieniaZebry.h"

#include <wx/wx.h>
#include <wx/spinctrl.h>

/**
 * Okno drukowania etykiet z kodem kreskowym (EAN-13 / EAN-8 / Code128 wg ustawień).
 * Kod zawiera numer elementu, więc zeskanowanie etykiety (skaner USB wpisuje kod w pole
 * szukania) od razu pokazuje element.
 */
class DialogEtykiet : public wxDialog {
public:
    /// `elementy` - elementy z listy; `idWybranego` - zaznaczony (0 = brak)
    DialogEtykiet(wxWindow* parent, const std::vector<Element>& elementy, long long idWybranego);

    std::vector<EtykietaDoDruku> Etykiety() const;
    UstawieniaDruku Ustawienia() const;
    /// Zapisuje wszystkie strony jako PNG (nazwa_1.png, nazwa_2.png...); zwraca liczbę zapisanych
    int ZapiszPNG(const wxString& sciezkaBazowa, double dpi = 200) const;
    void WybierzTylkoZaznaczony(bool tak);

    // --- druk na Zebrze (ZPL)
    bool ZebraAktywna() const;
    void UstawZebra(bool tak);
    /// Etykiety w postaci dla generatora ZPL (z kopiami, bez grupowania)
    std::vector<EtykietyZPL::Etykieta> EtykietyZebry() const;
    /// Pełne polecenia ZPL dla bieżącego wyboru
    std::string PoleceniaZPL() const;
    /// Wysyła na drukarkę według ustawień; zwraca powodzenie (błąd pokazuje komunikatem, gdy cicho = false)
    bool DrukujZebra(bool cicho = false);

private:
    std::vector<Element> elementy_;
    long long idWybranego_;
    wxRadioButton* rbZaznaczony = nullptr;
    wxRadioButton* rbWszystkie = nullptr;
    wxSpinCtrl* scKopie = nullptr;
    wxChoice* chFormat = nullptr;
    wxSpinCtrl* scStart = nullptr;
    wxSpinCtrlDouble* scX = nullptr;
    wxSpinCtrlDouble* scY = nullptr;
    wxCheckBox* cbRamki = nullptr;
    wxChoice* chSposob = nullptr;
    wxButton* bDrukuj_ = nullptr;
    wxButton* bUstZebry_ = nullptr;
    wxButton* bZapiszZpl_ = nullptr;
    wxButton* bPodglad_ = nullptr;
    wxButton* bPng_ = nullptr;
    UstawieniaZebry zebra_;
    wxStaticBitmap* podglad = nullptr;
    wxStaticText* lbInfo = nullptr;

    void Odswiez();
    void Podglad();
    void Drukuj();
    void UstawieniaZebryOknem();
    void ZapiszZPLOknem();
    void OdswiezAktywne();
    void ZapiszPNGOknem();
};

#endif // DIALOGETYKIET_H
