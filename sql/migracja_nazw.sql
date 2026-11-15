-- =====================================================================
-- Magmass - migracja nazw obiektów do konwencji z wersji 13
-- Uruchamiana PRZED schema.sql. Skrypt jest idempotentny; na świeżej bazie nic nie robi.
-- Zgodność: PostgreSQL 9.6 lub nowszy.
--
-- Konwencja nazw (wersja 13):
--   tbl_<nazwa>          tabele (liczba mnoga, po polsku)
--   wid_<nazwa>          widoki
--   fn_<nazwa>           funkcje
--   trg_<tabela>_<opis>  triggery
--   idx_<tabela>_<opis>, uq_<tabela>_<opis>, chk_<tabela>_<opis>   indeksy i ograniczenia
--   id_<obiekt>          klucze obce
--   dodano, zmieniono    znaczniki czasu; uzytkownik_dod, uzytkownik_mod  autor zmiany
-- =====================================================================

DO $$
DECLARE
    r record;
    v_przed text;
    v_po text;
BEGIN
    -- 1. Stare widoki (zostaną odtworzone pod nowymi nazwami przez widoki.sql)
    DROP VIEW IF EXISTS public.v_kategorie;
    DROP VIEW IF EXISTS public.v_elementy;
    DROP VIEW IF EXISTS public.v_niskie_stany;
    DROP VIEW IF EXISTS public.v_sklepy_elementow;
    DROP VIEW IF EXISTS public.v_najlepsze_ceny;
    DROP VIEW IF EXISTS public.v_bom_stan;

    -- 2. Stare triggery (odtworzone pod nowymi nazwami przez funkcje_i_trigger.sql)
    FOR r IN
        SELECT t.tgname, c.relname
        FROM pg_trigger t
        JOIN pg_class c ON c.oid = t.tgrelid
        JOIN pg_namespace n ON n.oid = c.relnamespace
        WHERE n.nspname = 'public' AND NOT t.tgisinternal
          AND t.tgname IN ('trigger_aktualizuj_date_modyfikacji_sklepu', 'trigger_aktualizuj_date_modyfikacji_element_sklep',
                           'trigger_ts_kategorie', 'trigger_ts_bom', 'trigger_ts_elementy', 'trigger_ts_projekty',
                           'trigger_kategorie_sciezka', 'trigger_kategorie_potomkowie', 'trigger_tworz_domyslne_linki_sklepow',
                           'trigger_jeden_domyslny_sklep', 'trigger_historia_cen')
    LOOP
        EXECUTE format('DROP TRIGGER %I ON public.%I', r.tgname, r.relname);
    END LOOP;

    -- 3. Stare funkcje
    DROP FUNCTION IF EXISTS public.fn_ts_modyfikowano();
    DROP FUNCTION IF EXISTS public.fn_ts_data_modyfikacji();
    DROP FUNCTION IF EXISTS public.fn_url_encode(text);
    DROP FUNCTION IF EXISTS public.dodaj_kategorie_jesli_brak(bigint, character varying, character varying);
    DROP FUNCTION IF EXISTS public.utworz_link_sklepu(bigint, character varying, character varying);
    DROP FUNCTION IF EXISTS public.tworz_domyslne_linki_sklepow();
    DROP FUNCTION IF EXISTS public.zmien_ilosc_elementu(bigint, integer, character varying, bigint, bigint, character varying, character varying, numeric);
    DROP FUNCTION IF EXISTS public.szukaj_elementow(text, integer);
    DROP FUNCTION IF EXISTS public.dm_unaccent(text);

    -- 4. Tabela tbl_bom -> tbl_pozycje_projektow (z sekwencją, indeksami i ograniczeniami)
    IF to_regclass('public.tbl_bom') IS NOT NULL AND to_regclass('public.tbl_pozycje_projektow') IS NULL THEN
        ALTER TABLE public.tbl_bom RENAME TO tbl_pozycje_projektow;
        FOR r IN SELECT conname FROM pg_constraint
                 WHERE conrelid = 'public.tbl_pozycje_projektow'::regclass AND conname LIKE 'tbl\_bom%'
        LOOP
            EXECUTE format('ALTER TABLE public.tbl_pozycje_projektow RENAME CONSTRAINT %I TO %I',
                           r.conname, replace(r.conname, 'tbl_bom', 'tbl_pozycje_projektow'));
        END LOOP;
        FOR r IN SELECT c.relname, c.relkind FROM pg_class c JOIN pg_namespace n ON n.oid = c.relnamespace
                 WHERE n.nspname = 'public' AND c.relkind IN ('i', 'S') AND c.relname LIKE '%tbl\_bom%'
        LOOP
            v_po := replace(r.relname, 'tbl_bom', 'tbl_pozycje_projektow');
            IF r.relkind = 'S' THEN
                EXECUTE format('ALTER SEQUENCE public.%I RENAME TO %I', r.relname, v_po);
            ELSE
                EXECUTE format('ALTER INDEX public.%I RENAME TO %I', r.relname, v_po);
            END IF;
        END LOOP;
    END IF;

    -- 5. Kolumny: znaczniki czasu i autor zmiany ujednolicone w całej bazie
    FOR r IN
        SELECT * FROM (VALUES
            ('tbl_kategorie',          'modyfikowano',     'zmieniono'),
            ('tbl_sklepy',             'modyfikowano',     'zmieniono'),
            ('tbl_elementy_sklepy',    'modyfikowano',     'zmieniono'),
            ('tbl_pozycje_projektow',  'modyfikowano',     'zmieniono'),
            ('tbl_opis_slownika',      'modyfikowano',     'zmieniono'),
            ('tbl_slownik',            'modyfikowano',     'zmieniono'),
            ('tbl_elementy',           'data_dodania',     'dodano'),
            ('tbl_elementy',           'data_modyfikacji', 'zmieniono'),
            ('tbl_projekty',           'data_utworzenia',  'dodano'),
            ('tbl_projekty',           'data_modyfikacji', 'zmieniono'),
            ('tbl_zamienniki',         'data_dodania',     'dodano'),
            ('tbl_zamienniki',         'dodano_przez',     'uzytkownik_dod'),
            ('tbl_zdjecia_elementow',  'data_dodania',     'dodano'),
            ('tbl_zdjecia_elementow',  'dodano_przez',     'uzytkownik_dod'),
            ('tbl_ksiazki_tresc',      'tsv',              'wektor_tekstu')
        ) AS x(tabela, stara, nowa)
    LOOP
        IF EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = 'public' AND table_name = r.tabela AND column_name = r.stara)
           AND NOT EXISTS (SELECT 1 FROM information_schema.columns
                           WHERE table_schema = 'public' AND table_name = r.tabela AND column_name = r.nowa) THEN
            EXECUTE format('ALTER TABLE public.%I RENAME COLUMN %I TO %I', r.tabela, r.stara, r.nowa);
        END IF;
    END LOOP;

    -- 6. Słowniki ogólne: kolumny character_01, bigint_01... -> tekst_01, liczba_01...
    FOR r IN
        SELECT c.table_name AS tabela, c.column_name AS stara,
               CASE substring(c.column_name FROM '^[a-z]+')
                    WHEN 'character' THEN 'tekst'
                    WHEN 'bigint'    THEN 'liczba'
                    WHEN 'real'      THEN 'rzeczywista'
                    WHEN 'numeric'   THEN 'dziesietna'
                    WHEN 'boolean'   THEN 'flaga'
               END || substring(c.column_name FROM '_[0-9]+$') AS nowa
        FROM information_schema.columns c
        WHERE c.table_schema = 'public' AND c.table_name IN ('tbl_opis_slownika', 'tbl_slownik')
          AND c.column_name ~ '^(character|bigint|real|numeric|boolean)_[0-9]+$'
    LOOP
        EXECUTE format('ALTER TABLE public.%I RENAME COLUMN %I TO %I', r.tabela, r.stara, r.nowa);
    END LOOP;

    -- 7. Nazwy indeksów i ograniczeń
    IF to_regclass('public.idx_tbl_ksiazki_tresc_tsv') IS NOT NULL THEN
        ALTER INDEX public.idx_tbl_ksiazki_tresc_tsv RENAME TO idx_tbl_ksiazki_tresc_wektor;
    END IF;
    IF EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'unikalny_element_sklep'
               AND conrelid = to_regclass('public.tbl_elementy_sklepy')) THEN
        ALTER TABLE public.tbl_elementy_sklepy RENAME CONSTRAINT unikalny_element_sklep TO uq_tbl_elementy_sklepy_element_sklep;
    END IF;
    IF EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'chk_pozyczki_przedmiot'
               AND conrelid = to_regclass('public.tbl_pozyczki')) THEN
        ALTER TABLE public.tbl_pozyczki RENAME CONSTRAINT chk_pozyczki_przedmiot TO chk_tbl_pozyczki_przedmiot;
    END IF;
END $$;
