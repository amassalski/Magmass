#include "KlientHTTP.h"

#include <wx/app.h>
#include <wx/convauto.h>
#include <wx/event.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/fontmap.h>
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/webrequest.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <thread>

// Dwie implementacje transportu: wxWebRequest (gdy wxWidgets zbudowano z wxUSE_WEBREQUEST, np. pakiety Ubuntu, MSYS2, macOS)
// albo libcurl (gdy wxWidgets jest bez WebRequest, np. pakiety Devuan/Debian: wtedy zainstaluj libcurl4-openssl-dev).
#if !wxUSE_WEBREQUEST || defined(DM_WYMUS_CURL)
#ifdef DM_UZYJ_CURL
#include <curl/curl.h>
#else
#error "wxWidgets bez wxWebRequest: zainstaluj libcurl4-openssl-dev i uruchom cmake ponownie (program użyje wtedy libcurl)."
#endif
#endif

namespace {

std::string malymi(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/// Nazwa kodowania z nagłówka Content-Type albo z <meta> w początku strony
std::string kodowanieStrony(const std::string& tresc, const std::string& typ) {
    auto zNapisu = [](const std::string& tekst) -> std::string {
        const std::string m = malymi(tekst);
        size_t p = m.find("charset=");
        if (p == std::string::npos) return std::string();
        p += 8;
        while (p < m.size() && (m[p] == '"' || m[p] == '\'' || m[p] == ' ')) ++p;
        size_t k = p;
        while (k < m.size() && (std::isalnum(static_cast<unsigned char>(m[k])) || m[k] == '-' || m[k] == '_')) ++k;
        return m.substr(p, k - p);
    };
    std::string k = zNapisu(typ);
    if (k.empty()) k = zNapisu(tresc.substr(0, 2048));
    return k;
}

#if wxUSE_WEBREQUEST && !defined(DM_WYMUS_CURL)
class Zadanie : public wxEvtHandler {
public:
    Zadanie(bool plik, const wxString& sciezka, std::string wymagany, size_t maks, KlientHTTP::Po po, int limitMs)
        : plik_(plik), sciezka_(sciezka), wymagany_(std::move(wymagany)), maks_(maks), po_(std::move(po)), limit_(limitMs),
          zegar_(this) {
        Bind(wxEVT_WEBREQUEST_STATE, &Zadanie::NaStan, this);
        Bind(wxEVT_WEBREQUEST_DATA, &Zadanie::NaDane, this);
        Bind(wxEVT_TIMER, &Zadanie::NaCzas, this);
    }

    void UstawCialo(std::string json) { cialo_ = std::move(json); }

    void Start(const std::string& url) {
        wynik_.adres = url;
        wxWebRequest zad = wxWebSession::GetDefault().CreateRequest(this, wxString::FromUTF8(url));
        if (!zad.IsOk()) { Zakoncz("Nie udało się utworzyć zapytania HTTP."); return; }
        zad_ = zad;
        zad_.SetHeader("User-Agent", KlientHTTP::agent());
        zad_.SetHeader("Accept-Language", "pl,en;q=0.8");
        zad_.SetHeader("Accept", plik_ ? "application/pdf,*/*;q=0.8" : "text/html,application/xhtml+xml,*/*;q=0.8");
        if (!cialo_.empty()) {
            zad_.SetMethod("POST");
            zad_.SetHeader("Accept", "application/json");
            zad_.SetData(wxString::FromUTF8(cialo_), "application/json; charset=utf-8");
        }
        zad_.SetStorage(wxWebRequest::Storage_None);
        if (plik_) {
            czesciowy_ = sciezka_ + ".part";
            bool utworzono;
            { wxLogNull cicho; utworzono = plikWyj_.Create(czesciowy_, true); }
            if (!utworzono) { Zakoncz("Nie można zapisać pliku: " + czesciowy_.utf8_string()); return; }
        }
        zegar_.StartOnce(limit_);
        zad_.Start();
    }

private:
    bool plik_;
    wxString sciezka_, czesciowy_;
    std::string wymagany_, cialo_;
    size_t maks_;
    KlientHTTP::Po po_;
    int limit_;
    wxTimer zegar_;
    wxWebRequest zad_;
    wxFile plikWyj_;
    WynikHTTP wynik_;
    size_t odebrano_ = 0;
    bool koniec_ = false, przekroczony_ = false, czas_ = false;
    std::string poczatek_;

    void NaDane(wxWebRequestEvent& e) {
        if (koniec_ || przekroczony_) return;
        const size_t n = e.GetDataSize();
        odebrano_ += n;
        if (odebrano_ > maks_) {
            przekroczony_ = true;
            zad_.Cancel();
            return;
        }
        const char* b = static_cast<const char*>(e.GetDataBuffer());
        if (poczatek_.size() < 16) poczatek_.append(b, std::min<size_t>(n, 16 - poczatek_.size()));
        if (plik_) {
            if (!plikWyj_.Write(b, n)) { przekroczony_ = true; zad_.Cancel(); }
        } else {
            wynik_.tresc.append(b, n);
        }
    }

    void NaCzas(wxTimerEvent&) {
        if (koniec_) return;
        czas_ = true;
        zad_.Cancel();
    }

    void NaStan(wxWebRequestEvent& e) {
        if (koniec_) return;
        const wxWebResponse odp = zad_.GetResponse();
        if (odp.IsOk()) {
            wynik_.kod = odp.GetStatus();
            wynik_.typ = odp.GetHeader("Content-Type").utf8_string();
            const std::string u = odp.GetURL().utf8_string();
            if (!u.empty()) wynik_.adres = u;
        }
        const auto stan = e.GetState();
        if (przekroczony_ && (stan == wxWebRequest::State_Completed || stan == wxWebRequest::State_Failed ||
                              stan == wxWebRequest::State_Cancelled)) {
            Zakoncz(plik_ && !plikWyj_.IsOpened() ? "Nie można zapisać pliku." : "Odpowiedź jest zbyt duża.");
            return;
        }
        switch (stan) {
            case wxWebRequest::State_Completed:
                if (wynik_.kod >= 200 && wynik_.kod < 300) Sukces();
                else Zakoncz("Serwer odpowiedział błędem " + std::to_string(wynik_.kod) + ".");
                break;
            case wxWebRequest::State_Unauthorized:
                Zakoncz("Strona wymaga logowania.");
                break;
            case wxWebRequest::State_Cancelled:
                if (czas_) Zakoncz("Serwer nie odpowiedział w wyznaczonym czasie.");
                else if (przekroczony_) Zakoncz(plik_ && !plikWyj_.IsOpened() ? "Nie można zapisać pliku." : "Odpowiedź jest zbyt duża.");
                else Zakoncz("Przerwano.");
                break;
            case wxWebRequest::State_Failed:
                Zakoncz(std::string("Błąd połączenia: ") + e.GetErrorDescription().utf8_string());
                break;
            default: break;
        }
    }

    void Sukces() {
        if (plik_) {
            plikWyj_.Close();
            if (!wymagany_.empty() && poczatek_.compare(0, wymagany_.size(), wymagany_) != 0) {
                Zakoncz("Pobrany plik nie jest oczekiwanego typu (to nie " + wymagany_ + ").");
                return;
            }
            if (odebrano_ == 0) { Zakoncz("Pobrany plik jest pusty."); return; }
            if (wxFileExists(sciezka_)) wxRemoveFile(sciezka_);
            if (!wxRenameFile(czesciowy_, sciezka_)) { Zakoncz("Nie można zapisać pliku " + sciezka_.utf8_string()); return; }
        } else {
            wynik_.tresc = KlientHTTP::naUTF8(wynik_.tresc, wynik_.typ);
        }
        wynik_.ok = true;
        Zakoncz("");
    }

    void Zakoncz(const std::string& blad) {
        if (koniec_) return;
        koniec_ = true;
        zegar_.Stop();
        if (plik_) {
            if (plikWyj_.IsOpened()) plikWyj_.Close();
            if (!wynik_.ok && wxFileExists(czesciowy_)) wxRemoveFile(czesciowy_);
        }
        wynik_.blad = blad;
        KlientHTTP::Po po = std::move(po_);
        WynikHTTP w = wynik_;
        // usunięcie obiektu dopiero po wyjściu z obsługi zdarzenia
        CallAfter([this, po, w]() {
            if (po) po(w);
            delete this;
        });
    }
};

#else   // libcurl w wątku roboczym; wynik wraca do wątku okna przez CallAfter
class Zadanie {
public:
    Zadanie(bool plik, const wxString& sciezka, std::string wymagany, size_t maks, KlientHTTP::Po po, int limitMs)
        : plik_(plik), sciezka_(sciezka), wymagany_(std::move(wymagany)), maks_(maks), po_(std::move(po)), limit_(limitMs) {}

    void UstawCialo(std::string json) { cialo_ = std::move(json); }

    void Start(const std::string& url) {
        wynik_.adres = url;
        if (plik_) {
            czesciowy_ = sciezka_ + ".part";
            bool utworzono;
            { wxLogNull cicho; utworzono = plikWyj_.Create(czesciowy_, true); }
            if (!utworzono) { Koniec("Nie można zapisać pliku: " + czesciowy_.utf8_string()); return; }
        }
        std::thread([this, url]() { Wykonaj(url); }).detach();
    }

private:
    bool plik_;
    wxString sciezka_, czesciowy_;
    std::string wymagany_, cialo_;
    size_t maks_;
    KlientHTTP::Po po_;
    int limit_;
    wxFile plikWyj_;
    WynikHTTP wynik_;
    size_t odebrano_ = 0;
    bool przekroczony_ = false, bladZapisu_ = false;
    std::string poczatek_, blad_;

    static size_t Zapis(char* b, size_t rozm, size_t n, void* ctx) {
        Zadanie* z = static_cast<Zadanie*>(ctx);
        const size_t ile = rozm * n;
        z->odebrano_ += ile;
        if (z->odebrano_ > z->maks_) { z->przekroczony_ = true; return 0; }
        if (z->poczatek_.size() < 16) z->poczatek_.append(b, std::min<size_t>(ile, 16 - z->poczatek_.size()));
        if (z->plik_) {
            if (!z->plikWyj_.Write(b, ile)) { z->bladZapisu_ = true; return 0; }
        } else {
            z->wynik_.tresc.append(b, ile);
        }
        return ile;
    }

    void Wykonaj(const std::string& url) {   // wątek roboczy: bez dotykania GUI
        CURL* c = curl_easy_init();
        if (!c) { Koniec("Nie udało się utworzyć zapytania HTTP."); return; }
        char blad[CURL_ERROR_SIZE] = {0};
        curl_slist* nag = nullptr;
        const std::string ua = KlientHTTP::agent();
        nag = curl_slist_append(nag, "Accept-Language: pl,en;q=0.8");
        nag = curl_slist_append(nag, cialo_.empty() ? (plik_ ? "Accept: application/pdf,*/*;q=0.8" : "Accept: text/html,application/xhtml+xml,*/*;q=0.8")
                                                    : "Accept: application/json");
        if (!cialo_.empty()) nag = curl_slist_append(nag, "Content-Type: application/json; charset=utf-8");
        curl_easy_setopt(c, CURLOPT_URL, url.c_str());
        curl_easy_setopt(c, CURLOPT_USERAGENT, ua.c_str());
        curl_easy_setopt(c, CURLOPT_HTTPHEADER, nag);
        curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(c, CURLOPT_MAXREDIRS, 10L);
        curl_easy_setopt(c, CURLOPT_PROTOCOLS_STR, "http,https");
        curl_easy_setopt(c, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(c, CURLOPT_TIMEOUT_MS, static_cast<long>(limit_));
        curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(c, CURLOPT_ERRORBUFFER, blad);
        curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, &Zadanie::Zapis);
        curl_easy_setopt(c, CURLOPT_WRITEDATA, this);
        if (!cialo_.empty()) {
            curl_easy_setopt(c, CURLOPT_POST, 1L);
            curl_easy_setopt(c, CURLOPT_POSTFIELDS, cialo_.c_str());
            curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, static_cast<long>(cialo_.size()));
        }
        const CURLcode r = curl_easy_perform(c);
        long kod = 0; char* typ = nullptr; char* ostatni = nullptr;
        curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &kod);
        curl_easy_getinfo(c, CURLINFO_CONTENT_TYPE, &typ);
        curl_easy_getinfo(c, CURLINFO_EFFECTIVE_URL, &ostatni);
        wynik_.kod = static_cast<int>(kod);
        if (typ) wynik_.typ = typ;
        if (ostatni && *ostatni) wynik_.adres = ostatni;
        const std::string opis = blad[0] ? blad : curl_easy_strerror(r);
        curl_slist_free_all(nag);
        curl_easy_cleanup(c);
        if (przekroczony_ || bladZapisu_) Koniec(bladZapisu_ ? "Nie można zapisać pliku." : "Odpowiedź jest zbyt duża.");
        else if (r == CURLE_OPERATION_TIMEDOUT) Koniec("Serwer nie odpowiedział w wyznaczonym czasie.");
        else if (r != CURLE_OK) Koniec("Błąd połączenia: " + opis);
        else if (wynik_.kod < 200 || wynik_.kod >= 300) Koniec(wynik_.kod == 401 ? "Strona wymaga logowania." : "Serwer odpowiedział błędem " + std::to_string(wynik_.kod) + ".");
        else Koniec("");
    }

