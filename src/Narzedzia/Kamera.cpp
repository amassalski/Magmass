#include "Kamera.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <cstdlib>

#ifdef DM_KAMERA_AVF
#include "KameraMac.h"
#endif

#ifdef DM_KAMERA
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#endif

namespace {
bool kameraTestowa() {
    const char* v = std::getenv("DM_KAMERA_TEST");
    return v && *v && std::string(v) != "0";
}
} // namespace

struct Kamera::Impl {
    bool testowa = false;
    bool otwarta = false;
    long licznik = 0;
#ifdef DM_KAMERA_AVF
    KameraMac::Sesja mac;
    bool uzywaMac = false;
    int indeksMac = -1;
#endif
#ifdef DM_KAMERA
    cv::VideoCapture cap;
#endif
};

Kamera::Kamera() : p_(new Impl) {}
Kamera::~Kamera() { zamknij(); }

bool Kamera::obslugiwana() {
#if defined(DM_KAMERA) || defined(DM_KAMERA_AVF)
    return true;
#else
    return kameraTestowa();
#endif
}

namespace { std::string g_raportWykrywania; }

std::string Kamera::raportWykrywania() { return g_raportWykrywania; }

std::vector<InfoKamery> Kamera::wykryjSurowe() {
#ifdef DM_KAMERA_AVF
    std::vector<InfoKamery> wynik;
    if (kameraTestowa()) return wykryj();
    g_raportWykrywania = "AVFoundation:\n";
    const auto n = KameraMac::nazwy();
    for (size_t i = 0; i < n.size(); ++i) {
        wynik.push_back({static_cast<int>(i), n[i]});
        g_raportWykrywania += "indeks " + std::to_string(i) + ": " + n[i] + "\n";
    }
    if (n.empty()) g_raportWykrywania += "(brak kamer)\n";
    return wynik;
#else
    return wykryj();
#endif
}

std::vector<InfoKamery> Kamera::wykryj(int maks) {
    std::vector<InfoKamery> wynik;
    g_raportWykrywania.clear();
    if (kameraTestowa()) {
        wynik.push_back({0, "Kamera testowa"});
        return wynik;
    }
#ifdef DM_KAMERA
    for (int i = 0; i < maks; ++i) {
        cv::VideoCapture c(i);
        if (c.isOpened()) {
            wynik.push_back({i, "Kamera " + std::to_string(i + 1) + " (" + std::to_string(static_cast<int>(c.get(cv::CAP_PROP_FRAME_WIDTH))) + "x" +
                                std::to_string(static_cast<int>(c.get(cv::CAP_PROP_FRAME_HEIGHT))) + ")"});
            g_raportWykrywania += "indeks " + std::to_string(i) + ": otwarta, " + wynik.back().nazwa + "\n";
        } else {
            // druga próba: bez konwersji do RGB (surowy format, np. YUYV 256x384 z kamer termowizyjnych)
            cv::VideoCapture c2(i, cv::CAP_ANY, {cv::CAP_PROP_CONVERT_RGB, 0});
            if (c2.isOpened()) {
                wynik.push_back({i, "Kamera " + std::to_string(i + 1) + " (surowa, " + std::to_string(static_cast<int>(c2.get(cv::CAP_PROP_FRAME_WIDTH))) + "x" +
                                    std::to_string(static_cast<int>(c2.get(cv::CAP_PROP_FRAME_HEIGHT))) + ")"});
                g_raportWykrywania += "indeks " + std::to_string(i) + ": otwarta dopiero bez konwersji RGB, " + wynik.back().nazwa + "\n";
            } else {
                g_raportWykrywania += "indeks " + std::to_string(i) + ": nie da się otworzyć\n";
            }
        }
    }
#else
    (void)maks;
#endif
    return wynik;
}

