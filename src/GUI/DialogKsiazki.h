#ifndef DIALOGKSIAZKI_H
#define DIALOGKSIAZKI_H

#include "../Import/MetadaneKsiazek.h"
#include "../Logika/LogikaKsiazek.h"

#include <wx/wx.h>
#include <wx/html/htmlwin.h>

#include <memory>
#include <vector>

/**
 * @class DialogKsiazki
 * @brief Formularz nowej / zmienianej książki. Tryb zbiorczy (@p liczbaWybranych > 1): pola indywidualne
 *        (tytuł, rok, ISBN, język, plik, uwagi) są wyszarzone i zablokowane; można zmienić tylko autora, wydawcę i tagi
 *        (każde pole włącza się swoim polem wyboru).
 *        Przycisk "Pobierz z Biblioteki Narodowej" wypełnia pola danymi z API BN (po ISBN albo tytule i autorze),
 *        pole Uwagi otwiera edytor HTML po dwukliku.
 */
class DialogKsiazki : public wxDialog {
public:
    DialogKsiazki(wxWindow* parent, const Ksiazka& k, int liczbaWybranych = 1);
    ~DialogKsiazki() override;

    const Ksiazka& Wynik() const { return ksiazka_; }
    /// Zmiany do zastosowania dla wielu książek (tryb zbiorczy)
    ZmianyZbiorczeKsiazek Zmiany() const;
    bool TrybZbiorczy() const { return zbiorczo_; }

    /// Wpisuje dane (np. z Biblioteki Narodowej albo z pliku) w pola formularza: nadpisuje pola, dla których dane mają wartość
    void UstawDane(const MetadaneKsiazek::Dane& d);
    /// Wkleja do pola roku liczbę wyciętą z tekstu; gdy w tekście nie ma roku, pole się nie zmienia. Zwraca true, gdy wklejono.
    bool WklejRok(const wxString& tekst);
    /// Edytor uwag w HTML (otwierany też dwuklikiem w polu Uwagi)
    bool EdytujUwagi();
    /// Odpowiedź BN (JSON) zastosowana do pól; przy wielu trafieniach pyta o wybór. Zwraca liczbę trafień.
    int ZastosujOdpowiedzBN(const std::string& json, bool pytajOWybor = true);
    /// Zapytanie do API Biblioteki Narodowej (asynchroniczne)
    void PobierzZBN();

    wxTextCtrl* PoleRoku() const { return tcRok_; }
    wxComboBox* PoleJezyka() const { return cbJezyk_; }
    /// Okno z uwagami pokazanymi jako HTML (po dwukliku otwiera się edytor)
    wxHtmlWindow* PodgladUwag() const { return podgladUwag_; }
    /// Uwagi w postaci źródłowej (tekst albo HTML)
    const wxString& Uwagi() const { return uwagi_; }
    void UstawUwagi(const wxString& zrodlo);
    wxTextCtrl* PoleWydania() const { return tcWydanie_; }
    wxTextCtrl* PoleAutora() const { return tcAutor_; }
    wxTextCtrl* PoleTytulu() const { return tcTytul_; }
    wxTextCtrl* PoleTagow() const { return tcTagi_; }
    wxTextCtrl* PoleWydawcy() const { return tcWydawca_; }
    wxTextCtrl* PoleIsbn() const { return tcIsbn_; }
    wxCheckBox* ZmianaAutora() const { return cbAutor_; }
    wxCheckBox* ZmianaWydawcy() const { return cbWydawca_; }
    wxCheckBox* ZmianaTagow() const { return cbTagi_; }

private:
    Ksiazka ksiazka_;
    bool zbiorczo_ = false;
    wxTextCtrl *tcTytul_ = nullptr, *tcAutor_ = nullptr, *tcWydawca_ = nullptr, *tcRok_ = nullptr, *tcIsbn_ = nullptr,
               *tcTagi_ = nullptr, *tcSciezka_ = nullptr, *tcWydanie_ = nullptr;
    wxHtmlWindow* podgladUwag_ = nullptr;
    wxString uwagi_;
    wxComboBox* cbJezyk_ = nullptr;
    wxCheckBox *cbAutor_ = nullptr, *cbWydawca_ = nullptr, *cbTagi_ = nullptr;
    wxChoice* chTagi_ = nullptr;
    wxButton* bBN_ = nullptr;
    std::shared_ptr<bool> zyje_ = std::make_shared<bool>(true);

    static std::string T(const wxTextCtrl* c) { return c->GetValue().Strip(wxString::both).utf8_string(); }
    int Rok() const;
    void OdczytajZPliku(bool komunikat);
    void NaOK(wxCommandEvent& e);
};

#endif // DIALOGKSIAZKI_H
