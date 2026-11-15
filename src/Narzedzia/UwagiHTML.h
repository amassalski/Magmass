#ifndef UWAGIHTML_H
#define UWAGIHTML_H

#include <string>

/// Pomocnicze funkcje dla uwag zapisanych jako HTML 4 (bez wxWidgets)
namespace UwagiHTML {

/// Czy tekst zawiera znaczniki HTML (<b>, <p>, <br>, <ul>... lub encje)
bool czyHtml(const std::string& tekst);
/// Zwykły tekst z HTML: <br>, </p>, </li>, </tr> -> nowa linia, <li> -> "- ", pozostałe znaczniki usunięte,
/// encje (&amp; &lt; &gt; &quot; &nbsp; &#NNN;) zamienione na znaki. Tekst bez znaczników zwracany bez zmian.
std::string naTekst(const std::string& html);
/// Zamienia zwykły tekst na HTML (znaki specjalne, nowe linie -> <br>); tekst będący już HTML zwracany bez zmian
std::string zTekstu(const std::string& tekst);

} // namespace UwagiHTML

#endif // UWAGIHTML_H
