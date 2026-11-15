-- =====================================================================
-- Magmass - struktura bazy (tabele, indeksy)
-- Skrypt jest idempotentny: można go uruchamiać wielokrotnie.
-- Kolejność skryptów: migracja_nazw.sql -> schema.sql -> funkcje_i_trigger.sql -> widoki.sql
--                     -> dane_startowe.sql -> pomoc_pola.sql -> opisy_obiektow.sql
-- Konwencja nazw (od wersji 13): tbl_<tabela>, wid_<widok>, fn_<funkcja>, trg_<tabela>_<opis>,
-- idx_/uq_/chk_<tabela>_<opis>; znaczniki czasu: dodano, zmieniono; autor: uzytkownik_dod, uzytkownik_mod.
-- Zgodność: PostgreSQL 9.6 lub nowszy.
-- Ręcznie:  psql -U postgres -d NAZWA_BAZY -f schema.sql
-- (aplikacja robi to sama: menu Baza danych -> Utwórz/aktualizuj strukturę)
-- =====================================================================

-- Rozszerzenie ltree (drzewo kategorii). Instalacja wymaga uprawnienia CREATE
-- na bazie (od PG 13 rozszerzenie jest "trusted"; wcześniej trzeba być superuserem).
-- Jeśli użytkownik aplikacji go nie ma, jednorazowo jako administrator:
--   psql -U postgres -d NAZWA_BAZY -c "CREATE EXTENSION IF NOT EXISTS ltree;"
DO $$
BEGIN
    CREATE EXTENSION IF NOT EXISTS ltree;
EXCEPTION WHEN insufficient_privilege THEN
    RAISE EXCEPTION 'Brak uprawnień do utworzenia rozszerzenia ltree. Jako administrator bazy wykonaj: CREATE EXTENSION IF NOT EXISTS ltree;  (baza: %)', current_database();
END $$;

-- Wersja struktury bazy (do przyszłych migracji)
CREATE TABLE IF NOT EXISTS public.tbl_wersja_schematu (
    wersja integer primary key,
    opis character varying(500),
    data_zastosowania timestamp default current_timestamp
);

CREATE TABLE IF NOT EXISTS public.tbl_opis_slownika (
    id bigserial primary key,
    nazwa_slownika character varying(100),
    opis character varying(4000),
    kod_slownika character varying(6),
    tekst_01 character varying(100),
    tekst_02 character varying(100),
    tekst_03 character varying(100),
    tekst_04 character varying(100),
    tekst_05 character varying(100),
    tekst_06 character varying(100),
    tekst_07 character varying(100),
    tekst_08 character varying(100),
    tekst_09 character varying(100),
    tekst_10 character varying(100),
    tekst_11 character varying(2000),
    tekst_12 character varying(2000),
    liczba_01 character varying(100),
    liczba_02 character varying(100),
    liczba_03 character varying(100),
    liczba_04 character varying(100),
    liczba_05 character varying(100),
    liczba_06 character varying(100),
    liczba_07 character varying(100),
    liczba_08 character varying(100),
    liczba_09 character varying(100),
    liczba_10 character varying(100),
    rzeczywista_01 character varying(100),
    rzeczywista_02 character varying(100),
    rzeczywista_03 character varying(100),
    rzeczywista_04 character varying(100),
    rzeczywista_05 character varying(100),
    dziesietna_01 character varying(100),
    dziesietna_02 character varying(100),
    czy_aktywny integer,
    aktywny_od date,
    wersja_aktywna real,
    widoczny integer,
    uzytkownik_mod character varying(20),
    dodano time without time zone,
    uzytkownik_dod character varying(20),
    zmieniono timestamp without time zone,
    data_01 date,
    data_02 date,
    flaga_01 boolean DEFAULT false,
    flaga_02 boolean DEFAULT false,
    flaga_03 boolean DEFAULT false,
    uwagi character varying(6000)
);


-- tabela słowników (taka sama jak wcześniej)
CREATE TABLE IF NOT EXISTS public.tbl_slownik (
    id bigserial primary key,
    id_opis bigint references tbl_opis_slownika(id) match simple on update cascade,
    tekst_01 character varying(100),
    tekst_02 character varying(100),
    tekst_03 character varying(100),
    tekst_04 character varying(2000),
    tekst_05 character varying(4000),
    tekst_06 character varying(4000),
    tekst_07 character varying(100),
    tekst_08 character varying(100),
    tekst_09 character varying(100),
    tekst_10 character varying(100),
    tekst_11 character varying(2000),
    tekst_12 character varying(2000),
    liczba_01 bigint,
    liczba_02 bigint,
    liczba_03 bigint,
    liczba_04 bigint,
    liczba_05 bigint,
    liczba_06 bigint,
    liczba_07 bigint,
    liczba_08 bigint,
    liczba_09 bigint,
    liczba_10 bigint,
    rzeczywista_01 real,
    rzeczywista_02 real,
    rzeczywista_03 real,
    rzeczywista_04 real,
    rzeczywista_05 real,
    rzeczywista_06 real,
    rzeczywista_07 real,
    rzeczywista_08 real,
    rzeczywista_09 real,
    rzeczywista_10 real,
    dziesietna_01 numeric(10,2),
    dziesietna_02 numeric(10,2),
    wersja real,
    aktywny integer,
    aktywny_od date,
    widoczny integer,
    uzytkownik_mod character varying(20),
    uzytkownik_dod character varying(20),
    dodano timestamp without time zone,
    zmieniono timestamp without time zone,
    data_01 date,
    data_02 date,
    flaga_01 boolean DEFAULT false,
    flaga_02 boolean DEFAULT false,
    flaga_03 boolean DEFAULT false,
    uwagi character varying(6000)
);


