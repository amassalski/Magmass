# Magmass: instrukcja użytkownika (etap 3: projekty, raporty, etykiety, sklepy i ceny)

## Pomoc, F1 i dymki
- **Pomoc → Spis treści** (F1 w oknie głównym) otwiera pomoc w oknie programu, w układzie wzorowanym na pomocy Eagle'a 7.6: spis treści, odnośniki, przyciski Wstecz / Dalej. **Otwórz pomoc w przeglądarce** otwiera ten sam plik (`pomoc/pomoc_pl.htm`) w przeglądarce systemowej.
- **F1 w polu formularza** pokazuje opis tego pola z przyciskiem *Więcej w pomocy*, który otwiera właściwy rozdział. Ten sam opis pojawia się w dymku po zatrzymaniu myszy nad polem.
- Nad listami (elementy, projekty, pozycje projektu, sklepy, linki, raporty) dymek pokazuje pełne dane wiersza, także to, co nie mieści się w kolumnach.
- Opisy pól są w bazie (tabela `tbl_pomoc_pol`) i można je poprawić; ręcznie zmieniony opis nie jest nadpisywany przy aktualizacji struktury bazy. Bez bazy program używa pliku `pomoc/pola.txt`.
- Pomoc budowana jest z `pomoc/pomoc_pl.src.htm` poleceniem `python3 narzedzia/zbuduj_pomoc.py` (generuje spis treści, `pomoc_pl.htm` i `sql/pomoc_pola.sql` oraz sprawdza odnośniki).
- **Po aktualizacji programu** wykonaj *Baza danych → Utwórz / zaktualizuj strukturę bazy* (schemat wersja 4).

## Okno główne
Po lewej jest drzewo kategorii, po prawej lista elementów wybranej kategorii, na dole
dziennik zdarzeń (można go ukryć: **Widok → Pokaż dziennik zdarzeń**). Panele są aktywne
dopiero po połączeniu z bazą, która ma strukturę programu.

## Kategorie
- **Nowa** dodaje kategorię główną, **Podkategoria** dodaje kategorię w zaznaczonej.
- **Zmień nazwę** (F2) i **Usuń** (Delete) działają na zaznaczonej kategorii. Usunąć można
  tylko pustą kategorię (bez podkategorii i elementów).
- Kategorię można przenieść przeciągnięciem na inną kategorię albo na „Wszystkie elementy”
  (poziom główny).
- Liczba w nawiasie to liczba elementów w kategorii razem z podkategoriami.
- **Magazyn → Importuj kategorie startowe** dodaje brakujące kategorie z pliku
  `sql/kategorie_startowe.txt`. **Importuj kategorie z pliku...** robi to samo z własnym
  plikiem tekstowym: jedna ścieżka w linii, nazwy rozdzielone znakiem ` / ` (ze spacjami)
  albo `>`, linie z `#` są pomijane. Istniejące kategorie są pomijane, więc import można
  powtarzać.

## Elementy
- **Dodaj element** (Ctrl+N, Insert na liście) otwiera formularz. Wymagany jest tylko symbol.
  Nowy element trafia do zaznaczonej kategorii. Ilość początkowa jest zapisywana w historii
  jako przyjęcie.
- **Edytuj** (Ctrl+E albo dwukrotne kliknięcie). Zmiana ilości dostępnej jest zapisywana
  w historii magazynu jako korekta.
- **Prawy przycisk myszy** na elemencie: Edytuj, Zmień stan, Dodaj do katalogu, Pokaż w katalogu, Pokaż datasheet (pobrany PDF albo zapisany adres), Usuń. Niedostępne pozycje są wyszarzone.
- **Magazyn / Obudowy...**: słownik obudów (R7.5, TO220AH, 0603...) z atrybutami: rodzina, wariant, montaż THT/SMD, liczba wyprowadzeń, moc strat, dopuszczalne napięcie i rozstaw. Element ma relację 1-wiele z obudowami: pole *Obudowy* w edycji jest tylko do odczytu (lista po przecinku, np. `R7.5, R10, R15L`), a przycisk **...** otwiera wybór ze słownika. Stan magazynowy jest wspólny dla elementu, obudowy to informacja, w których pasuje. *Wczytaj bibliotekę Eagle (.lbr)* dodaje obudowę dla każdego pakietu (moc i napięcie są czytane z opisu pakietu), *Uzupełnij z elementów* dodaje obudowy z katalogu. Pozycje z projektu w różnych obudowach tego samego elementu (np. 10k w R7.5 i R10) sumują się w jednej pozycji listy części. Części różne fizycznie (np. 2 W w R20 i R25) zakładaj jako osobny element.
- **Zmień stan** (Ctrl+T): przyjęcie, wydanie albo ustawienie stanu (inwentaryzacja), z komentarzem.
  Stan nie może spaść poniżej zera.
- **Usuń** (Delete): element bez powiązań jest kasowany razem z historią ilości. Element
  używany (lista części projektu, zamienniki, lokalizacje) jest tylko ukrywany.
- Elementy poniżej stanu minimalnego są wyróżnione kolorem. Pole **Stan minimalny** = 0
  wyłącza ostrzeżenie.
- Kliknięcie nagłówka kolumny sortuje listę, ponowne kliknięcie odwraca kolejność.

## Wyszukiwanie
Pole nad listą (Ctrl+F) szuka po symbolu, symbolu producenta, producencie, opisie, wartości,
obudowie i tagach. Wielkość liter i polskie znaki nie mają znaczenia („zolty” znajdzie
„żółty”). Kilka słów oznacza, że każde musi wystąpić. Wyszukiwanie dotyczy wybranej
kategorii; **Z podkategoriami** i **Tylko niski stan** zawężają lub rozszerzają wynik.

## Eksport do CSV
**Magazyn → Eksportuj listę do CSV...** zapisuje elementy widoczne na liście (z bieżącym
filtrem i sortowaniem). Format: UTF-8 z BOM, średnik jako separator, przecinek dziesiętny,
więc plik otwiera się w polskim Excelu bez ustawień.

## Projekty i lista części (BOM)

Zakładka **Projekty** (oraz menu **Projekty**) służy do planowania projektów i pilnowania, czy masz do nich komplet części.

**Menu pozycji.** Prawy przycisk myszy na pozycji listy części: „Idź do elementu w magazynie" przełącza na zakładkę Magazyn i zaznacza ten element (filtry kategorii i wyszukiwania są czyszczone), a także Edytuj i Usuń.

