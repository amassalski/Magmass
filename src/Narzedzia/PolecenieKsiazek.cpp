#include "PolecenieKsiazek.h"

#include "UwagiHTML.h"

#include <cctype>

namespace PolecenieKsiazek {

namespace {

std::string przytnij(const std::string& s) {
    const char* sp = " \t\r\n";
    const size_t a = s.find_first_not_of(sp);
    if (a == std::string::npos) return {};
    return s.substr(a, s.find_last_not_of(sp) - a + 1);
}

/// Wartość w jednej linii pliku argumentów (nowe linie i tabulatory -> spacje)
std::string wJednejLinii(const std::string& s) {
    std::string w;
    bool spacja = false;
    for (char c : s) {
        if (c == '\n' || c == '\r' || c == '\t' || c == ' ') { spacja = true; continue; }
        if (spacja && !w.empty()) w += ' ';
        spacja = false;
        w += c;
    }
    return w;
}

std::vector<std::string> rozdziel(const std::string& s) {
    std::vector<std::string> w;
    size_t start = 0;
    while (start <= s.size()) {
        size_t e = s.find_first_of(",;", start);
        if (e == std::string::npos) e = s.size();
        const std::string c = przytnij(s.substr(start, e - start));
        if (!c.empty()) w.push_back(c);
        start = e + 1;
    }
    return w;
}

} // namespace

std::string kodJezyka(const std::string& nazwa) {
    std::string k = przytnij(nazwa);
    for (char& c : k) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (k.size() == 2 || k.size() == 3) {
        bool litery = true;
        for (char c : k) if (!std::isalpha(static_cast<unsigned char>(c))) litery = false;
        if (litery && k.size() == 2) return k;
    }
    static const struct { const char* nazwa; const char* kod; } mapa[] = {
        {"polski", "pl"}, {"angielski", "en"}, {"niemiecki", "de"}, {"francuski", "fr"}, {"rosyjski", "ru"}, {"czeski", "cs"},
        {"słowacki", "sk"}, {"ukraiński", "uk"}, {"hiszpański", "es"}, {"włoski", "it"}, {"łaciński", "la"},
        {"pol", "pl"}, {"eng", "en"}, {"ger", "de"}, {"deu", "de"}, {"fre", "fr"}, {"fra", "fr"}, {"rus", "ru"}, {"cze", "cs"}, {"spa", "es"}, {"ita", "it"}};
    for (const auto& m : mapa) if (k == m.nazwa) return m.kod;
    return {};
}

std::vector<std::string> liniePlikuExifTool(const Ksiazka& k, const std::string& sciezkaPliku) {
    std::vector<std::string> l;
    auto dodaj = [&](const std::string& tag, const std::string& wartosc) {
        const std::string w = wJednejLinii(wartosc);
        if (!w.empty()) l.push_back("-" + tag + "=" + w);
    };
    dodaj("XMP-dc:Title", k.tytul);
    dodaj("PDF:Title", k.tytul);
    dodaj("XMP-dc:Creator", k.autor);
    dodaj("PDF:Author", k.autor);
    dodaj("XMP-dc:Publisher", k.wydawca);
    if (k.rok > 0) dodaj("XMP-dc:Date", std::to_string(k.rok));
    const std::string jez = kodJezyka(k.jezyk);
    dodaj("XMP-dc:Language", jez);
    if (!przytnij(k.isbn).empty()) dodaj("XMP-dc:Identifier", "urn:isbn:" + przytnij(k.isbn));
    dodaj("XMP-prism:Edition", k.wydanie);
    const std::vector<std::string> tagi = rozdziel(k.tagi);
    if (!tagi.empty()) {
        l.push_back("-XMP-dc:Subject=");
        std::string wszystkie;
        for (const std::string& t : tagi) {
            l.push_back("-XMP-dc:Subject+=" + wJednejLinii(t));
            wszystkie += (wszystkie.empty() ? "" : ", ") + wJednejLinii(t);
        }
        dodaj("PDF:Keywords", wszystkie);
    }
    const std::string uwagi = UwagiHTML::naTekst(k.uwagi);
    dodaj("XMP-dc:Description", uwagi);
    dodaj("PDF:Subject", uwagi);
    l.push_back(sciezkaPliku);
    return l;
}

std::vector<std::string> argumentyExifTool(const std::string& plikArgumentow, bool zachowajKopie) {
    std::vector<std::string> a = {"-charset", "utf8", "-charset", "filename=utf8"};
    if (!zachowajKopie) a.push_back("-overwrite_original");
    a.push_back("-@");
    a.push_back(plikArgumentow);
    return a;
}

std::string sciezkaWynikuOcr(const std::string& plik) {
    const size_t kropka = plik.find_last_of('.');
    const size_t separator = plik.find_last_of("/\\");
    if (kropka == std::string::npos || (separator != std::string::npos && kropka < separator))
        return plik + "_ocr.pdf";
    return plik.substr(0, kropka) + "_ocr" + plik.substr(kropka);
}

bool poprawneJezykiOcr(const std::string& jezyki) {
    if (jezyki.empty()) return false;
    for (char c : jezyki)
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '+')) return false;
    return jezyki.front() != '+' && jezyki.back() != '+';
}

std::vector<std::string> argumentyOcr(const std::string& wejscie, const std::string& wyjscie, const std::string& jezyki, bool pominTekst) {
    std::vector<std::string> a = {"-l", jezyki, "--output-type", "pdf"};
    if (pominTekst) a.push_back("--skip-text");
    a.push_back("--");   // dalej tylko ścieżki, nawet gdyby zaczynały się od "-"
    a.push_back(wejscie);
    a.push_back(wyjscie);
    return a;
}

std::string kodujSciezkeURL(const std::string& sciezka) {
    std::string s = sciezka;
    for (char& c : s) if (c == '\\') c = '/';
    if (s.size() > 1 && s[1] == ':') s = "/" + s;
    static const char* HEX = "0123456789ABCDEF";
    std::string w;
    for (unsigned char c : s) {
        const bool wolny = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                           c == '-' || c == '.' || c == '_' || c == '~' || c == '/' || c == ':';
        if (wolny) w += static_cast<char>(c);
        else { w += '%'; w += HEX[c >> 4]; w += HEX[c & 15]; }
    }
    return w;
}

std::string wypelnijPolecenieCzytnika(const std::string& szablon, const std::string& sciezka, int strona) {
    const std::string url = kodujSciezkeURL(sciezka), nr = std::to_string(strona);
    std::string w;
    for (size_t i = 0; i < szablon.size(); ++i) {
        if (szablon[i] == '%' && i + 1 < szablon.size()) {
            const char z = szablon[i + 1];
            if (z == 'f') { w += sciezka; ++i; continue; }
            if (z == 'u') { w += url; ++i; continue; }
            if (z == 'p') { w += nr; ++i; continue; }
        }
        w += szablon[i];
    }
    return w;
}

std::string rozszerzPATH(const std::string& path, const std::vector<std::string>& dodatkowe, char separator) {
    std::string w = path;
    auto jest = [&](const std::string& d) {
        size_t p = 0;
        while (p <= w.size()) {
            size_t k = w.find(separator, p);
            if (k == std::string::npos) k = w.size();
            if (w.compare(p, k - p, d) == 0 && k - p == d.size()) return true;
            p = k + 1;
        }
        return false;
    };
    for (const std::string& d : dodatkowe) {
        if (d.empty() || jest(d)) continue;
        if (!w.empty()) w += separator;
        w += d;
    }
    return w;
}

} // namespace PolecenieKsiazek