bool Kamera::otworz(int indeks, int szer, int wys) {
    zamknij();
    blad_.clear();
    if (kameraTestowa()) {
        p_->testowa = true;
        p_->otwarta = true;
        return true;
    }
#ifdef DM_KAMERA
    p_->cap.open(indeks);
    if (!p_->cap.isOpened()) p_->cap.open(indeks, cv::CAP_ANY, {cv::CAP_PROP_CONVERT_RGB, 0});
    if (!p_->cap.isOpened()) {
        blad_ = "Nie udało się otworzyć kamery. Sprawdź, czy inny program jej nie używa i czy system pozwala "
                "na dostęp do kamery.";
        return false;
    }
    p_->cap.set(cv::CAP_PROP_FRAME_WIDTH, szer);
    p_->cap.set(cv::CAP_PROP_FRAME_HEIGHT, wys);
    p_->otwarta = true;
    return true;
#else
    (void)indeks; (void)szer; (void)wys;
    blad_ = "Program zbudowano bez obsługi kamery (OpenCV).";
    return false;
#endif
}

void Kamera::zamknij() {
    if (!p_) return;
#ifdef DM_KAMERA_AVF
    p_->mac.zamknij();
    p_->uzywaMac = false;
#endif
#ifdef DM_KAMERA
    if (p_->cap.isOpened()) p_->cap.release();
#endif
    p_->otwarta = false;
    p_->testowa = false;
}

bool Kamera::otwarta() const { return p_->otwarta; }

bool Kamera::pobierz(KlatkaRGB& k) {
    if (!p_->otwarta) return false;
    if (p_->testowa) {
        // ruchomy gradient z ramką: widać, że podgląd żyje
        k.szer = 320; k.wys = 240;
        k.rgb.assign(static_cast<size_t>(k.szer) * k.wys * 3, 0);
        const long t = p_->licznik++;
        for (int y = 0; y < k.wys; ++y)
            for (int x = 0; x < k.szer; ++x) {
                unsigned char* px = &k.rgb[(static_cast<size_t>(y) * k.szer + x) * 3];
                px[0] = static_cast<unsigned char>((x + t * 3) % 256);
                px[1] = static_cast<unsigned char>(y * 255 / k.wys);
                px[2] = static_cast<unsigned char>(128 + 100 * std::sin((x + y + t * 4) / 25.0));
                if (x < 4 || y < 4 || x >= k.szer - 4 || y >= k.wys - 4) px[0] = px[1] = px[2] = 255;
            }
        return true;
    }
#ifdef DM_KAMERA
    cv::Mat m;
    if (!p_->cap.read(m) || m.empty() || m.type() != CV_8UC3) {
        if (!m.empty() && m.channels() == 1) {
            // czarno-biała kamera: powiel kanał
            k.szer = m.cols; k.wys = m.rows;
            k.rgb.resize(static_cast<size_t>(k.szer) * k.wys * 3);
            for (int y = 0; y < m.rows; ++y)
                for (int x = 0; x < m.cols; ++x)
                    for (int c = 0; c < 3; ++c) k.rgb[(static_cast<size_t>(y) * m.cols + x) * 3 + c] = m.at<uchar>(y, x);
            return true;
        }
        return false;
    }
    k.szer = m.cols; k.wys = m.rows;
    k.rgb.resize(static_cast<size_t>(k.szer) * k.wys * 3);
    for (int y = 0; y < m.rows; ++y) {
        const unsigned char* src = m.ptr<unsigned char>(y);
        unsigned char* dst = &k.rgb[static_cast<size_t>(y) * m.cols * 3];
        for (int x = 0; x < m.cols; ++x) {      // OpenCV daje BGR
            dst[x * 3] = src[x * 3 + 2];
            dst[x * 3 + 1] = src[x * 3 + 1];
            dst[x * 3 + 2] = src[x * 3];
        }
    }
    return true;
#else
    return false;
#endif
}

bool Kamera::otworzSurowo(int indeks) {
    zamknij();
    blad_.clear();
    if (kameraTestowa()) {
        p_->testowa = true;
        p_->otwarta = true;
        return true;
    }
#ifdef DM_KAMERA_AVF
    // macOS: indeks z wykryjSurowe() (lista AVFoundation), klatki bez konwersji
    if (!p_->mac.otworz(indeks, blad_)) return false;
    p_->uzywaMac = true;
    p_->indeksMac = indeks;
    p_->otwarta = true;
    return true;
#endif
#ifdef DM_KAMERA
    p_->cap.open(indeks);
    if (!p_->cap.isOpened()) p_->cap.open(indeks, cv::CAP_ANY, {cv::CAP_PROP_CONVERT_RGB, 0});
    if (!p_->cap.isOpened()) {
        blad_ = "Nie udało się otworzyć kamery. Sprawdź, czy inny program jej nie używa i czy system pozwala "
                "na dostęp do kamery.";
        return false;
    }
    p_->cap.set(cv::CAP_PROP_CONVERT_RGB, 0);
    p_->otwarta = true;
    return true;
#else
    (void)indeks;
    blad_ = "Program zbudowano bez obsługi kamery (OpenCV).";
    return false;
#endif
}

