#include "Termowizja.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace Termowizja {

RozpoznanyUklad rozpoznaj(const KlatkaSurowa& k) {
    RozpoznanyUklad u;
    u.obrazSzer = k.szer;
    u.obrazWys = k.wys;
    if (k.pusta()) { u.opis = "brak klatki"; return u; }
    const int polowa = k.wys / 2;
    if (k.bajtyNaPiksel() == 2 && k.wys % 2 == 0 && polowa * 4 == k.szer * 3) {
        u.uklad = Uklad::ObrazIDane;
        u.obrazWys = polowa;
        u.opis = "obraz " + std::to_string(k.szer) + "x" + std::to_string(polowa) + " u góry, dane 16-bitowe poniżej";
    } else if (k.bajtyNaPiksel() == 2 && k.szer % 4 == 0 && k.wys == k.szer * 3 / 4 + 4) {
        u.obrazWys = k.wys - 4;
        bool stala = true;
        for (int y = 0; y < u.obrazWys && stala; ++y) {
            const unsigned char* w = &k.dane[static_cast<size_t>(y) * k.bajtyWiersza()];
            for (int x = 0; x < k.szer; ++x) if (w[2 * x + 1] != 0x80) { stala = false; break; }
        }
        u.uklad = stala ? Uklad::ObrazZNaglowkiem : Uklad::Surowe16;
        u.opis = std::string(stala ? "obraz 8-bitowy " : "surowe dane 16-bitowe ") + std::to_string(k.szer) + "x" + std::to_string(u.obrazWys) + " + 4 wiersze nagłówka";
    } else {
        u.opis = "tylko obraz " + std::to_string(k.szer) + "x" + std::to_string(k.wys) + " (bez danych temperatury)";
    }
    return u;
}

std::vector<uint16_t> daneRadiometryczne(const KlatkaSurowa& k, const RozpoznanyUklad& u) {
    std::vector<uint16_t> d;
    if ((u.uklad != Uklad::ObrazIDane && u.uklad != Uklad::Surowe16) || k.pusta()) return d;
    d.reserve(static_cast<size_t>(u.obrazSzer) * u.obrazWys);
    const int przesuniecie = u.uklad == Uklad::ObrazIDane ? u.obrazWys : 0;
    for (int y = 0; y < u.obrazWys; ++y) {
        const unsigned char* w = &k.dane[(static_cast<size_t>(przesuniecie + y)) * k.bajtyWiersza()];
        for (int x = 0; x < u.obrazSzer; ++x) d.push_back(static_cast<uint16_t>(w[2 * x] | (w[2 * x + 1] << 8)));
    }
    return d;
}

double naCelsjusza(uint16_t surowe, double dzielnik, double offsetK) {
    if (!(dzielnik > 0)) return 0;
    return surowe / dzielnik - offsetK;
}

Statystyka statystyki(const std::vector<uint16_t>& d, int szer, int wys, double dz, double off) {
    Statystyka s;
    if (szer <= 0 || wys <= 0 || d.size() < static_cast<size_t>(szer) * wys || !(dz > 0)) return s;
    size_t imin = 0, imax = 0;
    double suma = 0;
    const size_t n = static_cast<size_t>(szer) * wys;
    for (size_t i = 0; i < n; ++i) {
        if (d[i] < d[imin]) imin = i;
        if (d[i] > d[imax]) imax = i;
        suma += d[i];
    }
    s.ok = true;
    s.minC = naCelsjusza(d[imin], dz, off);
    s.maxC = naCelsjusza(d[imax], dz, off);
    s.sredniaC = suma / n / dz - off;
    s.srodekC = naCelsjusza(d[static_cast<size_t>(wys / 2) * szer + szer / 2], dz, off);
    s.xMin = static_cast<int>(imin % szer); s.yMin = static_cast<int>(imin / szer);
    s.xMax = static_cast<int>(imax % szer); s.yMax = static_cast<int>(imax / szer);
    return s;
}

