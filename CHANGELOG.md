# Historia zmian

## 2.0 (w przygotowaniu)

Wersje robocze przed 2.0 są numerowane 1.9.N (kolejny numer N przy każdej paczce poprawek); numer bierze się z `CMakeLists.txt` i widać go w oknie „O programie”. Bieżąca wersja robocza: 1.9.9.

- Transformator głośnikowy PP, Etap 3: nowy układ uzwojeń „Mesa Boogie DR” (jedno trafo z dziesięcioma warstwami wg Rys.15 ze strony guitaramplifiers.prv.pl i odczepami 4, 8 i 16 Ω jednocześnie; z2 = zwoje odczepu 4 Ω). Wybór odczepu, dla którego liczone są wyniki, oraz zestawienie wszystkich trzech odczepów pod tabelą sekcji. Tabela sekcji dostała dopiski `g<N>` (grupy sekcji wtórnych połączonych równolegle, grupy szeregowo), `g<a>/<b>/<c>` (grupy dla odczepów 4/8/16 Ω), `x` (sekcja nieczynna) i `e<N>` (kolejność elektryczna sekcji pierwotnych); rozproszenie, rezystancje i pojemność własna uwzględniają je w modelu.
- Katalog: odczyt parametrów z datasheetu współpracuje ze słownikiem parametrów. W oknie propozycji przy każdym parametrze widać, czy symbol jest w słowniku (z opisem) czy nie; po zapisie program pyta o opisy symboli, których jeszcze nie zna, i dopisuje je do słownika (okno „Dodaj do słownika parametrów”).
- Repozytorium na GitHubie nazywa się `Magmass` (adres github.com/amassalski/Magmass): zmienione adresy w sprawdzaniu aktualizacji, oknie „O programie”, pakiecie DEB, pomocy i testach. Starsze wersje programu, wskazujące na stare repozytorium, nie znajdą aktualizacji, dopóki nie zmienią adresu w Ustawienia / Ścieżki / Aktualizacje.
- Katalog: słownik opisów parametrów jest teraz tabelą w bazie (`tbl_slownik_parametrow`, schemat 17) z oknem „Słownik parametrów...” (przycisk pod listą katalogu): dodawanie, zmiana, usuwanie, wyszukiwanie i „Przywróć brakujące wbudowane”. Słownik jest wspólny dla wszystkich komputerów z tą bazą; przy aktualizacji bazy dostaje wbudowany zestaw symboli. Własny opis parametru w pozycji nadal ma pierwszeństwo.
- Okno „O programie”, karta „Podziękowania”: dodany Claude (Anthropic) jako współtwórca (kod, testy, dokumentacja pod kierunkiem autora); tłumaczenie angielskie.
- Spójne nazwy: katalog projektu (i katalog główny w paczkach zip) nazywa się `Magmass`; repozytorium na GitHubie dostało tę samą nazwę. Wewnętrzna nazwa `DomowyMagazyn` w ustawieniach programu (katalog `~/.config/DomowyMagazyn/config.ini`) zostaje, żeby nie gubić istniejących konfiguracji. Skrypt `kompiluj_wszystko.sh` nie ma już na sztywno zapisanej ścieżki.

- macOS: `zbuduj_macos.sh --wlasny-opencv` nie pomija już po cichu istniejącego katalogu budowania z programem zbudowanym bez własnego OpenCV (czyści go i buduje od nowa); do linkowania statycznego OpenCV dołożone frameworki CoreGraphics, ImageIO, CoreFoundation, AppKit i Accelerate. Nie sprawdzone na Macu.

