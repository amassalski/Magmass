#include "UwagiHTML.h"

#include <algorithm>
#include <cctype>

namespace UwagiHTML {

namespace {

std::string male(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

void dopiszUTF8(std::string& w, unsigned long cp) {
    if (cp < 0x80) w += static_cast<char>(cp);
    else if (cp < 0x800) { w += static_cast<char>(0xC0 | (cp >> 6)); w += static_cast<char>(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) { w += static_cast<char>(0xE0 | (cp >> 12)); w += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); w += static_cast<char>(0x80 | (cp & 0x3F)); }
    else { w += static_cast<char>(0xF0 | (cp >> 18)); w += static_cast<char>(0x80 | ((cp >> 12) & 0x3F)); w += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); w += static_cast<char>(0x80 | (cp & 0x3F)); }
}

bool znacznikHtml(const std::string& t, size_t i) {
    // '<' + litera albo '/' + litera
    if (t[i] != '<' || i + 1 >= t.size()) return false;
    size_t j = i + 1;
    if (t[j] == '/') ++j;
    return j < t.size() && std::isalpha(static_cast<unsigned char>(t[j])) && t.find('>', j) != std::string::npos;
}

} // namespace

bool czyHtml(const std::string& t) {
    for (size_t i = 0; i < t.size(); ++i) {
        if (znacznikHtml(t, i)) return true;
        if (t[i] == '&') {
            const size_t k = t.find(';', i);
            if (k != std::string::npos && k - i <= 8 && k > i + 1) return true;
        }
    }
    return false;
}

std::string naTekst(const std::string& html) {
    if (!czyHtml(html)) return html;
    std::string w;
    for (size_t i = 0; i < html.size();) {
        if (znacznikHtml(html, i)) {
            const size_t k = html.find('>', i);
            std::string nazwa = male(html.substr(i + 1, k - i - 1));
            const bool zamykajacy = !nazwa.empty() && nazwa[0] == '/';
            if (zamykajacy) nazwa.erase(0, 1);
            size_t e = 0;
            while (e < nazwa.size() && std::isalnum(static_cast<unsigned char>(nazwa[e]))) ++e;
            nazwa = nazwa.substr(0, e);
            auto nowaLinia = [&] { if (!w.empty() && w.back() != '\n') w += '\n'; };
            if (nazwa == "br" || nazwa == "hr") nowaLinia();
            else if (zamykajacy && (nazwa == "p" || nazwa == "li" || nazwa == "tr" || nazwa == "div" || nazwa == "h1" || nazwa == "h2" ||
                                    nazwa == "h3" || nazwa == "h4" || nazwa == "ul" || nazwa == "ol" || nazwa == "table")) nowaLinia();
            else if (!zamykajacy && nazwa == "li") { nowaLinia(); w += "- "; }
            else if (!zamykajacy && (nazwa == "td" || nazwa == "th") && !w.empty() && w.back() != '\n') w += "\t";
            i = k + 1;
        } else if (html[i] == '&') {
            const size_t k = html.find(';', i);
            if (k != std::string::npos && k - i <= 8 && k > i + 1) {
                const std::string enc = html.substr(i + 1, k - i - 1);
                bool znane = true;
                if (enc == "amp") w += '&';
                else if (enc == "lt") w += '<';
                else if (enc == "gt") w += '>';
                else if (enc == "quot") w += '"';
                else if (enc == "apos") w += '\'';
                else if (enc == "nbsp") w += ' ';
                else if (enc[0] == '#' && enc.size() > 1) {
                    try {
                        const unsigned long cp = (enc[1] == 'x' || enc[1] == 'X') ? std::stoul(enc.substr(2), nullptr, 16) : std::stoul(enc.substr(1));
                        dopiszUTF8(w, cp);
                    } catch (...) { znane = false; }
                } else znane = false;
                if (znane) { i = k + 1; continue; }
            }
            w += html[i++];
        } else {
            w += html[i++];
        }
    }
    // koniec: bez zbędnych końcowych nowych linii
    while (!w.empty() && (w.back() == '\n' || w.back() == ' ')) w.pop_back();
    return w;
}

std::string zTekstu(const std::string& tekst) {
    if (czyHtml(tekst)) return tekst;
    std::string w;
    for (char c : tekst) {
        switch (c) {
            case '&': w += "&amp;"; break;
            case '<': w += "&lt;"; break;
            case '>': w += "&gt;"; break;
            case '\n': w += "<br>\n"; break;
            case '\r': break;
            default: w += c;
        }
    }
    return w;
}

} // namespace UwagiHTML
