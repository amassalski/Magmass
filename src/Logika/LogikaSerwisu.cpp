#include "LogikaSerwisu.h"
#include "LogikaNarzedzi.h"
#include "LogikaMagazynu.h"
#include "../Narzedzia/Liczby.h"

#include <algorithm>

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

const char* KOLUMNY =
    "s.id, to_char(s.data_serwisu, 'YYYY-MM-DD') AS data, s.urzadzenie, COALESCE(s.producent, '') AS producent, COALESCE(s.model, '') AS model, "
    "COALESCE(s.numer_seryjny, '') AS numer_seryjny, COALESCE(s.lokalizacja, '') AS lokalizacja, s.rodzaj, s.status, COALESCE(s.zgloszenie, '') AS zgloszenie, "
    "COALESCE(s.technik, '') AS technik, s.czas_min, COALESCE(s.usterka, '') AS usterka, COALESCE(s.wykonano, '') AS wykonano, COALESCE(s.uwagi, '') AS uwagi, "
    "(SELECT count(*) FROM tbl_serwis_czesci c WHERE c.id_serwisu = s.id) AS n_czesci, "
    "(SELECT count(*) FROM tbl_serwis_zdjecia z WHERE z.id_serwisu = s.id) AS n_zdjec";

WpisSerwisowy wpisZWiersza(const WynikSQL& w, int i) {
    WpisSerwisowy e;
    e.id = w.liczba(i, w.indeksKolumny("id"));
    e.data = w.tekst(i, "data");
    e.urzadzenie = w.tekst(i, "urzadzenie");
    e.producent = w.tekst(i, "producent");
    e.model = w.tekst(i, "model");
    e.numerSeryjny = w.tekst(i, "numer_seryjny");
    e.lokalizacja = w.tekst(i, "lokalizacja");
    e.rodzaj = w.tekst(i, "rodzaj");
    e.status = w.tekst(i, "status");
    e.zgloszenie = w.tekst(i, "zgloszenie");
    e.technik = w.tekst(i, "technik");
    const int kc = w.indeksKolumny("czas_min");
    if (!w.czyNull(i, kc)) e.czasMin = static_cast<int>(w.liczba(i, kc));
    e.usterka = w.tekst(i, "usterka");
    e.wykonano = w.tekst(i, "wykonano");
    e.uwagi = w.tekst(i, "uwagi");
    e.liczbaCzesci = static_cast<int>(w.liczba(i, w.indeksKolumny("n_czesci")));
    e.liczbaZdjec = static_cast<int>(w.liczba(i, w.indeksKolumny("n_zdjec")));
    return e;
}

std::string cudzyslow(const std::string& s) {
    if (s.find_first_of(";\"\n\r") == std::string::npos) return s;
    std::string w = "\"";
    for (char c : s) { if (c == '"') w += '"'; w += c; }
    return w + "\"";
}

}  // namespace

const std::vector<std::string>& LogikaSerwisu::rodzaje() {
    static const std::vector<std::string> v = {"naprawa", "wymiana", "przegląd", "konfiguracja", "instalacja", "pomiar", "inne"};
    return v;
}
const std::vector<std::string>& LogikaSerwisu::statusy() {
    static const std::vector<std::string> v = {"zakończone", "w trakcie", "oczekuje na części"};
    return v;
}

PolaczenieBazy* LogikaSerwisu::polaczenie() {
    PolaczenieBazy* pol = manager_.pobierzPolaczenie();
    if (!pol || !pol->czyPolaczony()) { blad_ = "Brak połączenia z bazą danych."; return nullptr; }
    return pol;
}

bool LogikaSerwisu::ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem) {
    const std::string blad = pol->pobierzBlad();
    blad_ = zaZnaczeniem.empty() ? blad : zaZnaczeniem + "\n" + blad;
    return false;
}

