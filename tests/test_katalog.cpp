// Testy katalogu elementów z/do CSV. Bez bazy danych i wxWidgets.
#include "Logika/KatalogCSV.h"
#include "Logika/LampyKatalog.h"
#include "Logika/OpisyParametrow.h"
#include <cmath>

#include <iostream>

static int bledy = 0, sprawdzenia = 0;
#define CHECK(w) do { ++sprawdzenia; if (!(w)) { ++bledy; std::cerr << "BŁĄD " << __FILE__ << ":" << __LINE__ << "  " #w "\n"; } } while (0)

int main() {
    {   // słownik opisów parametrów: klucz niezależny od wielkości liter, spacji, podkreśleń i nawiasów
        CHECK(OpisyParametrow::zeSlownika("W_amax").find("anody") != std::string::npos);
        CHECK(OpisyParametrow::zeSlownika("w_AMAX") == OpisyParametrow::zeSlownika("W_amax"));
        CHECK(OpisyParametrow::zeSlownika("V_{CE(sat)}") == OpisyParametrow::zeSlownika("VCE sat") && !OpisyParametrow::zeSlownika("VCE sat").empty());
        CHECK(OpisyParametrow::zeSlownika("coś nieznanego").empty());
        ParametrKatalogu p; p.nazwa = "W_amax";
        CHECK(OpisyParametrow::dlaParametru(p) == OpisyParametrow::zeSlownika("W_amax"));
        p.opis = "Moja moc"; CHECK(OpisyParametrow::dlaParametru(p) == "Moja moc");
    }

    auto p = KatalogCSV::parsujParametry("Napięcie=3-32 V; Pasmo = 1 MHz;;bez równości; Pusty=; =pusta nazwa");
    CHECK(p.size() == 2 && p[0].nazwa == "Napięcie" && p[0].wartosc == "3-32 V" && p[0].zrodlo == "csv");
    CHECK(p.size() == 2 && p[1].nazwa == "Pasmo" && p[1].wartosc == "1 MHz");
    CHECK(KatalogCSV::parametryNaTekst(p) == "Napięcie=3-32 V; Pasmo=1 MHz");

    // dowolne kolumny stają się parametrami, aliasy nagłówków działają
    auto w = KatalogCSV::parsuj(
        "MPN;Manufacturer;Description;Package;Category;Datasheet;Napięcie zasilania;Parametry\n"
        "LM358;TI;Wzmacniacz;DIP-8;Wzmacniacze;http://x/lm358.pdf;3-32 V;Pasmo=1 MHz\n"
        ";TI;bez symbolu;;;;;\n"
        "NE555P;;;;;;4,5-16 V;\n");
    CHECK(w.blad.empty() && w.pozycje.size() == 3);
    const auto& a = w.pozycje[0].katalog;
    CHECK(a.symbol == "LM358" && a.producent == "TI" && a.opis == "Wzmacniacz" && a.obudowa == "DIP-8" &&
          a.rodzina == "Wzmacniacze" && a.datasheetUrl == "http://x/lm358.pdf");
    CHECK(a.parametry.size() == 2 && a.parametry[0].nazwa == "Napięcie zasilania" && a.parametry[0].wartosc == "3-32 V" &&
          a.parametry[1].nazwa == "Pasmo");
    CHECK(a.parametr("pasmo") && a.parametr("PASMO")->wartosc == "1 MHz" && !a.parametr("nie ma"));
    CHECK(w.pozycje[1].blad == "brak symbolu" && w.pozycje[1].nrWiersza == 3);
    CHECK(w.pozycje[2].katalog.symbol == "NE555P" && w.pozycje[2].katalog.parametry.size() == 1);

    // brak kolumny Symbol
    CHECK(!KatalogCSV::parsuj("Opis;Obudowa\nx;y\n").blad.empty());
    CHECK(!KatalogCSV::parsuj("").blad.empty());

    // obieg: eksport -> import
    std::vector<PozycjaKatalogu> lista = {a, w.pozycje[2].katalog};
    lista[0].uwagi = "z \"cudzysłowem\"; i średnikiem";
    const std::string csv = KatalogCSV::naCSV(lista);
    CHECK(csv.compare(0, 3, "\xEF\xBB\xBF") == 0);
    auto z = KatalogCSV::parsuj(csv);
    CHECK(z.blad.empty() && z.pozycje.size() == 2);
    CHECK(z.pozycje[0].katalog.uwagi == lista[0].uwagi && z.pozycje[0].katalog.parametry.size() == 2);

    // szablon da się wczytać
    auto s = KatalogCSV::parsuj(KatalogCSV::szablon());
    CHECK(s.blad.empty() && s.pozycje.size() == 3 && s.pozycje[0].blad.empty());
    CHECK(s.pozycje[2].katalog.parametr("hFE") && s.pozycje[2].katalog.parametr("hFE")->wartosc == "200-450");

    // lampy w katalogu
    {
        auto pr = LampyKatalog::przykladowe();
        CHECK(pr.size() == 5 && pr[2].symbol == "ECC83" && pr[2].rodzina == LampyKatalog::kRodzina);
        auto d = LampyKatalog::zPozycji(pr[2]);
        CHECK(d && std::abs(d->sa - 1.6) < 1e-9 && std::abs(d->ka - 100) < 1e-9 && std::abs(d->rwew - 62.5) < 1e-9);
        CHECK(d && d->cak && d->cas && d->csk && std::abs(*d->cak - 0.45) < 1e-9 && std::abs(*d->csk - 1.6) < 1e-9);
        {   // lampy mocy dla Transformatora PP: Ia0 i Rwew z pozycji katalogu, Ka = Sa·Rwew
            const auto pm = LampyKatalog::przykladoweMocy();
            CHECK(pm.size() == 8 && pm[0].symbol == "EL34" && pm[7].symbol == "EBL21");
            bool ok8 = true;
            for (const auto& p : pm) {
                const auto dl = LampyKatalog::zPozycji(p);
                ok8 = ok8 && dl && dl->ia0Ma && *dl->ia0Ma > 0 && dl->raaPPOm && *dl->raaPPOm > 0 && dl->rwew > 0 && dl->sa > 0 && std::fabs(dl->ka - dl->sa * dl->rwew) < 1e-3;
            }
            CHECK(ok8);
            const auto el34 = LampyKatalog::zPozycji(pm[0]);
            CHECK(el34 && std::fabs(el34->rwew - 17) < 1e-9 && std::fabs(*el34->ia0Ma - 45) < 1e-9 && std::fabs(*el34->raaPPOm - 6600) < 1e-9);
        }
        CHECK(LampyKatalog::czyLampa(pr[0]));
        auto e85 = LampyKatalog::zPozycji(pr[3]);
        CHECK(e85 && !e85->cak && e85->cas && std::abs(e85->ka / e85->rwew - e85->sa) < 0.05);
        // przejście przez CSV (parametry w jednej kolumnie) nie gubi danych
        auto csv = KatalogCSV::naCSV(pr);
        auto w = KatalogCSV::parsuj(csv);
        CHECK(w.blad.empty() && w.pozycje.size() == 5);
        auto d2 = LampyKatalog::zPozycji(w.pozycje[4].katalog);
        CHECK(d2 && std::abs(d2->sa - 12.5) < 1e-9 && std::abs(d2->ka - 33) < 1e-9 && std::abs(d2->rwew - 2.6) < 1e-9 &&
              d2->cak && std::abs(*d2->cak - 0.18) < 1e-9);
        // brakujące wielkości są dopełniane, a zwykłe elementy nie są lampami
        PozycjaKatalogu x; x.symbol = "X"; x.parametry = {{LampyKatalog::kParKa, "60"}, {LampyKatalog::kParSa, "5,5"}};
        auto dx = LampyKatalog::zPozycji(x);
        CHECK(dx && std::abs(dx->rwew - 60.0 / 5.5) < 1e-9 && LampyKatalog::czyLampa(x));
        PozycjaKatalogu y; y.symbol = "LM358"; y.rodzina = "Wzmacniacze operacyjne";
        CHECK(!LampyKatalog::czyLampa(y) && !LampyKatalog::zPozycji(y));
        x.parametry = {{LampyKatalog::kParKa, "abc"}, {LampyKatalog::kParRwew, "0"}};
        CHECK(!LampyKatalog::zPozycji(x));
    }

    std::cout << sprawdzenia << " sprawdzeń, " << bledy << " błędów\n";
    return bledy ? 1 : 0;
}
