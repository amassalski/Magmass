#ifndef INDEKSY_H
#define INDEKSY_H

#include <string>

/**
 * Zapis indeksów dolnych i górnych w tekście parametrów (bez wxWidgets):
 *   V_CE, h_FE, I_C          indeks dolny do końca ciągu liter i cyfr
 *   V_{CE(sat)}              indeks dolny w nawiasach klamrowych
 *   m^2, 10^-3, 10^{-6}      indeks górny (liczba ze znakiem albo ciąg liter i cyfr)
 *   \_  \^  \\               znak specjalny wpisany dosłownie
 */
namespace Indeksy {

/// HTML 4 z <sub> i <sup>; pozostałe znaki są zabezpieczone (&amp; &lt; &gt; &quot;)
std::string naHtml(const std::string& tekst);
/// Zwykły tekst: indeksy zapisane znakami Unicode (₀-₉, ᵢ, ⁻, ²...), a gdy dla jakiegoś znaku brak odpowiednika,
/// indeks pozostaje zwykłym tekstem (bez znaczników _ i ^)
std::string naTekst(const std::string& tekst);
/// Czy tekst zawiera znaczniki indeksów
bool maIndeksy(const std::string& tekst);

} // namespace Indeksy

#endif // INDEKSY_H
