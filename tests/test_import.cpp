// Testy parserów list części (CSV, KiCad, Eagle). Nie wymagają bazy danych ani wxWidgets.

#include "Import/ImportBOM.h"
#include "Import/TekstEPUB.h"
#include "Import/ImportEagle.h"
#include "Import/ImportKiCad.h"
#include "Import/MiniXML.h"
#include "Import/ParserCSV.h"
#include "Import/BibliotekaNarodowa.h"
#include "Wyszukiwanie/Mouser.h"
#include "Wyszukiwanie/MiniJSON.h"
#include "Narzedzia/Indeksy.h"
#include "Narzedzia/PolecenieKsiazek.h"

#include <clocale>
#include <iostream>

static int bledy = 0;
static int sprawdzenia = 0;

#define CHECK(warunek)                                                                  \
    do {                                                                                \
        ++sprawdzenia;                                                                  \
        if (!(warunek)) {                                                               \
            ++bledy;                                                                    \
            std::cerr << "BŁĄD " << __FILE__ << ":" << __LINE__ << "  " #warunek "\n"; \
        }                                                                               \
    } while (0)

#define CHECK_EQ(a, b)                                                                              \
    do {                                                                                            \
        ++sprawdzenia;                                                                              \
        const auto va = (a);                                                                        \
        const auto vb = (b);                                                                        \
        if (!(va == vb)) {                                                                          \
            ++bledy;                                                                                \
            std::cerr << "BŁĄD " << __FILE__ << ":" << __LINE__ << "  " #a " == " #b "\n  jest: [" \
                      << va << "] oczekiwano: [" << vb << "]\n";                                    \
        }                                                                                           \
    } while (0)

static const PozycjaBOM* znajdz(const WynikImportuBOM& w, const std::string& oznaczenieZawiera) {
    for (const PozycjaBOM& p : w.pozycje) {
        if (p.oznaczenia.find(oznaczenieZawiera) != std::string::npos) return &p;
    }
    return nullptr;
}

static void testCSV() {
    std::cout << "[csv]\n";
    using ParserCSV::parsuj;
    auto w = parsuj("a,b,c\r\n1,\"x,y\",3\r\n");
    CHECK_EQ(w.size(), size_t(2));
    CHECK_EQ(w[1][1], std::string("x,y"));
    w = parsuj("\xEF\xBB\xBF" "a;b;c\n1;\"mówi \"\"cześć\"\"\";3\n\n4;5;6\n");
    CHECK_EQ(w.size(), size_t(3));
    CHECK_EQ(w[0][0], std::string("a"));  // BOM usunięty
    CHECK_EQ(w[1][1], std::string("mówi \"cześć\""));
    w = parsuj("a\tb\n1\t2\n");
    CHECK_EQ(w[1][1], std::string("2"));
    w = parsuj("a,b\n\"wiele\nlinii\",2\n");
    CHECK_EQ(w.size(), size_t(2));
    CHECK_EQ(w[1][0], std::string("wiele\nlinii"));
    CHECK_EQ(ParserCSV::wykryjSeparator("\"R1, R2\";x;y\n"), ';');
    CHECK(parsuj("").empty());
}

static void testXML() {
    std::cout << "[xml]\n";
    std::string blad;
    auto d = MiniXML::parsuj("<?xml version=\"1.0\"?><!-- k --><a x=\"1 &amp; 2\"><b/><b y='z'>t&lt;e</b></a>", blad);
    CHECK(d != nullptr);
    if (d) {
        CHECK_EQ(d->atrybut("x"), std::string("1 & 2"));
        CHECK_EQ(d->dzieciONazwie("b").size(), size_t(2));
        CHECK_EQ(d->dzieciONazwie("b")[1]->atrybut("y"), std::string("z"));
        CHECK_EQ(d->dzieciONazwie("b")[1]->tekst, std::string("t<e"));
    }
    CHECK(MiniXML::parsuj("<a><b></a>", blad) == nullptr);
    CHECK(!blad.empty());
}

