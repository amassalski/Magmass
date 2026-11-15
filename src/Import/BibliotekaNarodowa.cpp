#include "BibliotekaNarodowa.h"

#include "../Wyszukiwanie/MiniJSON.h"

#include <cctype>

namespace BibliotekaNarodowa {

namespace {

std::string zakodujURL(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string w;
    for (unsigned char c : s) {
        // tylko ASCII: isalnum() zależy od lokalizacji i w polskiej uznaje bajty >= 128 za litery
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') w += static_cast<char>(c);
        else { w += '%'; w += hex[c >> 4]; w += hex[c & 15]; }
    }
    return w;
}

std::string przytnij(std::string s) {
    auto bialy = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && bialy(static_cast<unsigned char>(s.back()))) s.pop_back();
    size_t p = 0;
    while (p < s.size() && bialy(static_cast<unsigned char>(s[p]))) ++p;
    return s.substr(p);
}

/// Usuwa interpunkcję rekordów MARC na końcu pola ("Tytuł /", "Wydawnictwo,", "2019.")
std::string bezInterpunkcjiMARC(std::string s) {
    s = przytnij(s);
    while (!s.empty() && (s.back() == '/' || s.back() == ':' || s.back() == ',' || s.back() == ';' || s.back() == '=' || s.back() == '.' ||
                          std::isspace(static_cast<unsigned char>(s.back())))) {
        const char c = s.back();
        // kropka po skrócie lub inicjale zostaje, jeśli przed nią jest pojedyncza litera ("J.")
        if (c == '.' && s.size() >= 2 && std::isalpha(static_cast<unsigned char>(s[s.size() - 2])) &&
            (s.size() == 2 || s[s.size() - 3] == ' ' || s[s.size() - 3] == '.')) break;
        s.pop_back();
    }
    return przytnij(s);
}

std::string tekstPola(const JsonWartosc& obiekt, const char* nazwa) {
    const JsonWartosc* p = obiekt.pole(nazwa);
    return p ? przytnij(p->jakoTekst()) : std::string();
}

/// Wartość podpola z pola MARC (obiekt z "subfields": [{"a": "..."}, ...]); @p kod pojedyncza litera
std::string podpole(const JsonWartosc& pole, char kod) {
    const JsonWartosc* pod = pole.pole("subfields");
    if (!pod || !pod->czyTablica()) return {};
    const std::string nazwa(1, kod);
    for (const JsonWartosc& e : pod->elementy)
        if (const JsonWartosc* v = e.pole(nazwa)) return przytnij(v->jakoTekst());
    return {};
}

/// "Miszczak, Stanisław (1923-2007) Wydawnictwa..." -> "Miszczak, Stanisław" (tylko gdy brak rekordu MARC)
std::string autorZPolaProstego(const std::string& t) {
    const size_t p = t.find(" (");
    if (p != std::string::npos && p + 2 < t.size() && std::isdigit(static_cast<unsigned char>(t[p + 2]))) return bezInterpunkcjiMARC(t.substr(0, p));
    return bezInterpunkcjiMARC(t);
}

/// "WKŁ WKŁ," (powtórzona nazwa) -> "WKŁ"
std::string wydawcaZPolaProstego(const std::string& t0) {
    const std::string t = bezInterpunkcjiMARC(t0);
    if (t.size() >= 3 && t.size() % 2 == 1) {
        const size_t pol = t.size() / 2;
        if (t[pol] == ' ' && t.compare(0, pol, t, pol + 1, pol) == 0) return t.substr(0, pol);
    }
    return t;
}

void zMARC(const JsonWartosc& marc, MetadaneKsiazek::Dane& d) {
    const JsonWartosc* pola = marc.pole("fields");
    if (!pola || !pola->czyTablica()) return;
    for (const JsonWartosc& wpis : pola->elementy) {
        if (!wpis.czyObiekt()) continue;
        for (const auto& kv : wpis.pola) {
            const std::string& num = kv.first;
            const JsonWartosc& w = kv.second;
            if (!w.czyObiekt()) continue;
            if (num == "020" && d.isbn.empty()) d.isbn = czystyISBN(podpole(w, 'a'));
            else if (num == "100" && d.autor.empty()) d.autor = bezInterpunkcjiMARC(podpole(w, 'a'));
            else if (num == "245" && d.tytul.empty()) {
                d.tytul = bezInterpunkcjiMARC(podpole(w, 'a'));
                const std::string b = bezInterpunkcjiMARC(podpole(w, 'b'));
                if (!b.empty()) d.tytul += ": " + b;
            } else if ((num == "260" || num == "264") && d.wydawca.empty()) {
                d.wydawca = bezInterpunkcjiMARC(podpole(w, 'b'));
                if (d.rok == 0) d.rok = rokZTekstu(podpole(w, 'c'));
            } else if (num == "250" && d.wydanie.empty()) {
                d.wydanie = bezInterpunkcjiMARC(podpole(w, 'a'));
            } else if (num == "041" && d.jezyk.empty()) {
                const std::string kod = podpole(w, 'a');
                if (!kod.empty()) d.jezyk = MetadaneKsiazek::nazwaJezyka(kod);
            }
        }
    }
}

}  // namespace

