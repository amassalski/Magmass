#!/bin/bash
# Buduje wersję dla Windows: folder z programem i bibliotekami, archiwum ZIP (wersja przenośna)
# oraz instalator .exe (jeśli jest NSIS). Uruchom w powłoce MSYS2 MinGW 64-bit, z katalogu projektu:
#
#   narzedzia/zbuduj_windows.sh [--wlasny-opencv] [katalog_budowania]
#
# --wlasny-opencv: zamiast pakietu OpenCV z MSYS2 (ciągnie ffmpeg i dziesiątki DLL-i) pobiera źródła OpenCV, buduje
#   minimalną wersję statyczną (narzedzia/zbuduj_opencv.sh) i wkompilowuje ją w program. Działa tylko przy świeżym
#   katalogu budowania (usuń build-win, jeśli program był już zbudowany inaczej).
#
# Wymaga: pakietów toolchain, cmake, ninja, wxWidgets, postgresql z jednego środowiska MSYS2 (zalecane UCRT64: mingw-w64-ucrt-x86_64-*;
#   w środowisku MINGW64 nie ma już wxWidgets 3.2);
# opcjonalnie: nsis (instalator), imagemagick (ikona), opencv (kamera), poppler (pdftotext), gettext (tłumaczenia):
#   pacman -S mingw-w64-ucrt-x86_64-nsis mingw-w64-ucrt-x86_64-imagemagick mingw-w64-ucrt-x86_64-poppler
set -euo pipefail

WLASNY_OPENCV=0
if [ "${1:-}" = "--wlasny-opencv" ]; then WLASNY_OPENCV=1; shift; fi
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
case "${MSYSTEM:-}" in MINGW64|UCRT64|CLANG64) ;; *)
    echo "Uruchom ten skrypt w powłoce MSYS2 UCRT64 (albo MinGW 64-bit / CLANG64; MSYSTEM=$MSYSTEM)." >&2; exit 1;; esac
B="$(realpath -m "${1:-$ROOT/build-win}")"
WERSJA="$(sed -n 's/^project(Magmass VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt | head -1)"
[ -n "$WERSJA" ] || { echo "Nie znaleziono wersji w CMakeLists.txt" >&2; exit 1; }

if [ ! -f "$B/Magmass.exe" ]; then
    GEN="MSYS Makefiles"; command -v ninja >/dev/null && GEN="Ninja"
    OPCJE=()
    if [ "$WLASNY_OPENCV" = 1 ]; then
        OCV="$(narzedzia/zbuduj_opencv.sh | sed -n 's/^OpenCV_DIR=//p' | tail -1)"
        [ -n "$OCV" ] || { echo "Budowanie OpenCV nie powiodło się." >&2; exit 1; }
        OPCJE=(-DDM_KAMERA=ON -DOpenCV_STATIC=ON "-DOpenCV_DIR=$OCV")
    fi
    cmake -S . -B "$B" -G "$GEN" -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_WX=ON ${OPCJE[@]+"${OPCJE[@]}"}
    cmake --build "$B" -j"$(nproc)"
fi
command -v python3 >/dev/null && python3 narzedzia/zbuduj_pomoc.py >/dev/null || true

NAZWA="Magmass-$WERSJA-win64"
OUT="$B/instalator"
STAGE="$OUT/$NAZWA"
rm -rf "$OUT"; mkdir -p "$STAGE"

install -m 755 "$B/Magmass.exe" "$STAGE/"
strip --strip-unneeded "$STAGE/Magmass.exe" 2>/dev/null || true
cp -r sql "$STAGE/sql"
mkdir -p "$STAGE/pomoc" && cp pomoc/pomoc_pl.htm pomoc/pola.txt "$STAGE/pomoc/"
if   [ -f "$B/pl/LC_MESSAGES/Magmass.mo" ]; then mkdir -p "$STAGE/pl/LC_MESSAGES"; cp "$B/pl/LC_MESSAGES/Magmass.mo" "$STAGE/pl/LC_MESSAGES/"
elif [ -f lang/pl.mo ];                            then mkdir -p "$STAGE/pl/LC_MESSAGES"; cp lang/pl.mo "$STAGE/pl/LC_MESSAGES/Magmass.mo"; fi
for J in en; do if [ -f "$B/$J/LC_MESSAGES/Magmass.mo" ]; then mkdir -p "$STAGE/$J/LC_MESSAGES"; cp "$B/$J/LC_MESSAGES/Magmass.mo" "$STAGE/$J/LC_MESSAGES/"; elif [ -f lang/$J.mo ]; then mkdir -p "$STAGE/$J/LC_MESSAGES"; cp lang/$J.mo "$STAGE/$J/LC_MESSAGES/Magmass.mo"; fi; done
[ -d assets/etykiety ] && { mkdir -p "$STAGE/assets"; cp -r assets/etykiety "$STAGE/assets/"; }
mkdir -p "$STAGE/dokumentacja" && cp -r dokumentacja/. "$STAGE/dokumentacja/" && cp README.md LICENSE "$STAGE/"

