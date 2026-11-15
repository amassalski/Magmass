#include "UstawieniaOkna.h"

#include <wx/config.h>
#include <wx/display.h>
#include <wx/frame.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>

#include <cstdlib>

namespace UstawieniaOkna {

namespace {
wxConfigBase* Konf() {
    static bool ustawiono = false;
    if (!ustawiono) {
        ustawiono = true;
        wxConfigBase::Get()->SetRecordDefaults(false);
    }
    return wxConfigBase::Get();
}
wxString Sciezka(const wxString& klucz) { return "/Okno/" + klucz; }
}  // namespace

bool Wlaczone() {
    const char* v = std::getenv("DM_BEZ_USTAWIEN");
    return !(v && *v && *v != '0');
}

int WczytajInt(const wxString& klucz, int domyslna) {
    if (!Wlaczone()) return domyslna;
    long w = domyslna;
    Konf()->Read(Sciezka(klucz), &w, domyslna);
    return static_cast<int>(w);
}

void ZapiszInt(const wxString& klucz, int wartosc) {
    if (!Wlaczone()) return;
    Konf()->Write(Sciezka(klucz), static_cast<long>(wartosc));
    Konf()->Flush();
}

void ZapiszKolumny(wxListCtrl* lista, const wxString& klucz) {
    if (!Wlaczone() || !lista) return;
    wxString s;
    for (int i = 0; i < lista->GetColumnCount(); ++i) {
        if (i) s += ",";
        s += wxString::Format("%d", lista->GetColumnWidth(i));
    }
    Konf()->Write(Sciezka("Kolumny/" + klucz), s);
    Konf()->Flush();
}

void WczytajKolumny(wxListCtrl* lista, const wxString& klucz) {
    if (!Wlaczone() || !lista) return;
    wxString s;
    if (!Konf()->Read(Sciezka("Kolumny/" + klucz), &s) || s.empty()) return;
    wxArrayString czesci = wxSplit(s, ',');
    if (static_cast<int>(czesci.size()) != lista->GetColumnCount()) return;  // zmienił się układ kolumn
    for (size_t i = 0; i < czesci.size(); ++i) {
        long w = 0;
        if (czesci[i].ToLong(&w) && w >= 20 && w <= 2000) lista->SetColumnWidth(static_cast<int>(i), static_cast<int>(w));
    }
}

void ZapiszPodzial(wxSplitterWindow* podzial, const wxString& klucz) {
    if (!podzial || !podzial->IsSplit()) return;
    // dla podziału pionowego zapamiętujemy szerokość lewej części
    ZapiszInt("Podzial/" + klucz, podzial->GetSashPosition());
}

void WczytajPodzial(wxSplitterWindow* podzial, const wxString& klucz) {
    if (!podzial || !podzial->IsSplit()) return;
    const int p = WczytajInt("Podzial/" + klucz, -1);
    if (p <= 0) return;
    const int calosc = podzial->GetSplitMode() == wxSPLIT_VERTICAL ? podzial->GetClientSize().x : podzial->GetClientSize().y;
    const int minimum = podzial->GetMinimumPaneSize();
    if (calosc > 0 && p > minimum && p < calosc - minimum) podzial->SetSashPosition(p);
}

void ZapiszRamke(wxFrame* ramka, const wxString& klucz) {
    if (!Wlaczone() || !ramka) return;
    const bool max = ramka->IsMaximized();
    ZapiszInt(klucz + "/Maks", max ? 1 : 0);
    if (max || ramka->IsIconized()) return;   // rozmiar sprzed maksymalizacji zostaje
    const wxPoint p = ramka->GetPosition();
    const wxSize s = ramka->GetSize();
    ZapiszInt(klucz + "/X", p.x);
    ZapiszInt(klucz + "/Y", p.y);
    ZapiszInt(klucz + "/W", s.x);
    ZapiszInt(klucz + "/H", s.y);
}

void WczytajRamke(wxFrame* ramka, const wxString& klucz) {
    if (!Wlaczone() || !ramka) return;
    const int w = WczytajInt(klucz + "/W", 0), h = WczytajInt(klucz + "/H", 0);
    if (w < 600 || h < 400) return;
    const int x = WczytajInt(klucz + "/X", 0), y = WczytajInt(klucz + "/Y", 0);
    // położenie musi mieścić się na którymś z podłączonych ekranów (monitor mógł zostać odłączony)
    const wxRect r(x, y, w, h);
    bool widoczne = false;
    for (unsigned i = 0; i < wxDisplay::GetCount(); ++i) {
        wxRect przeciecie = wxDisplay(i).GetClientArea().Intersect(r);
        if (przeciecie.width >= 200 && przeciecie.height >= 100) { widoczne = true; break; }
    }
    if (!widoczne) return;
    ramka->SetSize(x, y, w, h);
    if (WczytajInt(klucz + "/Maks", 0)) ramka->Maximize(true);
}

}  // namespace UstawieniaOkna
