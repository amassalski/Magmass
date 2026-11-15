// Testy kodów kreskowych (kodowanie wzorców sprawdzono też niezależnie biblioteką python-barcode).
#include "Eksport/DrukujEtykiety.h"
#include "Eksport/EtykietyZPL.h"
#include "Narzedzia/KodKreskowy.h"

#include <cstdlib>
#include <iostream>

using namespace KodKreskowy;

static int bledy = 0;
static int sprawdzenia = 0;
#define CHECK(w)                                                                   \
    do {                                                                           \
        ++sprawdzenia;                                                             \
        if (!(w)) {                                                                \
            ++bledy;                                                               \
            std::cerr << "BŁĄD " << __FILE__ << ":" << __LINE__ << "  " #w "\n";  \
        }                                                                          \
    } while (0)

static std::string napis(const std::vector<bool>& m) {
    std::string s;
    for (bool b : m) s += b ? '1' : '0';
    return s;
}

/// Linia pikseli: margines, moduły o szerokości `skala` px, margines
static std::vector<unsigned char> linia(const std::vector<bool>& m, double skala, int margines, bool odwroc = false,
                                        int szum = 0) {
    std::vector<bool> mm = m;
    if (odwroc) mm.assign(m.rbegin(), m.rend());
    std::vector<unsigned char> px;
    px.insert(px.end(), static_cast<size_t>(margines), 235);
    double poz = 0;
    for (bool b : mm) {
        poz += skala;
        const size_t ile = static_cast<size_t>(poz + 0.5) - (px.size() - static_cast<size_t>(margines));
        for (size_t i = 0; i < ile; ++i) {
            int v = b ? 25 : 235;
            if (szum) v += (std::rand() % (2 * szum + 1)) - szum;
            px.push_back(static_cast<unsigned char>(std::max(0, std::min(255, v))));
        }
    }
    px.insert(px.end(), static_cast<size_t>(margines), 235);
    return px;
}