bool LogikaSerwisu::pobierz(const FiltrSerwisu& f, std::vector<WpisSerwisowy>& wynik) {
    wynik.clear();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    const std::string od = przytnij(f.od), doD = przytnij(f.doDaty);
    if (!od.empty() && !LogikaNarzedzi::czyData(od)) { blad_ = "Data początkowa musi mieć postać RRRR-MM-DD."; return false; }
    if (!doD.empty() && !LogikaNarzedzi::czyData(doD)) { blad_ = "Data końcowa musi mieć postać RRRR-MM-DD."; return false; }
    std::string sql = std::string("SELECT ") + KOLUMNY + " FROM tbl_serwis s WHERE TRUE ";
    std::vector<Parametr> p;
    auto param = [&](const std::string& v) { p.push_back(v); return "$" + std::to_string(p.size()); };
    const std::string t = przytnij(f.tekst);
    if (!t.empty()) {
        const std::string a = param("%" + t + "%");
        sql += "AND (s.urzadzenie ILIKE " + a + " OR s.producent ILIKE " + a + " OR s.model ILIKE " + a + " OR s.numer_seryjny ILIKE " + a +
               " OR s.lokalizacja ILIKE " + a + " OR s.zgloszenie ILIKE " + a + " OR s.technik ILIKE " + a + " OR s.usterka ILIKE " + a +
               " OR s.wykonano ILIKE " + a + " OR s.uwagi ILIKE " + a +
               " OR EXISTS (SELECT 1 FROM tbl_serwis_czesci c WHERE c.id_serwisu = s.id AND (c.nazwa ILIKE " + a +
               " OR c.numer_stary ILIKE " + a + " OR c.numer_nowy ILIKE " + a + "))) ";
    }
    if (!przytnij(f.rodzaj).empty()) sql += "AND s.rodzaj = " + param(przytnij(f.rodzaj)) + " ";
    if (!przytnij(f.status).empty()) sql += "AND s.status = " + param(przytnij(f.status)) + " ";
    if (!od.empty()) sql += "AND s.data_serwisu >= " + param(od) + "::date ";
    if (!doD.empty()) sql += "AND s.data_serwisu <= " + param(doD) + "::date ";
    sql += "ORDER BY s.data_serwisu DESC, s.id DESC LIMIT 5000";
    auto odp = pol->zapytanie(sql, p);
    if (!odp) return ustawBladBazy(pol, "Nie udało się pobrać wpisów serwisowych.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back(wpisZWiersza(*odp, i));
    return true;
}

std::optional<WpisSerwisowy> LogikaSerwisu::pobierzWpis(long long id) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;
    auto odp = pol->zapytanie(std::string("SELECT ") + KOLUMNY + " FROM tbl_serwis s WHERE s.id = $1::bigint", {std::to_string(id)});
    if (!odp) { ustawBladBazy(pol); return std::nullopt; }
    if (odp->liczbaWierszy() != 1) { blad_ = "Wpis serwisowy nie istnieje."; return std::nullopt; }
    WpisSerwisowy w = wpisZWiersza(*odp, 0);
    auto cz = pol->zapytanie("SELECT c.id, c.nazwa, c.ilosc, COALESCE(c.numer_stary, '') AS ns, COALESCE(c.numer_nowy, '') AS nn, COALESCE(c.uwagi, '') AS uw, "
                             "c.id_elementu, COALESCE(e.symbol, '') AS symbol, c.pobrano::integer "
                             "FROM tbl_serwis_czesci c LEFT JOIN tbl_elementy e ON e.id = c.id_elementu WHERE c.id_serwisu = $1::bigint ORDER BY c.id", {std::to_string(id)});
    if (!cz) { ustawBladBazy(pol); return std::nullopt; }
    for (int i = 0; i < cz->liczbaWierszy(); ++i) {
        CzescSerwisowa c;
        c.id = cz->liczba(i, 0);
        c.nazwa = cz->tekst(i, 1);
        c.ilosc = cz->rzeczywista(i, 2);
        c.numerStary = cz->tekst(i, 3);
        c.numerNowy = cz->tekst(i, 4);
        c.uwagi = cz->tekst(i, 5);
        if (!cz->czyNull(i, 6)) c.idElementu = cz->liczba(i, 6);
        c.symbolElementu = cz->tekst(i, 7);
        c.pobrano = cz->liczba(i, 8) != 0;
        w.czesci.push_back(c);
    }
    auto zd = pol->zapytanie("SELECT id, sciezka, COALESCE(opis, '') FROM tbl_serwis_zdjecia WHERE id_serwisu = $1::bigint ORDER BY id", {std::to_string(id)});
    if (!zd) { ustawBladBazy(pol); return std::nullopt; }
    for (int i = 0; i < zd->liczbaWierszy(); ++i) {
        ZdjecieSerwisu z;
        z.id = zd->liczba(i, 0);
        z.sciezka = zd->tekst(i, 1);
        z.opis = zd->tekst(i, 2);
        w.zdjecia.push_back(z);
    }
    return w;
}

