#include "MetadaneKsiazek.h"
#include "MiniXML.h"

#include <algorithm>
#include <cctype>

namespace MetadaneKsiazek {
namespace {

std::string przytnij(const std::string& s) {
    const char* sp = " \t\r\n";
    const size_t a = s.find_first_not_of(sp);
    if (a == std::string::npos) return std::string();
    return s.substr(a, s.find_last_not_of(sp) - a + 1);
}
std::string malymi(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
std::string zwinBiale(const std::string& s) {
    std::string w;
    bool spacja = false;
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { spacja = true; continue; }
        if (spacja && !w.empty()) w += ' ';
        spacja = false;
        w += c;
    }
    return w;
}
/// Nazwa elementu bez prefiksu przestrzeni nazw ("dc:title" -> "title")
std::string lokalna(const std::string& n) {
    const size_t k = n.find(':');
    return k == std::string::npos ? n : n.substr(k + 1);
}

void zbierz(const MiniXML::Wezel* w, const std::string& nazwa, std::vector<const MiniXML::Wezel*>& wynik) {
    for (const auto& d : w->dzieci) {
        if (lokalna(d->nazwa) == nazwa) wynik.push_back(d.get());
        zbierz(d.get(), nazwa, wynik);
    }
}

/// Teksty rdf:li pod elementem (Alt/Seq/Bag); gdy ich nie ma, tekst samego elementu
std::vector<std::string> teksty(const MiniXML::Wezel* e) {
    std::vector<std::string> wynik;
    std::vector<const MiniXML::Wezel*> li;
    zbierz(e, "li", li);
    // rdf:Alt: wersja x-default ma pierwszeństwo przed innymi językami
    for (const auto* l : li) {
        const std::string t = zwinBiale(l->tekst);
        if (!t.empty() && l->atrybut("xml:lang") == "x-default") return {t};
    }
    for (const auto* l : li) {
        const std::string t = zwinBiale(l->tekst);
        if (!t.empty()) wynik.push_back(t);
    }
    if (wynik.empty()) {
        const std::string t = zwinBiale(e->tekst);
        if (!t.empty()) wynik.push_back(t);
    }
    return wynik;
}

/// Wszystkie wartości właściwości: elementy o tej nazwie lokalnej oraz atrybuty (rdf:Description xmp:CreateDate="...")
std::vector<std::string> wartosci(const MiniXML::Wezel* korzen, const std::string& nazwa) {
    std::vector<std::string> wynik;
    std::vector<const MiniXML::Wezel*> e;
    zbierz(korzen, nazwa, e);
    for (const auto* w : e) for (const std::string& t : teksty(w)) wynik.push_back(t);
    if (wynik.empty()) {
        std::vector<const MiniXML::Wezel*> wszystkie;
        wszystkie.push_back(korzen);
        zbierz(korzen, "Description", wszystkie);
        for (const auto* w : wszystkie)
            for (const auto& [k, v] : w->atrybuty)
                if (lokalna(k) == nazwa && !przytnij(v).empty()) wynik.push_back(zwinBiale(v));
    }
    return wynik;
}

std::string polacz(const std::vector<std::string>& v, const char* sep = ", ") {
    std::string w;
    for (const std::string& s : v) {
        if (s.empty()) continue;
        if (!w.empty()) w += sep;
        w += s;
    }
    return w;
}

/// Rok z daty ("2015-03-02T10:00:00Z", "D:20150302...", "Tue Mar 3 2015")
int rokZTekstu(const std::string& t) {
    for (size_t i = 0; i + 4 <= t.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(t[i])) || !std::isdigit(static_cast<unsigned char>(t[i + 1])) ||
            !std::isdigit(static_cast<unsigned char>(t[i + 2])) || !std::isdigit(static_cast<unsigned char>(t[i + 3]))) continue;
        if (i > 0 && std::isdigit(static_cast<unsigned char>(t[i - 1]))) continue;
        const int r = std::stoi(t.substr(i, 4));
        if (r >= 1800 && r <= 2200) return r;   // 1601 itp. to puste daty systemowe
    }
    return 0;
}

/// Wyciąga ISBN-10/13 z tekstu ("urn:isbn:978-83-206-1777-1", "ISBN 83-01-12345-6"); pusty, gdy to nie ISBN.
/// Bez słowa "isbn" cały tekst musi być numerem (identyfikatory typu UUID są odrzucane).
std::string isbnZTekstu(const std::string& t, bool wymagajSlowaIsbn) {
    std::string s = przytnij(t);
    const size_t poz = malymi(s).find("isbn");
    if (poz != std::string::npos) s = s.substr(poz + 4);
    else if (wymagajSlowaIsbn) return {};
    size_t i = 0;
    while (i < s.size() && !std::isdigit(static_cast<unsigned char>(s[i]))) {
        if (poz == std::string::npos) return {};   // bez słowa "isbn" tekst ma zaczynać się cyfrą
        ++i;
    }
    std::string cyfry, odczyt;
    for (; i < s.size(); ++i) {
        const char c = s[i];
        if (std::isdigit(static_cast<unsigned char>(c))) { cyfry += c; odczyt += c; }
        else if ((c == 'X' || c == 'x') && cyfry.size() == 9) { cyfry += 'X'; odczyt += 'X'; }
        else if ((c == '-' || c == ' ') && !odczyt.empty() && odczyt.back() != '-') odczyt += '-';
        else break;
    }
    while (!odczyt.empty() && odczyt.back() == '-') odczyt.pop_back();
    if (poz == std::string::npos && przytnij(s.substr(i)).size() > 0) return {};   // coś za numerem: to nie sam ISBN
    if (cyfry.size() != 10 && cyfry.size() != 13) return {};
    return odczyt;
}

}  // namespace

