#include "UstawieniaZebry.h"
#include "Konfiguracja.h"

#include <algorithm>

UstawieniaZebry UstawieniaZebry::wczytaj() {
    Konfiguracja& k = Konfiguracja::pobierz();
    UstawieniaZebry u;
    u.uzywaj = k.pobierzBool("zebra", "uzywaj", false);
    u.wysylka.tryb = WysylkaZPL::trybZNazwy(k.pobierz("zebra", "tryb", "siec"));
    u.wysylka.host = k.pobierz("zebra", "host", "");
    u.wysylka.port = std::max(1, std::min(65535, k.pobierzInt("zebra", "port", 9100)));
    u.wysylka.drukarka = k.pobierz("zebra", "drukarka", "");
    u.wysylka.plik = k.pobierz("zebra", "plik", "");
    // 50 x 80 mm: dłuższy bok w poprzek głowicy (domyślnie 80 szer. x 50 wys.)
    u.etykieta.szerMm = std::max(10, std::min(120, k.pobierzInt("zebra", "szerokosc_mm", 80)));
    u.etykieta.wysMm = std::max(10, std::min(400, k.pobierzInt("zebra", "wysokosc_mm", 50)));
    const int dpi = k.pobierzInt("zebra", "dpi", 203);
    u.etykieta.dpi = (dpi == 300 || dpi == 600) ? dpi : 203;
    u.etykieta.bezPolskichZnakow = k.pobierzBool("zebra", "bez_polskich", true);
    u.etykieta.obrot180 = k.pobierzBool("zebra", "obrot180", false);
    return u;
}

void UstawieniaZebry::zapisz() const {
    Konfiguracja& k = Konfiguracja::pobierz();
    k.ustawBool("zebra", "uzywaj", uzywaj);
    k.ustaw("zebra", "tryb", WysylkaZPL::nazwaTrybu(wysylka.tryb));
    k.ustaw("zebra", "host", wysylka.host);
    k.ustawInt("zebra", "port", wysylka.port);
    k.ustaw("zebra", "drukarka", wysylka.drukarka);
    k.ustaw("zebra", "plik", wysylka.plik);
    k.ustawInt("zebra", "szerokosc_mm", static_cast<int>(etykieta.szerMm + 0.5));
    k.ustawInt("zebra", "wysokosc_mm", static_cast<int>(etykieta.wysMm + 0.5));
    k.ustawInt("zebra", "dpi", etykieta.dpi);
    k.ustawBool("zebra", "bez_polskich", etykieta.bezPolskichZnakow);
    k.ustawBool("zebra", "obrot180", etykieta.obrot180);
    k.zapisz();
}