- Katalog: przycisk „OCR datasheetu...” (poza menu podręcznym), a „Odczytaj parametry z PDF” na skanie bez warstwy tekstowej proponuje OCR i odczytuje plik ponownie.
- Ustawienia, zakładka „Inne”: przy polach z programem (czytnik PDF, ExifTool, ocrmypdf) mały przycisk „...” otwiera okno wyboru pliku. Przy poleceniu czytnika podmieniana jest tylko nazwa programu, argumenty (%f, %p) zostają.
- Katalog: każdy parametr ma pole „Opis (dymek)”, które pokazuje się po najechaniu na nazwę parametru (okno edycji pozycji i lista parametrów w katalogu). Puste pole uzupełnia wbudowany słownik typowych symboli (np. W_amax = maksymalna moc strat anody). Schemat bazy: wersja 16.
- Kompilacja na systemach, w których wxWidgets jest bez wxWebRequest (np. Devuan/Debian): klient HTTP używa wtedy libcurl (wymaga libcurl4-openssl-dev); opcja `-DDM_WYMUS_CURL=ON` wymusza libcurl do testów.
- Kamera termowizyjna Xtherm (T3 itp.): przycisk „Biblioteka temperatur Xtherm” ładuje XthermDll.dll z kopii użytkownika (x64, Windows) i liczy z naszych klatek 384x292 temperatury (tryb surowy 0x8004: cały obraz, YUYV 0x8005: środek/min/max). Biblioteki producenta nie dołączamy. „Zapisz próbkę” zapisuje przy załadowanej bibliotece także jej temperatury (_xtherm.f32) oraz FPA i temperaturę rdzenia: dane do własnej kalibracji z surowych odczytów.\n- Kamera termowizyjna: opcjonalne SDK IRay (USBSDK.dll) ładowane w czasie działania z kopii użytkownika (biblioteki ani nagłówków producenta nie dołączamy, licencja nie pozwala na redystrybucję); przycisk „Sprawdź SDK IRay” wypisuje kamery, typ rdzenia, PN/SN i zakres surowych odczytów temperatury. Tylko Windows x64.
- Cewki powietrzne: nowe rysunki jedno- i wielowarstwowej w przekroju osiowym (karkas kreskowany, policzki, zwoje drutu w izolacji, oś, wymiary D, l, skok, b, c).
- Transformator sieciowy: pole „Margines warstwy” (domyślnie 1,5 mm): wysokość nawijania y = h − 2·ścianka − 2·margines; 0 = wzór książkowy.
- Transformator SE (uzwojenia wykonane): układ 2×2 (Rozmieszczenie obok Zestawienia drutu), Komunikaty w ramce pod Wynikami, wyrównane pola, ostrzeżenia modelu w Komunikatach.
- Transformatory PP i SE: „Poprawka k (brzegi)” domyślnie liczona automatycznie z modelu Rogowskiego (K = 1 − (1 − e^−a)/a, a = π·h/δ); wpisanie liczby wymusza wartość, wynik pokazuje „Użyta poprawka k”.
- Zniekształcenia harmoniczne: rysunek poglądowy (charakterystyki anodowe, prosta obciążenia, pięć punktów) pokazujący, co odczytać z wykresu; zmienia się z opcją: prądy Ia max…Ia min albo odcinki d, e, f, g oraz a = d+e, b = f+g, c = e+f, z opisem pod rysunkiem.
- Transformator PP (Etap 3) i SE (uzwojenia wykonane): nowe pola „Ścianka karkasu” i „Margines warstwy” (domyślnie 1,5 mm). Długość warstwy = h − 2·ścianka − 2·margines (liczba zwojów w warstwie, grubość uzwojeń, rozproszenie, pojemności), szerokość okna pomniejszona o ściankę od strony rdzenia (kontrola „mieści się”); 0 = okno idealne jak dotąd.
- Transformator PP, Etap 1: przeznaczenie „gitarowy” ustawia pasmo 50 Hz – 12 kHz (Hi-Fi: 40 Hz – 20 kHz).
- macOS: odczyt datasheetów i indeksowanie PDF-ów (pdftotext, pdfinfo) znajdują poppler zainstalowany przez MacPorts (/opt/local/bin) i Homebrew także po uruchomieniu aplikacji z Findera (minimalny PATH).
- Transformator SE i PP: wymiary blachy EI (A, B, a, c, d, h) w ramce „Rdzeń EI” – wybór kształtki wypełnia je, ale można je edytować (kształtka przechodzi na „własna”); z wymiarów i grubości pakietu liczone są przekrój, droga strumienia, długość zwoju, wysokość i szerokość okna oraz rysunek rdzenia.
- Transformator PP: lampa z katalogu wpisuje też Raa (nowy parametr katalogu „Raa PP [Ω]”; wcześniej dodane lampy przykładowe dostają go automatycznie).
- Transformator PP: lista lamp pokazuje tylko typ lampy; węższe pola Etapu 1, szerokości pól i wartości w ramkach „Wymiary zdobytego rdzenia”, „Etap 2” i „Porównanie: metoda gitarowa” liczone z czcionki (wartości nie są ucinane).
- Transformator PP, Etap 1: lista „Lampa z katalogu” (EL34, 6P14P, EL84, 6V6, 6L6, ECL86, ECL82, EBL21 dopisywane do katalogu przy pierwszym otwarciu) – wpisuje rezystancję wewnętrzną Ri i prąd spoczynkowy I0 lampy; nowy parametr katalogu „Ia0 PP [mA]”. Wartości orientacyjne, ECL82 i EBL21 do weryfikacji w kartach.
- Transformator PP Etap 3: „Rozmieszczenie uzwojeń” po lewej, „Zestawienie drutu” po prawej (zamiana miejscami).
- Transformator PP, porównanie metodą gitarową: uzwojenia wtórne (zwoje / drut) zawsze dla 4, 8 i 16 Ω, niezależnie od wpisanego obciążenia.
- Transformator PP Etap 3: siatka 2×2 – „Uzwojenia wykonane” i „Wyniki” mają tę samą wysokość, „Zestawienie drutu” i „Rozmieszczenie uzwojeń” również.
- Transformator PP Etap 3: „Sekcje od rdzenia” obok kolumn wykonane/proponowane (trzy kolumny), listy układu i połączenia wtórnych wyrównane z polami zwojów i drutów.
- Kolory wyników i komunikatów zależne od motywu (jasny/ciemny): w ciemnym motywie jaśniejszy błękit, bursztyn, szary i zieleń (nowy nagłówek KoloryMotywu.h); wykres analizy dokładnej też.
- Transformator PP Etap 3 (2): zestawienie drutu pod uzwojeniami wykonanymi, wyniki w dwóch kolumnach, komunikaty i przycisk Powrót pod nimi. Wcześniej: Etap 3 i okno „Analiza dokładna”: zwarty układ (uzwojenia wykonane po lewej; wyniki, rozmieszczenie i zestawienie drutu po prawej; analiza w dwóch kolumnach wyników) – formularz mieści się na ekranie 1280×900.
- Transformator PP: ramka „Pożądane parametry” pod Etapem 1 (Przeznaczenie i Spadek w jednej kolumnie z parametrami), rysunek rdzenia pod wymiarami zdobytego rdzenia. Rysunek rdzenia (wszystkie formularze): okna w 3D, zaokrąglona średnia długość zwoju, wymiary zakończone pełnymi grotami.
- Transformator sieciowy: pole „Wpisz uzwojenia wtórne” przeniesione do ramki Uzwojenia obok układu uzwojeń (przykład w dymku).
- Kalkulator zasilacza: schemat zmienia się z rodzajem prostownika (mostek Graetza, jednopołówkowy, dwupołówkowy z odczepem); w mostku i odczepie symbole masy.
- Narzędzia audio i lampowe: zakładki „dB i nepery” oraz „Krzywa RIAA” (bez „ECCLab” w nagłówkach); Punkt pracy lampy – wybór parametrów lampy z katalogu (μ, ra, Pa max).
- Dławik: objaśnienie, kiedy szczelina jest niepotrzebna (z liczbami), etykieta „Szczelina bez reluktancji żelaza”, nowy układ okna (Wyniki w dwóch kolumnach na całą szerokość, rysunek rdzenia dopasowany do pól wymiarów, średnia długość zwoju i zaokrąglona droga strumienia dla jednej połówki). Podobnie Transformator sieciowy i SE (rysunek pod Wynikami, uwaga na brązowo pod porównaniem Witorta).
- Pomoc: każde pole i każdy wynik w Dławiku, transformatorach, narzędziach audio/lampowych, metodach graficznych, zniekształceniach, cewce z AL i kodach ma własny opis (dymek i definicja w pliku pomocy), opisujący tylko to jedno pole.
- Menu: ikony przy pozostałych pozycjach menu Narzędzia i Baza danych.
- Słownik obudów: wielokrotny wybór (Ctrl/Shift), usuwanie zbiorcze i edycja zbiorcza zaznaczonych.
- Kalkulatory: LM317/LM337 (napięcie wyjściowe z rozrzutem Vref i Iadj, dobór R2, kontrola minimalnego prądu i napięcia wejściowego, moc strat) oraz (w menu Narzędzia > Elementy indukcyjne > Cewki powietrzne) cewka powietrzna jedno- i wielowarstwowa (indukcyjność z sumy indukcyjności własnych i wzajemnych zwojów, całki eliptyczne; wzór Wheelera obok do porównania; dobór liczby zwojów/warstw; długość i opór drutu, masa miedzi). Każda zakładka ma schemat z wyróżnionymi liczonymi elementami.

