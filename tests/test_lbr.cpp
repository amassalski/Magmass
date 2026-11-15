// Biblioteki Eagle (.lbr): rozkład nazw pakietów, opisy, atrybuty. Bez bazy danych.
#include "Import/ImportLBR.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

static int sprawdzenia = 0, bledy = 0;
#define CHECK(w) do { ++sprawdzenia; if (!(w)) { ++bledy; std::cerr << "BŁĄD " << __FILE__ << ":" << __LINE__ << ": " #w "\n"; } } while (0)

static const char* PRZYKLAD = R"(<?xml version="1.0" encoding="utf-8"?>
<eagle version="7.6.0"><drawing><library>
<description>&lt;b&gt;Tranzystory&lt;/b&gt;&lt;p&gt;moje</description>
<packages>
<package name="TO220AH"><description>&lt;b&gt;Molded Package&lt;/b&gt;&lt;p&gt;
grid 2.54 mm</description>
<pad name="B" x="0" y="0" drill="1"/><pad name="C" x="2.54" y="0" drill="1"/><pad name="E" x="5.08" y="0" drill="1"/></package>
<package name="TO220AV"><description>Molded, 2,5W max</description><pad name="1" x="0" y="0" drill="1"/></package>
<package name="SOT23-BEC"><description>SOT-23 250V</description><smd name="B" x="0" y="0" dx="1" dy="1" layer="1"/><smd name="E" x="1" y="0" dx="1" dy="1" layer="1"/></package>
<package name="MPC71-5"><description>&lt;b&gt;POWER RESISTOR&lt;/b&gt;&lt;p&gt; metal band R, 5W, grid 9 mm Buerklin</description><pad name="1" x="0" y="0" drill="1"/><pad name="2" x="9" y="0" drill="1"/></package>
</packages>
<devicesets>
<deviceset name="BD437" prefix="Q"><description>&lt;b&gt;NPN&lt;/b&gt;</description>
<devices><device name="" package="TO220AH"><technologies><technology name=""/></technologies></device>
<device name="V" package="TO220AV"><technologies><technology name=""/></technologies></device></devices></deviceset>
</devicesets></library></drawing></eagle>)";

int main() {
    // nazwy pakietów -> rodzina i wariant
    std::string r, w;
    auto rozloz = [&](const char* n) { ImportLBR::rozlozNazwe(n, r, w); };
    rozloz("TO220AH");       CHECK(r == "TO-220" && w == "AH");
    rozloz("TO92-BCE");      CHECK(r == "TO-92" && w == "BCE");
    rozloz("SOT23-BEC");     CHECK(r == "SOT-23" && w == "BEC");
    rozloz("TOP3AV");        CHECK(r == "TOP-3" && w == "AV");
    rozloz("TO-247AD-H");    CHECK(r == "TO-247" && w == "AD-H");
    rozloz("TO3/A");         CHECK(r == "TO-3" && w == "A");
    rozloz("TO92/");         CHECK(r == "TO-92" && w.empty());
    rozloz("TO18-");         CHECK(r == "TO-18" && w.empty());
    rozloz("TRANSISTOR-NPN_TO18-"); CHECK(r == "TO-18" && w.empty());
    rozloz("KH206-8");       CHECK(r == "KH206-8" && w.empty());     // nie rozpoznana rodzina: zostaje nazwa
    rozloz("DIL14");         CHECK(r == "DIL14" && w.empty());

    CHECK(ImportLBR::opisBezHTML("<b>Molded</b><p>\n grid  2.54 mm &amp; co") == "Molded grid 2.54 mm & co" ||
          ImportLBR::opisBezHTML("<b>Molded</b><p>\n grid  2.54 mm &amp; co") == "Molded grid 2.54 mm &amp; co");

    std::string blad;
    auto b = ImportLBR::parsuj(PRZYKLAD, blad);
    CHECK(b.has_value());
    if (b) {
        CHECK(b->pakiety.size() == 4 && b->urzadzenia.size() == 2);
        const auto& p = b->pakiety;
        CHECK(p[0].nazwa == "TO220AH" && p[0].rodzina == "TO-220" && p[0].wariant == "AH");
        CHECK(p[0].montaz == "THT" && p[0].liczbaPinow == 3 && p[0].rozstaw == "2.54 mm");
        CHECK(!p[0].mocW);
        CHECK(p[1].mocW && *p[1].mocW == 2.5);
        CHECK(p[2].montaz == "SMD" && p[2].liczbaPinow == 2 && p[2].napiecieV && *p[2].napiecieV == 250);
        CHECK(p[3].mocW && *p[3].mocW == 5 && p[3].rozstaw == "9 mm" && p[3].rodzina == "MPC71-5");
        CHECK(b->urzadzenia[0].zestaw == "BD437" && b->urzadzenia[0].pakiet == "TO220AH" && b->urzadzenia[0].prefiks == "Q");
        CHECK(b->urzadzenia[1].wariant == "V" && b->urzadzenia[1].pakiet == "TO220AV");
    }
    CHECK(!ImportLBR::parsuj("<eagle><drawing><schematic/></drawing></eagle>", blad) && !blad.empty());
    CHECK(!ImportLBR::parsuj("to nie xml <<<", blad));

    // prawdziwe biblioteki (opcjonalnie): DM_TEST_LBR=katalog z plikami .lbr
    if (const char* kat = std::getenv("DM_TEST_LBR")) {
        int plikow = 0;
        for (const auto& wpis : std::filesystem::directory_iterator(kat)) {
            if (wpis.path().extension() != ".lbr") continue;
            std::ifstream f(wpis.path(), std::ios::binary);
            std::stringstream ss; ss << f.rdbuf();
            auto lib = ImportLBR::parsuj(ss.str(), blad);
            CHECK(lib && !lib->pakiety.empty());
            ++plikow;
        }
        std::cout << "biblioteki z " << kat << ": " << plikow << "\n";
    }
    std::cout << sprawdzenia << " sprawdzeń, " << bledy << " błędów\n";
    return bledy ? 1 : 0;
}
