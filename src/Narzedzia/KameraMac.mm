// Kompilowane tylko na macOS, z -fobjc-arc (patrz CMakeLists.txt).
#include "KameraMac.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>

#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USB.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

@interface DMOdbiorcaKlatek : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property(nonatomic, copy) void (^odbior)(CMSampleBufferRef);
@end

@implementation DMOdbiorcaKlatek
- (void)captureOutput:(AVCaptureOutput*)output didOutputSampleBuffer:(CMSampleBufferRef)bufor fromConnection:(AVCaptureConnection*)polaczenie {
    void (^b)(CMSampleBufferRef) = self.odbior;
    if (b) b(bufor);
}
@end

namespace {

std::string czteroZnaki(OSType t) {
    char c[5] = {char((t >> 24) & 255), char((t >> 16) & 255), char((t >> 8) & 255), char(t & 255), 0};
    for (int i = 0; i < 4; ++i) if (c[i] < 32 || c[i] > 126) c[i] = '?';
    return c;
}

NSArray<AVCaptureDevice*>* urzadzenia() {
    return [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
}

// Zgoda na dostęp do kamery. Gdy system jeszcze nie pytał, pyta i czekamy na odpowiedź (do 60 s).
bool zgodaNaKamere(std::string& blad) {
    const AVAuthorizationStatus st = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (st == AVAuthorizationStatusAuthorized) return true;
    if (st == AVAuthorizationStatusNotDetermined) {
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);
        __block BOOL przyznano = NO;
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL ok) { przyznano = ok; dispatch_semaphore_signal(sem); }];
        dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 60LL * NSEC_PER_SEC));
        if (przyznano) return true;
    }
    blad = "System nie pozwala programowi na dostęp do kamery. Włącz go w Ustawienia systemowe > Prywatność i ochrona > Kamera "
           "(przy uruchamianiu z terminala zgodę dostaje terminal).";
    return false;
}

}  // namespace

namespace KameraMac {

std::vector<std::string> nazwy() {
    std::vector<std::string> r;
    @autoreleasepool {
        for (AVCaptureDevice* d in urzadzenia()) r.push_back(d.localizedName ? [d.localizedName UTF8String] : "?");
    }
    return r;
}

namespace {
std::string hex(unsigned v) { char b[16]; std::snprintf(b, sizeof b, "0x%X", v); return b; }
}  // namespace

bool wyslijZoomUvc(int indeks, unsigned wartosc, std::string& komunikat) {
    komunikat.clear();
    @autoreleasepool {
        NSArray<AVCaptureDevice*>* lista = urzadzenia();
        if (indeks < 0 || indeks >= static_cast<int>(lista.count)) { komunikat = "Nie ma kamery o takim numerze."; return false; }
        NSString* nazwa = lista[indeks].localizedName;

        io_iterator_t it = 0;
        if (IOServiceGetMatchingServices(0, IOServiceMatching(kIOUSBDeviceClassName), &it) != KERN_SUCCESS) {
            komunikat = "Nie udało się przejrzeć urządzeń USB (IOKit).";
            return false;
        }
        io_service_t usluga = 0, kandydat;
        while ((kandydat = IOIteratorNext(it)) != 0) {
            CFTypeRef prod = IORegistryEntryCreateCFProperty(kandydat, CFSTR("USB Product Name"), kCFAllocatorDefault, 0);
            const bool pasuje = prod && CFGetTypeID(prod) == CFStringGetTypeID() && [nazwa isEqualToString:(__bridge NSString*)prod];
            if (prod) CFRelease(prod);
            if (pasuje) { usluga = kandydat; break; }
            IOObjectRelease(kandydat);
        }
        IOObjectRelease(it);
        if (!usluga) { komunikat = "Nie znaleziono urządzenia USB o nazwie kamery."; return false; }

        IOCFPlugInInterface** wtyczka = nullptr;
        SInt32 wynikWtyczki = 0;
        IOReturn r = IOCreatePlugInInterfaceForService(usluga, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &wtyczka, &wynikWtyczki);
        IOObjectRelease(usluga);
        if (r != kIOReturnSuccess || !wtyczka) { komunikat = "Brak dostępu do urządzenia USB (IOReturn " + hex(r) + ")."; return false; }
        IOUSBDeviceInterface** dev = nullptr;
        const HRESULT hr = (*wtyczka)->QueryInterface(wtyczka, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), reinterpret_cast<LPVOID*>(&dev));
        (*wtyczka)->Release(wtyczka);
        if (hr != S_OK || !dev) { komunikat = "System nie udostępnia interfejsu urządzenia USB."; return false; }

        // identyfikator terminala kamery (wejście typu ITT_CAMERA 0x0201) i numer interfejsu VideoControl z opisu konfiguracji
        int terminal = -1, interfejsVC = -1;
        IOUSBConfigurationDescriptorPtr cfg = nullptr;
        if ((*dev)->GetConfigurationDescriptorPtr(dev, 0, &cfg) == kIOReturnSuccess && cfg) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(cfg);
            const size_t n = USBToHostWord(cfg->wTotalLength);
            bool wVC = false;
            for (size_t i = 0; i + 2 <= n && p[i] >= 2; i += p[i]) {
                if (p[i + 1] == 4 && p[i] >= 9) {
                    wVC = p[i + 5] == 14 && p[i + 6] == 1;
                    if (wVC) interfejsVC = p[i + 2];
                } else if (wVC && p[i + 1] == 0x24 && p[i] >= 8 && p[i + 2] == 0x02 && (p[i + 4] | (p[i + 5] << 8)) == 0x0201) {
                    terminal = p[i + 3];
                }
            }
        }
        if (terminal < 0 || interfejsVC < 0) {
            komunikat = "Nie znaleziono w opisie urządzenia terminala kamery UVC (terminal " + std::to_string(terminal) + ", interfejs " + std::to_string(interfejsVC) + ").";
            (*dev)->Release(dev);
            return false;
        }