-- Pozostałe tabele bez zmian
CREATE TABLE IF NOT EXISTS public.tbl_kategorie (
    id bigserial primary key,
    id_rodzica bigint references tbl_kategorie(id),
    nazwa character varying(200) not null,
    opis character varying(2000),
    sciezka ltree,
    poziom integer not null default 0,
    czy_aktywny boolean default true,
    dodano timestamp default current_timestamp,
    uzytkownik_dod character varying(50),
    zmieniono timestamp default current_timestamp,
    uzytkownik_mod character varying(50)
);

-- Tabela sklepów (nowa, elastyczna struktura)
CREATE TABLE IF NOT EXISTS public.tbl_sklepy (
    id bigserial primary key,
    nazwa character varying(200) not null,
    opis character varying(2000),
    url_bazowy character varying(500) not null,
    logo_url character varying(500),
    kraj character varying(100),
    waluta character varying(3),
    czy_aktywny boolean default true,
    priorytet integer default 5,
    parametr_wyszukiwania character varying(100),
    czy_wspiera_api boolean default false,
    api_klucz character varying(200),
    uzytkownik_dod character varying(50),
    dodano timestamp default current_timestamp,
    zmieniono timestamp default current_timestamp,
    uzytkownik_mod character varying(50)
);

COMMENT ON TABLE public.tbl_sklepy IS 'Tabela sklepów z częściami elektronicznymi';
COMMENT ON COLUMN public.tbl_sklepy.parametr_wyszukiwania IS 'Parametr URL do wyszukiwania (np. MPN, kod producenta)';

-- Aktualizowana tabela z elementami (bez kolumn sklepów)
CREATE TABLE IF NOT EXISTS public.tbl_elementy (
    id bigserial primary key,
    id_kategorii bigint references tbl_kategorie(id),
    symbol character varying(200) not null,
    symbol_producenta character varying(200),
    producent character varying(200),
    opis character varying(2000),
    wartosc character varying(100),
    jednostka character varying(20),
    obudowa character varying(100),
    datasheet_url character varying(500),
    zdjecie_sciezka character varying(500),
    ilosc_dostepna integer default 0 check (ilosc_dostepna >= 0),
    ilosc_minimalna integer default 0 check (ilosc_minimalna >= 0),
    cena_jednostkowa numeric(12,4),
    waluta character varying(3) default 'PLN',
    parametry_json jsonb,
    tagi character varying(1000),
    czy_aktywny boolean default true,
    dodano timestamp default current_timestamp,
    zmieniono timestamp default current_timestamp,
    uzytkownik_dod character varying(50),
    uzytkownik_mod character varying(50)
);

-- Tabela linków do sklepów dla elementów (nowa, elastyczna)
CREATE TABLE IF NOT EXISTS public.tbl_elementy_sklepy (
    id bigserial primary key,
    id_elementu bigint references tbl_elementy(id) on delete cascade,
    id_sklepu bigint references tbl_sklepy(id),
    url character varying(1000) not null,
    cena_aktualna numeric(12,4),
    waluta character varying(3),
    ostatnia_weryfikacja timestamp,
    status_dostepnosci character varying(50),
    minimalna_ilosc integer,
    czy_domyslny boolean default false,
    uwagi character varying(2000),
    dodano timestamp default current_timestamp,
    zmieniono timestamp default current_timestamp,
    CONSTRAINT uq_tbl_elementy_sklepy_element_sklep UNIQUE (id_elementu, id_sklepu)
);

CREATE INDEX IF NOT EXISTS idx_tbl_elementy_sklepy_id_elementu ON public.tbl_elementy_sklepy(id_elementu);
CREATE INDEX IF NOT EXISTS idx_tbl_elementy_sklepy_id_sklepu ON public.tbl_elementy_sklepy(id_sklepu);

-- Tabela historii cen w sklepach
CREATE TABLE IF NOT EXISTS public.tbl_historia_cen_sklepy (
    id bigserial primary key,
    id_elementu_sklepu bigint references tbl_elementy_sklepy(id) on delete cascade,
    cena numeric(12,4) not null,
    waluta character varying(3),
    data_ceny timestamp default current_timestamp,
    zrodlo character varying(100),
    czy_aktualna boolean default false
);

CREATE INDEX IF NOT EXISTS idx_tbl_historia_cen_sklepy_id_elementu_sklepu ON public.tbl_historia_cen_sklepy(id_elementu_sklepu);
CREATE INDEX IF NOT EXISTS idx_tbl_historia_cen_sklepy_data_ceny ON public.tbl_historia_cen_sklepy(data_ceny);



CREATE TABLE IF NOT EXISTS public.tbl_lokalizacje (
    id bigserial primary key,
    nazwa character varying(200) not null,
    opis character varying(1000),
    sciezka ltree,
    czy_aktywna boolean default true,
    dodano timestamp default current_timestamp
);

CREATE TABLE IF NOT EXISTS public.tbl_elementy_lokalizacje (
    id_elementu bigint references tbl_elementy(id),
    id_lokalizacji bigint references tbl_lokalizacje(id),
    ilosc integer not null,
    primary key (id_elementu, id_lokalizacji)
);