# Biblioteki DLL: wszystko, co program (i zależności zależności) bierze z /mingw64 lub /ucrt64
zbierz_dll() {
    ldd "$1" | awk '/=>/ {print $3}' | grep -iE '^/(mingw64|ucrt64|clang64)/' | sort -u | while read -r d; do
        cp -f "$d" "$STAGE/"
    done
}
zbierz_dll "$STAGE/Magmass.exe"
# pdftotext (odczyt parametrów z datasheetów), jeśli zainstalowany: leży obok programu i znajdzie swoje DLL
if PDFT="$(command -v pdftotext.exe 2>/dev/null)"; then
    cp -f "$PDFT" "$STAGE/"; zbierz_dll "$STAGE/pdftotext.exe"
fi
echo "Skopiowano $(ls "$STAGE"/*.dll 2>/dev/null | wc -l) bibliotek DLL"

# Wersja przenośna: ZIP
ZIP="$ROOT/$NAZWA.zip"
rm -f "$ZIP"
if   command -v zip >/dev/null; then ( cd "$OUT" && zip -qr "$ZIP" "$NAZWA" )
elif command -v 7z  >/dev/null; then ( cd "$OUT" && 7z a -tzip -bso0 "$ZIP" "$NAZWA" )
else powershell.exe -NoProfile -Command "Compress-Archive -Path '$(cygpath -w "$STAGE")' -DestinationPath '$(cygpath -w "$ZIP")'"; fi
echo "Gotowe (przenośna): $ZIP"

# Instalator: NSIS
if command -v makensis >/dev/null; then
    IKONA=""
    if command -v magick >/dev/null || command -v convert >/dev/null; then
        IM="magick"; command -v magick >/dev/null || IM="convert"
        if $IM assets/ikony/app_256.png -define icon:auto-resize=256,128,64,48,32,16 "$OUT/magmass.ico" 2>/dev/null; then
            IKONA="$OUT/magmass.ico"
        fi
    fi
    W() { if command -v cygpath >/dev/null; then cygpath -w "$1"; else echo "$1"; fi; }
    EXE="$ROOT/Magmass-$WERSJA-instalator.exe"
    # Tekst do zatwierdzenia w instalatorze: informacja po polsku + oficjalny tekst GPL (UTF-8 z BOM, jak wymaga NSIS Unicode)
    LIC="$OUT/licencja_instalatora.txt"
    { printf '\xEF\xBB\xBF'
      printf 'Magmass (c) 2026 Adam Massalski\r\n\r\n'
      printf 'Program jest wolnym oprogramowaniem na licencji GNU General Public License, wersja 3 lub nowsza.\r\n'
      printf 'Mozesz go rozpowszechniac i zmieniac na warunkach tej licencji. Program jest udostepniany BEZ ZADNEJ GWARANCJI.\r\n'
      printf 'Ponizej oficjalny (angielski) tekst licencji.\r\n\r\n'
      sed 's/\r$//; s/$/\r/' LICENSE; } > "$LIC"
    ARGS=(-V2 "-DLICENCJA=$(W "$LIC")" "-DWERSJA=$WERSJA" "-DSTAGE=$(W "$STAGE")" "-DWYNIK=$(W "$EXE")")
    [ -n "$IKONA" ] && ARGS+=("-DIKONA=$(W "$IKONA")")
    makensis "${ARGS[@]}" "$(W "$ROOT/narzedzia/instalator/windows.nsi")"
    echo "Gotowe (instalator): $EXE"
else
    echo "Brak makensis - pomijam instalator (pacman -S mingw-w64-ucrt-x86_64-nsis). Wersja przenośna jest gotowa."
fi
