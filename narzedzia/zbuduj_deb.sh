#!/bin/bash
# Buduje pakiet DEB (Debian / Ubuntu / Mint). Uruchom z katalogu głównego projektu:
#
#   narzedzia/zbuduj_deb.sh [katalog_budowania]
#
# Domyślny katalog budowania: build-deb (jeśli nie ma programu, skrypt go skonfiguruje i zbuduje).
# Wynik: magmass_<wersja>_<architektura>.deb w katalogu głównym projektu.
# Wymaga: cmake, g++, libwxgtk3.2-dev, libpq-dev, dpkg-dev; opcjonalnie gettext i libopencv-dev (kamera).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
B="$(realpath -m "${1:-$ROOT/build-deb}")"
cd "$ROOT"

WERSJA="$(sed -n 's/^project(Magmass VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt | head -1)"
ARCH="$(dpkg --print-architecture)"
[ -n "$WERSJA" ] || { echo "Nie znaleziono wersji w CMakeLists.txt" >&2; exit 1; }

if [ ! -x "$B/Magmass" ]; then
    cmake -S . -B "$B" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$B" -j"$(nproc)"
fi

# Plik pomocy musi być aktualny
if command -v python3 >/dev/null; then python3 narzedzia/zbuduj_pomoc.py >/dev/null; fi

PAK="$B/pakiet"
rm -rf "$PAK"
LIB="$PAK/usr/lib/magmass"
install -d "$LIB/sql" "$LIB/pomoc" "$PAK/usr/bin" "$PAK/usr/share/applications" \
           "$PAK/usr/share/doc/magmass" "$PAK/DEBIAN"

# Program w układzie, który zna sam program: obok pliku wykonywalnego leżą sql/, pomoc/ i pl/LC_MESSAGES/
install -m 755 "$B/Magmass" "$LIB/Magmass"
strip --strip-unneeded "$LIB/Magmass" 2>/dev/null || true
install -m 644 sql/*.sql "$LIB/sql/"
install -m 644 pomoc/pomoc_pl.htm pomoc/pola.txt "$LIB/pomoc/"
if [ -f "$B/pl/LC_MESSAGES/Magmass.mo" ]; then
    install -D -m 644 "$B/pl/LC_MESSAGES/Magmass.mo" "$LIB/pl/LC_MESSAGES/Magmass.mo"
elif [ -f lang/pl.mo ]; then
    install -D -m 644 lang/pl.mo "$LIB/pl/LC_MESSAGES/Magmass.mo"
fi
for J in pl en; do
    if   [ -f "$B/$J/LC_MESSAGES/Magmass.mo" ]; then install -D -m 644 "$B/$J/LC_MESSAGES/Magmass.mo" "$LIB/$J/LC_MESSAGES/Magmass.mo"
    elif [ -f lang/$J.mo ]; then install -D -m 644 lang/$J.mo "$LIB/$J/LC_MESSAGES/Magmass.mo"; fi
done
[ -d assets/etykiety ] && { install -d "$LIB/assets/etykiety"; install -m 644 assets/etykiety/* "$LIB/assets/etykiety/"; }

ln -s ../lib/magmass/Magmass "$PAK/usr/bin/magmass"
install -m 644 Magmass.desktop "$PAK/usr/share/applications/magmass.desktop"
for r in 32 128 256; do
    [ -f "assets/ikony/app_$r.png" ] && install -D -m 644 "assets/ikony/app_$r.png" \
        "$PAK/usr/share/icons/hicolor/${r}x${r}/apps/magmass.png"
done
install -m 644 README.md "$PAK/usr/share/doc/magmass/README.md"
install -m 644 LICENSE "$LIB/LICENSE"
install -m 644 LICENSE "$PAK/usr/share/doc/magmass/LICENSE"
[ -d dokumentacja ] && cp -r dokumentacja "$PAK/usr/share/doc/magmass/" && chmod -R u=rwX,go=rX "$PAK/usr/share/doc/magmass"
cat > "$PAK/usr/share/doc/magmass/copyright" <<'KOP'
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: Magmass
Source: https://github.com/amassalski/Magmass

Files: *
Copyright: 2026 Adam Massalski
License: GPL-3+
 This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 .
 On Debian systems, the full text of the GNU General Public License version 3 can be found in /usr/share/common-licenses/GPL-3.
KOP

# Zależności z faktycznie użytych bibliotek (dpkg-shlibdeps), z listą awaryjną
DEP=""
if command -v dpkg-shlibdeps >/dev/null; then
    TMP="$(mktemp -d)"; mkdir -p "$TMP/debian"
    printf 'Source: magmass\n\nPackage: magmass\nArchitecture: any\n' > "$TMP/debian/control"
    DEP="$(cd "$TMP" && dpkg-shlibdeps -O -e"$LIB/Magmass" 2>/dev/null | sed -n 's/^shlibs:Depends=//p')" || true
    rm -rf "$TMP"
fi
[ -n "$DEP" ] || DEP="libwxgtk3.2-1 | libwxgtk3.2-1t64, libpq5, libc6, libstdc++6"

ROZMIAR="$(du -sk "$PAK/usr" | cut -f1)"
cat > "$PAK/DEBIAN/control" <<KONTROLA
Package: magmass
Version: $WERSJA
Section: electronics
Priority: optional
Architecture: $ARCH
Depends: $DEP
Recommends: postgresql, poppler-utils
Suggests: postgresql-client
Installed-Size: $ROZMIAR
Maintainer: Użytkownik Domowy <user@example.com>
Description: Magazyn części elektronicznych z bazą PostgreSQL
 Ewidencja elementów (kategorie, miejsca, stany, ceny, sklepy), projekty
 i listy części (import z KiCada i Eagle'a), katalog elementów z odczytem
 parametrów z datasheetów, generator szeregów E, dekoder pasków rezystorów
 i kodów SMD, kalkulatory, etykiety z kodami kreskowymi, zdjęcia z kamery USB.
 .
 Program łączy się z serwerem PostgreSQL (może być na innym komputerze).
 Po pierwszym uruchomieniu wpisz dane połączenia i wybierz z menu
 "Baza danych / Utwórz / zaktualizuj strukturę bazy".
KONTROLA

cat > "$PAK/DEBIAN/postinst" <<'POST'
#!/bin/sh
set -e
if [ "$1" = "configure" ]; then
    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database -q /usr/share/applications || true
    command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -q -t /usr/share/icons/hicolor || true
fi
exit 0
POST
cp "$PAK/DEBIAN/postinst" "$PAK/DEBIAN/postrm"
sed -i 's/"\$1" = "configure"/"$1" = "remove" -o "$1" = "purge"/' "$PAK/DEBIAN/postrm"
chmod 755 "$PAK/DEBIAN/postinst" "$PAK/DEBIAN/postrm"

( cd "$PAK" && find usr -type f -exec md5sum {} + > DEBIAN/md5sums )
find "$PAK" -type d -exec chmod 755 {} +

WYNIK="$ROOT/magmass_${WERSJA}_${ARCH}.deb"
dpkg-deb --root-owner-group --build "$PAK" "$WYNIK" >/dev/null
echo "Gotowe: $WYNIK"
dpkg-deb -I "$WYNIK" | sed -n '1,14p'
