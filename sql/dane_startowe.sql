-- =====================================================================
-- Magmass - dane startowe (sklepy, kategorie)
-- Skrypt jest idempotentny: nic nie dubluje i nie nadpisuje zmian.
-- =====================================================================

INSERT INTO tbl_sklepy (nazwa, opis, url_bazowy, kraj, waluta, priorytet, parametr_wyszukiwania)
SELECT v.nazwa, v.opis, v.url_bazowy, v.kraj, v.waluta, v.priorytet, v.parametr
FROM (VALUES
    ('TME', 'Transfer Multisort Elektronik', 'https://www.tme.eu/pl/katalog/', 'Polska', 'PLN', 1, 'szukaj={KOD}/'),
    ('Allegro', 'Allegro.pl', 'https://allegro.pl/listing?string=', 'Polska', 'PLN', 2, '{KOD}'),
    ('AliExpress', 'AliExpress', 'https://pl.aliexpress.com/wholesale?catId=0&SearchText=', 'China', 'USD', 3, '{KOD}'),
    ('Farnell', 'Farnell/Element14', 'https://pl.farnell.com/search?st=', 'Polska', 'PLN', 4, '{KOD}'),
    ('Mouser', 'Mouser Electronics', 'https://www.mouser.pl/Search/Refine?Keyword=', 'USA', 'USD', 5, '{KOD}'),
    ('DigiKey', 'Digi-Key Electronics', 'https://www.digikey.pl/products/en?keywords=', 'USA', 'USD', 6, '{KOD}'),
    ('RS Components', 'RS Online', 'https://pl.rs-online.com/web/c/', 'Polska', 'PLN', 7, '{KOD}'),
    ('Conrad', 'Conrad Electronic', 'https://www.conrad.pl/', 'Polska', 'PLN', 8, '{KOD}'),
    ('Botland', 'Botland', 'https://botland.com.pl/', 'Polska', 'PLN', 9, '{KOD}'),
    ('AVT', 'AVT Kits', 'https://sklep.avt.pl/', 'Polska', 'PLN', 10, '{KOD}')
) AS v(nazwa, opis, url_bazowy, kraj, waluta, priorytet, parametr)
WHERE NOT EXISTS (SELECT 1 FROM tbl_sklepy s WHERE lower(s.nazwa) = lower(v.nazwa));

-- Uwaga: adresy wyszukiwania sklepów (url_bazowy + parametr_wyszukiwania) pochodzą
-- z pierwotnego schematu i nie były weryfikowane. Popraw je w tabeli tbl_sklepy.

-- Kategorie startowe
DO $$
DECLARE
    r bigint;
    p bigint;
BEGIN
    r := fn_dodaj_kategorie_jesli_brak(NULL, 'Elementy bierne');
        p := fn_dodaj_kategorie_jesli_brak(r, 'Rezystory');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Rezystory THT');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Rezystory SMD');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Potencjometry i trymery');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Termistory i warystory');
        p := fn_dodaj_kategorie_jesli_brak(r, 'Kondensatory');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Elektrolityczne');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Ceramiczne');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Foliowe');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Tantalowe');
        p := fn_dodaj_kategorie_jesli_brak(r, 'Cewki i dławiki');
        p := fn_dodaj_kategorie_jesli_brak(r, 'Kwarce i rezonatory');
        p := fn_dodaj_kategorie_jesli_brak(r, 'Bezpieczniki');

    r := fn_dodaj_kategorie_jesli_brak(NULL, 'Półprzewodniki');
        p := fn_dodaj_kategorie_jesli_brak(r, 'Diody');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Diody prostownicze');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Diody Zenera');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Diody Schottky''ego');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Diody LED');
        p := fn_dodaj_kategorie_jesli_brak(r, 'Tranzystory');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'Bipolarne');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'MOSFET');
            PERFORM fn_dodaj_kategorie_jesli_brak(p, 'IGBT');
        p := fn_dodaj_kategorie_jesli_brak(r, 'Tyrystory i triaki');
        p := fn_dodaj_kategorie_jesli_brak(r, 'Transoptory');

    r := fn_dodaj_kategorie_jesli_brak(NULL, 'Układy scalone');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Mikrokontrolery');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Stabilizatory napięcia');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Wzmacniacze operacyjne');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Logika');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Pamięci');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Interfejsy i komunikacja');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Przetworniki ADC/DAC');

    r := fn_dodaj_kategorie_jesli_brak(NULL, 'Złącza i elektromechanika');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Złącza i listwy');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Przełączniki i przyciski');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Przekaźniki');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Gniazda i podstawki');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Przewody i kable');

    r := fn_dodaj_kategorie_jesli_brak(NULL, 'Moduły i płytki');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Płytki rozwojowe');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Moduły czujników');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Moduły zasilające');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Wyświetlacze');

    r := fn_dodaj_kategorie_jesli_brak(NULL, 'Zasilanie');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Baterie i akumulatory');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Zasilacze');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Transformatory');

    r := fn_dodaj_kategorie_jesli_brak(NULL, 'Mechanika i obudowy');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Obudowy');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Śrubki, dystanse, wsporniki');
        PERFORM fn_dodaj_kategorie_jesli_brak(r, 'Radiatory');

    PERFORM fn_dodaj_kategorie_jesli_brak(NULL, 'Narzędzia i materiały');
    PERFORM fn_dodaj_kategorie_jesli_brak(NULL, 'Inne');
END
$$;

-- Wersja 3: szablony wyszukiwania sklepów ({q} = szukana fraza) i przykładowe kursy.
-- Adresy NIE były sprawdzane na działających sklepach (sklepy zmieniają strony) - popraw w oknie Sklepy.
UPDATE tbl_sklepy SET szablon_wyszukiwania = v.szablon
FROM (VALUES
    ('TME', 'https://www.tme.eu/pl/katalog/?search={q}'),
    ('Allegro', 'https://allegro.pl/listing?string={q}'),
    ('AliExpress', 'https://pl.aliexpress.com/wholesale?SearchText={q}'),
    ('Farnell', 'https://pl.farnell.com/search?st={q}'),
    ('Mouser', 'https://www.mouser.pl/c/?q={q}'),
    ('DigiKey', 'https://www.digikey.pl/pl/products/result?keywords={q}'),
    ('RS Components', 'https://pl.rs-online.com/web/c/?searchTerm={q}'),
    ('Conrad', 'https://www.conrad.pl/pl/search.html?search={q}'),
    ('Botland', 'https://botland.com.pl/szukaj?controller=search&s={q}'),
    ('AVT', 'https://sklep.avt.pl/szukaj.html?q={q}')
) AS v(nazwa, szablon)
WHERE lower(tbl_sklepy.nazwa) = lower(v.nazwa) AND tbl_sklepy.szablon_wyszukiwania IS NULL;

-- Kursy tylko przykładowe - zaktualizuj w oknie Sklepy (przycisk Kursy walut)
INSERT INTO tbl_kursy_walut (waluta, kurs_pln)
SELECT v.w, v.k FROM (VALUES ('PLN', 1.0), ('EUR', 4.30), ('USD', 4.00), ('GBP', 5.00)) AS v(w, k)
WHERE NOT EXISTS (SELECT 1 FROM tbl_kursy_walut k WHERE k.waluta = v.w);
