#include "DialogTermowizji.h"
#include "../Narzedzia/Tlumaczenia.h"
#include "../Pomoc/PomocGUI.h"
#include "../Narzedzia/InfirayUsb.h"
#include "../Narzedzia/Konfiguracja.h"

#include <wx/dcbuffer.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

namespace {
Termowizja::Paleta PaletaZNumeru(int nr) {
    switch (nr) {
        case 1: return Termowizja::Paleta::Teczowa;
        case 2: return Termowizja::Paleta::Szara;
        case 3: return Termowizja::Paleta::SzaraOdwrocona;
        default: return Termowizja::Paleta::Zelazo;
    }
}
wxString T1(double c) { return wxString::Format("%.1f", c) + wxString::FromUTF8(" \xC2\xB0" "C"); }
bool Liczba(const wxString& t, double& v) { wxString s = t; s.Trim().Trim(false); s.Replace(",", "."); return s.ToCDouble(&v); }
}  // namespace

class DialogTermowizji::Podglad : public wxPanel {
public:
    explicit Podglad(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(560, 440)) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(400, 320));
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) { Maluj(); });
        Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
    }
    void Pokaz(const wxImage& im, const Termowizja::Statystyka& st, bool zDanymi, Termowizja::Paleta p) {
        obraz_ = im; st_ = st; zDanymi_ = zDanymi; paleta_ = p; Refresh(); Update();
    }
    void Komunikat(const wxString& t) { obraz_ = wxImage(); tekst_ = t; Refresh(); }
    const wxImage& Obraz() const { return obraz_; }

private:
    wxImage obraz_;
    wxString tekst_;
    Termowizja::Statystyka st_;
    bool zDanymi_ = false;
    Termowizja::Paleta paleta_ = Termowizja::Paleta::Zelazo;

    void Maluj() {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(*wxBLACK_BRUSH);
        dc.Clear();
        const wxSize s = GetClientSize();
        if (!obraz_.IsOk()) {
            dc.SetTextForeground(*wxWHITE);
            const wxSize t = dc.GetTextExtent(tekst_);
            dc.DrawText(tekst_, (s.x - t.x) / 2, (s.y - t.y) / 2);
            return;
        }
        const int skala = 100;                                   // miejsce na skalę kolorów z prawej
        const double sk = std::min(double(s.x - skala) / obraz_.GetWidth(), double(s.y) / obraz_.GetHeight());
        const int w = std::max(1, int(obraz_.GetWidth() * sk)), h = std::max(1, int(obraz_.GetHeight() * sk));
        const int x0 = (s.x - skala - w) / 2, y0 = (s.y - h) / 2;
        dc.DrawBitmap(wxBitmap(obraz_.Scale(w, h, wxIMAGE_QUALITY_NORMAL)), x0, y0);
        // skala kolorów
        const int sx = s.x - skala + 12, sh = h;
        for (int i = 0; i < sh; ++i) {
            unsigned char c[3];
            Termowizja::kolorPalety(paleta_, 1.0 - double(i) / std::max(1, sh - 1), c);
            dc.SetPen(wxPen(wxColour(c[0], c[1], c[2])));
            dc.DrawLine(sx, y0 + i, sx + 14, y0 + i);
        }
        dc.SetTextForeground(*wxWHITE);
        if (zDanymi_ && st_.ok) {
            dc.DrawText(T1(st_.maxC), sx + 20, y0 - 2);
            dc.DrawText(T1(st_.minC), sx + 20, y0 + sh - 16);
            auto P = [&](int x, int y) { return wxPoint(x0 + int((x + 0.5) * sk), y0 + int((y + 0.5) * sk)); };
            const int ow = obraz_.GetWidth(), oh = obraz_.GetHeight();
            // środek kadru: krzyżyk z odczytem
            const wxPoint c = P(ow / 2, oh / 2);
            dc.SetPen(wxPen(*wxWHITE, 1));
            dc.DrawLine(c.x - 8, c.y, c.x + 8, c.y); dc.DrawLine(c.x, c.y - 8, c.x, c.y + 8);
            dc.DrawText(T1(st_.srodekC), c.x + 6, c.y + 4);
            // najgorętszy (czerwony) i najzimniejszy (niebieski) punkt
            const wxPoint hot = P(st_.xMax, st_.yMax), cold = P(st_.xMin, st_.yMin);
            dc.SetPen(wxPen(wxColour(255, 60, 60), 2)); dc.DrawCircle(hot, 6);
            dc.SetPen(wxPen(wxColour(80, 160, 255), 2)); dc.DrawCircle(cold, 6);
        } else {
            dc.DrawText(TR("brak temperatur"), sx + 20, y0 - 2);
        }
    }
};