CREATE TABLE IF NOT EXISTS public.tbl_projekty (
    id bigserial primary key,
    nazwa character varying(200) not null,
    opis character varying(4000),
    status character varying(50) default 'W_TRAKCIE',
    dodano timestamp default current_timestamp,
    zmieniono timestamp default current_timestamp,
    uzytkownik_dod character varying(50),
    czy_aktywny boolean default true
);

CREATE TABLE IF NOT EXISTS public.tbl_pozycje_projektow (
    id bigserial primary key,
    id_projektu bigint references tbl_projekty(id),
    id_elementu bigint references tbl_elementy(id),
    ilosc_wymagana integer not null,
    ilosc_zamowiona integer default 0,
    ilosc_zuzyta integer default 0,
    priorytet integer default 5,
    uwagi character varying(2000),
    dodano timestamp default current_timestamp,
    zmieniono timestamp default current_timestamp
);

CREATE TABLE IF NOT EXISTS public.tbl_transakcje (
    id bigserial primary key,
    typ_transakcji character varying(20) not null,
    id_elementu bigint references tbl_elementy(id),
    id_projektu bigint references tbl_projekty(id),
    id_lokalizacji bigint references tbl_lokalizacje(id),
    ilosc_zmiana integer not null,
    ilosc_przed integer,
    ilosc_po integer,
    cena_jednostkowa numeric(12,4),
    data_transakcji timestamp default current_timestamp,
    uzytkownik character varying(50),
    komentarz character varying(2000)
);

-- Nowa tabela dla zamienników elementów
CREATE TABLE IF NOT EXISTS public.tbl_zamienniki (
    id_elementu_podstawowego bigint references tbl_elementy(id),
    id_elementu_zamiennego bigint references tbl_elementy(id),
    stopien_zgodnosci integer not null default 100,
    uwagi character varying(2000),
    dodano timestamp default current_timestamp,
    uzytkownik_dod character varying(50),
    primary key (id_elementu_podstawowego, id_elementu_zamiennego),
    CHECK (id_elementu_podstawowego <> id_elementu_zamiennego)
);

COMMENT ON TABLE public.tbl_zamienniki IS 'Tabela przechowująca informacje o zamiennikach elementów';
COMMENT ON COLUMN public.tbl_zamienniki.stopien_zgodnosci IS 'Stopień zgodności w procentach (0-100)';

-- Tabela dla zdjęć elementów (wielość zdjęć)
CREATE TABLE IF NOT EXISTS public.tbl_zdjecia_elementow (
    id bigserial primary key,
    id_elementu bigint references tbl_elementy(id) on delete cascade,
    sciezka_pliku character varying(500) not null,
    opis character varying(1000),
    czy_glowne boolean default false,
    dodano timestamp default current_timestamp,
    uzytkownik_dod character varying(50)
);

CREATE INDEX IF NOT EXISTS idx_tbl_zdjecia_elementow_id_elementu ON public.tbl_zdjecia_elementow(id_elementu);

-- Dodatkowe indeksy i ograniczenia ------------------------------------
CREATE UNIQUE INDEX IF NOT EXISTS uq_tbl_kategorie_rodzic_nazwa
    ON public.tbl_kategorie (COALESCE(id_rodzica, 0), lower(nazwa));
CREATE INDEX IF NOT EXISTS idx_tbl_kategorie_sciezka
    ON public.tbl_kategorie USING gist (sciezka);
CREATE INDEX IF NOT EXISTS idx_tbl_kategorie_id_rodzica
    ON public.tbl_kategorie (id_rodzica);

CREATE UNIQUE INDEX IF NOT EXISTS uq_tbl_sklepy_nazwa
    ON public.tbl_sklepy (lower(nazwa));

CREATE INDEX IF NOT EXISTS idx_tbl_elementy_id_kategorii
    ON public.tbl_elementy (id_kategorii);
CREATE INDEX IF NOT EXISTS idx_tbl_elementy_symbol
    ON public.tbl_elementy (lower(symbol));
CREATE INDEX IF NOT EXISTS idx_tbl_elementy_symbol_producenta
    ON public.tbl_elementy (lower(symbol_producenta));

CREATE INDEX IF NOT EXISTS idx_tbl_pozycje_projektow_id_projektu ON public.tbl_pozycje_projektow (id_projektu);
CREATE INDEX IF NOT EXISTS idx_tbl_pozycje_projektow_id_elementu ON public.tbl_pozycje_projektow (id_elementu);

CREATE INDEX IF NOT EXISTS idx_tbl_transakcje_id_elementu ON public.tbl_transakcje (id_elementu);
CREATE INDEX IF NOT EXISTS idx_tbl_transakcje_data ON public.tbl_transakcje (data_transakcji);

CREATE INDEX IF NOT EXISTS idx_tbl_elementy_lokalizacje_lok
    ON public.tbl_elementy_lokalizacje (id_lokalizacji);

-- Wersja 2: oznaczenia części (np. "R1, R2, R5") w pozycjach listy części projektu
ALTER TABLE public.tbl_pozycje_projektow ADD COLUMN IF NOT EXISTS oznaczenia character varying(2000);

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 1, 'Struktura początkowa'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 1);

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 2, 'Projekty: oznaczenia części w liście BOM'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 2);

