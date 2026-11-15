#!/usr/bin/env python3
"""Generuje sql/opisy_obiektow.sql (COMMENT ON tabel, kolumn, widoków i funkcji).
Użycie: python3 narzedzia/zbuduj_opisy.py   (z katalogu głównego projektu)
Opisy są utrzymywane tutaj; test w tests/test_bazy.cpp sprawdza, że żadna tabela ani kolumna nie została bez opisu."""
import os, re, sys

WSPOLNE = {
    'id': 'Klucz główny (numer kolejny)',
    'dodano': 'Data i czas dodania wiersza',
    'zmieniono': 'Data i czas ostatniej zmiany (ustawiane automatycznie)',
    'uzytkownik_dod': 'Nazwa użytkownika, który dodał wiersz',
    'uzytkownik_mod': 'Nazwa użytkownika, który ostatnio zmienił wiersz',
    'uwagi': 'Dowolne uwagi użytkownika',
    'waluta': 'Kod waluty ISO, np. PLN, EUR, USD',
}

T = {}  # tabela -> (opis tabeli, {kolumna: opis})
def t(nazwa, opis, kol): T[nazwa] = (opis, kol)

t('tbl_wersja_schematu', 'Historia wersji struktury bazy (migracje wykonane przez program)',
  {'wersja': 'Numer wersji struktury', 'opis': 'Krótki opis zmian w tej wersji', 'data_zastosowania': 'Kiedy wersję zastosowano'})
t('tbl_opis_slownika', 'Nagłówki słowników ogólnych: opis i nazwy kolumn generycznych tbl_slownik (struktura zarezerwowana, program jej jeszcze nie używa)',
  {'nazwa_slownika': 'Nazwa słownika', 'opis': 'Opis słownika', 'kod_slownika': 'Krótki kod słownika',
   'czy_aktywny': 'Czy słownik jest aktywny (1/0)', 'aktywny_od': 'Data, od której słownik obowiązuje', 'wersja_aktywna': 'Numer aktywnej wersji słownika',
   'widoczny': 'Czy słownik jest widoczny w programie (1/0)', 'data_01': 'Opis pierwszego pola daty', 'data_02': 'Opis drugiego pola daty'})
t('tbl_slownik', 'Słowniki ogólne o generycznych kolumnach (tekst_NN, liczba_NN, ...); znaczenie kolumn opisuje tbl_opis_slownika (zarezerwowane, nieużywane)',
  {'id_opis': 'Słownik, do którego należy wiersz (tbl_opis_slownika)', 'wersja': 'Wersja wiersza słownika', 'aktywny': 'Czy wiersz jest aktywny (1/0)',
   'aktywny_od': 'Data, od której wiersz obowiązuje', 'widoczny': 'Czy wiersz jest widoczny (1/0)', 'data_01': 'Pole daty ogólnego przeznaczenia nr 1', 'data_02': 'Pole daty ogólnego przeznaczenia nr 2'})
t('tbl_kategorie', 'Drzewo kategorii elementów (ścieżka ltree i poziom liczone automatycznie przez trigger)',
  {'id_rodzica': 'Kategoria nadrzędna (NULL = kategoria główna)', 'nazwa': 'Nazwa kategorii (unikalna w obrębie rodzica)', 'opis': 'Opis kategorii',
   'sciezka': 'Ścieżka w drzewie (ltree, np. k1.k5.k9); liczona automatycznie', 'poziom': 'Głębokość w drzewie (0 = główna); liczona automatycznie',
   'czy_aktywny': 'Czy kategoria jest używana'})
t('tbl_sklepy', 'Sklepy z częściami elektronicznymi: adresy wyszukiwania, koszty dostawy, sposób odczytu cen',
  {'nazwa': 'Nazwa sklepu (unikalna)', 'opis': 'Opis sklepu', 'url_bazowy': 'Adres główny strony sklepu', 'logo_url': 'Adres obrazka z logo',
   'kraj': 'Kraj sklepu', 'waluta': 'Waluta cen w sklepie (3 litery)', 'czy_aktywny': 'Czy sklep jest używany w zestawieniach i linkach',
   'priorytet': 'Kolejność wyboru sklepu (1 = najchętniej)', 'parametr_wyszukiwania': 'Parametr URL do wyszukiwania (starszy sposób; np. MPN, kod producenta)',
   'czy_wspiera_api': 'Czy sklep udostępnia API cen', 'api_klucz': 'Klucz dostępu do API sklepu',
   'szablon_wyszukiwania': 'Adres wyszukiwania z {q} w miejscu szukanej frazy', 'koszt_dostawy': 'Koszt dostawy w walucie sklepu',
   'darmowa_dostawa_od': 'Wartość zamówienia, od której dostawa jest darmowa (NULL = nigdy)', 'regex_ceny': 'Własne wyrażenie regularne wyciągające cenę ze strony (pierwsza grupa)'})