DialogTermowizji::DialogTermowizji(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, TR("Kamera termowizyjna"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      timer_(this) {
    wxBoxSizer* glowny = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gora = new wxBoxSizer(wxHORIZONTAL);
    gora->Add(new wxStaticText(this, wxID_ANY, TR("Kamera:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    chKamera = new wxChoice(this, wxID_ANY);
    PomocGUI::Dodaj(chKamera, "termo.kamera");
    gora->Add(chKamera, 1, wxRIGHT, 8);
    wxButton* bOdswiez = new wxButton(this, wxID_ANY, TR("Wykryj ponownie"));
    gora->Add(bOdswiez, 0);
    glowny->Add(gora, 0, wxEXPAND | wxALL, 10);

    wxBoxSizer* srodek = new wxBoxSizer(wxHORIZONTAL);
    podglad_ = new Podglad(this);
    srodek->Add(podglad_, 1, wxEXPAND | wxLEFT, 10);
    wxBoxSizer* prawa = new wxBoxSizer(wxVERTICAL);
    prawa->Add(new wxStaticText(this, wxID_ANY, TR("Paleta:")), 0, wxTOP, 4);
    chPaleta = new wxChoice(this, wxID_ANY);
    chPaleta->Append(TR("Żelazo")); chPaleta->Append(TR("Tęcza")); chPaleta->Append(TR("Szara")); chPaleta->Append(TR("Szara odwrócona"));
    chPaleta->SetSelection(0);
    PomocGUI::Dodaj(chPaleta, "termo.paleta");
    prawa->Add(chPaleta, 0, wxEXPAND | wxBOTTOM, 6);
    prawa->Add(new wxStaticText(this, wxID_ANY, TR("Układ ramki:")), 0);
    chUklad = new wxChoice(this, wxID_ANY);
    chUklad->Append(TR("Automatycznie")); chUklad->Append(TR("Obraz i dane (górna/dolna połowa)")); chUklad->Append(TR("Tylko obraz"));
    chUklad->SetSelection(0);
    PomocGUI::Dodaj(chUklad, "termo.uklad");
    prawa->Add(chUklad, 0, wxEXPAND | wxBOTTOM, 6);
    wxFlexGridSizer* g = new wxFlexGridSizer(2, 4, 6);
    g->Add(new wxStaticText(this, wxID_ANY, TR("Dzielnik:")), 0, wxALIGN_CENTER_VERTICAL);
    tcDzielnik = new wxTextCtrl(this, wxID_ANY, "64", wxDefaultPosition, wxSize(80, -1));
    PomocGUI::Dodaj(tcDzielnik, "termo.dzielnik");
    g->Add(tcDzielnik, 0);
    g->Add(new wxStaticText(this, wxID_ANY, TR("Przesunięcie [K]:")), 0, wxALIGN_CENTER_VERTICAL);
    tcOffset = new wxTextCtrl(this, wxID_ANY, "273,15", wxDefaultPosition, wxSize(80, -1));
    PomocGUI::Dodaj(tcOffset, "termo.offset");
    g->Add(tcOffset, 0);
    prawa->Add(g, 0, wxBOTTOM, 8);
    bZamroz = new wxButton(this, wxID_ANY, TR("Zamroź obraz"));
    prawa->Add(bZamroz, 0, wxEXPAND | wxBOTTOM, 4);
    prawa->Add(new wxStaticText(this, wxID_ANY, TR("Polecenie UVC zoom (hex):")), 0, wxTOP, 4);
    tcZoom = new wxTextCtrl(this, wxID_ANY, "8004", wxDefaultPosition, wxSize(80, -1));
    PomocGUI::Dodaj(tcZoom, "termo.zoom");
    prawa->Add(tcZoom, 0, wxBOTTOM, 2);
    wxButton* bZoom = new wxButton(this, wxID_ANY, TR("Wyślij polecenie (tryb surowy)"));
    PomocGUI::Dodaj(bZoom, "termo.zoom_wyslij");
    prawa->Add(bZoom, 0, wxEXPAND | wxBOTTOM, 4);
    prawa->Add(new wxStaticText(this, wxID_ANY, TR("Kalibracja (tryb surowy) - temperatura środka [°C]:")), 0, wxTOP, 4);
    tcTRef = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1));
    PomocGUI::Dodaj(tcTRef, "termo.kalib_temp");
    prawa->Add(tcTRef, 0, wxBOTTOM, 2);
    wxButton* bKal = new wxButton(this, wxID_ANY, TR("Dodaj punkt kalibracji"));
    PomocGUI::Dodaj(bKal, "termo.kalib_dodaj");
    prawa->Add(bKal, 0, wxEXPAND | wxBOTTOM, 2);
    wxButton* bKalCzysc = new wxButton(this, wxID_ANY, TR("Wyczyść kalibrację"));
    PomocGUI::Dodaj(bKalCzysc, "termo.kalib_czysc");
    prawa->Add(bKalCzysc, 0, wxEXPAND | wxBOTTOM, 4);
    wxButton* bProbka = new wxButton(this, wxID_ANY, TR("Zapisz próbkę (do analizy)..."));
    PomocGUI::Dodaj(bProbka, "termo.probka");
    prawa->Add(bProbka, 0, wxEXPAND | wxBOTTOM, 4);
    wxButton* bPng = new wxButton(this, wxID_ANY, TR("Zapisz obraz PNG..."));
    prawa->Add(bPng, 0, wxEXPAND | wxBOTTOM, 4);
    wxButton* bXt = new wxButton(this, wxID_ANY, TR("Biblioteka temperatur Xtherm (XthermDll.dll)..."));
    PomocGUI::Dodaj(bXt, "termo.xtherm_dll");
    prawa->Add(bXt, 0, wxEXPAND | wxBOTTOM, 4);
    bXt->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { WybierzXthermDll(); });
    wxButton* bSdk = new wxButton(this, wxID_ANY, TR("Sprawdź SDK IRay (USBSDK.dll)..."));
    PomocGUI::Dodaj(bSdk, "termo.sdk_iray");
    prawa->Add(bSdk, 0, wxEXPAND);
    bSdk->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DiagnostykaSdk(); });
    srodek->Add(prawa, 0, wxALL, 10);
    glowny->Add(srodek, 1, wxEXPAND);

    lbWynik = new wxStaticText(this, wxID_ANY, wxEmptyString);
    lbWynik->SetFont(lbWynik->GetFont().Scaled(1.2f).Bold());
    glowny->Add(lbWynik, 0, wxLEFT | wxRIGHT, 10);
    raport_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 170),
                             wxTE_MULTILINE | wxTE_READONLY);
    PomocGUI::Dodaj(raport_, "termo.raport");
    glowny->Add(raport_, 0, wxEXPAND | wxALL, 10);
    glowny->Add(CreateStdDialogButtonSizer(wxCLOSE), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    SetSizerAndFit(glowny);

    {   // biblioteka Xtherm z poprzedniego uruchomienia (jeśli plik nadal jest)
        const std::string sc = Konfiguracja::pobierz().pobierz("Termowizja", "xtherm_dll", "");
        std::string bl;
        if (!sc.empty() && wxFileName::FileExists(wxString::FromUTF8(sc)) && xtherm_.zaladuj(sc, bl)) xthermStan_ = wxString::FromUTF8(sc);
    }
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) { OnTimer(); });
    bOdswiez->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { WykryjKamery(); });
    chKamera->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { OtworzWybrana(); });
    chPaleta->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    chUklad->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { Przelicz(); });
    tcDzielnik->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Przelicz(); });
    tcOffset->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Przelicz(); });
    bZamroz->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Zamroz(!zamrozony_); });
    bZoom->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { WyslijZoom(tcZoom->GetValue()); });
    bKal->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DodajPunktKalibracji(tcTRef->GetValue()); });
    bKalCzysc->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { WyczyscKalibracje(); });
    bProbka->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxDirDialog dd(this, TR("Katalog na próbkę"), wxStandardPaths::Get().GetDocumentsDir());
        if (dd.ShowModal() != wxID_OK) return;
        const wxString f = ZapiszProbke(dd.GetPath());
        wxMessageBox(f.empty() ? TR("Nie udało się zapisać próbki.") : TR("Zapisano próbkę: ") + f, TR("Kamera termowizyjna"),
                     wxOK | (f.empty() ? wxICON_ERROR : wxICON_INFORMATION), this);
    });
    bPng->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxFileDialog fd(this, TR("Zapisz obraz"), wxEmptyString, "termowizja.png", "PNG (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (fd.ShowModal() == wxID_OK && !ZapiszObraz(fd.GetPath()))
            wxMessageBox(TR("Nie udało się zapisać obrazu."), TR("Kamera termowizyjna"), wxOK | wxICON_ERROR, this);
    });
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) { timer_.Stop(); kamera_.zamknij(); e.Skip(); });
    WykryjKamery();
}

