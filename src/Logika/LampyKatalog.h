#ifndef LAMPYKATALOG_H
#define LAMPYKATALOG_H

#include "../Model/Katalog.h"

#include <optional>
#include <string>
#include <vector>

/// Lampy elektronowe w zwykłym katalogu elementów: pozycja z rodziny „Lampy elektronowe” i parametrami o ustalonych nazwach.
/// Nie ma osobnej struktury w bazie; wszystko przechodzi przez tbl_katalog, import/eksport CSV i okno katalogu.
namespace LampyKatalog {

constexpr const char* kRodzina = "Lampy elektronowe";
constexpr const char* kParTyp = "Typ lampy";
constexpr const char* kParSa = "Sa [mA/V]";
constexpr const char* kParKa = "Ka [V/V]";
constexpr const char* kParRwew = "Rwew [kΩ]";
constexpr const char* kParCak = "Cak [pF]";
constexpr const char* kParCas = "Cas [pF]";
constexpr const char* kParCsk = "Csk [pF]";
constexpr const char* kParPaMax = "Pa max [W]";
constexpr const char* kParRaaPP = "Raa PP [Ω]";   ///< typowa impedancja anoda-anoda pary lamp w stopniu przeciwsobnym
constexpr const char* kParIa0 = "Ia0 PP [mA]";   ///< typowy prąd spoczynkowy jednej lampy w stopniu przeciwsobnym

/// Dane małosygnałowe lampy (trioda; dla pentody nachylenie i współczynnik względem siatki 1)
struct DaneLampy {
    double sa = 0, ka = 0, rwew = 0;                  ///< mA/V, V/V, kΩ
    std::optional<double> cak, cas, csk;              ///< pF
    std::optional<double> paMaxW;                     ///< dopuszczalna moc strat anody (jednej sekcji) [W]
    std::optional<double> raaPPOm;                    ///< typowa impedancja Raa pary lamp w PP [Ω] (Transformator PP)
    std::optional<double> ia0Ma;                      ///< typowy prąd spoczynkowy lampy w stopniu PP [mA] (Transformator PP)
    std::string typ = "trioda";
};

/// Dane z pozycji katalogu; nullopt, gdy brak Ka albo Rwew albo Sa nie da się ustalić.
/// Brakujące Sa liczone jest z Ka / Rwew (Ka = Sa·Rwew), brakujące Rwew z Ka / Sa.
std::optional<DaneLampy> zPozycji(const PozycjaKatalogu& p);
/// Czy pozycja wygląda na lampę (rodzina albo parametry Ka oraz Rwew albo Sa)
bool czyLampa(const PozycjaKatalogu& p);
/// Parametry katalogowe dla danych lampy (liczby po polsku, bez zbędnych zer)
std::vector<ParametrKatalogu> parametry(const DaneLampy& d, const std::string& zrodlo = "reczny");
/// Pozycja katalogu dla lampy
PozycjaKatalogu pozycja(const std::string& symbol, const DaneLampy& d, const std::string& opis = "",
                        const std::string& uwagi = "");
/// Lampy mocy do transformatora PP (EL34, 6P14P, EL84, 6V6, 6L6, ECL86, ECL82, EBL21): Sa, Rwew pentody, typowy prąd spoczynkowy Ia0 i impedancja Raa pary.
/// Wartości orientacyjne z kart katalogowych (część do weryfikacji, patrz uwagi pozycji); Ka = Sa·Rwew.
std::vector<PozycjaKatalogu> przykladoweMocy();
/// Lampy przykładowe (ECC81, ECC82, ECC83, ECC85, ECC88) z przykładów ECCLab; do dodania jednym kliknięciem
std::vector<PozycjaKatalogu> przykladowe();

} // namespace LampyKatalog

#endif // LAMPYKATALOG_H