bool Kamera::pobierzSurowa(Termowizja::KlatkaSurowa& k) {
    if (!p_->otwarta) return false;
    if (p_->testowa) {
        const char* v = std::getenv("DM_KAMERA_TEST");
        k = (v && std::string(v) == "2") ? Termowizja::klatkaTestowaSurowa(p_->licznik++) : Termowizja::klatkaTestowa(p_->licznik++);
        return true;
    }
#ifdef DM_KAMERA_AVF
    if (p_->uzywaMac) return p_->mac.pobierz(k);
#endif
#ifdef DM_KAMERA
    cv::Mat m;
    if (!p_->cap.read(m) || m.empty()) return false;
    const int glebia = m.depth();
    if (glebia != CV_8U && glebia != CV_16U) { blad_ = "Nieobsługiwana głębia klatki (typ " + std::to_string(m.type()) + ")."; return false; }
    k.szer = m.cols;
    k.wys = m.rows;
    k.kanaly = m.channels();
    k.bajtyNaKanal = glebia == CV_16U ? 2 : 1;
    k.opisTypu = std::string(glebia == CV_16U ? "CV_16UC" : "CV_8UC") + std::to_string(k.kanaly);
    const size_t wiersz = k.bajtyWiersza();
    k.dane.resize(wiersz * k.wys);
    for (int y = 0; y < m.rows; ++y) std::copy(m.ptr<unsigned char>(y), m.ptr<unsigned char>(y) + wiersz, &k.dane[y * wiersz]);
    return true;
#else
    return false;
#endif
}

bool Kamera::wyslijZoom(unsigned wartosc, std::string& komunikat) {
    komunikat.clear();
    if (!p_->otwarta) { komunikat = "Kamera nie jest otwarta."; return false; }
    if (p_->testowa) { komunikat = "Kamera testowa: polecenie pominięte."; return true; }
#ifdef DM_KAMERA_AVF
    if (p_->uzywaMac) return KameraMac::wyslijZoomUvc(p_->indeksMac, wartosc, komunikat);
#endif
#ifdef DM_KAMERA
    const bool ok = p_->cap.set(cv::CAP_PROP_ZOOM, static_cast<double>(wartosc));
    komunikat = std::string("CAP_PROP_ZOOM = ") + std::to_string(wartosc) + (ok ? ": przyjęte przez sterownik" : ": sterownik odmówił");
    return ok;
#else
    komunikat = "Ta wersja programu nie potrafi wysłać polecenia do kamery.";
    return false;
#endif
}

std::string Kamera::wlasciwosci() const {
    std::ostringstream s;
    if (!p_->otwarta) return "Kamera nie jest otwarta.\n";
    if (p_->testowa) return "Kamera testowa (DM_KAMERA_TEST).\n";
#ifdef DM_KAMERA_AVF
    if (p_->uzywaMac) return p_->mac.opis();
#endif
#ifdef DM_KAMERA
    s << "Backend: " << p_->cap.getBackendName() << "\n";
    s << "Rozmiar zgłoszony: " << p_->cap.get(cv::CAP_PROP_FRAME_WIDTH) << " x " << p_->cap.get(cv::CAP_PROP_FRAME_HEIGHT)
      << ", FPS: " << p_->cap.get(cv::CAP_PROP_FPS) << "\n";
    const int f = static_cast<int>(p_->cap.get(cv::CAP_PROP_FOURCC));
    char cc[5] = {char(f & 255), char((f >> 8) & 255), char((f >> 16) & 255), char((f >> 24) & 255), 0};
    for (int i = 0; i < 4; ++i) if (cc[i] < 32 || cc[i] > 126) cc[i] = '?';
    s << "FOURCC: " << cc << ", konwersja do RGB: " << p_->cap.get(cv::CAP_PROP_CONVERT_RGB) << "\n";
#endif
    return s.str();
}
