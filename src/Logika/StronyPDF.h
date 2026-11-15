#ifndef STRONYPDF_H
#define STRONYPDF_H

#include <string>
#include <vector>

/// Pomocnicze operacje na plikach książek (wymagają wxBase i programu pdftotext z pakietu poppler)
namespace StronyPDF {

/// Tekst kolejnych stron PDF (strony[0] = strona 1) przez `pdftotext`. Pusta lista + @p blad, gdy się nie udało
/// albo plik nie ma warstwy tekstowej (skan).
std::vector<std::string> pobierz(const std::string& sciezka, std::string& blad);

/// Pliki PDF i EPUB w katalogu (rekurencyjnie), posortowane po ścieżce
std::vector<std::string> znajdzPlikiKsiazek(const std::string& katalog);

/// Polecenie otwierające PDF na stronie w znanych przeglądarkach (puste = nieznane): zwraca listę argumentów
/// dla programu o podanej nazwie pliku wykonywalnego (np. "evince" -> {"evince", "-p", "12", plik})
std::vector<std::string> argumentyOtwarciaNaStronie(const std::string& program, const std::string& sciezka, int strona);

} // namespace StronyPDF

#endif // STRONYPDF_H
