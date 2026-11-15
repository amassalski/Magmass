#!/bin/bash
# Buduje wersję dla macOS: pakiet Magmass.app (z dołączonymi bibliotekami), obraz dysku .dmg i instalator .pkg
# (ekran „Dostosuj” pyta, czy dodać program do Docka; opcja --bez-pkg pomija instalator .pkg).
# Uruchom na Macu, z katalogu projektu:
#
#   narzedzia/zbuduj_macos.sh [--bez-bibliotek] [--wlasny-opencv] [--bez-pkg] [katalog_budowania]
#
# Wymaga: Xcode Command Line Tools, Homebrew: cmake wxwidgets postgresql@15 (lub 14) dylibbundler;
# opcjonalnie: opencv (kamera), poppler (pdftotext, odczyt parametrów z datasheetów), gettext (tłumaczenia).
#   brew install cmake wxwidgets postgresql@15 dylibbundler poppler
# MacPorts zamiast Homebrew: sudo port install cmake wxWidgets-3.2 postgresql15 dylibbundler poppler opencv4
#   (CMake szuka OpenCV także w /opt/local; opencv3 też zadziała). Kamera jest opcjonalna: bez OpenCV program się zbuduje.
# --bez-bibliotek: nie dołącza bibliotek Homebrew (aplikacja zadziała tylko na tym Macu, ale nie potrzeba dylibbundler).
# --wlasny-opencv: pobiera źródła OpenCV i buduje minimalną wersję statyczną (narzedzia/zbuduj_opencv.sh) zamiast
#   używać OpenCV z Homebrew/MacPorts; w paczce jest wtedy mniej bibliotek. Wymaga świeżego katalogu budowania.
# Aplikacja jest podpisana "ad hoc" (bez konta Apple Developer), więc po pobraniu z internetu macOS zapyta o zgodę:
# prawy klik na aplikacji -> Otwórz.
set -euo pipefail

BIBLIOTEKI=1; WLASNY_OPENCV=0; PKG=1
while [ $# -gt 0 ]; do
    case "$1" in
        --bez-bibliotek) BIBLIOTEKI=0; shift;;
        --wlasny-opencv) WLASNY_OPENCV=1; shift;;
        --bez-pkg) PKG=0; shift;;
        *) break;;
    esac
done
[ "$(uname)" = "Darwin" ] || { echo "Ten skrypt działa tylko na macOS." >&2; exit 1; }
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
if [ -n "${1:-}" ]; then B="$(python3 -c 'import os,sys;print(os.path.abspath(sys.argv[1]))' "$1")"; else B="$ROOT/build-mac"; fi
WERSJA="$(sed -n 's/^project(Magmass VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt | head -1)"
ARCH="$(uname -m)"
[ -n "$WERSJA" ] || { echo "Nie znaleziono wersji w CMakeLists.txt" >&2; exit 1; }
if [ "$BIBLIOTEKI" = 1 ] && ! command -v dylibbundler >/dev/null; then
    echo "Brak dylibbundler (brew install dylibbundler) - albo użyj opcji --bez-bibliotek." >&2; exit 1
fi

# Opcja --wlasny-opencv ma sens tylko przy konfiguracji od zera: gotowy katalog budowania z programem zbudowanym z OpenCV
# z systemu skrypt wcześniej po cichu pomijał (bez żadnego komunikatu), więc opcja "nie działała". Teraz go czyścimy.
if [ "$WLASNY_OPENCV" = 1 ] && [ -x "$B/Magmass" ] && ! grep -q "^OpenCV_STATIC:BOOL=ON" "$B/CMakeCache.txt" 2>/dev/null; then
    echo "Katalog budowania $B zawiera program zbudowany bez własnego OpenCV: budujemy od nowa." >&2
    rm -rf "$B"
fi

if [ ! -x "$B/Magmass" ]; then
    OPCJE=()
    if [ "$WLASNY_OPENCV" = 1 ]; then
        OCV="$(narzedzia/zbuduj_opencv.sh | sed -n 's/^OpenCV_DIR=//p' | tail -1)"
        [ -n "$OCV" ] || { echo "Budowanie OpenCV nie powiodło się." >&2; exit 1; }
        OPCJE=(-DDM_KAMERA=ON -DOpenCV_STATIC=ON "-DOpenCV_DIR=$OCV")
    fi
    cmake -S . -B "$B" -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_WX=ON ${OPCJE[@]+"${OPCJE[@]}"}
    cmake --build "$B" -j"$(sysctl -n hw.ncpu)"
fi
command -v python3 >/dev/null && python3 narzedzia/zbuduj_pomoc.py >/dev/null || true

OUT="$B/instalator"
APP="$OUT/Magmass.app"
rm -rf "$OUT"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" "$APP/Contents/Frameworks"

