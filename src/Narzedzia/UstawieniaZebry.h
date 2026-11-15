#ifndef USTAWIENIAZEBRY_H
#define USTAWIENIAZEBRY_H

#include "../Eksport/EtykietyZPL.h"
#include "WysylkaZPL.h"

/// Ustawienia drukarki Zebra zapisane w config.ini (sekcja "zebra")
struct UstawieniaZebry {
    EtykietyZPL::Ustawienia etykieta;
    WysylkaZPL::Ustawienia wysylka;
    bool uzywaj = false;   ///< w oknie etykiet wybrany jest druk na Zebrze

    static UstawieniaZebry wczytaj();
    void zapisz() const;
};

#endif  // USTAWIENIAZEBRY_H
