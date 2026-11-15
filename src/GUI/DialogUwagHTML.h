#ifndef DIALOGUWAGHTML_H
#define DIALOGUWAGHTML_H

#include <wx/wx.h>
#include <wx/html/htmlwin.h>
#include <wx/splitter.h>

/**
 * @class DialogUwagHTML
 * @brief Edytor uwag w HTML 4: u góry podgląd na żywo, u dołu źródło HTML. Przyciski wstawiają
 *        najczęstsze znaczniki (pogrubienie, kursywa, indeksy, lista, nowa linia).
 *        Podgląd obsługuje podzbiór HTML 4 (wxHtmlWindow): akapity, nagłówki, listy, tabele, <b> <i> <u> <sub> <sup>, linki, obrazy.
 */
class DialogUwagHTML : public wxDialog {
public:
    DialogUwagHTML(wxWindow* parent, const wxString& zrodlo, const wxString& tytul = wxEmptyString);

    wxString Zrodlo() const { return edytor_->GetValue(); }
    void UstawZrodlo(const wxString& html) { edytor_->SetValue(html); }
    /// HTML pokazywany w podglądzie (tekst bez znaczników dostaje <br> w miejscu nowych linii)
    wxString HtmlPodgladu() const;
    /// Otacza zaznaczenie znacznikami (bez zaznaczenia wstawia pustą parę z kursorem w środku)
    void Otocz(const wxString& otwierajacy, const wxString& zamykajacy);

private:
    wxHtmlWindow* podglad_ = nullptr;
    wxTextCtrl* edytor_ = nullptr;
    void Odswiez();
};

#endif // DIALOGUWAGHTML_H