static void testNormalizacji() {
    std::cout << "[normalizacja]\n";
    using namespace ImportBOM;
    CHECK_EQ(normalizujWartosc("4k7"), std::string("4.7k"));
    CHECK_EQ(normalizujWartosc("4.7 kΩ"), std::string("4.7k"));
    CHECK_EQ(normalizujWartosc("10K"), std::string("10k"));
    CHECK_EQ(normalizujWartosc("10 kΩ"), std::string("10k"));
    CHECK_EQ(normalizujWartosc("100nF"), std::string("100n"));
    CHECK_EQ(normalizujWartosc("0.1uF"), normalizujWartosc("100nF") == "100n" ? std::string("0.1u") : std::string("?"));
    CHECK_EQ(normalizujWartosc("1µF"), std::string("1u"));
    CHECK_EQ(normalizujWartosc("LM358"), std::string("lm358"));
    CHECK_EQ(normalizujWartosc("  22R "), std::string("22"));
    CHECK_EQ(normalizujObudowe("Resistor_SMD:R_0805_2012Metric"), std::string("0805"));
    CHECK_EQ(normalizujObudowe("Capacitor_SMD:C_0603_1608Metric"), std::string("0603"));
    CHECK_EQ(normalizujObudowe("SOT-23"), std::string("sot23"));
    CHECK_EQ(normalizujObudowe("R0805"), std::string("0805"));
    CHECK_EQ(normalizujObudowe("C0603K"), std::string("0603"));

    CHECK(porownajNaturalnie("R2", "R10"));
    CHECK(!porownajNaturalnie("R10", "R2"));
    CHECK(porownajNaturalnie("C9", "R1"));

    auto r = rozbijOznaczenia("R1, R2,R5-R7 C3-5");
    CHECK_EQ(r.size(), size_t(8));
    if (r.size() == 8) {
        CHECK_EQ(r[2], std::string("R5"));
        CHECK_EQ(r[4], std::string("R7"));
        CHECK_EQ(r[7], std::string("C5"));
    }
}

static const char* KICAD_CSV =
    "\"Reference\",\"Value\",\"Footprint\",\"Qty\",\"DNP\"\n"
    "\"C1, C2\",\"100nF\",\"Capacitor_SMD:C_0603_1608Metric\",\"2\",\"\"\n"
    "\"R1, R3, R2\",\"10k\",\"Resistor_SMD:R_0805_2012Metric\",\"3\",\"\"\n"
    "\"R4\",\"10k\",\"Resistor_SMD:R_0805_2012Metric\",\"1\",\"DNP\"\n"
    "\"U1\",\"LM358\",\"Package_SO:SOIC-8_3.9x4.9mm_P1.27mm\",\"1\",\"\"\n";

static const char* POLSKI_CSV =
    "Oznaczenia;Wartość;Obudowa;Ilość;Producent;Symbol producenta\r\n"
    "R1-R3;4k7;0805;;Yageo;RC0805FR-074K7L\r\n"
    "C1;1µF;0603;5;;\r\n";

static void testTekstuEPUB() {
    std::cout << "[epub: tekst]\n";
    const std::string html = "<html><head><title>Tytul</title><style>p{color:red}</style><script>var x='<p>nie</p>';</script></head>"
                             "<body><!-- komentarz --><h1>Rozdzia\xC5\x82 1</h1><p>Lampa <b>ECC83</b> &amp; trioda&nbsp;&#8211; &#x142;&oacute;d\xC5\xBA.</p>"
                             "<p>Drugi   akapit<br/>z\n  zawini\xC4\x99" "ciem.</p></body></html>";
    const std::string t = TekstEPUB::tekstZHTML(html);
    CHECK(t.find("nie") == std::string::npos && t.find("color") == std::string::npos && t.find("komentarz") == std::string::npos);
    CHECK(t.find("Lampa ECC83 & trioda \xE2\x80\x93 \xC5\x82&oacute;d\xC5\xBA.") != std::string::npos);   // nieznana encja zostaje
    CHECK(t.find("Rozdzia\xC5\x82 1\n") != std::string::npos && t.find("Drugi akapit\nz zawini\xC4\x99" "ciem.") != std::string::npos);
    CHECK(TekstEPUB::tekstZHTML("").empty() && TekstEPUB::tekstZHTML("<p></p><p> </p>").empty());

    CHECK_EQ(TekstEPUB::polaczSciezke("OEBPS", "text/ch1.xhtml#a"), std::string("OEBPS/text/ch1.xhtml"));
    CHECK_EQ(TekstEPUB::polaczSciezke("OEBPS/text", "../img/a%20b.xhtml"), std::string("OEBPS/img/a b.xhtml"));
    CHECK_EQ(TekstEPUB::polaczSciezke("", "ch1.xhtml"), std::string("ch1.xhtml"));
    CHECK_EQ(TekstEPUB::polaczSciezke("OEBPS", "/abs/x.xhtml"), std::string("abs/x.xhtml"));

    const std::string opf = "<?xml version=\"1.0\"?><package xmlns=\"http://www.idpf.org/2007/opf\"><manifest>"
        "<item id=\"a\" href=\"ch2.xhtml\" media-type=\"application/xhtml+xml\"/><item id=\"b\" href=\"ch1.xhtml\" media-type=\"application/xhtml+xml\"/>"
        "<item id=\"c\" href=\"s.css\" media-type=\"text/css\"/><item id=\"ncx\" href=\"toc.ncx\" media-type=\"application/x-dtbncx+xml\"/></manifest>"
        "<spine toc=\"ncx\"><itemref idref=\"b\"/><itemref idref=\"a\"/><itemref idref=\"zly\"/></spine></package>";
    const auto kol = TekstEPUB::kolejnoscRozdzialow(opf, "OEBPS");
    CHECK(kol.size() == 2 && kol[0] == "OEBPS/ch1.xhtml" && kol[1] == "OEBPS/ch2.xhtml");
    CHECK(TekstEPUB::kolejnoscRozdzialow("to nie xml", "").empty());

    std::string dlugi;
    for (int i = 0; i < 100; ++i) dlugi += "Akapit numer " + std::to_string(i) + " z tekstem o lampach.\n";
    const auto kaw = TekstEPUB::podzielTekst(dlugi, 500);
    CHECK(kaw.size() > 5);
    bool wszystkieKrotkie = true; size_t znaki = 0;
    for (const auto& k : kaw) { wszystkieKrotkie = wszystkieKrotkie && k.size() <= 500 && !k.empty(); znaki += k.size(); }
    CHECK(wszystkieKrotkie && znaki >= dlugi.size() - 100);
    const auto jedenAkapit = TekstEPUB::podzielTekst(std::string(1200, 'x') + " " + std::string(1200, 'y'), 500);
    CHECK(jedenAkapit.size() >= 4);                                   // bardzo długi akapit tnie się na kawałki
    CHECK(TekstEPUB::podzielTekst("").empty() && TekstEPUB::podzielTekst("krotki").size() == 1);
}

