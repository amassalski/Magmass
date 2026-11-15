#ifndef DIALOGEDYCJIZBIORCZEJ_H
#define DIALOGEDYCJIZBIORCZEJ_H

#include "../Logika/LogikaMagazynu.h"

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

#include <vector>

/**
 * @class DialogEdycjiZbiorczej
 * @brief Zmiana wspólnych parametrów wielu zaznaczonych elementów: kategoria, producent, tolerancja,
 *        ilość, stan minimalny, cena jednostkowa, tagi. Zmieniane są tylko pola z zaznaczonym "Zmień".
 */
class DialogEdycjiZbiorczej : public wxDialog {
public:
    DialogEdycjiZbiorczej(wxWindow* parent, LogikaMagazynu& logika, int liczbaElementow);

    ZmianyZbiorcze Zmiany() const;
    /// Do testów: ustawia pola bez klikania
    void UstawProducenta(const wxString& t);
    void UstawIloscMinimalna(int n);
    void UstawTagi(const wxString& t, bool dopisz);

private:
    std::vector<long long> idKategorii_;   // indeks 0 = bez kategorii
    wxCheckBox *cbKategoria_, *cbProducent_, *cbTolerancja_, *cbIlosc_, *cbMinimum_, *cbCena_, *cbTagi_;
    wxChoice* chKategoria_;
    wxTextCtrl *tcProducent_, *tcTolerancja_, *tcCena_, *tcTagi_;
    wxSpinCtrl *scIlosc_, *scMinimum_;
    wxChoice* chTagi_;

    void Odswiez();
    void OnOK(wxCommandEvent& event);
};

#endif // DIALOGEDYCJIZBIORCZEJ_H