std::string nazwaJezyka(const std::string& kod) {
    std::string k = malymi(przytnij(kod));
    const size_t kr = k.find_first_of("-_");
    if (kr != std::string::npos) k.erase(kr);
    static const struct { const char* kod; const char* nazwa; } mapa[] = {
        {"pl", "polski"}, {"pol", "polski"}, {"en", "angielski"}, {"eng", "angielski"}, {"de", "niemiecki"}, {"ger", "niemiecki"}, {"deu", "niemiecki"},
        {"fr", "francuski"}, {"fre", "francuski"}, {"fra", "francuski"}, {"ru", "rosyjski"}, {"rus", "rosyjski"}, {"cs", "czeski"}, {"cze", "czeski"},
        {"es", "hiszpański"}, {"spa", "hiszpański"}, {"it", "włoski"}, {"ita", "włoski"}, {"sk", "słowacki"}, {"uk", "ukraiński"}};
    for (const auto& m : mapa) if (k == m.kod) return m.nazwa;
    return przytnij(kod);
}

bool czyAutorSmieciowy(const std::string& autor) {
    const std::string t = malymi(przytnij(autor));
    if (t.empty()) return false;
    for (const char* s : {"test", "admin", "administrator", "user", "owner", "unknown", "nieznany", "pc", "windows user", "scanner", "skaner", "author", "autor", "anonymous"})
        if (t == s) return true;
    for (char c : t) if (std::isdigit(static_cast<unsigned char>(c))) return true;   // nicki typu plus5volt
    if (t.find("microsoft") != std::string::npos || t.find("adobe") != std::string::npos) return true;
    return false;
}

bool czyOpisSmieciowy(const std::string& opis) {
    const std::string t = malymi(przytnij(opis));
    if (t.empty()) return false;
    for (const char* p : {"created", "generated", "scanned", "converted", "produced", "microsoft", "adobe", "untitled"})
        if (t.rfind(p, 0) == 0) return true;
    return t == "pdf" || t == "scan" || t == "skan" || t == "document";
}

bool czyTytulSmieciowy(const std::string& tytul) {
    const std::string t = malymi(przytnij(tytul));
    if (t.size() < 2) return true;
    {   // sam znacznik czasu, np. "2015-09-18 15;30;23"
        bool tylkoDatowe = t.size() >= 6 && std::isdigit(static_cast<unsigned char>(t[0]));
        for (char c : t) if (!std::isdigit(static_cast<unsigned char>(c)) && std::string(" -:;./_t").find(c) == std::string::npos) tylkoDatowe = false;
        if (tylkoDatowe) return true;
    }
    if (t.rfind("created ", 0) == 0 || t.rfind("scan", 0) == 0 && t.size() < 12) return true;
    if (t.find("microsoft word") != std::string::npos || t.find("microsoft powerpoint") != std::string::npos) return true;
    if (t == "untitled" || t == "bez tytułu" || t == "bez tytulu" || t == "document" || t == "dokument" || t == "print" || t.rfind("untitled", 0) == 0) return true;
    for (const char* rozsz : {".pdf", ".doc", ".docx", ".indd", ".tex", ".dvi", ".ps", ".qxd", ".odt", ".rtf", ".txt"})
        if (t.size() > std::string(rozsz).size() && t.compare(t.size() - std::string(rozsz).size(), std::string::npos, rozsz) == 0) return true;
    return false;
}

std::string wytnijXMP(const std::string& bajty) {
    size_t a = bajty.find("<x:xmpmeta");
    if (a == std::string::npos) a = bajty.find("<xmpmeta");
    if (a == std::string::npos) return {};
    size_t b = bajty.find("</x:xmpmeta>", a);
    size_t dl = 12;
    if (b == std::string::npos) { b = bajty.find("</xmpmeta>", a); dl = 10; }
    if (b == std::string::npos) return {};
    return bajty.substr(a, b + dl - a);
}

