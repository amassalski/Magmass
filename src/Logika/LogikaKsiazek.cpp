#include "LogikaKsiazek.h"
#include "LogikaMagazynu.h"

#include "LogikaNarzedzi.h"

#include <algorithm>
#include <cctype>

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
    "k.id, k.tytul, k.autor, k.wydawca, k.rok, k.wydanie, k.isbn, k.jezyk, k.tagi, k.uwagi, k.sciezka, "
    "COALESCE(k.strony, 0) AS strony, COALESCE(to_char(k.zaindeksowano, 'YYYY-MM-DD HH24:MI'), '') AS zaindeksowano, "
    "p.osoba AS pozyczona_przez, COALESCE(to_char(p.data_pozyczenia, 'YYYY-MM-DD'), '') AS pozyczona_od, "
    "COALESCE(to_char(p.termin, 'YYYY-MM-DD'), '') AS pozyczka_termin, "
    "COALESCE(p.termin < current_date, false) AS pozyczka_po_terminie";

const char* ZRODLO = " FROM tbl_ksiazki k LEFT JOIN tbl_pozyczki p ON p.id_ksiazki = k.id AND p.data_zwrotu IS NULL ";

Ksiazka ksiazkaZWiersza(const WynikSQL& w, int i) {
    Ksiazka k;
    k.id = w.liczba(i, w.indeksKolumny("id"));
    k.tytul = w.tekst(i, "tytul");
    k.autor = w.tekst(i, "autor");
    k.wydawca = w.tekst(i, "wydawca");
    const int kr = w.indeksKolumny("rok");
    k.rok = w.czyNull(i, kr) ? 0 : static_cast<int>(w.liczba(i, kr));
    k.wydanie = w.tekst(i, "wydanie");
    k.isbn = w.tekst(i, "isbn");
    k.jezyk = w.tekst(i, "jezyk");
    k.tagi = w.tekst(i, "tagi");
    k.uwagi = w.tekst(i, "uwagi");
    k.sciezka = w.tekst(i, "sciezka");
    k.strony = static_cast<int>(w.liczba(i, w.indeksKolumny("strony")));
    k.zaindeksowano = w.tekst(i, "zaindeksowano");
    k.pozyczona = !w.czyNull(i, w.indeksKolumny("pozyczona_przez"));
    if (k.pozyczona) {
        k.pozyczonaPrzez = w.tekst(i, "pozyczona_przez");
        k.pozyczonaOd = w.tekst(i, "pozyczona_od");
        k.termin = w.tekst(i, "pozyczka_termin");
        k.przeterminowana = w.tekst(i, "pozyczka_po_terminie") == "t";
    }
    return k;
}

Parametr rokLubNull(int r) { return r > 0 ? Parametr(std::to_string(r)) : std::nullopt; }

}  // namespace

PolaczenieBazy* LogikaKsiazek::polaczenie() {
    PolaczenieBazy* pol = manager_.pobierzPolaczenie();
    if (!pol || !pol->czyPolaczony()) {
        blad_ = "Brak połączenia z bazą danych.";
        return nullptr;
    }
    return pol;
}

bool LogikaKsiazek::ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem) {
    const std::string blad = pol->pobierzBlad();
    blad_ = zaZnaczeniem.empty() ? blad : zaZnaczeniem + "\n" + blad;
    return false;
}

int LogikaKsiazek::rokZNazwyPliku(const std::string& nazwa) {
    std::string t = nazwa;
    for (char& c : t) if (c == '_') c = ' ';
    const size_t p = t.find_first_not_of(' ');
    if (p == std::string::npos || t.size() < p + 6) return 0;
    for (size_t i = 0; i < 4; ++i) if (!std::isdigit(static_cast<unsigned char>(t[p + i]))) return 0;
    if (t[p + 4] != ' ' && t[p + 4] != '-') return 0;
    const int r = std::stoi(t.substr(p, 4));
    if (r < 1800 || r > 2100) return 0;
    // musi coś zostać po roku (książka "1984" bez dalszego tytułu zachowuje tytuł)
    if (t.find_first_not_of(" -", p + 4) == std::string::npos) return 0;
    return r;
}

