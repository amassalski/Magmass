#ifndef DIALOGOPROGRAMIE_H
#define DIALOGOPROGRAMIE_H

#include <wx/wx.h>
#include <wx/html/htmlwin.h>
#include <wx/notebook.h>

/**
 * @class DialogOProgramie
 * @brief Okno "O programie": nazwa, wersja, autor, link do repozytorium, lista możliwości, podziękowania dla użytych
 *        bibliotek i programów oraz pełny tekst licencji GPL (plik LICENSE obok programu). Jednakowe na każdym systemie.
 */
class DialogOProgramie : public wxDialog {
public:
    explicit DialogOProgramie(wxWindow* parent);

    static const char* Autor() { return "Adam Massalski"; }
    static const char* Repozytorium() { return "https://github.com/amassalski/Magmass"; }
    static const char* AdresLicencji() { return "https://www.gnu.org/licenses/gpl-3.0.html"; }

    /// Strona "O programie" (HTML): opis, możliwości, autor, repozytorium, krótka informacja o licencji
    static wxString TrescOProgramie();
    /// Strona "Podziękowania" (HTML): biblioteki i programy zewnętrzne z ich licencjami
    static wxString TrescPodziekowan();
    /// Pełny tekst licencji GPL z pliku LICENSE (obok programu, w zasobach albo w /usr/share/...); gdy go nie ma, krótka informacja i adres
    static wxString TekstLicencji();

    wxNotebook* Karty() const { return karty_; }

private:
    wxNotebook* karty_ = nullptr;
};

#endif // DIALOGOPROGRAMIE_H