- Instalator Windows: strona wyboru skrótów (menu Start, pulpit, pasek szybkiego uruchamiania).
- macOS: instalator .pkg (`narzedzia/zbuduj_pkg.sh`, wywoływany przez `zbuduj_macos.sh`; `--bez-pkg` pomija) z ekranem „Dostosuj” i opcją „Dodaj do Docka”.
- Kamera termowizyjna T3 Pro: tryb surowy (14-bitowe odczyty) działa po poleceniu UVC zoom (potwierdzone na macOS); raport nagłówka klatki (parametry pomiaru: T odbita, T otoczenia, wilgotność, emisyjność, odległość; słowa 4 i 7 = max i min odczytu); kalibracja użytkownika odczyt → °C z punktów odniesienia; polecenie zoom przyjmuje sekwencję.

- Kamera termowizyjna: rozpoznanie klatki 384x292 (obraz 8-bitowy + 4 wiersze nagłówka; tryb surowy 16-bitowy bez fałszywych °C), zrzut nagłówka w raporcie, przycisk wysyłający polecenie UVC „zoom” (8004 = tryb surowy w kamerach HT-301/Xtherm; na macOS przez IOKit).

- Kamera termowizyjna na macOS: odbiór klatek bezpośrednio przez AVFoundation (surowy YUYV, KameraMac.mm), bo OpenCV nie otwierał InfiRay T3 Pro; okno pokazuje nazwy urządzeń, formaty i raport wykrywania.

