#include "KsiazkiCSV.h"
#include "../Import/ParserCSV.h"

#include <cctype>
#include <cstdlib>
#include <map>

namespace KsiazkiCSV {
namespace {

std::string przytnij(const std::string& s) {
    const char* sp = " \t\r\n";
    const size_t a = s.find_first_not_of(sp);
    if (a == std::string::npos) return std::string();
    return s.substr(a, s.find_last_not_of(sp) - a + 1);
}

/// Nagłówek bez polskich znaków i znaków specjalnych, małymi literami
std::string klucz(const std::string& n) {
    static const std::pair<const char*, char> pl[] = {
        {"ą", 'a'}, {"ć", 'c'}, {"ę", 'e'}, {"ł", 'l'}, {"ń", 'n'}, {"ó", 'o'}, {"ś", 's'}, {"ź", 'z'}, {"ż", 'z'},
        {"Ą", 'a'}, {"Ć", 'c'}, {"Ę", 'e'}, {"Ł", 'l'}, {"Ń", 'n'}, {"Ó", 'o'}, {"Ś", 's'}, {"Ź", 'z'}, {"Ż", 'z'}};
    std::string w;
    for (size_t i = 0; i < n.size();) {
        bool zam = false;
        for (const auto& [z, c] : pl) {
            const std::string zz = z;
            if (n.compare(i, zz.size(), zz) == 0) { w += c; i += zz.size(); zam = true; break; }
        }
        if (zam) continue;
        const unsigned char c = static_cast<unsigned char>(n[i++]);
        if (std::isalnum(c) && c < 0x80) w += static_cast<char>(std::tolower(c));
    }
    return w;
}

enum class Pole { Tytul, Autor, Wydawca, Rok, Wydanie, Isbn, Jezyk, Tagi, Sciezka, Uwagi };

const std::map<std::string, Pole>& aliasy() {
    static const std::map<std::string, Pole> m = {
        {"tytul", Pole::Tytul}, {"title", Pole::Tytul}, {"nazwa", Pole::Tytul},
        {"autor", Pole::Autor}, {"autorzy", Pole::Autor}, {"author", Pole::Autor}, {"authors", Pole::Autor},
        {"wydawca", Pole::Wydawca}, {"wydawnictwo", Pole::Wydawca}, {"publisher", Pole::Wydawca},
        {"rok", Pole::Rok}, {"rokwydania", Pole::Rok}, {"year", Pole::Rok},
        {"wydanie", Pole::Wydanie}, {"edition", Pole::Wydanie},
        {"isbn", Pole::Isbn},
        {"jezyk", Pole::Jezyk}, {"language", Pole::Jezyk}, {"lang", Pole::Jezyk},
        {"tagi", Pole::Tagi}, {"tags", Pole::Tagi}, {"tematyka", Pole::Tagi},
        {"sciezka", Pole::Sciezka}, {"plik", Pole::Sciezka}, {"path", Pole::Sciezka}, {"file", Pole::Sciezka},
        {"uwagi", Pole::Uwagi}, {"notes", Pole::Uwagi}, {"komentarz", Pole::Uwagi}};
    return m;
}

std::string cudzyslow(const std::string& s) {
    if (s.find_first_of(";\"\n\r") == std::string::npos) return s;
    std::string w = "\"";
    for (char c : s) { if (c == '"') w += '"'; w += c; }
    return w + "\"";
}

const char* NAGLOWEK = "Tytuł;Autor;Wydawca;Rok;Wydanie;ISBN;Język;Tagi;Ścieżka;Uwagi";

}  // namespace

Wynik parsuj(const std::string& tekst) {
    Wynik wynik;
    const auto wiersze = ParserCSV::parsuj(tekst);
    if (wiersze.empty()) { wynik.blad = "Plik jest pusty."; return wynik; }
    std::map<size_t, Pole> kolumny;
    bool jestTytul = false;
    for (size_t i = 0; i < wiersze[0].size(); ++i) {
        auto it = aliasy().find(klucz(przytnij(wiersze[0][i])));
        if (it == aliasy().end()) continue;
        kolumny[i] = it->second;
        jestTytul = jestTytul || it->second == Pole::Tytul;
    }
    if (!jestTytul) { wynik.blad = "W pierwszym wierszu brakuje nagłówka \"Tytuł\". Użyj szablonu CSV jako wzoru."; return wynik; }
    for (size_t w = 1; w < wiersze.size(); ++w) {
        Ksiazka k;
        for (size_t c = 0; c < wiersze[w].size(); ++c) {
            auto it = kolumny.find(c);
            if (it == kolumny.end()) continue;
            const std::string v = przytnij(wiersze[w][c]);
            if (v.empty()) continue;
            switch (it->second) {
                case Pole::Tytul: k.tytul = v; break;
                case Pole::Autor: k.autor = v; break;
                case Pole::Wydawca: k.wydawca = v; break;
                case Pole::Rok: k.rok = std::atoi(v.c_str()); break;
                case Pole::Wydanie: k.wydanie = v; break;
                case Pole::Isbn: k.isbn = v; break;
                case Pole::Jezyk: k.jezyk = v; break;
                case Pole::Tagi: k.tagi = v; break;
                case Pole::Sciezka: k.sciezka = v; break;
                case Pole::Uwagi: k.uwagi = v; break;
            }
        }
        if (k.tytul.empty()) { wynik.ostrzezenia.push_back("Wiersz " + std::to_string(w + 1) + ": brak tytułu"); continue; }
        wynik.ksiazki.push_back(std::move(k));
    }
    return wynik;
}

std::string naCSV(const std::vector<Ksiazka>& ksiazki) {
    std::string w = std::string("\xEF\xBB\xBF") + NAGLOWEK + "\n";
    for (const Ksiazka& k : ksiazki)
        w += cudzyslow(k.tytul) + ";" + cudzyslow(k.autor) + ";" + cudzyslow(k.wydawca) + ";" + (k.rok > 0 ? std::to_string(k.rok) : "") + ";" + cudzyslow(k.wydanie) + ";" +
             cudzyslow(k.isbn) + ";" + cudzyslow(k.jezyk) + ";" + cudzyslow(k.tagi) + ";" + cudzyslow(k.sciezka) + ";" +
             cudzyslow(k.uwagi) + "\n";
    return w;
}

std::string szablon() {
    return std::string("\xEF\xBB\xBF") + NAGLOWEK + "\n"
           "Sztuka elektroniki;Paul Horowitz, Winfield Hill;WKŁ;2020;wydanie czwarte;978-83-206-1777-1;polski;wzmacniacze, analog;;Uwagi o książce\n"
           "The Art of Electronics;Paul Horowitz, Winfield Hill;Cambridge University Press;2015;3rd edition;;angielski;analog;D:\\Ksiazki\\aoe.pdf;\n";
}

} // namespace KsiazkiCSV