    void Koniec(const std::string& blad) {   // dowolny wątek: dokończenie w wątku okna
        blad_ = blad;
        wxTheApp->CallAfter([this]() { Zakoncz(blad_); });
    }

    void Zakoncz(std::string blad) {   // wątek okna
        if (plik_ && plikWyj_.IsOpened()) plikWyj_.Close();
        if (blad.empty()) {
            if (plik_) {
                if (!wymagany_.empty() && poczatek_.compare(0, wymagany_.size(), wymagany_) != 0)
                    blad = "Pobrany plik nie jest oczekiwanego typu (to nie " + wymagany_ + ").";
                else if (odebrano_ == 0) blad = "Pobrany plik jest pusty.";
                else {
                    if (wxFileExists(sciezka_)) wxRemoveFile(sciezka_);
                    if (!wxRenameFile(czesciowy_, sciezka_)) blad = "Nie można zapisać pliku " + sciezka_.utf8_string();
                }
            } else {
                wynik_.tresc = KlientHTTP::naUTF8(wynik_.tresc, wynik_.typ);
            }
        }
        wynik_.ok = blad.empty();
        if (plik_ && !wynik_.ok && wxFileExists(czesciowy_)) wxRemoveFile(czesciowy_);
        wynik_.blad = blad;
        KlientHTTP::Po po = std::move(po_);
        const WynikHTTP w = wynik_;
        if (po) po(w);
        delete this;
    }
};
#endif

} // namespace