t('tbl_elementy', 'Elementy magazynu (części elektroniczne): dane katalogowe, miejsce przechowywania i stan',
  {'id_kategorii': 'Kategoria elementu (tbl_kategorie)', 'symbol': 'Symbol / oznaczenie elementu, np. BC547B', 'symbol_producenta': 'Numer katalogowy producenta (MPN)',
   'producent': 'Producent', 'opis': 'Opis elementu', 'wartosc': 'Wartość, np. 10k, 100nF', 'jednostka': 'Jednostka wartości, np. Ω, F, H',
   'obudowa': 'Obudowy elementu jako tekst "R7.5, R10"; utrzymywane automatycznie z tbl_elementy_obudowy', 'datasheet_url': 'Adres karty katalogowej (datasheet)',
   'zdjecie_sciezka': 'Ścieżka głównego zdjęcia elementu', 'ilosc_dostepna': 'Stan magazynowy (szt.)', 'ilosc_minimalna': 'Stan minimalny; poniżej niego element trafia na listę braków',
   'cena_jednostkowa': 'Ostatnia znana cena jednostkowa', 'waluta': 'Waluta ceny jednostkowej', 'parametry_json': 'Parametry techniczne w formie JSON (zarezerwowane)',
   'tagi': 'Słowa kluczowe oddzielone przecinkami', 'czy_aktywny': 'Czy element jest aktywny (nieaktywne są ukryte)',
   'kontener': 'Kontener / szafka, np. "Kontener 1"', 'szuflada': 'Oznaczenie szuflady, np. "A1"', 'przegroda': 'Przegroda w szufladzie (opcjonalnie)',
   'tolerancja': 'Tolerancja wartości, np. "1%"', 'parametry': 'Pozostałe parametry techniczne, np. "0,1 W; 50 V; X7R"'})
t('tbl_elementy_sklepy', 'Linki elementów do sklepów: adres produktu, aktualna cena i dostępność',
  {'id_elementu': 'Element (tbl_elementy)', 'id_sklepu': 'Sklep (tbl_sklepy)', 'url': 'Adres strony produktu lub wyszukiwania w sklepie',
   'cena_aktualna': 'Ostatnio odczytana cena', 'waluta': 'Waluta ceny', 'ostatnia_weryfikacja': 'Kiedy cenę ostatnio sprawdzono',
   'status_dostepnosci': 'Dostępność podana przez sklep', 'minimalna_ilosc': 'Minimalna ilość zamówienia', 'czy_domyslny': 'Domyślny sklep elementu (najwyżej jeden)',
   'kod_w_sklepie': 'Numer produktu w sklepie'})
t('tbl_historia_cen_sklepy', 'Historia cen elementów w sklepach (wpis przy każdej zmianie ceny, tworzony przez trigger)',
  {'id_elementu_sklepu': 'Link elementu do sklepu (tbl_elementy_sklepy)', 'cena': 'Cena w chwili zapisu', 'waluta': 'Waluta ceny',
   'data_ceny': 'Kiedy cenę zapisano', 'zrodlo': 'Skąd cena pochodzi (ręcznie, strona sklepu, trigger)', 'czy_aktualna': 'Czy to bieżąca cena elementu w sklepie'})
t('tbl_lokalizacje', 'Lokalizacje magazynowe (drzewo ltree); zarezerwowane, miejsce elementu opisują kolumny kontener/szuflada/przegroda',
  {'nazwa': 'Nazwa lokalizacji', 'opis': 'Opis lokalizacji', 'sciezka': 'Ścieżka w drzewie lokalizacji (ltree)', 'czy_aktywna': 'Czy lokalizacja jest używana'})
