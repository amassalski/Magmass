#include "InfirayUsb.h"

#include <cstring>
#include <mutex>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace InfirayUsb {

namespace {
// Własny układ struktur zwracanych przez bibliotekę (SearchDevice): liczniki i tablice po 50 pozycji, nazwy po 260 znaków
constexpr int kMaxUrz = 50, kMaxPath = 260;
struct DeviceInfoC { int id; char nazwa[kMaxPath]; };
struct ComNameC { char port[kMaxPath]; };
struct DeviceLstC { int liczbaPortow; int liczbaUrzadzen; DeviceInfoC urz[kMaxUrz]; ComNameC porty[kMaxUrz]; };

using Uchwyt = void*;
using CbKlatki = void (*)(unsigned char*, int, int, void*);
}  // namespace

std::vector<uint16_t> slowa16(const Klatka& k) {
    std::vector<uint16_t> w;
    if (k.pusta()) return w;
    const size_t n = std::min(static_cast<size_t>(k.szer) * k.wys, k.dane.size() / 2);
    w.resize(n);
    for (size_t i = 0; i < n; ++i) w[i] = static_cast<uint16_t>(k.dane[2 * i] | (k.dane[2 * i + 1] << 8));
    return w;
}

std::string opisTypuRdzenia(int typ) {
    switch (typ) {
        case 0: return "błąd odczytu";
        case 1: return "LT, pomiar temperatury";
        case 2: return "MicroIII, pomiar temperatury";
        case 3: return "MicroIII, tylko obraz";
        case 4: return "AT200F";
        case 5: return "AT21F";
        case 6: return "inny";
        default: return "nieznany (" + std::to_string(typ) + ")";
    }
}

struct Sdk::Impl {
#ifdef _WIN32
    HMODULE dll = nullptr;
#endif
    Uchwyt h = nullptr;
    bool otwarte = false;
    // wskaźniki do funkcji biblioteki
    Uchwyt (*fCreate)() = nullptr;
    int (*fLogin)(Uchwyt, void*) = nullptr;
    void (*fRelease)(Uchwyt) = nullptr;
    int (*fSearch)(Uchwyt, DeviceLstC*) = nullptr;
    bool (*fOpen)(Uchwyt, int, int) = nullptr;
    void (*fClose)(Uchwyt) = nullptr;
    int (*fCore)(Uchwyt) = nullptr;
    void (*fSetVideo)(Uchwyt, CbKlatki, void*) = nullptr;
    void (*fSetTemp)(Uchwyt, CbKlatki, void*) = nullptr;
    int (*fShutter)(Uchwyt, int, int) = nullptr;
    int (*fSnPn)(Uchwyt, char*, int*, char*, int*) = nullptr;
    int (*fFpa)(Uchwyt, float*) = nullptr;
    int (*fCam)(Uchwyt, float*) = nullptr;
    int (*fW)(Uchwyt, int*) = nullptr;
    int (*fH)(Uchwyt, int*) = nullptr;

    mutable std::mutex mx;
    Klatka obraz, temp;

    static void wideo(unsigned char* b, int w, int hh, void* ctx) { static_cast<Impl*>(ctx)->zapisz(b, w, hh, false); }
    static void temperatura(unsigned char* b, int w, int hh, void* ctx) { static_cast<Impl*>(ctx)->zapisz(b, w, hh, true); }
    void zapisz(const unsigned char* b, int w, int hh, bool czyTemp) {
        if (!b || w <= 0 || hh <= 0 || w > 4096 || hh > 4096) return;
        std::lock_guard<std::mutex> l(mx);
        Klatka& k = czyTemp ? temp : obraz;
        k.szer = w; k.wys = hh;
        k.dane.assign(b, b + static_cast<size_t>(w) * hh * 2);
        ++k.numer;
    }
};

Sdk::Sdk() : p_(new Impl) {}
Sdk::~Sdk() { zwolnij(); }

bool Sdk::zaladowana() const {
#ifdef _WIN32
    return p_->dll != nullptr;
#else
    return false;
#endif
}

