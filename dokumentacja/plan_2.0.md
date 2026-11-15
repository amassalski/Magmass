# Plan wersji 2.0 (stan na 2026-11-02)

Zasady: etapami, każdy etap ma testy i trafia do paczki osobno; obliczenia sprawdzane niezależną drogą (inny wzór albo rozwiązanie układu równań), nie kopiowane z cudzych programów. Własny katalog lamp w katalogu elementów Magmass.

| Etap | Zakres | Stan |
|---|---|---|
| 1 | Drukarka Zebra ZD421: etykiety ZPL (sieć 9100, USB/system, plik) | zrobione, do sprawdzenia na drukarce |
| 1b | Czytnik Zebra CS6080: tryb klawiatury (HID) przez pole szukania | działa bez zmian |
| 2 | Okno „Narzędzia audio i lampowe” (Ctrl+Shift+L): ECCkalkulator (dB, nepery), krzywa RIAA, sieć RC | zrobione |
| 3 | Układy lampowe: sprzężenie RC, wtórnik, mi-wtórnik, superwtórnik, kaskoda, katodyna, inwerter, obciążenie aktywne, tryb triodowy pentody, układ o dużej amplitudzie, kompensacja małych częstotliwości (Sa, Ka, Rwew; oporność wyjściowa, wzmocnienie, pasmo) | w toku: sprzężenie RC, wtórniki, inwerter gotowe; reszta układów oraz wykres pasma dalej |
| 4 | Lampy w katalogu: rodzina „Lampy” z parametrami (Sa, Ka, Ri, Pa max, Ua max...), wybór lampy w układach | gotowe: rodzina „Lampy elektronowe”, parametry Sa, Ka, Rwew, Cak, Cas, Csk; Pa max, Ua max dojdą przy metodach graficznych |
| 5 | Metody graficzne: obraz charakterystyk, skalowanie osi, prosta obciążenia, hiperbola Pa max, Ra klasy A i B, moce SE i PP | |
| 6 | Elementy indukcyjne (menu Narzędzia / Elementy indukcyjne) | gotowe: dławik zasilacza, transformator sieciowy (Krize), transformator głośnikowy PP (Etapy 1-3); transformator głośnikowy SE (parametry, zwoje, drut, szczelina i uzwojenia wykonane: okno, rozproszenie, rezystancje, sprawność), dopracowanie Etapu 3 po porównaniu z pomiarem lub drugim przykładem |
| 7 (wersja 2.5) | Menu Kamera: kamera termowizyjna InfiRay T3 Pro (USB): podgląd, palety, min/max/środek, diagnostyka danych | pierwszy krok: podgląd i diagnostyka; temperatury po weryfikacji formatu na kamerze |

Zasada obliczeń: literatura i ECCLab są punktem odniesienia do porównania, nie źródłem prawdy; tam, gdzie wzór da się wyprowadzić z fizyki (szczelina dławika, rozproszenie, długość zwoju), program liczy fizycznie i pokazuje wynik wg literatury/ECCLab obok.

Do ustalenia z użytkownikiem: przykłady (dane wejściowe i wyniki) z ECCLab dla układów z etapu 3; zrzuty okna Elementów indukcyjnych; czy wyniki mają się zapisywać do projektu lub uwag elementu.