namespace {
unsigned char bajt(double v) { return static_cast<unsigned char>(std::lround(std::clamp(v, 0.0, 255.0))); }
}

void kolorPalety(Paleta p, double t, unsigned char rgb[3]) {
    t = std::clamp(t, 0.0, 1.0);
    switch (p) {
    case Paleta::Szara: rgb[0] = rgb[1] = rgb[2] = bajt(t * 255); return;
    case Paleta::SzaraOdwrocona: rgb[0] = rgb[1] = rgb[2] = bajt((1 - t) * 255); return;
    case Paleta::Teczowa: {
        // od niebieskiego (zimno) przez zieleń i żółć do czerwieni (gorąco)
        const double h = (1 - t) * 240.0 / 60.0;     // 4 -> 0
        const int sekt = static_cast<int>(h);
        const double f = h - sekt;
        double r = 0, g = 0, b = 0;
        switch (sekt) {
            case 0: r = 1; g = f; b = 0; break;
            case 1: r = 1 - f; g = 1; b = 0; break;
            case 2: r = 0; g = 1; b = f; break;
            case 3: r = 0; g = 1 - f; b = 1; break;
            default: r = 0; g = 0; b = 1; break;
        }
        rgb[0] = bajt(r * 255); rgb[1] = bajt(g * 255); rgb[2] = bajt(b * 255);
        return;
    }
    case Paleta::Zelazo: {
        // czerń, fiolet, czerwień, pomarańcz, żółć, biel
        static const double k[][4] = {{0, 0, 0, 0}, {0.25, 60, 0, 120}, {0.5, 200, 30, 40}, {0.75, 255, 150, 0}, {0.9, 255, 230, 60}, {1, 255, 255, 255}};
        for (int i = 1; i < 6; ++i)
            if (t <= k[i][0]) {
                const double f = (t - k[i - 1][0]) / (k[i][0] - k[i - 1][0]);
                for (int c = 0; c < 3; ++c) rgb[c] = bajt(k[i - 1][c + 1] + f * (k[i][c + 1] - k[i - 1][c + 1]));
                return;
            }
        rgb[0] = rgb[1] = rgb[2] = 255;
        return;
    }
    }
}

std::vector<unsigned char> obrazZDanych(const std::vector<uint16_t>& d, int szer, int wys, Paleta p) {
    std::vector<unsigned char> o;
    if (szer <= 0 || wys <= 0 || d.size() < static_cast<size_t>(szer) * wys) return o;
    const size_t n = static_cast<size_t>(szer) * wys;
    const auto mm = std::minmax_element(d.begin(), d.begin() + n);
    const double lo = *mm.first, zakres = std::max(1.0, double(*mm.second) - lo);
    o.resize(n * 3);
    for (size_t i = 0; i < n; ++i) kolorPalety(p, (d[i] - lo) / zakres, &o[i * 3]);
    return o;
}

std::vector<unsigned char> obrazZJasnosci(const KlatkaSurowa& k, const RozpoznanyUklad& u, Paleta p) {
    std::vector<unsigned char> o;
    if (k.pusta() || u.obrazSzer <= 0 || u.obrazWys <= 0) return o;
    const int bpp = k.bajtyNaPiksel();
    std::vector<double> jas(static_cast<size_t>(u.obrazSzer) * u.obrazWys);
    double lo = 1e9, hi = -1e9;
    for (int y = 0; y < u.obrazWys; ++y) {
        const unsigned char* w = &k.dane[static_cast<size_t>(y) * k.bajtyWiersza()];
        for (int x = 0; x < u.obrazSzer; ++x) {
            double j;
            if (k.bajtyNaKanal == 2) j = w[x * bpp] | (w[x * bpp + 1] << 8);
            else if (k.kanaly >= 3) j = 0.114 * w[x * bpp] + 0.587 * w[x * bpp + 1] + 0.299 * w[x * bpp + 2];
            else j = w[x * bpp];                       // YUYV lub jeden kanał: bajt Y jest pierwszy w każdym pikselu
            jas[static_cast<size_t>(y) * u.obrazSzer + x] = j;
            lo = std::min(lo, j); hi = std::max(hi, j);
        }
    }
    const double zakres = std::max(1.0, hi - lo);
    o.resize(jas.size() * 3);
    for (size_t i = 0; i < jas.size(); ++i) kolorPalety(p, (jas[i] - lo) / zakres, &o[i * 3]);
    return o;
}

