#ifndef PANELKSIAZEK_H
#define PANELKSIAZEK_H

#include "../Logika/LogikaKsiazek.h"
#include "SortowanieList.h"

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>

#include <functional>
#include <memory>
#include <vector>

/**
 * @class PanelKsiazek
 * @brief Zakładka "Literatura": książki użytkownika (papierowe i elektroniczne), ścieżka do PDF/EPUB z otwieraniem,
 *        import z CSV i z katalogu na dysku, indeks pełnotekstowy PDF w bazie i wyszukiwanie w treści
 *        z otwieraniem książki na znalezionej stronie.
 */
class PanelKsiazek : public wxPanel {
public:
    PanelKsiazek(wxWindow* parent, LogikaKsiazek& logika);

    void Odswiez();
    void Wyczysc();

    void Nowa();
    void Edytuj();
    void Usun();
    /// Usuwa wszystkie zaznaczone książki (@p bezPytan: bez potwierdzenia); zwraca liczbę usuniętych
    int UsunZaznaczone(bool bezPytan = false);
    /// Zmiana wspólnych pól (autor, wydawca, tagi) zaznaczonych książek
    void EdytujZbiorczo();
    int LiczbaZaznaczonych() const;
    std::vector<long long> ZaznaczoneId() const;
    void ZaznaczWszystkie();
    void ZaznaczZakres(long od, long do_);
    void Pozycz();
    /// Rejestruje zwrot zaznaczonej książki (@p bezPytan: bez potwierdzenia, testy)
    bool Zwroc(bool bezPytan = false);
    /// Wpisuje do dziennika przeterminowane pożyczki książek; zwraca ich liczbę (-1 = błąd)
    int Przypomnij();
    /// Importuje CSV; @p sciezka pusta = zapytaj o plik. Zwraca liczbę dodanych książek (-1 = błąd)
    int ImportujCSV(const wxString& sciezka = wxEmptyString);
    bool EksportujCSV(const wxString& sciezka = wxEmptyString);
    bool ZapiszSzablon(const wxString& sciezka = wxEmptyString);
    /// Dodaje książki ze wszystkich plików PDF i EPUB w katalogu (tytuł z nazwy pliku); @p katalog pusty = zapytaj.
    /// Zwraca liczbę dodanych (-1 = błąd lub anulowano).
    int DodajZKatalogu(const wxString& katalog = wxEmptyString);
    /// Otwiera plik zaznaczonej książki; @p strona > 0 otwiera na tej stronie, o ile program to potrafi
    bool Otworz(int strona = 0);
    /// Indeksuje treść PDF zaznaczonej książki (@p bezPytan: bez okien komunikatów). Zwraca liczbę stron z tekstem (-1 = błąd).
    int Indeksuj(bool bezPytan = false);
    /// Indeksuje wszystkie książki z plikiem PDF, które nie mają jeszcze indeksu. Zwraca liczbę zaindeksowanych książek.
    int IndeksujWszystkie(bool bezPytan = false);
    /// Szuka w treści (@p tylkoZaznaczona: tylko w zaznaczonej książce); wynik w Trafienia()
    int SzukajWTresci(const wxString& tekst, bool tylkoZaznaczona = false);

    /// Zapisuje dane z bazy (tytuł, autor, wydawca, rok, ISBN, język, tagi, uwagi) do plików PDF zaznaczonych książek
    /// programem ExifTool (XMP i słownik Info). Zwraca liczbę zapisanych plików (-1: brak programu albo nic do zrobienia).
    int ZapiszMetadaneDoPliku(bool bezPytan = false);

    enum class TrybOcr { Pytaj, ZastapOryginal, ObokOryginalu };
    /// OCR (ocrmypdf) zaznaczonych PDF. @p tryb: czy wynik zastępuje plik (oryginał zostaje jako "plik.pdf.bak"),
    /// czy trafia obok jako "plik_ocr.pdf". Po zastąpieniu indeks treści jest budowany od nowa.
    /// Zwraca liczbę przetworzonych plików (-1: brak programu, nic do zrobienia albo przerwano).
    int Ocr(TrybOcr tryb = TrybOcr::Pytaj, bool bezPytan = false);

    /// Usuwa indeks treści zaznaczonych książek (@p bezPytan: bez potwierdzenia). Zwraca liczbę książek, którym usunięto indeks.
    int UsunIndeks(bool bezPytan = false);
    /// Menu podręczne listy (prawy przycisk): usuń książkę, usuń indeks, zapisz metadane, OCR, informacje
    std::unique_ptr<wxMenu> MenuKontekstowe();
    /// Karta informacyjna wybranej książki jako HTML: dane z bazy i uwagi (też w HTML)
    wxString TrescInfo() const;
    /// Okno z kartą informacyjną wybranej książki
    void PokazInfo();

    void Szukaj(const std::string& tekst);
    const Ksiazka* Wybrana() const;
    bool MaWybor() const { return Wybrana() != nullptr; }
    void Wybierz(long long id);
    const std::vector<Ksiazka>& Lista() const { return lista_; }
    void SortujWedlug(int kolumna) { sort_.Klik(kolumna); Odswiez(); }
    wxString NaglowekKolumny(int kolumna) const;
    const std::vector<TrafienieKsiazki>& Trafienia() const { return trafienia_; }

    std::function<void(const wxString&)> naStatus;
    std::function<void(const wxString&)> naKomunikat;

private:
    enum { ID_KS_NOWA = wxID_HIGHEST + 700, ID_KS_EDYTUJ, ID_KS_USUN, ID_KS_IMPORT, ID_KS_EKSPORT, ID_KS_SZABLON, ID_KS_KATALOG,
           ID_KS_OTWORZ, ID_KS_INDEKSUJ, ID_KS_INDEKSUJ_WSZYSTKIE, ID_KS_POZYCZ, ID_KS_ZWROC, ID_KS_ZAPISZ_META, ID_KS_OCR,
           ID_KS_MENU_USUN, ID_KS_MENU_USUN_INDEKS, ID_KS_MENU_META, ID_KS_MENU_OCR, ID_KS_MENU_INFO };
    enum Kol { K_TYTUL, K_AUTOR, K_ROK, K_WYDAWCA, K_PLIK, K_STRONY, K_POZYCZONA, K_LICZBA };

    LogikaKsiazek& logika_;
    std::vector<Ksiazka> lista_;
    SortowanieList::Stan sort_;
    std::vector<TrafienieKsiazki> trafienia_;
    wxSearchCtrl* szukaj_ = nullptr;
    wxSearchCtrl* szukajTresci_ = nullptr;
    wxCheckBox* cbZaznaczona_ = nullptr;
    wxListCtrl* widok_ = nullptr;
    wxListCtrl* wyniki_ = nullptr;
    wxStaticText* naglowek_ = nullptr;
    wxStaticText* szczegoly_ = nullptr;
    wxStaticText* licznik_ = nullptr;
    bool odswiezanie_ = false;

    std::vector<Ksiazka> WybraneZPdf() const;
    void PokazSzczegoly();
    void NaWyszukiwanieTresci();
    void Komunikat(const wxString& t) const { if (naKomunikat) naKomunikat(t); }
    void Status(const wxString& t) const { if (naStatus) naStatus(t); }
    void Blad(const wxString& t);
};

#endif // PANELKSIAZEK_H
