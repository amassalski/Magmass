#include "DialogUwagHTML.h"

#include "../Narzedzia/Tlumaczenia.h"
#include "../Narzedzia/UwagiHTML.h"
#include "../Pomoc/PomocGUI.h"

DialogUwagHTML::DialogUwagHTML(wxWindow* parent, const wxString& zrodlo, const wxString& tytul)
    : wxDialog(parent, wxID_ANY, tytul.empty() ? TR("Uwagi (HTML)") : TR("Uwagi (HTML): ") + tytul, wxDefaultPosition, wxSize(760, 620),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* pasek = new wxBoxSizer(wxHORIZONTAL);
    auto przycisk = [&](const wxString& et, const wxString& podpowiedz, const wxString& otw, const wxString& zam) {
        wxButton* b = new wxButton(this, wxID_ANY, et, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
        b->SetToolTip(podpowiedz);
        pasek->Add(b, 0, wxRIGHT, 4);
        b->Bind(wxEVT_BUTTON, [this, otw, zam](wxCommandEvent&) { Otocz(otw, zam); });
    };
    przycisk("B", TR("Pogrubienie"), "<b>", "</b>");
    przycisk("I", TR("Kursywa"), "<i>", "</i>");
    przycisk("U", TR("Podkreślenie"), "<u>", "</u>");
    przycisk("x₂", TR("Indeks dolny"), "<sub>", "</sub>");
    przycisk("x²", TR("Indeks górny"), "<sup>", "</sup>");
    przycisk(TR("Lista"), TR("Lista punktowana"), "<ul>\n<li>", "</li>\n</ul>");
    przycisk("<br>", TR("Nowa linia"), "<br>\n", "");
    przycisk("<p>", TR("Akapit"), "<p>", "</p>");
    glowny->Add(pasek, 0, wxALL, 6);

    wxSplitterWindow* podzial = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);
    podglad_ = new wxHtmlWindow(podzial, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO | wxBORDER_THEME);
    edytor_ = new wxTextCtrl(podzial, wxID_ANY, zrodlo, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_DONTWRAP);
    wxFont mono(wxFontInfo(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize()).Family(wxFONTFAMILY_TELETYPE));
    edytor_->SetFont(mono);
    podzial->SplitHorizontally(podglad_, edytor_, 260);
    podzial->SetMinimumPaneSize(80);
    podzial->SetSashGravity(0.5);
    glowny->Add(podzial, 1, wxEXPAND | wxLEFT | wxRIGHT, 6);
    glowny->Add(new wxStaticText(this, wxID_ANY, TR("Góra: podgląd na żywo. Dół: źródło HTML (do edycji).")), 0, wxALL, 6);
    glowny->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 8);
    SetSizer(glowny);
    edytor_->Bind(wxEVT_TEXT, [this](wxCommandEvent& e) { Odswiez(); e.Skip(); });
    Odswiez();
    edytor_->SetFocus();
}

wxString DialogUwagHTML::HtmlPodgladu() const {
    return "<html><body>" + TekstZUTF8(UwagiHTML::zTekstu(edytor_->GetValue().utf8_string())) + "</body></html>";
}

void DialogUwagHTML::Odswiez() { podglad_->SetPage(HtmlPodgladu()); }

void DialogUwagHTML::Otocz(const wxString& otwierajacy, const wxString& zamykajacy) {
    long od = 0, do_ = 0;
    edytor_->GetSelection(&od, &do_);
    const wxString wybrany = edytor_->GetRange(od, do_);
    edytor_->Replace(od, do_, otwierajacy + wybrany + zamykajacy);
    edytor_->SetInsertionPoint(wybrany.empty() ? od + static_cast<long>(otwierajacy.length()) : od + static_cast<long>((otwierajacy + wybrany + zamykajacy).length()));
    edytor_->SetFocus();
}
