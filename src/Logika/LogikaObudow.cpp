#include "LogikaObudow.h"
#include "../Narzedzia/Liczby.h"
#include "../Import/ImportBOM.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace {

std::string przytnij(const std::string& s) {
    const char* sp = " \t\r\n";
    const size_t a = s.find_first_not_of(sp);
    if (a == std::string::npos) return std::string();
    return s.substr(a, s.find_last_not_of(sp) - a + 1);
}
Parametr tekstLubNull(const std::string& s) {
    const std::string t = przytnij(s);
    if (t.empty()) return std::nullopt;
    return t;
}
Parametr liczbaLubNull(const std::optional<double>& v) {
    if (!v) return std::nullopt;
    return liczbaNaTekstKlasyczny(*v);
}
Parametr calkowitaLubNull(int v) {
    if (v <= 0) return std::nullopt;
    return std::to_string(v);
}

// Wyrażenie SQL: klucz porównania nazw ("TO-220" == "to220")
const char* KLUCZ_SQL = "regexp_replace(lower(%s), '[^a-z0-9]', '', 'g')";
std::string kluczSql(const std::string& kolumna) {
    std::string s = KLUCZ_SQL;
    s.replace(s.find("%s"), 2, kolumna);
    return s;
}

const std::string LICZBA_ELEMENTOW =
    "(SELECT count(*) FROM tbl_elementy_obudowy eo JOIN tbl_elementy e ON e.id = eo.id_elementu "
    "WHERE eo.id_obudowy = o.id AND e.czy_aktywny) AS liczba_elementow";

std::string kolumny() {
    return "o.id, o.nazwa, o.opis, o.montaz, o.liczba_pinow, o.moc_strat_w, o.napiecie_max_v, o.rozstaw, o.uwagi, o.rodzina, o.wariant, " + LICZBA_ELEMENTOW;
}

Obudowa obudowaZWiersza(const WynikSQL& w, int i) {
    Obudowa o;
    o.id = w.liczba(i, w.indeksKolumny("id"));
    o.nazwa = w.tekst(i, "nazwa");
    o.opis = w.tekst(i, "opis");
    o.montaz = w.tekst(i, "montaz");
    o.liczbaPinow = static_cast<int>(w.liczba(i, w.indeksKolumny("liczba_pinow")));
    const int km = w.indeksKolumny("moc_strat_w"), kn = w.indeksKolumny("napiecie_max_v");
    if (!w.czyNull(i, km)) o.mocStratW = w.rzeczywista(i, km);
    if (!w.czyNull(i, kn)) o.napiecieMaxV = w.rzeczywista(i, kn);
    o.rozstaw = w.tekst(i, "rozstaw");
    o.uwagi = w.tekst(i, "uwagi");
    o.rodzina = w.tekst(i, "rodzina");
    o.wariant = w.tekst(i, "wariant");
    o.liczbaElementow = static_cast<int>(w.liczba(i, w.indeksKolumny("liczba_elementow")));
    return o;
}

}  // namespace

std::string LogikaObudow::klucz(const std::string& nazwa) { return kluczObudowy(nazwa); }

std::vector<std::string> LogikaObudow::rozbijTekst(const std::string& tekst) {
    std::vector<std::string> wynik;
    std::string biezacy;
    auto zapisz = [&]() {
        const std::string t = przytnij(biezacy);
        if (!klucz(t).empty()) wynik.push_back(t);
        biezacy.clear();
    };
    for (char c : tekst) {
        if (c == ',' || c == ';') zapisz(); else biezacy += c;
    }
    zapisz();
    return wynik;
}

std::string LogikaObudow::zlozTekst(const std::vector<std::string>& nazwy) {
    std::string t;
    for (const std::string& n : nazwy) {
        if (!t.empty()) t += ", ";
        t += n;
    }
    return t;
}

std::vector<Obudowa> LogikaObudow::zTekstu(const std::string& tekst) {
    std::vector<Obudowa> wynik;
    for (const std::string& n : rozbijTekst(tekst))
        if (auto o = znajdz(n)) wynik.push_back(*o);
    return wynik;
}

PolaczenieBazy* LogikaObudow::polaczenie() {
    PolaczenieBazy* pol = manager_.pobierzPolaczenie();
    if (!pol || !pol->czyPolaczony()) {
        blad_ = "Brak połączenia z bazą danych.";
        return nullptr;
    }
    return pol;
}

bool LogikaObudow::ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem) {
    const std::string blad = pol->pobierzBlad();
    blad_ = zaZnaczeniem.empty() ? blad : zaZnaczeniem + "\n" + blad;
    return false;
}