bool Sdk::zaladuj(const std::string& sciezka, std::string& blad) {
    zwolnij();
#ifndef _WIN32
    (void)sciezka;
    blad = "Biblioteka USBSDK.dll od IRay działa tylko w systemie Windows.";
    return false;
#else
#if !defined(_WIN64)
    blad = "Obsługa SDK IRay jest przewidziana tylko dla programu 64-bitowego (USBSDK.dll z katalogu x64).";
    return false;
#else
    if (sciezka.empty()) { blad = "Nie podano ścieżki do USBSDK.dll."; return false; }
    const int n = MultiByteToWideChar(CP_UTF8, 0, sciezka.c_str(), -1, nullptr, 0);
    std::wstring ws(n > 0 ? n : 1, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, sciezka.c_str(), -1, &ws[0], n);
    // LOAD_WITH_ALTERED_SEARCH_PATH: zależności (np. opencv*.dll) szukane obok pliku biblioteki
    p_->dll = LoadLibraryExW(ws.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!p_->dll) {
        blad = "Nie można załadować biblioteki (kod błędu Windows " + std::to_string(GetLastError()) +
               "). Sprawdź, czy to wersja x64 i czy obok leżą jej zależności.";
        return false;
    }
    auto f = [&](const char* nazwa) { return reinterpret_cast<void*>(GetProcAddress(p_->dll, nazwa)); };
#define DM_FN(pole, nazwa) pole = reinterpret_cast<decltype(pole)>(f(nazwa))
    DM_FN(p_->fCreate, "sdk_create"); DM_FN(p_->fLogin, "sdk_loginDevice"); DM_FN(p_->fRelease, "ReleaseSDK");
    DM_FN(p_->fSearch, "SearchDevice"); DM_FN(p_->fOpen, "OpenDevice"); DM_FN(p_->fClose, "CloseDevice");
    DM_FN(p_->fCore, "CoreType"); DM_FN(p_->fSetVideo, "SetVideoCallBack"); DM_FN(p_->fSetTemp, "SetTempCallBack");
    DM_FN(p_->fShutter, "sdk_shutter_correction"); DM_FN(p_->fSnPn, "sdk_get_SN_PN");
    DM_FN(p_->fFpa, "sdk_get_FPA_temp"); DM_FN(p_->fCam, "sdk_get_camera_temp");
    DM_FN(p_->fW, "sdk_get_width"); DM_FN(p_->fH, "sdk_get_height");
#undef DM_FN
    // do działania wystarczą funkcje podstawowe; pozostałe (SN, wymiary, migawka) są opcjonalne
    if (!p_->fCreate || !p_->fSearch || !p_->fOpen || !p_->fClose || !p_->fSetVideo || !p_->fSetTemp) {
        blad = "To nie wygląda na USBSDK.dll od IRay: brak wymaganych funkcji (sdk_create, SearchDevice, OpenDevice...).";
        zwolnij();
        return false;
    }
    p_->h = p_->fCreate();
    if (!p_->h) { blad = "Biblioteka nie utworzyła obiektu SDK (sdk_create zwróciło 0)."; zwolnij(); return false; }
    return true;
#endif
#endif
}

void Sdk::zwolnij() {
    zamknij();
#ifdef _WIN32
    if (p_->h && p_->fRelease) p_->fRelease(p_->h);
    p_->h = nullptr;
    if (p_->dll) { FreeLibrary(p_->dll); p_->dll = nullptr; }
#endif
    p_.reset(new Impl);
}

std::vector<Urzadzenie> Sdk::szukaj(std::vector<std::string>* porty) {
    std::vector<Urzadzenie> wynik;
    if (!zaladowana() || !p_->h) return wynik;
    DeviceLstC l;
    std::memset(&l, 0, sizeof(l));
    p_->fSearch(p_->h, &l);
    for (int i = 0; i < l.liczbaUrzadzen && i < kMaxUrz; ++i) {
        l.urz[i].nazwa[kMaxPath - 1] = '\0';
        wynik.push_back({l.urz[i].id, l.urz[i].nazwa});
    }
    if (porty)
        for (int i = 0; i < l.liczbaPortow && i < kMaxUrz; ++i) { l.porty[i].port[kMaxPath - 1] = '\0'; porty->push_back(l.porty[i].port); }
    return wynik;
}

bool Sdk::otworz(int id, int port, void* okno, std::string& blad) {
    if (!zaladowana() || !p_->h) { blad = "Biblioteka nie jest załadowana."; return false; }
    if (p_->otwarte) zamknij();
    if (p_->fLogin) p_->fLogin(p_->h, okno);
    if (!p_->fOpen(p_->h, id, port)) { blad = "OpenDevice nie powiodło się (kamera zajęta, inny sterownik albo zły numer urządzenia)."; return false; }
    p_->fSetVideo(p_->h, &Impl::wideo, p_.get());
    p_->fSetTemp(p_->h, &Impl::temperatura, p_.get());
    p_->otwarte = true;
    return true;
}

void Sdk::zamknij() {
    if (p_->otwarte && p_->h && p_->fClose) p_->fClose(p_->h);
    p_->otwarte = false;
}

bool Sdk::otwarte() const { return p_->otwarte; }

int Sdk::typRdzenia() { return p_->otwarte && p_->fCore ? p_->fCore(p_->h) : 0; }

bool Sdk::numeryPNSN(std::string& pn, std::string& sn) {
    if (!p_->otwarte || !p_->fSnPn) return false;
    char s[256] = {0}, p[256] = {0};
    int ls = 255, lp = 255;
    if (p_->fSnPn(p_->h, s, &ls, p, &lp) != 0) return false;
    s[255] = p[255] = '\0';
    sn = s; pn = p;
    return true;
}

bool Sdk::wymiary(int& szer, int& wys) {
    return p_->otwarte && p_->fW && p_->fH && p_->fW(p_->h, &szer) == 0 && p_->fH(p_->h, &wys) == 0;
}
bool Sdk::temperaturaFPA(float& t) { return p_->otwarte && p_->fFpa && p_->fFpa(p_->h, &t) == 0; }
bool Sdk::temperaturaKamery(float& t) { return p_->otwarte && p_->fCam && p_->fCam(p_->h, &t) == 0; }
bool Sdk::migawka(int typ) { return p_->otwarte && p_->fShutter && p_->fShutter(p_->h, typ, 0) == 0; }

bool Sdk::ostatniObraz(Klatka& k) const {
    std::lock_guard<std::mutex> l(p_->mx);
    if (p_->obraz.pusta()) return false;
    k = p_->obraz;
    return true;
}
bool Sdk::ostatniaTemp(Klatka& k) const {
    std::lock_guard<std::mutex> l(p_->mx);
    if (p_->temp.pusta()) return false;
    k = p_->temp;
    return true;
}

}  // namespace InfirayUsb