install -m 755 "$B/Magmass" "$APP/Contents/MacOS/Magmass"
R="$APP/Contents/Resources"
cp -r sql "$R/sql"
cp LICENSE "$R/LICENSE"
mkdir -p "$R/pomoc" && cp pomoc/pomoc_pl.htm pomoc/pola.txt "$R/pomoc/"
if   [ -f "$B/pl/LC_MESSAGES/Magmass.mo" ]; then mkdir -p "$R/pl/LC_MESSAGES"; cp "$B/pl/LC_MESSAGES/Magmass.mo" "$R/pl/LC_MESSAGES/"
elif [ -f lang/pl.mo ];                            then mkdir -p "$R/pl/LC_MESSAGES"; cp lang/pl.mo "$R/pl/LC_MESSAGES/Magmass.mo"; fi
for J in en; do if [ -f "$B/$J/LC_MESSAGES/Magmass.mo" ]; then mkdir -p "$R/$J/LC_MESSAGES"; cp "$B/$J/LC_MESSAGES/Magmass.mo" "$R/$J/LC_MESSAGES/"; elif [ -f lang/$J.mo ]; then mkdir -p "$R/$J/LC_MESSAGES"; cp lang/$J.mo "$R/$J/LC_MESSAGES/Magmass.mo"; fi; done
[ -d assets/etykiety ] && { mkdir -p "$R/assets"; cp -r assets/etykiety "$R/assets/"; }

# Ikona: .icns z PNG (sips i iconutil są w systemie)
ZESTAW="$OUT/AppIcon.iconset"; mkdir -p "$ZESTAW"
for r in 16 32 128 256 512; do
    sips -z $r $r assets/ikony/app_256.png --out "$ZESTAW/icon_${r}x${r}.png" >/dev/null
    sips -z $((r*2)) $((r*2)) assets/ikony/app_256.png --out "$ZESTAW/icon_${r}x${r}@2x.png" >/dev/null
done
iconutil -c icns "$ZESTAW" -o "$R/AppIcon.icns"

cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key><string>Magmass</string>
    <key>CFBundleDisplayName</key><string>Magmass</string>
    <key>CFBundleIdentifier</key><string>pl.magmass.app</string>
    <key>CFBundleExecutable</key><string>Magmass</string>
    <key>CFBundleIconFile</key><string>AppIcon</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>$WERSJA</string>
    <key>CFBundleVersion</key><string>$WERSJA</string>
    <key>CFBundleDevelopmentRegion</key><string>pl</string>
    <key>LSMinimumSystemVersion</key><string>10.15</string>
    <key>LSApplicationCategoryType</key><string>public.app-category.utilities</string>
    <key>NSHighResolutionCapable</key><true/>
    <key>NSCameraUsageDescription</key><string>Kamera służy do robienia zdjęć elementów i czytania kodów kreskowych.</string>
</dict>
</plist>
PLIST
plutil -lint "$APP/Contents/Info.plist" >/dev/null

# pdftotext (opcjonalnie): obok programu, żeby odczyt datasheetów działał bez Homebrew
if PDFT="$(command -v pdftotext 2>/dev/null)"; then
    cp -f "$PDFT" "$APP/Contents/MacOS/pdftotext"
    [ "$BIBLIOTEKI" = 1 ] && dylibbundler -of -b -cd -x "$APP/Contents/MacOS/pdftotext" \
        -d "$APP/Contents/Frameworks" -p @executable_path/../Frameworks/ >/dev/null || true
fi

if [ "$BIBLIOTEKI" = 1 ]; then
    dylibbundler -of -b -cd -x "$APP/Contents/MacOS/Magmass" \
        -d "$APP/Contents/Frameworks" -p @executable_path/../Frameworks/ >/dev/null
    echo "Dołączono $(ls "$APP/Contents/Frameworks" | wc -l | tr -d ' ') bibliotek"
fi

# Podpis "ad hoc": bez niego aplikacje na Apple Silicon w ogóle się nie uruchomią
codesign --force --deep --sign - "$APP"

# Obraz dysku
DMG="$ROOT/Magmass-$WERSJA-macos-$ARCH.dmg"
rm -f "$DMG"
DYSK="$OUT/dysk"; mkdir -p "$DYSK"
cp -R "$APP" "$DYSK/"; cp LICENSE "$DYSK/LICENSE.txt"; ln -s /Applications "$DYSK/Applications"
hdiutil create -volname "Magmass" -srcfolder "$DYSK" -ov -format UDZO "$DMG" >/dev/null
echo "Gotowe: $DMG"

# Instalator .pkg (z pytaniem o dodanie do Docka). Pomijany, gdy brak narzędzi lub --bez-pkg.
if [ "$PKG" = 1 ] && command -v pkgbuild >/dev/null && command -v productbuild >/dev/null; then
    narzedzia/zbuduj_pkg.sh "$APP" "$WERSJA" "$ARCH" "$ROOT/Magmass-$WERSJA-macos-$ARCH.pkg"
fi