- Dławik i wszystkie transformatory: zestawienie drutu na wagę (długość i masa miedzi oraz masa do zakupu dla każdej średnicy).

- Nowe menu Kamera: kamera termowizyjna USB (InfiRay T3 Pro): podgląd z paletami, krzyżyk i znaczniki min/max, diagnostyka surowych danych, zapis próbki i obrazu PNG (wersja 2.5, temperatury do sprawdzenia pomiarem).

- Skrypty budowania Windows i macOS: opcja `--wlasny-opencv` (narzedzia/zbuduj_opencv.sh) pobiera źródła OpenCV i buduje minimalną wersję statyczną bez ffmpeg, co usuwa większość z ok. 150 bibliotek DLL w paczce.

- Narzędzia / Kalkulatory: usunięty zdublowany kalkulator trafa głośnikowego (jest w Elementach indukcyjnych); zakładka „Lampa” przeniesiona do Narzędzi audio i lampowych („Punkt pracy lampy”). Nowe: schematy z wyróżnionymi liczonymi elementami (dzielnik, LED, filtr RC, zasilacz, tranzystor, lampa, kondensator) oraz zakładka „ESR kondensatorów” (tabela pojemność × napięcie, moc strat od prądu tętnień).

- Transformator PP, Etap 3: tabela sekcji uzwojeń (A = pierwotne, W = wtórne, | = przegroda karkasu), szablony (gitarowy, HiFi dwukomorowy), wtórne szeregowo/równolegle, rozproszenie w komorach, pojemności z rzeczywistych potencjałów warstw.

