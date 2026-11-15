#include "XthermDll.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace XthermLib {

struct Biblioteka::Impl {
#ifdef _WIN32
    HMODULE dll = nullptr;
#endif
    int szer = 0, wys = 0;
    int (*fInit)(int, int) = nullptr;
    void (*fUpdate)(int, unsigned char*) = nullptr;
    void (*fGet)(int, unsigned char*, float*, int*, int*, float*, int*, int*, float*, float*, float*) = nullptr;
    void (*fDev)(float*, float*, int*, int*) = nullptr;
};

Biblioteka::Biblioteka() : p_(new Impl) {}
Biblioteka::~Biblioteka() { zwolnij(); }

bool Biblioteka::zaladowana() const {
#ifdef _WIN32
    return p_->dll != nullptr;
#else
    return false;
#endif
}

void Biblioteka::zwolnij() {
#ifdef _WIN32
    if (p_->dll) FreeLibrary(p_->dll);
#endif
    p_.reset(new Impl);
}

bool Biblioteka::zaladuj(const std::string& sciezka, std::string& blad) {
    zwolnij();
#ifndef _WIN32
    (void)sciezka;
    blad = "Biblioteka XthermDll.dll działa tylko w systemie Windows.";
    return false;
#elif !defined(_WIN64)
    (void)sciezka;
    blad = "Obsługa XthermDll.dll jest przewidziana dla programu 64-bitowego (potrzebna wersja x64 biblioteki).";
    return false;
#else
    if (sciezka.empty()) { blad = "Nie podano ścieżki do XthermDll.dll."; return false; }
    const int n = MultiByteToWideChar(CP_UTF8, 0, sciezka.c_str(), -1, nullptr, 0);
    std::wstring ws(n > 0 ? n : 1, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, sciezka.c_str(), -1, &ws[0], n);
    p_->dll = LoadLibraryExW(ws.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!p_->dll) {
        blad = "Nie można załadować biblioteki (kod błędu Windows " + std::to_string(GetLastError()) +
               "). Sprawdź, czy to wersja x64 (wersja x86 nie zadziała w programie 64-bitowym).";
        return false;
    }
    auto f = [&](const char* nazwa) { return reinterpret_cast<void*>(GetProcAddress(p_->dll, nazwa)); };
    p_->fInit = reinterpret_cast<decltype(p_->fInit)>(f("DataInit"));
    p_->fUpdate = reinterpret_cast<decltype(p_->fUpdate)>(f("UpdateParam"));
    p_->fGet = reinterpret_cast<decltype(p_->fGet)>(f("GetTmpData"));
    p_->fDev = reinterpret_cast<decltype(p_->fDev)>(f("GetDevData"));
    if (!p_->fInit || !p_->fUpdate || !p_->fGet) {
        blad = "To nie wygląda na XthermDll.dll: brak funkcji DataInit, UpdateParam lub GetTmpData.";
        zwolnij();
        return false;
    }
    return true;
#endif
}

Wynik Biblioteka::oblicz(const unsigned char* klatka, size_t bajtow, int szer, int wys, bool pelna, bool odswiez, bool wysoki) {
    Wynik w;
    if (!zaladowana() || !klatka || szer <= 0 || wys <= 0 || bajtow < static_cast<size_t>(szer) * wys * 2) return w;
    // biblioteka zapisuje i czyta bufor bez własnej kontroli rozmiaru: pracujemy na kopii z zapasem
    std::vector<unsigned char> kopia(static_cast<size_t>(szer) * wys * 2 + 4096, 0);
    std::copy(klatka, klatka + static_cast<size_t>(szer) * wys * 2, kopia.begin());
    bool nowy = false;
    if (p_->szer != szer || p_->wys != wys) {
        p_->fInit(szer, wys);
        p_->szer = szer; p_->wys = wys;
        nowy = true;
    }
    if (nowy || odswiez) p_->fUpdate(wysoki ? 1 : 0, kopia.data());
    std::vector<float> wszystkie(pelna ? static_cast<size_t>(szer) * wys + 4096 : 4096, 0.f);
    float punkty[8] = {0};
    float mx = 0, mn = 0, sr = 0;
    int xM = 0, yM = 0, xm = 0, ym = 0;
    p_->fGet(pelna ? 0 : 1, kopia.data(), &mx, &xM, &yM, &mn, &xm, &ym, &sr, punkty, wszystkie.data());
    w.maxC = mx; w.minC = mn; w.srodekC = sr; w.xMax = xM; w.yMax = yM; w.xMin = xm; w.yMin = ym;
    if (p_->fDev) { int a = 0, b = 0; p_->fDev(&w.fpaC, &w.rdzenC, &a, &b); }
    if (pelna) { wszystkie.resize(static_cast<size_t>(szer) * wys); w.wszystkie = std::move(wszystkie); }
    w.ok = mx >= mn && mx > -100 && mx < 2000;
    return w;
}

}  // namespace XthermLib
