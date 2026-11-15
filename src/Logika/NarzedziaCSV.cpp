#include "NarzedziaCSV.h"
#include "../Import/ParserCSV.h"
#include "../Narzedzia/Liczby.h"

#include <cctype>
#include <map>

namespace NarzedziaCSV {
namespace {

std::string przytnij(const std::string& s) {
    const char* sp = " \t\r\n";
    const size_t a = s.find_first_not_of(sp);
    if (a == std::string::npos) return std::string();
    return s.substr(a, s.find_last_not_of(sp) - a + 1);
}

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

enum class Pole { Nazwa, Producent, Model, Kategoria, Seryjny, Inwentarzowy, Miejsce, Zakup, Cena, Instrukcja, Uwagi };

const std::map<std::string, Pole>& aliasy() {
    static const std::map<std::string, Pole> m = {
        {"nazwa", Pole::Nazwa}, {"narzedzie", Pole::Nazwa}, {"name", Pole::Nazwa}, {"tool", Pole::Nazwa},
        {"producent", Pole::Producent}, {"manufacturer", Pole::Producent},
        {"model", Pole::Model},
        {"kategoria", Pole::Kategoria}, {"category", Pole::Kategoria}, {"rodzaj", Pole::Kategoria},
        {"numerseryjny", Pole::Seryjny}, {"nrseryjny", Pole::Seryjny}, {"serial", Pole::Seryjny}, {"serialnumber", Pole::Seryjny},
        {"numerinwentarzowy", Pole::Inwentarzowy}, {"nrinwentarzowy", Pole::Inwentarzowy}, {"inwentarzowy", Pole::Inwentarzowy}, {"nrinw", Pole::Inwentarzowy}, {"inventory", Pole::Inwentarzowy},
        {"miejsce", Pole::Miejsce}, {"location", Pole::Miejsce},
        {"datazakupu", Pole::Zakup}, {"zakup", Pole::Zakup}, {"purchased", Pole::Zakup},
        {"cena", Pole::Cena}, {"price", Pole::Cena},
        {"instrukcja", Pole::Instrukcja}, {"manual", Pole::Instrukcja},
        {"uwagi", Pole::Uwagi}, {"notes", Pole::Uwagi}, {"komentarz", Pole::Uwagi}};
    return m;
}

std::string cudzyslow(const std::string& s) {
    if (s.find_first_of(";\"\n\r") == std::string::npos) return s;
    std::string w = "\"";
    for (char c : s) { if (c == '"') w += '"'; w += c; }
    return w + "\"";
}

const char* NAGLOWEK = "Nazwa;Producent;Model;Kategoria;Numer seryjny;Numer inwentarzowy;Miejsce;Data zakupu;Cena;Instrukcja;Uwagi";

}  // namespace

Wynik parsuj(const std::string& tekst) {
    Wynik wynik;
    const auto wiersze = ParserCSV::parsuj(tekst);
    if (wiersze.empty()) { wynik.blad = "Plik jest pusty."; return wynik; }
    std::map<size_t, Pole> kolumny;
    bool jestNazwa = false;
    for (size_t i = 0; i < wiersze[0].size(); ++i) {
        auto it = aliasy().find(klucz(przytnij(wiersze[0][i])));
        if (it == aliasy().end()) continue;
        kolumny[i] = it->second;
        jestNazwa = jestNazwa || it->second == Pole::Nazwa;
    }
    if (!jestNazwa) { wynik.blad = "W pierwszym wierszu brakuje nagłówka \"Nazwa\". Użyj szablonu CSV jako wzoru."; return wynik; }
    for (size_t w = 1; w < wiersze.size(); ++w) {
        Narzedzie n;
        for (size_t c = 0; c < wiersze[w].size(); ++c) {
            auto it = kolumny.find(c);
            if (it == kolumny.end()) continue;
            const std::string v = przytnij(wiersze[w][c]);
            if (v.empty()) continue;
            switch (it->second) {
                case Pole::Nazwa: n.nazwa = v; break;
                case Pole::Producent: n.producent = v; break;
                case Pole::Model: n.model = v; break;
                case Pole::Kategoria: n.kategoria = v; break;
                case Pole::Seryjny: n.numerSeryjny = v; break;
                case Pole::Inwentarzowy: n.numerInwentarzowy = v; break;
                case Pole::Miejsce: n.miejsce = v; break;
                case Pole::Zakup: n.dataZakupu = v; break;
                case Pole::Cena: {
                    double d = 0;
                    if (tekstNaLiczbe(v, d) && d >= 0) n.cena = d;
                    else wynik.ostrzezenia.push_back("Wiersz " + std::to_string(w + 1) + ": nieprawidłowa cena \"" + v + "\" pominięta");
                    break;
                }
                case Pole::Instrukcja: n.instrukcja = v; break;
                case Pole::Uwagi: n.uwagi = v; break;
            }
        }
        if (n.nazwa.empty()) { wynik.ostrzezenia.push_back("Wiersz " + std::to_string(w + 1) + ": brak nazwy"); continue; }
        wynik.narzedzia.push_back(std::move(n));
    }
    return wynik;
}

std::string naCSV(const std::vector<Narzedzie>& narzedzia) {
    std::string w = std::string("\xEF\xBB\xBF") + NAGLOWEK + "\n";
    for (const Narzedzie& n : narzedzia)
        w += cudzyslow(n.nazwa) + ";" + cudzyslow(n.producent) + ";" + cudzyslow(n.model) + ";" + cudzyslow(n.kategoria) + ";" +
             cudzyslow(n.numerSeryjny) + ";" + cudzyslow(n.numerInwentarzowy) + ";" + cudzyslow(n.miejsce) + ";" + n.dataZakupu + ";" +
             (n.cena ? liczbaNaTekstPolski(*n.cena, 2, 2) : "") + ";" + cudzyslow(n.instrukcja) + ";" + cudzyslow(n.uwagi) + "\n";
    return w;
}

std::string szablon() {
    return std::string("\xEF\xBB\xBF") + NAGLOWEK + "\n"
           "Lutownica transformatorowa;Weller;WTCP 51;Lutowanie;;N-001;Biurko, szuflada 1;2019-03-14;450,00;;Grot ST7\n"
           "Multimetr;Uni-T;UT61E;Mierniki;123456;N-002;Półka nad biurkiem;2021-11-02;230,00;D:\\\\Instrukcje\\\\ut61e.pdf;\n";
}

} // namespace NarzedziaCSV
