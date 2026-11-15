#include "Mouser.h"

#include "MiniJSON.h"

namespace Mouser {

namespace {

std::string przytnij(std::string s) {
    auto bialy = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && bialy(s.back())) s.pop_back();
    size_t p = 0;
    while (p < s.size() && bialy(s[p])) ++p;
    return s.substr(p);
}

std::string maleASCII(std::string s) {
    for (char& c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');   // bez tolower(): zależy od lokalizacji
    return s;
}

std::string zakodujURL(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string w;
    for (unsigned char c : s) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') w += static_cast<char>(c);
        else { w += '%'; w += hex[c >> 4]; w += hex[c & 15]; }
    }
    return w;
}

std::string uciekniJSON(const std::string& s) {
    static const char* hex = "0123456789abcdef";
    std::string w;
    for (unsigned char c : s) {
        switch (c) {
            case '"': w += "\\\""; break;
            case '\\': w += "\\\\"; break;
            case '\n': w += "\\n"; break;
            case '\r': w += "\\r"; break;
            case '\t': w += "\\t"; break;
            default:
                if (c < 0x20) { w += "\\u00"; w += hex[c >> 4]; w += hex[c & 15]; }
                else w += static_cast<char>(c);
        }
    }
    return w;
}

std::string tekstPola(const JsonWartosc& o, const char* nazwa) {
    const JsonWartosc* p = o.pole(nazwa);
    return p ? przytnij(p->jakoTekst()) : std::string();
}

} // namespace

std::string Czesc::obudowa() const {
    for (const auto& a : atrybuty) {
        const std::string n = maleASCII(a.first);
        if (a.second.empty()) continue;
        const bool opakowanie = n.find("packaging") != std::string::npos ||
                                (n.find("opakowanie") != std::string::npos && n.find("obudowa") == std::string::npos);
        if (opakowanie) continue;
        if (n.find("package / case") != std::string::npos || n.find("package/case") != std::string::npos || n.find("obudowa") != std::string::npos ||
            n == "case" || n == "package" || n.find("case code") != std::string::npos)
            return a.second;
    }
    return {};
}

std::string adresZapytania(const std::string& klucz) {
    return "https://api.mouser.com/api/v1/search/keyword?apiKey=" + zakodujURL(przytnij(klucz));
}

std::string cialoZapytania(const std::string& fraza, int limit) {
    if (limit < 1) limit = 1;
    if (limit > 50) limit = 50;
    std::string f = przytnij(fraza);
    if (f.size() > 120) f.resize(120);
    return "{\"SearchByKeywordRequest\":{\"keyword\":\"" + uciekniJSON(f) + "\",\"records\":" + std::to_string(limit) +
           ",\"startingRecord\":0,\"searchOptions\":\"None\",\"searchWithYourSignUpLanguage\":\"false\"}}";
}

std::string poprawAdres(std::string adres) {
    adres = przytnij(adres);
    if (adres.empty()) return {};
    if (adres.compare(0, 2, "//") == 0) adres = "https:" + adres;
    const std::string m = maleASCII(adres.substr(0, 8));
    if (m.compare(0, 7, "http://") != 0 && m.compare(0, 8, "https://") != 0) return {};
    std::string w;
    for (char c : adres) { if (c == ' ') w += "%20"; else w += c; }
    return w;
}

bool czyKluczPoprawny(const std::string& klucz) {
    const std::string k = przytnij(klucz);
    if (k.size() != 36) return false;
    for (size_t i = 0; i < k.size(); ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) { if (k[i] != '-') return false; continue; }
        const char c = k[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
    }
    return true;
}

bool zOdpowiedzi(const std::string& json, std::vector<Czesc>& czesci, std::string& blad) {
    czesci.clear();
    blad.clear();
    JsonWartosc korzen;
    std::string b;
    if (!MiniJSON::parsuj(json, korzen, b) || !korzen.czyObiekt()) {
        blad = "Nieprawidłowa odpowiedź Mousera (to nie jest JSON).";
        return false;
    }
    if (const JsonWartosc* bl = korzen.pole("Errors"); bl && bl->czyTablica() && !bl->elementy.empty()) {
        const JsonWartosc& e = bl->elementy.front();
        const std::string kod = e.czyObiekt() ? tekstPola(e, "Code") : std::string();
        const std::string wiad = e.czyObiekt() ? tekstPola(e, "Message") : std::string();
        const std::string pole = e.czyObiekt() ? tekstPola(e, "PropertyName") : std::string();
        if (maleASCII(pole).find("api key") != std::string::npos || maleASCII(wiad).find("unique identifier") != std::string::npos)
            blad = "Mouser odrzucił klucz API (sprawdź go w Ustawieniach).";
        else
            blad = "Mouser zgłosił błąd: " + (wiad.empty() ? kod : wiad);
        return false;
    }
    const JsonWartosc* wyniki = korzen.pole("SearchResults");
    const JsonWartosc* czesciJson = wyniki && wyniki->czyObiekt() ? wyniki->pole("Parts") : nullptr;
    if (!czesciJson || !czesciJson->czyTablica()) return true;   // brak trafień
    for (const JsonWartosc& p : czesciJson->elementy) {
        if (!p.czyObiekt()) continue;
        Czesc c;
        c.numerMouser = tekstPola(p, "MouserPartNumber");
        c.numerProducenta = tekstPola(p, "ManufacturerPartNumber");
        c.producent = tekstPola(p, "Manufacturer");
        c.opis = tekstPola(p, "Description");
        c.kategoria = tekstPola(p, "Category");
        c.adresDatasheetu = poprawAdres(tekstPola(p, "DataSheetUrl"));
        c.adresProduktu = poprawAdres(tekstPola(p, "ProductDetailUrl"));
        c.cyklZycia = tekstPola(p, "LifecycleStatus");
        if (const JsonWartosc* at = p.pole("ProductAttributes"); at && at->czyTablica())
            for (const JsonWartosc& a : at->elementy)
                if (a.czyObiekt()) {
                    const std::string n = tekstPola(a, "AttributeName"), w = tekstPola(a, "AttributeValue");
                    if (!n.empty() && !w.empty()) c.atrybuty.emplace_back(n, w);
                }
        if (c.numerProducenta.empty() && c.numerMouser.empty()) continue;
        czesci.push_back(std::move(c));
    }
    return true;
}

} // namespace Mouser