        bool otwarte = true;
        IOReturn ro = (*dev)->USBDeviceOpen(dev);
        if (ro != kIOReturnSuccess) { ro = (*dev)->USBDeviceOpenSeize(dev); }
        if (ro != kIOReturnSuccess) otwarte = false;   // spróbujemy mimo to: żądania sterujące bywają dozwolone bez otwarcia

        uint16_t dane = static_cast<uint16_t>(wartosc);
        IOUSBDevRequest zad = {};
        zad.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface);
        zad.bRequest = 0x01;                                  // SET_CUR
        zad.wValue = 0x0B << 8;                               // CT_ZOOM_ABSOLUTE_CONTROL
        zad.wIndex = static_cast<UInt16>((terminal << 8) | interfejsVC);
        zad.wLength = 2;
        zad.pData = &dane;
        const IOReturn rs = (*dev)->DeviceRequest(dev, &zad);

        uint16_t odczyt = 0;
        IOUSBDevRequest zap = {};
        zap.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
        zap.bRequest = 0x81;                                  // GET_CUR
        zap.wValue = 0x0B << 8;
        zap.wIndex = zad.wIndex;
        zap.wLength = 2;
        zap.pData = &odczyt;
        const IOReturn rg = (*dev)->DeviceRequest(dev, &zap);

        if (otwarte) (*dev)->USBDeviceClose(dev);
        (*dev)->Release(dev);
        komunikat = "Terminal kamery " + std::to_string(terminal) + ", interfejs " + std::to_string(interfejsVC) + "; otwarcie urządzenia: " + hex(ro) +
                    "; SET_CUR zoom=" + hex(wartosc) + ": " + hex(rs) + "; GET_CUR: " + hex(rg) + (rg == kIOReturnSuccess ? " wartość " + hex(odczyt) : std::string()) +
                    (rs == kIOReturnSuccess ? "" : " (0xE000... = błąd IOKit, np. 0xE00002C5 brak dostępu)");
        return rs == kIOReturnSuccess;
    }
}

struct Sesja::Impl {
    AVCaptureSession* sesja = nil;
    AVCaptureVideoDataOutput* wyjscie = nil;
    DMOdbiorcaKlatek* odbiorca = nil;
    dispatch_queue_t kolejka = nil;
    std::string opis;
    std::mutex mx;
    Termowizja::KlatkaSurowa ostatnia;
    bool maKlatke = false;
    bool otwarta = false;
};

Sesja::Sesja() : p_(new Impl) {}
Sesja::~Sesja() { zamknij(); }
bool Sesja::otwarta() const { return p_->otwarta; }

std::string Sesja::opis() const { return p_->opis; }

void Sesja::zamknij() {
    @autoreleasepool {
        if (p_->sesja) [p_->sesja stopRunning];
        if (p_->wyjscie) [p_->wyjscie setSampleBufferDelegate:nil queue:nil];
        p_->sesja = nil;
        p_->wyjscie = nil;
        p_->odbiorca = nil;
        p_->kolejka = nil;
    }
    std::lock_guard<std::mutex> blokada(p_->mx);
    p_->otwarta = false;
    p_->maKlatke = false;
}