DialogTermowizji::~DialogTermowizji() { timer_.Stop(); kamera_.zamknij(); }

double DialogTermowizji::Dzielnik() const { double v; return Liczba(tcDzielnik->GetValue(), v) ? v : 64.0; }
double DialogTermowizji::OffsetK() const { double v; return Liczba(tcOffset->GetValue(), v) ? v : 273.15; }

void DialogTermowizji::UstawPalete(int nr) { chPaleta->SetSelection(nr); Przelicz(); }
void DialogTermowizji::UstawUklad(int nr) { chUklad->SetSelection(nr); Przelicz(); }
void DialogTermowizji::UstawParametry(const wxString& d, const wxString& o) {
    tcDzielnik->ChangeValue(d); tcOffset->ChangeValue(o); Przelicz();
}

void DialogTermowizji::Zamroz(bool z) {
    zamrozony_ = z;
    bZamroz->SetLabel(z ? TR("Wznów podgląd") : TR("Zamroź obraz"));
    if (!z && kamera_.otwarta()) timer_.Start(100);
}

void DialogTermowizji::WykryjKamery() {
    timer_.Stop();
    kamera_.zamknij();
    chKamera->Clear();
    {
        wxBusyCursor zajety;
        kamery_ = Kamera::wykryjSurowe();
    }
    if (kamery_.empty()) {
        podglad_->Komunikat(TR("Nie wykryto kamery"));
        lbWynik->SetLabel(wxEmptyString);
        raport_->SetValue(Kamera::obslugiwana()
            ? TR("Podłącz kamerę termowizyjną przez USB i naciśnij „Wykryj ponownie”. System może zapytać o zgodę na dostęp do kamery.") + "\n\n" +
              TR("Próby otwarcia (OpenCV):") + "\n" + TekstZUTF8(Kamera::raportWykrywania())
            : TR("Ta wersja programu została zbudowana bez obsługi kamery (OpenCV)."));
        return;
    }
    for (const InfoKamery& k : kamery_) chKamera->Append(TekstZUTF8(k.nazwa));
    chKamera->SetSelection(0);
    OtworzWybrana();
}

