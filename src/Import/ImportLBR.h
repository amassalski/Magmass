#ifndef IMPORTLBR_H
#define IMPORTLBR_H

#include <optional>
#include <string>
#include <vector>

/**
 * Biblioteka Eagle (.lbr, XML): obudowy (packages) z opisami oraz zestawy (devicesets) z wariantami,
 * czyli informacją, jaki element w jakiej obudowie występuje.
 *
 * Nazwy pakietów Eagle kodują rodzinę i wariant: TO220AH = TO-220, wariant AH (A, poziomo),
 * TO92-BCE = TO-92, wariant BCE (kolejność wyprowadzeń), SOT23-EBC = SOT-23, wariant EBC.
 */
namespace ImportLBR {

struct Pakiet {
    std::string nazwa;              ///< nazwa w Eagle, np. "TO220AH"
    std::string rodzina;            ///< np. "TO-220" (gdy nazwy nie da się rozłożyć: taka sama jak nazwa)
    std::string wariant;            ///< np. "AH"; pusty, gdy brak
    std::string opis;               ///< opis z biblioteki bez znaczników HTML
    std::string montaz;             ///< "THT", "SMD" albo pusty
    int liczbaPinow = 0;
    std::optional<double> mocW;         ///< z opisu, np. "5W"
    std::optional<double> napiecieV;    ///< z opisu, np. "250V"
    std::string rozstaw;            ///< z opisu, np. "grid 5,08 mm" -> "5,08 mm"
};

struct Urzadzenie {
    std::string zestaw;             ///< nazwa zestawu (symbol elementu, np. "BD437" albo "2N2222*")
    std::string opisZestawu;        ///< bez znaczników HTML
    std::string prefiks;            ///< np. "Q"
    std::string wariant;            ///< nazwa urządzenia (przyrostek), często pusty
    std::string pakiet;             ///< nazwa pakietu
};

struct Biblioteka {
    std::string nazwa;
    std::string opis;
    std::vector<Pakiet> pakiety;
    std::vector<Urzadzenie> urzadzenia;
};

/// Rozkłada nazwę pakietu Eagle na rodzinę i wariant ("TO220AH" -> "TO-220", "AH")
void rozlozNazwe(const std::string& nazwa, std::string& rodzina, std::string& wariant);
/// Usuwa znaczniki HTML i zamienia encje; zwija białe znaki
std::string opisBezHTML(const std::string& html);

std::optional<Biblioteka> parsuj(const std::string& xml, std::string& blad);

} // namespace ImportLBR

#endif // IMPORTLBR_H
