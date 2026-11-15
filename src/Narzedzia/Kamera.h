#ifndef KAMERA_H
#define KAMERA_H

#include "../Logika/Termowizja.h"

#include <memory>
#include <string>
#include <vector>

/// Jedna klatka obrazu: piksele RGB, 3 bajty na piksel, wiersz po wierszu od lewej-górnej
struct KlatkaRGB {
    int szer = 0;
    int wys = 0;
    std::vector<unsigned char> rgb;
    bool pusta() const { return szer <= 0 || wys <= 0 || rgb.size() < static_cast<size_t>(szer) * wys * 3; }
};

struct InfoKamery {
    int indeks = 0;
    std::string nazwa;
};

/**
 * @class Kamera
 * @brief Kamera USB (UVC) z podglądem na żywo i zdjęciami.
 *
 * Obsługa urządzeń korzysta z OpenCV (videoio), jeśli program zbudowano z opcją DM_KAMERA. Bez niej
 * obslugiwana() zwraca false, a reszta programu działa normalnie (zdjęcie można wskazać z pliku).
 * Ustawienie zmiennej środowiskowej DM_KAMERA_TEST=1 włącza kamerę testową (generowany obraz): do testów i
 * sprawdzania okna bez urządzenia.
 */
class Kamera {
public:
    Kamera();
    ~Kamera();
    Kamera(const Kamera&) = delete;
    Kamera& operator=(const Kamera&) = delete;

    /// Czy program potrafi w ogóle obsługiwać kamery (zbudowany z OpenCV albo włączona kamera testowa)
    static bool obslugiwana();
    /// Sprawdza kolejne indeksy urządzeń 0...maks-1. Może chwilę potrwać.
    static std::vector<InfoKamery> wykryj(int maks = 8);
    /// Lista kamer do otworzSurowo(): na macOS z AVFoundation (z prawdziwymi nazwami urządzeń), gdzie indziej jak wykryj()
    static std::vector<InfoKamery> wykryjSurowe();
    /// Tekst z ostatniego wykrywania: które indeksy próbowano otworzyć i z jakim wynikiem (do diagnostyki)
    static std::string raportWykrywania();

    bool otworz(int indeks, int szerZadana = 1280, int wysZadana = 720);
    void zamknij();
    bool otwarta() const;
    /// Pobiera bieżącą klatkę. false, gdy kamera nie jest otwarta albo nie dała obrazu.
    bool pobierz(KlatkaRGB& klatka);
    /// Otwiera kamerę bez konwersji do RGB (CAP_PROP_CONVERT_RGB=0): klatki są takie, jak je podaje sterownik.
    /// Potrzebne dla kamer termowizyjnych, które w ramce przesyłają też dane temperatury.
    bool otworzSurowo(int indeks);
    /// Pobiera klatkę w postaci surowej (po otworzSurowo). W trybie testowym: klatka syntetyczna 256x384.
    bool pobierzSurowa(Termowizja::KlatkaSurowa& klatka);
    /// Wysyła do otwartej kamery polecenie UVC „Zoom absolutny” (kamery termowizyjne HT-301/Xtherm przełączają tak tryb:
    /// 0x8004 = dane surowe). komunikat: wynik albo powód porażki. Na macOS idzie bezpośrednio przez USB (IOKit).
    bool wyslijZoom(unsigned wartosc, std::string& komunikat);
    /// Co sterownik raportuje o otwartej kamerze: backend, rozmiar, FPS, FOURCC, czy konwertuje do RGB
    std::string wlasciwosci() const;
    const std::string& blad() const { return blad_; }

private:
    struct Impl;
    std::unique_ptr<Impl> p_;
    std::string blad_;
};

#endif // KAMERA_H