Dane zXMP(const std::string& xml) {
    Dane d;
    std::string blad;
    auto korzen = MiniXML::parsuj(xml, blad);
    if (!korzen) return d;
    const MiniXML::Wezel* k = korzen.get();
    auto pierwszy = [&](const char* n) { const auto v = wartosci(k, n); return v.empty() ? std::string() : v.front(); };
    d.tytul = pierwszy("title");
    d.autor = polacz(wartosci(k, "creator"));
    d.wydawca = polacz(wartosci(k, "publisher"));
    for (const char* n : {"publicationDate", "date"}) {   // CreateDate to data utworzenia pliku, nie wydania
        if (d.rok) break;
        for (const std::string& t : wartosci(k, n)) if ((d.rok = rokZTekstu(t)) != 0) break;
    }
    for (const char* n : {"isbn", "identifier"}) {
        if (!d.isbn.empty()) break;
        for (const std::string& t : wartosci(k, n)) {
            d.isbn = isbnZTekstu(t, std::string(n) == "identifier");
            if (!d.isbn.empty()) break;
        }
    }
    d.wydanie = pierwszy("edition");
    const auto jezyki = wartosci(k, "language");
    if (!jezyki.empty()) d.jezyk = nazwaJezyka(jezyki.front());
    d.tagi = polacz(wartosci(k, "subject"));
    if (d.tagi.empty()) {
        // pdf:Keywords: jeden tekst rozdzielony przecinkami lub średnikami
        std::string s = pierwszy("Keywords");
        for (char& c : s) if (c == ';') c = ',';
        std::string w;
        size_t start = 0;
        while (start <= s.size()) {
            size_t e = s.find(',', start);
            if (e == std::string::npos) e = s.size();
            const std::string czesc = przytnij(s.substr(start, e - start));
            if (!czesc.empty()) w += (w.empty() ? "" : ", ") + czesc;
            start = e + 1;
        }
        d.tagi = w;
    }
    d.opis = pierwszy("description");
    if (czyTytulSmieciowy(d.tytul)) d.tytul.clear();
    if (czyAutorSmieciowy(d.autor)) d.autor.clear();
    if (czyOpisSmieciowy(d.opis)) d.opis.clear();
    return d;
}

Dane zOPF(const std::string& xml) {
    Dane d;
    std::string blad;
    auto korzen = MiniXML::parsuj(xml, blad);
    if (!korzen) return d;
    const MiniXML::Wezel* k = korzen.get();
    auto pierwszy = [&](const char* n) { const auto v = wartosci(k, n); return v.empty() ? std::string() : v.front(); };
    d.tytul = pierwszy("title");
    d.autor = polacz(wartosci(k, "creator"));
    d.wydawca = pierwszy("publisher");
    for (const std::string& t : wartosci(k, "date")) if ((d.rok = rokZTekstu(t)) != 0) break;
    for (const std::string& t : wartosci(k, "identifier")) {
        d.isbn = isbnZTekstu(t, false);
        if (!d.isbn.empty()) break;
    }
    const auto jezyki = wartosci(k, "language");
    if (!jezyki.empty()) d.jezyk = nazwaJezyka(jezyki.front());
    d.tagi = polacz(wartosci(k, "subject"));
    d.opis = pierwszy("description");
    if (czyTytulSmieciowy(d.tytul)) d.tytul.clear();
    return d;
}

Dane zInfoPDF(const std::string& wyjscie) {
    Dane d;
    size_t start = 0;
    while (start < wyjscie.size()) {
        size_t e = wyjscie.find('\n', start);
        if (e == std::string::npos) e = wyjscie.size();
        const std::string linia = wyjscie.substr(start, e - start);
        start = e + 1;
        const size_t k = linia.find(':');
        if (k == std::string::npos) continue;
        const std::string klucz = linia.substr(0, k), wartosc = przytnij(linia.substr(k + 1));
        if (wartosc.empty()) continue;
        if (klucz == "Title") d.tytul = zwinBiale(wartosc);
        else if (klucz == "Author") d.autor = zwinBiale(wartosc);
        else if (klucz == "Subject") d.opis = zwinBiale(wartosc);
        else if (klucz == "Keywords") {
            std::string s = wartosc;
            for (char& c : s) if (c == ';') c = ',';
            d.tagi = zwinBiale(s);
        }   // CreationDate pomijamy: to data utworzenia pliku PDF, nie rok wydania książki
    }
    if (czyTytulSmieciowy(d.tytul)) d.tytul.clear();
    if (czyAutorSmieciowy(d.autor)) d.autor.clear();
    if (czyOpisSmieciowy(d.opis)) d.opis.clear();
    return d;
}

Dane polacz(const Dane& a, const Dane& b) {
    Dane w = a;
    if (w.tytul.empty()) w.tytul = b.tytul;
    if (w.autor.empty()) w.autor = b.autor;
    if (w.wydawca.empty()) w.wydawca = b.wydawca;
    if (w.rok == 0) w.rok = b.rok;
    if (w.isbn.empty()) w.isbn = b.isbn;
    if (w.jezyk.empty()) w.jezyk = b.jezyk;
    if (w.tagi.empty()) w.tagi = b.tagi;
    if (w.opis.empty()) w.opis = b.opis;
    return w;
}

} // namespace MetadaneKsiazek