// tylko ASCII: z polskimi znakami libcurl gubi cały nagłówek
std::string KlientHTTP::agent() { return "Magmass/1.0 (personal inventory program; price check)"; }

bool KlientHTTP::adresDozwolony(const std::string& url) {
    const std::string m = malymi(url.substr(0, 8));
    return (m.rfind("http://", 0) == 0 && url.size() > 7) || (m.rfind("https://", 0) == 0 && url.size() > 8);
}

std::string KlientHTTP::naUTF8(const std::string& tresc, const std::string& typ) {
    std::string k = kodowanieStrony(tresc, typ);
    if (k.empty() || k == "utf-8" || k == "utf8" || k == "us-ascii") return tresc;
    wxCSConv konw(wxString::FromAscii(k.c_str()));
    if (!konw.IsOk()) return tresc;
    const wxString w(tresc.c_str(), konw, tresc.size());
    if (w.empty() && !tresc.empty()) return tresc;
    return w.utf8_string();
}

void KlientHTTP::pobierzStrone(const std::string& url, Po po, int limitCzasuMs) {
    if (!adresDozwolony(url)) {
        WynikHTTP w; w.adres = url; w.blad = "Adres musi zaczynać się od http:// albo https://.";
        wxTheApp->CallAfter([po, w]() { po(w); });
        return;
    }
    (new Zadanie(false, wxString(), "", MAKS_STRONA, std::move(po), limitCzasuMs))->Start(url);
}

void KlientHTTP::wyslijJSON(const std::string& url, const std::string& json, Po po, int limitCzasuMs) {
    if (!adresDozwolony(url)) {
        WynikHTTP w; w.adres = url; w.blad = "Adres musi zaczynać się od http:// albo https://.";
        wxTheApp->CallAfter([po, w]() { po(w); });
        return;
    }
    auto* z = new Zadanie(false, wxString(), "", MAKS_STRONA, std::move(po), limitCzasuMs);
    z->UstawCialo(json);
    z->Start(url);
}

void KlientHTTP::pobierzPlik(const std::string& url, const wxString& sciezka, Po po, int limitCzasuMs,
                             const std::string& wymaganyPoczatek) {
    if (!adresDozwolony(url)) {
        WynikHTTP w; w.adres = url; w.blad = "Adres musi zaczynać się od http:// albo https://.";
        wxTheApp->CallAfter([po, w]() { po(w); });
        return;
    }
    (new Zadanie(true, sciezka, wymaganyPoczatek, MAKS_PLIK, std::move(po), limitCzasuMs))->Start(url);
}