t('tbl_elementy_lokalizacje', 'Ilość elementu w poszczególnych lokalizacjach',
  {'id_elementu': 'Element (tbl_elementy)', 'id_lokalizacji': 'Lokalizacja (tbl_lokalizacje)', 'ilosc': 'Ilość sztuk w tej lokalizacji'})
t('tbl_projekty', 'Projekty (np. urządzenia do zbudowania) z listą części',
  {'nazwa': 'Nazwa projektu', 'opis': 'Opis projektu', 'status': 'Status: W_TRAKCIE, ZAKONCZONY itp.', 'czy_aktywny': 'Czy projekt jest aktywny (nieaktywne są zarchiwizowane)'})
t('tbl_pozycje_projektow', 'Listy części projektów (BOM): jakie elementy i w jakiej ilości zużywa projekt',
  {'id_projektu': 'Projekt (tbl_projekty)', 'id_elementu': 'Element z magazynu (tbl_elementy)', 'ilosc_wymagana': 'Ile sztuk potrzeba w projekcie',
   'ilosc_zamowiona': 'Ile sztuk już zamówiono', 'ilosc_zuzyta': 'Ile sztuk już pobrano z magazynu na projekt', 'priorytet': 'Priorytet pozycji (1 = najważniejsza)',
   'oznaczenia': 'Oznaczenia części na schemacie, np. "R1, R2, R5"'})
t('tbl_transakcje', 'Dziennik ruchów magazynowych (przyjęcia, wydania, korekty)',
  {'typ_transakcji': 'Rodzaj ruchu: PRZYJECIE, WYDANIE, KOREKTA', 'id_elementu': 'Element (tbl_elementy)', 'id_projektu': 'Projekt, na który wydano (opcjonalnie)',
   'id_lokalizacji': 'Lokalizacja (opcjonalnie)', 'ilosc_zmiana': 'Zmiana ilości (dodatnia = przyjęcie, ujemna = wydanie)', 'ilosc_przed': 'Stan przed ruchem',
   'ilosc_po': 'Stan po ruchu', 'cena_jednostkowa': 'Cena jednostkowa w tym ruchu', 'data_transakcji': 'Kiedy wykonano ruch',
   'uzytkownik': 'Kto wykonał ruch', 'komentarz': 'Komentarz do ruchu'})
t('tbl_zamienniki', 'Zamienniki elementów (para: element podstawowy i jego zamiennik)',
  {'id_elementu_podstawowego': 'Element, który zastępujemy', 'id_elementu_zamiennego': 'Element zastępujący',
   'stopien_zgodnosci': 'Stopień zgodności w procentach (0-100)'})
t('tbl_zdjecia_elementow', 'Zdjęcia elementów (wiele zdjęć na element)',
  {'id_elementu': 'Element (tbl_elementy)', 'sciezka_pliku': 'Ścieżka pliku ze zdjęciem', 'opis': 'Opis zdjęcia', 'czy_glowne': 'Czy to zdjęcie główne elementu'})
t('tbl_kursy_walut', 'Kursy walut do przeliczania cen na złote',
  {'waluta': 'Kod waluty ISO', 'kurs_pln': 'Ile złotych kosztuje jednostka waluty'})
t('tbl_pomoc_pol', 'Opisy pól formularzy (pomoc kontekstowa F1 i dymki); zmienione = true chroni opis przed nadpisaniem przy aktualizacji',
  {'klucz': 'Klucz pola, np. element.symbol', 'tytul': 'Tytuł opisu', 'opis': 'Treść opisu', 'kotwica': 'Kotwica w pliku pomocy',
   'zmienione': 'Czy opis zmieniono ręcznie (chroni przed nadpisaniem)'})
t('tbl_katalog', 'Katalog elementów: wzorce z opisem, parametrami i datasheetem (powiązanie z magazynem po symbolu)',
  {'symbol': 'Symbol elementu w katalogu', 'producent': 'Producent (pusty = dowolny)', 'opis': 'Opis elementu', 'obudowa': 'Typowa obudowa',
   'rodzina': 'Swobodna nazwa grupy, np. "Wzmacniacze operacyjne"', 'datasheet_url': 'Adres karty katalogowej', 'datasheet_plik': 'Ścieżka lokalnego pliku z kartą katalogową', 'zdjecie': 'Ścieżka zdjęcia elementu (plik w katalogu zdjęć)'})