void DialogTermowizji::OtworzWybrana() {
    timer_.Stop();
    const int i = chKamera->GetSelection();
    if (i < 0 || i >= static_cast<int>(kamery_.size())) return;
    if (!kamera_.otworzSurowo(kamery_[i].indeks)) {
        podglad_->Komunikat(TekstZUTF8(kamera_.blad()));
        raport_->SetValue(TekstZUTF8(kamera_.blad()));
        return;
    }
    zamrozony_ = false;
    bZamroz->SetLabel(TR("Zamroź obraz"));
    timer_.Start(100);
    OnTimer();
}

void DialogTermowizji::OnTimer() {
    if (zamrozony_ || !kamera_.otwarta()) return;
    Termowizja::KlatkaSurowa k;
    if (!kamera_.pobierzSurowa(k)) {
        if (klatek_ == 0 && !kamera_.blad().empty()) raport_->SetValue(TekstZUTF8(kamera_.blad()));
        return;
    }
    klatka_ = std::move(k);
    ++klatek_;
    Przelicz();
}

void DialogTermowizji::Przelicz() {
    if (klatka_.pusta()) return;
    uklad_ = Termowizja::rozpoznaj(klatka_);
    const int wyb = chUklad->GetSelection();
    if (wyb == 1 && klatka_.bajtyNaPiksel() == 2 && klatka_.wys % 2 == 0) {
        uklad_.uklad = Termowizja::Uklad::ObrazIDane; uklad_.obrazSzer = klatka_.szer; uklad_.obrazWys = klatka_.wys / 2;
    } else if (wyb == 2) {
        uklad_.uklad = Termowizja::Uklad::TylkoObraz; uklad_.obrazSzer = klatka_.szer; uklad_.obrazWys = klatka_.wys;
    }
    const Termowizja::Paleta pal = PaletaZNumeru(chPaleta->GetSelection());
    const auto dane = Termowizja::daneRadiometryczne(klatka_, uklad_);
    std::vector<unsigned char> rgb;
    stat_ = Termowizja::Statystyka();
    if (!dane.empty()) {
        if (uklad_.uklad == Termowizja::Uklad::ObrazIDane) stat_ = Termowizja::statystyki(dane, uklad_.obrazSzer, uklad_.obrazWys, Dzielnik(), OffsetK());
        else if (uklad_.uklad == Termowizja::Uklad::Surowe16) stat_ = Termowizja::statystykiKalibrowane(dane, uklad_.obrazSzer, uklad_.obrazWys, kal_);
        rgb = Termowizja::obrazZDanych(dane, uklad_.obrazSzer, uklad_.obrazWys, pal);
    } else {
        rgb = Termowizja::obrazZJasnosci(klatka_, uklad_, pal);
    }
    bool zXtherm = false;
    xthermOst_ = XthermLib::Wynik();
    if (xtherm_.zaladowana() && klatka_.bajtyNaPiksel() == 2 &&
        (uklad_.uklad == Termowizja::Uklad::Surowe16 || uklad_.uklad == Termowizja::Uklad::ObrazZNaglowkiem)) {
        const bool pelna = uklad_.uklad == Termowizja::Uklad::Surowe16;
        const auto w = xtherm_.oblicz(klatka_.dane.data(), klatka_.dane.size(), klatka_.szer, klatka_.wys, pelna, xthermOdswiez_);
        xthermOdswiez_ = false;
        if (w.ok) {
            stat_ = Termowizja::Statystyka();
            stat_.ok = true; stat_.minC = w.minC; stat_.maxC = w.maxC; stat_.srodekC = w.srodekC;
            stat_.xMin = w.xMin; stat_.yMin = w.yMin; stat_.xMax = w.xMax; stat_.yMax = w.yMax;
            double suma = 0; size_t n = 0;
            for (int y = 0; y < uklad_.obrazWys && pelna; ++y)
                for (int x = 0; x < uklad_.obrazSzer; ++x) { suma += w.wszystkie[static_cast<size_t>(y) * klatka_.szer + x]; ++n; }
            stat_.sredniaC = n ? suma / n : w.srodekC;
            zXtherm = true;
            xthermOst_ = w;
        }
    }
    if (rgb.empty()) return;
    wxImage im(uklad_.obrazSzer, uklad_.obrazWys);
    std::memcpy(im.GetData(), rgb.data(), rgb.size());
    podglad_->Pokaz(im, stat_, stat_.ok, pal);
    if (stat_.ok)
        lbWynik->SetLabel((zXtherm ? TR("(biblioteka Xtherm) ") : uklad_.uklad == Termowizja::Uklad::Surowe16 ? TR("(kalibracja użytkownika) ") : wxString()) + TR("Środek: ") + T1(stat_.srodekC) + TR("   min: ") + T1(stat_.minC) + TR("   max: ") + T1(stat_.maxC));
    else if (uklad_.uklad == Termowizja::Uklad::Surowe16 && !dane.empty()) {
        const auto mm = std::minmax_element(dane.begin(), dane.end());
        lbWynik->SetLabel(TR("Surowe odczyty (bez kalibracji do °C) - środek: ") + wxString::Format("%u", dane[dane.size() / 2 + uklad_.obrazSzer / 2]) +
                          TR("   min: ") + wxString::Format("%u", *mm.first) + TR("   max: ") + wxString::Format("%u", *mm.second));
    } else
        lbWynik->SetLabel(TR("Brak danych temperatury w tej ramce (tylko obraz)"));
    raport_->ChangeValue((komunikatPolecenia_.empty() ? wxString() : komunikatPolecenia_ + "\n\n") + TekstZUTF8("Wykrywanie (OpenCV):\n" + Kamera::raportWykrywania() + "\n" + kamera_.wlasciwosci() + Termowizja::raportDiagnostyczny(klatka_, Dzielnik(), OffsetK())));
}

