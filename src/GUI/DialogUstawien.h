#ifndef DIALOGUSTAWIEN_H
#define DIALOGUSTAWIEN_H

#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/colordlg.h>
#include <wx/listctrl.h>

/**
 * @class DialogUstawien
 * @brief Dialog do konfiguracji ustawień aplikacji i bazy danych
 */
class DialogUstawien : public wxDialog {
public:
    explicit DialogUstawien(wxWindow* parent);

    /// Czy w tym oknie zmieniono ustawienia połączenia z bazą (okno główne powinno połączyć się ponownie)
    bool ZmienionoPolaczenie() const { return zmienionoPolaczenie; }

private:
    // === BAZA DANYCH ===
    wxStaticText* stOpisBaza = nullptr;
    wxButton* btnPolaczenie = nullptr;

    // === ŚCIEŻKI ===
    wxTextCtrl* tcSciezkaObrazki;
    wxButton* btnBrowseObrazki;
    wxTextCtrl* tcSciezkaPDF;
    wxTextCtrl* tcRepozytorium = nullptr;
    wxButton* btnBrowsePDF;

    // === KOD EAN ===
    wxChoice* choiceTypEAN;
    wxSpinCtrl* scPrefixEAN;

    // === CZCIONKA ===
    wxChoice* choiceFonty;
    wxSpinCtrl* scRozmiarFontu;
    wxCheckBox* cbPogrubienie;
    wxCheckBox* cbKursywa;
    wxStaticText* stPrzeglad;

    // === JĘZYK I LITERATURA ===
    wxChoice* choiceJezyk = nullptr;
    wxTextCtrl* tcCzytnikPDF = nullptr;
    wxTextCtrl* tcExifTool = nullptr;
    wxTextCtrl* tcOcrmypdf = nullptr;
    wxTextCtrl* tcOcrJezyki = nullptr;
    wxTextCtrl* tcMouserKlucz = nullptr;

    // === PRZYCISKI ===
    wxButton* btnOK;
    wxButton* btnAnuluj;

    // Event handlery
    void OnUstawieniaPolaczenia(wxCommandEvent& event);
    void OnBrowseObrazki(wxCommandEvent& event);
    void OnBrowsePDF(wxCommandEvent& event);
    void OnFontChanged(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnAnuluj(wxCommandEvent& event);

    bool zmienionoPolaczenie = false;

    // Metody pomocnicze
    void UtworzKontrole();
    void UlozKontrole();
    void WczytajUstawienia();
    void AktualizujPrzeglad();
    void OdswiezOpisBazy();

    wxDECLARE_EVENT_TABLE();
};

#endif // DIALOGUSTAWIEN_H

