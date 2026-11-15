#ifndef ETYKIETYZPL_H
#define ETYKIETYZPL_H

#include "../Narzedzia/KodKreskowy.h"

#include <string>
#include <vector>

/**
 * Etykiety w języku ZPL II (drukarki Zebra, np. ZD421), bez wxWidgets.
 * Układ liczony jest w kropkach drukarki (dpi); ten sam układ rysuje podgląd w oknie etykiet,
 * a zpl() zamienia go na polecenia dla drukarki: tekst czcionką skalowalną 0 i kod kreskowy
 * generowany przez samą drukarkę (ostry, bez rastrowania).
 */
namespace EtykietyZPL {

struct Ustawienia {
    double szerMm = 80;        ///< szerokość etykiety (w poprzek głowicy)
    double wysMm = 50;         ///< długość etykiety (wzdłuż rolki)
    int dpi = 203;             ///< 203, 300 albo 600
    bool bezPolskichZnakow = true;  ///< zamienia ąćęłńóśźż, µ, Ω itp. na znaki ASCII (gdy czcionka drukarki ich nie ma)
    bool obrot180 = false;     ///< wydruk obrócony o 180 stopni
};

struct Etykieta {
    std::string symbol;        ///< pierwsza, pogrubiona linia
    std::string drugaLinia;    ///< wartość, tolerancja, obudowa
    std::string miejsce;
    std::string producent;
    std::string opis;
    std::string kod;           ///< tekst kodu kreskowego (pusty = bez kodu)
    KodKreskowy::Typ typ = KodKreskowy::Typ::EAN13;
    int kopie = 1;
};

struct Tekst {
    int x = 0, y = 0;          ///< lewy górny róg w kropkach
    int wysokosc = 0;          ///< wysokość znaków w kropkach
    bool gruby = false;
    std::string tekst;         ///< UTF-8, już po ewentualnej zamianie znaków
};

struct Kod {
    bool jest = false;
    int x = 0, y = 0;          ///< lewy górny róg pasków
    int wysokosc = 0;          ///< wysokość pasków (bez cyfr pod spodem)
    int modul = 2;             ///< szerokość najwęższego paska w kropkach (^BY)
    KodKreskowy::Typ typ = KodKreskowy::Typ::EAN13;
    std::string tekst;
};

struct Uklad {
    int szer = 0, wys = 0;     ///< wymiary etykiety w kropkach
    std::vector<Tekst> teksty;
    Kod kod;
};

int mmNaKropki(double mm, int dpi);

/// ąćęłńóśźż (także wielkie), µ, Ω, ±, ° na odpowiedniki ASCII; inne znaki bez zmian
std::string bezPolskichZnakow(const std::string& utf8);
/// Zabezpiecza tekst do pola ^FD (znaki ^ ~ _ jako sekwencje _XX, do użycia z ^FH)
std::string zabezpiecz(const std::string& utf8);

Uklad zbudujUklad(const Etykieta& etykieta, const Ustawienia& ust);

/// Polecenia ZPL jednej etykiety (od ^XA do ^XZ), z liczbą kopii (^PQ)
std::string zpl(const Etykieta& etykieta, const Ustawienia& ust);
/// Wiele etykiet; kolejne identyczne (ten sam symbol i kod) są łączone w jedną z ^PQ
std::string zplWielu(const std::vector<Etykieta>& etykiety, const Ustawienia& ust);
/// Etykieta próbna: ramka, wymiary i przykładowy kod (do sprawdzenia ustawień drukarki)
std::string zplProbny(const Ustawienia& ust);

}  // namespace EtykietyZPL

#endif  // ETYKIETYZPL_H
