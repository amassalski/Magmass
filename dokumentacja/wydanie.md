# Wydanie wersji (procedura)

Przykład dla 1.0.0. Przed wydaniem upewnij się, że w repozytorium nie ma `config.ini`, kluczy API ani haseł do bazy (`.gitignore` je wyklucza; sprawdź `git status`).

0. **Wersje robocze**: między wydaniami podnoś w `CMakeLists.txt` ostatni człon numeru (1.9.1, 1.9.2, ...), a przy wydaniu ustaw docelowy (np. 2.0.0). Plik `wersja.txt` zmieniaj dopiero przy wydaniu, bo na jego podstawie program sprawdza aktualizacje.
1. **Numer wersji**: `CMakeLists.txt` (`project(Magmass VERSION 1.0.0 ...)`). Z niego biorą numer okno „O programie”, instalatory (Windows, macOS, DEB) i sprawdzanie aktualizacji.
2. **Testy**: `run_tests`, `test_import`, `test_gui` bez błędów.
3. **Instalatory**: `narzedzia/zbuduj_windows.sh` (Windows, MSYS2), `narzedzia/zbuduj_macos.sh` (macOS), `narzedzia/zbuduj_deb.sh` (Linux). Sprawdź instalację na czystym systemie i okno z licencją w instalatorze Windows.
4. **Commit i tag**:
   ```
   git add -A
   git status            # sprawdź, że nie ma config.ini ani plików .exe/.deb/.zip
   git commit -m "Wydanie 1.0.0"
   git tag -a v1.0.0 -m "Magmass 1.0.0"
   git push origin HEAD --tags
   ```
5. **Wydanie na GitHubie**: Releases, Draft a new release, wybierz tag `v1.0.0`, wklej sekcję 1.0.0 z `CHANGELOG.md`, dołącz instalatory (`Magmass-1.0.0-instalator.exe`, `Magmass-1.0.0-macos-<arch>.dmg`, `magmass_1.0.0_amd64.deb`).
6. **Aktualizacje**: dopiero po publikacji instalatorów zmień `wersja.txt` (pierwsza linia: numer; opcjonalnie druga: adres pobierania, dalej opis) i wyślij do repozytorium. Program użytkowników pokaże wtedy informację o nowej wersji.

Kolejna wersja: podnieś numer w `CMakeLists.txt`, dopisz sekcję w `CHANGELOG.md`, powtórz kroki.