- Rysunek rdzenia EI: wymiar b obok A, linie blaszek na bokach, lewy skos, średnia droga strumienia (przerywana), wymiar c przy drugim oknie.
- Nowe: Cewka na rdzeniu z AL (zwoje z AL i indukcyjności, tolerancja, kontrola indukcji, porównanie z liczbą zwojów z innego programu). Formularze SE i PP: rysunek rdzenia EI z wymiarami.

- Transformatory SE i PP: wybór spadku na krańcach pasma (1, 2 lub 3 dB) zamiast stałych 3 dB; wpływa na Li, Lr, krańce pasma i wykres. Analiza dokładna: domyślnie brak szczeliny styku.

- Transformatory głośnikowe SE i PP: wspólny moduł „Analiza dokładna” (TrafoModel): pełny schemat zastępczy, μ przyrostowe z polaryzacją i szczeliną, rozproszenie, pojemność własna, straty w żelazie, pasmo z transmitancji zespolonej, wykres; sprawdzony z granicami analitycznymi.

- Transformator PP: porównanie z metodą gitarową (guitaramplifiers.prv.pl): L1 z Mn, objętość rdzenia, zwoje, I1, drut; wzory sprawdzone niezależnie.

- Elementy indukcyjne: transformator głośnikowy SE (wybór kształtki, model fizyczny, uzwojenia wykonane z rozmieszczeniem w oknie, rozproszeniem i sprawnością); porównanie z programem ze zrzutu i z modułem Cykina. Przekładnia liczona fizycznie jako √(η·Ra/Rg).
- Transformator PP: indukcja dla wariantu gitarowego 8000 Gs (zamiast roboczych 3000 Gs), wg literatury gitarowej dla ok. 50 W.
- Projekty: menu kontekstowe pozycji części z poleceniem „Idź do elementu w magazynie” (przełącza na zakładkę Magazyn i zaznacza element).
- Poprawka: lista części projektu odświeża się przy powrocie na zakładkę Projekty, więc zmiana stanu w Magazynie jest widoczna od razu (wcześniej dopiero po ponownym uruchomieniu).
- Transformator PP, Etap 3: uzwojenia wykonane, rozmieszczenie w warstwach, rezystancje, rozproszenie (model fizyczny, porównanie z ECCLab), sprawność. Dławik: szczelina fizyczna, wzór Krize/ECCLab tylko jako porównanie.
- Transformator sieciowy (Krize) i transformator głośnikowy PP (etapy 1 i 2) w menu Elementy indukcyjne.
- Dławik zasilacza (elementy indukcyjne): rdzeń EI, zwoje, drut, szczelina, opór, spadek napięcia, wypełnienie okna.
- Zniekształcenia harmoniczne metodą pięciu rzędnych (z prądów albo z odcinków).
- Metody graficzne: okno z obrazem charakterystyki anodowej, skalowanie osi z trzech punktów, prosta obciążenia (Ra klasy A i B, moce SE i PP), blokady Ra i amplitudy, siatka kontrolna i krzywa Pa max.
- Schemat układu lampowego obok pól (elementy użyte w obliczeniach wyróżnione).
- Lampy w katalogu: pozycje rodziny „Lampy elektronowe” z parametrami Sa, Ka, Rwew, Cak, Cas, Csk; wybór i zapis lampy z okna układów lampowych.
- Układy lampowe (zakładka w Narzędziach audio): sprzężenie RC, wtórnik anodowy, wtórnik katodowy, inwerter różnicowy, mi-wtórnik, SRPP, wtórnik White'a, kaskodowy wtórnik katodowy, superwtórnik; moduły `tube_audio` i `tube_magnetics` (obliczenia wtórników, SRPP, WCF, LTP, transformatory i dławiki) włączone do `src/Logika`; poprawiony licznik napięcia katody w `calcLTPTriode`.

