#include "ImportLBR.h"
#include "MiniXML.h"
#include "../Narzedzia/Liczby.h"

#include <cctype>
#include <cstdlib>
#include <set>

namespace ImportLBR {

namespace {

const MiniXML::Wezel* znajdzPotomka(const MiniXML::Wezel* w, const std::string& nazwa) {
    if (!w) return nullptr;
    if (auto d = w->dziecko(nazwa)) return d;
    for (const auto& dziecko : w->dzieci)
        if (auto r = znajdzPotomka(dziecko.get(), nazwa)) return r;
    return nullptr;
}

// Liczba przed jednostką, np. "5W", "2,5 W", "250V"; pierwsze trafienie
std::optional<double> liczbaPrzedJednostka(const std::string& t, char jednostka) {
    for (size_t i = 0; i < t.size(); ++i) {
        if (t[i] != jednostka) continue;
        const bool wolneZPrawej = i + 1 >= t.size() || !std::isalnum(static_cast<unsigned char>(t[i + 1]));
        if (!wolneZPrawej) continue;
        size_t j = i;
        while (j > 0 && t[j - 1] == ' ') --j;
        size_t k = j;
        while (k > 0 && (std::isdigit(static_cast<unsigned char>(t[k - 1])) || t[k - 1] == '.' || t[k - 1] == ',')) --k;
        if (k == j) continue;
        // przed liczbą nie może być litery (np. "TO3W"), cyfra z kodu modelu itp.
        if (k > 0 && std::isalpha(static_cast<unsigned char>(t[k - 1]))) continue;
        std::string s = t.substr(k, j - k);
        for (char& c : s) if (c == ',') c = '.';
        char* koniec = nullptr;
        const double v = strtodKlasyczny(s.c_str(), &koniec);
        if (koniec && *koniec == 0 && v > 0) return v;
    }
    return std::nullopt;
}

std::string rozstawZOpisu(const std::string& t) {
    const size_t p = t.find("grid ");
    if (p == std::string::npos) return "";
    size_t i = p + 5, k = i;
    while (k < t.size() && (std::isdigit(static_cast<unsigned char>(t[k])) || t[k] == '.' || t[k] == ',')) ++k;
    if (k == i) return "";
    std::string liczba = t.substr(i, k - i);
    size_t u = k;
    while (u < t.size() && t[u] == ' ') ++u;
    std::string jedn;
    if (t.compare(u, 2, "mm") == 0) jedn = " mm";
    else if (t.compare(u, 3, "mil") == 0) jedn = " mil";
    return liczba + jedn;
}

}  // namespace

std::string opisBezHTML(const std::string& html) {
    std::string wynik;
    bool wZnaczniku = false;
    for (char c : html) {
        if (c == '<') { wZnaczniku = true; wynik += ' '; continue; }
        if (c == '>') { wZnaczniku = false; continue; }
        if (!wZnaczniku) wynik += (c == '\n' || c == '\r' || c == '\t') ? ' ' : c;
    }
    std::string zwiniety;
    bool spacja = true;
    for (char c : wynik) {
        if (c == ' ') { if (!spacja) zwiniety += ' '; spacja = true; }
        else { zwiniety += c; spacja = false; }
    }
    while (!zwiniety.empty() && zwiniety.back() == ' ') zwiniety.pop_back();
    return zwiniety;
}

void rozlozNazwe(const std::string& nazwaPelna, std::string& rodzina, std::string& wariant) {
    rodzina = nazwaPelna;
    wariant.clear();
    std::string n = nazwaPelna;
    // "TRANSISTOR-NPN_TO18-" -> "TO18-": liczy się to, co po ostatnim '_' (o ile dalej jest rodzina z cyframi)
    const size_t pod = n.rfind('_');
    if (pod != std::string::npos && pod + 1 < n.size()) n = n.substr(pod + 1);
    static const char* const RODZINY[] = {"TOP", "TO", "SOT", "SC", "DO"};
    for (const char* r : RODZINY) {
        const size_t d = std::char_traits<char>::length(r);
        if (n.compare(0, d, r) != 0) continue;
        size_t i = d;
        if (i < n.size() && n[i] == '-') ++i;
        size_t k = i;
        while (k < n.size() && std::isdigit(static_cast<unsigned char>(n[k]))) ++k;
        if (k == i) continue;
        rodzina = std::string(r) + "-" + n.substr(i, k - i);
        std::string reszta = n.substr(k);
        while (!reszta.empty() && (reszta[0] == '-' || reszta[0] == '/' || reszta[0] == '_')) reszta.erase(0, 1);
        while (!reszta.empty() && (reszta.back() == '-' || reszta.back() == '/' || reszta.back() == '_')) reszta.pop_back();
        wariant = reszta;
        return;
    }
}

std::optional<Biblioteka> parsuj(const std::string& xml, std::string& blad) {
    auto korzen = MiniXML::parsuj(xml, blad);
    if (!korzen) return std::nullopt;
    const MiniXML::Wezel* lib = korzen->nazwa == "library" ? korzen.get() : znajdzPotomka(korzen.get(), "library");
    if (!lib) { blad = "To nie jest biblioteka Eagle (brak elementu <library>)."; return std::nullopt; }

    Biblioteka b;
    if (auto d = lib->dziecko("description")) b.opis = opisBezHTML(d->tekst);
    b.nazwa = opisBezHTML(b.opis).substr(0, 80);

    if (const auto* pakiety = lib->dziecko("packages")) {
        for (const auto* p : pakiety->dzieciONazwie("package")) {
            Pakiet pk;
            pk.nazwa = p->atrybut("name");
            if (pk.nazwa.empty()) continue;
            if (auto d = p->dziecko("description")) pk.opis = opisBezHTML(d->tekst);
            int pady = 0, smd = 0;
            std::set<std::string> nazwyPinow;
            for (const auto& dz : p->dzieci) {
                if (dz->nazwa == "pad") { ++pady; nazwyPinow.insert(dz->atrybut("name")); }
                else if (dz->nazwa == "smd") { ++smd; nazwyPinow.insert(dz->atrybut("name")); }
            }
            pk.montaz = pady ? "THT" : (smd ? "SMD" : "");
            pk.liczbaPinow = static_cast<int>(nazwyPinow.size());
            pk.mocW = liczbaPrzedJednostka(pk.opis, 'W');
            pk.napiecieV = liczbaPrzedJednostka(pk.opis, 'V');
            pk.rozstaw = rozstawZOpisu(pk.opis);
            rozlozNazwe(pk.nazwa, pk.rodzina, pk.wariant);
            b.pakiety.push_back(std::move(pk));
        }
    }
    if (const auto* zestawy = lib->dziecko("devicesets")) {
        for (const auto* z : zestawy->dzieciONazwie("deviceset")) {
            const std::string opis = z->dziecko("description") ? opisBezHTML(z->dziecko("description")->tekst) : "";
            const auto* urz = z->dziecko("devices");
            if (!urz) continue;
            for (const auto* d : urz->dzieciONazwie("device")) {
                Urzadzenie u;
                u.zestaw = z->atrybut("name");
                u.opisZestawu = opis;
                u.prefiks = z->atrybut("prefix");
                u.wariant = d->atrybut("name");
                u.pakiet = d->atrybut("package");
                if (!u.pakiet.empty()) b.urzadzenia.push_back(std::move(u));
            }
        }
    }
    return b;
}

} // namespace ImportLBR
