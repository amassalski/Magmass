#!/bin/bash
# Odświeża katalog tłumaczeń: wyciąga napisy z TR("...") i scala z lang/en.po (msgid są po polsku).
# Użycie: ./create_translations.sh   (z katalogu głównego projektu)
set -e
cd "$(dirname "$0")"
mkdir -p lang

find src -name "*.cpp" -o -name "*.h" | sort | xgettext \
    --language=C++ --keyword=TR --from-code=UTF-8 \
    --package-name=Magmass --msgid-bugs-address="" \
    --output=lang/Magmass.pot --files-from=-

for jez in en; do
    if [ -f lang/$jez.po ]; then
        msgmerge --update --backup=none lang/$jez.po lang/Magmass.pot
    else
        msginit --no-translator --locale=$jez --input=lang/Magmass.pot --output=lang/$jez.po
    fi
    msgfmt -c --statistics -o lang/$jez.mo lang/$jez.po
done
echo "Tłumaczenia gotowe (CMake kompiluje je samo przy budowaniu)."