std::string LogikaKsiazek::tytulZNazwyPliku(const std::string& nazwa) {
    std::string t = nazwa;
    const size_t kropka = t.find_last_of('.');
    if (kropka != std::string::npos && kropka > 0 && t.size() - kropka <= 6) t.erase(kropka);
    for (char& c : t) if (c == '_' || c == '.') c = ' ';
    // początkowy rok wydania z nazwy pliku ("1964 ABC radioamatora") nie należy do tytułu
    {
        const int rok = rokZNazwyPliku(nazwa);
        if (rok > 0) {
            size_t p = t.find_first_not_of(' ');
            t.erase(0, p + 4);
        }
    }
    std::string w;
    bool spacja = false;
    for (char c : t) {
        if (c == ' ') { spacja = true; continue; }
        if (spacja && !w.empty()) w += ' ';
        spacja = false;
        w += c;
    }
    return w;
}

bool LogikaKsiazek::pobierz(const std::string& tekst, std::vector<Ksiazka>& wynik) {
    wynik.clear();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    std::string sql = std::string("SELECT ") + KOLUMNY + ZRODLO + "WHERE TRUE ";
    std::vector<Parametr> p;
    const std::string t = przytnij(tekst);
    if (!t.empty()) {
        p.push_back("%" + t + "%");
        sql += "AND (k.tytul ILIKE $1 OR k.autor ILIKE $1 OR k.wydawca ILIKE $1 OR k.isbn ILIKE $1 OR k.tagi ILIKE $1 OR k.uwagi ILIKE $1) ";
    }
    sql += "ORDER BY lower(k.tytul) LIMIT 5000";
    auto odp = pol->zapytanie(sql, p);
    if (!odp) return ustawBladBazy(pol, "Nie udało się pobrać książek.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back(ksiazkaZWiersza(*odp, i));
    return true;
}

std::optional<Ksiazka> LogikaKsiazek::pobierzKsiazke(long long id) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;
    auto odp = pol->zapytanie(std::string("SELECT ") + KOLUMNY + ZRODLO + "WHERE k.id = $1::bigint", {std::to_string(id)});
    if (!odp) { ustawBladBazy(pol); return std::nullopt; }
    if (odp->liczbaWierszy() != 1) { blad_ = "Książka nie istnieje."; return std::nullopt; }
    return ksiazkaZWiersza(*odp, 0);
}

std::optional<long long> LogikaKsiazek::zapisz(const Ksiazka& k) {
    if (przytnij(k.tytul).empty()) { blad_ = "Podaj tytuł książki."; return std::nullopt; }
    if (k.rok < 0 || k.rok > 2200) { blad_ = "Rok wydania jest nieprawidłowy."; return std::nullopt; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;
    const std::string sciezka = przytnij(k.sciezka);
    if (!sciezka.empty()) {
        auto powtorka = pol->zapytanie("SELECT 1 FROM tbl_ksiazki WHERE lower(sciezka) = lower($1) AND id <> $2::bigint",
                                       {sciezka, std::to_string(k.id)});
        if (!powtorka) { ustawBladBazy(pol); return std::nullopt; }
        if (powtorka->liczbaWierszy() > 0) { blad_ = "Książka z tym plikiem już jest w biblioteczce."; return std::nullopt; }
    }
    if (k.id == 0) {
        auto odp = pol->zapytanie(
            "INSERT INTO tbl_ksiazki (tytul, autor, wydawca, rok, isbn, jezyk, tagi, uwagi, sciezka, wydanie) "
            "VALUES ($1,$2,$3,$4::integer,$5,$6,$7,$8,$9,$10) RETURNING id",
            {przytnij(k.tytul), tekstLubNull(k.autor), tekstLubNull(k.wydawca), rokLubNull(k.rok), tekstLubNull(k.isbn),
             tekstLubNull(k.jezyk), tekstLubNull(k.tagi), tekstLubNull(k.uwagi), tekstLubNull(k.sciezka), tekstLubNull(k.wydanie)});
        if (!odp || odp->liczbaWierszy() != 1) { ustawBladBazy(pol, "Nie udało się dodać książki."); return std::nullopt; }
        return odp->liczba(0, 0);
    }
    if (!pol->zapytanie(
            "UPDATE tbl_ksiazki SET tytul=$2, autor=$3, wydawca=$4, rok=$5::integer, isbn=$6, jezyk=$7, tagi=$8, uwagi=$9, "
            "sciezka=$10, wydanie=$11, zmieniono=current_timestamp WHERE id=$1::bigint",
            {std::to_string(k.id), przytnij(k.tytul), tekstLubNull(k.autor), tekstLubNull(k.wydawca), rokLubNull(k.rok),
             tekstLubNull(k.isbn), tekstLubNull(k.jezyk), tekstLubNull(k.tagi), tekstLubNull(k.uwagi), tekstLubNull(k.sciezka), tekstLubNull(k.wydanie)})) {
        ustawBladBazy(pol, "Nie udało się zapisać książki.");
        return std::nullopt;
    }
    return k.id;
}

bool LogikaKsiazek::zmienZbiorczo(const std::vector<long long>& idki, const ZmianyZbiorczeKsiazek& zmiany, int* zmieniono, int* bledy) {
    int ok = 0, zle = 0;
    if (zmiany.pusta()) { blad_ = "Nie wybrano żadnej zmiany."; return false; }
    for (long long id : idki) {
        auto k = pobierzKsiazke(id);
        if (!k) { ++zle; continue; }
        if (zmiany.autor) k->autor = przytnij(*zmiany.autor);
        if (zmiany.wydawca) k->wydawca = przytnij(*zmiany.wydawca);
        if (zmiany.tagi) k->tagi = zmiany.tagiDopisz ? LogikaMagazynu::polaczTagi(k->tagi, *zmiany.tagi) : przytnij(*zmiany.tagi);
        if (zapisz(*k)) ++ok; else ++zle;
    }
    if (zmieniono) *zmieniono = ok;
    if (bledy) *bledy = zle;
    return zle == 0;
}

bool LogikaKsiazek::usun(long long id) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    if (!pol->zapytanie("DELETE FROM tbl_ksiazki WHERE id = $1::bigint", {std::to_string(id)}))
        return ustawBladBazy(pol, "Nie udało się usunąć książki.");
    return true;
}

