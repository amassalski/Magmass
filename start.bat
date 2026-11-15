@echo off
echo Uruchamianie Magmass...
if not exist build (
    echo Tworzenie katalogu build...
    mkdir build
)
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
Magmass.exe
