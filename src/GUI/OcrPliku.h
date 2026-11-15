#ifndef OCRPLIKU_H
#define OCRPLIKU_H

#include <wx/wx.h>

#include <string>

/**
 * OCR plików PDF programem ocrmypdf, wspólny dla zakładek Literatura i Katalog (datasheety):
 * wyszukanie programu i sprawdzenie języków z ustawień, pytanie o sposób zapisu wyniku i uruchomienie z oknem postępu.
 */
namespace OcrPliku {

/// Gdzie trafia wynik: Pytaj = zapytaj użytkownika; Zastap = zamiast pliku (oryginał zostaje jako "plik.pdf.bak"); Obok = "plik_ocr.pdf"
enum class Tryb { Pytaj, Zastap, Obok };

struct Ustawienia {
    wxString program;      ///< pełna ścieżka ocrmypdf
    std::string jezyki;    ///< np. "pol+eng"
};

/// Wczytuje program i języki z ustawień. Gdy się nie da, zwraca false i wpisuje opis problemu do @p blad.
bool Przygotuj(Ustawienia& ust, wxString& blad);

/// Pyta, czy wynik zastąpi plik, czy trafi obok (@p liczba plików). false = anulowano.
bool WybierzTryb(wxWindow* rodzic, size_t liczba, Tryb& tryb);

enum class Wynik { Ok, Blad, Przerwano };

/// OCR jednego pliku @p sciezka. @p zastap: podmienia plik (oryginał jako ".bak"). @p bezPytan: bez okna postępu.
/// Przy błędzie opis trafia do @p blad. Okno postępu pozwala przerwać pracę.
Wynik Wykonaj(wxWindow* rodzic, const Ustawienia& ust, const std::string& sciezka, const wxString& tytul, size_t nr, size_t razem,
              bool zastap, bool bezPytan, std::string& blad);

} // namespace OcrPliku

#endif // OCRPLIKU_H