static void testGrupowaniaZapisow() {
    std::cout << "[bom: grupowanie zapisów]\n";
    std::vector<PozycjaBOM> surowe;
    auto dodaj = [&](const char* ozn, const char* wartosc, const char* obudowa) {
        PozycjaBOM p; p.oznaczenia = ozn; p.wartosc = wartosc; p.obudowa = obudowa; surowe.push_back(p);
    };
    dodaj("R1", "10k", "Resistor_SMD:R_0805_2012Metric");
    dodaj("R2", "10K", "R0805");
    dodaj("R3", "10 k", "0805");
    dodaj("R4", "10k", "R0603");               // inny rozmiar: osobna pozycja
    dodaj("C1", "100n", "C0603");
    dodaj("C2", "100nF", "Capacitor_SMD:C_0603_1608Metric");
    auto w = ImportBOM::zgrupuj(surowe);
    CHECK_EQ(w.size(), size_t(3));
    for (const PozycjaBOM& p : w) {
        if (p.wartosc == "10k" && p.obudowa == "Resistor_SMD:R_0805_2012Metric") { CHECK_EQ(p.ilosc, 3); CHECK_EQ(p.oznaczenia, std::string("R1, R2, R3")); }
        else if (p.oznaczenia == "R4") CHECK_EQ(p.ilosc, 1);
        else if (p.oznaczenia == "C1-C2" || p.oznaczenia == "C1, C2") CHECK_EQ(p.ilosc, 2);
        else CHECK(false);
    }
}

static void testCSVBOM() {
    std::cout << "[bom csv]\n";
    WynikImportuBOM w;
    std::string blad;
    CHECK(ImportBOM::parsujTekst(KICAD_CSV, w, blad));
    CHECK_EQ(w.pozycje.size(), size_t(3));  // C, R (bez DNP), U
    CHECK_EQ(w.liczbaSztuk(), 6);
    const PozycjaBOM* r = znajdz(w, "R1");
    CHECK(r != nullptr);
    if (r) {
        CHECK_EQ(r->oznaczenia, std::string("R1, R2, R3"));  // posortowane naturalnie
        CHECK_EQ(r->ilosc, 3);
        CHECK_EQ(r->wartosc, std::string("10k"));
    }
    CHECK(!w.ostrzezenia.empty());  // informacja o pominiętym DNP

    WynikImportuBOM p;
    CHECK(ImportBOM::parsujTekst(POLSKI_CSV, p, blad));
    CHECK_EQ(p.pozycje.size(), size_t(2));
    r = znajdz(p, "R1");
    CHECK(r != nullptr);
    if (r) {
        CHECK_EQ(r->ilosc, 3);  // z liczby oznaczeń, gdy kolumna Ilość pusta
        CHECK_EQ(r->producent, std::string("Yageo"));
        CHECK_EQ(r->symbol, std::string("RC0805FR-074K7L"));
    }
    r = znajdz(p, "C1");
    CHECK(r != nullptr);
    if (r) CHECK_EQ(r->ilosc, 5);

    WynikImportuBOM zly;
    CHECK(!ImportBOM::parsujTekst("coś;innego\n1;2\n", zly, blad));
    CHECK(!blad.empty());
    CHECK(!ImportBOM::parsujTekst("", zly, blad));
    CHECK(!ImportBOM::parsujTekst("(kicad_sch (version 2023))", zly, blad));
    CHECK(blad.find("netlist") != std::string::npos || blad.find("Netlist") != std::string::npos ||
          blad.find("list") != std::string::npos);
}

