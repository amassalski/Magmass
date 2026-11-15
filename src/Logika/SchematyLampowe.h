#ifndef SCHEMATYLAMPOWE_H
#define SCHEMATYLAMPOWE_H

#include <string>
#include <vector>

/// Schematy układów lampowych jako lista prostych elementów rysunku (bez GUI), współrzędne w polu 260 x 225.
/// Elementy biorące udział w obliczeniach są oznaczone jako użyte (w oknie rysowane wyróżnionym kolorem).
namespace SchematyLampowe {

constexpr int kSzer = 260, kWys = 225;

enum class Typ {
    Linia,          ///< (x1,y1)-(x2,y2)
    RezystorPion,   ///< pionowy rezystor od (x1,y1) do (x1,y2), etykieta po prawej
    KondPoz,        ///< poziomy kondensator od (x1,y1) do (x2,y1), etykieta nad nim
    KondPion,       ///< pionowy kondensator od (x1,y1) do (x1,y2), etykieta po prawej
    Trioda,         ///< środek (x1,y1); wyprowadzenia: anoda (x1,y1-30), siatka (x1-30,y1), katoda (x1,y1+30)
    Masa,           ///< symbol masy w (x1,y1)
    Zasilanie,      ///< szyna zasilania z etykietą (np. +Ua) w (x1,y1)
    Zacisk,         ///< kółko zacisku z etykietą (we, wy)
    Kropka,         ///< węzeł połączenia
    Tekst           ///< etykieta w (x1,y1)
};

struct ElementRysunku {
    Typ typ = Typ::Linia;
    double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    std::string tekst;
    bool uzyty = false;    ///< bierze udział w obliczeniach (wyróżniony)
    bool kreska = false;   ///< linia kreskowa (elementy tylko poglądowe)
};

struct Schemat {
    std::string podpis;               ///< jedna linia pod rysunkiem (np. pojemności międzyelektrodowe)
    std::vector<ElementRysunku> elementy;
    /// Etykiety elementów użytych w obliczeniach (Ra, Rs, ...)
    std::vector<std::string> uzyte() const;
    bool maEtykiete(const std::string& e) const;
};

/// Numery układów jak w oknie: 0 sprzężenie RC, 1 wtórnik anodowy, 2 wtórnik katodowy, 3 inwerter różnicowy,
/// 4 mi-wtórnik, 5 SRPP, 6 wtórnik White'a, 7 kaskodowy wtórnik, 8 superwtórnik. @p automatyczna: polaryzacja RC.
Schemat zbuduj(int uklad, bool automatyczna = true);

} // namespace SchematyLampowe

#endif // SCHEMATYLAMPOWE_H