int main() {
    // --- cyfry kontrolne i identyfikatory
    CHECK(cyfraKontrolna("590123412345") == '7');
    CHECK(cyfraKontrolna("4006381333931") == cyfraKontrolna("4006381333931"));
    CHECK(cyfraKontrolna("400638133393") == '1');
    CHECK(cyfraKontrolna("9638507") == '4');
    CHECK(poprawnyEAN("5901234123457"));
    CHECK(!poprawnyEAN("5901234123458"));
    CHECK(poprawnyEAN("96385074"));
    CHECK(!poprawnyEAN("9638507"));
    CHECK(!poprawnyEAN("59012341234a7"));

    CHECK(tekstKoduElementu(Typ::EAN13, 1, 20) == "2000000000015");
    CHECK(tekstKoduElementu(Typ::EAN13, 1234567890, 29) == tekstKoduElementu(Typ::EAN13, 1234567890, 29));
    CHECK(tekstKoduElementu(Typ::EAN13, 12345678901LL, 20).empty());
    CHECK(tekstKoduElementu(Typ::EAN13, 0, 20).empty());
    CHECK(tekstKoduElementu(Typ::EAN8, 17, 20) == "00000178");
    CHECK(poprawnyEAN(tekstKoduElementu(Typ::EAN8, 17, 20)));
    CHECK(tekstKoduElementu(Typ::EAN8, 12345678, 20).empty());
    CHECK(tekstKoduElementu(Typ::EAN13, 5, 7) == tekstKoduElementu(Typ::EAN13, 5, 7));
    for (long long id : {1LL, 7LL, 42LL, 99999LL, 9999999999LL}) {
        const std::string k = tekstKoduElementu(Typ::EAN13, id, 27);
        CHECK(poprawnyEAN(k));
        CHECK(idZKodu(k, 27) == std::optional<long long>(id));
        CHECK(!idZKodu(k, 28).has_value());           // inny prefiks = cudzy kod
    }
    CHECK(idZKodu("00000178", 20) == std::optional<long long>(17));
    CHECK(!idZKodu("5901234123457", 20).has_value());
    CHECK(!idZKodu("2000000000008", 20).has_value());  // numer 0
    CHECK(typZNazwy("EAN8") == Typ::EAN8 && typZNazwy("Code128") == Typ::Code128 && typZNazwy("???") == Typ::EAN13);

    // --- wzorce (porównane z niezależną implementacją)
    CHECK(napis(modulyEAN("5901234123457")) ==
          "10100010110100111011001100100110111101001110101010110011011011001000010101110010011101000100101");
    CHECK(modulyEAN("5901234123457").size() == 95);
    CHECK(modulyEAN("96385074").size() == 67);
    CHECK(modulyEAN("5901234123458").empty());
    CHECK(napis(modulyCode128("Abc 12~")) ==
          "1101001000010100011000100100001101000010110011011001100100111001101100111001010001011110100111100101100011101011");
    CHECK(modulyCode128("Abc 12~").size() % 11 == 2);  // n symboli po 11 modułów + stop 13
    CHECK(modulyCode128("zażółć").empty());             // poza ASCII
    CHECK(modulyCode128("").empty());

    // --- odczyt z linii: różne skale, szum, odwrócenie
    for (const std::string& kod : {std::string("5901234123457"), tekstKoduElementu(Typ::EAN13, 1234, 20),
                                   tekstKoduElementu(Typ::EAN13, 987654, 27), std::string("96385074"),
                                   tekstKoduElementu(Typ::EAN8, 4321, 0), std::string("4006381333931")}) {
        const auto m = modulyEAN(kod);
        CHECK(!m.empty());
        for (double skala : {2.0, 2.7, 3.0, 4.3}) {
            auto w = odczytajLinie(linia(m, skala, 30));
            CHECK(w && *w == kod);
        }
        auto w = odczytajLinie(linia(m, 3.0, 30, true));
        CHECK(w && *w == kod);                         // odwrócony kod
        std::srand(7);
        w = odczytajLinie(linia(m, 3.0, 25, false, 18));
        CHECK(w && *w == kod);                         // szum
    }
    CHECK(!odczytajLinie(std::vector<unsigned char>(300, 128)).has_value());
    CHECK(!odczytajLinie(std::vector<unsigned char>(10, 0)).has_value());
    {
        // losowe paski nie dają kodu
        std::srand(3);
        std::vector<unsigned char> px(400);
        for (auto& v : px) v = (std::rand() % 2) ? 20 : 240;
        CHECK(!odczytajLinie(px).has_value());
    }

    // --- obraz: kod w środku, wokół biało
    {
        const std::string kod = tekstKoduElementu(Typ::EAN13, 555, 20);
        const auto l = linia(modulyEAN(kod), 2.0, 40);
        const int w = static_cast<int>(l.size()), h = 90;
        std::vector<unsigned char> obraz(static_cast<size_t>(w) * h, 235);
        for (int y = 25; y < 70; ++y) std::copy(l.begin(), l.end(), obraz.begin() + static_cast<std::ptrdiff_t>(y) * w);
        auto odczyt = odczytajObraz(obraz, w, h);
        CHECK(odczyt && *odczyt == kod);
        // kod poza środkiem
        std::fill(obraz.begin(), obraz.end(), 235);
        for (int y = 60; y < 85; ++y) std::copy(l.begin(), l.end(), obraz.begin() + static_cast<std::ptrdiff_t>(y) * w);
        odczyt = odczytajObraz(obraz, w, h);
        CHECK(odczyt && *odczyt == kod);
        CHECK(!odczytajObraz(std::vector<unsigned char>(w * h, 200), w, h).has_value());
        CHECK(!odczytajObraz({}, 0, 0).has_value());
    }

    // --- układ arkusza
    {
        for (const FormatEtykiet& f : DrukujEtykiety::formatyWbudowane()) {
            std::string powod;
            CHECK(DrukujEtykiety::poprawny(f, &powod));
            if (!DrukujEtykiety::poprawny(f, &powod)) std::cerr << f.nazwa << ": " << powod << "\n";
        }
        const FormatEtykiet& a4 = DrukujEtykiety::formatyWbudowane()[1];  // 3 x 7
        CHECK(a4.naStrone() == 21);
        auto p0 = DrukujEtykiety::polozenie(a4, 0);
        CHECK(p0.strona == 0 && p0.x == 7.2 && p0.y == 15.15);
        auto p4 = DrukujEtykiety::polozenie(a4, 4);  // druga linia, druga kolumna
        CHECK(p4.x == 7.2 + 66.0 && p4.y == 15.15 + 38.1);
        CHECK(DrukujEtykiety::polozenie(a4, 21).strona == 1);
        CHECK(DrukujEtykiety::liczbaStron(a4, 0, 21) == 1);
        CHECK(DrukujEtykiety::liczbaStron(a4, 0, 22) == 2);
        CHECK(DrukujEtykiety::liczbaStron(a4, 5, 16) == 1);   // 5 pominięte + 16 = 21
        CHECK(DrukujEtykiety::liczbaStron(a4, 5, 17) == 2);
        CHECK(DrukujEtykiety::liczbaStron(a4, 0, 0) == 0);
        CHECK(DrukujEtykiety::liczbaStron(a4, 25, 1) == 1);   // pominięcie liczone w obrębie arkusza
        auto s1 = DrukujEtykiety::naStronie(a4, 5, 20, 0);
        CHECK(s1.size() == 16 && s1.front().numer == 0 && s1.back().numer == 15);
        auto s2 = DrukujEtykiety::naStronie(a4, 5, 20, 1);
        CHECK(s2.size() == 4 && s2.front().numer == 16 && s2.front().polozenie.x == 7.2 && s2.front().polozenie.y == 15.15);
        CHECK(DrukujEtykiety::naStronie(a4, 0, 3, 1).empty());
        FormatEtykiet zly = a4;
        zly.kolumny = 4;
        CHECK(!DrukujEtykiety::poprawny(zly));
        zly = a4;
        zly.etykietaSzer = 5;
        CHECK(!DrukujEtykiety::poprawny(zly));
        zly = a4;
        zly.wiersze = 0;
        CHECK(!DrukujEtykiety::poprawny(zly));
    }


    // --- ZPL (Zebra ZD421)
    {
        using namespace EtykietyZPL;
        CHECK(mmNaKropki(25.4, 203) == 203);
        CHECK(mmNaKropki(80, 203) == 639);
        CHECK(mmNaKropki(50, 300) == 591);
        CHECK(bezPolskichZnakow("Zażółć gęślą jaźń ŁÓDŹ") == "Zazolc gesla jazn LODZ");
        CHECK(bezPolskichZnakow("10 µF 5 ±1 °C 4k7 Ω") == "10 uF 5 +/-1 degC 4k7 ohm");
        CHECK(bezPolskichZnakow("abc XYZ 123") == "abc XYZ 123");
        CHECK(zabezpiecz("a^b~c_d") == "a_5Eb_7Ec_5Fd");
        CHECK(zabezpiecz("x\ny") == "x y");

        Ustawienia u;  // 80 x 50 mm, 203 dpi
        Etykieta e;
        e.symbol = "BC547B";
        e.drugaLinia = "NPN TO-92";
        e.miejsce = "Kontener 1 / A1/2";
        e.producent = "ON Semi";
        e.opis = "Tranzystor ogólnego\nzastosowania";
        e.kod = "2000000000015";
        e.typ = Typ::EAN13;
        const Uklad uk = zbudujUklad(e, u);
        CHECK(uk.szer == 639 && uk.wys == 400);
        CHECK(uk.kod.jest && uk.kod.modul >= 2 && uk.kod.modul <= 4);
        CHECK(uk.kod.x > 0 && uk.kod.x + 95 * uk.kod.modul <= uk.szer);
        CHECK(uk.kod.y > 0 && uk.kod.y + uk.kod.wysokosc < uk.wys);
        CHECK(!uk.teksty.empty() && uk.teksty[0].tekst == "BC547B" && uk.teksty[0].gruby);
        // teksty mieszczą się nad kodem, bez nakładania
        for (const Tekst& t : uk.teksty) CHECK(t.y + t.wysokosc <= uk.kod.y);
        for (size_t i = 1; i < uk.teksty.size(); ++i) CHECK(uk.teksty[i].y >= uk.teksty[i - 1].y + uk.teksty[i - 1].wysokosc);
        // opis tylko pierwsza linia
        for (const Tekst& t : uk.teksty) CHECK(t.tekst.find("zastosowania") == std::string::npos);

        const std::string z = zpl(e, u);
        CHECK(z.rfind("^XA\n", 0) == 0);
        CHECK(z.find("^CI28") != std::string::npos);
        CHECK(z.find("^PW639") != std::string::npos && z.find("^LL400") != std::string::npos);
        CHECK(z.find("^FDBC547B^FS") != std::string::npos);
        CHECK(z.find("^BEN,") != std::string::npos);
        CHECK(z.find("^FD2000000000015^FS") != std::string::npos);
        CHECK(z.find("^PQ1") != std::string::npos);
        CHECK(z.size() >= 4 && z.compare(z.size() - 4, 4, "^XZ\n") == 0);
        CHECK(z.find("^PON") != std::string::npos);
        // żaden tekst użytkownika nie może wprowadzić własnych poleceń
        Etykieta zly = e;
        zly.symbol = "A^XZ^XA~JA_";
        const std::string zz = zpl(zly, u);
        CHECK(zz.find("A_5EXZ_5EXA_7EJA_5F") != std::string::npos);
        CHECK(zz.find("^FH_^FDA_5E") != std::string::npos);
        size_t xz = 0, ile = 0;
        while ((xz = zz.find("^XZ", xz)) != std::string::npos) { ++ile; ++xz; }
        CHECK(ile == 1);

        // ASCII zamiast polskich liter (domyślnie), polskie litery po wyłączeniu opcji
        Etykieta pl;
        pl.symbol = "Rezystor";
        pl.producent = "Łódź";
        CHECK(zpl(pl, u).find("Lodz") != std::string::npos);
        Ustawienia utf = u;
        utf.bezPolskichZnakow = false;
        CHECK(zpl(pl, utf).find("\xC5\x81\xC3\xB3" "d" "\xC5\xBA") != std::string::npos);

        // inne typy kodów i dpi
        Etykieta c128 = e;
        c128.typ = Typ::Code128;
        CHECK(zpl(c128, u).find("^BCN,") != std::string::npos);
        Etykieta e8 = e;
        e8.typ = Typ::EAN8;
        e8.kod = tekstKoduElementu(Typ::EAN8, 1, 0);
        CHECK(poprawnyEAN(e8.kod));
        CHECK(zpl(e8, u).find("^B8N,") != std::string::npos);
        Ustawienia u300 = u;
        u300.dpi = 300;
        const Uklad u3 = zbudujUklad(e, u300);
        CHECK(u3.szer == 945 && u3.wys == 591);
        CHECK(u3.kod.modul > uk.kod.modul || u3.kod.modul == 4);
        CHECK(u3.kod.x + 95 * u3.kod.modul <= u3.szer);
        Ustawienia male = u;
        male.szerMm = 50;
        male.wysMm = 25;
        const Uklad um = zbudujUklad(e, male);
        CHECK(um.kod.jest && um.kod.x + 95 * um.kod.modul <= um.szer);
        for (const Tekst& t : um.teksty) CHECK(t.y + t.wysokosc <= um.kod.y);
        // bardzo długi symbol: nie wychodzi poza etykietę (zmniejszony lub skrócony)
        Etykieta dlugi = e;
        dlugi.symbol = "SUPERDLUGIOZNACZENIEELEMENTUKTOREJESTZDECYDOWANIEZADLUGIE";
        const Uklad ud = zbudujUklad(dlugi, u);
        CHECK(ud.teksty[0].tekst.size() < dlugi.symbol.size());
        // bez kodu: tylko teksty
        Etykieta bk = e;
        bk.kod.clear();
        CHECK(!zbudujUklad(bk, u).kod.jest && zpl(bk, u).find("^BE") == std::string::npos);
        // obrót
        Ustawienia ob = u;
        ob.obrot180 = true;
        CHECK(zpl(e, ob).find("^POI") != std::string::npos);

        // kopie: kolejne identyczne łączone w ^PQ
        std::vector<Etykieta> wiele(3, e);
        Etykieta inna = e;
        inna.symbol = "BC548";
        wiele.push_back(inna);
        const std::string zw = zplWielu(wiele, u);
        CHECK(zw.find("^PQ3") != std::string::npos && zw.find("^PQ1") != std::string::npos);
        size_t start = 0, blokow = 0;
        while ((start = zw.find("^XA", start)) != std::string::npos) { ++blokow; ++start; }
        CHECK(blokow == 2);
        CHECK(zplWielu({}, u).empty());
        Etykieta k5 = e;
        k5.kopie = 5;
        CHECK(zpl(k5, u).find("^PQ5") != std::string::npos);

        // etykieta próbna
        const std::string pr = zplProbny(u);
        CHECK(pr.find("^GB") != std::string::npos && pr.find("80 x 50 mm") != std::string::npos);
        CHECK(pr.find("^XA") == 0 && pr.find("^XZ") != std::string::npos);
    }

    std::cout << sprawdzenia << " sprawdzeń, " << bledy << " błędów\n";
    return bledy == 0 ? 0 : 1;
}
