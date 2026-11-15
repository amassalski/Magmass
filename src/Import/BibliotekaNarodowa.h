#ifndef BIBLIOTEKANARODOWA_H
#define BIBLIOTEKANARODOWA_H

#include "MetadaneKsiazek.h"

#include <string>
#include <vector>

/**
 * Dane książek z API Biblioteki Narodowej (https://data.bn.org.pl/api/networks/bibs.json).
 * Moduł nie łączy się z siecią: buduje adres zapytania i parsuje odpowiedź JSON; pobieranie robi KlientHTTP.
 */
namespace BibliotekaNarodowa {

/// ISBN bez myślników i spacji (cyfry i końcowe X); pusty, gdy tekst nie wygląda na ISBN-10/13
std::string czystyISBN(const std::string& tekst);

/// Adres zapytania: po ISBN (gdy poprawny) albo po tytule i autorze; @p limit - liczba wyników
std::string adresZapytania(const std::string& isbn, const std::string& tytul, const std::string& autor, int limit = 10);

/// Książki z odpowiedzi JSON (pola "title", "author", "publisher", "publicationYear", "language", "isbnIssn" albo rekord MARC).
/// Zwraca false przy błędnym JSON; pusty wynik oznacza brak trafień.
bool zOdpowiedzi(const std::string& json, std::vector<MetadaneKsiazek::Dane>& wynik, std::string& blad);

/// Rok z dowolnego tekstu (np. "©2019", "wyd. 2, 2015 r.", "[2004]"): pierwsza czterocyfrowa liczba 1000-2999;
/// 0, gdy w tekście nie ma takiej liczby
int rokZTekstu(const std::string& tekst);

}  // namespace BibliotekaNarodowa

#endif // BIBLIOTEKANARODOWA_H
