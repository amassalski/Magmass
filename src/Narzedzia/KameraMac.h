#ifndef KAMERAMAC_H
#define KAMERAMAC_H

// macOS: bezpośredni odbiór klatek z kamery UVC przez AVFoundation, bez OpenCV.
// OpenCV na macOS nie otwiera niektórych kamer (np. termowizyjnych o nietypowym formacie), a ta wersja prosi
// system o format surowy YUYV i zwraca bajty bez konwersji.

#include "../Logika/Termowizja.h"

#include <memory>
#include <string>
#include <vector>

namespace KameraMac {

/// Nazwy kamer widocznych w systemie (w kolejności indeksów używanej przez Sesja::otworz)
std::vector<std::string> nazwy();

/// Wysyła do kamery o indeksie z nazwy() polecenie UVC SET_CUR na kontrolkę Zoom absolutny terminala kamery
/// (kamery termowizyjne HT-301/Xtherm przełączają tak tryb: 0x8004 = dane surowe). Bezpośrednio przez USB (IOKit),
/// bo AVFoundation nie udostępnia tej kontrolki. komunikat: wynik albo powód porażki (kody IOReturn).
bool wyslijZoomUvc(int indeks, unsigned wartosc, std::string& komunikat);

class Sesja {
public:
    Sesja();
    ~Sesja();
    Sesja(const Sesja&) = delete;
    Sesja& operator=(const Sesja&) = delete;

    /// Otwiera kamerę o indeksie z nazwy(). Przy pierwszym użyciu system pyta o zgodę na dostęp do kamery.
    bool otworz(int indeks, std::string& blad);
    void zamknij();
    bool otwarta() const;
    /// Ostatnia odebrana klatka (surowe bajty, bez konwersji). false, gdy jeszcze żadna nie dotarła.
    bool pobierz(Termowizja::KlatkaSurowa& klatka);
    /// Nazwa urządzenia, lista formatów, wybrany format piksela
    std::string opis() const;

private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};

}  // namespace KameraMac

#endif  // KAMERAMAC_H