std::string opisNaglowka(const KlatkaSurowa& k, const RozpoznanyUklad& u) {
    std::ostringstream s;
    if (k.pusta() || (u.uklad != Uklad::ObrazZNaglowkiem && u.uklad != Uklad::Surowe16)) return std::string();
    const size_t wiersz = k.bajtyWiersza();
    const unsigned char* p0 = &k.dane[static_cast<size_t>(u.obrazWys) * wiersz];
    const unsigned char* p3 = &k.dane[static_cast<size_t>(u.obrazWys + 3) * wiersz];
    s << "Nagłówek, wiersz 1, słowa 16-bitowe:";
    unsigned slowa[16];
    for (int i = 0; i < 16; ++i) { slowa[i] = p0[2 * i] | (p0[2 * i + 1] << 8); s << " " << slowa[i]; }
    s << "\n";
    // w obserwowanych ramkach słowo 4 to największy, a słowo 7 najmniejszy odczyt obrazu (sprawdzane poniżej)
    const auto d = daneRadiometryczne(k, u);
    if (!d.empty()) {
        const auto mm = std::minmax_element(d.begin(), d.end());
        s << "Słowo 4 = największy odczyt: " << (slowa[4] == *mm.second ? "zgadza się" : "nie zgadza się")
          << ", słowo 7 = najmniejszy odczyt: " << (slowa[7] == *mm.first ? "zgadza się" : "nie zgadza się") << "\n";
    }
    // parametry pomiaru: od bajtu 26 ostatniego wiersza: 4 liczby float (od +4: T odbita, T otoczenia, wilgotność, emisyjność) i odległość (uint16)
    float f[4];
    std::memcpy(f, p3 + 26 + 4, sizeof f);
    const unsigned odl = p3[26 + 20] | (p3[26 + 21] << 8);
    char b[200];
    std::snprintf(b, sizeof b, "Parametry z nagłówka: T odbita %.4g, T otoczenia %.4g, wilgotność %.4g, emisyjność %.4g, odległość %u\n",
                  static_cast<double>(f[0]), static_cast<double>(f[1]), static_cast<double>(f[2]), static_cast<double>(f[3]), odl);
    s << b;
    return s.str();
}

