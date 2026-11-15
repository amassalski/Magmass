#ifndef WYSYLKAZPL_H
#define WYSYLKAZPL_H

#include <string>
#include <vector>

/**
 * Wysyłanie poleceń ZPL do drukarki Zebra (np. ZD421):
 *  - Sieć: surowe połączenie TCP z portem 9100 (Ethernet / Wi-Fi),
 *  - System: kolejka drukarki zainstalowanej w systemie, dane "RAW" bez sterownika graficznego
 *            (Windows: WritePrinter z typem RAW; macOS i Linux: lp -o raw; ścieżka /dev/usb/lp0 jest zapisywana wprost),
 *  - Plik: zapis do pliku .zpl (podgląd, kopiowanie na pendrive, wysyłka innym programem).
 */
namespace WysylkaZPL {

enum class Tryb { Siec, System, Plik };

struct Ustawienia {
    Tryb tryb = Tryb::Siec;
    std::string host;           ///< adres IP albo nazwa drukarki w sieci
    int port = 9100;
    std::string drukarka;       ///< nazwa kolejki w systemie albo ścieżka urządzenia (/dev/usb/lp0)
    std::string plik;           ///< ścieżka pliku .zpl
    int limitSekund = 5;
};

std::string nazwaTrybu(Tryb t);          ///< "siec", "system", "plik"
Tryb trybZNazwy(const std::string& s);   ///< nieznany tekst daje Siec

bool przezSiec(const std::string& host, int port, const std::string& dane, int limitSekund, std::string* blad);
bool przezSystem(const std::string& drukarka, const std::string& dane, std::string* blad);
bool doPliku(const std::string& sciezka, const std::string& dane, std::string* blad);

/// Wysyła według ustawień; przy błędzie zwraca false i opis po polsku
bool wyslij(const Ustawienia& u, const std::string& dane, std::string* blad);

/// Nazwy drukarek zainstalowanych w systemie (pusta lista, gdy nie da się ustalić)
std::vector<std::string> listaDrukarek();

}  // namespace WysylkaZPL

#endif  // WYSYLKAZPL_H