t('tbl_katalog_parametry', 'Parametry techniczne pozycji katalogu (nazwa i wartość)',
  {'id_katalogu': 'Pozycja katalogu (tbl_katalog)', 'nazwa': 'Nazwa parametru, np. Uce max', 'wartosc': 'Wartość parametru wraz z jednostką',
   'zrodlo': 'reczny, csv albo datasheet (odczytane z karty katalogowej)', 'kolejnosc': 'Kolejność wyświetlania'})
t('tbl_obudowy', 'Słownik obudów (pakietów) z atrybutami; powiązanie z elementami po nazwie obudowy',
  {'nazwa': 'Nazwa obudowy, np. TO-220, 0805', 'opis': 'Opis obudowy', 'montaz': 'THT albo SMD', 'liczba_pinow': 'Liczba wyprowadzeń',
   'moc_strat_w': 'Typowa moc strat w watach', 'napiecie_max_v': 'Maksymalne napięcie w woltach', 'rozstaw': 'Rozstaw wyprowadzeń, np. 2,54 mm',
   'rodzina': 'Rodzina obudów, np. TO-220', 'wariant': 'Wariant obudowy (np. pionowy, leżący)'})
t('tbl_elementy_obudowy', 'Obudowy, w których pasuje element (jeden element, wiele obudów; stan magazynowy wspólny)',
  {'id_elementu': 'Element (tbl_elementy)', 'id_obudowy': 'Obudowa (tbl_obudowy)', 'kolejnosc': 'Kolejność na liście obudów elementu'})
t('tbl_obudowy_typowe', 'Typowe elementy w obudowie (z zestawów "devicesets" bibliotek Eagle); podpowiadają obudowę przy nowym elemencie',
  {'id_obudowy': 'Obudowa (tbl_obudowy)', 'zestaw': 'Nazwa zestawu (devicesetu) lub symbolu elementu', 'opis': 'Opis zestawu', 'prefiks': 'Prefiks oznaczenia, np. R, C, IC'})
t('tbl_ksiazki', 'Literatura: książki i dokumenty użytkownika (plik PDF/EPUB lub książka papierowa)',
  {'tytul': 'Tytuł', 'autor': 'Autor', 'wydawca': 'Wydawca', 'rok': 'Rok wydania', 'wydanie': 'Wydanie (np. pierwsze, drugie poprawione)', 'isbn': 'Numer ISBN', 'jezyk': 'Język książki',
   'tagi': 'Słowa kluczowe oddzielone przecinkami', 'sciezka': 'Ścieżka pliku PDF/EPUB (pusta = książka papierowa)',
   'strony': 'Liczba zaindeksowanych stron', 'zaindeksowano': 'Kiedy zaindeksowano treść'})
t('tbl_ksiazki_tresc', 'Indeks pełnotekstowy: tekst stron książek (jedna strona = jeden wiersz)',
  {'id_ksiazki': 'Książka (tbl_ksiazki)', 'strona': 'Numer strony', 'tekst': 'Tekst strony',
   'wektor_tekstu': 'Wektor wyszukiwania tsvector (fn_bez_akcentow + konfiguracja simple), liczony przez program'})
t('tbl_narzedzia', 'Narzędzia warsztatowe użytkownika',
  {'nazwa': 'Nazwa narzędzia', 'producent': 'Producent', 'model': 'Model', 'kategoria': 'Kategoria (np. Lutowanie, Pomiary)',
   'numer_seryjny': 'Numer seryjny', 'numer_inwentarzowy': 'Własny numer inwentarzowy (np. z naklejki)', 'miejsce': 'Gdzie narzędzie jest przechowywane', 'data_zakupu': 'Data zakupu',
   'cena': 'Cena zakupu w złotych', 'zdjecie': 'Ścieżka zdjęcia narzędzia', 'instrukcja': 'Ścieżka pliku PDF lub adres strony z instrukcją'})
t('tbl_pozyczki', 'Pożyczki narzędzi i książek; otwarta pożyczka to taka bez daty zwrotu (jeden przedmiot tylko jednej osobie naraz)',
  {'id_narzedzia': 'Pożyczone narzędzie (tbl_narzedzia); dokładnie jedno z id_narzedzia i id_ksiazki', 'id_ksiazki': 'Pożyczona książka (tbl_ksiazki)',
   'osoba': 'Komu pożyczono', 'data_pozyczenia': 'Data pożyczenia', 'termin': 'Termin zwrotu (opcjonalny)', 'data_zwrotu': 'Data zwrotu (pusta = nadal pożyczone)'})