std::string raportDiagnostyczny(const KlatkaSurowa& k, double dz, double off) {
    std::ostringstream s;
    if (k.pusta()) return "Brak klatki.\n";
    const RozpoznanyUklad u = rozpoznaj(k);
    s << "Rozmiar: " << k.szer << " x " << k.wys << ", kanały: " << k.kanaly << ", bajtów na kanał: " << k.bajtyNaKanal
      << (k.opisTypu.empty() ? "" : " (" + k.opisTypu + ")") << "\n";
    s << "Bajtów w klatce: " << k.dane.size() << ", w wierszu: " << k.bajtyWiersza() << "\n";
    s << "Układ (rozpoznany z rozmiaru): " << u.opis << "\n";
    s << "Pierwsze 16 bajtów:";
    for (size_t i = 0; i < 16 && i < k.dane.size(); ++i) { char b[8]; std::snprintf(b, sizeof b, " %02X", k.dane[i]); s << b; }
    s << "\n";
    const auto mm = std::minmax_element(k.dane.begin(), k.dane.end());
    s << "Zakres bajtów: " << int(*mm.first) << " ... " << int(*mm.second) << "\n";
    const auto d = daneRadiometryczne(k, u);
    if (u.uklad == Uklad::ObrazZNaglowkiem || u.uklad == Uklad::Surowe16) s << opisNaglowka(k, u);
    if (u.uklad == Uklad::Surowe16 && !d.empty()) {
        const auto r = std::minmax_element(d.begin(), d.end());
        char b[160];
        std::snprintf(b, sizeof b, "Surowe odczyty 16-bitowe: %u ... %u (bez kalibracji do °C: wzór producenta nie jest publiczny)\n", unsigned(*r.first), unsigned(*r.second));
        s << b;
    } else if (u.uklad == Uklad::ObrazZNaglowkiem) {
        s << "To obraz 8-bitowy ze skalowaniem jasności w każdej klatce: temperatury z niego nie wynikają. Spróbuj przełączyć kamerę w tryb surowy (przycisk obok).\n";
    } else if (!d.empty()) {
        const auto st = statystyki(d, u.obrazSzer, u.obrazWys, dz, off);
        const auto r = std::minmax_element(d.begin(), d.end());
        char b[256];
        std::snprintf(b, sizeof b, "Dane 16-bitowe: %u ... %u\n", unsigned(*r.first), unsigned(*r.second));
        s << b;
        std::snprintf(b, sizeof b, "Przy T = surowe/%.4g − %.2f: min %.1f °C, max %.1f °C, średnia %.1f °C, środek %.1f °C\n",
                      dz, off, st.minC, st.maxC, st.sredniaC, st.srodekC);
        s << b;
        s << "Sprawdź wynik znaną temperaturą (woda z lodem 0 °C, dłoń ok. 33 °C). Jeśli się nie zgadza, zmień dzielnik i "
             "przesunięcie albo prześlij zapisaną próbkę do analizy.\n";
    } else {
        s << "Ta klatka nie wygląda na ramkę z danymi temperatury. Zapisz próbkę i sprawdź, co kamera podaje.\n";
    }
    return s.str();
}

void KalibracjaUzytkownika::dodaj(double surowe, double t) {
    for (auto& p : p_) if (std::fabs(p.surowe - surowe) <= 0.5) { p.surowe = surowe; p.temperaturaC = t; return; }
    p_.push_back({surowe, t});
    std::sort(p_.begin(), p_.end(), [](const PunktKalibracji& a, const PunktKalibracji& b) { return a.surowe < b.surowe; });
}

double KalibracjaUzytkownika::naCelsjusza(double s) const {
    if (!gotowa()) return 0;
    size_t i = 1;
    while (i + 1 < p_.size() && s > p_[i].surowe) ++i;
    const PunktKalibracji &a = p_[i - 1], &b = p_[i];
    return a.temperaturaC + (s - a.surowe) * (b.temperaturaC - a.temperaturaC) / (b.surowe - a.surowe);
}

double sredniaSurowych(const std::vector<uint16_t>& d, int szer, int wys, int cx, int cy, int r) {
    if (szer <= 0 || wys <= 0 || d.size() < static_cast<size_t>(szer) * wys) return 0;
    double suma = 0;
    int n = 0;
    for (int y = std::max(0, cy - r); y <= std::min(wys - 1, cy + r); ++y)
        for (int x = std::max(0, cx - r); x <= std::min(szer - 1, cx + r); ++x) { suma += d[static_cast<size_t>(y) * szer + x]; ++n; }
    return n ? suma / n : 0;
}

