#include "LampyKatalog.h"
#include "../Narzedzia/Liczby.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace LampyKatalog {
namespace {

std::optional<double> liczba(const PozycjaKatalogu& p, const char* nazwa) {
    const ParametrKatalogu* par = p.parametr(nazwa);
    if (!par) return std::nullopt;
    double v = 0;
    if (!tekstNaLiczbe(par->wartosc, v) || !std::isfinite(v) || v < 0) return std::nullopt;
    return v;
}
void dodaj(std::vector<ParametrKatalogu>& w, const char* nazwa, double v, const std::string& zr) {
    w.push_back({nazwa, liczbaNaTekstPolski(v, 4, 0), zr});
}
std::string malymi(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

} // namespace

bool czyLampa(const PozycjaKatalogu& p) {
    if (malymi(p.rodzina) == malymi(kRodzina)) return true;
    return p.parametr(kParKa) && (p.parametr(kParRwew) || p.parametr(kParSa));
}

std::optional<DaneLampy> zPozycji(const PozycjaKatalogu& p) {
    auto ka = liczba(p, kParKa), rw = liczba(p, kParRwew), sa = liczba(p, kParSa);
    if (ka && *ka <= 0) ka.reset();
    if (rw && *rw <= 0) rw.reset();
    if (sa && *sa <= 0) sa.reset();
    DaneLampy d;
    if (ka && rw) {
        d.ka = *ka;
        d.rwew = *rw;
        d.sa = sa ? *sa : *ka / *rw;
    } else if (ka && sa) {
        d.ka = *ka;
        d.sa = *sa;
        d.rwew = *ka / *sa;
    } else if (rw && sa) {
        d.rwew = *rw;
        d.sa = *sa;
        d.ka = *sa * *rw;
    } else {
        return std::nullopt;
    }
    d.cak = liczba(p, kParCak);
    d.cas = liczba(p, kParCas);
    d.csk = liczba(p, kParCsk);
    d.paMaxW = liczba(p, kParPaMax);
    if (d.paMaxW && *d.paMaxW <= 0) d.paMaxW.reset();
    d.raaPPOm = liczba(p, kParRaaPP);
    if (d.raaPPOm && *d.raaPPOm <= 0) d.raaPPOm.reset();
    d.ia0Ma = liczba(p, kParIa0);
    if (d.ia0Ma && *d.ia0Ma <= 0) d.ia0Ma.reset();
    if (const ParametrKatalogu* t = p.parametr(kParTyp); t && !t->wartosc.empty()) d.typ = t->wartosc;
    return d;
}

std::vector<ParametrKatalogu> parametry(const DaneLampy& d, const std::string& zr) {
    std::vector<ParametrKatalogu> w;
    w.push_back({kParTyp, d.typ, zr});
    dodaj(w, kParSa, d.sa, zr);
    dodaj(w, kParKa, d.ka, zr);
    dodaj(w, kParRwew, d.rwew, zr);
    if (d.cak) dodaj(w, kParCak, *d.cak, zr);
    if (d.cas) dodaj(w, kParCas, *d.cas, zr);
    if (d.csk) dodaj(w, kParCsk, *d.csk, zr);
    if (d.paMaxW) dodaj(w, kParPaMax, *d.paMaxW, zr);
    if (d.ia0Ma) dodaj(w, kParIa0, *d.ia0Ma, zr);
    if (d.raaPPOm) dodaj(w, kParRaaPP, *d.raaPPOm, zr);
    return w;
}

PozycjaKatalogu pozycja(const std::string& symbol, const DaneLampy& d, const std::string& opis, const std::string& uwagi) {
    PozycjaKatalogu p;
    p.symbol = symbol;
    p.opis = opis;
    p.rodzina = kRodzina;
    p.uwagi = uwagi;
    p.parametry = parametry(d);
    return p;
}

std::vector<PozycjaKatalogu> przykladowe() {
    const std::string uw = "Parametry z przykładów ECCLab 7.2 (moc anody max: typowa z kart katalogowych, na jedną sekcję); sprawdź w karcie katalogowej dla swojego punktu pracy.";
    auto mk = [&](const char* sym, const char* opis, double sa, double ka, double rw, std::optional<double> cak, double cas,
                  double csk, double paMax, const std::string& dodatek = "") {
        DaneLampy d;
        d.sa = sa; d.ka = ka; d.rwew = rw; d.cak = cak; d.cas = cas; d.csk = csk; d.paMaxW = paMax;
        return pozycja(sym, d, opis, dodatek.empty() ? uw : uw + " " + dodatek);
    };
    return {mk("ECC81", "Podwójna trioda, wzmacniacz w.cz./audio", 5.5, 60, 11, 0.2, 1.6, 2.3, 2.5),
            mk("ECC82", "Podwójna trioda, wzmacniacz audio/sterownik", 2.2, 17, 7.7, 0.5, 1.5, 1.8, 2.75),
            mk("ECC83", "Podwójna trioda, wzmacniacz napięciowy (12AX7)", 1.6, 100, 62.5, 0.45, 1.1, 1.6, 1.0),
            mk("ECC85", "Podwójna trioda, wzmacniacz audio", 6.1, 55, 9, std::nullopt, 1.5, 3, 2.5,
               "Sa obliczone z Ka / Rwew; brak Cak."),
            mk("ECC88", "Podwójna trioda, wzmacniacz kaskodowy (6DJ8)", 12.5, 33, 2.6, 0.18, 1.4, 3.1, 1.8)};
}

std::vector<PozycjaKatalogu> przykladoweMocy() {
    const std::string uw = "Pentoda mocy do stopnia przeciwsobnego. Sa i Rwew orientacyjne z kart katalogowych (punkt pracy zbliżony do typowego); "
                           "Ia0 = typowy prąd spoczynkowy jednej lampy w PP klasy AB, Raa PP = typowa impedancja anoda-anoda pary. Sprawdź w karcie dla swojego napięcia anody.";
    auto mk = [&](const char* sym, const char* opis, double sa, double rw, double ia0, double raa, double paMax, const std::string& dodatek = "") {
        DaneLampy d;
        d.typ = "pentoda";
        d.sa = sa; d.rwew = rw; d.ka = sa * rw; d.ia0Ma = ia0; d.raaPPOm = raa; d.paMaxW = paMax;
        return pozycja(sym, d, opis, dodatek.empty() ? uw : uw + " " + dodatek);
    };
    return {mk("EL34", "Pentoda mocy 25 W (6CA7)", 11, 17, 45, 6600, 25, "Sa i Ri jak w karcie dla Ia = 100 mA."),
            mk("6P14P", "Pentoda mocy 12 W (odpowiednik EL84)", 11.3, 38, 25, 8000, 12, "Dane jak EL84; do weryfikacji."),
            mk("EL84", "Pentoda mocy 12 W (6BQ5)", 11.3, 38, 25, 8000, 12),
            mk("6V6", "Tetroda strumieniowa mocy 12 W", 4.1, 52, 30, 8000, 12),
            mk("6L6", "Tetroda strumieniowa mocy 30 W (6L6GC)", 6, 22.5, 40, 4000, 30),
            mk("ECL86", "Trioda + pentoda mocy 9 W (sekcja pentody)", 10, 48, 20, 10000, 9),
            mk("ECL82", "Trioda + pentoda mocy 7 W (sekcja pentody)", 6.4, 20, 20, 10000, 7, "Ri orientacyjne, do weryfikacji w karcie."),
            mk("EBL21", "Podwójna dioda + pentoda mocy 9 W (sekcja pentody)", 8.5, 50, 20, 10000, 9, "Sa i Ri orientacyjne, do weryfikacji w karcie.")};
}

} // namespace LampyKatalog