static const char* KICAD_NETLISTA =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<export version=\"E\"><design><source>x.kicad_sch</source></design>\n"
    "<components>\n"
    "<comp ref=\"R1\"><value>10k</value><footprint>Resistor_SMD:R_0805_2012Metric</footprint></comp>\n"
    "<comp ref=\"R2\"><value>10k</value><footprint>Resistor_SMD:R_0805_2012Metric</footprint></comp>\n"
    "<comp ref=\"U1\"><value>NE555</value><footprint>Package_DIP:DIP-8_W7.62mm</footprint>\n"
    "  <fields><field name=\"MPN\">NE555P</field><field name=\"Manufacturer\">TI</field></fields></comp>\n"
    "<comp ref=\"TP1\"><value>TP</value><footprint>TestPoint:TestPoint_Pad_D1.0mm</footprint>\n"
    "  <property name=\"exclude_from_bom\"/></comp>\n"
    "<comp ref=\"C9\"><value>1u</value><footprint>Capacitor_SMD:C_0805_2012Metric</footprint>\n"
    "  <property name=\"dnp\"/></comp>\n"
    "</components></export>\n";

static void testKiCadNetlista() {
    std::cout << "[kicad xml]\n";
    WynikImportuBOM w;
    std::string blad;
    CHECK(ImportBOM::parsujTekst(KICAD_NETLISTA, w, blad));
    CHECK_EQ(w.pozycje.size(), size_t(2));
    const PozycjaBOM* r = znajdz(w, "R1");
    CHECK(r != nullptr);
    if (r) {
        CHECK_EQ(r->ilosc, 2);
        CHECK_EQ(r->oznaczenia, std::string("R1, R2"));
    }
    const PozycjaBOM* u = znajdz(w, "U1");
    CHECK(u != nullptr);
    if (u) {
        CHECK_EQ(u->symbol, std::string("NE555P"));
        CHECK_EQ(u->producent, std::string("TI"));
    }
    CHECK(znajdz(w, "TP1") == nullptr);
    CHECK(znajdz(w, "C9") == nullptr);
    CHECK(w.format.find("KiCad") != std::string::npos);
}

static const char* EAGLE_SCH =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!DOCTYPE eagle SYSTEM \"eagle.dtd\">\n"
    "<eagle version=\"9.6.2\"><drawing><schematic>\n"
    "<libraries>\n"
    "<library name=\"rcl\"><packages/><symbols/><devicesets>\n"
    "  <deviceset name=\"R-EU_\" prefix=\"R\"><devices>\n"
    "    <device name=\"R0805\" package=\"R0805\"/><device name=\"R0603\" package=\"R0603\"/></devices></deviceset>\n"
    "  <deviceset name=\"GND\" prefix=\"\"><devices><device name=\"\"/></devices></deviceset>\n"
    "</devicesets></library>\n"
    "<library name=\"linear\"><devicesets>\n"
    "  <deviceset name=\"LM358\" prefix=\"IC\"><devices><device name=\"D\" package=\"SO08\"/></devices></deviceset>\n"
    "</devicesets></library>\n"
    "</libraries>\n"
    "<parts>\n"
    "<part name=\"R1\" library=\"rcl\" deviceset=\"R-EU_\" device=\"R0805\" value=\"10k\"/>\n"
    "<part name=\"R2\" library=\"rcl\" deviceset=\"R-EU_\" device=\"R0805\" value=\"10k\"/>\n"
    "<part name=\"R3\" library=\"rcl\" deviceset=\"R-EU_\" device=\"R0603\" value=\"4k7\"/>\n"
    "<part name=\"GND1\" library=\"rcl\" deviceset=\"GND\" device=\"\"/>\n"
    "<part name=\"IC1\" library=\"linear\" deviceset=\"LM358\" device=\"D\">\n"
    "  <attribute name=\"MPN\" value=\"LM358DR\"/><attribute name=\"MANUFACTURER\" value=\"TI\"/></part>\n"
    "</parts></schematic></drawing></eagle>\n";

static void testEagle() {
    std::cout << "[eagle]\n";
    WynikImportuBOM w;
    std::string blad;
    CHECK(ImportBOM::parsujTekst(EAGLE_SCH, w, blad));
    CHECK(w.format.find("Eagle") != std::string::npos);
    CHECK_EQ(w.pozycje.size(), size_t(3));  // R 10k 0805 x2, R 4k7 0603, IC1; bez GND
    const PozycjaBOM* r = znajdz(w, "R1");
    CHECK(r != nullptr);
    if (r) {
        CHECK_EQ(r->ilosc, 2);
        CHECK_EQ(r->obudowa, std::string("R0805"));
    }
    const PozycjaBOM* ic = znajdz(w, "IC1");
    CHECK(ic != nullptr);
    if (ic) {
        CHECK_EQ(ic->symbol, std::string("LM358DR"));
        CHECK_EQ(ic->producent, std::string("TI"));
        CHECK_EQ(ic->obudowa, std::string("SO08"));
    }
    CHECK(znajdz(w, "GND") == nullptr);
}


