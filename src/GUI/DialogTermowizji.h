#ifndef DIALOGTERMOWIZJI_H
#define DIALOGTERMOWIZJI_H

#include "../Narzedzia/Kamera.h"
#include "../Narzedzia/XthermDll.h"
#include "../Logika/Termowizja.h"

#include <wx/wx.h>
#include <wx/timer.h>

#include <vector>

/**
 * Kamera termowizyjna USB (InfiRay T3 Pro i podobne): podgląd na żywo z paletą barw, znacznikami najzimniejszego
 * i najgorętszego punktu, temperaturą środka kadru oraz diagnostyką tego, co kamera przesyła po USB.
 * Temperatury wymagają założenia o układzie danych (patrz Logika/Termowizja.h): dzielnik i przesunięcie są
 * parametrami do sprawdzenia pomiarem. "Zapisz próbkę" zapisuje surową klatkę i raport do dalszej analizy.
 */
class DialogTermowizji : public wxDialog {
public:
    explicit DialogTermowizji(wxWindow* parent);
    ~DialogTermowizji() override;

    // do testów
    bool Otwarta() const { return kamera_.otwarta(); }
    int LiczbaKlatek() const { return klatek_; }
    void Odswiez() { OnTimer(); }
    void Zamroz(bool z);
    bool Zamrozony() const { return zamrozony_; }
    void UstawPalete(int nr);
    void UstawParametry(const wxString& dzielnik, const wxString& offsetK);
    void UstawUklad(int nr);                       ///< 0 = automatycznie, 1 = obraz i dane, 2 = tylko obraz
    /// Wysyła polecenie UVC zoom (wartość szesnastkowo, np. "8004") do otwartej kamery; zwraca komunikat
    wxString WyslijZoom(const wxString& hex);
    /// Kalibracja użytkownika (tryb surowy): zapamiętuje odczyt ze środka obrazu (średnia 9x9) jako podaną temperaturę [°C]
    bool DodajPunktKalibracji(const wxString& temperaturaC);
    void WyczyscKalibracje();
    size_t LiczbaPunktowKalibracji() const { return kal_.liczba(); }
    wxString Raport() const { return raport_->GetValue(); }
    const Termowizja::Statystyka& Stat() const { return stat_; }
    Termowizja::Uklad UkladAktualny() const { return uklad_.uklad; }
    wxString ZapiszProbke(const wxString& katalog);   ///< zapisuje .bin i .txt; zwraca ścieżkę pliku .bin albo pusty
    bool ZapiszObraz(const wxString& plikPng);

private:
    class Podglad;
    Kamera kamera_;
    XthermLib::Biblioteka xtherm_;   ///< opcjonalna biblioteka producenta (z kopii użytkownika)
    bool xthermOdswiez_ = true;
    XthermLib::Wynik xthermOst_;     ///< ostatni wynik biblioteki (do zapisu próbki kalibracyjnej)
    wxString xthermStan_;
    wxTimer timer_;
    wxChoice *chKamera = nullptr, *chPaleta = nullptr, *chUklad = nullptr;
    wxTextCtrl *tcDzielnik = nullptr, *tcOffset = nullptr, *raport_ = nullptr;
    wxButton* bZamroz = nullptr;
    wxTextCtrl *tcZoom = nullptr, *tcTRef = nullptr;
    Termowizja::KalibracjaUzytkownika kal_;
    wxString komunikatPolecenia_;
    wxStaticText* lbWynik = nullptr;
    Podglad* podglad_ = nullptr;
    std::vector<InfoKamery> kamery_;
    Termowizja::KlatkaSurowa klatka_;
    Termowizja::RozpoznanyUklad uklad_;
    Termowizja::Statystyka stat_;
    bool zamrozony_ = false;
    int klatek_ = 0;

    void WybierzXthermDll();         ///< ładuje XthermDll.dll wskazany przez użytkownika (liczy °C z surowych klatek)
    void DiagnostykaSdk();           ///< ładuje USBSDK.dll wskazany przez użytkownika, wypisuje urządzenia i parametry do raportu
    void WykryjKamery();
    void OtworzWybrana();
    void OnTimer();
    void Przelicz();                 ///< układ, statystyki, obraz i raport dla bieżącej klatki
    double Dzielnik() const;
    double OffsetK() const;
};

#endif  // DIALOGTERMOWIZJI_H
