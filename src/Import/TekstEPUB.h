#ifndef TEKSTEPUB_H
#define TEKSTEPUB_H

#include <string>
#include <vector>

/// Czysta logika (bez wx i bazy) do wyciągania tekstu z EPUB: kolejność rozdziałów z OPF i tekst z XHTML.
namespace TekstEPUB {

/// Tekst z (X)HTML: bez znaczników, skryptów i stylów, z rozwiniętymi encjami; akapity oddzielone znakiem nowej linii
std::string tekstZHTML(const std::string& html);

/// Ścieżka pliku w archiwum: @p href względem katalogu @p katalog ("OEBPS", "" dla korzenia); obsługuje "../", "#fragment" i %20
std::string polaczSciezke(const std::string& katalog, const std::string& href);

/// Pliki rozdziałów w kolejności czytania (spine) jako ścieżki w archiwum. @p katalogOPF to katalog pliku OPF
std::vector<std::string> kolejnoscRozdzialow(const std::string& opf, const std::string& katalogOPF);

/// Dzieli długi tekst na kawałki do @p maksZnakow na granicy akapitu (puste kawałki są pomijane)
std::vector<std::string> podzielTekst(const std::string& tekst, size_t maksZnakow = 5000);

} // namespace TekstEPUB

#endif // TEKSTEPUB_H