bool Sesja::otworz(int indeks, std::string& blad) {
    zamknij();
    p_->opis.clear();
    @autoreleasepool {
        if (!zgodaNaKamere(blad)) return false;
        NSArray<AVCaptureDevice*>* lista = urzadzenia();
        if (indeks < 0 || indeks >= static_cast<int>(lista.count)) { blad = "Nie ma kamery o takim numerze."; return false; }
        AVCaptureDevice* urz = lista[indeks];

        std::ostringstream opis;
        opis << "Backend: AVFoundation (bezpośrednio)\nUrządzenie: " << [urz.localizedName UTF8String] << "\nFormaty urządzenia:\n";
        AVCaptureDeviceFormat* wybrany = nil;
        for (AVCaptureDeviceFormat* f in urz.formats) {
            const CMVideoDimensions w = CMVideoFormatDescriptionGetDimensions(f.formatDescription);
            const FourCharCode typ = CMFormatDescriptionGetMediaSubType(f.formatDescription);
            opis << "  " << w.width << "x" << w.height << " " << czteroZnaki(typ) << "\n";
            if (w.width == 256 && w.height == 384 && !wybrany) wybrany = f;   // ramka obraz + dane temperatury
        }
        NSError* err = nil;
        if (wybrany && [urz lockForConfiguration:&err]) {
            urz.activeFormat = wybrany;
            [urz unlockForConfiguration];
        }

        err = nil;
        AVCaptureDeviceInput* wej = [AVCaptureDeviceInput deviceInputWithDevice:urz error:&err];
        if (!wej) {
            blad = std::string("Nie udało się otworzyć kamery: ") + (err ? [err.localizedDescription UTF8String] : "nieznany błąd");
            return false;
        }
        p_->sesja = [[AVCaptureSession alloc] init];
        if (![p_->sesja canAddInput:wej]) { blad = "System nie przyjmuje tej kamery jako wejścia."; p_->sesja = nil; return false; }
        [p_->sesja addInput:wej];

        p_->wyjscie = [[AVCaptureVideoDataOutput alloc] init];
        p_->wyjscie.alwaysDiscardsLateVideoFrames = YES;
        // prosimy o surowy YUYV; gdy go nie ma, bierzemy pierwszy dostępny format piksela
        NSNumber* zadany = @(kCVPixelFormatType_422YpCbCr8_yuvs);
        NSArray<NSNumber*>* dostepne = p_->wyjscie.availableVideoCVPixelFormatTypes;
        opis << "Formaty pikseli dostępne na wyjściu:";
        for (NSNumber* n in dostepne) opis << " " << czteroZnaki(n.unsignedIntValue);
        opis << "\n";
        if (![dostepne containsObject:zadany] && dostepne.count > 0) zadany = dostepne[0];
        p_->wyjscie.videoSettings = @{(id)kCVPixelBufferPixelFormatTypeKey : zadany};
        opis << "Wybrany format pikseli: " << czteroZnaki(zadany.unsignedIntValue) << "\n";

        p_->kolejka = dispatch_queue_create("magmass.kamera", DISPATCH_QUEUE_SERIAL);
        p_->odbiorca = [[DMOdbiorcaKlatek alloc] init];
        Impl* ip = p_.get();
        p_->odbiorca.odbior = ^(CMSampleBufferRef bufor) {
            CVImageBufferRef px = CMSampleBufferGetImageBuffer(bufor);
            if (!px) return;
            CVPixelBufferLockBaseAddress(px, kCVPixelBufferLock_ReadOnly);
            const OSType fmt = CVPixelBufferGetPixelFormatType(px);
            int bajtyNaPiksel = 0;
            if (fmt == kCVPixelFormatType_422YpCbCr8_yuvs || fmt == kCVPixelFormatType_422YpCbCr8) bajtyNaPiksel = 2;
            else if (fmt == kCVPixelFormatType_32BGRA || fmt == kCVPixelFormatType_32ARGB) bajtyNaPiksel = 4;
            if (bajtyNaPiksel > 0 && !CVPixelBufferIsPlanar(px)) {
                const size_t szer = CVPixelBufferGetWidth(px), wys = CVPixelBufferGetHeight(px), wiersz = CVPixelBufferGetBytesPerRow(px);
                const unsigned char* zrodlo = static_cast<const unsigned char*>(CVPixelBufferGetBaseAddress(px));
                Termowizja::KlatkaSurowa k;
                k.szer = static_cast<int>(szer);
                k.wys = static_cast<int>(wys);
                k.kanaly = bajtyNaPiksel;
                k.bajtyNaKanal = 1;
                k.opisTypu = "AVF " + czteroZnaki(fmt);
                const size_t ile = szer * bajtyNaPiksel;
                k.dane.resize(ile * wys);
                for (size_t y = 0; y < wys; ++y) std::memcpy(&k.dane[y * ile], zrodlo + y * wiersz, ile);
                std::lock_guard<std::mutex> blokada(ip->mx);
                ip->ostatnia = std::move(k);
                ip->maKlatke = true;
            }
            CVPixelBufferUnlockBaseAddress(px, kCVPixelBufferLock_ReadOnly);
        };
        [p_->wyjscie setSampleBufferDelegate:p_->odbiorca queue:p_->kolejka];
        if (![p_->sesja canAddOutput:p_->wyjscie]) { blad = "System nie przyjmuje wyjścia wideo dla tej kamery."; zamknij(); return false; }
        [p_->sesja addOutput:p_->wyjscie];
        [p_->sesja startRunning];
        p_->opis = opis.str();
        {
            std::lock_guard<std::mutex> blokada(p_->mx);
            p_->otwarta = true;
        }
    }
    return true;
}

bool Sesja::pobierz(Termowizja::KlatkaSurowa& k) {
    std::lock_guard<std::mutex> blokada(p_->mx);
    if (!p_->otwarta || !p_->maKlatke) return false;
    k = p_->ostatnia;
    return true;
}

}  // namespace KameraMac