-- ---------------------------------------------------------------------
-- Wersja 3: sklepy (ceny, dostawa), linki elementów, kursy walut
-- ---------------------------------------------------------------------
ALTER TABLE public.tbl_sklepy ADD COLUMN IF NOT EXISTS szablon_wyszukiwania character varying(500);
ALTER TABLE public.tbl_sklepy ADD COLUMN IF NOT EXISTS koszt_dostawy numeric(10,2) DEFAULT 0;
ALTER TABLE public.tbl_sklepy ADD COLUMN IF NOT EXISTS darmowa_dostawa_od numeric(10,2);
ALTER TABLE public.tbl_sklepy ADD COLUMN IF NOT EXISTS regex_ceny character varying(500);
ALTER TABLE public.tbl_elementy_sklepy ADD COLUMN IF NOT EXISTS kod_w_sklepie character varying(100);

COMMENT ON COLUMN public.tbl_sklepy.szablon_wyszukiwania IS 'Adres wyszukiwania z {q} w miejscu szukanej frazy';
COMMENT ON COLUMN public.tbl_sklepy.koszt_dostawy IS 'Koszt dostawy w walucie sklepu';
COMMENT ON COLUMN public.tbl_sklepy.darmowa_dostawa_od IS 'Wartość zamówienia, od której dostawa jest darmowa (NULL = nigdy)';
COMMENT ON COLUMN public.tbl_sklepy.regex_ceny IS 'Własne wyrażenie regularne wyciągające cenę ze strony (pierwsza grupa)';

CREATE TABLE IF NOT EXISTS public.tbl_kursy_walut (
    waluta character varying(3) primary key,
    kurs_pln numeric(12,4) not null check (kurs_pln > 0),
    zmieniono timestamp default current_timestamp
);

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 3, 'Sklepy: szablon wyszukiwania, dostawa, kursy walut'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 3);

-- ---------------------------------------------------------------------
-- Wersja 4: opisy pól formularzy (pomoc kontekstowa F1 i dymki)
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS public.tbl_pomoc_pol (
    klucz character varying(80) primary key,
    tytul character varying(200) not null,
    opis text not null,
    kotwica character varying(80),
    zmienione boolean not null default false,
    zmieniono timestamp default current_timestamp
);
COMMENT ON TABLE public.tbl_pomoc_pol IS 'Opisy pól formularzy; zmienione = true chroni opis przed nadpisaniem przy aktualizacji';

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 4, 'Pomoc kontekstowa: tabela opisów pól'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 4);

-- ---------------------------------------------------------------------
-- Wersja 5: miejsce magazynowania elementu (kontener, szuflada, przegroda)
-- ---------------------------------------------------------------------
ALTER TABLE public.tbl_elementy ADD COLUMN IF NOT EXISTS kontener character varying(100);
ALTER TABLE public.tbl_elementy ADD COLUMN IF NOT EXISTS szuflada character varying(50);
ALTER TABLE public.tbl_elementy ADD COLUMN IF NOT EXISTS przegroda character varying(50);
ALTER TABLE public.tbl_elementy ADD COLUMN IF NOT EXISTS tolerancja character varying(20);
ALTER TABLE public.tbl_elementy ADD COLUMN IF NOT EXISTS parametry character varying(300);
COMMENT ON COLUMN public.tbl_elementy.tolerancja IS 'Tolerancja wartości, np. "1%"';
COMMENT ON COLUMN public.tbl_elementy.parametry IS 'Pozostałe parametry techniczne, np. "0,1 W; 50 V; X7R"';
COMMENT ON COLUMN public.tbl_elementy.kontener IS 'Kontener / szafka, np. "Kontener 1"';
COMMENT ON COLUMN public.tbl_elementy.szuflada IS 'Oznaczenie szuflady, np. "A1"';
COMMENT ON COLUMN public.tbl_elementy.przegroda IS 'Przegroda w szufladzie (opcjonalnie), np. "1"';
CREATE INDEX IF NOT EXISTS idx_tbl_elementy_miejsce ON public.tbl_elementy (kontener, szuflada, przegroda);

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 5, 'Miejsce magazynowania, tolerancja i parametry elementu'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 5);