std::optional<long long> LogikaSerwisu::zapisz(const WpisSerwisowy& w, bool zwrocUsuniete) {
    if (przytnij(w.urzadzenie).empty()) { blad_ = "Podaj nazwę urządzenia."; return std::nullopt; }
    const std::string data = przytnij(w.data);
    if (!data.empty() && !LogikaNarzedzi::czyData(data)) { blad_ = "Data serwisu musi mieć postać RRRR-MM-DD."; return std::nullopt; }
    if (std::find(rodzaje().begin(), rodzaje().end(), w.rodzaj) == rodzaje().end()) { blad_ = "Nieznany rodzaj serwisu: " + w.rodzaj; return std::nullopt; }
    if (std::find(statusy().begin(), statusy().end(), w.status) == statusy().end()) { blad_ = "Nieznany status serwisu: " + w.status; return std::nullopt; }
    if (w.czasMin && *w.czasMin < 0) { blad_ = "Czas pracy nie może być ujemny."; return std::nullopt; }
    for (const CzescSerwisowa& c : w.czesci) {
        if (przytnij(c.nazwa).empty()) { blad_ = "Każda część musi mieć nazwę."; return std::nullopt; }
        if (c.ilosc <= 0) { blad_ = "Ilość części musi być dodatnia: " + c.nazwa; return std::nullopt; }
        if (c.idElementu && c.pierwszePobranie() && c.ilosc != static_cast<int>(c.ilosc)) {
            blad_ = "Część z magazynu musi mieć całkowitą ilość sztuk: " + c.nazwa;
            return std::nullopt;
        }
    }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;
    if (!pol->rozpocznijTransakcje()) { ustawBladBazy(pol); return std::nullopt; }
    auto wycofaj = [&](const std::string& opis) { ustawBladBazy(pol, opis); pol->wycofajTransakcje(); return std::optional<long long>(); };
    auto wycofajMagazyn = [&](const std::string& opis, const LogikaMagazynu& mag) { blad_ = opis + "\n" + mag.pobierzBlad(); pol->wycofajTransakcje(); return std::optional<long long>(); };
    Parametr czas = w.czasMin ? Parametr(std::to_string(*w.czasMin)) : std::nullopt;
    long long id = w.id;
    if (id == 0) {
        auto odp = pol->zapytanie(
            "INSERT INTO tbl_serwis (data_serwisu, urzadzenie, producent, model, numer_seryjny, lokalizacja, rodzaj, status, zgloszenie, technik, czas_min, usterka, wykonano, uwagi) "
            "VALUES (COALESCE($1::date, current_date),$2,$3,$4,$5,$6,$7,$8,$9,$10,$11::integer,$12,$13,$14) RETURNING id",
            {tekstLubNull(data), przytnij(w.urzadzenie), tekstLubNull(w.producent), tekstLubNull(w.model), tekstLubNull(w.numerSeryjny),
             tekstLubNull(w.lokalizacja), w.rodzaj, w.status, tekstLubNull(w.zgloszenie), tekstLubNull(w.technik), czas,
             tekstLubNull(w.usterka), tekstLubNull(w.wykonano), tekstLubNull(w.uwagi)});
        if (!odp || odp->liczbaWierszy() != 1) return wycofaj("Nie udało się dodać wpisu serwisowego.");
        id = odp->liczba(0, 0);
    } else {
        auto odp = pol->zapytanie(
            "UPDATE tbl_serwis SET data_serwisu=COALESCE($2::date, current_date), urzadzenie=$3, producent=$4, model=$5, numer_seryjny=$6, lokalizacja=$7, rodzaj=$8, status=$9, "
            "zgloszenie=$10, technik=$11, czas_min=$12::integer, usterka=$13, wykonano=$14, uwagi=$15, zmieniono=current_timestamp WHERE id=$1::bigint RETURNING id",
            {std::to_string(id), tekstLubNull(data), przytnij(w.urzadzenie), tekstLubNull(w.producent), tekstLubNull(w.model), tekstLubNull(w.numerSeryjny),
             tekstLubNull(w.lokalizacja), w.rodzaj, w.status, tekstLubNull(w.zgloszenie), tekstLubNull(w.technik), czas,
             tekstLubNull(w.usterka), tekstLubNull(w.wykonano), tekstLubNull(w.uwagi)});
        if (!odp) return wycofaj("Nie udało się zapisać wpisu serwisowego.");
        if (odp->liczbaWierszy() != 1) { blad_ = "Wpis serwisowy nie istnieje."; pol->wycofajTransakcje(); return std::nullopt; }
        if (zwrocUsuniete) {
            for (const CzescSerwisowa& stara : czesciPobrane(id)) {
                const bool zostaje = std::any_of(w.czesci.begin(), w.czesci.end(), [&](const CzescSerwisowa& n) { return n.id == stara.id; });
                if (zostaje || !stara.idElementu) continue;
                LogikaMagazynu mag(manager_);
                if (!mag.zmienIlosc(*stara.idElementu, static_cast<int>(stara.ilosc), "PRZYJECIE", "Zwrot z serwisu: " + przytnij(w.urzadzenie)))
                    return wycofajMagazyn("Nie udało się zwrócić części do magazynu: " + stara.nazwa, mag);
            }
        }
        if (!pol->zapytanie("DELETE FROM tbl_serwis_czesci WHERE id_serwisu = $1::bigint", {std::to_string(id)})) return wycofaj("Nie udało się zapisać części.");
    }
    for (const CzescSerwisowa& c : w.czesci) {
        bool pobrano = c.pobrano;
        if (c.idElementu && c.pierwszePobranie()) {
            LogikaMagazynu mag(manager_);
            if (!mag.zmienIlosc(*c.idElementu, -static_cast<int>(c.ilosc), "WYDANIE", "Serwis: " + przytnij(w.urzadzenie)))
                return wycofajMagazyn("Nie udało się pobrać z magazynu: " + c.nazwa, mag);
            pobrano = true;
        }
        Parametr idEl = c.idElementu ? Parametr(std::to_string(*c.idElementu)) : std::nullopt;
        if (!pol->zapytanie("INSERT INTO tbl_serwis_czesci (id_serwisu, nazwa, ilosc, numer_stary, numer_nowy, uwagi, id_elementu, pobrano) "
                            "VALUES ($1::bigint,$2,$3::numeric,$4,$5,$6,$7::bigint,$8::boolean)",
                            {std::to_string(id), przytnij(c.nazwa), liczbaNaTekstKlasyczny(c.ilosc), tekstLubNull(c.numerStary), tekstLubNull(c.numerNowy),
                             tekstLubNull(c.uwagi), idEl, std::string(pobrano ? "true" : "false")}))
            return wycofaj("Nie udało się zapisać części.");
    }
    if (!pol->zatwierdzTransakcje()) { ustawBladBazy(pol); return std::nullopt; }
    return id;
}