bool LogikaObudow::pobierz(const std::string& tekst, std::vector<Obudowa>& wynik) {
    wynik.clear();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    std::string sql = "SELECT " + kolumny() + " FROM tbl_obudowy o WHERE TRUE ";
    std::vector<Parametr> p;
    const std::string t = przytnij(tekst);
    if (!t.empty()) {
        p.push_back("%" + t + "%");
        sql += "AND (o.nazwa ILIKE $1 OR o.opis ILIKE $1 OR o.rodzina ILIKE $1 OR o.wariant ILIKE $1) ";
    }
    sql += "ORDER BY lower(o.nazwa) LIMIT 5000";
    auto odp = pol->zapytanie(sql, p);
    if (!odp) return ustawBladBazy(pol, "Nie udało się pobrać obudów.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back(obudowaZWiersza(*odp, i));
    return true;
}

std::optional<Obudowa> LogikaObudow::pobierzObudowe(long long id) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;
    auto odp = pol->zapytanie("SELECT " + kolumny() + " FROM tbl_obudowy o WHERE o.id = $1::bigint", {std::to_string(id)});
    if (!odp) { ustawBladBazy(pol); return std::nullopt; }
    if (odp->liczbaWierszy() != 1) { blad_ = "Obudowa nie istnieje."; return std::nullopt; }
    return obudowaZWiersza(*odp, 0);
}