wxString DialogTermowizji::WyslijZoom(const wxString& hex) {
    xthermOdswiez_ = true;   // po poleceniu (migawka, zmiana trybu) biblioteka ma ponownie wczytać kalibrację z klatki
    // jedna wartość albo ciąg oddzielony spacjami lub przecinkami, np. "8004 8020 8000" (sekwencja inicjalizacji opisana dla kamer HT-301)
    std::vector<unsigned> wartosci;
    wxString h = hex;
    h.Replace(",", " ");
    wxStringTokenizer tok(h, " \t");
    while (tok.HasMoreTokens()) {
        unsigned long v = 0;
        if (!tok.GetNextToken().ToULong(&v, 16) || v > 0xFFFF) { wartosci.clear(); break; }
        wartosci.push_back(static_cast<unsigned>(v));
    }
    if (wartosci.empty()) {
        komunikatPolecenia_ = TR("Wartość polecenia musi być liczbą szesnastkową do FFFF (można podać kilka, oddzielonych spacją).");
        return komunikatPolecenia_;
    }
    komunikatPolecenia_.clear();
    for (size_t n = 0; n < wartosci.size(); ++n) {
        std::string k;
        const bool ok = kamera_.wyslijZoom(wartosci[n], k);
        komunikatPolecenia_ += (ok ? TR("Polecenie wysłane: ") : TR("Polecenie nie powiodło się: ")) + TekstZUTF8(k) + "\n";
        if (!ok) break;
        if (n + 1 < wartosci.size()) wxMilliSleep(400);   // kamera potrzebuje chwili między poleceniami
    }
    raport_->ChangeValue(komunikatPolecenia_);
    return komunikatPolecenia_;
}