static void testBibliotekiNarodowej() {
    CHECK_EQ(BibliotekaNarodowa::rokZTekstu("(c) 2019 r."), 2019);
    CHECK_EQ(BibliotekaNarodowa::rokZTekstu("wyd. 2, [2004]"), 2004);
    CHECK_EQ(BibliotekaNarodowa::rokZTekstu("abc"), 0);
    CHECK_EQ(BibliotekaNarodowa::rokZTekstu("12345"), 0);
    CHECK_EQ(BibliotekaNarodowa::rokZTekstu("  1999"), 1999);
    CHECK_EQ(BibliotekaNarodowa::czystyISBN("978-83-01-13908-7"), std::string("9788301139087"));
    CHECK_EQ(BibliotekaNarodowa::czystyISBN("ISBN 83-01-13908-X"), std::string());
    CHECK_EQ(BibliotekaNarodowa::czystyISBN("0-306-40615-2 (oprawa)"), std::string("0306406152"));
    CHECK_EQ(BibliotekaNarodowa::czystyISBN("123"), std::string());
    CHECK(BibliotekaNarodowa::adresZapytania("9788301139087", "", "").find("isbnIssn=9788301139087") != std::string::npos);
    const std::string a = BibliotekaNarodowa::adresZapytania("", "Sztuka elektroniki", "Horowitz");
    CHECK(a.find("title=Sztuka%20elektroniki") != std::string::npos);
    CHECK(a.find("author=Horowitz") != std::string::npos);
    {   // polskie znaki muszą być zakodowane niezależnie od lokalizacji
        const char* stare = setlocale(LC_ALL, nullptr);
        const std::string zapisana = stare ? stare : "C";
        for (const char* l : {"pl_PL.UTF-8", "pl_PL.ISO-8859-2", "pl_PL"}) setlocale(LC_ALL, l);
        const std::string u = BibliotekaNarodowa::adresZapytania("", "Urządzenia elektroakustyczne", "Łukasz");
        CHECK(u.find("title=Urz%C4%85dzenia%20elektroakustyczne") != std::string::npos);
        CHECK(u.find("author=%C5%81ukasz") != std::string::npos);
        setlocale(LC_ALL, zapisana.c_str());
    }

    const std::string prosta =
        R"({"nextPage":"x","bibs":[{"id":1,"title":"Sztuka elektroniki /","author":"Horowitz, Paul.","publisher":"WKiŁ","publicationYear":"2013","language":"polski","isbnIssn":"978-83-206-1500-0"}]})";
    std::vector<MetadaneKsiazek::Dane> w;
    std::string blad;
    CHECK(BibliotekaNarodowa::zOdpowiedzi(prosta, w, blad));
    CHECK_EQ(w.size(), static_cast<size_t>(1));
    if (!w.empty()) {
        CHECK_EQ(w[0].tytul, std::string("Sztuka elektroniki"));
        CHECK_EQ(w[0].autor, std::string("Horowitz, Paul"));
        CHECK_EQ(w[0].rok, 2013);
        CHECK_EQ(w[0].isbn, std::string("9788320615000"));
    }
    const std::string marc =
        R"({"bibs":[{"marc":{"leader":"x","fields":[{"020":{"ind1":" ","ind2":" ","subfields":[{"a":"9788301139087"}]}},)"
        R"({"100":{"subfields":[{"a":"Kowalski, J."}]}},{"245":{"subfields":[{"a":"Wzmacniacze lampowe :"},{"b":"projekty /"}]}},)"
        R"({"250":{"subfields":[{"a":"Wyd. 2 poprawione."}]}},{"260":{"subfields":[{"b":"Helion,"},{"c":"2015."}]}},{"041":{"subfields":[{"a":"pol"}]}}]}}]})";
    CHECK(BibliotekaNarodowa::zOdpowiedzi(marc, w, blad));
    CHECK_EQ(w.size(), static_cast<size_t>(1));
    if (!w.empty()) {
        CHECK_EQ(w[0].tytul, std::string("Wzmacniacze lampowe: projekty"));
        CHECK_EQ(w[0].autor, std::string("Kowalski, J."));
        CHECK_EQ(w[0].wydawca, std::string("Helion"));
        CHECK_EQ(w[0].rok, 2015);
        CHECK_EQ(w[0].wydanie, std::string("Wyd. 2 poprawione"));
        CHECK_EQ(w[0].isbn, std::string("9788301139087"));
        CHECK(!w[0].jezyk.empty());
    }
    // rzeczywisty kształt odpowiedzi BN: pola proste sklejone, dane czyste tylko w MARC; ten sam rekord dwa razy
    {
        const std::string rekord =
            R"j({"language":"polski","isbnIssn":"","author":"Miszczak, Stanisław (1923-2007) Wydawnictwa Komunikacji i Łączności",)j"
            R"j("title":"Urządzenia elektroakustyczne /","publisher":"Wydawnictwa Komunikacji i Łączności Wydawnictwa Komunikacji i Łączności,","publicationYear":"1964",)j"
            R"j("marc":{"fields":[{"100":{"subfields":[{"a":"Miszczak, Stanisław"},{"d":"(1923-2007)"}]}},{"245":{"subfields":[{"a":"Urządzenia elektroakustyczne /"},{"c":"Stanisław Miszczak."}]}},)j"
            R"j({"260":{"subfields":[{"a":"Warszawa :"},{"b":"Wydawnictwa Komunikacji i Łączności,"},{"c":"1964."}]}}]}})j";
        CHECK(BibliotekaNarodowa::zOdpowiedzi(R"j({"bibs":[)j" + rekord + "," + rekord + "]}", w, blad));
        CHECK_EQ(w.size(), static_cast<size_t>(1));
        if (!w.empty()) {
            CHECK_EQ(w[0].tytul, std::string("Urządzenia elektroakustyczne"));
            CHECK_EQ(w[0].autor, std::string("Miszczak, Stanisław"));
            CHECK_EQ(w[0].wydawca, std::string("Wydawnictwa Komunikacji i Łączności"));
            CHECK_EQ(w[0].rok, 1964);
        }
        // bez MARC: pola proste są czyszczone
        const std::string bezMarc =
            R"j({"bibs":[{"title":"X /","author":"Miszczak, Stanisław (1923-2007) WKŁ","publisher":"WKŁ WKŁ,","publicationYear":"1964"}]})j";
        CHECK(BibliotekaNarodowa::zOdpowiedzi(bezMarc, w, blad) && w.size() == 1);
        if (!w.empty()) { CHECK_EQ(w[0].autor, std::string("Miszczak, Stanisław")); CHECK_EQ(w[0].wydawca, std::string("WKŁ")); }
    }
    CHECK(BibliotekaNarodowa::zOdpowiedzi(R"({"bibs":[]})", w, blad));
    CHECK(w.empty());
    CHECK(!BibliotekaNarodowa::zOdpowiedzi("to nie json", w, blad));
}

