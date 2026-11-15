#include "LogikaSlownikaParametrow.h"

#include "OpisyParametrow.h"

#include <map>

namespace {
std::string przytnij(const std::string& s) {
    const char* sp = " \t\r\n";
    const size_t a = s.find_first_not_of(sp);
    if (a == std::string::npos) return std::string();
    return s.substr(a, s.find_last_not_of(sp) - a + 1);
}
size_t znaki(const std::string& s) {   // liczba znaków UTF-8 (varchar liczy znaki, nie bajty)
    size_t n = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
    return n;
}
} // namespace

PolaczenieBazy* LogikaSlownikaParametrow::polaczenie() {
    PolaczenieBazy* pol = manager_.pobierzPolaczenie();
    if (!pol || !pol->czyPolaczony()) {
        blad_ = "Brak połączenia z bazą danych.";
        return nullptr;
    }
    return pol;
}

bool LogikaSlownikaParametrow::ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem) {
    const std::string blad = pol->pobierzBlad();
    blad_ = zaZnaczeniem.empty() ? blad : zaZnaczeniem + "\n" + blad;
    return false;
}

bool LogikaSlownikaParametrow::pobierz(const std::string& szukaj, std::vector<WpisSlownikaParametrow>& wynik) {
    wynik.clear();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    const std::string t = przytnij(szukaj);
    auto odp = t.empty()
        ? pol->zapytanie("SELECT id, symbol, opis FROM tbl_slownik_parametrow ORDER BY lower(symbol), id")
        : pol->zapytanie("SELECT id, symbol, opis FROM tbl_slownik_parametrow "
                         "WHERE symbol ILIKE '%' || $1 || '%' OR opis ILIKE '%' || $1 || '%' OR klucz LIKE '%' || $2 || '%' "
                         "ORDER BY lower(symbol), id", {t, OpisyParametrow::klucz(t)});
    if (!odp) return ustawBladBazy(pol);
    for (int i = 0; i < odp->liczbaWierszy(); ++i) {
        WpisSlownikaParametrow w;
        w.id = odp->liczba(i, odp->indeksKolumny("id"));
        w.symbol = odp->tekst(i, "symbol");
        w.opis = odp->tekst(i, "opis");
        wynik.push_back(w);
    }
    return true;
}

std::optional<long long> LogikaSlownikaParametrow::zapisz(const WpisSlownikaParametrow& wpis) {
    const std::string symbol = przytnij(wpis.symbol), opis = przytnij(wpis.opis);
    if (symbol.empty()) { blad_ = "Podaj symbol parametru."; return std::nullopt; }
    if (opis.empty()) { blad_ = "Podaj opis parametru."; return std::nullopt; }
    if (znaki(symbol) > 100) { blad_ = "Symbol może mieć najwyżej 100 znaków."; return std::nullopt; }
    if (znaki(opis) > 500) { blad_ = "Opis może mieć najwyżej 500 znaków."; return std::nullopt; }
    const std::string klucz = OpisyParametrow::klucz(symbol);
    if (klucz.empty()) { blad_ = "Symbol musi zawierać litery lub cyfry."; return std::nullopt; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;
    auto powtorka = pol->zapytanie("SELECT id FROM tbl_slownik_parametrow WHERE klucz = $1 AND id <> $2::bigint",
                                   {klucz, std::to_string(wpis.id)});
    if (!powtorka) { ustawBladBazy(pol); return std::nullopt; }
    if (powtorka->liczbaWierszy() > 0) { blad_ = "Taki symbol już jest w słowniku (wielkość liter, spacje, _ i nawiasy się nie liczą)."; return std::nullopt; }
    if (wpis.id == 0) {
        auto odp = pol->zapytanie("INSERT INTO tbl_slownik_parametrow (symbol, klucz, opis) VALUES ($1,$2,$3) RETURNING id",
                                  {symbol, klucz, opis});
        if (!odp || odp->liczbaWierszy() != 1) { ustawBladBazy(pol); return std::nullopt; }
        return odp->liczba(0, 0);
    }
    auto odp = pol->zapytanie("UPDATE tbl_slownik_parametrow SET symbol = $2, klucz = $3, opis = $4, zmieniono = current_timestamp "
                              "WHERE id = $1::bigint", {std::to_string(wpis.id), symbol, klucz, opis});
    if (!odp) { ustawBladBazy(pol); return std::nullopt; }
    if (odp->wierszeZmienione() != 1) { blad_ = "Nie ma takiego wpisu."; return std::nullopt; }
    return wpis.id;
}

bool LogikaSlownikaParametrow::usun(long long id) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto odp = pol->zapytanie("DELETE FROM tbl_slownik_parametrow WHERE id = $1::bigint", {std::to_string(id)});
    if (!odp) return ustawBladBazy(pol);
    return true;
}

int LogikaSlownikaParametrow::przywrocBrakujace() {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return -1;
    Transakcja transakcja(*pol);
    int dopisano = 0;
    for (const auto& p : OpisyParametrow::wbudowane()) {
        auto odp = pol->zapytanie("INSERT INTO tbl_slownik_parametrow (symbol, klucz, opis) VALUES ($1,$2,$3) "
                                  "ON CONFLICT (klucz) DO NOTHING", {p.first, OpisyParametrow::klucz(p.first), p.second});
        if (!odp) { ustawBladBazy(pol); return -1; }
        dopisano += static_cast<int>(odp->wierszeZmienione());
    }
    if (!transakcja.zatwierdz()) { ustawBladBazy(pol); return -1; }
    return dopisano;
}

bool LogikaSlownikaParametrow::wczytajDoPamieci() {
    std::vector<WpisSlownikaParametrow> w;
    if (!pobierz("", w)) return false;
    std::map<std::string, std::string> m;
    for (const auto& e : w) m[e.symbol] = e.opis;
    OpisyParametrow::ustawZBazy(m);
    return true;
}
