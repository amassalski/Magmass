#!/bin/bash
# Pobiera źródła OpenCV, buduje je w minimalnej, STATYCZNEJ wersji (tylko to, czego używa Magmass: core + videoio,
# bez ffmpeg i gstreamera) i wypisuje na końcu linię "OpenCV_DIR=<katalog>" do użycia w CMake.
# Dzięki temu do paczki nie trafiają dziesiątki DLL-i (ffmpeg i reszta) z pakietu OpenCV systemu.
#
#   narzedzia/zbuduj_opencv.sh [katalog_roboczy]       (domyślnie: build-opencv w katalogu projektu)
#
# Działa w MSYS2 MinGW 64-bit, na macOS i na Linuksie. Wymaga: cmake, make albo ninja, kompilatora C/C++, curl, tar.
# Zmienne: OPENCV_WERSJA (domyślnie 4.10.0), OPENCV_SHA256 (opcjonalna suma kontrolna tarballa), OPENCV_URL.
# Wynik jest zapamiętany: kolejne uruchomienia nic nie budują. Pierwsze trwa kilka do kilkunastu minut.
# OpenCV ma licencję Apache 2.0 (zgodną z GPL-3.0-or-later); źródła są pobierane z oficjalnego repozytorium na GitHubie
# i NIE są dołączone do repozytorium Magmass.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WERSJA="${OPENCV_WERSJA:-4.10.0}"
W="${1:-$ROOT/build-opencv}"
mkdir -p "$W"; W="$(cd "$W" && pwd)"   # bez realpath -m: nie ma go w macOS
URL="${OPENCV_URL:-https://github.com/opencv/opencv/archive/refs/tags/$WERSJA.tar.gz}"
SRC="$W/opencv-$WERSJA"
BLD="$W/build-$WERSJA"
INST="$W/install-$WERSJA"
log() { echo "[opencv] $*" >&2; }

# CMake z MSYS2 jest programem Windows: nie rozumie ścieżek w stylu /g/Projekt, tylko G:/Projekt
sciezka_cmake() { if command -v cygpath >/dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi; }

znajdz_konfig() { find "$INST" -name OpenCVConfig.cmake 2>/dev/null | head -1; }

# znacznik wersji ustawień: starsza (niepełna) instalacja bez niego jest budowana od nowa (obiekty zostają w pamięci podręcznej)
ZNACZNIK="$INST/.magmass_ok_2"
if [ -f "$ZNACZNIK" ] && K="$(znajdz_konfig)" && [ -n "$K" ]; then
    log "Gotowe (z pamięci podręcznej): $INST"
    echo "OpenCV_DIR=$(sciezka_cmake "$(dirname "$K")")"; exit 0
fi

mkdir -p "$W"
if [ ! -f "$SRC/CMakeLists.txt" ]; then
    TAR="$W/opencv-$WERSJA.tar.gz"
    if [ ! -f "$TAR" ]; then log "Pobieram $URL"; curl -fL --retry 3 -o "$TAR.part" "$URL"; mv "$TAR.part" "$TAR"; fi
    if [ -n "${OPENCV_SHA256:-}" ]; then
        echo "$OPENCV_SHA256  $TAR" | (sha256sum -c - >&2 2>/dev/null || shasum -a 256 -c - >&2) \
            || { echo "Zła suma kontrolna $TAR" >&2; rm -f "$TAR"; exit 1; }
    fi
    log "Rozpakowuję"
    tar -xzf "$TAR" -C "$W"
fi

GEN=(); case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) GEN=(-G "MSYS Makefiles"); command -v ninja >/dev/null && GEN=(-G Ninja)
                          # MSMF nie kompiluje się kompilatorem GCC z MinGW (cap_msmf.cpp wymaga rozszerzeń MSVC), więc kamera działa przez DirectShow
                          SYS=(-DWITH_MSMF=OFF -DWITH_MSMF_DXVA=OFF -DWITH_DSHOW=ON -DWITH_V4L=OFF) ;;
    Darwin)               SYS=(-DWITH_AVFOUNDATION=ON) ;;
    *)                    command -v ninja >/dev/null && GEN=(-G Ninja); SYS=(-DWITH_V4L=ON) ;;
esac
JOBS="$( (nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) )"

log "Konfiguruję (core, imgproc, imgcodecs, videoio; statycznie, bez ffmpeg)"
cmake -S "$SRC" -B "$BLD" ${GEN[@]+"${GEN[@]}"} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INST" \
    -DBUILD_LIST=core,imgproc,imgcodecs,videoio -DBUILD_SHARED_LIBS=OFF -DBUILD_opencv_world=OFF \
    -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DOCS=OFF -DBUILD_opencv_apps=OFF \
    -DBUILD_JAVA=OFF -DBUILD_opencv_python2=OFF -DBUILD_opencv_python3=OFF -DOPENCV_ENABLE_NONFREE=OFF \
    -DWITH_FFMPEG=OFF -DWITH_GSTREAMER=OFF -DWITH_1394=OFF -DWITH_OPENCL=OFF -DWITH_IPP=OFF -DWITH_ITT=OFF \
    -DWITH_ADE=OFF -DWITH_TBB=OFF -DWITH_EIGEN=OFF -DWITH_LAPACK=OFF -DWITH_PROTOBUF=OFF -DWITH_QUIRC=OFF \
    -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_OPENJPEG=OFF -DWITH_JASPER=OFF -DWITH_OPENEXR=OFF -DWITH_GDAL=OFF \
    -DWITH_PNG=ON -DWITH_JPEG=ON -DBUILD_PNG=ON -DBUILD_JPEG=ON -DBUILD_ZLIB=ON \
    "${SYS[@]}" >&2
log "Buduję ($JOBS wątków)"
cmake --build "$BLD" -j"$JOBS" >&2
cmake --install "$BLD" >&2

K="$(znajdz_konfig)"
[ -n "$K" ] || { echo "Nie znaleziono OpenCVConfig.cmake w $INST" >&2; exit 1; }
touch "$ZNACZNIK"
log "Gotowe: $INST"
echo "OpenCV_DIR=$(sciezka_cmake "$(dirname "$K")")"