-- ---------------------------------------------------------------------
-- Wersja 6: katalog elementów (wzorce z parametrami i datasheetem)
-- Powiązanie z magazynem jest liczone po symbolu (lub symbolu producenta) elementu.
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS public.tbl_katalog (
    id bigserial primary key,
    symbol character varying(120) not null,
    producent character varying(150) not null default '',
    opis character varying(1000),
    obudowa character varying(100),
    rodzina character varying(150),
    datasheet_url character varying(500),
    datasheet_plik character varying(500),
    uwagi text,
    dodano timestamp default current_timestamp,
    zmieniono timestamp default current_timestamp
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_tbl_katalog_symbol_producent
    ON public.tbl_katalog (lower(symbol), lower(producent));
CREATE INDEX IF NOT EXISTS idx_tbl_katalog_rodzina ON public.tbl_katalog (lower(rodzina));

CREATE TABLE IF NOT EXISTS public.tbl_katalog_parametry (
    id bigserial primary key,
    id_katalogu bigint not null references public.tbl_katalog(id) on delete cascade,
    nazwa character varying(100) not null,
    wartosc character varying(300) not null,
    zrodlo character varying(20) not null default 'reczny',
    kolejnosc integer not null default 0
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_tbl_katalog_parametry_nazwa
    ON public.tbl_katalog_parametry (id_katalogu, lower(nazwa));

COMMENT ON TABLE public.tbl_katalog IS 'Katalog elementów: wzorce z opisem, parametrami i datasheetem';
COMMENT ON COLUMN public.tbl_katalog.rodzina IS 'Swobodna nazwa grupy, np. "Wzmacniacze operacyjne"';
COMMENT ON COLUMN public.tbl_katalog_parametry.zrodlo IS 'reczny, csv albo datasheet (odczytane z karty katalogowej)';

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 6, 'Katalog elementów z parametrami'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 6);

-- ---------------------------------------------------------------------
-- Wersja 7: typy obudów z atrybutami (moc strat, napięcie, rozstaw) i wariantami
-- Powiązanie z elementami jest liczone po nazwie obudowy (bez znaków specjalnych i wielkości liter:
-- "TO-220" = "TO220"), więc istniejące elementy z tekstową obudową działają bez zmian.
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS public.tbl_obudowy (
    id bigserial primary key,
    nazwa character varying(100) not null,
    opis character varying(1000),
    montaz character varying(10),
    liczba_pinow integer,
    moc_strat_w numeric(12,3),
    napiecie_max_v numeric(12,2),
    rozstaw character varying(40),
    uwagi text,
    dodano timestamp default current_timestamp,
    zmieniono timestamp default current_timestamp
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_tbl_obudowy_klucz
    ON public.tbl_obudowy ((regexp_replace(lower(nazwa), '[^a-z0-9]', '', 'g')));

CREATE TABLE IF NOT EXISTS public.tbl_obudowy_warianty (
    id bigserial primary key,
    id_obudowy bigint not null references public.tbl_obudowy(id) on delete cascade,
    wariant character varying(60) not null default '',
    pakiet_eagle character varying(100) not null default '',
    opis character varying(500)
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_tbl_obudowy_warianty
    ON public.tbl_obudowy_warianty (id_obudowy, lower(pakiet_eagle), lower(wariant));
CREATE INDEX IF NOT EXISTS idx_tbl_obudowy_warianty_pakiet
    ON public.tbl_obudowy_warianty ((regexp_replace(lower(pakiet_eagle), '[^a-z0-9]', '', 'g')));

COMMENT ON TABLE public.tbl_obudowy IS 'Typy obudów z atrybutami; powiązanie z elementami po nazwie obudowy';
COMMENT ON COLUMN public.tbl_obudowy.montaz IS 'THT albo SMD';
COMMENT ON COLUMN public.tbl_obudowy_warianty.pakiet_eagle IS 'Nazwa pakietu w bibliotece Eagle, np. TO220AH';

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 7, 'Typy obudów z atrybutami i wariantami'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 7);

-- ---------------------------------------------------------------------
-- Wersja 8: relacja element -> wiele obudów (wspólny stan magazynowy, obudowy to informacja, w których element pasuje)
-- Kolumna tbl_elementy.obudowa zostaje jako tekst "R7.5, R10, R15L" utrzymywany automatycznie z relacji
-- (trigger w funkcje_i_trigger.sql), więc eksporty, wyszukiwanie, CSV i dopasowanie projektów działają bez zmian.
-- Każda obudowa w słowniku to jeden pakiet (np. "R7.5", "TO220AH"); rodzina i wariant to jej atrybuty.
-- ---------------------------------------------------------------------
DROP TABLE IF EXISTS public.tbl_obudowy_warianty;
ALTER TABLE public.tbl_obudowy ADD COLUMN IF NOT EXISTS rodzina character varying(100);
ALTER TABLE public.tbl_obudowy ADD COLUMN IF NOT EXISTS wariant character varying(60);

CREATE TABLE IF NOT EXISTS public.tbl_elementy_obudowy (
    id_elementu bigint not null references public.tbl_elementy(id) on delete cascade,
    id_obudowy bigint not null references public.tbl_obudowy(id) on delete cascade,
    kolejnosc integer not null default 0,
    primary key (id_elementu, id_obudowy)
);
CREATE INDEX IF NOT EXISTS idx_tbl_elementy_obudowy_obudowa ON public.tbl_elementy_obudowy (id_obudowy);
COMMENT ON TABLE public.tbl_elementy_obudowy IS 'Obudowy, w których pasuje element (jeden element, wiele obudów; stan magazynowy wspólny)';

-- Istniejące elementy: teksty obudów trafiają do słownika i do relacji (kilka obudów rozdzielonych przecinkiem lub średnikiem)
INSERT INTO public.tbl_obudowy (nazwa)
SELECT DISTINCT ON (regexp_replace(lower(x.nazwa), '[^a-z0-9]', '', 'g')) x.nazwa
FROM (SELECT btrim(unnest(regexp_split_to_array(obudowa, '\s*[,;]\s*'))) AS nazwa
      FROM public.tbl_elementy WHERE btrim(COALESCE(obudowa, '')) <> '') x
WHERE regexp_replace(lower(x.nazwa), '[^a-z0-9]', '', 'g') <> ''
  AND NOT EXISTS (SELECT 1 FROM public.tbl_obudowy o
                  WHERE regexp_replace(lower(o.nazwa), '[^a-z0-9]', '', 'g') = regexp_replace(lower(x.nazwa), '[^a-z0-9]', '', 'g'))
ORDER BY regexp_replace(lower(x.nazwa), '[^a-z0-9]', '', 'g'), x.nazwa;

INSERT INTO public.tbl_elementy_obudowy (id_elementu, id_obudowy, kolejnosc)
SELECT DISTINCT ON (x.id, o.id) x.id, o.id, x.kol
FROM (SELECT e.id, btrim(t.nazwa) AS nazwa, (t.nr - 1)::integer AS kol
      FROM public.tbl_elementy e,
           LATERAL unnest(regexp_split_to_array(COALESCE(e.obudowa, ''), '\s*[,;]\s*')) WITH ORDINALITY AS t(nazwa, nr)) x
JOIN public.tbl_obudowy o
  ON regexp_replace(lower(o.nazwa), '[^a-z0-9]', '', 'g') = regexp_replace(lower(x.nazwa), '[^a-z0-9]', '', 'g')
WHERE x.nazwa <> ''
ORDER BY x.id, o.id, x.kol
ON CONFLICT DO NOTHING;

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 8, 'Relacja element - wiele obudów'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 8);

-- ---------------------------------------------------------------------
-- Wersja 9: Literatura (książki użytkownika, ścieżka do pliku, indeks pełnotekstowy PDF)
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS public.tbl_ksiazki (
    id bigserial PRIMARY KEY,
    tytul character varying(300) NOT NULL,
    autor character varying(200),
    wydawca character varying(150),
    rok integer,
    isbn character varying(30),
    jezyk character varying(30),
    tagi character varying(300),
    uwagi text,
    sciezka text,
    strony integer,
    zaindeksowano timestamp without time zone,
    dodano timestamp without time zone NOT NULL DEFAULT current_timestamp,
    zmieniono timestamp without time zone NOT NULL DEFAULT current_timestamp
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_tbl_ksiazki_sciezka ON public.tbl_ksiazki (lower(sciezka)) WHERE sciezka IS NOT NULL AND sciezka <> '';

-- Tekst stron PDF (jedna strona = jeden wiersz); wektor_tekstu liczy aplikacja (fn_bez_akcentow + 'simple'), więc szukanie nie zależy od polskich znaków
CREATE TABLE IF NOT EXISTS public.tbl_ksiazki_tresc (
    id_ksiazki bigint NOT NULL REFERENCES public.tbl_ksiazki(id) ON DELETE CASCADE,
    strona integer NOT NULL,
    tekst text NOT NULL,
    wektor_tekstu tsvector,
    PRIMARY KEY (id_ksiazki, strona)
);
CREATE INDEX IF NOT EXISTS idx_tbl_ksiazki_tresc_wektor ON public.tbl_ksiazki_tresc USING gin (wektor_tekstu);

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 9, 'Literatura: książki i indeks treści PDF'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 9);

-- ---------------------------------------------------------------------
-- Wersja 10: Narzędzia i pożyczki
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS public.tbl_narzedzia (
    id bigserial PRIMARY KEY,
    nazwa character varying(200) NOT NULL,
    producent character varying(100),
    model character varying(100),
    kategoria character varying(100),
    numer_seryjny character varying(100),
    miejsce character varying(150),
    data_zakupu date,
    cena numeric(12,2),
    zdjecie text,
    instrukcja text,
    uwagi text,
    dodano timestamp without time zone NOT NULL DEFAULT current_timestamp,
    zmieniono timestamp without time zone NOT NULL DEFAULT current_timestamp
);

-- Pożyczki narzędzi; otwarta pożyczka to taka bez daty zwrotu (narzędzie może być pożyczone tylko jednej osobie naraz)
CREATE TABLE IF NOT EXISTS public.tbl_pozyczki (
    id bigserial PRIMARY KEY,
    id_narzedzia bigint NOT NULL REFERENCES public.tbl_narzedzia(id) ON DELETE CASCADE,
    osoba character varying(150) NOT NULL,
    data_pozyczenia date NOT NULL DEFAULT current_date,
    termin date,
    data_zwrotu date,
    uwagi text,
    dodano timestamp without time zone NOT NULL DEFAULT current_timestamp
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_tbl_pozyczki_otwarta ON public.tbl_pozyczki (id_narzedzia) WHERE data_zwrotu IS NULL;
CREATE INDEX IF NOT EXISTS idx_tbl_pozyczki_narzedzie ON public.tbl_pozyczki (id_narzedzia);

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 10, 'Narzędzia i pożyczki'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 10);

-- Typowe elementy w obudowie (z zestawów "devicesets" bibliotek Eagle): symbol elementu -> obudowa.
-- Służy do podpowiadania obudowy przy nowym elemencie i do listy "typowe elementy" w słowniku obudów.
CREATE TABLE IF NOT EXISTS public.tbl_obudowy_typowe (
    id bigserial PRIMARY KEY,
    id_obudowy bigint NOT NULL REFERENCES public.tbl_obudowy(id) ON DELETE CASCADE,
    zestaw character varying(100) NOT NULL,
    opis character varying(500),
    prefiks character varying(10),
    UNIQUE (id_obudowy, zestaw)
);
CREATE INDEX IF NOT EXISTS idx_tbl_obudowy_typowe_zestaw ON public.tbl_obudowy_typowe (lower(zestaw));

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 11, 'Typowe elementy w obudowach'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 11);

