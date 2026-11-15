# Kopiuje biblioteki DLL (z /mingw64, /ucrt64, /clang64 itd.), których używa program, obok exe.
# Wywołanie: cmake -DEXE=<plik exe> -DDOCEL=<katalog> [-DWZORZEC=<regex ścieżki>] -P KopiujDLL.cmake
# Nie wymaga powłoki ani ldd w PATH; zawsze nadpisuje (stare DLL nie zasłaniają nowych).
if(NOT EXE OR NOT DOCEL)
    message(FATAL_ERROR "KopiujDLL: podaj -DEXE=... i -DDOCEL=...")
endif()
if(NOT WZORZEC)
    set(WZORZEC "/(mingw|ucrt|clang)(32|64|arm64)/")
endif()
# CMake >= 4.x: ścieżki z Windows (C:\WINDOWS\system32) są normalizowane do ukośników / przed
# dopasowaniem wyrażeń; nasze wzorce i tak używają "/", więc wybieramy nowe zachowanie.
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${EXE}"
    RESOLVED_DEPENDENCIES_VAR zaleznosci
    UNRESOLVED_DEPENDENCIES_VAR nierozwiazane
    POST_EXCLUDE_REGEXES "^.*/[Ww]indows/.*$")
set(licznik 0)
foreach(biblioteka IN LISTS zaleznosci)
    string(TOLOWER "${biblioteka}" mala)
    if(mala MATCHES "${WZORZEC}")
        file(COPY "${biblioteka}" DESTINATION "${DOCEL}")
        math(EXPR licznik "${licznik} + 1")
    endif()
endforeach()
message(STATUS "Skopiowano lub zaktualizowano ${licznik} bibliotek DLL")
