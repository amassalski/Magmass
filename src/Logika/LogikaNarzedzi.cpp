#include "LogikaNarzedzi.h"
#include "../Narzedzia/Liczby.h"

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
    "n.id, n.nazwa, n.producent, n.model, n.kategoria, n.numer_seryjny, n.numer_inwentarzowy, n.miejsce, "
    "COALESCE(to_char(n.data_zakupu, 'YYYY-MM-DD'), '') AS data_zakupu, n.cena, n.zdjecie, n.instrukcja, n.uwagi, "
    "p.osoba AS poz_osoba, COALESCE(to_char(p.data_pozyczenia, 'YYYY-MM-DD'), '') AS poz_od, "
    "COALESCE(to_char(p.termin, 'YYYY-MM-DD'), '') AS poz_termin, "
    "(p.id IS NOT NULL) AS poz_jest, (p.termin IS NOT NULL AND p.termin < current_date) AS poz_po_terminie";
const char* ZRODLO = "tbl_narzedzia n LEFT JOIN tbl_pozyczki p ON p.id_narzedzia = n.id AND p.data_zwrotu IS NULL";

Narzedzie narzedzieZWiersza(const WynikSQL& w, int i) {
    Narzedzie n;
    n.id = w.liczba(i, w.indeksKolumny("id"));
    n.nazwa = w.tekst(i, "nazwa");
    n.producent = w.tekst(i, "producent");
    n.model = w.tekst(i, "model");
    n.kategoria = w.tekst(i, "kategoria");
    n.numerSeryjny = w.tekst(i, "numer_seryjny");
    n.numerInwentarzowy = w.tekst(i, "numer_inwentarzowy");
    n.miejsce = w.tekst(i, "miejsce");
    n.dataZakupu = w.tekst(i, "data_zakupu");
    const int kc = w.indeksKolumny("cena");
    if (!w.czyNull(i, kc)) n.cena = w.rzeczywista(i, kc);
    n.zdjecie = w.tekst(i, "zdjecie");
    n.instrukcja = w.tekst(i, "instrukcja");
    n.uwagi = w.tekst(i, "uwagi");
    n.pozyczone = w.tekst(i, "poz_jest") == "t";
    if (n.pozyczone) {
        n.pozyczonePrzez = w.tekst(i, "poz_osoba");
        n.pozyczoneOd = w.tekst(i, "poz_od");
        n.termin = w.tekst(i, "poz_termin");
        n.przeterminowane = w.tekst(i, "poz_po_terminie") == "t";
    }
    return n;
}

Pozyczka pozyczkaZWiersza(const WynikSQL& w, int i) {
    Pozyczka p;
    p.id = w.liczba(i, w.indeksKolumny("id"));
    p.idNarzedzia = w.liczba(i, w.indeksKolumny("id_narzedzia"));
    p.nazwaNarzedzia = w.tekst(i, "nazwa");
    p.osoba = w.tekst(i, "osoba");
    p.od = w.tekst(i, "od");
    p.termin = w.tekst(i, "termin");
    p.zwrot = w.tekst(i, "zwrot");
    p.uwagi = w.tekst(i, "uwagi");
    p.przeterminowana = w.tekst(i, "po_terminie") == "t";
    return p;
}

const char* KOLUMNY_POZYCZKI =
    "p.id, p.id_narzedzia, n.nazwa, p.osoba, to_char(p.data_pozyczenia, 'YYYY-MM-DD') AS od, "
    "COALESCE(to_char(p.termin, 'YYYY-MM-DD'), '') AS termin, COALESCE(to_char(p.data_zwrotu, 'YYYY-MM-DD'), '') AS zwrot, "
    "COALESCE(p.uwagi, '') AS uwagi, (p.data_zwrotu IS NULL AND p.termin IS NOT NULL AND p.termin < current_date) AS po_terminie";

}  // namespace

