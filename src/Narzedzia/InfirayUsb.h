#ifndef INFIRAYUSB_H
#define INFIRAYUSB_H

// Opcjonalna obsługa kamer termowizyjnych IRay/InfiRay przez ich bibliotekę USBSDK.dll.
//
// Biblioteki ani nagłówków producenta NIE dołączamy do programu (ich licencja nie pozwala na redystrybucję): użytkownik
// sam pobiera SDK od IRay Technology lub dostawcy modułu i wskazuje plik USBSDK.dll. Program ładuje go w czasie
// działania (LoadLibrary/GetProcAddress), więc nie zawiera kodu producenta i działa też bez SDK.
// Deklaracje funkcji i struktur poniżej są własne, napisane na podstawie publicznie opisanych nazw eksportów
// i układu struktur; format danych w buforach klatek (patrz ostatniaTemp) nie jest w dokumentacji producenta
// opisany jednoznacznie, więc program podaje je surowe, bez przeliczenia na °C.
//
// Biblioteka jest tylko dla Windows (x64 dla 64-bitowego programu); na innych systemach zaladuj() zwraca błąd.
// Bez zależności od wxWidgets.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace InfirayUsb {

struct Urzadzenie {
    int id = 0;
    std::string nazwa;
};

struct Klatka {
    int szer = 0, wys = 0;
    std::vector<unsigned char> dane;   ///< szer·wys·2 bajty, tak jak je przekazała biblioteka
    unsigned numer = 0;                ///< rośnie z każdą nową klatką
    bool pusta() const { return szer <= 0 || wys <= 0 || dane.empty(); }
};

/// 16-bitowe słowa little endian z bufora (szer·wys liczb); puste, gdy klatka pusta
std::vector<uint16_t> slowa16(const Klatka& k);

/// Jak biblioteka opisuje typ rdzenia (CoreType): 0 błąd odczytu, 1 LT pomiar temperatury, 2 MicroIII pomiar, 3 MicroIII obraz, 4 AT200F, 5 AT21F, 6 inny
std::string opisTypuRdzenia(int typ);

class Sdk {
public:
    Sdk();
    ~Sdk();
    Sdk(const Sdk&) = delete;
    Sdk& operator=(const Sdk&) = delete;

    /// Ładuje bibliotekę z podanego pliku (ścieżka UTF-8). Zwraca false i opis błędu (po polsku) przy niepowodzeniu.
    bool zaladuj(const std::string& sciezka, std::string& blad);
    bool zaladowana() const;
    void zwolnij();   ///< zamyka urządzenie i zwalnia bibliotekę

    /// Szuka urządzeń; puste przy braku lub błędzie. Porty szeregowe w osobnej liście.
    std::vector<Urzadzenie> szukaj(std::vector<std::string>* porty = nullptr);
    /// Otwiera urządzenie i rejestruje odbiór klatek. okno: uchwyt HWND okna (może być nullptr).
    bool otworz(int idUrzadzenia, int indeksPortu, void* okno, std::string& blad);
    void zamknij();
    bool otwarte() const;

    int typRdzenia();   ///< 0 przy błędzie
    bool numeryPNSN(std::string& pn, std::string& sn);
    bool wymiary(int& szer, int& wys);
    bool temperaturaFPA(float& t);
    bool temperaturaKamery(float& t);
    bool migawka(int typRdzenia);   ///< kalibracja migawką (shutter), 0 = sukces w bibliotece

    /// Ostatnie odebrane klatki (kopie); false, gdy jeszcze żadnej nie było
    bool ostatniObraz(Klatka& k) const;   ///< obraz widzialny, 2 bajty na piksel (UYVY/YUYV zależnie od rdzenia)
    bool ostatniaTemp(Klatka& k) const;   ///< dane temperaturowe, 2 bajty na piksel; skala nieudokumentowana: kalibrować pomiarem

private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};

}  // namespace InfirayUsb

#endif  // INFIRAYUSB_H