std::optional<Obudowa> LogikaObudow::znajdz(const std::string& nazwa) {
    const std::string k = klucz(nazwa);
    if (k.empty()) { blad_.clear(); return std::nullopt; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;
    auto odp = pol->zapytanie(
        "SELECT o.id FROM tbl_obudowy o WHERE " + kluczSql("o.nazwa") + " = $1 ORDER BY o.id LIMIT 1", {k});
    if (!odp) { ustawBladBazy(pol); return std::nullopt; }
    if (odp->liczbaWierszy() == 0) { blad_.clear(); return std::nullopt; }
    return pobierzObudowe(odp->liczba(0, 0));
}

std::vector<std::string> LogikaObudow::nazwy() {
    std::vector<std::string> w;
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return w;
    auto odp = pol->zapytanie("SELECT nazwa FROM tbl_obudowy ORDER BY lower(nazwa)");
    if (!odp) return w;
    for (int i = 0; i < odp->liczbaWierszy(); ++i) w.push_back(odp->tekst(i, 0));
    return w;
}

std::optional<long long> LogikaObudow::zapisz(const Obudowa& o) {
    const std::string nazwa = przytnij(o.nazwa);
    if (klucz(nazwa).empty()) { blad_ = "Podaj nazwę obudowy."; return std::nullopt; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;

    auto powtorka = pol->zapytanie("SELECT 1 FROM tbl_obudowy WHERE " + kluczSql("nazwa") + " = $1 AND id <> $2::bigint",
                                   {klucz(nazwa), std::to_string(o.id)});
    if (!powtorka) { ustawBladBazy(pol); return std::nullopt; }
    if (powtorka->liczbaWierszy() > 0) { blad_ = "Obudowa o takiej nazwie już istnieje (nazwy TO-220 i TO220 są traktowane jako te same)."; return std::nullopt; }

    Transakcja transakcja(*pol);
    long long id = o.id;
    if (id == 0) {
        auto odp = pol->zapytanie(
            "INSERT INTO tbl_obudowy (nazwa, opis, montaz, liczba_pinow, moc_strat_w, napiecie_max_v, rozstaw, uwagi, rodzina, wariant) "
            "VALUES ($1,$2,$3,$4::integer,$5::numeric,$6::numeric,$7,$8,$9,$10) RETURNING id",
            {nazwa, tekstLubNull(o.opis), tekstLubNull(o.montaz), calkowitaLubNull(o.liczbaPinow), liczbaLubNull(o.mocStratW),
             liczbaLubNull(o.napiecieMaxV), tekstLubNull(o.rozstaw), tekstLubNull(o.uwagi), tekstLubNull(o.rodzina), tekstLubNull(o.wariant)});
        if (!odp || odp->liczbaWierszy() != 1) { ustawBladBazy(pol, "Nie udało się dodać obudowy."); return std::nullopt; }
        id = odp->liczba(0, 0);
    } else {
        if (!pol->zapytanie(
                "UPDATE tbl_obudowy SET nazwa=$2, opis=$3, montaz=$4, liczba_pinow=$5::integer, moc_strat_w=$6::numeric, "
                "napiecie_max_v=$7::numeric, rozstaw=$8, uwagi=$9, rodzina=$10, wariant=$11, zmieniono=current_timestamp WHERE id=$1::bigint",
                {std::to_string(id), nazwa, tekstLubNull(o.opis), tekstLubNull(o.montaz), calkowitaLubNull(o.liczbaPinow),
                 liczbaLubNull(o.mocStratW), liczbaLubNull(o.napiecieMaxV), tekstLubNull(o.rozstaw), tekstLubNull(o.uwagi), tekstLubNull(o.rodzina), tekstLubNull(o.wariant)})) {
            ustawBladBazy(pol, "Nie udało się zapisać obudowy.");
            return std::nullopt;
        }
    }
    if (!transakcja.zatwierdz()) { ustawBladBazy(pol); return std::nullopt; }
    return id;
}

bool LogikaObudow::usun(long long id) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    if (!pol->zapytanie("DELETE FROM tbl_obudowy WHERE id = $1::bigint", {std::to_string(id)}))
        return ustawBladBazy(pol, "Nie udało się usunąć obudowy.");
    return true;
}

int LogikaObudow::uzupelnijZElementow() {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return -1;
    auto odp = pol->zapytanie(
        "SELECT min(nazwa) AS nazwa FROM ("
        "  SELECT btrim(obudowa) AS nazwa FROM tbl_elementy WHERE czy_aktywny AND btrim(COALESCE(obudowa, '')) <> '' "
        "  UNION ALL SELECT btrim(obudowa) FROM tbl_katalog WHERE btrim(COALESCE(obudowa, '')) <> '') x "
        "GROUP BY " + kluczSql("nazwa") + " HAVING " + kluczSql("min(nazwa)") + " <> ''");
    if (!odp) { ustawBladBazy(pol, "Nie udało się odczytać obudów z elementów."); return -1; }
    int dodano = 0;
    for (int i = 0; i < odp->liczbaWierszy(); ++i) {
        const std::string nazwa = odp->tekst(i, 0);
        if (znajdz(nazwa)) continue;
        Obudowa o;
        o.nazwa = nazwa;
        if (zapisz(o)) ++dodano;
    }
    return dodano;
}

std::vector<TypowyElement> LogikaObudow::typowe(long long idObudowy) {
    std::vector<TypowyElement> wynik;
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return wynik;
    auto odp = pol->zapytanie("SELECT zestaw, opis, prefiks FROM tbl_obudowy_typowe WHERE id_obudowy = $1::bigint ORDER BY lower(zestaw)",
                              {std::to_string(idObudowy)});
    if (!odp) { ustawBladBazy(pol); return wynik; }
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back({odp->tekst(i, 0), odp->tekst(i, 1), odp->tekst(i, 2)});
    return wynik;
}

std::vector<Obudowa> LogikaObudow::podpowiedzi(const std::string& symbol, size_t maksymalnie) {
    std::vector<Obudowa> wynik;
    const std::string s = przytnij(symbol);
    PolaczenieBazy* pol = polaczenie();
    if (s.empty() || !pol) return wynik;
    std::set<long long> widziane;
    auto odp = pol->zapytanie(
        "SELECT " + kolumny() + " FROM tbl_obudowy o WHERE o.id IN (SELECT t.id_obudowy FROM tbl_obudowy_typowe t "
        "WHERE lower(t.zestaw) = lower($1) OR (length(t.zestaw) >= 4 AND left(lower($1), length(t.zestaw)) = lower(t.zestaw))) "
        "ORDER BY (SELECT bool_or(lower(t.zestaw) = lower($1)) FROM tbl_obudowy_typowe t WHERE t.id_obudowy = o.id) DESC NULLS LAST, "
        "lower(o.nazwa) LIMIT " + std::to_string(maksymalnie), {s});
    if (!odp) { ustawBladBazy(pol); return wynik; }
    for (int i = 0; i < odp->liczbaWierszy(); ++i) {
        Obudowa o = obudowaZWiersza(*odp, i);
        if (widziane.insert(o.id).second) wynik.push_back(o);
    }
    // obudowa z katalogu o tym samym symbolu
    auto kat = pol->zapytanie("SELECT obudowa FROM tbl_katalog WHERE lower(symbol) = lower($1) AND coalesce(obudowa, '') <> ''", {s});
    if (kat) {
        for (int i = 0; i < kat->liczbaWierszy(); ++i) {
            for (const Obudowa& o : zTekstu(kat->tekst(i, 0))) {
                if (wynik.size() < maksymalnie && widziane.insert(o.id).second) wynik.push_back(o);
            }
        }
    }
    return wynik;
}

std::optional<Obudowa> LogikaObudow::rozwiazStopke(const std::string& stopka) {
    const std::string t = przytnij(stopka);
    if (t.empty()) return std::nullopt;
    if (auto o = znajdz(t)) return o;
    std::string bez = t;                                        // "Resistor_SMD:R_0805_2012Metric" -> "R_0805_2012Metric"
    const size_t dwukropek = bez.rfind(':');
    if (dwukropek != std::string::npos) bez = bez.substr(dwukropek + 1);
    if (auto o = znajdz(bez)) return o;
    const std::string kod = ImportBOM::normalizujObudowe(t);    // "0805" dla R0805, R_0805_2012Metric, C-0805
    if (kod.empty()) return std::nullopt;
    std::vector<Obudowa> wszystkie;
    if (!pobierz("", wszystkie)) return std::nullopt;
    std::vector<const Obudowa*> pasujace;
    for (const Obudowa& o : wszystkie) {
        if (ImportBOM::normalizujObudowe(o.nazwa) == kod) pasujace.push_back(&o);
    }
    if (pasujace.size() == 1) return *pasujace[0];
    // kilka obudów o tym samym kodzie ("R0805", "C0805", "0805"): najpierw z przedrostkiem z footprintu (C_0805 -> C0805), potem sam kod
    std::string przedrostek;
    for (char c : klucz(bez)) { if (std::isalpha(static_cast<unsigned char>(c))) przedrostek += c; else break; }
    for (const Obudowa* o : pasujace) if (!przedrostek.empty() && klucz(o->nazwa) == przedrostek + kod) return *o;
    for (const Obudowa* o : pasujace) if (klucz(o->nazwa) == kod) return *o;
    return std::nullopt;
}

bool LogikaObudow::importuj(const ImportLBR::Biblioteka& b, bool nadpisz, WynikImportuObudow& wynik) {
    wynik = WynikImportuObudow();
    if (!polaczenie()) return false;
    // każdy pakiet Eagle to osobna obudowa (R7.5, R10, R15L, TO220AH...); rodzina i wariant to jej atrybuty
    std::set<std::string> widziane;
    for (const ImportLBR::Pakiet& p : b.pakiety) {
        const std::string k = klucz(p.nazwa);
        if (k.empty() || !widziane.insert(k).second) continue;
        auto istn = znajdz(p.nazwa);
        Obudowa o = istn ? *istn : Obudowa();
        const bool nowa = !istn;
        bool zmieniono = nowa;
        auto ustawTekst = [&](std::string& cel, const std::string& nowy) {
            if (nowy.empty()) return;
            if (cel.empty() || nadpisz) { if (cel != nowy) { cel = nowy; zmieniono = true; } }
        };
        if (nowa) o.nazwa = p.nazwa;
        ustawTekst(o.opis, p.opis.substr(0, 480));
        ustawTekst(o.montaz, p.montaz);
        ustawTekst(o.rozstaw, p.rozstaw);
        ustawTekst(o.rodzina, p.rodzina);
        ustawTekst(o.wariant, p.wariant);
        if (p.liczbaPinow > 0 && (o.liczbaPinow == 0 || nadpisz) && o.liczbaPinow != p.liczbaPinow) { o.liczbaPinow = p.liczbaPinow; zmieniono = true; }
        if (p.mocW && (!o.mocStratW || nadpisz) && o.mocStratW != p.mocW) { o.mocStratW = p.mocW; zmieniono = true; }
        if (p.napiecieV && (!o.napiecieMaxV || nadpisz) && o.napiecieMaxV != p.napiecieV) { o.napiecieMaxV = p.napiecieV; zmieniono = true; }
        if (!zmieniono) continue;
        if (!zapisz(o)) { wynik.bledy.push_back(o.nazwa + ": " + blad_); continue; }
        if (nowa) ++wynik.dodano; else ++wynik.uzupelniono;
    }
    // typowe elementy: zestawy biblioteki z ich pakietami ("*" w nazwie zestawu zastępuje nazwa urządzenia)
    std::set<std::string> zapisane;
    for (const ImportLBR::Urzadzenie& u : b.urzadzenia) {
        if (u.pakiet.empty() || u.zestaw.empty()) continue;
        auto ob = znajdz(u.pakiet);
        if (!ob) continue;
        std::string nazwa = u.zestaw;
        const size_t gw = nazwa.find('*');
        if (gw != std::string::npos) nazwa.replace(gw, 1, u.wariant);
        else nazwa += u.wariant;
        nazwa = przytnij(nazwa);
        if (nazwa.empty() || !zapisane.insert(std::to_string(ob->id) + "|" + nazwa).second) continue;
        auto odp = polaczenie()->zapytanie(
            "INSERT INTO tbl_obudowy_typowe (id_obudowy, zestaw, opis, prefiks) VALUES ($1::bigint, $2, $3, $4) "
            "ON CONFLICT (id_obudowy, zestaw) DO UPDATE SET opis = EXCLUDED.opis, prefiks = EXCLUDED.prefiks",
            {std::to_string(ob->id), nazwa.substr(0, 100), tekstLubNull(u.opisZestawu.substr(0, 480)), tekstLubNull(u.prefiks.substr(0, 10))});
        if (!odp) { wynik.bledy.push_back(nazwa + ": " + polaczenie()->pobierzBlad()); continue; }
        ++wynik.typowych;
    }
    return true;
}