Statystyka statystykiKalibrowane(const std::vector<uint16_t>& d, int szer, int wys, const KalibracjaUzytkownika& k) {
    Statystyka s;
    if (!k.gotowa() || szer <= 0 || wys <= 0 || d.size() < static_cast<size_t>(szer) * wys) return s;
    const size_t n = static_cast<size_t>(szer) * wys;
    double suma = 0, tmin = 1e18, tmax = -1e18;
    size_t imin = 0, imax = 0;
    for (size_t i = 0; i < n; ++i) {
        const double t = k.naCelsjusza(d[i]);
        suma += t;
        if (t < tmin) { tmin = t; imin = i; }
        if (t > tmax) { tmax = t; imax = i; }
    }
    s.ok = true;
    s.minC = tmin; s.maxC = tmax; s.sredniaC = suma / n;
    s.srodekC = k.naCelsjusza(sredniaSurowych(d, szer, wys, szer / 2, wys / 2, 2));
    s.xMin = static_cast<int>(imin % szer); s.yMin = static_cast<int>(imin / szer);
    s.xMax = static_cast<int>(imax % szer); s.yMax = static_cast<int>(imax / szer);
    return s;
}

KlatkaSurowa klatkaTestowaSurowa(long n) {
    KlatkaSurowa k;
    k.szer = 384; k.wys = 292; k.kanaly = 2; k.bajtyNaKanal = 1; k.opisTypu = "AVF yuvs (test)";
    k.dane.assign(static_cast<size_t>(k.szer) * k.wys * 2, 0);
    const double cx = 192 + 80 * std::sin(n / 15.0), cy = 144 + 40 * std::cos(n / 11.0);
    uint16_t mn = 0xFFFF, mx = 0;
    for (int y = 0; y < 288; ++y)
        for (int x = 0; x < 384; ++x) {
            const double r = std::hypot(x - cx, y - cy);
            const uint16_t v = static_cast<uint16_t>(std::lround(4800 + 200.0 * x / 383.0 + 400.0 * std::exp(-r * r / 600.0)));
            unsigned char* p = &k.dane[(static_cast<size_t>(y) * 384 + x) * 2];
            p[0] = v & 0xFF; p[1] = v >> 8;
            mn = std::min(mn, v); mx = std::max(mx, v);
        }
    auto slowo = [&](int nr, uint16_t v) { unsigned char* p = &k.dane[static_cast<size_t>(288) * 768 + nr * 2]; p[0] = v & 0xFF; p[1] = v >> 8; };
    slowo(4, mx); slowo(7, mn);
    unsigned char* w4 = &k.dane[static_cast<size_t>(291) * 768 + 26];
    const float par[4] = {25.0f, 25.0f, 0.45f, 0.98f};
    std::memcpy(w4 + 4, par, sizeof par);
    w4[20] = 1;
    return k;
}

KlatkaSurowa klatkaTestowa(long n) {
    KlatkaSurowa k;
    k.szer = 256; k.wys = 384; k.kanaly = 2; k.bajtyNaKanal = 1; k.opisTypu = "CV_8UC2 (test)";
    k.dane.assign(static_cast<size_t>(k.szer) * k.wys * 2, 128);
    const double cx = 128 + 60 * std::sin(n / 15.0), cy = 96 + 30 * std::cos(n / 11.0);
    for (int y = 0; y < 192; ++y)
        for (int x = 0; x < 256; ++x) {
            const double r = std::hypot(x - cx, y - cy);
            const double tC = 20 + 40.0 * x / 255.0 + 20.0 * std::exp(-r * r / 400.0);      // 20 ... ~80 °C
            const uint16_t surowe = static_cast<uint16_t>(std::lround((tC + 273.15) * 64.0));
            unsigned char* dol = &k.dane[(static_cast<size_t>(192 + y) * 256 + x) * 2];
            dol[0] = surowe & 0xFF; dol[1] = surowe >> 8;
            unsigned char* gora = &k.dane[(static_cast<size_t>(y) * 256 + x) * 2];
            gora[0] = static_cast<unsigned char>(std::clamp((tC - 20) * 3.2, 0.0, 255.0));
            gora[1] = 128;
        }
    return k;
}

}  // namespace Termowizja
