#include "OdczytDatasheetu.h"


#include <algorithm>
#include <regex>
#include <set>
#include <sstream>

namespace OdczytDatasheetu {
namespace {

std::vector<std::string> linie(const std::string& t) {
    std::vector<std::string> w;
    std::string l;
    std::istringstream s(t);
    while (std::getline(s, l)) {
        while (!l.empty() && (l.back() == '\r' || l.back() == ' ' || l.back() == '\t')) l.pop_back();
        size_t p = 0;
        while (p < l.size() && (l[p] == ' ' || l[p] == '\t')) ++p;
        w.push_back(l.substr(p));
    }
    return w;
}

std::string maleLitery(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// ujednolica znaki: minus Unicode, stopnie, "–"
std::string normalizuj(std::string s) {
    auto zamien = [&](const std::string& co, const std::string& naCo) {
        size_t p = 0;
        while ((p = s.find(co, p)) != std::string::npos) { s.replace(p, co.size(), naCo); p += naCo.size(); }
    };
    zamien("\xE2\x88\x92", "-");   // −
    zamien("\xE2\x80\x93", "-");   // –
    zamien("\xE2\x80\x94", "-");   // —
    zamien("\xC2\xB0", " deg");    // °
    zamien("\xCB\x9A", " deg");    // ˚ (kółko nad literą, często zamiast stopnia)
    zamien("\xEF\xBC\x88", "(");   // （
    zamien("\xEF\xBC\x89", ")");   // ）
    zamien("\xE2\x84\x83", " degC"); // ℃
    zamien("\xC2\xA0", " ");
    zamien("\xCE\xBC", "\xC2\xB5");   // μ (grecka mi) -> µ
    zamien("\xCE\xA9", " ohm");        // Ω
    zamien("\xE2\x84\xA6", " ohm");   // Ω (znak om)
    zamien("\xE2\x80\xA2", "- ");     // •
    zamien("\xEF\x82\xB7", "- ");     // punktor z czcionki Symbol
    return s;
}

bool zawiera(const std::string& l, std::initializer_list<const char*> slowa) {
    const std::string m = maleLitery(l);
    for (const char* s : slowa) if (m.find(s) != std::string::npos) return true;
    return false;
}

// Karty w układzie dwukolumnowym (pdftotext -layout) sklejają kolumny dużymi odstępami: bierze tekst do pierwszego odstępu >= @p odstep spacji
std::string pierwszaKolumna(const std::string& l, size_t odstep = 4) {
    const std::string luka(odstep, ' ');
    const size_t p = l.find(luka);
    return p == std::string::npos ? l : l.substr(0, p);
}

std::string skroc(const std::string& s, size_t n = 140) { return s.size() <= n ? s : s.substr(0, n) + "..."; }

void dodaj(Wynik& w, std::set<std::string>& mamy, const std::string& nazwa, const std::string& wart, const std::string& frag, bool pewny = true) {
    if (mamy.count(nazwa)) return;
    mamy.insert(nazwa);
    w.parametry.push_back({nazwa, wart, skroc(frag), pewny});
}


// ===================== sekcje tabel parametrów =====================

enum class Grupa { Brak, Maks, Elektr };

struct Wzor {
    const char* klucz;     // wyrażenie regularne (bez rozróżniania wielkości liter)
    const char* nazwa;     // polska nazwa
    const char* symbol;    // symbol z kart katalogowych
    bool bezJednostki;     // true np. dla hFE
};

const Wzor kMaks[] = {
    {R"(collector[- ]emitter (breakdown )?voltage|\bV\s?\(?BR\)?\s?CEO\b|\bV\s?CEO\b)", "Napięcie kolektor-emiter", "VCEO", false},
    {R"(collector[- ]base voltage|\bV\s?CBO\b)", "Napięcie kolektor-baza", "VCBO", false},
    {R"(emitter[- ]base voltage|\bV\s?EBO\b)", "Napięcie emiter-baza", "VEBO", false},
    {R"(peak collector current|\bI\s?CM\b)", "Prąd kolektora szczytowy", "ICM", false},
    {R"(collector current|\bI\s?C\b)", "Prąd kolektora", "IC", false},
    {R"(base current|\bI\s?B\b)", "Prąd bazy", "IB", false},
    {R"(drain[- ]source voltage|\bV\s?DSS?\b)", "Napięcie dren-źródło", "VDS", false},
    {R"(gate[- ]source voltage|\bV\s?GSS?\b)", "Napięcie bramka-źródło", "VGS", false},
    {R"(pulsed drain current|\bI\s?DM\b)", "Prąd drenu impulsowy", "IDM", false},
    {R"(drain current|\bI\s?D\b)", "Prąd drenu", "ID", false},
    {R"(repetitive peak reverse voltage|\bV\s?RRM\b|\bV\s?RM\b)", "Napięcie wsteczne powtarzalne", "VRRM", false},
    {R"(reverse voltage|\bV\s?R\b)", "Napięcie wsteczne", "VR", false},
    {R"(surge|\bI\s?FSM\b)", "Prąd udarowy", "IFSM", false},
    {R"(average (rectified )?(forward )?current|\bI\s?F\s?\(AV\)|\bI\s?O\b|output current)", "Prąd średni / wyjściowy", "IO", false},
    {R"(forward current|\bI\s?F\b)", "Prąd przewodzenia", "IF", false},
    {R"(supply voltage|\bV\s?CC\b|\bV\s?DD\b|\bV\s?S\b)", "Napięcie zasilania", "VCC", false},
    {R"(input voltage|\bV\s?IN\b|\bV\s?I\b)", "Napięcie wejściowe", "VIN", false},
    {R"(power dissipation|total dissipation|device dissipation|\bP\s?tot\b|\bP\s?D\b)", "Moc admisyjna", "Ptot", false},
    {R"(storage temperature|junction temperature|\bT\s?stg\b|\bT\s?J\b)", "Temperatura złącza / przechowywania", "Tj, Tstg", false},
    {R"(operating (free[- ]air |ambient )?temperature|ambient temperature|\bT\s?A\b)", "Temperatura pracy", "TA", false},
    {R"(thermal resistance|\bR\s?(th|θ)\s?J\s?[AC]\b)", "Rezystancja cieplna", "Rth", false},
};

const Wzor kElektr[] = {
    {R"(\bh\s?FE\b|\bh\s?fe\b|dc current gain|current gain)", "Wzmocnienie prądowe", "hFE", true},
    {R"(collector[- ]emitter saturation voltage|\bV\s?CE\s?\(sat\))", "Napięcie nasycenia C-E", "VCE(sat)", false},
    {R"(base[- ]emitter (saturation |on )?voltage|\bV\s?BE\s?\((sat|on)\))", "Napięcie baza-emiter", "VBE", false},
    {R"(collector cut-?off current|\bI\s?CBO\b|\bI\s?CEO\b|\bI\s?CES\b)", "Prąd zerowy kolektora", "ICBO", false},
    {R"(transition frequency|current[- ]gain bandwidth|gain[- ]bandwidth|\bf\s?T\b|\bGBW\b)", "Częstotliwość graniczna", "fT", false},
    {R"(output capacitance|\bC\s?ob[o]?\b)", "Pojemność wyjściowa", "Cob", false},
    {R"(input capacitance|\bC\s?ib[o]?\b|\bC\s?iss\b)", "Pojemność wejściowa", "Cib", false},
    {R"(noise figure|\bNF\b)", "Współczynnik szumów", "NF", false},
    {R"(drain[- ]source on[- ]resistance|on[- ]resistance|\bR\s?DS\s?\(on\))", "Rezystancja w stanie włączenia", "RDS(on)", false},
    {R"(gate threshold voltage|\bV\s?GS\s?\(th\))", "Napięcie progowe bramki", "VGS(th)", false},
    {R"(zero gate voltage drain current|\bI\s?DSS\b)", "Prąd drenu IDSS", "IDSS", false},
    {R"(forward transconductance|\bg\s?fs\b)", "Transkonduktancja", "gfs", false},
    {R"(zener voltage|\bV\s?Z\b)", "Napięcie Zenera", "VZ", false},
    {R"(forward voltage|\bV\s?F\b)", "Napięcie przewodzenia", "VF", false},
    {R"(reverse (leakage )?current|\bI\s?R\b)", "Prąd wsteczny", "IR", false},
    {R"(reverse recovery time|\bt\s?rr\b)", "Czas wstecznego wyłączania", "trr", false},
    {R"(quiescent current|supply current|\bI\s?Q\b|\bI\s?CC\b|\bI\s?DD\b)", "Prąd zasilania", "ICC", false},
    {R"(dropout voltage|\bV\s?DO\b)", "Spadek napięcia (dropout)", "Vdo", false},
    {R"(reference voltage|\bV\s?REF\b)", "Napięcie odniesienia", "VREF", false},
    {R"(output voltage|\bV\s?OUT\b|\bV\s?O\b)", "Napięcie wyjściowe", "VOUT", false},
    {R"(input offset voltage|\bV\s?IO\b|\bV\s?OS\b)", "Napięcie niezrównoważenia", "VIO", false},
    {R"(input bias current|\bI\s?IB\b|\bI\s?B\b)", "Prąd polaryzacji wejścia", "IIB", false},
    {R"(slew rate|\bSR\b)", "Szybkość narastania", "SR", false},
    {R"(common[- ]mode rejection|\bCMRR\b)", "Tłumienie sygnału wspólnego", "CMRR", false},
    {R"(open[- ]loop (voltage )?gain|large[- ]signal voltage gain|\bA\s?VD\b|\bA\s?OL\b)", "Wzmocnienie w otwartej pętli", "AVD", false},
    {R"(propagation delay|\bt\s?PD\b|\bt\s?PLH\b|\bt\s?PHL\b)", "Opóźnienie propagacji", "tpd", false},
    {R"(rise time|\bt\s?r\b)", "Czas narastania", "tr", false},
    {R"(fall time|\bt\s?f\b)", "Czas opadania", "tf", false},
    {R"(input (logic )?high( level)? voltage|\bV\s?IH\b)", "Napięcie wejściowe stanu H", "VIH", false},
    {R"(input (logic )?low( level)? voltage|\bV\s?IL\b)", "Napięcie wejściowe stanu L", "VIL", false},
};

std::string zwin(const std::string& s) {
    std::string w;
    bool spacja = false;
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\f') { spacja = true; continue; }
        if (spacja && !w.empty()) w += ' ';
        spacja = false;
        w += c;
    }
    return w;
}

// Jednostka na pozycji (zaczyna się od @p od). Zwraca długość dopasowania (0 = brak jednostki).
size_t jednostkaOd(const std::string& s, size_t od, std::string& jedn) {
    static const std::regex re(
        R"((?:(?:\xC2\xB5|u|m|k|M|n|p|G)?(?:Hz|V|A|W|F|H)(?:dc|DC|rms|RMS|pk)?|(?:k|M)?\s?ohms?|dB|ppm|%|(?:\xC2\xB5|u|m|n)?s|deg\s?C|degC|C)(?![A-Za-z]))");
    std::smatch m;
    auto b = s.begin() + static_cast<std::ptrdiff_t>(od);
    if (!std::regex_search(b, s.end(), m, re, std::regex_constants::match_continuous)) return 0;
    std::string j = m[0].str();
    if (j == "C") return 0;   // pojedyncze "C" bez "deg" to raczej symbol, nie stopnie
    if (j.find("deg") == 0) j = "\xC2\xB0" "C";
    else if (j.find("ohm") != std::string::npos) {
        j = (j[0] == 'k' ? "k" : j[0] == 'M' ? "M" : "") + std::string("\xCE\xA9");
    } else {
        for (const char* suf : {"dc", "DC", "rms", "RMS", "pk"}) {
            const size_t n = std::char_traits<char>::length(suf);
            if (j.size() > n && j.compare(j.size() - n, n, suf) == 0 && j.size() - n >= 1 && (j[j.size() - n - 1] == 'V' || j[j.size() - n - 1] == 'A'))
                j.erase(j.size() - n);
        }
    }
    jedn = j;
    return m[0].length();
}

// Wartość z wiersza tabeli: usuwa warunki pomiaru ("IC = 10 mA"), przypisy i nazwę parametru,
// a z reszty składa "min ... max jednostka" albo "min / typ / max jednostka".
bool wartoscZWiersza(const std::string& wiersz, const std::regex& klucz, bool bezJednostki, std::string& wynik) {
    static const std::regex warunek(R"([A-Za-z][A-Za-z0-9()]{0,9}\s*=\s*[-+]?[0-9]+(?:[.,][0-9]+)?\s*(?:[A-Za-z\xC2\xB5]+(?:\s?deg\s?C)?)?\s*,?)");
    static const std::regex przypis(R"(\(\s*note\s*[0-9]+\s*\)|\bnote\s*[0-9]+\b|\b[0-9]\))", std::regex::icase);
    std::string t = wiersz;
    t = std::regex_replace(t, warunek, " ");
    t = std::regex_replace(t, przypis, " ");
    t = std::regex_replace(t, klucz, " ");

    struct Tok { std::string liczba, jedn; };
    std::vector<Tok> tok;
    static const std::regex num(R"([-+]?[0-9]+(?:[.,][0-9]+)?)");
    for (auto it = std::sregex_iterator(t.begin(), t.end(), num); it != std::sregex_iterator(); ++it) {
        const size_t poz = static_cast<size_t>(it->position());
        const size_t kon = poz + static_cast<size_t>(it->length());
        if (poz > 0 && (std::isalnum(static_cast<unsigned char>(t[poz - 1])) || t[poz - 1] == '_' || t[poz - 1] == '/')) continue;
        if (kon < t.size() && (std::isalpha(static_cast<unsigned char>(t[kon])) && t[kon] != 'm' && t[kon] != 'k' && t[kon] != 'u' && t[kon] != 'M' && t[kon] != 'n' && t[kon] != 'p' && t[kon] != 'V' && t[kon] != 'A' && t[kon] != 'W' && t[kon] != 'H' && t[kon] != 'F' && t[kon] != 'd' && t[kon] != '%')) continue;
        size_t k = kon;
        while (k < t.size() && t[k] == ' ') ++k;
        Tok x{it->str(), ""};
        const size_t dl = jednostkaOd(t, k, x.jedn);
        (void)dl;
        // przecinek dziesiętny -> kropka? zostaw tak jak w dokumencie, ale unikaj tysięcy "1,000"
        tok.push_back(x);
    }
    if (tok.empty()) return false;
    // jednostka dla liczb bez własnej: najbliższa następna, a gdy brak - ostatnia
    std::string ost;
    for (size_t i = tok.size(); i-- > 0;) { if (tok[i].jedn.empty()) tok[i].jedn = ost; else ost = tok[i].jedn; }
    if (!bezJednostki && ost.empty()) return false;
    if (tok.size() > 3) tok.resize(3);
    std::string j = tok.back().jedn;
    bool jednakowa = true;
    for (const auto& x : tok) if (x.jedn != j) jednakowa = false;
    std::string w;
    if (jednakowa) {
        if (tok.size() == 1) w = tok[0].liczba;
        else if (tok.size() == 2) w = tok[0].liczba + " ... " + tok[1].liczba;
        else w = tok[0].liczba + " / " + tok[1].liczba + " / " + tok[2].liczba;
        if (!j.empty()) w += " " + j;
    } else {
        for (size_t i = 0; i < tok.size(); ++i) {
            if (i) w += tok.size() == 2 ? " ... " : " / ";
            w += tok[i].liczba + (tok[i].jedn.empty() ? "" : " " + tok[i].jedn);
        }
    }
    wynik = w;
    return true;
}

bool naglowekSekcji(const std::string& l, Grupa& g, bool& koniec) {
    static const std::regex pocz(R"(^(?:table\s*[0-9]+[.:]?\s*|[0-9]+(?:\.[0-9]+)*\.?\s*)?)", std::regex::icase);
    static const std::regex maks(R"(^(?:absolute )?maximum ratings?\b|^limiting values\b|^absolute maximum\b)", std::regex::icase);
    static const std::regex elektr(R"(^(?:electrical|dc|ac|static|dynamic|operating|recommended operating|switching) characteristics?\b|^characteristics\b|^electrical specifications?\b)", std::regex::icase);
    static const std::regex stop(R"(^(?:typical (?:performance |operating )?(?:characteristics|curves|applications?)|ordering information|package (?:information|outline|dimensions|drawing)|pin (?:configuration|description|assignment|functions)|mechanical data|revision history|marking|tape and reel|application information|block diagram|physical dimensions)\b)", std::regex::icase);
    koniec = false;
    if (l.size() > 100) return false;
    std::smatch m;
    std::string reszta = l;
    if (std::regex_search(l, m, pocz)) reszta = l.substr(static_cast<size_t>(m[0].length()));
    if (std::regex_search(reszta, maks)) { g = Grupa::Maks; return true; }
    if (std::regex_search(reszta, elektr)) { g = Grupa::Elektr; return true; }
    if (std::regex_search(reszta, stop)) { g = Grupa::Brak; koniec = true; return true; }
    return false;
}

void odczytajTabele(const std::vector<std::string>& L, Wynik& w, std::set<std::string>& mamy) {
    struct Skomp { std::regex re; const Wzor* wz; };
    auto skompiluj = [](const Wzor* tab, size_t n) {
        std::vector<Skomp> v;
        for (size_t i = 0; i < n; ++i) v.push_back({std::regex(tab[i].klucz, std::regex::icase), &tab[i]});
        return v;
    };
    static const std::vector<Skomp> wMaks = skompiluj(kMaks, sizeof(kMaks) / sizeof(kMaks[0]));
    static const std::vector<Skomp> wElektr = skompiluj(kElektr, sizeof(kElektr) / sizeof(kElektr[0]));

    Grupa g = Grupa::Brak;
    int wSekcji = 0, ogolnych = 0;
    for (const std::string& surowa : L) {
        Grupa nowa = g;
        bool koniec = false;
        if (naglowekSekcji(surowa, nowa, koniec)) { g = nowa; wSekcji = 0; continue; }
        if (g == Grupa::Brak) continue;
        if (++wSekcji > 220) { g = Grupa::Brak; continue; }
        const std::string l = zwin(surowa);
        if (l.size() < 6) continue;
        const std::string przedrostek = g == Grupa::Maks ? "Maks.: " : "Elektr.: ";
        bool znane = false;
        for (const Skomp& s : (g == Grupa::Maks ? wMaks : wElektr)) {
            if (!std::regex_search(l, s.re)) continue;
            znane = true;
            std::string wart;
            if (!wartoscZWiersza(l, s.re, s.wz->bezJednostki, wart)) break;
            const std::string nazwa = przedrostek + s.wz->nazwa + " (" + s.wz->symbol + ")";
            dodaj(w, mamy, nazwa, wart, l, true);
            break;
        }
        if (znane || ogolnych >= 25) continue;
        // ogólne zgadywanie: tekst z początku wiersza + liczba z jednostką
        std::string nazwa;
        for (size_t i = 0; i < l.size() && nazwa.size() < 42; ++i) {
            if (std::isdigit(static_cast<unsigned char>(l[i])) || l[i] == '=' || l[i] == '@') break;
            if (l[i] == ' ' && i + 1 < l.size() && l[i + 1] == ' ') break;
            nazwa += l[i];
        }
        while (!nazwa.empty() && (nazwa.back() == ' ' || nazwa.back() == ',' || nazwa.back() == '-' || nazwa.back() == '(')) nazwa.pop_back();
        int litery = 0;
        for (unsigned char c : nazwa) if (std::isalpha(c)) ++litery;
        if (litery < 4 || !std::isalpha(static_cast<unsigned char>(nazwa[0]))) continue;
        if (zawiera(nazwa, {"rating", "symbol", "unit", "value", "note", "characteristic", "parameter", "test condition", "min", "typ", "max"}) && litery < 12) continue;
        static const std::regex niczego("^$");
        std::string wart;
        if (!wartoscZWiersza(l, niczego, false, wart)) continue;
        dodaj(w, mamy, przedrostek + nazwa, wart, l, false);
        ++ogolnych;
    }
}

// ===================== uwagi: opis zastosowania =====================

bool tytulInnejSekcji(const std::string& l) {
    static const std::regex re(R"(^(?:[0-9]+\.?\s*)?(features?|key features|description|general description|ordering|order information|pin |absolute|electrical|block diagram|package|typical|specifications?|contents|revision|maximum ratings|benefits|overview|applications? information|functional|climatic|construction|terminals|marking|delivery|dimension)\b)", std::regex::icase);
    return std::regex_search(l, re);
}

std::string akapitPoNaglowku(const std::vector<std::string>& L, const std::regex& naglowek) {
    for (size_t i = 0; i < L.size(); ++i) {
        std::smatch m;
        const std::string l = zwin(pierwszaKolumna(L[i]));
        if (l.size() > 140 || !std::regex_search(l, m, naglowek)) continue;
        std::string tekst = l.substr(static_cast<size_t>(m[0].length()));
        while (!tekst.empty() && (tekst[0] == ':' || tekst[0] == ' ')) tekst.erase(0, 1);
        int puste = 0, wierszy = 0;
        for (size_t j = i + 1; j < L.size() && wierszy < 10 && tekst.size() < 500; ++j) {
            const std::string x = zwin(pierwszaKolumna(L[j]));
            if (x.empty()) { if (++puste >= 2 || (!tekst.empty() && puste >= 1 && wierszy >= 3)) break; continue; }
            puste = 0;
            if (tytulInnejSekcji(x)) break;
            std::string y = x;
            if (y.size() > 2 && y[0] == '-' && y[1] == ' ') y = y.substr(2);
            if (!tekst.empty()) tekst += (tekst.back() == '.' || tekst.back() == ';' || tekst.back() == ':') ? " " : "; ";
            tekst += y;
            ++wierszy;
        }
        tekst = zwin(tekst);
        if (tekst.size() >= 12) return tekst.size() > 500 ? tekst.substr(0, 500) + "..." : tekst;
    }
    return {};
}

void odczytajUwagi(const std::vector<std::string>& L, Wynik& w) {
    static const std::regex zast(R"(^(?:[0-9]+\.?\s*)?(?:typical )?applications?\s*:?\s*)", std::regex::icase);
    static const std::regex opis(R"(^(?:[0-9]+\.?\s*)?(?:general )?description\s*:?\s*)", std::regex::icase);
    std::string t = akapitPoNaglowku(L, zast);
    if (!t.empty()) { w.uwagi = "Zastosowanie: " + t; return; }
    t = akapitPoNaglowku(L, opis);
    if (!t.empty()) w.uwagi = "Opis: " + t;
}


// ===================== kondensatory (elektrolityczne, foliowe, EMI, ceramiczne, tantalowe) =====================

std::vector<double> liczbyZ(const std::string& t) {
    std::vector<double> v;
    static const std::regex num(R"([0-9]+(?:\.[0-9]+)?)");
    for (auto it = std::sregex_iterator(t.begin(), t.end(), num); it != std::sregex_iterator(); ++it) v.push_back(std::stod(it->str()));
    return v;
}

std::string liczbaTekst(double x) {
    std::ostringstream o;
    o << x;
    return o.str();
}

std::string zakresTekst(const std::vector<double>& v, const std::string& jedn) {
    if (v.empty()) return {};
    const double a = *std::min_element(v.begin(), v.end()), b = *std::max_element(v.begin(), v.end());
    return a == b ? liczbaTekst(a) + " " + jedn : liczbaTekst(a) + " ... " + liczbaTekst(b) + " " + jedn;
}

// wyrażenie regularne budowane raz (static w lambdzie)
#define RE(...) ([]() -> const std::regex& { static const std::regex r(__VA_ARGS__); return r; }())

void odczytajKondensator(const std::vector<std::string>& L, Wynik& w, std::set<std::string>& mamy) {
    int slowKond = 0;
    for (const std::string& l : L) if (zawiera(l, {"capacitor", "capacitance"})) ++slowKond;
    if (slowKond < 3) return;

    // --- rodzaj kondensatora (pierwszy pasujący w kolejności ważności)
    std::string rodzaj, fragRodzaju;
    {
        struct R { const char* wzor; const char* nazwa; };
        static const R rodzaje[] = {
            {R"(alumin(?:um|ium)\s+electrolytic|electrolytic\s+capacitor)", "Elektrolityczny aluminiowy"},
            {R"(tantalum)", "Tantalowy"},
            {R"(\bMKP\b|polypropylene)", "Foliowy polipropylenowy (MKP)"},
            {R"(\bMKT\b|polyester|\bPET\b film)", "Foliowy poliestrowy (MKT)"},
            {R"(multilayer ceramic|\bMLCC\b|ceramic capacitor|\b(?:X7R|X5R|C0G|NP0)\b)", "Ceramiczny"},
            {R"(film capacitor)", "Foliowy"},
        };
        for (const R& r : rodzaje) {
            const std::regex re(r.wzor, std::regex::icase);
            for (const std::string& l : L) if (std::regex_search(l, re)) { rodzaj = r.nazwa; fragRodzaju = zwin(l); break; }
            if (!rodzaj.empty()) break;
        }
        if (!rodzaj.empty()) dodaj(w, mamy, "Rodzaj kondensatora", rodzaj, fragRodzaju);
    }

    std::string poj, tolerancja;
    bool maRatedV = false;
    std::string jednostkaCR;
    double minCR = 1e300, maxCR = -1;
    std::set<double> tol;
    std::vector<double> rozstaw, srednica;
    for (size_t i = 0; i < L.size(); ++i) {
        const std::string l = zwin(L[i]);
        if (l.empty()) continue;
        std::smatch m;

        // pojemność znamionowa (zakres)
        if (poj.empty() && std::regex_search(l, m, RE(R"(^(?:rated )?capacitance(?: range)?\s*:?\s*([0-9]+(?:[.,][0-9]+)?)\s*(?:to|-|\.\.\.|~)\s*([0-9]+(?:[.,][0-9]+)?)\s*(\xC2\xB5F|uF|nF|pF|mF)\b)", std::regex::icase))) {
            poj = m[1].str() + " ... " + m[2].str() + " " + m[3].str();
            dodaj(w, mamy, "Pojemność", poj, l);
        }
        // tabela zamawiania: kolumna CR, jednostka w nagłówku
        if (std::regex_search(l, RE(R"(\bCR\b)"))) {
            for (size_t j = i; j < L.size() && j < i + 3; ++j) {
                std::smatch mu;
                const std::string lj = zwin(L[j]);
                if (std::regex_search(lj, mu, RE(R"((\xC2\xB5F|uF|nF|pF)\b)"))) { jednostkaCR = mu[1].str(); break; }
            }
        }
        if (!jednostkaCR.empty() && std::regex_search(l, m, RE(R"(^(?:[0-9]+(?:\.[0-9]+)?\s+)?([0-9]+(?:[.,][0-9]+)?)\s+[0-9]+(?:[.,][0-9]+)?\s*(?:\xC3\x97|x)\s*[0-9])"))) {
            std::string c = m[1].str();
            std::replace(c.begin(), c.end(), ',', '.');
            const double v = std::stod(c);
            minCR = std::min(minCR, v);
            maxCR = std::max(maxCR, v);
        }
        // tolerancja
        if (tolerancja.empty() && std::regex_search(l, m, RE(R"(capacitance tolerance\s*:?\s*(?:\xC2\xB1|\+/-)\s*([0-9]+(?:[.,][0-9]+)?)\s*%)", std::regex::icase))) {
            tolerancja = "\xC2\xB1" + m[1].str() + " %";
            dodaj(w, mamy, "Tolerancja pojemności", tolerancja, l);
        }
        {   // kody tolerancji w numerze zamówieniowym: "M = ±20%", "K = ±10%"
            for (auto it = std::sregex_iterator(l.begin(), l.end(), RE(R"(\b[A-Z]\s*=\s*(?:\xC2\xB1|\+/-)\s*([0-9]+(?:\.[0-9]+)?)\s*%)")); it != std::sregex_iterator(); ++it)
                tol.insert(std::stod((*it)[1].str()));
        }
        // napięcie znamionowe
        if (!maRatedV && std::regex_search(l, m, RE(R"(^rated voltage(?: range)?\s*:?\s*([0-9]+(?:[.,][0-9]+)?)\s*(?:to|-|\.\.\.|~)\s*([0-9]+(?:[.,][0-9]+)?)\s*V\b)", std::regex::icase))) {
            maRatedV = true;
            dodaj(w, mamy, "Napięcie znamionowe", m[1].str() + " ... " + m[2].str() + " V", l);
        }
        if (std::regex_search(l, m, RE(R"(^rated (?:AC )?voltage[^0-9]{0,40}([0-9]+)\s*V\s*(?:\(?\s*(?:AC|50/60 ?Hz))?)", std::regex::icase)) &&
            zawiera(l, {"ac", "50/60"}))
            dodaj(w, mamy, "Napięcie znamionowe AC", m[1].str() + " V AC", l);
        if (std::regex_search(l, m, RE(R"(\bX[12]\b\s*/\s*([0-9]+)\s*V\s*AC)", std::regex::icase)))
            dodaj(w, mamy, "Napięcie znamionowe AC", m[1].str() + " V AC", l);
        if (std::regex_search(l, m, RE(R"(maximum continuous DC voltage[^0-9]{0,20}([0-9]+)\s*V)", std::regex::icase)))
            dodaj(w, mamy, "Maks. napięcie ciągłe DC", m[1].str() + " V", l);
        if (std::regex_search(l, m, RE(R"(maximum continuous AC voltage[^0-9]{0,20}([0-9]+)\s*V)", std::regex::icase)))
            dodaj(w, mamy, "Maks. napięcie ciągłe AC", m[1].str() + " V", l);
        if (std::regex_search(l, m, RE(R"(\b(X[12]|Y[1-4])\b[^.]{0,12}\bclass\b|\bclass\s+(X[12]|Y[1-4])\b)", std::regex::icase)))
            dodaj(w, mamy, "Klasa przeciwzakłóceniowa", m[1].matched ? m[1].str() : m[2].str(), l);
        if (std::regex_search(l, m, RE(R"(DC test voltage\s*:?\s*([0-9]+)\s*V\s*,?\s*([0-9]+)\s*s)", std::regex::icase)))
            dodaj(w, mamy, "Napięcie probiercze DC", m[1].str() + " V / " + m[2].str() + " s", l);

        // temperatura
        if (std::regex_search(l, m, RE(R"(max\.? operating temperature[^0-9+-]{0,20}\+?([0-9]{2,3})\s*deg\s*C)", std::regex::icase)))
            dodaj(w, mamy, "Maks. temperatura pracy", m[1].str() + " \xC2\xB0" "C", l);
        if (std::regex_search(l, m, RE(R"(climatic category.{0,30}?\b([0-9]{2})/([0-9]{2,3})/([0-9]{2,3}))", std::regex::icase))) {
            dodaj(w, mamy, "Kategoria klimatyczna", m[1].str() + "/" + m[2].str() + "/" + m[3].str(), l);
            dodaj(w, mamy, "Temperatura pracy", "-" + m[1].str() + " ... +" + m[2].str() + " \xC2\xB0" "C", l);
        }

        // wyprowadzenia, rozstaw, średnica
        if (std::regex_search(l, RE(R"(radial lead|parallel wire leads)", std::regex::icase)))
            dodaj(w, mamy, "Wyprowadzenia", "Radialne", l);
        if (rozstaw.empty() && std::regex_search(l, m, RE(R"(^lead spacing\s+((?:[0-9]+(?:\.[0-9]+)?\s*(?:mm)?\s*)+)$)", std::regex::icase)))
            rozstaw = liczbyZ(m[1].str());
        {
            const std::string kol = zwin(pierwszaKolumna(L[i], 20));
            if (rozstaw.empty() && std::regex_search(kol, m, RE(R"(^P\s+((?:[0-9]+(?:\.[0-9]+)?\s*)+)$)")))
                rozstaw = liczbyZ(m[1].str());
            if (srednica.empty() && std::regex_search(kol, m, RE(R"(^(?:\xCF\x86|\xC3\x98)D\s+((?:[0-9]+(?:\.[0-9]+)?\s*)+)$)")))
                srednica = liczbyZ(m[1].str());
        }

        // tg δ
        if (std::regex_search(l, RE(R"(^tan\s*\xCE\xB4)", std::regex::icase)) && zawiera(l, {"max"})) {
            std::vector<double> v;
            static const std::regex mal(R"(\b0[.,][0-9]+\b)");
            for (auto it = std::sregex_iterator(l.begin(), l.end(), mal); it != std::sregex_iterator(); ++it) { std::string t = it->str(); std::replace(t.begin(), t.end(), ',', '.'); v.push_back(std::stod(t)); }
            if (v.size() >= 2) dodaj(w, mamy, "Tangens kąta strat tg \xCE\xB4", zakresTekst(v, "(max., 120 Hz, 20 \xC2\xB0" "C)"), l);
        }
        if (zawiera(l, {"dissipation factor"}) && l.find("10-3") != std::string::npos) {
            for (size_t j = i + 1; j < L.size() && j < i + 4; ++j) {
                const std::string lj = zwin(L[j]);
                if (std::regex_search(lj, m, RE(R"(at\s+([0-9]+\s*kHz)\s+([0-9]+(?:\.[0-9]+)?(?:\s+[0-9]+(?:\.[0-9]+)?){0,2}))", std::regex::icase))) {
                    const std::vector<double> v = liczbyZ(m[2].str());
                    if (!v.empty()) dodaj(w, mamy, "Tangens kąta strat tg \xCE\xB4", zakresTekst(v, "x10^-3 (" + m[1].str() + ", 20 \xC2\xB0" "C, max.)"), lj);
                    break;
                }
            }
        }

        // prąd upływu (kondensatory elektrolityczne): "After 2 minutes ... 0.01CV"
        if (std::regex_search(l, m, RE(R"(after\s+([0-9]+)\s+minutes?.{0,120}?([0-9]*\.?[0-9]+)\s*CV)", std::regex::icase))) {
            const std::string wart = m[2].str() + "\xC2\xB7" "C\xC2\xB7" "V \xC2\xB5" "A po " + m[1].str() + " min";
            std::string nazwa = "Prąd upływu (po " + m[1].str() + " min)";
            dodaj(w, mamy, nazwa, "\xE2\x89\xA4 " + wart, l);
        }
        // trwałość
        if (std::regex_search(l, m, RE(R"(applied for\s+([0-9]+)\s+hours?\s+at\s+([0-9]+)\s*deg)", std::regex::icase)))
            dodaj(w, mamy, "Trwałość", m[1].str() + " h / " + m[2].str() + " \xC2\xB0" "C", l);
        // rezystancja izolacji / stała czasowa (kondensatory foliowe)
        if (std::regex_search(l, m, RE(R"(\b([0-9]{1,3}(?: [0-9]{3})+)\s*M\s?ohm\s+([0-9]{1,3}(?: [0-9]{3})*)\s*s\b)", std::regex::icase))) {
            auto bez = [](std::string t) { t.erase(std::remove(t.begin(), t.end(), ' '), t.end()); return t; };
            dodaj(w, mamy, "Rezystancja izolacji", bez(m[1].str()) + " M\xCE\xA9 (mniejsze pojemności) / stała czasowa " + bez(m[2].str()) + " s", l);
        }
    }

    if (poj.empty() && maxCR > 0) dodaj(w, mamy, "Pojemność", zakresTekst({minCR, maxCR}, jednostkaCR), "(tabela zamawiania)");
    if (tolerancja.empty() && !tol.empty()) {
        std::string t;
        for (double x : tol) t += (t.empty() ? "" : " / ") + std::string("\xC2\xB1") + liczbaTekst(x) + " %";
        dodaj(w, mamy, "Tolerancja pojemności", t, "(kody tolerancji w numerze zamówieniowym)");
    }
    if (!rozstaw.empty()) dodaj(w, mamy, "Rozstaw wyprowadzeń", zakresTekst(rozstaw, "mm"), "(rozstaw wyprowadzeń)");
    if (!srednica.empty()) dodaj(w, mamy, "Średnica obudowy \xCF\x86" "D", zakresTekst(srednica, "mm"), "(wymiary)");

    // --- opis: rodzaj + oznaczenie serii
    if (!rodzaj.empty()) {
        std::string seria;
        static const std::regex typ(R"(^series\s*/\s*type\s*:\s*(.+)$)", std::regex::icase);
        static const std::regex krotka(R"(^[A-Z]{2,6}[0-9]?$)");
        int n = 0;
        for (const std::string& raw : L) {
            const std::string l = zwin(raw);
            std::smatch m;
            if (std::regex_search(l, m, typ)) { seria = m[1].str(); break; }
            if (++n <= 12 && std::regex_match(l, krotka) && l != "RoHS") { seria = l; break; }
        }
        std::string r = rodzaj;
        r[0] = static_cast<char>(r[0]);
        w.opis = "Kondensator " + maleLitery(rodzaj.substr(0, 1)) + rodzaj.substr(1);
        if (!seria.empty()) w.opis += " " + seria;
    }
}

#undef RE

} // namespace

Wynik wyciagnij(const std::string& surowy) {
    Wynik w;
    std::set<std::string> mamy;
    const std::vector<std::string> L = linie(normalizuj(surowy));

    // --- opis: pierwsza linia z początku dokumentu, która wygląda jak zdanie/tytuł
    {
        static const std::regex smiec(R"((copyright|all rights reserved|www\.|http|datasheet$|rev\.? ?[0-9a-z]|page \d|\bdocument\b))", std::regex::icase);
        int sprawdzone = 0;
        for (const std::string& l : L) {
            if (l.empty()) continue;
            if (++sprawdzone > 25) break;
            if (l.size() < 15 || l.size() > 140) continue;
            if (std::regex_search(l, smiec)) continue;
            int litery = 0;
            for (unsigned char c : l) if (std::isalpha(c)) ++litery;
            if (litery < static_cast<int>(l.size()) / 2) continue;
            // dwie lub więcej liter małych/wyrazów - odrzuć same symbole
            if (std::count(l.begin(), l.end(), ' ') < 1) continue;
            w.opis = l;
            break;
        }
    }

    // --- obudowa: pierwszy dopasowany typ w tekście
    {
        static const std::regex re(R"(\b(PDIP|DIP|SOIC|SOP|SSOP|TSSOP|MSOP|SOT|TQFP|LQFP|QFP|QFN|DFN|BGA|LGA|TO)[- ]?([0-9]{1,3}[A-Z]?(?:-[0-9]+)?)\b)");
        for (const std::string& l : L) {
            std::smatch m;
            if (std::regex_search(l, m, re)) {
                w.obudowa = m[1].str() + "-" + m[2].str();
                break;
            }
        }
    }

    static const std::regex liczba(R"(([-+]?[0-9]+(?:[.,][0-9]+)?))");
    // --- napięcie zasilania: "X V to Y V" w linii o zasilaniu
    {
        static const std::regex zakres(R"(([0-9]+(?:[.,][0-9]+)?)\s*V?\s*(?:to|-|~|\.\.\.)\s*([0-9]+(?:[.,][0-9]+)?)\s*V\b)", std::regex::icase);
        for (const std::string& l : L) {
            if (!zawiera(l, {"supply voltage", "operating voltage", "vcc", "vdd", "power supply", "input voltage"})) continue;
            std::smatch m;
            if (std::regex_search(l, m, zakres)) {
                dodaj(w, mamy, "Napięcie zasilania", m[1].str() + " V ... " + m[2].str() + " V", l);
                break;
            }
        }
    }
    // --- temperatura pracy
    {
        static const std::regex zakres(R"(([-+]?[0-9]{1,3})\s*(?:deg\s*C|C)?\s*(?:to|-|\.\.\.|~)\s*\+?([0-9]{1,3})\s*(?:deg\s*)?C\b)", std::regex::icase);
        for (const std::string& l : L) {
            if (!zawiera(l, {"temperature", "temp.", "operating"})) continue;
            std::smatch m;
            if (std::regex_search(l, m, zakres)) {
                int a = std::stoi(m[1].str()), b = std::stoi(m[2].str());
                if (a >= b || a < -100 || b > 300) continue;
                dodaj(w, mamy, "Temperatura pracy", std::to_string(a) + " ... +" + std::to_string(b) + " °C", l);
                break;
            }
        }
    }
    // --- liczba wyprowadzeń
    {
        static const std::regex re(R"(\b([0-9]{1,3})[- ](?:pin|lead|pins|leads)\b)", std::regex::icase);
        for (const std::string& l : L) {
            std::smatch m;
            if (std::regex_search(l, m, re)) {
                const int n = std::stoi(m[1].str());
                if (n >= 2 && n <= 400) { dodaj(w, mamy, "Liczba wyprowadzeń", std::to_string(n), l); break; }
            }
        }
    }
    // --- prąd (wyjściowy / maksymalny)
    {
        static const std::regex re(R"(([0-9]+(?:[.,][0-9]+)?)\s*(mA|A|uA|µA)\b)");
        for (const std::string& l : L) {
            if (!zawiera(l, {"output current", "continuous current", "collector current", "forward current", "max current", "drain current"})) continue;
            std::smatch m;
            if (std::regex_search(l, m, re)) {
                dodaj(w, mamy, "Prąd maksymalny", m[1].str() + " " + m[2].str(), l);
                break;
            }
        }
    }
    // --- napięcie maksymalne
    {
        static const std::regex re(R"(([0-9]+(?:[.,][0-9]+)?)\s*V\b)");
        for (const std::string& l : L) {
            if (!zawiera(l, {"collector-emitter", "drain-source", "breakdown voltage", "reverse voltage", "maximum voltage", "max. voltage"})) continue;
            std::smatch m;
            if (std::regex_search(l, m, re)) {
                dodaj(w, mamy, "Napięcie maksymalne", m[1].str() + " V", l);
                break;
            }
        }
    }
    // --- częstotliwość
    {
        static const std::regex re(R"(([0-9]+(?:[.,][0-9]+)?)\s*(kHz|MHz|GHz)\b)", std::regex::icase);
        for (const std::string& l : L) {
            if (!zawiera(l, {"frequency", "clock", "oscillator", "bandwidth", "speed"})) continue;
            std::smatch m;
            if (std::regex_search(l, m, re)) {
                dodaj(w, mamy, "Częstotliwość", m[1].str() + " " + m[2].str(), l);
                break;
            }
        }
    }
    // --- moc
    {
        static const std::regex re(R"(([0-9]+(?:[.,][0-9]+)?)\s*(mW|W)\b)");
        for (const std::string& l : L) {
            if (!zawiera(l, {"power dissipation", "total power", "power rating"})) continue;
            std::smatch m;
            if (std::regex_search(l, m, re)) {
                dodaj(w, mamy, "Moc maksymalna", m[1].str() + " " + m[2].str(), l);
                break;
            }
        }
    }
    (void)liczba;
    odczytajTabele(L, w, mamy);
    odczytajKondensator(L, w, mamy);
    odczytajUwagi(L, w);
    return w;
}

} // namespace OdczytDatasheetu