**Projekty.** „Nowy projekt" zakłada projekt (nazwa, status: planowany / w trakcie / wstrzymany / zakończony, opis). Na liście projektów kolor pomarańczowy oznacza, że czegoś brakuje w magazynie. „Usuń" kasuje projekt razem z listą części; jeśli wydano już do niego części, projekt jest tylko archiwizowany (pokażesz go polem „Pokaż zarchiwizowane").

**Lista części projektu.** Po prawej widać: oznaczenia (R1, R2...), symbol, wartość, obudowę, ile potrzeba, ile już wydano, ile jest w magazynie i ile brakuje. Pomarańczowy wiersz = brakuje w magazynie, zielony = wydano w całości.
- „Dodaj część..." dodaje element z magazynu (jeśli już jest w projekcie, ilość się zwiększa).
- „Edytuj pozycję..." zmienia oznaczenia, ilość, ilość zamówioną, priorytet i uwagi.
- „Wydaj z magazynu..." zdejmuje wybraną ilość ze stanu magazynu (zapis w historii jako WYDANIE dla tego projektu); „Wydaj wszystko dostępne" robi to dla całej listy naraz.
- „Lista zakupów (CSV)..." zapisuje to, czego brakuje (z uwzględnieniem tego, co już zamówiono), w formacie czytelnym dla polskiego Excela.

**Import listy części z KiCada i Eagle'a** (Ctrl+I albo „Importuj BOM..."):
- KiCad: raport BOM w CSV (Plik → Wytwórz → Lista materiałowa) albo netlista XML.
- Eagle: plik schematu `.sch` albo CSV wyeksportowany skryptem BOM.
- Pozycje o tej samej wartości i obudowie są łączone, części oznaczone DNP / „nie montować" pomijane.
- Program proponuje dla każdej pozycji element z magazynu (ten sam symbol albo ta sama wartość i obudowa); zielone = istniejący element, niebieskie = zostanie utworzony nowy (ze stanem 0), szare = pominięte. Wybór można zmienić przyciskami pod listą, zanim cokolwiek zostanie zapisane.
- „Liczba płytek" mnoży ilości. „Zastąp obecną listę" czyści pozycje, z których jeszcze nic nie wydano.
- Ponowny import tego samego pliku zwiększa ilości, nie dubluje elementów.

Uwaga: pliki `.kicad_sch` (sam schemat KiCada 6+) nie są wczytywane. Wyeksportuj BOM albo netlistę.

## Raporty (menu Raporty, Ctrl+R)

Dostępne raporty: stan magazynu (z wartością), niskie stany (z ilością do zamówienia), wartość magazynu wg kategorii głównych, braki w projektach oraz historia zmian stanów (z zakresem dat RRRR-MM-DD). Raport widać w tabeli; zapisz go jako CSV (do Excela), jako HTML albo kliknij „Otwórz do wydruku / PDF", co otwiera go w przeglądarce, gdzie można wydrukować lub zapisać jako PDF. Wartości liczone są tylko dla elementów z podaną ceną; sumy podawane są osobno dla każdej waluty.

## Etykiety z kodem kreskowym (Magazyn, Ctrl+P)

Okno drukuje etykiety dla zaznaczonego elementu albo dla wszystkich elementów z bieżącej listy (np. po przefiltrowaniu kategorią). Na etykiecie jest symbol, wartość i obudowa, producent oraz kod kreskowy z numerem elementu. Typ kodu (EAN-13, EAN-8, Code128) i prefiks ustawia się w Ustawieniach aplikacji.
- Formaty arkuszy: A4 3x8, 3x7, 5x13, 2x4 oraz etykiety pojedyncze 50x25 i 62x29 mm.
- „Zacznij od etykiety nr" pozwala dokończyć częściowo zużyty arkusz; przesunięcia korygują położenie wydruku. Na początek wydrukuj próbę z ramkami na zwykłym papierze.
- „Podgląd wydruku" i „Drukuj" korzystają z systemowego okna drukowania (tam też „Zapisz jako PDF"); „Zapisz jako PNG" zapisuje strony jako obrazy.

## Skanowanie kodów i zdjęcia

- Skaner USB (działa jak klawiatura): kliknij pole „Szukaj" i zeskanuj etykietę. Kod z naszym prefiksem od razu pokazuje element.
- „Znajdź element po zdjęciu kodu..." (menu Magazyn) odczytuje kod z pliku obrazu, np. zdjęcia etykiety zrobionego telefonem.
- W formularzu elementu przycisk „Wybierz..." przy zdjęciu kopiuje obraz do katalogu zdjęć z ustawień.
- Podgląd na żywo z kamery komputera nie jest jeszcze zrealizowany.

## Sklepy, ceny i datasheety (menu Sklepy)

**Po aktualizacji programu** wybierz *Baza danych → Utwórz / zaktualizuj strukturę bazy* (schemat wersja 3: dostawa, adresy wyszukiwania, kursy walut).

### Lista sklepów (Ctrl+Shift+S)
- *Dodaj / Zmień / Usuń*: nazwa, adres strony, **adres wyszukiwania** (z `{q}` w miejscu szukanej frazy, np. `https://sklep.pl/szukaj?q={q}`), waluta, **koszt dostawy**, **próg darmowej dostawy**, priorytet i aktywność. Sklep użyty w linkach elementów nie jest kasowany, tylko wyłączany.
- *Wczytaj listę...*: wklej wiele sklepów naraz (jeden w wierszu, pola oddzielone średnikiem lub tabulatorem, także wprost z Excela): `nazwa; adres strony; adres wyszukiwania; waluta; dostawa; darmowa od`. Sklep o tej samej nazwie jest aktualizowany. Można też wczytać plik albo wstawić bieżącą listę, poprawić ją i wczytać z powrotem.
- *Kursy walut...*: ile złotych kosztuje jednostka waluty (`USD = 4,05`). Kursy wpisuje się ręcznie, program nie pobiera ich z internetu; domyślne wartości są tylko przykładowe, **zaktualizuj je**.
- Adresy wyszukiwania sklepów startowych nie były sprawdzane na działających stronach: popraw je, jeśli sklep zmienił wyszukiwarkę.

### Sklepy, ceny i datasheet elementu (Ctrl+K, po zaznaczeniu elementu)
- Każdy nowy element dostaje automatycznie linki wyszukiwania do głównych sklepów. Żeby program mógł sprawdzać ceny, wklej w *Zmień...* adres **strony samego produktu** (przycisk *Szukaj w sklepie* otwiera wyniki wyszukiwania w przeglądarce). Cenę można też wpisać ręcznie.
- *Sprawdź ceny wszystkich / zaznaczony*: program pobiera strony po jednej, z przerwą, **tylko na Twoje polecenie**. Cenę szuka w danych strukturalnych strony (JSON-LD schema.org, znaczniki `product:price:amount`, `itemprop="price"`). Dla sklepu, który ich nie podaje, ustaw w oknie sklepu **wzorzec ceny**: wyrażenie regularne z jedną grupą w nawiasie, np. `cena-brutto">([0-9 ,.]+)`. Program niczego nie zgaduje z przypadkowych liczb na stronie. Zmiany cen trafiają do historii (*Historia cen...*).
- **Datasheet**: *Otwórz* (plik z katalogu dokumentów albo adres w przeglądarce), *Pobierz z adresu...* (sprawdza, że to naprawdę PDF; plik trafia do katalogu z Ustawień i ma nazwę z symbolu elementu), *Znajdź na stronach sklepów* (szuka odnośników do PDF na stronach linków elementu) oraz *Szukaj w internecie...* (Google, Octopart, AllDatasheet w przeglądarce).

### Najtańsze zestawienie zakupów (Ctrl+Shift+Z)
Zestawia braki wybranego projektu (albo wszystkich aktywnych) i rozdziela je między sklepy:
- *Najniższa suma z dostawą*: uwzględnia koszt dostawy i progi darmowej dostawy, więc czasem opłaca się kupić całość w jednym sklepie, choć pojedyncze części są tam droższe; okno pokazuje, ile łączenie zakupów oszczędza,
- *Najniższe ceny części*: każda część tam, gdzie jest najtańsza,
- *Wszystko w jednym sklepie*.
Ceny w obcych walutach są przeliczane wg kursów z listy sklepów. Pozycje bez ceny w żadnym sklepie są wypisane osobno. Opcja *Odejmij już zamówione* uwzględnia kolumnę „zamówione” z projektu. *Sprawdź aktualne ceny* odświeża ceny linków potrzebnych części. Wynik można skopiować jako listę zamówień (sklep → ilość x symbol) albo zapisać do CSV i HTML.

**Ograniczenia:** programu nie sprawdzano na prawdziwych sklepach (sklepy często blokują automatyczne zapytania i zmieniają strony), dlatego wzorce i adresy są do ustawienia; wiele sklepów podaje cenę dopiero po zalogowaniu albo dynamicznie (JavaScript), czego program nie odczyta. Wtedy wpisz cenę ręcznie.

## Miejsce magazynowania elementu

W formularzu elementu podajesz kontener, szufladę i opcjonalnie przegrodę, np. `Kontener 1 szufladka A1/1` albo samo `A1`. Zapis swobodny w polu kontenera program sam rozdzieli na części. Miejsce widać na liście elementów, w etykietach, w eksporcie CSV i w raporcie stanu.

## Generator szeregów E (Magazyn)

Wprowadza cały zestaw rezystorów, kondensatorów lub cewek z szeregu E6, E12, E24, E48 albo E96 w wybranym zakresie. Wartości wpisujesz zapisem `4k7`, `100n`, `2M2`. Podaj obudowę, tolerancję, ilość, minimum, miejsce (opcja: osobna szuflada na każdą dekadę), kategorię i wzór symbolu. Lista podglądu pokazuje, które elementy są nowe, a które już są w magazynie. Istniejące symbole można pominąć, uzupełnić ilością albo danymi. Zestaw można też zapisać jako CSV.

## Import elementów z pliku CSV (Magazyn)

Wczytuje dowolną tabelę elementów (rezystory, kondensatory, tranzystory itd.). Wymagana jest kolumna `Symbol`. Rozpoznawane kolumny: Wartość, Tolerancja, Obudowa, Parametry, Ilość, Minimum, Kategoria, Miejsce, Producent, Cena, Tagi i inne. Przycisk *Zapisz szablon CSV* tworzy plik wzorcowy. Brakujące kategorie są tworzone, błędne wiersze raportowane po imporcie.

Uwaga: po aktualizacji programu uruchom `Baza danych / Utwórz / zaktualizuj strukturę bazy` (struktura w wersji 5).

## Narzędzia (Ctrl+D)

*Paski kolorów i kody SMD* odczytuje paski rezystorów (3 do 6) i cewek (3 lub 4); lista kolorów zależy od roli paska, a brak paska tolerancji w kodzie czteropaskowym to ±20 %, pokazuje paski dla wpisanej wartości oraz rozpoznaje kody SMD (472, 4R7, EIA-96 jak 01A, kondensatory jak 104).

## Ikony

Ikony programu i menu są własnym, płaskim zestawem rysowanym skryptem `narzedzia/zbuduj_ikony.py` i wkompilowanym w program. Zmiana wyglądu to edycja skryptu i ponowne uruchomienie: `python3 narzedzia/zbuduj_ikony.py`.

## Zdjęcia elementów i kamera USB

Po prawej stronie listy elementów widać zdjęcie i dane zaznaczonego elementu. Przycisk *Zdjęcie z kamery* otwiera podgląd na żywo: *Zrób zdjęcie*, potem *Użyj zdjęcia* (albo *Powtórz*). Zdjęcie jest zapisywane jako JPEG w katalogu zdjęć z ustawień i przypisywane do elementu. To samo robi przycisk *Z kamery* w formularzu elementu. Zdjęcie można też wskazać z pliku, a *Usuń zdjęcie* odpina je od elementu (plik zostaje).

### Wymagania do budowania z obsługą kamery

Kamerę obsługuje OpenCV (tylko moduły `core` i `videoio`). CMake wykrywa je sam (opcja `-DDM_KAMERA=AUTO|ON|OFF`):

- MSYS2: `pacman -S mingw-w64-ucrt-x86_64-opencv`
- macOS: `brew install opencv`
- Linux (Debian/Ubuntu): `sudo apt install libopencv-dev`

Pakiet OpenCV z MSYS2 ciągnie za sobą ffmpeg i dziesiątki bibliotek DLL (w paczce dla Windows ok. 150 plików). Żeby ich uniknąć, skrypty `narzedzia/zbuduj_windows.sh` i `narzedzia/zbuduj_macos.sh` mają opcję `--wlasny-opencv`: `narzedzia/zbuduj_opencv.sh` pobiera źródła OpenCV (domyślnie 4.10.0, zmienna `OPENCV_WERSJA`) do katalogu `build-opencv/`, buduje tylko `core`, `imgproc`, `imgcodecs` i `videoio` statycznie, bez ffmpeg (kamera działa przez DirectShow (Windows) albo AVFoundation) i wkompilowuje je w program. Pierwsze budowanie trwa kilka do kilkunastu minut i wymaga internetu, następne używają zapamiętanego wyniku. Źródła OpenCV (Apache 2.0) nie są częścią repozytorium. Użyj opcji przy świeżym katalogu budowania, np. `narzedzia/zbuduj_windows.sh --wlasny-opencv`.

Bez OpenCV program buduje się i działa normalnie, a okno kamery informuje, że obsługi nie ma. Do sprawdzania okna bez kamery służy zmienna środowiskowa `DM_KAMERA_TEST=1` (generowany obraz).

## Współpraca z Eagle CAD 7.6 (menu Projekty)

- *Program ULP dla Eagle* zapisuje `dm_bom.ulp`. Uruchomiony w edytorze schematu (File / Run ULP) otwiera okno znane z standardowego `bom.ulp` (wariant montażu, lista części lub wartości). Format *Magmass CSV* ma kolumny Reference;Value;Package;MPN;Manufacturer;Description;Qty (MPN i producent z atrybutów `MPN` i `MANUFACTURER`) i importujesz go jak każdą listę części. Skrypt nie był uruchamiany w prawdziwym Eagle 7.6 – w razie błędu podaj jego treść.
- *Skrypt atrybutów dla Eagle* zapisuje plik `.scr` dla wybranego projektu. Wykonany w edytorze schematu (File / Execute Script) ustawia w częściach atrybuty `DM_SYMBOL`, `DM_OBUDOWA`, `DM_STAN`, `DM_MIEJSCE`, `MPN` i `MANUFACTURER`. Wartości są zapisywane bez polskich znaków.

Skrypty były sprawdzane testami na składnię i zgodność z importem, ale nie w samym Eagle. Jeśli polecenie `ATTRIBUTE` w Twojej wersji wymaga innej składni, daj znać.

## Katalog elementów (zakładka Katalog)
Katalog to baza wiedzy o elementach, niezależna od stanów magazynowych. Pozycje łączą się z magazynem po symbolu (symbol lub symbol producenta, bez względu na wielkość liter); pozycje, które masz w magazynie, są zielone.
- **Nowa / Edytuj**: symbol, producent, opis, obudowa, rodzina, adres datasheetu, uwagi i dowolne parametry (nazwa + wartość).
- **Import / Eksport / Szablon CSV**: kolumny Symbol, Producent, Opis, Obudowa, Rodzina, Datasheet, Uwagi, Parametry ("A=1; B=2" w cudzysłowie) – dodatkowe kolumny stają się parametrami.
- **Dodaj do magazynu**: tworzy element z danymi z katalogu (ilość, miejsce, kategoria).
- **Z katalogu...** w formularzu elementu wypełnia pola z wybranej pozycji.
- **Pobierz datasheet**: bez klucza pyta o adres pliku PDF. Z kluczem API Mouser (Ustawienia → Inne → „Klucz API Mouser”) wyszukuje część w Mouser Search API po symbolu albo numerze części, pokazuje listę trafień i pobiera datasheet wybranej pozycji, uzupełniając puste pola producenta, opisu i obudowy. Klucz: zaloguj się na konto My Mouser na mouser.com, wypełnij *Search API Request Form* (mouser.com/en/MyMouser/MouserSearchApplication.aspx) i poczekaj na mail z kluczem. Limity Mousera: 30 zapytań na minutę i 1000 na dobę. Klucz zapisuje się wyłącznie w lokalnym `config.ini` (nigdy w repozytorium). Mouser często blokuje automatyczne pobieranie PDF – wtedy program otwiera datasheet w przeglądarce i podaje ścieżkę, pod którą trzeba go zapisać, żeby został rozpoznany.
- **Menu podręczne** (prawy przycisk na liście): **Usuń z katalogu** (także dla wielu zaznaczonych pozycji; elementy w magazynie zostają) i **Wykonaj OCR datasheetu**. OCR dodaje warstwę tekstową do zeskanowanego, pobranego datasheetu (program ocrmypdf z Tesseractem, ustawienia jak w Literaturze), dzięki czemu można potem odczytać z niego parametry. Wynik zastępuje plik (oryginał zostaje jako `plik.pdf.bak`) albo trafia obok jako `plik_ocr.pdf`.
- **Odczytaj parametry z PDF**: wymaga programu `pdftotext` (poppler-utils; MSYS2: `pacman -S mingw-w64-ucrt-x86_64-poppler`). Program czyta tabele Absolute Maximum Ratings (przedrostek "Maks.:") i Electrical Characteristics ("Elektr.:"), w tym hFE, a opis zastosowania proponuje do pola Uwagi. Znane parametry mają polską nazwę i symbol, pozostałe wiersze są oznaczone "zgadywane" i domyślnie odznaczone. Propozycje zatwierdzasz; zapisują się ze źródłem "datasheet".


## Literatura (zakładka Literatura)
Biblioteczka książek o elektronice (papierowych i elektronicznych): tytuł, autor, wydawca, rok, wydanie, ISBN, język, tagi, uwagi i opcjonalna ścieżka do pliku PDF/EPUB (przycisk *Otwórz plik*; plik nie jest kopiowany).
- **Nowa / Edytuj / Usuń**: ręczne zarządzanie; usunięcie nie kasuje pliku na dysku.
- **Import / Eksport / Szablon CSV**: kolumny Tytuł, Autor, Wydawca, Rok, Wydanie, ISBN, Język, Tagi, Ścieżka, Uwagi. Książki, które już są (ta sama ścieżka albo tytuł i autor), są pomijane.
- **Dodaj z katalogu na dysku**: znajduje wszystkie PDF i EPUB w katalogu i podkatalogach, dane (tytuł, autorzy, wydawca, rok, ISBN, język, tagi, opis) czyta z metadanych pliku: XMP i słownika Info w PDF oraz OPF w EPUB; gdy ich brak lub tytuł jest śmieciowy ("Microsoft Word - plik.docx"), tytułem zostaje nazwa pliku. W oknie edycji książki przycisk *Odczytaj dane z pliku* uzupełnia puste pola.
- **Indeksuj treść / Indeksuj wszystkie**: tekst książki trafia do bazy. PDF wymaga `pdftotext` (pakiet poppler); EPUB czytany jest bezpośrednio (rozdziały w kolejności czytania, bez dodatkowych programów). Skany bez tekstu nie mają czego indeksować.
- **Pożycz / Zwrot**: ten sam mechanizm co w narzędziach (komu, od kiedy, termin). Przycisk zmienia się na *Zwrot*, gdy książka jest pożyczona; historia pożyczek jest w szczegółach, a przeterminowane pożyczki książek program wypisuje w dzienniku po uruchomieniu.
- **Szukaj w treści** (Enter): strony, na których są wszystkie wpisane słowa (bez rozróżniania wielkości liter i polskich znaków), z fragmentem tekstu. Dwuklik otwiera książkę na tej stronie (Linux: evince, okular, zathura, atril, xpdf, mupdf; własne polecenie w ustawieniach: klucz `/Ksiazki/PolecenieStrony`, `%f` = plik, `%p` = strona).

## Narzędzia i pożyczki (zakładka Narzędzia)
Baza narzędzi: nazwa, producent, model, kategoria, numer seryjny, miejsce, data i cena zakupu, instrukcja (plik lub adres), zdjęcie (podgląd w panelu po prawej), uwagi; import, eksport i szablon CSV (narzędzie z tą samą nazwą, modelem i numerem seryjnym jest pomijane).
- **Pożycz...**: komu, od kiedy, termin zwrotu (opcjonalnie) i uwagi; jedno narzędzie może być pożyczone tylko jednej osobie naraz.
- **Zwrócono**: zamyka pożyczkę z dzisiejszą datą; historia pożyczek narzędzia zostaje po prawej stronie.
- Pożyczone narzędzia są na liście niebieskie, po terminie pomarańczowe; filtry *tylko pożyczone* i *tylko po terminie*; szukanie znajduje też po nazwisku. Przeterminowane pożyczki są wypisywane w dzienniku przy starcie programu.

## Aktualizacje (menu Pomoc)

*Sprawdź aktualizacje* pobiera plik `wersja.txt` z głównego katalogu repozytorium na GitHubie (gałąź domyślna) i porównuje numer z wersją programu. Gdy jest nowsza, pokazuje numer, opis zmian i proponuje otwarcie strony pobierania; niczego nie instaluje. Opcja *Sprawdzaj aktualizacje przy uruchomieniu* (domyślnie włączona) sprawdza po cichu, najwyżej raz dziennie.

Wydanie nowej wersji: podnieś numer w `CMakeLists.txt` (`project(... VERSION ...)`), zbuduj instalatory, a po ich opublikowaniu zmień pierwszą linię `wersja.txt` (opcjonalnie: druga linia z adresem pobierania zaczynającym się od `http`, dalej opis zmian) i wyślij plik do repozytorium. Adres repozytorium zmienisz w *Ustawienia / Ścieżki / Aktualizacje* (domyślnie `https://github.com/amassalski/Magmass`).

## Kalkulatory: zasilacz, tranzystor, ESR

Menu `Narzędzia / Kalkulatory` (Ctrl+Shift+K): dzielnik napięcia, rezystor do LED, filtr RC, zasilacz z transformatora, tranzystor i tabela ESR. Każda zakładka ma schemat, na którym liczone elementy są wyróżnione kolorem i pogrubieniem (elementy zadane: zwykła kreska).

- **Zasilacz (trafo)**: z napięcia wtórnego, prądu i kondensatora liczy napięcie wyjściowe, tętnienia, zalecaną moc transformatora (VA) i prąd szczytowy diody. Dolna część liczy odwrotnie (jakie Usek i C dla zadanego wyjścia).
- **Tranzystor**: punkt pracy NPN z dzielnikiem bazowym (Ic, Ib, Vce, nasycenie lub zatkanie) oraz dobór R1, R2, RC, RE z szeregu.
- **ESR kondensatorów**: typowy ESR = tg δ / (2π f C) przy 120 Hz w zależności od pojemności i napięcia znamionowego (tg δ rośnie dla niskich i bardzo wysokich napięć); dla prądu tętnień moc strat I²·ESR. To rząd wielkości dla kondensatorów aluminiowych ogólnego przeznaczenia; dokładne wartości podaje karta katalogowa.
- **LM317 / LM337**: Uwy = Vref·(1 + R2/R1) + Iadj·R2 (LM337: napięcie ujemne). Pokazuje napięcie typowe i zakres z rozrzutu Vref (1,20–1,30 V) i Iadj (do 100 µA), prąd dzielnika, moc strat, minimalne napięcie wejściowe (|Uwy| + 3 V) i ostrzega przy zbyt małym prądzie obciążenia; dolna część dobiera R2 z szeregu dla zadanego napięcia. Vref, Iadj i prąd minimalny zależą od producenta – sprawdź kartę katalogową.
- **Cewki powietrzne: jednowarstwowa / wielowarstwowa** (menu Narzędzia > Elementy indukcyjne > Cewki powietrzne): indukcyjność jest sumą indukcyjności własnych i wzajemnych wszystkich zwojów (całki eliptyczne); wzór Wheelera pokazany obok do porównania. Pola opcjonalne: średnica z izolacją (puste = drut × 1,06) i skok zwojów (puste = zwoje obok siebie). Po wpisaniu docelowej indukcyjności program dobiera liczbę zwojów (jednowarstwowa) lub warstw (wielowarstwowa); podaje też długość i opór drutu oraz masę miedzi. Dla bardzo dużych cewek używany jest wzór Wheelera (program to zaznacza).

Transformator głośnikowy liczysz w `Narzędzia / Elementy indukcyjne` (SE, PP), a punkt pracy lampy w `Narzędzia audio i lampowe` (zakładka „Punkt pracy lampy”: polaryzacja katodowa, Rk, napięcie i moc anody, linia obciążenia, rezystor ekranu, wzmocnienie, kondensator katodowy).

Wyniki są przybliżone (reguły inżynierskie, stałe β i Vbe), więc traktuj je jako punkt startowy do dopracowania na stole.

## Kamera termowizyjna (wersja 2.5, w przygotowaniu)

**Tryb surowy i kalibracja.** Polecenie zoom 8004 (dla pełnej sekwencji: `8004 8020 8000`) przełącza InfiRay T3 Pro na surowe 14-bitowe odczyty z sensora (potwierdzone na macOS: kamera potwierdza wartość 0x8004). Producent nie publikuje wzoru na temperaturę, więc program nie podaje °C od razu. Skieruj środek obrazu na miejsce o znanej temperaturze (np. szklanka z wodą z lodem 0 °C, wrzątek; najlepiej czarna matowa powierzchnia, bo emisyjność błyszczących jest niska), wpisz temperaturę i naciśnij „Dodaj punkt kalibracji”. Po dwóch punktach program pokazuje temperatury oznaczone „(kalibracja użytkownika)”: przy dwóch punktach to prosta, przy większej liczbie łamana. Wynik jest orientacyjny, bo odczyt sensora zależy też od jego temperatury; po rozgrzaniu kamery kalibrację warto powtórzyć. Raport pokazuje też parametry zapisane przez kamerę w nagłówku klatki.

InfiRay T3 Pro zgłasza się jako klatka 384×292: 288 wierszy zwykłego obrazu 8-bitowego (jasność skalowana w każdej klatce, więc bez temperatur) i 4 wiersze nagłówka z parametrami. Program rozpoznaje ten układ i nie pokazuje wtedy temperatur. Przycisk „Wyślij polecenie (tryb surowy)” wysyła do kamery polecenie UVC „zoom” (domyślnie 8004; według forów kamery HT-301 i Xtherm przełączają tak tryb na surowe 16-bitowe odczyty). To funkcja eksperymentalna: po przełączeniu program pokazuje surowe wartości, bez przeliczenia na °C, bo wzór producenta nie jest publiczny.

Na macOS kamera jest odbierana bezpośrednio przez AVFoundation (plik `KameraMac.mm`), bez OpenCV, w surowym formacie YUYV; przy pierwszym uruchomieniu system pyta o zgodę na dostęp do kamery (można ją zmienić w Ustawieniach systemowych > Prywatność i ochrona > Kamera). Pole diagnostyczne pokazuje nazwę urządzenia, listę jego formatów i wybrany format piksela.

Menu `Kamera / Kamera termowizyjna` (Ctrl+Shift+T) obsługuje kamerę USB typu InfiRay T3 Pro: podgląd na żywo z paletą barw (Żelazo, Tęcza, Szara, Szara odwrócona), skala temperatur, krzyżyk z temperaturą środka kadru oraz znaczniki najgorętszego i najzimniejszego punktu. Obraz można zamrozić i zapisać jako PNG.

Kamera jest otwierana bez konwersji do RGB (`CAP_PROP_CONVERT_RGB=0`), żeby dostać surowe dane. Producent nie publikuje protokołu temperatur, więc przyjęto **hipotezę** opartą na opisach społeczności dla tej rodziny kamer: ramka 256×384 (2 bajty na piksel) to obraz 256×192 u góry i 16-bitowe dane radiometryczne 256×192 poniżej, a T[°C] = surowe / 64 − 273,15. Dzielnik i przesunięcie są polami w oknie. **Sprawdź je pomiarem** (woda z lodem 0 °C, dłoń ok. 33 °C, element o znanej temperaturze). Jeśli kamera przesyła dane inaczej, okno pokaże sam obraz z komunikatem „brak temperatur”.

Dolny panel to raport diagnostyczny (backend, rozmiar i typ klatki, pierwsze bajty, zakres wartości). Przycisk „Zapisz próbkę” zapisuje surową klatkę (`.bin`) i raport (`.txt`); na ich podstawie można ustalić prawdziwy format danych. Uwaga: zależnie od systemu i backendu OpenCV (na macOS AVFoundation) kamera może oddawać już przekonwertowany obraz bez danych temperatury; raport to pokaże.

## Język interfejsu
*Ustawienia / Inne*: język programu (Według systemu, Polski, English). Zmiana działa po ponownym uruchomieniu. Tłumaczenia są w `lang/*.po`; po zmianie napisów w kodzie odśwież je skryptem `create_translations.sh`.

## Zaznaczanie wielu pozycji i edycja zbiorcza

W listach Magazyn, Projekt (pozycje), Katalog i Literatura można zaznaczyć kilka wierszy: Ctrl+klik, Shift+klik (zakres) lub Ctrl+A. Klawisz Delete albo przycisk Usuń usuwa wszystkie zaznaczone. Przycisk Edytuj przy kilku zaznaczonych otwiera okno zmiany zbiorczej: w Magazynie można zmienić kategorię, producenta, tolerancję, ilość, stan minimalny, cenę i tagi (tagi dopisywane albo zastępowane); w Literaturze autora, wydawcę i tagi (pola indywidualne są wyszarzone).

## Katalog: parametry z indeksami i grupy wzmocnienia

W wartościach i nazwach parametrów: `V_CE` (indeks dolny), `V_{CE(sat)}`, `m^2`, `10^-3` (indeks górny); `\_` wpisuje dosłowny podkreślnik. Grupy hFE tranzystorów zapisz jako jeden parametr przy typie bazowym, np. BC547: `hFE = A: 110-220; B: 200-450; C: 420-800`. Elementy BC547A, BC547B, BC547C w magazynie są dopasowywane do tej pozycji.

## Literatura: Biblioteka Narodowa i uwagi HTML

W formularzu książki przycisk „Pobierz z Biblioteki Narodowej” szuka po ISBN (albo tytule i autorze) i wypełnia pola. Dwuklik w polu Uwagi otwiera edytor HTML (u góry podgląd, u dołu źródło).

## Literatura: zapis danych do PDF i OCR

Oba polecenia działają na zaznaczonych książkach, które mają istniejący plik PDF, i wymagają programów zewnętrznych (nie są dołączone do aplikacji).

**Zapisz dane do PDF** – zapisuje dane z bazy (tytuł, autor, wydawca, rok, ISBN, język, tagi, uwagi) w metadanych pliku PDF (pola PDF i XMP) za pomocą programu **ExifTool** (exiftool.org). Zapisywane są tylko pola niepuste. Obok pliku zostaje kopia oryginału `plik.pdf_original`; można to wyłączyć kluczem `exiftool_kopia = 0` w sekcji `[literatura]` pliku config.ini. Formatów innych niż PDF (np. EPUB) ta funkcja nie obsługuje.

**OCR...** – rozpoznaje tekst w zeskanowanych PDF-ach programem **ocrmypdf** (wymaga Tesseracta z danymi języka polskiego i angielskiego). Strony, które już mają tekst, są pomijane. Do wyboru: „Zapisz obok” (powstaje `plik_ocr.pdf`) albo „Zastąp oryginał” (oryginał zostaje jako `plik.pdf.bak`, a indeks treści jest budowany od nowa). OCR dużej książki trwa kilka minut lub dłużej; można go przerwać.

**Ustawienia** (zakładka Inne): „Program ExifTool”, „Program ocrmypdf” (puste = szukanie w PATH; można wpisać pełną ścieżkę) oraz „Języki OCR” (kody Tesseracta, domyślnie `pol+eng`).

## Literatura: menu podręczne, wydanie i uwagi

Prawy przycisk myszy na liście książek otwiera menu: **Usuń książkę** (albo zaznaczone książki), **Usuń indeks** (kasuje tylko indeks treści używany do wyszukiwania w treści; książka i plik zostają), **Zapisz metadane** (dane z bazy do pliku PDF, ExifTool), **Wykonaj OCR** i **Info** – okno z kartą książki, w którym uwagi są pokazane jako sformatowany HTML.

W formularzu książki jest pole **Wydanie** (np. „pierwsze”, „drugie poprawione”, „3rd edition”). Pobranie z Biblioteki Narodowej uzupełnia je z rekordu MARC (pole 250), a *Zapisz dane do PDF* zapisuje je w pliku (XMP prism:Edition). W plikach CSV ma własną kolumnę „Wydanie”.

Pole **Uwagi** w formularzu pokazuje tekst jako HTML (pogrubienia, listy, tabele). Dwuklik w polu otwiera edytor, w którym można zmieniać źródło tekstu.

## Przeglądarka PDF i ścieżki programów

**Ustawienia → Inne → Polecenie czytnika PDF** mówi, jakim programem otwierać pliki PDF, także na konkretnej stronie (po wyszukiwaniu w treści). Polecenie jest zwykłym wierszem poleceń z symbolami:

- `%f` – pełna ścieżka pliku,
- `%p` – numer strony (przy zwykłym otwarciu pliku: 1),
- `%u` – ścieżka zakodowana jako adres URL (spacje jako `%20`, polskie litery jako `%C4%85` itd.); do adresów typu `skim://` i `file://`.

Cudzysłowy ustawiaj wokół `%f` i `%u` (ścieżki mogą mieć spacje), nigdy wokół `%p`. Puste pole oznacza: aplikacja szuka znanego czytnika (evince, okular, zathura…), a gdy go nie ma, używa programu domyślnego, który jednak nie przechodzi na wskazaną stronę.

| System | Program | Polecenie |
|---|---|---|
| macOS | Skim | `open "skim://%u#page=%p"` |
| macOS | Brave / Chrome | `osascript -e 'tell application "Brave Browser" to open location "file://%u#page=%p"'` (dla Chrome zamień nazwę na `Google Chrome`; polecenie `open -a ... "file://...#page=N"` gubi numer strony, dlatego użyto osascript) |
| Windows | Adobe Reader | `"C:\Program Files\Adobe\Acrobat Reader\Reader\AcroRd32.exe" /A page=%p "%f"` |
| Windows | SumatraPDF | `"C:\Program Files\SumatraPDF\SumatraPDF.exe" -page %p "%f"` |
| Linux | Okular / Evince | `okular --page %p "%f"` / `evince -p %p "%f"` |

Skim trzeba zainstalować (skim-app.sourceforge.io) i raz uruchomić. Zapis `skim:/"%f"#page="%p"` nie działa: cudzysłowy trafiają do adresu, a spacje w ścieżce muszą być zakodowane – dlatego służy do tego `%u`.

**Ścieżki programów na macOS.** Pola „Program ExifTool” i „Program ocrmypdf” (Ustawienia → Inne) przyjmują pełną ścieżkę. Po instalacji przez Homebrew (`brew install exiftool ocrmypdf tesseract tesseract-lang`) są to na komputerach z Apple Silicon `/opt/homebrew/bin/exiftool` i `/opt/homebrew/bin/ocrmypdf`, na Intelu `/usr/local/bin/...`; sprawdzisz to w Terminalu poleceniem `which exiftool`. Aplikacja uruchomiona z Findera nie dostaje PATH z Terminala, więc sama dopisuje katalogi Homebrew (`/opt/homebrew/bin`, `/usr/local/bin`, `/opt/local/bin`) do PATH – zwykle pola można zostawić puste. Te katalogi dotyczą także programów, które ocrmypdf uruchamia sam (Tesseract, Ghostscript). Na Windows wpisz np. `C:\exiftool\exiftool.exe`.

## Sortowanie list

W zakładkach Magazyn, Projekty (lista projektów i lista części), Literatura i Katalog kliknięcie nagłówka kolumny sortuje listę po tej kolumnie (strzałka ▲ rosnąco). Ponowne kliknięcie odwraca kierunek (▼). Teksty są porównywane bez rozróżniania wielkości liter, z polskimi literami i liczbami „naturalnie” (R2 przed R10); kolumny liczbowe sortują się numerycznie. Zaznaczenie pozostaje na tej samej pozycji.

## Drukarka Zebra (ZPL): etykiety na rolce

W oknie etykiet (`Magazyn / Drukuj etykiety`) pole *Sposób drukowania* przełącza między drukarką systemową a drukarką Zebra (np. ZD421). Zebra dostaje polecenia ZPL, a tekst i kod kreskowy rysuje sama drukarka. Istniejące drukowanie arkuszy przez system zostaje bez zmian.

1. **Ustawienia Zebry...**: wybierz połączenie i rozmiar etykiety.
   - *Sieć*: adres IP drukarki i port 9100 (ZD421 z Ethernetem; adres pokaże wydruk konfiguracji sieci po przytrzymaniu *Feed*).
   - *USB / drukarka w systemie*: nazwa drukarki z listy (Windows: sterownik ZDesigner; macOS i Linux: kolejka CUPS) albo `/dev/usb/lp0` (Linux). Dane idą jako RAW, bez sterownika graficznego.
   - *Tylko zapis do pliku .zpl*: do sprawdzenia poleceń albo wysłania innym programem.
2. Rozmiar etykiety (np. 80 x 50 mm) i rozdzielczość (203 dpi w ZD421) ustaw zgodnie z rolką. Drukarkę skalibruj raz na dane etykiety.
3. **Etykieta próbna** drukuje ramkę z wymiarami i przykładowym kodem, żeby sprawdzić położenie, obrót i czcionkę.
4. Polskie litery, µ i Ω są domyślnie zamieniane na ASCII, bo czcionka wbudowana w drukarkę może ich nie mieć. Po sprawdzeniu etykiety próbnej opcję można wyłączyć.
5. **Drukuj na Zebrze** wysyła wybrane etykiety (kolejne identyczne łączone w jedno polecenie z liczbą kopii); **Zapisz ZPL do pliku...** zapisuje te same polecenia.

Czytnik Zebra CS6080 (z stacją dokującą) w trybie klawiatury (HID) wpisuje kod jak tekst, więc skan etykiety w polu szukania odnajduje element bez dodatkowej konfiguracji.

## Dławik zasilacza

`Narzędzia / Dławik zasilacza` (Ctrl+Shift+D). Wybierz kształtkę EI (np. EI102) i grubość pakietu albo wpisz własne wymiary rdzenia (A, B, a, b, c, d, h w mm) oraz grubość blachy; podaj indukcyjność [H], prąd stały [mA], przenikalność żelaza [Gs/Oe], dopuszczalną indukcję od prądu stałego i gęstość prądu. Program liczy zwoje N = L·I / (B·Qfe), szczelinę z reluktancji (szczelina = μ0·N·I/B − lFe/μ), średnicę drutu z gęstości prądu, opór, spadek napięcia i wypełnienie okna. Układ okna jak w ECCLab: rysunek rdzenia obok wymiarów, ramki Parametry, Wyniki obliczeń, Rdzeń i Uzwojenie, suwak średnicy drutu i przycisk Pomoc. Przycisk Oblicz pokazuje ostrzeżenie, gdy uzwojenie zajmuje ponad 75% okna (nie zmieści się) albo mniej niż 50% (rdzeń nie jest optymalnie wypełniony). Wymiary rdzenia, oporność i liczby zwojów na cm² dopasowano do przykładu ECCLab (EI102, pakiet 51 mm); „Zalecana objętość rdzenia” to wzór [35] z książki S. N. Krize (1953): L·I0²·10⁻³/2 (L w H, I0 w mA), zgodna z ECCLab. Szczelina jest liczona fizycznie (całkowita reluktancja minus reluktancja drogi w żelazie; przekładka pod kolumny to połowa); wzór [38] Krize'a i ECCLab, z·I0/8·10⁻⁵ mm, który reluktancji żelaza nie odejmuje, pokazany jest obok jako porównanie. Gdy sama droga w żelazie przekracza wymaganą reluktancję (mała przenikalność), szczelina wynosi 0, a „Indukcyjność uzyskana” jest mniejsza od zadanej — wtedy trzeba większego przekroju żelaza albo więcej zwojów.

### Zestawienie drutu (waga)

W oknach dławika, transformatora sieciowego oraz SE i PP (wykonane) pole „Zestawienie drutu (waga)” podaje dla każdej średnicy: długość drutu, masę miedzi (π/4·d²·długość·8,96 g/cm³) i masę do zakupu. Długość to zwoje × średnia długość zwoju (w transformatorach z tabeli sekcji, osobno dla każdego uzwojenia). Masa do zakupu = miedź + ok. 3% lakieru + 10% zapasu na odpady, początek i koniec uzwojenia i wyprowadzenia; oba dodatki są orientacyjne. Uzwojenia o tej samej średnicy są sumowane.

## Transformator sieciowy

`Narzędzia / Elementy indukcyjne / Transformator sieciowy`. Układ okna jak w dławiku (rdzeń EI z rysunkiem). Parametry: napięcie zasilania (230 V) i uzwojenia wtórne wpisane w jednym polu, po jednym w wierszu: napięcie [V] i prąd [A]. Program liczy wg Krize'a moc pierwotną, prąd pierwotny, wymagany iloczyn q_o·q_z, zwoje (wzory [15] i [17]) i normalne średnice drutu, a w tabeli liczbę warstw i grubość każdego uzwojenia oraz zapas δ w oknie. Przycisk „Wartości typowe wg mocy” podstawia B, η i j z tablicy 5. Wzory sprawdzono na przykładzie z książki (688 / 4232 / 38 / 30 zwojów).

## Transformator głośnikowy SE

`Narzędzia / Elementy indukcyjne / Transformator głośnikowy SE`. Parametry: moc, Ra, Ri, prąd anodowy, najniższa częstotliwość, opór głośnika, sprawność i rdzeń (kształtka EI albo przekrój, droga magnetyczna i wymiary okna wpisane ręcznie).

**Formularz główny** podaje indukcyjność pierwotną Li = (Ra∥Ri)/(2π·Fd), przekładnię n = √(η·Ra/Rg), zwoje i średnice drutu oraz szczelinę. Zwoje wynikają z warunku, że indukcja szczytowa (składowa stała + zmienna) równa się zadanej (domyślnie 1 T): z = Li·Iszczyt/(B·S). Szczelina jest liczona z reluktancji obwodu z uwzględnieniem żelaza: lg = μ0·S·z²/Li − lFe/μ. Gdy żądana moc przekracza możliwości lampy w klasie A (Ia²·Ra/2·η), pojawia się ostrzeżenie. Podmagnesowanie z1·Ia/l w A·zw./cm powyżej 2 oznacza wg Cykina (rozdz. XI.3), że rdzeń musi mieć szczelinę.

**Przekładnia.** Rezystancje uzwojeń są szeregowo z obciążeniem, więc Ra widziane z lampy to n²·Rg/η, a stąd n = √(η·Ra/Rg) (20,62 dla danych z przykładu). Program ze zrzutu pokazuje √(Ra/(η·Rg)) = 24,25, czyli o czynnik 1/η więcej. Obie wartości są w formularzu, druga jako porównanie. Indukcyjność z tamtego programu to (Ra∥Ri)/(7,2·Fd), o ok. 13% mniej niż (Ra∥Ri)/(2π·Fd), czyli spadek ok. 3,6 dB zamiast 3 dB przy Fd.

**Porównanie z modułem Cykina.** W ramce są wyniki funkcji `calcSETransformer` z modułu tube_magnetics dla tych samych danych: zwoje, szczelina, indukcyjność, indukcja szczytowa i sprawność. Moduł przyjmuje stałą indukcję Bdc = 0,7 T, Lp = Ra/(2π·Fd) bez Ri, szczelinę μ0·N·I/Bdc bez reluktancji żelaza i η = 0,88, więc różni się od modelu fizycznego. Sam Cykin w książce z 1953 r. dobiera przenikalność zastępczą i optymalną szczelinę z wykresów (rys. XV.25 i XV.26), których nie da się wiarygodnie zapisać wzorem.

**Porównanie z literaturą.** W trzeciej kolumnie obok wyników fizycznych są wartości liczone wzorami z literatury dla tych samych danych:

- Krize (Transformatory wyjściowe, transformator z podmagnesowaniem): iloczyn okno × rdzeń Q0·Qs = A·P (A = 20 pentoda bez sprzężenia zwrotnego, 10 pentoda ze sprzężeniem albo trioda bez, 5 trioda ze sprzężeniem; wybór „Lampa i układ"), zwoje wg (23) i (24), w2 = n·w1, szczelina l3 = w1·I0/8·10⁻⁵ mm (I0 w mA), drut wg (26) i (27).
- Witort (Amatorskie wzmacniacze elektroakustyczne, 1972): Q = (3÷5)·√P, n1 = 9000·√(L1·lr/(500·Q)), L1 ≥ Ra/(6,28·fd), przekładnia p = √(η·Ra/Rg).
- Moduł tube_magnetics (opisany jako oparty na Cykinie).

Szczelina wg Krizego jest równa μ0·N·I/B dla B = 1 T z samej składowej stałej i pomija żelazo oraz składową zmienną. Wzory Krizego (przekładnia √(Rg/Ra)) pomijają sprawność, a Witort ma n = √(η·Ra/Rg) jak Magmass. Program ze zrzutu liczy szczelinę dokładnie wzorem Krizego (28) z całkowitą liczbą zwojów (1526·85/800000 = 0,1621375 mm), ale zwoje pierwotne (1526,9) nie wynikają z (23) ani (24) dla jego danych.

**Dalej: uzwojenia wykonane.** Wpisujesz całkowite zwoje, normalne średnice drutu i szczelinę, wybierasz układ (nieprzeplatane, P/2-S-P/2 albo S/2-P-S/2) i dostajesz: indukcyjność i dolną częstotliwość wykonanego transformatora, indukcje, rozmieszczenie w warstwach (z gwiazdką przy nieparzystej liczbie warstw), rezystancje, spadek napięcia stałego i stratę mocy od prądu stałego, rozproszenie dla wszystkich trzech układów, górną częstotliwość -3 dB i sprawność. Rozproszenie liczone jest z energii pola w oknie, a wzór z książki jest pokazany obok jako porównanie.

W formularzach SE i PP rysunek rdzenia EI z wymiarami A, B, a, b, c, d, h pokazuje wybraną kształtkę.

### Spadek na krańcach pasma (1, 2 lub 3 dB)

W formularzach SE i PP pole „Spadek na krańcach pasma” (1, 2 lub 3 dB) określa, gdzie leżą krańce pasma. Wymagana indukcyjność pierwotna to Li = (Ra∥Ri)/(2π·fd·k), a dopuszczalne rozproszenie Lr = k·(Ra+Ri)/(2π·fg), gdzie k = √(M²−1), a M to spadek amplitudy. Dla 3 dB (punkt połowy mocy, M = √2) k = 1, dla 2 dB k = 0,765, dla 1 dB k = 0,508 – mniejszy spadek wymaga większej indukcyjności i mniejszego rozproszenia. Ten sam spadek stosuje analiza dokładna przy wyznaczaniu krańców pasma i na wykresie. Metoda gitarowa ze strony używa stałego Mn = 1,25 (ok. 1,9 dB) i tak pozostaje w bloku porównawczym.

## Analiza dokładna transformatorów głośnikowych (SE i PP)

W oknach „uzwojenia wykonane” (SE i PP Etap 3) na dole jest panel **Analiza dokładna**. Liczy ją jeden wspólny moduł (ten sam dla SE i PP), bez uproszczeń „stała μ” i „−3 dB z Rp/ω”:

- schemat zastępczy: źródło (Ri) – Rp – węzeł, w którym równolegle są pojemność własna Cd, indukcyjność Lp, straty w żelazie Rc(f) oraz gałąź rozproszenie Lr + rezystancja wtórna + obciążenie; z pełnej transmitancji zespolonej program wyznacza krańce pasma (spadek dowolny, domyślnie 3 dB), rezonans i rysuje wykres;
- Lp = z1²/(l_Fe/(μ0·μprzyr·S) + l_g/(μ0·A_g)); punkt pracy z równania amperozwojów z1·I = H(B)·l_Fe + B·S·l_g/(μ0·A_g) i krzywej magnesowania Fröhlicha–Kennelly'ego (μprzyr = μi·(1−B/Bs)²); w PP polaryzacja wynika z różnicy prądów połówek (domyślnie 5 mA);
- indukcja zmienna liczona z napięcia w węźle przy dolnej częstotliwości i pełnej mocy (nie z uproszczonego Um1);
- pojemność własna z energii ½CV² (układ warstw „tam i z powrotem”: 4/3·(N−1)/N² · C_warstw, sekcje niosą tylko swoją część napięcia), plus pojemność pierwotne–wtórne;
- straty w żelazie P = pFe·m·(f/50)^1,5·B² jako rezystancja równoległa.

Ograniczenia modelu (świadomie): krzywa magnesowania jest dwuparametrowa (μi, Bs), więc dla pewnego wyniku trzeba ustawić μi na podstawie pomiaru indukcyjności własnego rdzenia; rozpływ pola w szczelinie jest uwzględniony tylko gdy znane są wymiary kolumny środkowej; wagi pojemności pierwotne–wtórne zależą od układu uzwojeń (1 – nieprzeplatane SE, 0,5 – P/2-S-P/2). Wzory z literatury (Krize, Witort, Cykin, strona gitarowa, ECCLab) pozostają w oknach jako porównanie.

## Cewka na rdzeniu z AL

`Narzędzia / Elementy indukcyjne / Cewka na rdzeniu z AL`. Podajesz AL rdzenia (nH/zw², z katalogu) i żądaną indukcyjność (µH, mH lub H), a program liczy liczbę zwojów N = √(L/AL) – wprost z definicji AL – oraz indukcyjność dla zaokrąglonej liczby zwojów. Opcjonalnie: tolerancja AL (zakres zwojów dla skrajnych AL), liczba zwojów do sprawdzenia (np. z innego programu – pokazuje indukcyjność i odchyłkę) oraz Ae i prąd szczytowy (indukcja B = L·I/(N·Ae) z ostrzeżeniem). Przykład: AL = 7000, L = 770 mH → 331,66, czyli 332 zwoje (771,6 mH). 320 zwojów daje 716,8 mH (−6,9%), więc inny program musi przyjmować inne AL albo inną definicję. AL dotyczy małego sygnału bez prądu stałego; przy polaryzacji maleje.

## Transformator głośnikowy PP: tabela sekcji uzwojeń (Etap 3)

W Etapie 3 układ uzwojeń zadaje się tabelą sekcji od rdzenia na zewnątrz, po jednym wierszu na sekcję: `A1 600`, `W1 100`, `A2 300` ... (A = pierwotne/anoda, W = wtórne). Wiersz `|` oznacza przegrodę karkasu i początek następnej komory (HiFi: karkas dzielony na pół, w każdej komorze połowa uzwojeń). Suma A to zwoje pierwotne, suma W wtórne; zmiana jednych aktualizuje drugie. Gotowe szablony (P-S, P/2-S-P/2, gitarowy A 40% / W 50% / A 20% / W 50% / A 40%, HiFi dwukomorowy i dwukomorowy przeplatany) wypełniają tabelę, którą można dowolnie edytować. Sekcje wtórne można połączyć szeregowo albo równolegle (wtedy każda ma tyle zwojów, ile całe wtórne), a przegroda zabiera wysokość okna.

Bardziej złożone połączenia opisują dopiski za liczbą zwojów: `g<N>` przy sekcji W oznacza grupę N (sekcje jednej grupy są połączone równolegle i muszą mieć tyle samo zwojów, grupy są połączone szeregowo w kolejności numerów, a suma zwojów grup daje wtórne), `x` oznacza sekcję nieczynną (leży w oknie, ale nie przewodzi), a `e<N>` przy sekcji A podaje kolejność elektryczną pierwotnych, gdy różni się od kolejności w oknie. Transformator z odczepami zapisuje się jako `g<a>/<b>/<c>`: numer grupy tej sekcji dla odczepu 4, 8 i 16 Ω (albo `-`, gdy sekcja przy tym odczepie nie przewodzi). Na tym opiera się szablon „Mesa Boogie DR”: jedno trafo z dziesięcioma warstwami wg Rys.15 ze strony guitaramplifiers.prv.pl i odczepami 4, 8 i 16 Ω jednocześnie, np. `W1 78 g1/1/1`, `W2 33 g-/2/-`, `W4 78 g-/-/2`. Odczep 4 Ω to warstwy 2∥5∥8, dla 8 Ω dochodzą szeregowo warstwy 3∥9, a dla 16 Ω warstwa 6. W polu „Odczep wtórnego (DR)” wybierasz odczep, dla którego liczone są wyniki, a pod tabelą sekcji program podaje zestawienie wszystkich trzech odczepów (każdy dla własnego obciążenia). Pole z2 oznacza wtedy zwoje odczepu 4 Ω. Każda warstwa wtórna oryginału jest nawinięta dwoma drutami równolegle; program liczy jeden drut o średnicy z pola d2, więc liczba zwojów na warstwę i rezystancja są dla DR przybliżone.

Obliczenia dla tabeli: warstwy i grubość każdej sekcji w jej komorze (wysokość komory = (h − przegrody)/liczba komór); rozproszenie jako suma energii pola w komorach, z siłomotoryczną wynikającą z liczby zwojów każdej sekcji; sprawdzenie równowagi amperozwojów w komorach (ostrzeżenie przy rozbieżności powyżej 3%); pojemność własna z pojemności międzywarstwowych oraz z pojemności między sąsiednimi sekcjami A i W liczonych z rzeczywistych potencjałów warstw (środek uzwojenia pierwotnego na masie, jeden koniec wtórnego na masie; przy nieznanym kierunku nawijania brany jest gorszy przypadek). Zakładamy, że sekcje A są połączone szeregowo w kolejności tabeli, a rozproszenie liczone jest dla prądu zastępczego całego uzwojenia (nie dla pracy jednej połowy w klasie B).

## Transformator głośnikowy PP

`Narzędzia / Elementy indukcyjne / Transformator głośnikowy PP`. Etap 1: wymagania (R0, Raa, moc, Fd, Fg, Ri, I0, przeznaczenie Hi-Fi lub gitarowy) dają pożądane Li, Lr, przekładnię i przekrój rdzenia. Etap 2: dla zdobytego rdzenia (kształtka EI lub wymiary wpisane ręcznie) program podaje zwoje i średnice drutu uzwojenia pierwotnego i wtórnego. Wzory Etapów 1 i 2 zgodne ze zrzutami programu Laboratorium2005. Przycisk „Dalej” otwiera Etap 3: wpisujesz całkowite zwoje i normalne średnice drutu, wybierasz układ (nieprzeplatane albo pierwotne w dwóch częściach wokół wtórnego) i dostajesz rozmieszczenie w warstwach (z ostrzeżeniem o nieparzystej liczbie warstw), rezystancje, rozproszenie i sprawność. Rozproszenie i długość zwoju liczone są z geometrii i energii pola, a wyniki wg książki i ECCLab pokazane są obok jako porównanie (różnią się, bo przyjmują grubość uzwojeń równą szerokości okna).

W trzeciej kolumnie okna zielone wartości to porównanie z metodą uproszczoną ze strony guitaramplifiers.prv.pl (L1 z Mn = 1,25 i czynnikiem 1+Ri/Raa, objętość rdzenia z Um1 i indukcji, zwoje z L1, prąd I1 = √(I0² + 0,25·Imax²)). Wzory sprawdzono niezależnie, w przykładzie ze strony (EL34, 55 W, Raa 3400 Ω, R2 = 4·1,25 Ω) wychodzi L1 = 12,96 H, Vż = 287 cm³, z1 ≈ 1785, I1 = 95 mA, d1 = 0,246 mm, z2 = 72. Przekładnia z1/z2 = √(η·Raa/R2) zgadza się z naszym modelem. Dla głośnika gitarowego wpisz R2 z mnożnikiem 1,25 (impedancja dla prądu zmiennego).

## Zniekształcenia harmoniczne

`Narzędzia / Zniekształcenia harmoniczne` (Ctrl+Shift+Z). Metoda pięciu rzędnych (Klina): z prostej obciążenia odczytujesz prądy Ia max, Ia1, Ia0, Ia2, Ia min dla wysterowania +Um, +0,5 Um, 0, -0,5 Um, -Um (prądy z okna Metody graficzne). Program wyznacza składową stałą, amplitudy harmonicznych i współczynniki zawartości h2, h3, h4 oraz h (suma geometryczna) w procentach i dB. Tryb „odcinków” przyjmuje długości odcinków a..g zmierzone na wykresie (a = d+e, b = f+g, c = e+f). Przykład z formularza ECCLab (93; 67,5; 41; 20; 7 mA) daje h2 = 10,11%, h3 = 3,37%, h4 = 0,75%.

## Metody graficzne

`Narzędzia / Metody graficzne` (Ctrl+Shift+G). **Wczytaj obraz** z charakterystyką anodową lampy (np. skan karty katalogowej). W trybie **Skalowanie charakterystyk** wybierz krok i kliknij na obrazie: punkt (0,0) (przecięcie osi), punkt na osi napięcia i punkt na osi prądu; wpisz napięcie i prąd tych dwóch punktów i naciśnij **Skaluj**. Program liczy skalę osobno dla obu osi (w ECCLab służy do tego linia pod kątem 45° i osobne skalowanie mocy; tu wystarczą trzy punkty). W trybie **Wyniki i wyznaczanie Ra** przeciągasz myszą czerwony koniec (prąd) i zielony koniec (napięcie) prostej obciążenia; program podaje ΔUa, ΔIa, Ra klasy A (ΔUa/ΔIa), Raa klasy B (4 Ra), moc SE klasy A (ΔUa ΔIa / 8) i moc PP klasy B (4 razy moc SE). Koniec napięciowy nie może wejść na lewo od prądowego; gdy koniec prądowy jest na osi, napięcie to 0 V. Blokada Ra utrzymuje nachylenie prostej, blokada amplitudy różnicę prądów. **Zniekształcenia**: dwuklik na prostej w miejscu przecięcia z krzywymi napięcia siatki ustawia kolejno punkty Ia1, Ia0, Ia2 (skrajne prądy to końce prostej), a poniżej pokazane są h, h2, h3, h4; osobne okno Zniekształcenia przyjmuje te same dane wpisane ręcznie. **Siatka kontrolna** rysuje linie co zadany prąd i napięcie, a **Pa max** niebieską krzywą mocy admisyjnej anody: jeśli pokrywa się z krzywą producenta, skalowanie jest poprawne.

## Narzędzia audio i lampowe (ECCkalkulator)

`Narzędzia / Narzędzia audio i lampowe` (Ctrl+Shift+L). Zakładka **Układy lampowe** liczy cztery układy małosygnałowe (rezystancje w kΩ, pojemności w pF, Cs w nF, Sa w mA/V): sprzężenie RC (wzmocnienie, oporność wyjściowa, fd, fg), wtórnik anodowy (Rwy, wzmocnienie pod obciążeniem Ro), wtórnik katodowy (Rwe, Rwy = 1/Sa, wzmocnienie, fg, maksymalne napięcie wejściowe) i inwerter różnicowy (q, wzmocnienia obu triod) oraz układy złożone: mi-wtórnik, SRPP, wtórnik White'a, kaskodowy wtórnik katodowy i superwtórnik (wzory z modułu `tube_audio`; nie były porównywane z ECCLab, dla R0 = 0 w mi-wtórniku i SRPP obciążenie pomijane). Wyniki zgadzają się z przykładami z ECCLab 7.2 co do drukowanych cyfr z dwiema różnicami: górna częstotliwość sprzężenia RC przy polaryzacji automatycznej jest przybliżona (ECCLab daje wartość niższą), a górna częstotliwość wtórnika katodowego program podaje w MHz (ECCLab opisuje tę samą liczbę jako kHz; przy Csk w pF jednostką fizyczną są MHz). Obok pól pokazywany jest schemat wybranego układu; elementy biorące udział w obliczeniach (te, których wartości wpisujesz) są wyróżnione kolorem i pogrubieniem, pozostałe są szare. Schematy mi-wtórnika, SRPP, wtórnika White'a, kaskody i superwtórnika są poglądowe. Dane lampy (Sa, Ka, Rwew, pojemności) można wstawić z katalogu przyciskiem **Lampa z katalogu** albo zapisać do niego przyciskiem **Zapisz lampę do katalogu**; lampy są zwykłymi pozycjami katalogu w rodzinie „Lampy elektronowe” z parametrami `Sa [mA/V]`, `Ka [V/V]`, `Rwew [kΩ]`, `Cak/Cas/Csk [pF]`, więc działają na nich okno Katalog oraz import i eksport CSV. Przy pustym katalogu program proponuje lampy przykładowe (ECC81, ECC82, ECC83, ECC85, ECC88). Zakładka **dB i nepery** przelicza napięcia, moce i miarę liniową na decybele i nepery oraz odwrotnie (wyniki na bieżąco). Zakładka **Krzywa RIAA** pokazuje tłumienie lub wzmocnienie korektora gramofonowego (odtwarzanie) albo krzywej tłoczenia (nagrywanie) względem 1 kHz dla wpisanej częstotliwości, tabelę z wybranego zakresu (od, do, co ile Hz) i wykres 20 Hz..20 kHz; opcja IEC dodaje filtr 20 Hz. Po wpisaniu R2 i R3 program liczy pozostałe elementy biernej sieci korekcyjnej (sekcja A: 3180/318 µs, sekcja B: 75 µs, rozdzielone stopniem wzmacniającym), zaokrągla je do wybranych szeregów E i podaje największą odchyłkę od krzywej RIAA oraz tłumienie przy 1 kHz (ok. 40 dB).


## Zmiany etapu 3z72

- **Zasilacz (kalkulator):** schemat zmienia się razem z rodzajem prostownika.
- **Punkt pracy lampy:** przycisk „Lampa z katalogu…” wstawia μ, ra i Pa max.
- **Dławik:** szczelina jest niepotrzebna, gdy wymagana szczelina wychodzi mniejsza niż ok. 0,05 mm (rdzeń z naturalną szczeliną styku blach, tzw. prąd mały zależy od L, B i przekroju – program wyświetla konkretną liczbę).
- **Dymki i pomoc:** najechanie na pole pokazuje opis wyłącznie tego pola; każde pole ma własną definicję w pliku pomocy.
- **Słownik obudów:** zaznaczanie wielu pozycji, usuwanie i edycja zbiorcza.