static void testMouser() {
    using namespace Mouser;
    CHECK_EQ(adresZapytania(" 00000000-1111-2222-3333-444444444444 "), std::string("https://api.mouser.com/api/v1/search/keyword?apiKey=00000000-1111-2222-3333-444444444444"));
    CHECK(czyKluczPoprawny("00000000-1111-2222-3333-444444444444") && !czyKluczPoprawny("00000000") && !czyKluczPoprawny("zz000000-1111-2222-3333-444444444444"));
    const std::string cialo = cialoZapytania("LM358 \"x\"\n", 99);
    CHECK_EQ(cialo, std::string("{\"SearchByKeywordRequest\":{\"keyword\":\"LM358 \\\"x\\\"\",\"records\":50,\"startingRecord\":0,\"searchOptions\":\"None\",\"searchWithYourSignUpLanguage\":\"false\"}}"));
    JsonWartosc jw; std::string jb;
    CHECK(MiniJSON::parsuj(cialo, jw, jb));
    CHECK_EQ(poprawAdres("//www.mouser.com/datasheet/2/405/lm 358.pdf"), std::string("https://www.mouser.com/datasheet/2/405/lm%20358.pdf"));
    CHECK_EQ(poprawAdres("https://x.pl/a.pdf"), std::string("https://x.pl/a.pdf"));
    CHECK_EQ(poprawAdres("ftp://x/a.pdf"), std::string());
    CHECK_EQ(poprawAdres("  "), std::string());

    // odpowiedź w kształcie dokumentowanym przez Mouser (pola ProductAttributes, DataSheetUrl itd.)
    const std::string odp = R"j({"Errors":[],"SearchResults":{"NumberOfResult":3,"Parts":[
      {"Availability":"12000 In Stock","DataSheetUrl":"https://www.mouser.com/datasheet/2/405/lm358-1116230.pdf","Description":"Operational Amplifiers - Op Amps Dual Op Amp",
       "Category":"Op Amps","LifecycleStatus":"Active","Manufacturer":"Texas Instruments","ManufacturerPartNumber":"LM358DR","MouserPartNumber":"595-LM358DR",
       "ProductDetailUrl":"https://www.mouser.com/ProductDetail/Texas-Instruments/LM358DR?qs=abc",
       "ProductAttributes":[{"AttributeName":"Packaging","AttributeValue":"Reel"},{"AttributeName":"Package / Case","AttributeValue":"SOIC-8"},{"AttributeName":"Supply Voltage","AttributeValue":"3 V to 32 V"}]},
      {"DataSheetUrl":"","Description":"Dual Op Amp","Manufacturer":"ON Semiconductor","ManufacturerPartNumber":"LM358DG","MouserPartNumber":"863-LM358DG","ProductAttributes":[{"AttributeName":"Opakowanie","AttributeValue":"Tuba"},{"AttributeName":"Opakowanie / Obudowa","AttributeValue":"DIP-8"}]},
      {"Description":"bez numerów"}]}})j";
    std::vector<Czesc> c; std::string blad;
    CHECK(zOdpowiedzi(odp, c, blad));
    CHECK_EQ(c.size(), static_cast<size_t>(2));   // trzecia pozycja bez numerów jest pomijana
    if (c.size() == 2) {
        CHECK_EQ(c[0].numerProducenta, std::string("LM358DR"));
        CHECK_EQ(c[0].producent, std::string("Texas Instruments"));
        CHECK_EQ(c[0].adresDatasheetu, std::string("https://www.mouser.com/datasheet/2/405/lm358-1116230.pdf"));
        CHECK_EQ(c[0].obudowa(), std::string("SOIC-8"));      // atrybut "Packaging" (taśma) nie jest obudową
        CHECK_EQ(c[0].atrybuty.size(), static_cast<size_t>(3));
        CHECK_EQ(c[0].cyklZycia, std::string("Active"));
        CHECK(c[1].adresDatasheetu.empty());
        CHECK_EQ(c[1].obudowa(), std::string("DIP-8"));       // wersja polska: "Opakowanie / Obudowa"
    }
    // błąd klucza i inne błędy
    CHECK(!zOdpowiedzi(R"j({"Errors":[{"Id":0,"Code":"Invalid","Message":"Invalid unique identifier.","PropertyName":"API Key"}],"SearchResults":null})j", c, blad));
    CHECK(blad.find("klucz") != std::string::npos && c.empty());
    CHECK(!zOdpowiedzi(R"j({"Errors":[{"Code":"TooManyRequests","Message":"Too many requests"}]})j", c, blad) && blad.find("Too many requests") != std::string::npos);
    CHECK(!zOdpowiedzi("<html>", c, blad) && !blad.empty());
    CHECK(zOdpowiedzi(R"j({"Errors":[],"SearchResults":{"NumberOfResult":0,"Parts":[]}})j", c, blad) && c.empty());   // brak trafień to nie błąd
    CHECK(zOdpowiedzi(R"j({"Errors":[],"SearchResults":{"NumberOfResult":0}})j", c, blad) && c.empty());
}