-- Pożyczki książek: ta sama tabela co pożyczki narzędzi; każda pożyczka dotyczy dokładnie jednego przedmiotu
ALTER TABLE public.tbl_pozyczki ALTER COLUMN id_narzedzia DROP NOT NULL;
ALTER TABLE public.tbl_pozyczki ADD COLUMN IF NOT EXISTS id_ksiazki bigint REFERENCES public.tbl_ksiazki(id) ON DELETE CASCADE;
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'chk_tbl_pozyczki_przedmiot') THEN
        ALTER TABLE public.tbl_pozyczki ADD CONSTRAINT chk_tbl_pozyczki_przedmiot CHECK ((id_narzedzia IS NULL) <> (id_ksiazki IS NULL));
    END IF;
END $$;
CREATE UNIQUE INDEX IF NOT EXISTS uq_tbl_pozyczki_otwarta_ksiazka ON public.tbl_pozyczki (id_ksiazki) WHERE data_zwrotu IS NULL AND id_ksiazki IS NOT NULL;
CREATE INDEX IF NOT EXISTS idx_tbl_pozyczki_ksiazka ON public.tbl_pozyczki (id_ksiazki);

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 12, 'Pożyczki książek'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 12);

-- ---------------------------------------------------------------------
-- Wersja 13: ujednolicone nazwy obiektów (po polsku, tbl_/wid_/fn_/trg_), opisy tabel i kolumn
-- Zmiany nazw dla istniejących baz wykonuje migracja_nazw.sql (uruchamiana przed tym skryptem).
-- ---------------------------------------------------------------------
INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 13, 'Ujednolicone nazwy obiektów, opisy tabel i kolumn'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 13);

