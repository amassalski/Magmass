#ifndef POLECENIEKSIAZEK_H
#define POLECENIEKSIAZEK_H

#include "../Model/Ksiazka.h"

#include <string>
#include <vector>

/**
 * Budowanie poleceń dla programów zewnętrznych obsługujących pliki książek (bez wxWidgets):
 *   ExifTool  - zapis metadanych z bazy do pliku PDF (XMP i słownik Info), spójny z odczytem w MetadaneKsiazek
 *   ocrmypdf  - rozpoznawanie tekstu (OCR) w zeskanowanych PDF
 * Wartości trafiają do pliku argumentów ExifTool (-@), nie do wiersza poleceń, więc polskie znaki i cudzysłowy
 * nie zależą od powłoki ani od strony kodowej systemu.
 */
namespace PolecenieKsiazek {

/// "polski" -> "pl", "angielski" -> "en"; kody dwuliterowe bez zmian; "" dla nieznanego języka
std::string kodJezyka(const std::string& nazwa);

/// Linie pliku argumentów ExifTool (po jednym argumencie w linii, UTF-8), na końcu ścieżka pliku.
/// Zapisywane są tylko niepuste pola książki (puste nie kasują istniejących danych w pliku).
std::vector<std::string> liniePlikuExifTool(const Ksiazka& k, const std::string& sciezkaPliku);

/// Argumenty programu exiftool: `-charset utf8 -charset filename=utf8 [-overwrite_original] -@ <plik>`
/// (@p zachowajKopie: ExifTool zostawia obok oryginału plik "<nazwa>.pdf_original")
std::vector<std::string> argumentyExifTool(const std::string& plikArgumentow, bool zachowajKopie);

/// "C:\\Ks\\a.pdf" -> "C:\\Ks\\a_ocr.pdf"
std::string sciezkaWynikuOcr(const std::string& plik);

/// Języki Tesseracta: litery, cyfry, "_" i "+" (np. "pol+eng"); chroni przed wstrzyknięciem argumentów
bool poprawneJezykiOcr(const std::string& jezyki);

/// Argumenty programu ocrmypdf (bez nazwy programu); strony z tekstem są pomijane (@p pominTekst)
std::vector<std::string> argumentyOcr(const std::string& wejscie, const std::string& wyjscie, const std::string& jezyki, bool pominTekst = true);

/// Ścieżka pliku zakodowana do adresu URL (bez schematu): "/Users/Jan/Moje ksiązki/a.pdf" -> "/Users/Jan/Moje%20ksi%C4%85%C5%BCki/a.pdf";
/// ścieżki Windows ("C:\\Ks\\a.pdf") dostają postać "/C:/Ks/a.pdf" (po dopisaniu "file://" daje poprawny adres)
std::string kodujSciezkeURL(const std::string& sciezka);

/// Wstawia do polecenia czytnika PDF: %f = ścieżka pliku, %u = ścieżka zakodowana do URL (np. "skim://%u#page=%p"), %p = numer strony
std::string wypelnijPolecenieCzytnika(const std::string& szablon, const std::string& sciezka, int strona);

/// Dopisuje do listy katalogów PATH (rozdzielonej @p separator) te z @p dodatkowe, których jeszcze nie ma
std::string rozszerzPATH(const std::string& path, const std::vector<std::string>& dodatkowe, char separator = ':');

} // namespace PolecenieKsiazek

#endif // POLECENIEKSIAZEK_H