bool DialogTermowizji::DodajPunktKalibracji(const wxString& temperaturaC) {
    wxString t = temperaturaC;
    t.Trim().Trim(false);
    t.Replace(",", ".");
    double tc = 0;
    if (uklad_.uklad != Termowizja::Uklad::Surowe16 || klatka_.pusta() || t.empty() || !t.ToCDouble(&tc)) {
        komunikatPolecenia_ = TR("Kalibracja wymaga klatki w trybie surowym i temperatury podanej liczbą.");
        return false;
    }
    const auto dane = Termowizja::daneRadiometryczne(klatka_, uklad_);
    const double s = Termowizja::sredniaSurowych(dane, uklad_.obrazSzer, uklad_.obrazWys, uklad_.obrazSzer / 2, uklad_.obrazWys / 2, 4);
    kal_.dodaj(s, tc);
    komunikatPolecenia_ = wxString::Format(TR("Punkt kalibracji: odczyt %.1f = %.1f °C (punktów: %d)"), s, tc, static_cast<int>(kal_.liczba()));
    Przelicz();
    return true;
}

void DialogTermowizji::WyczyscKalibracje() {
    kal_.wyczysc();
    komunikatPolecenia_.clear();
    Przelicz();
}

wxString DialogTermowizji::ZapiszProbke(const wxString& katalog) {
    if (klatka_.pusta()) return wxString();
    const wxString baza = wxFileName(katalog, "termo_" + wxDateTime::Now().Format("%Y%m%d_%H%M%S")).GetFullPath();
    wxFFile bin(baza + ".bin", "wb");
    if (!bin.IsOpened() || bin.Write(klatka_.dane.data(), klatka_.dane.size()) != klatka_.dane.size()) return wxString();
    bin.Close();
    wxFFile txt(baza + ".txt", "wb");
    if (!txt.IsOpened()) return wxString();
    const std::string r = kamera_.wlasciwosci() + Termowizja::raportDiagnostyczny(klatka_, Dzielnik(), OffsetK()) +
                          "Plik .bin: surowe bajty klatki, wiersz po wierszu.\n";
    std::string r2 = r;
    if (xthermOst_.ok) {
        char b[400];
        std::snprintf(b, sizeof(b),
                      "\nBiblioteka Xtherm: srodek %.2f, min %.2f (%d,%d), max %.2f (%d,%d), FPA %.2f, rdzen %.2f [C]\n",
                      xthermOst_.srodekC, xthermOst_.minC, xthermOst_.xMin, xthermOst_.yMin, xthermOst_.maxC, xthermOst_.xMax,
                      xthermOst_.yMax, xthermOst_.fpaC, xthermOst_.rdzenC);
        r2 += b;
        if (!xthermOst_.wszystkie.empty()) {
            wxFFile tf(baza + "_xtherm.f32", "wb");
            if (tf.IsOpened()) {
                tf.Write(xthermOst_.wszystkie.data(), xthermOst_.wszystkie.size() * sizeof(float));
                r2 += "Plik _xtherm.f32: temperatury z biblioteki, float32, szer x wys klatki (wiersz po wierszu).\n";
            }
        }
    }
    txt.Write(r2.data(), r2.size());
    return baza + ".bin";
}