std::vector<CzescSerwisowa> LogikaSerwisu::czesciPobrane(long long idWpisu) {
    std::vector<CzescSerwisowa> wynik;
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return wynik;
    auto odp = pol->zapytanie("SELECT id, nazwa, ilosc, id_elementu FROM tbl_serwis_czesci WHERE id_serwisu = $1::bigint AND pobrano AND id_elementu IS NOT NULL ORDER BY id",
                              {std::to_string(idWpisu)});
    if (!odp) return wynik;
    for (int i = 0; i < odp->liczbaWierszy(); ++i) {
        CzescSerwisowa c;
        c.id = odp->liczba(i, 0);
        c.nazwa = odp->tekst(i, 1);
        c.ilosc = odp->rzeczywista(i, 2);
        c.idElementu = odp->liczba(i, 3);
        c.pobrano = true;
        wynik.push_back(c);
    }
    return wynik;
}

bool LogikaSerwisu::usun(long long id, bool zwroc) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    if (!pol->rozpocznijTransakcje()) return ustawBladBazy(pol);
    if (zwroc) {
        auto w = pobierzWpis(id);
        for (const CzescSerwisowa& c : czesciPobrane(id)) {
            LogikaMagazynu mag(manager_);
            if (!mag.zmienIlosc(*c.idElementu, static_cast<int>(c.ilosc), "PRZYJECIE", "Zwrot z serwisu: " + (w ? w->urzadzenie : std::string()))) {
                blad_ = "Nie udało się zwrócić części do magazynu: " + c.nazwa + "\n" + mag.pobierzBlad();
                pol->wycofajTransakcje();
                return false;
            }
        }
    }
    if (!pol->zapytanie("DELETE FROM tbl_serwis WHERE id = $1::bigint", {std::to_string(id)})) {
        ustawBladBazy(pol, "Nie udało się usunąć wpisu serwisowego.");
        pol->wycofajTransakcje();
        return false;
    }
    if (!pol->zatwierdzTransakcje()) return ustawBladBazy(pol);
    return true;
}