std::string czystyISBN(const std::string& tekst) {
    std::string c;
    for (unsigned char ch : tekst) {
        if (std::isdigit(ch)) c += static_cast<char>(ch);
        else if ((ch == 'X' || ch == 'x') && c.size() == 9) c += 'X';
        else if (ch == ' ' || ch == '-') continue;
        else if (c.size() >= 10) break;   // dopisek po numerze, np. "(oprawa twarda)"
        else if (std::isalpha(ch) || ch == '(') break;
    }
    return (c.size() == 10 || c.size() == 13) ? c : std::string();
}

std::string adresZapytania(const std::string& isbn, const std::string& tytul, const std::string& autor, int limit) {
    std::string url = "https://data.bn.org.pl/api/networks/bibs.json?limit=" + std::to_string(limit);
    const std::string i = czystyISBN(isbn);
    if (!i.empty()) return url + "&isbnIssn=" + i;
    if (!przytnij(tytul).empty()) url += "&title=" + zakodujURL(przytnij(tytul));
    if (!przytnij(autor).empty()) url += "&author=" + zakodujURL(przytnij(autor));
    return url;
}

bool zOdpowiedzi(const std::string& json, std::vector<MetadaneKsiazek::Dane>& wynik, std::string& blad) {
    wynik.clear();
    JsonWartosc k;
    if (!MiniJSON::parsuj(json, k, blad)) return false;
    const JsonWartosc* bibs = k.czyTablica() ? &k : k.pole("bibs");
    if (!bibs || !bibs->czyTablica()) { blad = "brak pola bibs w odpowiedzi"; return false; }
    for (const JsonWartosc& b : bibs->elementy) {
        if (!b.czyObiekt()) continue;
        MetadaneKsiazek::Dane d;
        // rekord MARC jest źródłem głównym: pola proste BN bywają sklejone (autor + daty + wydawca, wydawca podwojony)
        if (const JsonWartosc* marc = b.pole("marc")) zMARC(*marc, d);
        if (d.tytul.empty()) d.tytul = bezInterpunkcjiMARC(tekstPola(b, "title"));
        if (d.autor.empty()) d.autor = autorZPolaProstego(tekstPola(b, "author"));
        if (d.wydawca.empty()) d.wydawca = wydawcaZPolaProstego(tekstPola(b, "publisher"));
        if (d.rok == 0) d.rok = rokZTekstu(tekstPola(b, "publicationYear"));
        if (d.isbn.empty()) d.isbn = czystyISBN(tekstPola(b, "isbnIssn"));
        if (d.jezyk.empty()) {
            const std::string jez = tekstPola(b, "language");
            if (!jez.empty()) d.jezyk = MetadaneKsiazek::nazwaJezyka(jez);
        }
        // to samo wydanie opisane w kilku rekordach pokazujemy raz
        bool powtorka = false;
        for (const auto& x : wynik)
            if (x.tytul == d.tytul && x.autor == d.autor && x.wydawca == d.wydawca && x.rok == d.rok) powtorka = true;
        if (powtorka) continue;
        if (!d.tytul.empty()) wynik.push_back(d);
    }
    return true;
}

int rokZTekstu(const std::string& tekst) {
    for (size_t i = 0; i + 4 <= tekst.size(); ++i) {
        bool cyfry = true;
        for (size_t j = 0; j < 4; ++j) if (!std::isdigit(static_cast<unsigned char>(tekst[i + j]))) { cyfry = false; break; }
        if (!cyfry) continue;
        if (i > 0 && std::isdigit(static_cast<unsigned char>(tekst[i - 1]))) continue;
        if (i + 4 < tekst.size() && std::isdigit(static_cast<unsigned char>(tekst[i + 4]))) continue;
        const int r = std::stoi(tekst.substr(i, 4));
        if (r >= 1000 && r <= 2999) return r;
    }
    return 0;
}

}  // namespace BibliotekaNarodowa