bool DialogTermowizji::ZapiszObraz(const wxString& plik) {
    return podglad_->Obraz().IsOk() && podglad_->Obraz().SaveFile(plik, wxBITMAP_TYPE_PNG);
}

void DialogTermowizji::DiagnostykaSdk() {
    Konfiguracja& kf = Konfiguracja::pobierz();
    std::string sciezka = kf.pobierz("Termowizja", "sdk_iray", "");
    wxString tekst = TR("Diagnostyka SDK IRay (biblioteka z Twojej kopii, program jej nie zawiera)") + "\n";
    if (sciezka.empty() || !wxFileName::FileExists(wxString::FromUTF8(sciezka))) {
        wxFileDialog fd(this, TR("Wskaż USBSDK.dll (wersja x64)"), wxEmptyString, wxEmptyString,
                        "USBSDK.dll|USBSDK.dll|DLL (*.dll)|*.dll", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (fd.ShowModal() != wxID_OK) return;
        sciezka = fd.GetPath().ToStdString(wxConvUTF8);
    }
    InfirayUsb::Sdk sdk;
    std::string blad;
    if (!sdk.zaladuj(sciezka, blad)) {
        raport_->SetValue(tekst + wxString::FromUTF8(blad));
        return;
    }
    kf.ustaw("Termowizja", "sdk_iray", sciezka);
    tekst += TR("Biblioteka załadowana: ") + wxString::FromUTF8(sciezka) + "\n";
    std::vector<std::string> porty;
    const auto urz = sdk.szukaj(&porty);
    if (urz.empty()) {
        raport_->SetValue(tekst + TR("Nie znaleziono urządzeń (kamera podłączona? zamknięta inna aplikacja?)."));
        return;
    }
    for (const auto& u : urz) tekst += wxString::Format("  [%d] ", u.id) + wxString::FromUTF8(u.nazwa) + "\n";
    std::string b2;
    if (!sdk.otworz(urz[0].id, 0, GetHandle(), b2)) {
        raport_->SetValue(tekst + wxString::FromUTF8(b2));
        return;
    }
    wxMilliSleep(800);
    tekst += TR("Typ rdzenia: ") + wxString::FromUTF8(InfirayUsb::opisTypuRdzenia(sdk.typRdzenia())) + "\n";
    std::string pn, sn;
    if (sdk.numeryPNSN(pn, sn)) tekst += "PN: " + wxString::FromUTF8(pn) + "   SN: " + wxString::FromUTF8(sn) + "\n";
    int w = 0, h = 0;
    if (sdk.wymiary(w, h)) tekst += wxString::Format(TR("Matryca: %d x %d") + "\n", w, h);
    float t = 0;
    if (sdk.temperaturaFPA(t)) tekst += wxString::Format(TR("Temperatura detektora (FPA): %.1f") + "\n", t);
    InfirayUsb::Klatka k;
    if (sdk.ostatniaTemp(k)) {
        const auto s = InfirayUsb::slowa16(k);
        uint16_t mn = 65535, mx = 0;
        for (uint16_t v : s) { mn = std::min(mn, v); mx = std::max(mx, v); }
        tekst += wxString::Format(TR("Klatka temperatur: %d x %d, surowe słowa 16-bitowe min %u, max %u, środek %u") + "\n",
                                  k.szer, k.wys, mn, mx, s.empty() ? 0u : s[s.size() / 2]);
        tekst += TR("Skala tych liczb nie jest opisana przez producenta: przelicz ją własną kalibracją (lód 0 °C, dłoń ok. 33 °C).");
    } else {
        tekst += TR("Brak klatki temperatur (kamera bez pomiaru albo za krótki czas oczekiwania).");
    }
    raport_->SetValue(tekst);
}

void DialogTermowizji::WybierzXthermDll() {
    Konfiguracja& kf = Konfiguracja::pobierz();
    wxFileDialog fd(this, TR("Wskaż XthermDll.dll (wersja x64)"), wxEmptyString, wxEmptyString,
                    "XthermDll.dll|XthermDll.dll|DLL (*.dll)|*.dll", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    const std::string stara = kf.pobierz("Termowizja", "xtherm_dll", "");
    if (!stara.empty()) fd.SetPath(wxString::FromUTF8(stara));
    if (fd.ShowModal() != wxID_OK) return;
    const std::string sc = fd.GetPath().ToStdString(wxConvUTF8);
    std::string bl;
    if (!xtherm_.zaladuj(sc, bl)) {
        wxMessageBox(wxString::FromUTF8(bl), TR("Kamera termowizyjna"), wxOK | wxICON_ERROR, this);
        return;
    }
    kf.ustaw("Termowizja", "xtherm_dll", sc);
    xthermStan_ = wxString::FromUTF8(sc);
    xthermOdswiez_ = true;
    wxMessageBox(TR("Biblioteka załadowana. Dla klatek 384x292 (tryb surowy 0x8004 albo YUYV 0x8005) temperatury będą liczone przez bibliotekę producenta."),
                 TR("Kamera termowizyjna"), wxOK | wxICON_INFORMATION, this);
}
