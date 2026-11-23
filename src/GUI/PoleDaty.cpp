#include "PoleDaty.h"

#include "../Narzedzia/Tlumaczenia.h"

#include <functional>

#include <wx/button.h>
#include <wx/dateevt.h>
#include <wx/datectrl.h>
#include <wx/calctrl.h>
#include <wx/generic/calctrlg.h>
#include <wx/popupwin.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

namespace {

/// Kolory kalendarza z kolorów systemu (przy ciemnym motywie są ciemne).
void Pokoloruj(wxWindow* w, const wxColour& tlo, const wxColour& tekst) {
    w->SetBackgroundColour(tlo);
    w->SetForegroundColour(tekst);
    for (wxWindow* d : w->GetChildren()) Pokoloruj(d, tlo, tekst);
}

/// Okienko z kalendarzem pod polem daty.
class KalendarzPopup : public wxPopupTransientWindow {
public:
    KalendarzPopup(wxWindow* rodzic, const wxDateTime& data, bool brak, std::function<void(const wxDateTime&, bool)> wybrano)
        : wxPopupTransientWindow(rodzic, wxBORDER_SIMPLE | wxPU_CONTAINS_CONTROLS), wybrano_(std::move(wybrano)) {
        const wxColour tlo = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        const wxColour tekst = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        const wxColour zazn = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
        const wxColour zaznTekst = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
        const wxColour szary = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);

        wxPanel* panel = new wxPanel(this);
        wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
        kal_ = new wxGenericCalendarCtrl(panel, wxID_ANY, data.IsValid() ? data : wxDateTime::Today(), wxDefaultPosition, wxDefaultSize,
                                         wxCAL_MONDAY_FIRST | wxCAL_SHOW_SURROUNDING_WEEKS);
        s->Add(kal_, 0, wxEXPAND | wxALL, 4);
        wxBoxSizer* dol = new wxBoxSizer(wxHORIZONTAL);
        wxButton* dzis = new wxButton(panel, wxID_ANY, TR("Dziś"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
        dol->Add(dzis, 0, wxRIGHT, 6);
        if (brak) {
            wxButton* b = new wxButton(panel, wxID_ANY, TR("Brak daty"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
            dol->Add(b, 0);
            b->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Zakoncz(wxDateTime(), false); });
        }
        s->Add(dol, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
        panel->SetSizer(s);

        Pokoloruj(panel, tlo, tekst);
        kal_->SetHeaderColours(tekst, szary);
        kal_->SetHighlightColours(zaznTekst, zazn);
        kal_->SetHolidayColours(tekst, tlo);

        wxBoxSizer* g = new wxBoxSizer(wxVERTICAL);
        g->Add(panel, 1, wxEXPAND);
        SetSizerAndFit(g);

        dzis->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Zakoncz(wxDateTime::Today(), true); });
        kal_->Bind(wxEVT_CALENDAR_DOUBLECLICKED, [this](wxCalendarEvent& e) { Zakoncz(e.GetDate(), true); });
        // pojedyncze kliknięcie w dzień wybiera datę; zmiana miesiąca (listy, strzałki) tylko przewija kalendarz
        kal_->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
            wxDateTime d;
            if (kal_->HitTest(e.GetPosition(), &d) == wxCAL_HITTEST_DAY) Zakoncz(d, true);
            else e.Skip();
        });
    }

    void Zakoncz(const wxDateTime& d, bool ok) {
        if (zakonczono_) return;
        zakonczono_ = true;
        auto f = wybrano_;
        Dismiss();
        if (f) f(d, ok);
    }

private:
    wxGenericCalendarCtrl* kal_ = nullptr;
    std::function<void(const wxDateTime&, bool)> wybrano_;
    bool zakonczono_ = false;
};

wxString Format(const wxDateTime& d) { return d.IsValid() ? d.FormatISODate() : wxString(); }

}  // namespace

PoleDaty::PoleDaty(wxWindow* rodzic, const wxDateTime& data, long styl) : wxPanel(rodzic, wxID_ANY), dozwolonyBrak_((styl & wxDP_ALLOWNONE) != 0) {
    wxBoxSizer* s = new wxBoxSizer(wxHORIZONTAL);
    tekst_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    const wxSize rozm = tekst_->GetTextExtent(wxT("0000-00-00MM"));
    tekst_->SetMinSize(wxSize(rozm.x + 12, -1));
    przycisk_ = new wxButton(this, wxID_ANY, wxString::FromUTF8("\xE2\x96\xBE"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    przycisk_->SetToolTip(TR("Pokaż kalendarz"));
    s->Add(tekst_, 1, wxALIGN_CENTER_VERTICAL);
    s->Add(przycisk_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
    SetSizerAndFit(s);

    SetValue(data);

    przycisk_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PokazKalendarz(); });
    tekst_->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { PrzeczytajTekst(); });
    tekst_->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { PrzeczytajTekst(); e.Skip(); });
    tekst_->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_DOWN && e.AltDown()) PokazKalendarz();
        else e.Skip();
    });
}

void PoleDaty::SetValue(const wxDateTime& data) {
    if (data.IsValid()) data_ = data;
    else data_ = dozwolonyBrak_ ? wxDateTime() : wxDateTime::Today();
    Odswiez();
}

void PoleDaty::Odswiez() { tekst_->ChangeValue(Format(data_)); }

void PoleDaty::Powiadom() {
    wxDateEvent z(this, data_, wxEVT_DATE_CHANGED);
    z.SetEventObject(this);
    GetEventHandler()->ProcessEvent(z);
}

void PoleDaty::Wybierz(const wxDateTime& data) {
    const bool zmiana = data.IsValid() != data_.IsValid() || (data.IsValid() && !data.IsSameDate(data_));
    SetValue(data);
    if (zmiana) Powiadom();
}

void PoleDaty::PrzeczytajTekst() {
    const wxString t = tekst_->GetValue().Strip(wxString::both);
    if (t.empty()) {
        if (dozwolonyBrak_) Wybierz(wxDateTime());
        else Odswiez();
        return;
    }
    wxDateTime d;
    wxString::const_iterator koniec;
    const bool ok = (d.ParseFormat(t, wxT("%Y-%m-%d"), &koniec) && koniec == t.end()) || (d.ParseFormat(t, wxT("%d.%m.%Y"), &koniec) && koniec == t.end());
    if (ok && d.IsValid()) Wybierz(d);
    else Odswiez();
}

void PoleDaty::PokazKalendarz() {
    PrzeczytajTekst();
    KalendarzPopup* p = new KalendarzPopup(this, data_, dozwolonyBrak_, [this](const wxDateTime& d, bool ok) {
        if (ok || dozwolonyBrak_) Wybierz(d);
    });
    wxWindow* podstawa = this;
    const wxPoint pozycja = podstawa->ClientToScreen(wxPoint(0, podstawa->GetSize().y));
    p->Position(pozycja, wxSize(0, 0));
    p->Popup();
}
