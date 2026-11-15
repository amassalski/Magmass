#ifndef HTMLZDYMKAMI_H
#define HTMLZDYMKAMI_H

#include <wx/html/htmlwin.h>
#include <wx/tooltip.h>

#include <vector>

/// Okno HTML, w którym łącza "opis:N" pokazują dymek (opis parametru) po najechaniu; kliknięcie nic nie robi.
class HtmlZDymkami : public wxHtmlWindow {
public:
    using wxHtmlWindow::wxHtmlWindow;
    /// opisy[N] to tekst dymka dla łącza "opis:N"
    void UstawOpisy(std::vector<wxString> opisy) { opisy_ = std::move(opisy); }

    void OnCellMouseHover(wxHtmlCell* komorka, wxCoord x, wxCoord y) override {
        wxString tekst;
        if (komorka) {
            if (wxHtmlLinkInfo* l = komorka->GetLink(x, y)) {
                const wxString h = l->GetHref();
                long n;
                if (h.StartsWith("opis:") && h.Mid(5).ToLong(&n) && n >= 0 && static_cast<size_t>(n) < opisy_.size()) tekst = opisy_[n];
            }
        }
        if (tekst != ostatni_) {
            ostatni_ = tekst;
            if (tekst.empty()) UnsetToolTip(); else SetToolTip(tekst);
        }
    }
    void OnLinkClicked(const wxHtmlLinkInfo&) override {}

private:
    std::vector<wxString> opisy_;
    wxString ostatni_;
};

#endif // HTMLZDYMKAMI_H