bool LogikaNarzedzi::czyData(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (size_t i : {0, 1, 2, 3, 5, 6, 8, 9}) if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    const int r = std::stoi(s.substr(0, 4)), m = std::stoi(s.substr(5, 2)), d = std::stoi(s.substr(8, 2));
    if (r < 1900 || r > 2200 || m < 1 || m > 12 || d < 1) return false;
    static const int dni[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool przestepny = (r % 4 == 0 && r % 100 != 0) || r % 400 == 0;
    return d <= dni[m - 1] + ((m == 2 && przestepny) ? 1 : 0);
}

PolaczenieBazy* LogikaNarzedzi::polaczenie() {
    PolaczenieBazy* pol = manager_.pobierzPolaczenie();
    if (!pol || !pol->czyPolaczony()) {
        blad_ = "Brak połączenia z bazą danych.";
        return nullptr;
    }
    return pol;
}

bool LogikaNarzedzi::ustawBladBazy(PolaczenieBazy* pol, const std::string& zaZnaczeniem) {
    const std::string blad = pol->pobierzBlad();
    blad_ = zaZnaczeniem.empty() ? blad : zaZnaczeniem + "\n" + blad;
    return false;
}

bool LogikaNarzedzi::pobierz(const FiltrNarzedzi& f, std::vector<Narzedzie>& wynik) {
    wynik.clear();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    std::string sql = std::string("SELECT ") + KOLUMNY + " FROM " + ZRODLO + " WHERE TRUE ";
    std::vector<Parametr> p;
    const std::string t = przytnij(f.tekst);
    if (!t.empty()) {
        p.push_back("%" + t + "%");
        sql += "AND (n.nazwa ILIKE $1 OR n.producent ILIKE $1 OR n.model ILIKE $1 OR n.kategoria ILIKE $1 OR n.numer_seryjny ILIKE $1 OR n.numer_inwentarzowy ILIKE $1 "
               "OR n.miejsce ILIKE $1 OR n.uwagi ILIKE $1 OR p.osoba ILIKE $1) ";
    }
    if (f.tylkoPrzeterminowane) sql += "AND p.termin IS NOT NULL AND p.termin < current_date ";
    else if (f.tylkoPozyczone) sql += "AND p.id IS NOT NULL ";
    sql += "ORDER BY lower(n.nazwa), n.id LIMIT 5000";
    auto odp = pol->zapytanie(sql, p);
    if (!odp) return ustawBladBazy(pol, "Nie udało się pobrać narzędzi.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back(narzedzieZWiersza(*odp, i));
    return true;
}

std::optional<Narzedzie> LogikaNarzedzi::pobierzNarzedzie(long long id) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;
    auto odp = pol->zapytanie(std::string("SELECT ") + KOLUMNY + " FROM " + ZRODLO + " WHERE n.id = $1::bigint", {std::to_string(id)});
    if (!odp) { ustawBladBazy(pol); return std::nullopt; }
    if (odp->liczbaWierszy() != 1) { blad_ = "Narzędzie nie istnieje."; return std::nullopt; }
    return narzedzieZWiersza(*odp, 0);
}

std::optional<long long> LogikaNarzedzi::zapisz(const Narzedzie& n) {
    if (przytnij(n.nazwa).empty()) { blad_ = "Podaj nazwę narzędzia."; return std::nullopt; }
    const std::string zakup = przytnij(n.dataZakupu);
    if (!zakup.empty() && !czyData(zakup)) { blad_ = "Data zakupu musi mieć postać RRRR-MM-DD."; return std::nullopt; }
    if (n.cena && *n.cena < 0) { blad_ = "Cena nie może być ujemna."; return std::nullopt; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return std::nullopt;
    Parametr cena = n.cena ? Parametr(liczbaNaTekstKlasyczny(*n.cena)) : std::nullopt;
    if (n.id == 0) {
        auto odp = pol->zapytanie(
            "INSERT INTO tbl_narzedzia (nazwa, producent, model, kategoria, numer_seryjny, miejsce, data_zakupu, cena, zdjecie, instrukcja, uwagi, numer_inwentarzowy) "
            "VALUES ($1,$2,$3,$4,$5,$6,$7::date,$8::numeric,$9,$10,$11,$12) RETURNING id",
            {przytnij(n.nazwa), tekstLubNull(n.producent), tekstLubNull(n.model), tekstLubNull(n.kategoria), tekstLubNull(n.numerSeryjny),
             tekstLubNull(n.miejsce), tekstLubNull(zakup), cena, tekstLubNull(n.zdjecie), tekstLubNull(n.instrukcja), tekstLubNull(n.uwagi), tekstLubNull(n.numerInwentarzowy)});
        if (!odp || odp->liczbaWierszy() != 1) { ustawBladBazy(pol, "Nie udało się dodać narzędzia."); return std::nullopt; }
        return odp->liczba(0, 0);
    }
    if (!pol->zapytanie(
            "UPDATE tbl_narzedzia SET nazwa=$2, producent=$3, model=$4, kategoria=$5, numer_seryjny=$6, miejsce=$7, data_zakupu=$8::date, "
            "cena=$9::numeric, zdjecie=$10, instrukcja=$11, uwagi=$12, numer_inwentarzowy=$13, zmieniono=current_timestamp WHERE id=$1::bigint",
            {std::to_string(n.id), przytnij(n.nazwa), tekstLubNull(n.producent), tekstLubNull(n.model), tekstLubNull(n.kategoria),
             tekstLubNull(n.numerSeryjny), tekstLubNull(n.miejsce), tekstLubNull(zakup), cena, tekstLubNull(n.zdjecie),
             tekstLubNull(n.instrukcja), tekstLubNull(n.uwagi), tekstLubNull(n.numerInwentarzowy)})) {
        ustawBladBazy(pol, "Nie udało się zapisać narzędzia.");
        return std::nullopt;
    }
    return n.id;
}

bool LogikaNarzedzi::usun(long long id) {
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    if (!pol->zapytanie("DELETE FROM tbl_narzedzia WHERE id = $1::bigint", {std::to_string(id)}))
        return ustawBladBazy(pol, "Nie udało się usunąć narzędzia.");
    return true;
}

bool LogikaNarzedzi::importuj(const std::vector<Narzedzie>& narzedzia, WynikImportuNarzedzi& wynik) {
    wynik = WynikImportuNarzedzi();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    for (const Narzedzie& n : narzedzia) {
        if (przytnij(n.nazwa).empty()) { wynik.bledy.push_back("Pozycja bez nazwy pominięta."); continue; }
        auto istn = pol->zapytanie(
            "SELECT 1 FROM tbl_narzedzia WHERE lower(nazwa) = lower($1) AND lower(COALESCE(model, '')) = lower($2) "
            "AND lower(COALESCE(numer_seryjny, '')) = lower($3) LIMIT 1",
            {przytnij(n.nazwa), przytnij(n.model), przytnij(n.numerSeryjny)});
        if (!istn) return ustawBladBazy(pol);
        if (istn->liczbaWierszy() > 0) { ++wynik.pominieto; continue; }
        Narzedzie kopia = n;
        kopia.id = 0;
        if (zapisz(kopia)) ++wynik.dodano;
        else wynik.bledy.push_back(n.nazwa + ": " + blad_);
    }
    return true;
}

std::vector<std::string> LogikaNarzedzi::kategorie() {
    std::vector<std::string> w;
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return w;
    auto odp = pol->zapytanie("SELECT DISTINCT kategoria FROM tbl_narzedzia WHERE COALESCE(kategoria, '') <> '' ORDER BY kategoria");
    if (!odp) return w;
    for (int i = 0; i < odp->liczbaWierszy(); ++i) w.push_back(odp->tekst(i, 0));
    return w;
}

bool LogikaNarzedzi::pozycz(long long id, const std::string& osoba, const std::string& od, const std::string& termin, const std::string& uwagi) {
    const std::string kto = przytnij(osoba), dataOd = przytnij(od), dataTermin = przytnij(termin);
    if (kto.empty()) { blad_ = "Podaj, komu pożyczasz narzędzie."; return false; }
    if (!dataOd.empty() && !czyData(dataOd)) { blad_ = "Data pożyczenia musi mieć postać RRRR-MM-DD."; return false; }
    if (!dataTermin.empty() && !czyData(dataTermin)) { blad_ = "Termin zwrotu musi mieć postać RRRR-MM-DD."; return false; }
    if (!dataTermin.empty() && !dataOd.empty() && dataTermin < dataOd) { blad_ = "Termin zwrotu nie może być wcześniejszy niż data pożyczenia."; return false; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto istn = pol->zapytanie("SELECT osoba FROM tbl_pozyczki WHERE id_narzedzia = $1::bigint AND data_zwrotu IS NULL", {std::to_string(id)});
    if (!istn) return ustawBladBazy(pol);
    if (istn->liczbaWierszy() > 0) { blad_ = "To narzędzie jest już pożyczone (" + istn->tekst(0, 0) + "). Najpierw zarejestruj zwrot."; return false; }
    if (!pol->zapytanie("INSERT INTO tbl_pozyczki (id_narzedzia, osoba, data_pozyczenia, termin, uwagi) "
                        "VALUES ($1::bigint, $2, COALESCE($3::date, current_date), $4::date, $5)",
                        {std::to_string(id), kto, tekstLubNull(dataOd), tekstLubNull(dataTermin), tekstLubNull(uwagi)}))
        return ustawBladBazy(pol, "Nie udało się zapisać pożyczki.");
    return true;
}

bool LogikaNarzedzi::zwroc(long long id, const std::string& data) {
    const std::string d = przytnij(data);
    if (!d.empty() && !czyData(d)) { blad_ = "Data zwrotu musi mieć postać RRRR-MM-DD."; return false; }
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto odp = pol->zapytanie("UPDATE tbl_pozyczki SET data_zwrotu = GREATEST(COALESCE($2::date, current_date), data_pozyczenia) "
                              "WHERE id_narzedzia = $1::bigint AND data_zwrotu IS NULL RETURNING id",
                              {std::to_string(id), tekstLubNull(d)});
    if (!odp) return ustawBladBazy(pol, "Nie udało się zapisać zwrotu.");
    if (odp->liczbaWierszy() == 0) { blad_ = "To narzędzie nie jest pożyczone."; return false; }
    return true;
}

bool LogikaNarzedzi::historia(long long id, std::vector<Pozyczka>& wynik) {
    wynik.clear();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto odp = pol->zapytanie(std::string("SELECT ") + KOLUMNY_POZYCZKI + " FROM tbl_pozyczki p JOIN tbl_narzedzia n ON n.id = p.id_narzedzia "
                              "WHERE ($1::bigint = 0 OR p.id_narzedzia = $1::bigint) ORDER BY p.data_pozyczenia DESC, p.id DESC LIMIT 2000",
                              {std::to_string(id)});
    if (!odp) return ustawBladBazy(pol, "Nie udało się pobrać historii pożyczek.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back(pozyczkaZWiersza(*odp, i));
    return true;
}

bool LogikaNarzedzi::przeterminowane(std::vector<Pozyczka>& wynik) {
    wynik.clear();
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return false;
    auto odp = pol->zapytanie(std::string("SELECT ") + KOLUMNY_POZYCZKI + " FROM tbl_pozyczki p JOIN tbl_narzedzia n ON n.id = p.id_narzedzia "
                              "WHERE p.data_zwrotu IS NULL AND p.termin IS NOT NULL AND p.termin < current_date ORDER BY p.termin, p.id");
    if (!odp) return ustawBladBazy(pol, "Nie udało się pobrać przeterminowanych pożyczek.");
    for (int i = 0; i < odp->liczbaWierszy(); ++i) wynik.push_back(pozyczkaZWiersza(*odp, i));
    return true;
}

std::vector<std::string> LogikaNarzedzi::osoby() {
    std::vector<std::string> w;
    PolaczenieBazy* pol = polaczenie();
    if (!pol) return w;
    auto odp = pol->zapytanie("SELECT osoba FROM tbl_pozyczki GROUP BY osoba ORDER BY max(data_pozyczenia) DESC, osoba LIMIT 200");
    if (!odp) return w;
    for (int i = 0; i < odp->liczbaWierszy(); ++i) w.push_back(odp->tekst(i, 0));
    return w;
}