static void testPolecenKsiazek() {
    using namespace PolecenieKsiazek;
    // polecenie czytnika PDF: %f, %u (URL), %p
    CHECK_EQ(kodujSciezkeURL("/Users/Jan/Moje książki/a b.pdf"), std::string("/Users/Jan/Moje%20ksi%C4%85%C5%BCki/a%20b.pdf"));
    CHECK_EQ(kodujSciezkeURL("C:\\Ks\\a.pdf"), std::string("/C:/Ks/a.pdf"));
    CHECK_EQ(wypelnijPolecenieCzytnika("open \"skim://%u#page=%p\"", "/Users/Jan/a b.pdf", 7), std::string("open \"skim:///Users/Jan/a%20b.pdf#page=7\""));
    CHECK_EQ(wypelnijPolecenieCzytnika("okular --page %p \"%f\" 100%", "/x/a b.pdf", 3), std::string("okular --page 3 \"/x/a b.pdf\" 100%"));
    CHECK_EQ(rozszerzPATH("/usr/bin:/bin", {"/opt/homebrew/bin", "/usr/bin"}), std::string("/usr/bin:/bin:/opt/homebrew/bin"));
    CHECK_EQ(rozszerzPATH("", {"/usr/local/bin"}), std::string("/usr/local/bin"));
    CHECK_EQ(rozszerzPATH("/usr/local/bin2", {"/usr/local/bin"}), std::string("/usr/local/bin2:/usr/local/bin"));
    CHECK_EQ(kodJezyka("polski"), std::string("pl"));
    CHECK_EQ(kodJezyka("Angielski"), std::string("en"));
    CHECK_EQ(kodJezyka("pl"), std::string("pl"));
    CHECK_EQ(kodJezyka("klingon"), std::string());
    Ksiazka k;
    k.tytul = "Urządzenia elektroakustyczne"; k.autor = "Miszczak, Stanisław"; k.wydawca = "WKŁ"; k.rok = 1964; k.isbn = "9788320615000"; k.wydanie = "wydanie drugie";
    k.jezyk = "polski"; k.tagi = "lampy; audio, radio"; k.uwagi = "<b>Linia 1</b><br>Linia 2";
    const auto l = liniePlikuExifTool(k, "C:\\Ks\\a b.pdf");
    auto ma = [&](const std::string& s) { for (const auto& x : l) if (x == s) return true; return false; };
    CHECK(ma("-XMP-dc:Title=Urządzenia elektroakustyczne") && ma("-PDF:Title=Urządzenia elektroakustyczne"));
    CHECK(ma("-XMP-dc:Creator=Miszczak, Stanisław") && ma("-XMP-dc:Publisher=WKŁ") && ma("-XMP-dc:Date=1964") && ma("-XMP-dc:Language=pl"));
    CHECK(ma("-XMP-dc:Identifier=urn:isbn:9788320615000") && ma("-XMP-prism:Edition=wydanie drugie"));
    CHECK(ma("-XMP-dc:Subject=") && ma("-XMP-dc:Subject+=lampy") && ma("-XMP-dc:Subject+=audio") && ma("-XMP-dc:Subject+=radio") && ma("-PDF:Keywords=lampy, audio, radio"));
    CHECK(ma("-XMP-dc:Description=Linia 1 Linia 2"));     // uwagi HTML zamienione na tekst w jednej linii
    CHECK_EQ(l.back(), std::string("C:\\Ks\\a b.pdf"));
    // puste pola nie trafiają do pliku (nie kasują istniejących danych)
    Ksiazka pusta; pusta.tytul = "Tylko tytuł";
    const auto lp = liniePlikuExifTool(pusta, "x.pdf");
    CHECK_EQ(lp.size(), static_cast<size_t>(3));           // XMP Title, PDF Title, ścieżka
    // nic z wartości nie może zacząć nowej linii pliku argumentów
    Ksiazka n; n.tytul = "a\n-overwrite_original\n"; n.uwagi = "x\r\ny";
    for (const auto& x : liniePlikuExifTool(n, "p.pdf")) CHECK(x.find('\n') == std::string::npos && x.find('\r') == std::string::npos);
    const auto a = argumentyExifTool("args.txt", true);
    CHECK(a.size() == 6 && a.back() == "args.txt" && a[4] == "-@");
    CHECK(argumentyExifTool("args.txt", false)[4] == "-overwrite_original");
    CHECK_EQ(sciezkaWynikuOcr("C:\\Ks\\a.pdf"), std::string("C:\\Ks\\a_ocr.pdf"));
    CHECK_EQ(sciezkaWynikuOcr("/x.y/a"), std::string("/x.y/a_ocr.pdf"));
    CHECK(poprawneJezykiOcr("pol+eng") && poprawneJezykiOcr("pol") && !poprawneJezykiOcr("") && !poprawneJezykiOcr("pol --evil") && !poprawneJezykiOcr("+pol"));
    const auto o = argumentyOcr("in.pdf", "out.pdf", "pol+eng");
    CHECK(o.size() == 8 && o[1] == "pol+eng" && o[o.size() - 2] == "in.pdf" && o.back() == "out.pdf" && o[o.size() - 3] == "--");
}

