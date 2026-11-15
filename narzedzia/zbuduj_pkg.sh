#!/bin/bash
# Buduje instalator .pkg dla macOS z gotowego pakietu Magmass.app. Uruchamiany przez zbuduj_macos.sh; można też ręcznie:
#
#   narzedzia/zbuduj_pkg.sh <Magmass.app> <wersja> <arch> <plik_wyjsciowy.pkg>
#
# Instalator pyta (ekran „Dostosuj”), czy dodać Magmass do Docka. Używa wyłącznie narzędzi z macOS: pkgbuild, productbuild.
# Pakiet nie jest podpisany certyfikatem Apple, chyba że ustawisz zmienną PKG_PODPIS na nazwę certyfikatu
# „Developer ID Installer: ...” (wymaga konta Apple Developer); wtedy dodatkowo jest podpisywany (productsign).
set -euo pipefail
[ "$(uname)" = "Darwin" ] || { echo "Ten skrypt działa tylko na macOS." >&2; exit 1; }
[ $# -eq 4 ] || { echo "Użycie: $0 <Magmass.app> <wersja> <arch> <plik.pkg>" >&2; exit 1; }
APP="$1"; WERSJA="$2"; ARCH="$3"; WYNIK="$4"
[ -d "$APP" ] || { echo "Brak $APP" >&2; exit 1; }
command -v pkgbuild >/dev/null && command -v productbuild >/dev/null || { echo "Brak pkgbuild/productbuild (Xcode Command Line Tools)." >&2; exit 1; }
KAT="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$KAT/.." && pwd)"
case "$ARCH" in arm64) HOSTARCH="arm64";; x86_64) HOSTARCH="x86_64";; *) HOSTARCH="x86_64,arm64";; esac

T="$(mktemp -d "${TMPDIR:-/tmp}/magmass-pkg.XXXXXX")"
trap 'rm -rf "$T"' EXIT
mkdir -p "$T/root" "$T/pakiety" "$T/zasoby"
cp -R "$APP" "$T/root/Magmass.app"

# 1. Pakiet z programem: instalacja w /Applications, bez „relokacji” (nie szuka innej kopii na dysku)
pkgbuild --analyze --root "$T/root" "$T/komponenty.plist" >/dev/null
/usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$T/komponenty.plist"
pkgbuild --root "$T/root" --component-plist "$T/komponenty.plist" --install-location /Applications \
    --identifier pl.magmass.app --version "$WERSJA" "$T/pakiety/Magmass-app.pkg" >/dev/null

# 2. Pakiet bez plików, tylko ze skryptem dodającym ikonę do Docka (instalowany, gdy użytkownik zostawi zaznaczone „Dodaj do Docka”)
pkgbuild --nopayload --scripts "$KAT/instalator/macos/skrypty_dock" \
    --identifier pl.magmass.dock --version "$WERSJA" "$T/pakiety/Magmass-dock.pkg" >/dev/null

# 3. Opis instalatora i ekrany
sed -e "s/@WERSJA@/$WERSJA/g" -e "s/@ARCH@/$HOSTARCH/g" "$KAT/instalator/macos/dystrybucja.xml.in" > "$T/dystrybucja.xml"
cp "$KAT"/instalator/macos/zasoby/*.html "$T/zasoby/"
cp "$ROOT/LICENSE" "$T/zasoby/LICENSE.txt"

rm -f "$WYNIK"
if [ -n "${PKG_PODPIS:-}" ]; then
    productbuild --distribution "$T/dystrybucja.xml" --resources "$T/zasoby" --package-path "$T/pakiety" "$T/bez_podpisu.pkg" >/dev/null
    productsign --sign "$PKG_PODPIS" "$T/bez_podpisu.pkg" "$WYNIK" >/dev/null
else
    productbuild --distribution "$T/dystrybucja.xml" --resources "$T/zasoby" --package-path "$T/pakiety" "$WYNIK" >/dev/null
fi
echo "Gotowe (instalator pkg): $WYNIK"