-- ---------------------------------------------------------------------
-- Wersja 14: zdjęcie pozycji katalogu, numer inwentarzowy narzędzia
-- ---------------------------------------------------------------------
ALTER TABLE public.tbl_katalog ADD COLUMN IF NOT EXISTS zdjecie text;
ALTER TABLE public.tbl_narzedzia ADD COLUMN IF NOT EXISTS numer_inwentarzowy character varying(50);

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 14, 'Zdjęcie w katalogu, numer inwentarzowy narzędzia'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 14);

-- ---------------------------------------------------------------------
-- Wersja 15: wydanie książki (np. "wydanie drugie")
-- ---------------------------------------------------------------------
ALTER TABLE public.tbl_ksiazki ADD COLUMN IF NOT EXISTS wydanie character varying(100);

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 15, 'Wydanie książki'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 15);

-- ---------------------------------------------------------------------
-- Wersja 16: opis parametru katalogu (wyjaśnienie symbolu, np. W_amax = maksymalna moc strat anody), pokazywany w dymku
-- ---------------------------------------------------------------------
ALTER TABLE public.tbl_katalog_parametry ADD COLUMN IF NOT EXISTS opis character varying(500);
COMMENT ON COLUMN public.tbl_katalog_parametry.opis IS 'Opis parametru (co oznacza symbol), pokazywany w dymku po najechaniu na parametr';

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 16, 'Opis parametru katalogu'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 16);

-- ---------------------------------------------------------------------
-- Wersja 17: słownik opisów parametrów katalogu (symbol -> opis w dymku), edytowany w programie (Katalog / Słownik parametrów)
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS public.tbl_slownik_parametrow (
    id bigserial PRIMARY KEY,
    symbol character varying(100) NOT NULL,
    klucz character varying(100) NOT NULL UNIQUE,
    opis character varying(500) NOT NULL,
    zmieniono timestamp without time zone NOT NULL DEFAULT current_timestamp
);
COMMENT ON TABLE public.tbl_slownik_parametrow IS 'Słownik opisów symboli parametrów (W_amax = ...); klucz: symbol bez wielkości liter, spacji, _ i nawiasów';
COMMENT ON COLUMN public.tbl_slownik_parametrow.id IS 'Identyfikator wpisu';
COMMENT ON COLUMN public.tbl_slownik_parametrow.symbol IS 'Symbol parametru tak, jak go wpisano, np. W_amax';
COMMENT ON COLUMN public.tbl_slownik_parametrow.klucz IS 'Symbol porównawczy: małe litery, bez spacji, _, kropek, myślników i nawiasów; unikalny';
COMMENT ON COLUMN public.tbl_slownik_parametrow.opis IS 'Opis parametru pokazywany w dymku';
COMMENT ON COLUMN public.tbl_slownik_parametrow.zmieniono IS 'Data ostatniej zmiany wpisu';

