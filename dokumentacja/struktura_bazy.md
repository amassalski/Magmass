# Struktura bazy

Skrypty w katalogu `sql/` (kolejność wykonania):

| Plik | Zawartość |
|------|-----------|
| `migracja_nazw.sql` | przenosi starsze bazy na obecne nazwy obiektów (uruchamiany pierwszy; na świeżej bazie nic nie robi) |
| `schema.sql` | tabele, indeksy, tabela wersji struktury (`tbl_wersja_schematu`) |
| `funkcje_i_trigger.sql` | funkcje i triggery |
| `widoki.sql` | widoki |
| `dane_startowe.sql` | sklepy i kategorie startowe |
| `pomoc_pola.sql` | opisy pól formularzy (generowany z pomoc/pola.txt) |
| `opisy_obiektow.sql` | opisy (COMMENT) tabel, kolumn, widoków i funkcji (generowany) |

Wszystkie skrypty są idempotentne (można je uruchamiać wielokrotnie).

## Funkcje

- `fn_zmien_ilosc_elementu(id_elementu, zmiana, typ, id_projektu, id_lokalizacji, komentarz, uzytkownik, cena)` zmienia stan, zapisuje transakcję i pilnuje, by stan nie spadł poniżej zera.
- `fn_szukaj_elementow(tekst, limit)` szuka po symbolu, producencie, opisie, wartości, obudowie i tagach, bez rozróżniania wielkości liter i polskich znaków.
- `fn_dodaj_kategorie_jesli_brak(id_rodzica, nazwa, opis)`.
- `fn_utworz_link_sklepu(id_sklepu, kod_producenta, symbol)`, `fn_kodowanie_url(tekst)`.

## Triggery

- ścieżka (`ltree`) i poziom kategorii są liczone automatycznie, także po zmianie rodzica; zabezpieczenie przed cyklami,
- nowy element dostaje linki do 5 najważniejszych sklepów,
- najwyżej jeden domyślny link sklepu na element,
- każda zmiana ceny w sklepie trafia do `tbl_historia_cen_sklepy`,
- automatyczne daty modyfikacji.

## Widoki

`wid_kategorie`, `wid_elementy`, `wid_niskie_stany`, `wid_sklepy_elementow`, `wid_najlepsze_ceny`, `wid_realizacja_projektow`.

## Wersja 3 (sklepy i ceny)

- `tbl_sklepy`: `szablon_wyszukiwania` (adres z `{q}`), `koszt_dostawy`, `darmowa_dostawa_od`, `regex_ceny`.
- `tbl_elementy_sklepy`: `kod_w_sklepie`. Historię cen zapisuje wyzwalacz `fn_historia_cen` (źródło z ustawienia sesji `dm.zrodlo_ceny`: `reczna`, `strona`).
- `tbl_kursy_walut`: kursy do złotego, wpisywane ręcznie.
- `fn_utworz_link_sklepu` używa `szablon_wyszukiwania`, a bez niego dawnej pary `url_bazowy` + `parametr_wyszukiwania`.

## Wersja 4 (pomoc kontekstowa)

- `tbl_pomoc_pol`: opisy pól formularzy (`klucz`, `tytul`, `opis`, `kotwica` w pliku pomocy). Zasilana skryptem `sql/pomoc_pola.sql`, generowanym z `pomoc/pola.txt` przez `narzedzia/zbuduj_pomoc.py`.
- Kolumna `zmienione`: wyzwalacz `fn_pomoc_pol_zmiana` ustawia ją na `true` przy ręcznej zmianie wiersza. Takiego opisu aktualizacja struktury nie nadpisuje. Żeby wrócić do opisu fabrycznego, usuń wiersz i uruchom aktualizację struktury.

## Wersja 5 (miejsce, tolerancja, parametry)

- `tbl_elementy`: `kontener`, `szuflada`, `przegroda` (miejsce magazynowania, każda część opcjonalna), `tolerancja` (np. `5%`) i `parametry` (swobodny tekst, np. `0,1 W; X7R`).
- Widoki `wid_elementy`, `wid_niskie_stany` i raport stanu magazynu zawierają nowe kolumny. Każda zmiana używa `ADD COLUMN IF NOT EXISTS`, więc aktualizacja struktury jest bezpieczna dla istniejących danych.

## Wersja 6: katalog elementów
- `tbl_katalog` (symbol, producent, opis, obudowa, rodzina, datasheet_url, datasheet_plik, uwagi); unikalny klucz (lower(symbol), lower(producent)).
- `tbl_katalog_parametry` (id_katalogu → kaskadowo, nazwa, wartosc, zrodlo: reczny/csv/datasheet, kolejnosc); nazwa unikalna w obrębie pozycji.
- Powiązanie z `tbl_elementy` po symbolu / symbolu producenta (bez klucza obcego).

