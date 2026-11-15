# Budowanie programu (Windows, MSYS2)

Cały program buduje się w powłoce **MSYS2 UCRT64** (nie MSYS). W środowisku MINGW64 repozytorium MSYS2 nie ma już wxWidgets 3.2 (są tylko wersje `ucrt64`, `clang64` i `clangarm64`), więc zalecane jest UCRT64; skrypty i CMake działają też w MINGW64 i CLANG64, jeśli masz tam wxWidgets.

## Pakiety

```bash
pacman -S --needed base-devel git zip unzip curl \
  mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-wxwidgets3.2-msw mingw-w64-ucrt-x86_64-postgresql \
  mingw-w64-ucrt-x86_64-gettext-tools mingw-w64-ucrt-x86_64-python
```

## Budowanie

```bash
cd /g/Massaada/Develop/Magmass
rm -rf build && mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release          # -DDM_CONSOLE=ON dodaje konsolę do debugowania
ninja
./Magmass.exe
```

Po zbudowaniu CMake kopiuje obok exe katalog `sql/` oraz (w MSYS2) wymagane DLL-ki z `/ucrt64` (albo `/mingw64`).
Nie kopiuj nic z innych instalacji wxWidgets/PostgreSQL, bo pomieszane wersje DLL
dają błąd 0xc0000139.

## Konfiguracja

Program szuka `config.ini` obok exe, potem w katalogu bieżącym i katalog wyżej.
Jeśli pliku nie ma, zapisze nowy obok exe. Połączenie ustawiasz w programie:
**Baza danych → Ustawienia połączenia...**. Hasło można zapisać w pliku (zwykły tekst)
albo nie zapamiętywać; wtedy program zapyta o nie przy starcie.

## Pierwsze uruchomienie bazy

Po połączeniu z pustą bazą program zaproponuje utworzenie struktury. Można też zrobić to
później: **Baza danych → Utwórz / zaktualizuj strukturę bazy...** (skrypty z katalogu `sql/`
można uruchamiać wielokrotnie, nie kasują danych). Ręcznie, kolejno:
`schema.sql`, `funkcje_i_trigger.sql`, `widoki.sql`, `dane_startowe.sql`.
Wymagany PostgreSQL 9.6 lub nowszy i rozszerzenie `ltree`. Jeśli użytkownik aplikacji
nie ma uprawnienia CREATE na bazie, administrator wykonuje jednorazowo:
`psql -U postgres -d NAZWA_BAZY -c "CREATE EXTENSION IF NOT EXISTS ltree;"`.

## Obsługa programu
Opis okien i funkcji: `instrukcja_uzytkownika.md`.

## Testy

```bash
cmake .. -G Ninja -DDM_BUILD_TESTS=ON
ninja
# osobna, pusta baza testowa! Test czyści schemat public.
DM_TEST_DB=dm_test DM_TEST_USER=postgres DM_TEST_PASSWORD=... ctest --output-on-failure
```

## Pakiet DEB (Debian, Ubuntu, Mint)
Budowanie: `narzedzia/zbuduj_deb.sh` (wymaga cmake, g++, libwxgtk3.2-dev, libpq-dev, dpkg-dev; kamera: libopencv-dev). Wynik: `magmass_<wersja>_amd64.deb`.

Instalacja: `sudo apt install ./magmass_1.0.0_amd64.deb` (apt doinstaluje wymagane biblioteki). Uruchomienie: polecenie `magmass` albo ikona w menu aplikacji.

Program leży w `/usr/lib/magmass`. Ponieważ ten katalog jest tylko do odczytu, konfiguracja trafia do `~/.config/DomowyMagazyn/config.ini`, a zdjęcia i datasheety (jeśli nie ustawisz własnych ścieżek w Ustawieniach) do `~/.local/share/Magmass/`. Serwer PostgreSQL nie jest instalowany razem z pakietem (może działać na innym komputerze); po pierwszym uruchomieniu wpisz dane połączenia i wybierz "Baza danych / Utwórz / zaktualizuj strukturę bazy". Do odczytu parametrów z datasheetów przyda się `poppler-utils` (pakiet go poleca).

## Instalatory dla Windows i macOS
Każdy pakiet buduje się na swoim systemie. Wspólne wejście: `narzedzia/zbuduj_instalator.sh [deb|windows|macos]` (bez argumentu wybiera bieżący system).
- **Windows** (powłoka MSYS2 UCRT64): `narzedzia/zbuduj_windows.sh`. Wynik: `Magmass-<wersja>-win64.zip` (wersja przenośna, z bibliotekami DLL) oraz `Magmass-<wersja>-instalator.exe`, jeśli jest NSIS (`pacman -S mingw-w64-ucrt-x86_64-nsis`; do ikony `mingw-w64-ucrt-x86_64-imagemagick`; do odczytu datasheetów `mingw-w64-ucrt-x86_64-poppler`). Instalator kopiuje program do Program Files i dodaje wpis w „Aplikacje i funkcje". Na stronie „Wybór składników” pytania o skróty: w menu Start (domyślnie tak), na pulpicie (domyślnie tak) i na pasku szybkiego uruchamiania (domyślnie nie; to folder klasycznego paska Quick Launch, widoczny tylko gdy włączysz go w systemie; przypięcie do paska zadań Windows robi się ręcznie prawym klikiem na skrócie, bo instalator nie może tego zrobić w sposób obsługiwany przez system). Konfiguracja użytkownika trafia do `%APPDATA%\DomowyMagazyn`.
- **macOS**: `narzedzia/zbuduj_macos.sh` (potrzebne: `brew install cmake wxwidgets postgresql@15 dylibbundler`, opcjonalnie `poppler`). Wynik: `Magmass-<wersja>-macos-<arch>.dmg` z `Magmass.app` z dołączonymi bibliotekami (dylibbundler) oraz `Magmass-<wersja>-macos-<arch>.pkg`: instalator z ekranami powitania, licencji i „Dostosuj”, na którym można zostawić albo odznaczyć „Dodaj do Docka” (`narzedzia/zbuduj_pkg.sh`, narzędzia pkgbuild i productbuild z Xcode Command Line Tools; opcja `--bez-pkg` pomija instalator). Pakiet .pkg można zbudować tylko na Macu. Bez certyfikatu Apple Developer ID jest niepodpisany, więc przy pierwszym otwarciu kliknij go prawym przyciskiem i wybierz Otwórz; z certyfikatem ustaw zmienną `PKG_PODPIS="Developer ID Installer: ..."` przed uruchomieniem skryptu. Odinstalowanie: przeniesienie Magmass.app z Programów do Kosza. Aplikacja jest podpisana tylko „ad hoc", więc po pobraniu z internetu otwórz ją prawym klikiem → Otwórz. Opcja `--bez-bibliotek` pomija dołączanie bibliotek (działa wtedy tylko na tym Macu). Konfiguracja użytkownika: `~/.config/DomowyMagazyn`. Przy pierwszym użyciu kamery macOS zapyta o zgodę.
- Skrypty dla Windows i macOS nie były uruchamiane na tych systemach (nie mam do nich dostępu); sprawdzono składnię, szablon instalatora NSIS (kompiluje się) i plik Info.plist. Pierwsze uruchomienie u Ciebie może wymagać drobnych poprawek.