INSERT INTO public.tbl_slownik_parametrow (symbol, klucz, opis) VALUES
    ('W_amax', 'wamax', 'Maksymalna dopuszczalna moc strat anody [W]'),
    ('P_amax', 'pamax', 'Maksymalna dopuszczalna moc strat anody [W]'),
    ('W_g2max', 'wg2max', 'Maksymalna dopuszczalna moc strat siatki ekranującej g2 [W]'),
    ('P_g2max', 'pg2max', 'Maksymalna dopuszczalna moc strat siatki ekranującej g2 [W]'),
    ('U_amax', 'uamax', 'Maksymalne dopuszczalne napięcie anodowe [V]'),
    ('V_amax', 'vamax', 'Maksymalne dopuszczalne napięcie anodowe [V]'),
    ('U_a', 'ua', 'Napięcie anodowe (anoda względem katody) [V]'),
    ('V_a', 'va', 'Napięcie anodowe (anoda względem katody) [V]'),
    ('U_g2', 'ug2', 'Napięcie siatki ekranującej g2 względem katody [V]'),
    ('V_g2', 'vg2', 'Napięcie siatki ekranującej g2 względem katody [V]'),
    ('U_g1', 'ug1', 'Napięcie siatki sterującej g1 względem katody (ujemne) [V]'),
    ('V_g1', 'vg1', 'Napięcie siatki sterującej g1 względem katody (ujemne) [V]'),
    ('I_a', 'ia', 'Prąd anodowy [mA]'),
    ('I_g2', 'ig2', 'Prąd siatki ekranującej g2 [mA]'),
    ('I_k', 'ik', 'Prąd katody [mA]'),
    ('I_kmax', 'ikmax', 'Maksymalny dopuszczalny prąd katody [mA]'),
    ('U_f', 'uf', 'Napięcie żarzenia [V]'),
    ('U_h', 'uh', 'Napięcie żarzenia [V]'),
    ('I_h', 'ih', 'Prąd żarzenia [A]'),
    ('R_i', 'ri', 'Rezystancja wewnętrzna (dynamiczna) lampy [Ω]'),
    ('R_a', 'ra', 'Rezystancja obciążenia w obwodzie anody [Ω]'),
    ('R_aa', 'raa', 'Rezystancja obciążenia anoda-anoda w układzie przeciwsobnym [Ω]'),
    ('g_m', 'gm', 'Nachylenie (transkonduktancja) [mA/V]'),
    ('S', 's', 'Nachylenie lampy: zmiana prądu anodowego przypadająca na zmianę napięcia siatki [mA/V]'),
    ('mu', 'mu', 'Współczynnik wzmocnienia napięciowego lampy (bez jednostki)'),
    ('U_hk', 'uhk', 'Maksymalne napięcie między żarnikiem a katodą [V]'),
    ('V_CE', 'vce', 'Napięcie kolektor-emiter [V]'),
    ('V_CEO', 'vceo', 'Maksymalne napięcie kolektor-emiter przy otwartej bazie [V]'),
    ('V_CBO', 'vcbo', 'Maksymalne napięcie kolektor-baza przy otwartym emiterze [V]'),
    ('V_EBO', 'vebo', 'Maksymalne napięcie emiter-baza przy otwartym kolektorze [V]'),
    ('V_CE(sat)', 'vcesat', 'Napięcie nasycenia kolektor-emiter [V]'),
    ('V_BE', 'vbe', 'Napięcie baza-emiter [V]'),
    ('I_C', 'ic', 'Prąd kolektora [A]'),
    ('I_Cmax', 'icmax', 'Maksymalny dopuszczalny prąd kolektora [A]'),
    ('P_tot', 'ptot', 'Całkowita dopuszczalna moc strat elementu [W]'),
    ('h_FE', 'hfe', 'Statyczne wzmocnienie prądowe tranzystora w układzie ze wspólnym emiterem (bez jednostki)'),
    ('f_T', 'ft', 'Częstotliwość graniczna (przy której wzmocnienie prądowe spada do 1) [MHz]'),
    ('V_DS', 'vds', 'Napięcie dren-źródło [V]'),
    ('V_DSS', 'vdss', 'Maksymalne napięcie dren-źródło przy zwartej bramce [V]'),
    ('V_GS', 'vgs', 'Napięcie bramka-źródło [V]'),
    ('V_GS(th)', 'vgsth', 'Napięcie progowe bramki: przy nim tranzystor zaczyna przewodzić [V]'),
    ('R_DS(on)', 'rdson', 'Rezystancja kanału dren-źródło w stanie przewodzenia [Ω]'),
    ('I_D', 'id', 'Prąd drenu [A]'),
    ('V_F', 'vf', 'Napięcie przewodzenia diody [V]'),
    ('I_F', 'if', 'Prąd przewodzenia diody [A]'),
    ('V_R', 'vr', 'Napięcie wsteczne diody [V]'),
    ('V_RRM', 'vrrm', 'Maksymalne powtarzalne napięcie wsteczne [V]'),
    ('I_R', 'ir', 'Prąd wsteczny diody [µA]'),
    ('T_j', 'tj', 'Temperatura złącza [°C]'),
    ('T_jmax', 'tjmax', 'Maksymalna dopuszczalna temperatura złącza [°C]'),
    ('R_thJC', 'rthjc', 'Rezystancja cieplna złącze-obudowa [K/W]'),
    ('R_thJA', 'rthja', 'Rezystancja cieplna złącze-otoczenie [K/W]'),
    ('V_CC', 'vcc', 'Napięcie zasilania [V]'),
    ('V_DD', 'vdd', 'Napięcie zasilania [V]'),
    ('I_Q', 'iq', 'Prąd spoczynkowy pobierany przez układ [mA]'),
    ('GBW', 'gbw', 'Iloczyn wzmocnienia i szerokości pasma wzmacniacza [MHz]'),
    ('V_OS', 'vos', 'Napięcie niezrównoważenia wejścia [mV]'),
    ('I_B', 'ib', 'Prąd bazy (dla wzmacniacza operacyjnego: prąd polaryzacji wejścia) [A]'),
    ('SR', 'sr', 'Szybkość narastania napięcia wyjściowego (slew rate) [V/µs]'),
    ('ESR', 'esr', 'Zastępcza rezystancja szeregowa kondensatora [Ω]')
ON CONFLICT (klucz) DO NOTHING;

INSERT INTO public.tbl_wersja_schematu (wersja, opis)
SELECT 17, 'Słownik opisów parametrów'
WHERE NOT EXISTS (SELECT 1 FROM public.tbl_wersja_schematu WHERE wersja = 17);