- Narzędzia audio i lampowe (Ctrl+Shift+L): ECCkalkulator (dB, nepery, miara liniowa), krzywa RIAA (odtwarzanie i nagrywanie, opcja IEC) z tabelą i wykresem oraz dobór elementów RC biernej sieci korekcyjnej z zaokrągleniem do szeregów E.
- Drukarka Zebra ZD421 (ZPL): etykiety na rolce drukowane przez sieć (port 9100), USB / kolejkę systemową albo zapisywane do pliku .zpl; okno ustawień, etykieta próbna, podgląd układu.

## 1.0.0 (2026-10-27)

Pierwsze wydanie programu Magmass (dawniej DomowyMagazyn): magazyn części elektronicznych w C++/wxWidgets z bazą PostgreSQL.

### Magazyn
- Drzewo kategorii, lista elementów, formularz dodawania i edycji, wyszukiwanie, edycja zbiorcza.
- Miejsce magazynowania: kontener, szuflada, przegroda; stany minimalne, historia zmian, numer inwentarzowy.
- Hurtowe dodawanie szeregów E12, E24, E48, E96 oraz import i eksport CSV.
- Zdjęcia elementów, kody kreskowe i etykiety, odczyt kodu z kamery lub obrazu, raporty.
- Sortowanie list po kliknięciu nagłówka kolumny (Magazyn, Projekty, Literatura, Katalog).

### Projekty
- Listy części, import z KiCada i Eagle'a (BOM), braki, lista zakupów, wydawanie części z magazynu.
- Sklepy, ceny, kursy walut i najtańsze zestawienie zakupów z dostawą.

### Katalog
- Katalog elementów z parametrami, rodziną, obudową i datasheetami (pobieranie, odczyt danych z PDF, OCR).
- Wyszukiwanie datasheetów w Mouser (wymaga własnego klucza API).
- Menu podręczne: usuń z katalogu, OCR datasheetu.

### Literatura
- Baza książek: import z folderu PDF/EPUB, metadane XMP/Info i OPF, dane z Biblioteki Narodowej.
- Indeks treści PDF i wyszukiwanie w treści z przejściem do strony, OCR (ocrmypdf), zapis metadanych do PDF (ExifTool).
- Pola: wydanie, uwagi w HTML; pożyczanie książek; menu podręczne.
- Ustawialne polecenie czytnika PDF (np. Skim, Brave) z numerem strony.

### Narzędzia
- Baza narzędzi z pożyczkami, terminami zwrotu i historią.
- Dekoder pasków rezystorów i kodów SMD; kalkulatory: zasilacz z transformatorem, transformator głośnikowy, punkt pracy tranzystora i lampy.
- Współpraca z Eagle CAD 7.6: skrypt .scr z atrybutami części, program ULP z listą części.

### Program
- Pomoc HTML (układ jak Eagle 7.6), pomoc kontekstowa F1, dymki z opisami pól, własny zestaw ikon.
- Interfejs po polsku i angielsku, sprawdzanie aktualizacji, okno „O programie” z licencją i podziękowaniami.
- Instalatory: Windows (NSIS, licencja do zatwierdzenia), macOS, pakiet DEB. Licencja GPL-3.0 lub nowsza.
