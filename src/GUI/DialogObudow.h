#ifndef DIALOGOBUDOW_H
#define DIALOGOBUDOW_H

#include "../Logika/LogikaObudow.h"

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/srchctrl.h>

#include <optional>
#include <vector>

/// Słownik obudów: atrybuty (moc strat, napięcie, rozstaw, liczba pinów, rodzina, wariant),
/// import z bibliotek Eagle (.lbr) i uzupełnienie z tekstów obudów używanych w elementach
class DialogObudow : public wxDialog {
public:
    DialogObudow(wxWindow* parent, LogikaObudow& logika);

    void Odswiez();
    const std::vector<Obudowa>& Obudowy() const { return obudowy_; }
    /// Operacje z przycisków (publiczne dla testów); zwracają, czy coś zmieniły
    bool Dodaj();
    bool Edytuj();
    bool Usun();                       ///< usuwa wszystkie zaznaczone obudowy (zaznaczenie wielokrotne: Ctrl/Shift)
    /// Zbiorcza zmiana atrybutów zaznaczonych obudów; pole bez wartości (nullopt) zostaje bez zmian. Zwraca liczbę zmienionych (-1 = błąd)
    struct ZmianyZbiorcze {
        std::optional<std::string> rodzina, montaz, rozstaw;
        std::optional<double> mocStratW, napiecieMaxV;
        bool czyCos() const { return rodzina || montaz || rozstaw || mocStratW || napiecieMaxV; }
    };
    int ZmienZaznaczone(const ZmianyZbiorcze& zmiany);
    void Wybierz(const std::vector<long long>& id);   ///< zaznacza wiele obudów
    size_t LiczbaZaznaczonych() const;
    /// Wczytuje biblioteki .lbr; @p pliki puste = zapytaj o pliki. Zwraca liczbę dodanych i uzupełnionych obudów (-1 = błąd)
    int ImportujBiblioteki(const wxArrayString& pliki = wxArrayString());
    int UzupelnijZElementow();
    void Wybierz(long long id);
    /// Tekst listy "Typowe elementy" zaznaczonej obudowy (do testów)
    wxString TekstTypowych() const { return typowe_->GetValue(); }

private:
    LogikaObudow& logika_;
    std::vector<Obudowa> obudowy_;
    wxTextCtrl* typowe_ = nullptr;
    void OdswiezTypowe();
    wxListCtrl* lista_ = nullptr;
    wxSearchCtrl* szukaj_ = nullptr;
    wxStaticText* licznik_ = nullptr;

    const Obudowa* Wybrana() const;
    std::vector<const Obudowa*> Zaznaczone() const;
    bool EdytujZbiorczo();
    bool Edycja(Obudowa obudowa);
};

/// Wybór obudów elementu (relacja 1-wiele): lista obudów ze słownika z polem wyszukiwania i znacznikami;
/// przycisk "Słownik obudów..." otwiera pełny słownik (import z Eagle, nowe obudowy).
class DialogWyboruObudow : public wxDialog {
public:
    DialogWyboruObudow(wxWindow* parent, LogikaObudow& logika, const std::vector<std::string>& zaznaczone);

    /// Zaznaczone nazwy w kolejności wyboru
    std::vector<std::string> Wybrane() const { return wybrane_; }
    /// Dla testów
    void Zaznacz(const std::string& nazwa, bool tak);
    void Szukaj(const wxString& tekst);
    size_t LiczbaPozycji() const { return widoczne_.size(); }

private:
    LogikaObudow& logika_;
    std::vector<std::string> wybrane_;
    std::vector<Obudowa> widoczne_;
    wxSearchCtrl* szukaj_ = nullptr;
    wxCheckListBox* lista_ = nullptr;
    wxStaticText* podsumowanie_ = nullptr;
    void Odswiez();
    void NaZaznaczenie(wxCommandEvent& event);
    void OtworzSlownik();
    void OdswiezPodsumowanie();
};

#endif // DIALOGOBUDOW_H