bool LogikaKsiazek::importuj(const std::vector<Ksiazka>& ksiazki, WynikImportuKsiazek& wynik) {
    wynik = WynikImportuKsiazek();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    for (const Ksiazka& k : ksiazki) {
        if (przytnij(k.tytul).empty()) { wynik.bledy.push_back("Pozycja bez tytułu pominięta."); continue; }
        auto istn = pol->zapytanie(
            "SELECT 1 FROM tbl_ksiazki WHERE (sciezka IS NOT NULL AND $3 <> '' AND lower(sciezka) = lower($3)) "
            "OR (lower(tytul) = lower($1) AND lower(COALESCE(autor, '')) = lower($2)) LIMIT 1",
            {przytnij(k.tytul), przytnij(k.autor), przytnij(k.sciezka)});
        if (!istn) return ustawBladBazy(pol);
        if (istn->liczbaWierszy() > 0) { ++wynik.pominieto; continue; }
        if (zapisz(k)) ++wynik.dodano;
        else wynik.bledy.push_back(k.tytul + ": " + blad_);
    }
    return true;
}

bool LogikaKsiazek::zapiszIndeks(long long id, const std::vector<std::string>& strony) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    Transakcja transakcja(*pol);
    if (!pol->zapytanie("DELETE FROM tbl_ksiazki_tresc WHERE id_ksiazki = $1::bigint", {std::to_string(id)}))
        return ustawBladBazy(pol);
    int zapisane = 0;
    for (size_t i = 0; i < strony.size(); ++i) {
        // znaki NUL nie mogą trafić do bazy; pdftotext czasem zwraca śmieci sterujące
        std::string t;
        t.reserve(strony[i].size());
        for (char c : strony[i]) if (c != '\0' && c != '\f') t += c;
        if (przytnij(t).empty()) continue;
        if (!pol->zapytanie("INSERT INTO tbl_ksiazki_tresc (id_ksiazki, strona, tekst, wektor_tekstu) "
                            "VALUES ($1::bigint, $2::integer, $3, to_tsvector('simple', fn_bez_akcentow(lower($3))))",
                            {std::to_string(id), std::to_string(i + 1), t}))
            return ustawBladBazy(pol, "Nie udało się zapisać indeksu strony " + std::to_string(i + 1) + ".");
        ++zapisane;
    }
    if (!pol->zapytanie("UPDATE tbl_ksiazki SET strony = $2::integer, zaindeksowano = current_timestamp WHERE id = $1::bigint",
                        {std::to_string(id), std::to_string(zapisane)}))
        return ustawBladBazy(pol);
    if (!transakcja.zatwierdz()) return ustawBladBazy(pol);
    return true;
}