bool LogikaSerwisu::dodajZdjecie(long long idWpisu, const std::string& sciezka, const std::string& opis) {
    if (przytnij(sciezka).empty()) { blad_ = "Brak ścieżki zdjęcia."; return false; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto odp = pol->zapytanie("INSERT INTO tbl_serwis_zdjecia (id_serwisu, sciezka, opis) SELECT id, $2, $3 FROM tbl_serwis WHERE id = $1::bigint RETURNING id",
                              {std::to_string(idWpisu), przytnij(sciezka), tekstLubNull(opis)});
    if (!odp) return ustawBladBazy(pol, "Nie udało się dodać zdjęcia.");
    if (odp->liczbaWierszy() != 1) { blad_ = "Wpis serwisowy nie istnieje."; return false; }
    return true;
}

bool LogikaSerwisu::usunZdjecie(long long idZdjecia) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    if (!pol->zapytanie("DELETE FROM tbl_serwis_zdjecia WHERE id = $1::bigint", {std::to_string(idZdjecia)})) return ustawBladBazy(pol, "Nie udało się usunąć zdjęcia.");
    return true;
}

bool LogikaSerwisu::historiaUrzadzenia(const std::string& seryjny, const std::string& urzadzenie, std::vector<WpisSerwisowy>& wynik) {
    wynik.clear();
    const std::string sn = przytnij(seryjny), nazwa = przytnij(urzadzenie);
    if (sn.empty() && nazwa.empty()) { blad_ = "Podaj numer seryjny lub nazwę urządzenia."; return false; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto odp = pol->zapytanie(std::string("SELECT ") + KOLUMNY + " FROM tbl_serwis s WHERE ($1 <> '' AND lower(COALESCE(s.numer_seryjny, '')) = lower($1)) "
                              "OR ($1 = '' AND lower(s.urzadzenie) = lower($2)) ORDER BY s.data_serwisu DESC, s.id DESC LIMIT 2000",
                              {sn, nazwa});
    if (!odp) return ustawBladBazy(pol, "Nie udało się pobrać historii urządzenia.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back(wpisZWiersza(*odp, i));
    return true;
}

std::vector<std::string> LogikaSerwisu::unikalne(const char* kolumna, const char* tabela) {
    std::vector<std::string> w;
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return w;
    auto odp = pol->zapytanie(std::string("SELECT ") + kolumna + " FROM " + tabela + " WHERE COALESCE(" + kolumna + ", '') <> '' GROUP BY " + kolumna +
                              " ORDER BY max(dodano) DESC, " + kolumna + " LIMIT 300");
    if (!odp) return w;
    for (int i = 0; i < odp->liczbaWierszy(); ++i) w.push_back(odp->tekst(i, 0));
    return w;
}
std::vector<std::string> LogikaSerwisu::urzadzenia() { return unikalne("urzadzenie"); }
std::vector<std::string> LogikaSerwisu::lokalizacje() { return unikalne("lokalizacja"); }
std::vector<std::string> LogikaSerwisu::technicy() { return unikalne("technik"); }

std::string LogikaSerwisu::naCSV(const std::vector<WpisSerwisowy>& wpisy) {
    std::string w = "Data;Urządzenie;Producent;Model;Numer seryjny;Lokalizacja;Rodzaj;Status;Zgłoszenie;Technik;Czas [min];Usterka;Wykonano;Uwagi;"
                    "Część;Ilość;Numer zdjęty;Numer zamontowany;Uwagi do części\n";
    for (const WpisSerwisowy& e : wpisy) {
        const std::string baza = cudzyslow(e.data) + ";" + cudzyslow(e.urzadzenie) + ";" + cudzyslow(e.producent) + ";" + cudzyslow(e.model) + ";" +
                                 cudzyslow(e.numerSeryjny) + ";" + cudzyslow(e.lokalizacja) + ";" + cudzyslow(e.rodzaj) + ";" + cudzyslow(e.status) + ";" +
                                 cudzyslow(e.zgloszenie) + ";" + cudzyslow(e.technik) + ";" + (e.czasMin ? std::to_string(*e.czasMin) : std::string()) + ";" +
                                 cudzyslow(e.usterka) + ";" + cudzyslow(e.wykonano) + ";" + cudzyslow(e.uwagi) + ";";
        if (e.czesci.empty()) { w += baza + ";;;;\n"; continue; }
        for (const CzescSerwisowa& c : e.czesci)
            w += baza + cudzyslow(c.nazwa) + ";" + liczbaNaTekstPolski(c.ilosc, 3, 0) + ";" + cudzyslow(c.numerStary) + ";" + cudzyslow(c.numerNowy) + ";" + cudzyslow(c.uwagi) + "\n";
    }
    return w;
}