WIDOKI = {
    'wid_kategorie': 'Drzewo kategorii z pełną nazwą i liczbą elementów',
    'wid_elementy': 'Elementy z nazwą kategorii i znacznikiem niskiego stanu',
    'wid_niskie_stany': 'Elementy poniżej stanu minimalnego wraz z brakującą ilością',
    'wid_sklepy_elementow': 'Linki elementów do sklepów z cenami i dostępnością',
    'wid_najlepsze_ceny': 'Ceny elementów w sklepach z rankingiem (1 = najtańszy)',
    'wid_realizacja_projektow': 'Stan realizacji list części projektów: wymagane, zużyte, dostępne i brakujące sztuki',
}
FUNKCJE = {
    'fn_kodowanie_url(text)': 'Kodowanie procentowe tekstu do użycia w adresach URL (UTF-8)',
    'fn_ustaw_zmieniono()': 'Trigger: ustawia kolumnę zmieniono na bieżący czas przy każdej zmianie wiersza',
    'fn_kategorie_sciezka()': 'Trigger: liczy ścieżkę ltree i poziom kategorii oraz pilnuje, by kategoria nie była własnym potomkiem',
    'fn_kategorie_potomkowie()': 'Trigger: po przeniesieniu kategorii przelicza ścieżki jej potomków',
    'fn_dodaj_kategorie_jesli_brak(bigint, character varying, character varying)': 'Dodaje kategorię, jeśli jeszcze nie istnieje; zwraca jej id',
    'fn_utworz_link_sklepu(bigint, character varying, character varying)': 'Buduje adres wyszukiwania elementu w sklepie',
    'fn_tworz_domyslne_linki_sklepow()': 'Trigger: po dodaniu elementu tworzy linki do najważniejszych sklepów (maks. 5)',
    'fn_jeden_domyslny_sklep()': 'Trigger: utrzymuje najwyżej jeden domyślny link sklepu na element',
    'fn_historia_cen()': 'Trigger: każda zmiana ceny aktualnej trafia do tbl_historia_cen_sklepy',
    'fn_zmien_ilosc_elementu(bigint, integer, character varying, bigint, bigint, character varying, character varying, numeric)': 'Zmienia stan elementu i zapisuje ruch w tbl_transakcje; nie pozwala zejść poniżej zera; zwraca id transakcji',
    'fn_bez_akcentow(text)': 'Usuwa znaki diakrytyczne (polskie i popularne europejskie) bez rozszerzenia unaccent',
    'fn_szukaj_elementow(text, integer)': 'Wyszukuje elementy po fragmentach tekstu (bez wielkości liter i polskich znaków)',
    'fn_pomoc_pol_zmiana()': 'Trigger: ręczna zmiana opisu pola ustawia flagę zmienione',
    'fn_klucz_obudowy(text)': 'Klucz porównawczy nazwy obudowy: małe litery i cyfry (TO-220 = TO220)',
    'fn_tekst_obudow_elementu(bigint, bigint)': 'Tekst obudów elementu "A, B, C" z relacji tbl_elementy_obudowy',
    'fn_elementy_obudowa_sync()': 'Trigger: utrzymuje relację tbl_elementy_obudowy z tekstu tbl_elementy.obudowa',
    'fn_obudowy_zmiana_nazwy()': 'Trigger: po zmianie nazwy obudowy odświeża teksty obudów elementów',
    'fn_obudowy_usuniecie()': 'Trigger: przed usunięciem obudowy odświeża teksty obudów elementów',
}

def q(s): return "'" + s.replace("'", "''") + "'"

def slownik_kolumna(k):
    m = re.match(r'(tekst|liczba|rzeczywista|dziesietna|flaga)_(\d\d)$', k)
    if not m: return None
    rodzaj = {'tekst': 'tekstowe', 'liczba': 'całkowite', 'rzeczywista': 'rzeczywiste', 'dziesietna': 'dziesiętne', 'flaga': 'logiczne'}[m.group(1)]
    return 'Pole %s ogólnego przeznaczenia nr %d' % (rodzaj, int(m.group(2)))