bool LogikaKsiazek::usunIndeks(long long id) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    Transakcja transakcja(*pol);
    if (!pol->zapytanie("DELETE FROM tbl_ksiazki_tresc WHERE id_ksiazki = $1::bigint", {std::to_string(id)}) ||
        !pol->zapytanie("UPDATE tbl_ksiazki SET strony = NULL, zaindeksowano = NULL WHERE id = $1::bigint", {std::to_string(id)}))
        return ustawBladBazy(pol);
    if (!transakcja.zatwierdz()) return ustawBladBazy(pol);
    return true;
}

bool LogikaKsiazek::szukajWTresci(const std::string& tekst, long long idKsiazki, std::vector<TrafienieKsiazki>& wynik, int limit) {
    wynik.clear();
    const std::string t = przytnij(tekst);
    if (t.empty()) return true;
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    // najpierw ograniczamy trafienia, dopiero potem liczymy fragmenty (ts_headline jest kosztowne)
    auto odp = pol->zapytanie(
        "SELECT x.id_ksiazki, k.tytul, COALESCE(k.sciezka, '') AS sciezka, x.strona, "
        "       ts_headline('simple', fn_bez_akcentow(lower(x.tekst)), q.zapytanie, 'MaxFragments=1, MaxWords=24, MinWords=8, StartSel=[, StopSel=]') AS fragment "
        "FROM (SELECT plainto_tsquery('simple', fn_bez_akcentow(lower($1))) AS zapytanie) q, LATERAL ("
        "   SELECT c.id_ksiazki, c.strona, c.tekst, ts_rank(c.wektor_tekstu, q.zapytanie) AS ranga FROM tbl_ksiazki_tresc c "
        "   WHERE c.wektor_tekstu @@ q.zapytanie AND ($2::bigint = 0 OR c.id_ksiazki = $2::bigint) "
        "   ORDER BY ranga DESC, c.id_ksiazki, c.strona LIMIT $3::integer) x "
        "JOIN tbl_ksiazki k ON k.id = x.id_ksiazki ORDER BY x.ranga DESC, lower(k.tytul), x.strona",
        {t, std::to_string(idKsiazki), std::to_string(limit)});
    if (!odp) return ustawBladBazy(pol, "Nie udało się przeszukać treści książek.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) {
        TrafienieKsiazki h;
        h.idKsiazki = odp->liczba(i, 0);
        h.tytul = odp->tekst(i, "tytul");
        h.sciezka = odp->tekst(i, "sciezka");
        h.strona = static_cast<int>(odp->liczba(i, 3));
        std::string f = odp->tekst(i, "fragment");
        for (char& c : f) if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        h.fragment = f;
        wynik.push_back(std::move(h));
    }
    return true;
}

// ---------------------------------------------------------------- pożyczki książek
namespace {
Pozyczka pozyczkaZWiersza(const WynikSQL& w, int i) {
    Pozyczka p;
    p.id = w.liczba(i, w.indeksKolumny("id"));
    p.idKsiazki = w.liczba(i, w.indeksKolumny("id_ksiazki"));
    p.nazwaNarzedzia = w.tekst(i, "tytul");
    p.osoba = w.tekst(i, "osoba");
    p.od = w.tekst(i, "od");
    p.termin = w.tekst(i, "termin");
    p.zwrot = w.tekst(i, "zwrot");
    p.uwagi = w.tekst(i, "uwagi");
    p.przeterminowana = w.tekst(i, "po_terminie") == "t";
    return p;
}
const char* KOLUMNY_POZYCZKI =
    "p.id, p.id_ksiazki, k.tytul, p.osoba, to_char(p.data_pozyczenia, 'YYYY-MM-DD') AS od, "
    "COALESCE(to_char(p.termin, 'YYYY-MM-DD'), '') AS termin, COALESCE(to_char(p.data_zwrotu, 'YYYY-MM-DD'), '') AS zwrot, "
    "COALESCE(p.uwagi, '') AS uwagi, (p.data_zwrotu IS NULL AND p.termin IS NOT NULL AND p.termin < current_date) AS po_terminie";
}  // namespace

