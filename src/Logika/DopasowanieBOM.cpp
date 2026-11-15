#include "DopasowanieBOM.h"

#include <algorithm>
#include <cctype>

namespace DopasowanieBOM {

namespace {

/// Postać do porównań symboli: bez spacji i myślników, małe litery
std::string kluczSymbolu(const std::string& s) {
    std::string w;
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c)) || static_cast<unsigned char>(c) >= 0x80)
            w += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return w;
}

/// Obudowy z pola "Obudowa" elementu (rozdzielone przecinkiem lub średnikiem), po normalizacji
std::vector<std::string> podzielObudowy(const std::string& tekst) {
    std::vector<std::string> wynik;
    std::string biezacy;
    auto zapisz = [&]() {
        const std::string n = ImportBOM::normalizujObudowe(biezacy);
        if (!n.empty()) wynik.push_back(n);
        biezacy.clear();
    };
    for (char c : tekst) {
        if (c == ',' || c == ';') zapisz(); else biezacy += c;
    }
    zapisz();
    return wynik;
}

} // namespace

std::vector<KandydatElementu> znajdz(const PozycjaBOM& p, const std::vector<Element>& magazyn,
                                     size_t maksymalnie) {
    const std::string mpn = kluczSymbolu(p.symbol);
    const std::string wartosc = ImportBOM::normalizujWartosc(p.wartosc);
    const std::string wartoscSymbol = kluczSymbolu(p.wartosc);
    const std::string obudowa = ImportBOM::normalizujObudowe(p.obudowa);

    struct Wpis {
        KandydatElementu k;
        int stan;
    };
    std::vector<Wpis> wpisy;

    for (const Element& e : magazyn) {
        if (!e.aktywny) continue;
        const std::string sym = kluczSymbolu(e.symbol);
        const std::string symProd = kluczSymbolu(e.symbolProducenta);

        KandydatElementu k;
        k.idElementu = e.id;

        if (!mpn.empty() && (mpn == sym || mpn == symProd)) {
            k.punkty = 100;
            k.powod = "ten sam symbol";
        } else if (wartoscSymbol.size() >= 3 && (wartoscSymbol == sym || wartoscSymbol == symProd)) {
            k.punkty = 90;
            k.powod = "wartość równa symbolowi";
        } else if (!wartosc.empty() && !e.wartosc.empty() &&
                   wartosc == ImportBOM::normalizujWartosc(e.wartosc)) {
            // element może mieć kilka obudów ("R7.5, R10, R15L"): wystarczy, że pasuje jedna z nich
            const auto obudowyElementu = podzielObudowy(e.obudowa);
            const bool pasuje = !obudowa.empty() && std::find(obudowyElementu.begin(), obudowyElementu.end(), obudowa) != obudowyElementu.end();
            if (pasuje) {
                k.punkty = 80;
                k.powod = "wartość i obudowa";
            } else if (obudowa.empty() || obudowyElementu.empty()) {
                k.punkty = 50;
                k.powod = "wartość (obudowa nieznana)";
            } else {
                k.punkty = 40;
                k.powod = "wartość (inna obudowa)";
            }
        } else {
            continue;
        }
        wpisy.push_back({k, e.iloscDostepna});
    }

    std::stable_sort(wpisy.begin(), wpisy.end(), [](const Wpis& a, const Wpis& b) {
        if (a.k.punkty != b.k.punkty) return a.k.punkty > b.k.punkty;
        return a.stan > b.stan;
    });

    std::vector<KandydatElementu> wynik;
    for (const Wpis& w : wpisy) {
        if (wynik.size() >= maksymalnie) break;
        wynik.push_back(w.k);
    }
    return wynik;
}

} // namespace DopasowanieBOM