static void testIndeksow() {
    CHECK_EQ(Indeksy::naHtml("V_CE"), std::string("V<sub>CE</sub>"));
    CHECK_EQ(Indeksy::naHtml("V_{CE(sat)}"), std::string("V<sub>CE(sat)</sub>"));
    CHECK_EQ(Indeksy::naHtml("m^2"), std::string("m<sup>2</sup>"));
    CHECK_EQ(Indeksy::naHtml("10^-3"), std::string("10<sup>-3</sup>"));
    CHECK_EQ(Indeksy::naHtml("a<b&c"), std::string("a&lt;b&amp;c"));
    CHECK_EQ(Indeksy::naHtml("a\\_b"), std::string("a_b"));
    CHECK(Indeksy::maIndeksy("V_CE"));
    CHECK(!Indeksy::maIndeksy("BC547"));
    CHECK_EQ(Indeksy::naTekst("m^2"), std::string("m\xC2\xB2"));
}

int main() {
    testTekstuEPUB();
    testGrupowaniaZapisow();
    testCSV();
    testXML();
    testNormalizacji();
    testCSVBOM();
    testKiCadNetlista();
    testEagle();
    testBibliotekiNarodowej();
    testMouser();
    testIndeksow();
    testPolecenKsiazek();
    std::cout << sprawdzenia << " sprawdzeń, " << bledy << " błędów\n";
    return bledy == 0 ? 0 : 1;
}