def main():
    out = ['-- =====================================================================',
           '-- Magmass - opisy tabel, kolumn, widoków i funkcji (COMMENT ON)',
           '-- Plik generowany: python3 narzedzia/zbuduj_opisy.py. Nie edytuj ręcznie. Idempotentny, PostgreSQL 9.6+.',
           '-- =====================================================================', '']
    for tab, (opis, kol) in T.items():
        out.append('COMMENT ON TABLE public.%s IS %s;' % (tab, q(opis)))
        for k, v in kol.items():
            out.append('COMMENT ON COLUMN public.%s.%s IS %s;' % (tab, k, q(v)))
        for k, v in WSPOLNE.items():
            if k not in kol:
                out.append('COMMENT ON COLUMN public.%s.%s IS %s;' % (tab, k, q(v)))
        if tab in ('tbl_slownik', 'tbl_opis_slownika'):
            for pref in ('tekst', 'liczba', 'rzeczywista', 'dziesietna', 'flaga'):
                for n in range(1, 13):
                    k = '%s_%02d' % (pref, n)
                    d = slownik_kolumna(k)
                    if tab == 'tbl_opis_slownika': d = 'Opis (nazwa) kolumny ' + k + ' w tbl_slownik'
                    out.append('COMMENT ON COLUMN public.%s.%s IS %s;' % (tab, k, q(d)))
        out.append('')
    # nie każda tabela ma wszystkie kolumny wspólne: pomijamy te, których nie ma (DO-blok z obsługą braku)
    sql = '\n'.join(out)
    # bezpieczne wykonanie: każdy COMMENT w bloku, który pomija nieistniejące kolumny
    wiersze = [l for l in out if l.startswith('COMMENT ON COLUMN')]
    tabele = [l for l in out if l.startswith('COMMENT ON TABLE')]
    res = out[:5]
    res.append('DO $opisy$')
    res.append('DECLARE r record;')
    res.append('BEGIN')
    res.append('    FOR r IN SELECT * FROM (VALUES')
    wpisy = []
    for tab, (opis, kol) in T.items():
        wpisy.append('(%s, NULL::text, %s)' % (q(tab), q(opis)))
        wsz = dict(kol)
        for k, v in WSPOLNE.items(): wsz.setdefault(k, v)
        if tab in ('tbl_slownik', 'tbl_opis_slownika'):
            for pref in ('tekst', 'liczba', 'rzeczywista', 'dziesietna', 'flaga'):
                for n in range(1, 13):
                    k = '%s_%02d' % (pref, n)
                    wsz[k] = ('Opis (nazwa) kolumny %s w tbl_slownik' % k) if tab == 'tbl_opis_slownika' else slownik_kolumna(k)
        for k, v in wsz.items():
            wpisy.append('(%s, %s, %s)' % (q(tab), q(k), q(v)))
    res.append('        ' + ',\n        '.join(wpisy))
    res.append('    ) AS x(tabela, kolumna, opis)')
    res.append('    LOOP')
    res.append("        IF r.kolumna IS NULL THEN")
    res.append("            IF to_regclass('public.' || r.tabela) IS NOT NULL THEN")
    res.append("                EXECUTE format('COMMENT ON TABLE public.%I IS %L', r.tabela, r.opis);")
    res.append("            END IF;")
    res.append("        ELSIF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema = 'public'")
    res.append("                      AND table_name = r.tabela AND column_name = r.kolumna) THEN")
    res.append("            EXECUTE format('COMMENT ON COLUMN public.%I.%I IS %L', r.tabela, r.kolumna, r.opis);")
    res.append("        END IF;")
    res.append('    END LOOP;')
    res.append('END $opisy$;')
    res.append('')
    for w, o in WIDOKI.items():
        res.append('COMMENT ON VIEW public.%s IS %s;' % (w, q(o)))
    res.append('')
    for f, o in FUNKCJE.items():
        res.append('COMMENT ON FUNCTION public.%s IS %s;' % (f, q(o)))
    res.append('')
    kat = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    with open(os.path.join(kat, 'sql', 'opisy_obiektow.sql'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(res))
    print('OK: %d tabel, %d widoków, %d funkcji' % (len(T), len(WIDOKI), len(FUNKCJE)))

main()
