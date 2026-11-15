#ifndef PANELKATALOGU_H
#define PANELKATALOGU_H

#include "../Logika/LogikaKatalogu.h"
#include "SortowanieList.h"
#include "../Logika/LogikaMagazynu.h"

#include <wx/wx.h>
#include "OcrPliku.h"
#include "../Wyszukiwanie/Mouser.h"
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/html/htmlwin.h>
#include <wx/statbmp.h>
#include <wx/srchctrl.h>

#include <functional>
#include <memory>
#include <utility>
#include <optional>
#include <vector>

/**
 * @class PanelKatalogu
 * @brief Zakładka "Katalog": katalog elementów elektronicznych (niezależny od stanów magazynowych),
 *        powiązany z magazynem przez symbol. Lista z wyszukiwaniem po lewej, szczegóły i parametry
 *        po prawej, import/eksport CSV, dodawanie do magazynu i odczyt parametrów z datasheetu.
 */
class PanelKatalogu : public wxPanel {
public:
    PanelKatalogu(wxWindow* parent, LogikaMagazynu& magazyn, LogikaKatalogu& katalog);

    void Odswiez();
    void Wyczysc();

    void Nowa();
    void Edytuj();
    /// Usuwa zaznaczone pozycje katalogu; zwraca liczbę usuniętych
    int Usun(bool bezPytan = false);
    int LiczbaZaznaczonych() const;
    void ZaznaczWszystkie();
    bool ZdjeciePokazane() const { return zdjeciePokazane_; }
    /// HTML parametrów zaznaczonej pozycji (do testów)
    wxString HtmlParametrow() const { return htmlParametrow_; }
    void DodajDoMagazynu();
    /// Importuje CSV; @p sciezka pusta = zapytaj o plik. Zwraca liczbę dodanych + zaktualizowanych (-1 = błąd)
    int ImportujCSV(const wxString& sciezka = wxEmptyString, TrybKatalogu tryb = TrybKatalogu::Pomin);
    bool EksportujCSV(const wxString& sciezka = wxEmptyString);
    bool ZapiszSzablon(const wxString& sciezka = wxEmptyString);
    void OtworzDatasheet();
    /// Pobieranie datasheetu: przy ustawionym kluczu Mouser wyszukuje część w Mouserze i pobiera jej PDF, w przeciwnym razie pyta o adres pliku
    void PobierzDatasheet();
    /// Wpisuje do zaznaczonej pozycji dane wybranej części z Mousera: uzupełnia puste pola (producent, opis, obudowa) i adres datasheetu.
    /// Nie pobiera pliku. Zwraca true, gdy zapisano.
    bool ZastosujWynikMouser(const Mouser::Czesc& czesc);
    /// Czyta parametry z PDF-a (pdftotext) i proponuje je do zapisu. @p sciezka pusta = plik pobrany lub zapytaj.
    /// @p bezPytania: zapisuje wszystkie propozycje bez okna (testy). Zwraca liczbę zapisanych parametrów (-1 = błąd).
    int OdczytajDatasheet(const wxString& sciezka = wxEmptyString, bool bezPytania = false, bool bezOcr = false);   ///< bezOcr: nie proponuj OCR (po OCR-ze albo gdy już go wykonano)

    /// OCR (ocrmypdf) pobranych datasheetów zaznaczonych pozycji: skan dostaje warstwę tekstową, dzięki czemu działa "Odczytaj parametry".
    /// @p tryb: wynik zastępuje plik (oryginał jako "plik.pdf.bak") albo trafia obok jako "plik_ocr.pdf". Zwraca liczbę przetworzonych plików (-1: brak programu, nic do zrobienia albo anulowano).
    int OcrDatasheetow(OcrPliku::Tryb tryb = OcrPliku::Tryb::Pytaj, bool bezPytan = false);
    /// Okno słownika opisów parametrów (tabela w bazie); po zamknięciu dymki są odświeżane
    void OtworzSlownikParametrow();
    /// Menu podręczne listy (prawy przycisk): usuń z katalogu, OCR datasheetu
    std::unique_ptr<wxMenu> MenuKontekstowe();

    void Szukaj(const std::string& tekst);
    bool MaWybor() const { return Wybrana() != nullptr; }
    const PozycjaKatalogu* Wybrana() const;
    void Wybierz(long long id);
    /// Czyści filtry (szukanie, rodzina, "tylko w magazynie"), odświeża listę i zaznacza pozycję
    void PokazPozycje(long long id);
    const std::vector<PozycjaKatalogu>& Lista() const { return lista_; }
    /// Jak kliknięcie nagłówka kolumny: sortuje po niej, kolejne wywołanie odwraca kierunek.
    void SortujWedlug(int kolumna) { sort_.Klik(kolumna); Odswiez(); }
    wxString NaglowekKolumny(int kolumna) const;

    std::function<void(const wxString&)> naStatus;
    std::function<void(const wxString&)> naKomunikat;
    std::function<void()> naZmianaMagazynu;

private:
    enum { ID_K_NOWA = wxID_HIGHEST + 500, ID_K_EDYTUJ, ID_K_USUN, ID_K_DO_MAGAZYNU, ID_K_IMPORT, ID_K_EKSPORT,
           ID_K_SZABLON, ID_K_DS_OTWORZ, ID_K_DS_POBIERZ, ID_K_DS_ODCZYT, ID_K_DS_OCR, ID_K_SLOWNIK,
           ID_K_MENU_USUN, ID_K_MENU_OCR };
    enum Kol { K_SYMBOL, K_PRODUCENT, K_OPIS, K_OBUDOWA, K_RODZINA, K_MAGAZYN, K_LICZBA };

    LogikaMagazynu& magazyn_;
    LogikaKatalogu& katalog_;
    std::vector<PozycjaKatalogu> lista_;
    SortowanieList::Stan sort_;
    wxSearchCtrl* szukaj_ = nullptr;
    wxChoice* chRodzina_ = nullptr;
    wxCheckBox* cbMagazyn_ = nullptr;
    wxListCtrl* widok_ = nullptr;
    class HtmlZDymkami* parametry_ = nullptr;   ///< parametry z dymkami opisów
    wxStaticBitmap* zdjecie_ = nullptr;
    bool zdjeciePokazane_ = false;
    wxString htmlParametrow_;
    wxStaticText* naglowek_ = nullptr;
    wxStaticText* szczegoly_ = nullptr;
    wxStaticText* licznik_ = nullptr;
    std::vector<std::string> rodziny_;
    bool odswiezanie_ = false;

    /// Zaznaczone pozycje, które mają pobrany plik PDF datasheetu, z jego ścieżką
    std::vector<std::pair<PozycjaKatalogu, wxString>> WybraneZDatasheetem() const;
    /// Pobiera PDF z @p url do katalogu datasheetów. Gdy się nie uda i @p otworzPrzyBledzie, otwiera adres w przeglądarce
    /// (część serwisów, m.in. dystrybutorzy, blokuje automatyczne pobieranie) i podaje, gdzie zapisać plik.
    void PobierzZAdresu(long long id, const std::string& url, const std::string& symbol, bool otworzPrzyBledzie);
    void WypelnijRodziny();
    void PokazSzczegoly();
    wxString KatalogDokumentow() const;
    void Komunikat(const wxString& t) const { if (naKomunikat) naKomunikat(t); }
    void Status(const wxString& t) const { if (naStatus) naStatus(t); }
    void Blad(const wxString& t);
};

#endif // PANELKATALOGU_H
