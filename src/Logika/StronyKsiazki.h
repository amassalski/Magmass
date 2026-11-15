#ifndef STRONYKSIAZEK_H
#define STRONYKSIAZEK_H

#include <string>
#include <vector>

/// Tekst książki do indeksu pełnotekstowego: PDF (strony przez pdftotext) i EPUB (rozdziały i ich fragmenty z archiwum ZIP).
namespace StronyKsiazki {

/// Czy plik (po rozszerzeniu) da się zaindeksować: pdf albo epub
bool czyMoznaIndeksowac(const std::string& sciezka);
bool czyEpub(const std::string& sciezka);

/// Kolejne "strony": dla PDF prawdziwe strony, dla EPUB kawałki tekstu rozdziałów w kolejności czytania (do ok. 5000 znaków).
/// Pusta lista + @p blad, gdy się nie udało albo nie ma tekstu.
std::vector<std::string> pobierz(const std::string& sciezka, std::string& blad);

} // namespace StronyKsiazki

#endif // STRONYKSIAZEK_H