## Obudowy (schemat 8)
- `tbl_obudowy` (nazwa, opis, montaz, liczba_pinow, moc_strat_w, napiecie_max_v, rozstaw, rodzina, wariant, uwagi): słownik; nazwa jednoznaczna po kluczu `fn_klucz_obudowy` (małe litery i cyfry).
- `tbl_elementy_obudowy` (id_elementu, id_obudowy, kolejnosc): relacja element - wiele obudów.
- `tbl_obudowy_typowe` (id_obudowy, zestaw, opis, prefiks): typowe elementy w obudowie z zestawów (devicesets) bibliotek Eagle; służą do podpowiadania obudowy dla symbolu (schemat 11).
- `tbl_elementy.obudowa` zostaje jako tekst "R7.5, R10" utrzymywany przez trigger `trg_elementy_obudowa_sync` (zapis tekstu rozbija go na relację i dodaje brakujące obudowy do słownika); `trg_obudowy_zmiana_nazwy` i `trg_obudowy_usuniecie` odświeżają teksty elementów.

## Literatura (schemat 9)
- `tbl_ksiazki` (tytul, autor, wydawca, rok, isbn, jezyk, tagi, uwagi, sciezka, strony, zaindeksowano); unikalna ścieżka (bez rozróżniania wielkości liter).
- `tbl_ksiazki_tresc` (id_ksiazki, strona, tekst, wektor_tekstu): tekst stron PDF; `wektor_tekstu` = `to_tsvector('simple', fn_bez_akcentow(lower(tekst)))` liczone przez aplikację, indeks GIN.

## Narzędzia i pożyczki (schemat 10)
- `tbl_narzedzia` (nazwa, producent, model, kategoria, numer_seryjny, miejsce, data_zakupu, cena, zdjecie, instrukcja, uwagi).
- `tbl_pozyczki` (id_narzedzia, osoba, data_pozyczenia, termin, data_zwrotu, uwagi); otwarta pożyczka = brak daty zwrotu, indeks unikalny pozwala na jedną otwartą pożyczkę narzędzia.

## Konwencja nazw (od wersji 13)

| Obiekt | Wzorzec | Przykład |
|---|---|---|
| tabela | `tbl_<nazwa>` (po polsku, liczba mnoga) | `tbl_pozycje_projektow` |
| widok | `wid_<nazwa>` | `wid_niskie_stany` |
| funkcja | `fn_<nazwa>` | `fn_zmien_ilosc_elementu` |
| trigger | `trg_<tabela>_<opis>` | `trg_elementy_zmieniono` |
| indeks / ograniczenie | `idx_`, `uq_`, `chk_` + `<tabela>_<opis>` | `uq_tbl_katalog_symbol_producent` |
| klucz obcy | `id_<obiekt>` | `id_projektu` |
| znaczniki czasu | `dodano`, `zmieniono` (aktualizowane triggerem `fn_ustaw_zmieniono`) | |
| autor zmiany | `uzytkownik_dod`, `uzytkownik_mod` | |

Wersja 13 zmieniła nazwy: `tbl_bom` → `tbl_pozycje_projektow`; widoki `v_*` → `wid_*`; funkcje bez prefiksu dostały `fn_`
(`dm_unaccent` → `fn_bez_akcentow`, `fn_url_encode` → `fn_kodowanie_url`); kolumny `modyfikowano`, `data_modyfikacji` → `zmieniono`,
`data_dodania`, `data_utworzenia` → `dodano`, `dodano_przez` → `uzytkownik_dod`, `tsv` → `wektor_tekstu`; kolumny słowników ogólnych
`character_NN`, `bigint_NN`... → `tekst_NN`, `liczba_NN`... Istniejące bazy przenosi automatycznie `sql/migracja_nazw.sql`
(uruchamiany przed `schema.sql`, bez utraty danych).

## Opisy obiektów

Każda tabela, kolumna, widok i funkcja ma opis (`COMMENT ON`), widoczny np. w pgAdmin lub poleceniem `\d+ tbl_elementy` w psql.
Opisy są w `sql/opisy_obiektow.sql`, generowanym przez `python3 narzedzia/zbuduj_opisy.py` (źródło opisów jest w tym skrypcie);
test w `tests/test_bazy.cpp` pilnuje, żeby żaden obiekt nie został bez opisu.

## Wersja 14
`tbl_katalog.zdjecie` (ścieżka zdjęcia pozycji katalogu), `tbl_narzedzia.numer_inwentarzowy`.
