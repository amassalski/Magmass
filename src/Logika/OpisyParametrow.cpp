#include "OpisyParametrow.h"

#include <cctype>
#include <map>
#include <vector>

namespace OpisyParametrow {

std::string klucz(const std::string& nazwa) {
    std::string k;
    for (size_t i = 0; i < nazwa.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(nazwa[i]);
        if (c == ' ' || c == '_' || c == '{' || c == '}' || c == '(' || c == ')' || c == '.' || c == '-') continue;
        // μ (U+03BC) i µ (U+00B5) jako "mu"
        if (c == 0xCE && i + 1 < nazwa.size() && static_cast<unsigned char>(nazwa[i + 1]) == 0xBC) { k += "mu"; ++i; continue; }
        if (c == 0xC2 && i + 1 < nazwa.size() && static_cast<unsigned char>(nazwa[i + 1]) == 0xB5) { k += "mu"; ++i; continue; }
        k += static_cast<char>(std::tolower(c));
    }
    return k;
}

namespace {
std::map<std::string, std::string>& zBazy() { static std::map<std::string, std::string> m; return m; }
bool& czyZBazy() { static bool b = false; return b; }
} // namespace

const std::vector<std::pair<std::string, std::string>>& wbudowane() {
    static const std::vector<std::pair<std::string, std::string>> w = {
            // lampy elektronowe
        {"W_amax", "Maksymalna dopuszczalna moc strat anody [W]"},
        {"P_amax", "Maksymalna dopuszczalna moc strat anody [W]"},
        {"W_g2max", "Maksymalna dopuszczalna moc strat siatki ekranującej g2 [W]"},
        {"P_g2max", "Maksymalna dopuszczalna moc strat siatki ekranującej g2 [W]"},
        {"U_amax", "Maksymalne dopuszczalne napięcie anodowe [V]"},
        {"V_amax", "Maksymalne dopuszczalne napięcie anodowe [V]"},
        {"U_a", "Napięcie anodowe (anoda względem katody) [V]"},
        {"V_a", "Napięcie anodowe (anoda względem katody) [V]"},
        {"U_g2", "Napięcie siatki ekranującej g2 względem katody [V]"},
        {"V_g2", "Napięcie siatki ekranującej g2 względem katody [V]"},
        {"U_g1", "Napięcie siatki sterującej g1 względem katody (ujemne) [V]"},
        {"V_g1", "Napięcie siatki sterującej g1 względem katody (ujemne) [V]"},
        {"I_a", "Prąd anodowy [mA]"},
        {"I_g2", "Prąd siatki ekranującej g2 [mA]"},
        {"I_k", "Prąd katody [mA]"},
        {"I_kmax", "Maksymalny dopuszczalny prąd katody [mA]"},
        {"U_f", "Napięcie żarzenia [V]"},
        {"U_h", "Napięcie żarzenia [V]"},
        {"I_h", "Prąd żarzenia [A]"},
        {"R_i", "Rezystancja wewnętrzna (dynamiczna) lampy [Ω]"},
        {"R_a", "Rezystancja obciążenia w obwodzie anody [Ω]"},
        {"R_aa", "Rezystancja obciążenia anoda-anoda w układzie przeciwsobnym [Ω]"},
        {"g_m", "Nachylenie (transkonduktancja) [mA/V]"},
        {"S", "Nachylenie lampy: zmiana prądu anodowego przypadająca na zmianę napięcia siatki [mA/V]"},
        {"μ", "Współczynnik wzmocnienia napięciowego lampy (bez jednostki)"},
        {"mu", "Współczynnik wzmocnienia napięciowego lampy (bez jednostki)"},
        {"U_hk", "Maksymalne napięcie między żarnikiem a katodą [V]"},
            // tranzystory, diody, układy scalone
        {"V_CE", "Napięcie kolektor-emiter [V]"},
        {"V_CEO", "Maksymalne napięcie kolektor-emiter przy otwartej bazie [V]"},
        {"V_CBO", "Maksymalne napięcie kolektor-baza przy otwartym emiterze [V]"},
        {"V_EBO", "Maksymalne napięcie emiter-baza przy otwartym kolektorze [V]"},
        {"V_CE(sat)", "Napięcie nasycenia kolektor-emiter [V]"},
        {"V_BE", "Napięcie baza-emiter [V]"},
        {"I_C", "Prąd kolektora [A]"},
        {"I_Cmax", "Maksymalny dopuszczalny prąd kolektora [A]"},
        {"P_tot", "Całkowita dopuszczalna moc strat elementu [W]"},
        {"h_FE", "Statyczne wzmocnienie prądowe tranzystora w układzie ze wspólnym emiterem (bez jednostki)"},
        {"f_T", "Częstotliwość graniczna (przy której wzmocnienie prądowe spada do 1) [MHz]"},
        {"V_DS", "Napięcie dren-źródło [V]"},
        {"V_DSS", "Maksymalne napięcie dren-źródło przy zwartej bramce [V]"},
        {"V_GS", "Napięcie bramka-źródło [V]"},
        {"V_GS(th)", "Napięcie progowe bramki: przy nim tranzystor zaczyna przewodzić [V]"},
        {"R_DS(on)", "Rezystancja kanału dren-źródło w stanie przewodzenia [Ω]"},
        {"I_D", "Prąd drenu [A]"},
        {"V_F", "Napięcie przewodzenia diody [V]"},
        {"I_F", "Prąd przewodzenia diody [A]"},
        {"V_R", "Napięcie wsteczne diody [V]"},
        {"V_RRM", "Maksymalne powtarzalne napięcie wsteczne [V]"},
        {"I_R", "Prąd wsteczny diody [µA]"},
        {"T_j", "Temperatura złącza [°C]"},
        {"T_jmax", "Maksymalna dopuszczalna temperatura złącza [°C]"},
        {"R_thJC", "Rezystancja cieplna złącze-obudowa [K/W]"},
        {"R_thJA", "Rezystancja cieplna złącze-otoczenie [K/W]"},
        {"V_CC", "Napięcie zasilania [V]"},
        {"V_DD", "Napięcie zasilania [V]"},
        {"I_Q", "Prąd spoczynkowy pobierany przez układ [mA]"},
        {"GBW", "Iloczyn wzmocnienia i szerokości pasma wzmacniacza [MHz]"},
        {"V_OS", "Napięcie niezrównoważenia wejścia [mV]"},
        {"I_B", "Prąd bazy (dla wzmacniacza operacyjnego: prąd polaryzacji wejścia) [A]"},
        {"SR", "Szybkość narastania napięcia wyjściowego (slew rate) [V/µs]"},
        {"ESR", "Zastępcza rezystancja szeregowa kondensatora [Ω]"},
    };
    return w;
}

void ustawZBazy(const std::map<std::string, std::string>& opisy) {
    zBazy().clear();
    for (const auto& p : opisy) zBazy()[klucz(p.first)] = p.second;
    czyZBazy() = true;
}

void wyczyscZBazy() { zBazy().clear(); czyZBazy() = false; }

namespace {
const std::map<std::string, std::string>& slownik() {
    static const std::map<std::string, std::string> s = [] {
        std::map<std::string, std::string> m;
        for (const auto& p : wbudowane()) m[klucz(p.first)] = p.second;
        return m;
    }();
    return s;
}
} // namespace

std::string zeSlownika(const std::string& nazwa) {
    const std::string k = klucz(nazwa);
    // po wczytaniu słownika z bazy obowiązuje on sam (usunięte w nim symbole nie wracają z wbudowanego)
    const auto& m = czyZBazy() ? zBazy() : slownik();
    const auto it = m.find(k);
    return it == m.end() ? std::string() : it->second;
}

std::vector<std::string> nieznane(const std::vector<std::string>& nazwy) {
    std::vector<std::string> w;
    std::vector<std::string> klucze;
    for (const std::string& n : nazwy) {
        const std::string k = klucz(n);
        if (k.empty() || !zeSlownika(n).empty()) continue;
        bool juz = false;
        for (const std::string& x : klucze) if (x == k) { juz = true; break; }
        if (juz) continue;
        klucze.push_back(k);
        w.push_back(n);
    }
    return w;
}

std::string dlaParametru(const ParametrKatalogu& p) {
    return p.opis.empty() ? zeSlownika(p.nazwa) : p.opis;
}

} // namespace OpisyParametrow