bool LogikaKsiazek::pozycz(long long id, const std::string& osoba, const std::string& od, const std::string& termin, const std::string& uwagi) {
    const std::string kto = przytnij(osoba), dataOd = przytnij(od), dataTermin = przytnij(termin);
    if (kto.empty()) { blad_ = "Podaj, komu pożyczasz książkę."; return false; }
    if (!dataOd.empty() && !LogikaNarzedzi::czyData(dataOd)) { blad_ = "Data pożyczenia musi mieć postać RRRR-MM-DD."; return false; }
    if (!dataTermin.empty() && !LogikaNarzedzi::czyData(dataTermin)) { blad_ = "Termin zwrotu musi mieć postać RRRR-MM-DD."; return false; }
    if (!dataTermin.empty() && !dataOd.empty() && dataTermin < dataOd) { blad_ = "Termin zwrotu nie może być wcześniejszy niż data pożyczenia."; return false; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto istn = pol->zapytanie("SELECT osoba FROM tbl_pozyczki WHERE id_ksiazki = $1::bigint AND data_zwrotu IS NULL", {std::to_string(id)});
    if (!istn) return ustawBladBazy(pol);
    if (istn->liczbaWierszy() > 0) { blad_ = "Ta książka jest już pożyczona (" + istn->tekst(0, 0) + "). Najpierw zarejestruj zwrot."; return false; }
    if (!pol->zapytanie("INSERT INTO tbl_pozyczki (id_ksiazki, osoba, data_pozyczenia, termin, uwagi) "
                        "VALUES ($1::bigint, $2, COALESCE($3::date, current_date), $4::date, $5)",
                        {std::to_string(id), kto, tekstLubNull(dataOd), tekstLubNull(dataTermin), tekstLubNull(uwagi)}))
        return ustawBladBazy(pol, "Nie udało się zapisać pożyczki.");
    return true;
}

bool LogikaKsiazek::zwroc(long long id, const std::string& data) {
    const std::string d = przytnij(data);
    if (!d.empty() && !LogikaNarzedzi::czyData(d)) { blad_ = "Data zwrotu musi mieć postać RRRR-MM-DD."; return false; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto odp = pol->zapytanie("UPDATE tbl_pozyczki SET data_zwrotu = GREATEST(COALESCE($2::date, current_date), data_pozyczenia) "
                              "WHERE id_ksiazki = $1::bigint AND data_zwrotu IS NULL RETURNING id",
                              {std::to_string(id), tekstLubNull(d)});
    if (!odp) return ustawBladBazy(pol, "Nie udało się zapisać zwrotu.");
    if (odp->liczbaWierszy() == 0) { blad_ = "Ta książka nie jest pożyczona."; return false; }
    return true;
}

bool LogikaKsiazek::historia(long long id, std::vector<Pozyczka>& wynik) {
    wynik.clear();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto odp = pol->zapytanie(std::string("SELECT ") + KOLUMNY_POZYCZKI + " FROM tbl_pozyczki p JOIN tbl_ksiazki k ON k.id = p.id_ksiazki "
                              "WHERE ($1::bigint = 0 OR p.id_ksiazki = $1::bigint) ORDER BY p.data_pozyczenia DESC, p.id DESC LIMIT 2000",
                              {std::to_string(id)});
    if (!odp) return ustawBladBazy(pol, "Nie udało się pobrać historii pożyczek.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back(pozyczkaZWiersza(*odp, i));
    return true;
}

bool LogikaKsiazek::przeterminowane(std::vector<Pozyczka>& wynik) {
    wynik.clear();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto odp = pol->zapytanie(std::string("SELECT ") + KOLUMNY_POZYCZKI + " FROM tbl_pozyczki p JOIN tbl_ksiazki k ON k.id = p.id_ksiazki "
                              "WHERE p.data_zwrotu IS NULL AND p.termin IS NOT NULL AND p.termin < current_date ORDER BY p.termin, p.id");
    if (!odp) return ustawBladBazy(pol, "Nie udało się pobrać przeterminowanych pożyczek.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back(pozyczkaZWiersza(*odp, i));
    return true;
}
