// Test dymny GUI: tworzy okno główne i dialogi, sprawdza, że wxWidgets nie
// zgłasza asercji (np. błędnych rodziców kontrolek), że przyciski robią to, co
// trzeba, i zapisuje zrzuty ekranu do katalogu z DM_TEST_SHOTS.
//
// Uruchamianie wymaga ekranu (na serwerze: xvfb-run) oraz PostgreSQL:
//   DM_TEST_HOST, DM_TEST_PORT, DM_TEST_DB, DM_TEST_USER, DM_TEST_PASSWORD
//   DM_TEST_SQL  - katalog ze skryptami SQL
//   DM_TEST_SHOTS - katalog na zrzuty (opcjonalnie)

#include <wx/log.h>
#include <wx/wx.h>
#include <wx/dcscreen.h>
#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/evtloop.h>
#include <wx/timer.h>

#include <cstdlib>
#include <functional>
#include <vector>
#include <map>
#include <wx/spinctrl.h>
#include <fstream>
#include <iostream>

#include "BazaDanych/ManagerBazyDanych.h"
#include "GUI/DialogElementu.h"
#include "Logika/LogikaProjektow.h"
#include "GUI/DialogEtykiet.h"
#include "GUI/DialogZebra.h"
#include "GUI/DialogAudioLampy.h"
#include "GUI/DialogMetodyGraficzne.h"
#include "GUI/DialogZnieksztalcen.h"
#include "GUI/DialogDlawika.h"
#include "GUI/DialogTrafoSieciowy.h"
#include "GUI/DialogTrafoPP.h"
#include "GUI/DialogTrafoSE.h"
#include "GUI/DialogTrafoSEWykonane.h"
#include "GUI/DialogCewkiAL.h"
#include "GUI/DialogTrafoPPEtap3.h"
#include "Narzedzia/UstawieniaZebry.h"
#include "Narzedzia/WysylkaZPL.h"
#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#endif
#include "GUI/DialogImportuElementow.h"
#include "GUI/DialogKodow.h"
#include "GUI/DialogKalkulatorow.h"
#include "GUI/PanelSchematu.h"
#include "GUI/DialogKamery.h"
#include "GUI/DialogTermowizji.h"
#include "GUI/PanelPodgladu.h"
#include "GUI/DialogImportBOM.h"
#include "GUI/DialogRaportu.h"
#include "GUI/DialogSklepow.h"
#include "GUI/DialogObudow.h"
#include "GUI/DialogSlownikaParametrow.h"
#include "Logika/LogikaObudow.h"
#include "GUI/DialogDodajSklep.h"
#include "GUI/DialogLinkowElementu.h"
#include "GUI/DialogZestawienia.h"
#include "GUI/OdczytKoduZObrazu.h"
#include "GUI/DialogProjektu.h"
#include "GUI/PanelProjektow.h"
#include "GUI/PanelKatalogu.h"
#include "GUI/SortowanieList.h"
#include "GUI/DialogKsiazki.h"
#include "GUI/DialogOProgramie.h"
#include "GUI/DialogDatasheetMouser.h"
#include "Wyszukiwanie/WyszukiwanieDatasheet.h"
#include "Narzedzia/UwagiHTML.h"
#include "GUI/DialogUwagHTML.h"
#include "GUI/DialogEdycjiZbiorczej.h"
#include "Logika/ProgramyZewnetrzne.h"
#include "Import/MetadaneKsiazek.h"
#include "Logika/StronyPDF.h"
#include "GUI/PanelKsiazek.h"
#include "Logika/LogikaKsiazek.h"
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include "GUI/PanelNarzedzi.h"
#include "GUI/PanelSerwisu.h"
#include "Logika/LogikaSerwisu.h"
#include "Logika/LogikaNarzedzi.h"
#include "GUI/DialogKatalogu.h"
#include "Logika/LogikaKatalogu.h"
#include "Logika/OpisyParametrow.h"
#include "Logika/OdczytDatasheetu.h"
#include "GUI/DialogPolaczenia.h"
#include "GUI/DialogUstawien.h"
#include "GUI/OknoGlowne.h"
#include "GUI/PanelElementow.h"
#include "GUI/PanelKategorii.h"
#include "Logika/LogikaMagazynu.h"
#include "Narzedzia/Konfiguracja.h"
#include "Narzedzia/KodKreskowy.h"
#include "Pomoc/PomocGUI.h"
#include "Pomoc/ObrazyPomocy.h"
#include <wx/mstream.h>
#include <wx/image.h>

static int bledy = 0;
#define CHECK(w)                                                                   \
    do {                                                                           \
        if (!(w)) {                                                                \
            ++bledy;                                                               \
            std::cerr << "BŁĄD " << __FILE__ << ":" << __LINE__ << "  " #w "\n";  \
        }                                                                          \
    } while (0)

static std::string env(const char* n, const char* d) {
    const char* v = std::getenv(n);
    return v ? v : d;
}

/// Uruchamia na chwilę zagnieżdżoną pętlę zdarzeń, żeby okna zdążyły się narysować
static void PrzerwaNaRysowanie(int ms) {
    wxEvtHandler wlasciciel;
    wxTimer timer(&wlasciciel);
    wxEventLoop petla;
    wlasciciel.Bind(wxEVT_TIMER, [&petla](wxTimerEvent&) { petla.Exit(); });
    timer.StartOnce(ms);
    petla.Run();
}

static void Zrzut(wxWindow* okno, const wxString& nazwa) {
    std::cerr << "krok: " << nazwa.utf8_string() << std::endl;
    const std::string katalog = env("DM_TEST_SHOTS", "");
    if (katalog.empty() || !okno) return;
    // GTK rysuje dopiero w prawdziwej pętli zdarzeń (samo wxYield nie wystarcza)
    okno->Raise();
    PrzerwaNaRysowanie(500);
    wxScreenDC ekran;
    const wxRect r = okno->GetScreenRect();
    wxBitmap bmp(r.width, r.height);
    wxMemoryDC mem(bmp);
    mem.Blit(0, 0, r.width, r.height, &ekran, r.x, r.y);
    mem.SelectObject(wxNullBitmap);
    bmp.SaveFile(wxString::FromUTF8(katalog) + "/" + nazwa + ".png", wxBITMAP_TYPE_PNG);
}

/// Zrzut całego ekranu zewnętrznym programem (ImageMagick "import"): pewniejszy dla okien modalnych bez menedżera okien
static void ZrzutEkranu(const wxString& nazwa) {
    std::cerr << "krok: " << nazwa.utf8_string() << std::endl;
    const std::string katalog = env("DM_TEST_SHOTS", "");
    if (katalog.empty()) return;
    PrzerwaNaRysowanie(1200);
    wxExecute("import -window root \"" + wxString::FromUTF8(katalog) + "/" + nazwa + ".png\"", wxEXEC_SYNC);
}

static wxWindow* Znajdz(wxWindow* rodzic, const wxString& etykieta) {
    for (wxWindow* dziecko : rodzic->GetChildren()) {
        if (auto* kontrolka = wxDynamicCast(dziecko, wxControl)) {
            if (kontrolka->GetLabelText() == etykieta) return kontrolka;
        }
        if (wxWindow* w = Znajdz(dziecko, etykieta)) return w;
    }
    return nullptr;
}

static wxWindow* ZnajdzZawierajacy(wxWindow* rodzic, const wxString& fragment) {
    for (wxWindow* dziecko : rodzic->GetChildren()) {
        if (auto* tekst = wxDynamicCast(dziecko, wxStaticText)) {
            if (tekst->GetLabel().Contains(fragment)) return tekst;
        }
        if (wxWindow* w = ZnajdzZawierajacy(dziecko, fragment)) return w;
    }
    return nullptr;
}

template <class T>
static T* ZnajdzTyp(wxWindow* rodzic) {
    for (wxWindow* dziecko : rodzic->GetChildren()) {
        if (auto* t = dynamic_cast<T*>(dziecko)) return t;
        if (T* w = ZnajdzTyp<T>(dziecko)) return w;
    }
    return nullptr;
}

static wxWindow* Gorne() {
    return wxTopLevelWindows.GetLast()->GetData();
}

static long long Liczba(ManagerBazyDanych& m, const char* sql) {
    auto w = m.zapytanie(sql);
    return (w && w->liczbaWierszy() == 1) ? w->liczba(0, 0) : -1;
}

static void Kliknij(wxWindow* przycisk) {
    wxCommandEvent zdarzenie(wxEVT_BUTTON, przycisk->GetId());
    zdarzenie.SetEventObject(przycisk);
    przycisk->GetEventHandler()->ProcessEvent(zdarzenie);
}

class TestApp : public wxApp {
public:
    bool OnInit() override {
        setenv("DM_BEZ_AKTUALIZACJI", "1", 1);   // test nie odpytuje GitHuba
        setenv("DM_BEZ_USTAWIEN", "1", 1);       // test nie czyta ani nie zapisuje ustawień użytkownika
        wxInitAllImageHandlers();
        delete wxLog::SetActiveTarget(new wxLogStderr);   // komunikaty logu na stderr, bez modalnych okien blokujących test
        const wxString tmp = wxFileName::GetTempDir() + "/dm_gui_test";
        wxFileName::Mkdir(tmp, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        const wxString plik = tmp + "/config.ini";
        wxRemoveFile(plik);

        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        m.ustawSciezkeKonfiguracji(plik.utf8_string());
        m.ustawKatalogiSQL({env("DM_TEST_SQL", "../sql")});

        UstawieniaBazy us;
        us.host = env("DM_TEST_HOST", "localhost");
        us.port = std::atoi(env("DM_TEST_PORT", "5432").c_str());
        us.bazaDanych = env("DM_TEST_DB", "dm_test");
        us.uzytkownik = env("DM_TEST_USER", "postgres");
        us.haslo = env("DM_TEST_PASSWORD", "");
        us.timeoutSek = 3;
        m.ustawUstawienia(us);
        CHECK(m.zapiszUstawienia());

        CallAfter([this] { Uruchom(); });
        return true;
    }

    void OnAssertFailure(const wxChar* plik, int linia, const wxChar* funkcja,
                         const wxChar* warunek, const wxChar* komunikat) override {
        ++bledy;
        std::cerr << "ASERCJA wx: " << wxString(plik) << ":" << linia << " " << wxString(warunek)
                  << " " << wxString(komunikat) << "\n";
    }

    int OnExit() override {
        ManagerBazyDanych::pobierz().rozlacz();
        return wxApp::OnExit();
    }

private:
    // =====================================================================
    // Etap 2: drzewo kategorii, lista elementów, formularze
    // =====================================================================
    // =====================================================================
    // Pomoc: menu, opisy pól (dymki), F1, okno pomocy
    // =====================================================================
    void TestPomocy(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        CHECK(!PomocGUI::ZnajdzKatalogPomocy().empty());
        PomocGUI::WczytajOpisyZPliku();
        PomocGUI::WczytajOpisyZBazy();
        CHECK(okno->GetMenuBar()->FindItem(wxID_HELP) != nullptr);

        // dymki i klucze F1 w formularzu elementu
        {
            Element e;
            DialogElementu d(okno, logika, e);
            auto* symbol = ZnajdzTyp<wxTextCtrl>(&d);   // pierwsze pole tekstowe to symbol
            CHECK(symbol != nullptr);
            if (symbol) {
                CHECK(PomocGUI::KluczKontrolki(symbol) == "element.symbol");
                CHECK(symbol->GetToolTipText().Contains("Nazwa elementu"));
            }
        }

        // F1 w polu: okienko z opisem pola, z przyciskiem "Więcej w pomocy"
        {
            Element e;
            DialogElementu d(okno, logika, e);
            CallAfter([&] {
                if (auto* t = ZnajdzTyp<wxTextCtrl>(&d)) t->SetFocus();
                CallAfter([&] {
                    auto* opis = dynamic_cast<wxDialog*>(Gorne());
                    CHECK(opis != nullptr && opis != &d);
                    if (!opis) return;
                    CHECK(opis->GetTitle().Contains("Symbol"));
                    ZrzutEkranu("13_f1_opis_pola");
                    opis->EndModal(wxID_OK);
                });
                PomocGUI::PokazPomocDlaFokusu();
                d.EndModal(wxID_CANCEL);
            });
            d.ShowModal();
        }

        // okno pomocy na kotwicy
        CHECK(PomocGUI::PokazPomoc("pole-wartosc"));
        wxFrame* ramka = nullptr;
        for (wxWindowList::compatibility_iterator it = wxTopLevelWindows.GetFirst(); it; it = it->GetNext()) {
            auto* f = dynamic_cast<wxFrame*>(it->GetData());
            if (f && f->GetTitle().Contains("pomoc")) ramka = f;
        }
        CHECK(ramka != nullptr);
        if (ramka) {
            wxYield();
            ZrzutEkranu("14_pomoc");
            ramka->Close(true);
        }
        wxYield();
    }

    // =====================================================================
    // Miejsce magazynowania w formularzu elementu
    // =====================================================================
    void TestMiejsca(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIM\\_%'");
        auto kombinacje = [](wxWindow* rodzic) {
            std::vector<wxComboBox*> v;
            std::function<void(wxWindow*)> idz = [&](wxWindow* w) {
                for (wxWindow* d : w->GetChildren()) {
                    if (auto* c = dynamic_cast<wxComboBox*>(d)) v.push_back(c);
                    idz(d);
                }
            };
            idz(rodzic);
            return v;
        };
        // istniejące miejsce trafia do pól, a swobodny zapis w polu kontenera zostaje rozdzielony
        Element e; e.symbol = "GUIM_A"; e.ustawMiejsce({"GUIM Kontener 1", "A1", "1"});
        auto id = logika.zapiszElement(e);
        CHECK(id.has_value());
        if (!id) return;
        {
            auto el = logika.pobierzElement(*id);
            DialogElementu d(okno, logika, *el);
            auto combos = kombinacje(&d);
            CHECK(combos.size() == 2);   // waluta i kontener
            if (combos.size() == 2) {
                CHECK(combos[1]->GetValue() == "GUIM Kontener 1");
                CHECK(PomocGUI::KluczKontrolki(combos[1]) == "element.kontener");
                Element z = d.PobierzElement();
                CHECK(z.kontener == "GUIM Kontener 1" && z.szuflada == "A1" && z.przegroda == "1");
                combos[1]->SetValue("GUIM Kontener 2 szufladka B3/4");
                for (wxWindow* w : d.GetChildren())
                    if (auto* t = dynamic_cast<wxTextCtrl*>(w)) {
                        if (t->GetHint() == "np. A1" || t->GetHint() == "np. 1") t->SetValue("");
                    }
                z = d.PobierzElement();
                CHECK(z.kontener == "GUIM Kontener 2" && z.szuflada == "B3" && z.przegroda == "4");
                CallAfter([&] { ZrzutEkranu("15_miejsce_formularz"); d.EndModal(wxID_CANCEL); });
                d.ShowModal();
            }
        }
        // lista: kolumna Miejsce
        FiltrElementow f; f.tekst = "GUIM Kontener 1";
        std::vector<Element> w;
        logika.pobierzElementy(f, w);
        CHECK(w.size() == 1 && w[0].miejsceTekst() == "GUIM Kontener 1, A1/1");
        m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIM\\_%'");
    }

    // =====================================================================
    // Generator szeregów E i import CSV
    // =====================================================================
    void TestImportuElementow(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE obudowa = 'GUII' OR symbol LIKE 'GUII\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE obudowa = 'GUII' OR symbol LIKE 'GUII\\_%'");
            for (int i = 0; i < 4; ++i)
                m.wykonaj("DELETE FROM tbl_kategorie k WHERE k.nazwa LIKE 'GUII%' AND NOT EXISTS (SELECT 1 FROM tbl_kategorie c WHERE c.id_rodzica = k.id)");
        };
        sprzatanie();
        {
            DialogImportuElementow d(okno, logika, DialogImportuElementow::Tryb::Szereg);
            CHECK(d.Pozycje().size() > 50);              // domyślnie E24, 10 Ω...1 MΩ
            d.UstawPole("obudowa", "GUII");
            d.UstawPole("od", "100");
            d.UstawPole("do", "1k");
            d.UstawPole("ilosc", "7");
            d.UstawPole("kategoria", "GUII Rezystory / SMD");
            d.UstawPole("miejsce", "GUII Kontener 1 szufladka A1");
            CHECK(d.Pozycje().size() == 25 && d.LiczbaPoprawnych() == 25);
            d.UstawPole("od", "zle");
            CHECK(d.Pozycje().empty());                  // zły zakres: pusty podgląd, komunikat zamiast zapisu
            d.UstawPole("od", "100");
            CallAfter([&] { ZrzutEkranu("16_generator_szeregow"); d.EndModal(wxID_CANCEL); });
            d.ShowModal();
            CHECK(d.Zapisz());
            CHECK(d.Wynik().dodano == 25);
            d.UstawPole("duplikaty", "1");
            CHECK(d.Zapisz());
            CHECK(d.Wynik().zsumowano == 25);
            auto e = logika.znajdzPoSymbolu("R-470R-GUII-5%");
            CHECK(e && e->iloscDostepna == 14 && e->szuflada == "A1");
        }
        {
            DialogImportuElementow d(okno, logika, DialogImportuElementow::Tryb::Plik);
            const wxString sciezka = wxFileName::GetTempDir() + "/guii.csv";
            {
                wxFFile f(sciezka, "wb");
                const std::string csv = "Symbol;Obudowa;Ilość;Kategoria\nGUII_BC547;TO-92;10;GUII Tranzystory\n;;5;\n";
                f.Write(csv.data(), csv.size());
            }
            CHECK(d.WczytajPlik(sciezka, true));
            CHECK(d.Pozycje().size() == 2 && d.LiczbaPoprawnych() == 1);
            CHECK(!d.WczytajPlik("/nie/ma/takiego.csv", true));
            CHECK(d.WczytajPlik(sciezka, true));
            CHECK(d.Zapisz() && d.Wynik().dodano == 1 && d.Wynik().bledy.size() == 1);
            wxRemoveFile(sciezka);
        }
        sprzatanie();
    }

    void TestKodow(OknoGlowne* okno) {
        using K = KodyElementow::Kolor;
        DialogKodow d(okno);
        d.UstawPaski({K::Zolty, K::Fioletowy, K::Czerwony, K::Zloty});
        CHECK(d.OpisPaskow().Contains("4,7") && d.OpisPaskow().Contains("5"));
        CHECK(d.Wartosc() == "4k7");
        // 4 paski bez paska tolerancji = 20 %
        d.UstawPaski({K::Zolty, K::Fioletowy, K::Czerwony, K::Brak});
        CHECK(d.OpisPaskow().Contains("4,7") && d.OpisPaskow().Contains("20"));
        // 6 pasków: 1 kΩ 1 % 50 ppm/K
        d.UstawPaski({K::Brazowy, K::Czarny, K::Czarny, K::Brazowy, K::Brazowy, K::Czerwony});
        CHECK(d.OpisPaskow().Contains("50 ppm/K") && d.Wartosc() == "1k");
        d.UstawWartosc("4k7", 20, 3);
        CHECK(d.PaskiZWartosci().size() == 3);
        d.UstawWartosc("4k7", 20, 4);
        CHECK(d.PaskiZWartosci().size() == 4 && d.PaskiZWartosci()[3] == K::Brak);
        d.UstawWartosc("4k7", 1, 6);
        CHECK(d.PaskiZWartosci().size() == 6);
        d.UstawWartosc("330", 1, 4);
        CHECK(d.PaskiZWartosci().size() == 4);
        d.UstawWartosc("4k75", 1, 4);
        CHECK(d.PaskiZWartosci().empty());
        d.UstawWartosc("4k75", 1, 5);
        CHECK(d.PaskiZWartosci().size() == 5);
        d.UstawKodSMD("104");
        CHECK(d.OpisSMD().Contains("100 nF"));
        d.UstawKodSMD("01A");
        CHECK(d.OpisSMD().Contains("100 Ω"));
        d.UstawKodSMD("zzz");
        CHECK(d.OpisSMD().Contains("Nie rozpoznaję"));
        d.UstawPaski({K::Zolty, K::Fioletowy, K::Czerwony, K::Zloty});
        d.PokazStrone(1);
        d.UstawWartosc("4k7", 5, 4);
        CallAfter([&] { ZrzutEkranu("17b_wartosc4"); d.UstawWartosc("4k7", 5, 5); ZrzutEkranu("17c_wartosc5"); d.UstawWartosc("4k7", 1, 6); ZrzutEkranu("17d_wartosc6"); d.PokazStrone(0); d.EndModal(wxID_CLOSE); });
        d.ShowModal();
        d.UstawPaski({K::Zolty, K::Fioletowy, K::Czerwony, K::Zloty});
        CallAfter([&] { ZrzutEkranu("17_narzedzia_kody"); d.EndModal(wxID_CLOSE); });
        d.ShowModal();
    }

    // =====================================================================
    // Kamera USB (kamera testowa) i zdjęcie w panelu po prawej
    // =====================================================================
    void TestKamery(OknoGlowne* okno) {
        setenv("DM_KAMERA_TEST", "1", 1);
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUIK\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIK\\_%'");
        };
        sprzatanie();
        Konfiguracja& kf = Konfiguracja::pobierz();
        UstawieniaBazowe stare = kf.pobierzUstawieniaAplikacji(), us = stare;
        const wxString katalog = wxFileName::GetTempDir() + "/guik_zdjecia";
        us.sciezkaObrazki = katalog.utf8_string();
        kf.ustawUstawieniaAplikacji(us);

        CHECK(Kamera::obslugiwana());
        Element e; e.symbol = "GUIK_A"; e.opis = "Element ze zdjęciem"; e.iloscDostepna = 5; e.iloscMinimalna = 2;
        auto id = logika.zapiszElement(e);
        CHECK(id.has_value());
        PanelElementow* pe = ZnajdzTyp<PanelElementow>(okno);
        PanelPodgladu* pp = ZnajdzTyp<PanelPodgladu>(okno);
        CHECK(pe && pp);
        if (!id || !pe || !pp) { kf.ustawUstawieniaAplikacji(stare); return; }
        pe->UstawKategorie(std::nullopt);   // wszystkie elementy
        pe->Szukaj("GUIK_A");
        pe->WybierzPierwszy();
        CHECK(pp->IdElementu() == *id);
        CHECK(pp->Opis().Contains("Stan: 5") && pp->Opis().Contains("minimum 2"));
        CHECK(!pp->ZdjecieWidoczne());

        // menu kontekstowe: "Dodaj do katalogu" tworzy pozycję katalogu z danych elementu
        {
            LogikaKatalogu lk(m);
            m.wykonaj("DELETE FROM tbl_katalog WHERE symbol = 'GUIK_A'");
            CHECK(!lk.znajdz("GUIK_A"));
            pe->DodajDoKatalogu();
            auto k = lk.znajdz("GUIK_A");
            CHECK(k && k->opis == "Element ze zdjęciem");
            pe->DodajDoKatalogu();   // drugi raz: bez duplikatu i bez okna
            m.wykonaj("DELETE FROM tbl_katalog WHERE symbol = 'GUIK_A'");
        }

        {   // kamera termowizyjna (kamera testowa: ramka 256x384, obraz + dane 16-bitowe)
            DialogTermowizji t(okno);
            CHECK(t.Otwarta() && t.LiczbaKlatek() == 1);
            for (int i = 0; i < 3; ++i) t.Odswiez();
            CHECK(t.LiczbaKlatek() == 4);
            CHECK(t.UkladAktualny() == Termowizja::Uklad::ObrazIDane && t.Stat().ok);
            CHECK(t.Stat().minC > 19.9 && t.Stat().maxC < 100 && t.Stat().maxC > t.Stat().minC);
            CHECK(t.Raport().Contains("256 x 384") && t.Raport().Contains("Pierwsze 16"));
            const double max1 = t.Stat().maxC;
            t.UstawParametry("128", "273,15");                  // zły dzielnik: wyraźnie inne temperatury
            CHECK(std::fabs(t.Stat().maxC - max1) > 10);
            t.UstawParametry("64", "273,15");
            CHECK(std::fabs(t.Stat().maxC - max1) < 1e-9);
            for (int p = 0; p < 4; ++p) t.UstawPalete(p);
            t.UstawPalete(0);
            t.Show(); wxYield(); t.Centre(); t.Raise(); PrzerwaNaRysowanie(600); ZrzutEkranu("termowizja");
            CHECK(t.WyslijZoom("8004").Contains("pominięte") && t.WyslijZoom("zzz").Contains("szesnastkow") && t.WyslijZoom("12345").Contains("szesnastkow") && t.WyslijZoom("8004 8020 8000").Contains("pominięte") && t.WyslijZoom("8004 xx").Contains("szesnastkow"));
            CHECK(!t.DodajPunktKalibracji("0"));                // klatka nie jest surowa: kalibracja odrzucona
            t.UstawUklad(2);                                    // wymuszony tylko obraz: bez temperatur
            CHECK(t.UkladAktualny() == Termowizja::Uklad::TylkoObraz && !t.Stat().ok);
            t.UstawUklad(1);
            CHECK(t.Stat().ok);
            t.UstawUklad(0);
            t.Zamroz(true);
            const int po = t.LiczbaKlatek();
            t.Odswiez();
            CHECK(t.Zamrozony() && t.LiczbaKlatek() == po);
            t.Zamroz(false);
            const wxString kat = wxFileName::GetTempDir();
            const wxString probka = t.ZapiszProbke(kat);
            CHECK(!probka.empty() && wxFileExists(probka) && wxFileName(probka).GetSize() == 256 * 384 * 2);
            CHECK(wxFileExists(probka.BeforeLast('.') + ".txt"));
            const wxString png = wxFileName(kat, "termo_test.png").GetFullPath();
            CHECK(t.ZapiszObraz(png) && wxFileExists(png));
            wxRemoveFile(probka); wxRemoveFile(probka.BeforeLast('.') + ".txt"); wxRemoveFile(png);
        }

        {   // kamera termowizyjna w trybie surowym (384x292, odczyty 14-bitowe): kalibracja użytkownika
            setenv("DM_KAMERA_TEST", "2", 1);
            DialogTermowizji t(okno);
            CHECK(t.Otwarta() && t.UkladAktualny() == Termowizja::Uklad::Surowe16 && !t.Stat().ok);
            CHECK(t.Raport().Contains("surowe dane 16-bitowe") && t.Raport().Contains("Parametry z nagłówka"));
            CHECK(t.DodajPunktKalibracji("0") && t.LiczbaPunktowKalibracji() == 1 && !t.Stat().ok);   // jeden punkt: jeszcze bez temperatur
            t.Odswiez(); t.Odswiez();                                                              // inny odczyt środka (gorący punkt się przesuwa)
            CHECK(!t.DodajPunktKalibracji("abc"));
            CHECK(t.DodajPunktKalibracji("100") && t.LiczbaPunktowKalibracji() >= 1);
            t.WyczyscKalibracje();
            CHECK(t.LiczbaPunktowKalibracji() == 0 && !t.Stat().ok);
            setenv("DM_KAMERA_TEST", "1", 1);
        }

        wxString sciezka;
        {
            DialogKamery d(okno, "GUIK_A");
            CHECK(d.Otwarta());
            for (int i = 0; i < 3; ++i) d.Odswiez();
            CHECK(d.LiczbaKlatek() == 4);               // jedna z otwarcia plus trzy
            d.ZrobZdjecie();
            CHECK(d.Zamrozony());
            const int po = d.LiczbaKlatek();
            d.Odswiez();
            CHECK(d.LiczbaKlatek() == po);               // zamrożony obraz się nie zmienia
            d.Powtorz();
            CHECK(!d.Zamrozony());
            d.ZrobZdjecie();
            CHECK(d.Zatwierdz());
            sciezka = d.Sciezka();
            CHECK(sciezka.EndsWith(".jpg") && sciezka.Contains("GUIK_A") && wxFileExists(sciezka));
            wxImage im(sciezka);
            CHECK(im.IsOk() && im.GetWidth() == 320 && im.GetHeight() == 240);
            CallAfter([&] { ZrzutEkranu("18_kamera"); d.EndModal(wxID_CANCEL); });
            d.ShowModal();
        }
        CHECK(pp->UstawZdjecie(sciezka));
        CHECK(pp->ZdjecieWidoczne() && pp->Sciezka() == sciezka);
        auto z = logika.pobierzElement(*id);
        CHECK(z && z->zdjecieSciezka == sciezka.utf8_string() && z->iloscDostepna == 5);   // stan bez zmian
        auto hist = m.zapytanie("SELECT count(*) FROM tbl_transakcje WHERE id_elementu = " + std::to_string(*id) +
                                " AND typ_transakcji = 'KOREKTA'");
        CHECK(hist && hist->liczba(0, 0) == 0);
        CHECK(pe->Wybrany() && pe->Wybrany()->zdjecieSciezka == sciezka.utf8_string());   // lista odświeżona
        wxYield();
        ZrzutEkranu("19_panel_zdjecia");

        CHECK(pp->UstawZdjecie(""));
        CHECK(!pp->ZdjecieWidoczne());
        z = logika.pobierzElement(*id);
        CHECK(z && z->zdjecieSciezka.empty());
        pe->Szukaj("");
        pe->Wyczysc();
        CHECK(pp->IdElementu() == 0);                    // brak zaznaczenia czyści panel

        wxRemoveFile(sciezka);
        wxFileName::Rmdir(katalog);
        kf.ustawUstawieniaAplikacji(stare);
        sprzatanie();
    }

    void TestKalkulatorow(OknoGlowne* okno) {
        DialogKalkulatorow d(okno);
        d.UstawDzielnik("5", "3,3", "10k", "20k");
        CHECK(d.OpisDzielnika().Contains("R1 = ") && d.OpisDzielnika().Contains("Uwy = 3,"));
        CHECK(d.SprawdzenieDzielnika().Contains("3,333"));
        d.UstawDzielnik("5", "6", "10k", "0");
        CHECK(d.OpisDzielnika().Contains("mniejsze"));
        d.UstawDzielnik("5", "3,3", "10k", "20k");
        d.UstawLED("5", "2", "20");
        CHECK(d.OpisLED().Contains("150 \u03a9") && d.OpisLED().Contains("20 mA"));
        d.UstawLED("2", "2", "20");
        CHECK(d.OpisLED().Contains("musi być większe"));
        d.UstawLED("5", "2", "20");
        d.UstawRC("10k", "100n", "1k");
        CHECK(d.OpisRC().Contains("159 Hz") && d.OpisRC().Contains("1 ms"));
        CHECK(d.OpisRCDobor().Contains("R = ") && d.OpisRCDobor().Contains("C = "));
        d.UstawPola(3, {"18", "1", "4700u", "12", "1"});
        CHECK(d.RodzajSchematuZasilacza() == PanelSchematu::Rodzaj::ZasilaczMostek);
        d.UstawProstownik(1);
        CHECK(d.RodzajSchematuZasilacza() == PanelSchematu::Rodzaj::ZasilaczOdczep);
        d.UstawProstownik(2);
        CHECK(d.RodzajSchematuZasilacza() == PanelSchematu::Rodzaj::Zasilacz);
        d.UstawProstownik(0);
        CHECK(d.RodzajSchematuZasilacza() == PanelSchematu::Rodzaj::ZasilaczMostek);
        CHECK(d.WynikStrony(3).Contains("wyj") && d.WynikStrony(3).Contains("VA"));
        CHECK(d.WynikStrony(3, 1).Contains("Usek co najmniej") && d.WynikStrony(3, 1).Contains("C co najmniej"));
        CHECK(d.LiczbaStron() == 7);   // trafo głośnikowe i lampa przeniesione (Elementy indukcyjne, Audio i lampowe)
        d.UstawPola(4, {"12", "47k", "10k", "2k2", "470", "100", "2"});
        CHECK(d.WynikStrony(4).Contains("praca liniowa") && d.WynikStrony(4, 1).Contains("R1 = "));
        d.UstawPola(4, {"12", "4k7", "10k", "2k2", "10", "100", "2"});
        CHECK(d.WynikStrony(4).Contains("NASYCENIE"));
        d.UstawPola(4, {"12", "47k", "10k", "2k2", "470", "100", "2"});
        // ESR: tg delta 0,14 przy 25 V, C = 1000 uF: 0,14 / (2 pi 120 1e-3) = 186 mOhm
        d.UstawESR("1000u", "25", "1");
        CHECK(d.WynikStrony(5).Contains("0,14") && d.WynikStrony(5).Contains("186 m") && d.WynikStrony(5).Contains("Moc strat"));
        d.UstawESR("1000u", "25", "");
        CHECK(!d.WynikStrony(5).Contains("Moc strat"));
        d.UstawESR("abc", "25", "");
        CHECK(d.WynikStrony(5).Contains("Wpisz"));
        // LM317 / LM337: 1,25·(1 + 2200/240) + 50 µA·2200 = 12,82 V
        d.UstawPola(6, {"18", "500", "240", "1,25", "50", "2k2", "12"});
        CHECK(d.WynikStrony(6).Contains("12,82 V") && !d.WynikStrony(6).Contains("-12,82"));
        CHECK(d.WynikStrony(6, 1).Contains("R2 = 2,04") || d.WynikStrony(6, 1).Contains("R2 = 2.04"));
        d.UstawTypLM(1);
        CHECK(d.WynikStrony(6).Contains("-12,82 V"));
        d.UstawTypLM(0);
        d.UstawPola(6, {"14", "500", "240", "1,25", "50", "2k2", "12"});
        CHECK(d.WynikStrony(6).Contains("za małe"));
        d.UstawPola(6, {"18", "1", "24k", "1,25", "50", "2k2", "1"});
        CHECK(d.WynikStrony(6).Contains("minimalny") && d.WynikStrony(6, 1).Contains("większe niż Vref"));
        {
            DialogKalkulatorow dc(okno, true);   // okno Elementy indukcyjne > Cewki powietrzne
            CHECK(dc.LiczbaStron() == 2);
        dc.UstawPola(7, {"10", "0,5", "", "", "50", "10"});
        CHECK(dc.WynikStrony(7).Contains("L = 8,5") && dc.WynikStrony(7).Contains("Wheeler") && dc.WynikStrony(7, 1).Contains("zwojów: 58"));
        dc.UstawPola(7, {"10", "0,5", "", "", "abc", ""});
        CHECK(dc.WynikStrony(7).Contains("Wpisz"));
        dc.UstawPola(7, {"10", "0,5", "0,4", "", "50", ""});
        CHECK(dc.WynikStrony(7).Contains("Dane muszą"));
        dc.UstawPola(8, {"10", "0,5", "", "", "50", "10", "5000"});
        CHECK(dc.WynikStrony(8).Contains("L = 13") && dc.WynikStrony(8).Contains("grubość") && dc.WynikStrony(8, 1).Contains("warstw: "));
        }
        d.UstawESR("1000u", "25", "1");
        CHECK(d.LiczbaWierszyESR() == 14 && d.KomorkaESR(0, 0).Contains("tg") && d.KomorkaESR(0, 4).Contains("0,14"));
        // schematy: liczone elementy wyróżnione
        {
            using R = PanelSchematu::Rodzaj;
            CHECK(PanelSchematu::ElementyLiczone(R::Dzielnik).size() == 3 && PanelSchematu::ElementyLiczone(R::Tranzystor).size() == 6);
            CHECK(PanelSchematu::ElementyLiczone(R::KondensatorESR).front() == "ESR");
            CHECK(PanelSchematu::ElementyLiczone(R::LM317).size() == 2 && PanelSchematu::ElementyLiczone(R::LM337).back() == "Uwy");
            CHECK(PanelSchematu::ElementyLiczone(R::CewkaJedno).front() == "L" && PanelSchematu::ElementyLiczone(R::CewkaWiele).size() == 1);
            PanelSchematu ps(&d, R::Lampa);
            CHECK(ps.Liczony("Rk") && ps.Liczony("Ck") && !ps.Liczony("Ra"));
        }
        d.PokazStrone(0);
        CallAfter([&] {
            d.PokazStrone(3); ZrzutEkranu("25_kalkulator_zasilacz");
            d.PokazStrone(4); ZrzutEkranu("27_kalkulator_tranzystor");
            d.PokazStrone(5); ZrzutEkranu("26_kalkulator_esr");
            d.PokazStrone(0);
            ZrzutEkranu("22_kalkulator_dzielnik");
            d.PokazStrone(1); ZrzutEkranu("23_kalkulator_led");
            d.PokazStrone(2); ZrzutEkranu("24_kalkulator_rc");
            d.EndModal(wxID_CLOSE);
        });
        d.ShowModal();
    }

    void TestEagle(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        LogikaProjektow lp(m, logika);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_pozycje_projektow WHERE id_projektu IN (SELECT id FROM tbl_projekty WHERE nazwa LIKE 'GUIE2\\_%')");
            m.wykonaj("DELETE FROM tbl_projekty WHERE nazwa LIKE 'GUIE2\\_%'");
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUIE2\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIE2\\_%'");
        };
        sprzatanie();
        Element e; e.symbol = "GUIE2_R10K"; e.symbolProducenta = "RC0603-10K"; e.producent = "Yageo"; e.iloscDostepna = 12;
        e.ustawMiejsce({"Kontener 1", "A1", ""});
        auto ide = logika.zapiszElement(e);
        Projekt p; p.nazwa = "GUIE2_Zasilacz";
        auto idp = lp.zapiszProjekt(p);
        CHECK(ide && idp);
        if (!ide || !idp) { sprzatanie(); return; }
        PozycjaProjektu poz; poz.idProjektu = *idp; poz.idElementu = *ide; poz.oznaczenia = "R1, R2"; poz.wymagana = 2;
        CHECK(lp.dodajPozycje(poz).has_value());

        PanelProjektow* pp = ZnajdzTyp<PanelProjektow>(okno);
        CHECK(pp != nullptr);
        if (pp) {
            pp->Odswiez();
            // wybór projektu na liście
            pp->WybierzProjekt(*idp);
            CHECK(pp->WybranyId() == idp);
            const wxString scr = wxFileName::GetTempDir() + "/guie2.scr", ulp = wxFileName::GetTempDir() + "/guie2.ulp";
            CHECK(pp->SkryptEagle(scr) == 2);
            wxFFile f(scr, "rb"); wxString tresc;
            CHECK(f.IsOpened() && f.ReadAll(&tresc));
            CHECK(tresc.Contains("ATTRIBUTE R1 DM_SYMBOL 'GUIE2_R10K';") && tresc.Contains("ATTRIBUTE R2 DM_STAN '12';") &&
                  tresc.Contains("ATTRIBUTE R1 DM_MIEJSCE 'Kontener 1, A1';") && tresc.Contains("ATTRIBUTE R1 MPN 'RC0603-10K';"));
            CHECK(pp->ZapiszUlp(ulp) && wxFileExists(ulp));
            wxRemoveFile(scr); wxRemoveFile(ulp);
        }
        sprzatanie();
    }


    void TestObudow(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        LogikaObudow lo(m);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUIO\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIO\\_%'");
            m.wykonaj("DELETE FROM tbl_obudowy WHERE nazwa ILIKE 'GUIO%'");
        };
        sprzatanie();
        {   // słownik opisów parametrów
            LogikaSlownikaParametrow sl(ManagerBazyDanych::pobierz());
            ManagerBazyDanych::pobierz().wykonaj("DELETE FROM tbl_slownik_parametrow WHERE klucz LIKE 'guislow%'");
            WpisSlownikaParametrow w; w.symbol = "GUISlow_A"; w.opis = "Opis testowy";
            CHECK(sl.zapisz(w).has_value());
            DialogSlownikaParametrow ds(okno, sl);
            ds.Szukaj("guislow");
            CHECK(ds.Wpisy().size() == 1 && ds.Wpisy()[0].opis == "Opis testowy");
            ds.Wybierz(ds.Wpisy()[0].id);
            CHECK(sl.usun(ds.Wpisy()[0].id));
            ds.Odswiez();
            CHECK(ds.Wpisy().empty());
            ds.Szukaj("");
            CHECK(ds.Wpisy().size() > 10);
            {   // dopisywanie nieznanych symboli po odczycie datasheetu
                DialogDoSlownika dd(okno, {"GUISlow_B", "GUISlow_C"});
                CHECK(dd.LiczbaWierszy() == 2 && dd.Wpisy().empty());
                dd.UstawOpis(0, "Opis B");
                dd.UstawOpis(1, "Pominięty", false);
                CHECK(dd.Wpisy().size() == 1 && dd.Wpisy()[0].symbol == "GUISlow_B");
                CHECK(sl.zapisz(dd.Wpisy()[0]).has_value());
                sl.wczytajDoPamieci();
                CHECK(OpisyParametrow::zeSlownika("guislow b") == "Opis B");
                ManagerBazyDanych::pobierz().wykonaj("DELETE FROM tbl_slownik_parametrow WHERE klucz LIKE 'guislow%'");
            }
            sl.wczytajDoPamieci();
            CHECK(!OpisyParametrow::zeSlownika("W_amax").empty());
        }
        Obudowa o; o.nazwa = "GUIO-220"; o.montaz = "THT"; o.liczbaPinow = 3; o.mocStratW = 2.5; o.napiecieMaxV = 400;
        o.rodzina = "TO-220";
        auto id = lo.zapisz(o);
        if (!id) std::cout << "blad obudowy: " << lo.pobierzBlad() << "\n";
        CHECK(id.has_value());
        Element e; e.symbol = "GUIO_E"; e.obudowa = "guio220, GUIO-R7.5"; e.iloscDostepna = 1;
        auto ide = logika.zapiszElement(e);
        CHECK(ide.has_value());
        if (id && ide) {
            DialogObudow dlg(okno, lo);
            bool jest = false;
            for (const Obudowa& x : dlg.Obudowy()) if (x.id == *id) { jest = true; CHECK(x.liczbaElementow == 1); }
            CHECK(jest);
            // typowe elementy z zestawu biblioteki Eagle
            {
                ImportLBR::Biblioteka lib;
                ImportLBR::Pakiet pk; pk.nazwa = "GUIO-220";
                lib.pakiety.push_back(pk);
                ImportLBR::Urzadzenie u; u.zestaw = "GUIO_BD437"; u.opisZestawu = "NPN"; u.prefiks = "Q"; u.pakiet = "GUIO-220";
                lib.urzadzenia.push_back(u);
                WynikImportuObudow wi;
                CHECK(lo.importuj(lib, false, wi) && wi.typowych == 1);
            }
            dlg.Wybierz(*id);
            CHECK(dlg.TekstTypowych().Contains("GUIO_BD437"));
            // zaznaczenie wielokrotne: zbiorcza zmiana atrybutów
            {
                Obudowa o2; o2.nazwa = "GUIO-ZB2";
                auto id2 = lo.zapisz(o2);
                CHECK(id2.has_value());
                if (id2) {
                    dlg.Odswiez();
                    dlg.Wybierz(std::vector<long long>{*id, *id2});
                    CHECK(dlg.LiczbaZaznaczonych() == 2);
                    DialogObudow::ZmianyZbiorcze z; z.rodzina = "ZBIORCZA"; z.mocStratW = 1.5;
                    CHECK(dlg.ZmienZaznaczone(z) == 2);
                    auto p1 = lo.pobierzObudowe(*id), p2 = lo.pobierzObudowe(*id2);
                    CHECK(p1 && p2 && p1->rodzina == "ZBIORCZA" && p2->rodzina == "ZBIORCZA" && p2->mocStratW && *p2->mocStratW == 1.5);
                    CHECK(lo.usun(*id2));
                    if (p1) { Obudowa przywroc = *p1; przywroc.mocStratW = 2.5; przywroc.rodzina = ""; CHECK(lo.zapisz(przywroc).has_value()); }
                    dlg.Odswiez();
                    dlg.Wybierz(*id);
                }
            }
            CallAfter([&] { ZrzutEkranu("22_obudowy"); dlg.EndModal(wxID_CANCEL); });
            dlg.ShowModal();

            // podpowiedź obudowy w formularzu elementu: symbol z dopiskiem wskazuje obudowę z biblioteki
            {
                Element nowy; nowy.symbol = "GUIO_BD437G";
                DialogElementu de(okno, logika, nowy);
                auto podp = de.PodpowiedziObudow();
                CHECK(podp.size() == 1 && podp[0] == "GUIO-220");
                de.DodajObudowe("guio220");
                de.DodajObudowe("GUIO-220");                       // duplikat po kluczu: bez zmian
                CHECK(de.PobierzElement().obudowa == "guio220" || de.PobierzElement().obudowa == "GUIO-220");
                CHECK(de.PodpowiedziObudow().empty());              // już ma tę obudowę
            }
            // import listy części: footprint z pliku zamieniany na obudowę ze słownika
            {
                const wxString plik = wxFileName::GetTempDir() + "/gui_bom_obudowy.csv";
                {
                    std::ofstream f(plik.utf8_string(), std::ios::binary);
                    f << "Reference,Value,Footprint,Qty\nQ1,GUIO_NIEMA,GUIO-220,1\nQ2,GUIO_BD437,,1\n";
                }
                LogikaProjektow lp(m, logika);
                DialogImportBOM imp(okno, logika, lp, 0, "GUIO", plik);
                CHECK(imp.LiczbaWierszy() == 2);
                CHECK(imp.OpisWiersza(0).Contains("GUIO-220"));      // obudowa ze słownika
                CHECK(imp.OpisWiersza(1).Contains("GUIO-220"));      // podpowiedź z typowych elementów
                wxRemoveFile(plik);
            }

            // wybór obudów elementu: zaznaczone z pola, dodanie i zdjęcie zaznaczenia, wyszukiwanie
            {
                DialogWyboruObudow wyb(okno, lo, LogikaObudow::rozbijTekst("GUIO-220, GUIO-R7.5"));
                CHECK(wyb.Wybrane().size() == 2);
                wyb.Szukaj("GUIO");
                CHECK(wyb.LiczbaPozycji() == 2);
                wyb.Zaznacz("GUIO-R7.5", false);
                CHECK(wyb.Wybrane() == std::vector<std::string>{"GUIO-220"});
                wyb.Zaznacz("guio-r7.5", true);
                CHECK(wyb.Wybrane().size() == 2 && wyb.Wybrane()[1] == "GUIO-R7.5");
            }
            // podgląd elementu pokazuje atrybuty obudowy
            PanelElementow* pe = ZnajdzTyp<PanelElementow>(okno);
            PanelPodgladu* pp = ZnajdzTyp<PanelPodgladu>(okno);
            CHECK(pe && pp);
            if (pe && pp) {
                pe->UstawKategorie(std::nullopt);
                pe->Szukaj("GUIO_E");
                pe->WybierzPierwszy();
                CHECK(pp->IdElementu() == *ide);
                wxString op = pp->Opis(); op.Replace("\n", " ");   // panel zawija wiersze
                if (!(op.Contains("THT") && op.Contains("2,5 W") && op.Contains("400 V") && op.Contains("3 wyprowadze"))) std::cerr << "OPIS: " << op.utf8_string() << std::endl;
                CHECK(op.Contains("THT") && op.Contains("2,5 W") && op.Contains("400 V") && op.Contains("3 wyprowadze"));
            }
        }
        sprzatanie();
    }


    // =====================================================================
    // Ostatni etap: zaznaczanie wielu pozycji, edycja zbiorcza, formularz książki
    // =====================================================================
    void TestZbiorczychGUI(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        LogikaProjektow lp(m, logika);
        LogikaKatalogu lkat(m);
        LogikaKsiazek lks(m);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_pozycje_projektow WHERE id_projektu IN (SELECT id FROM tbl_projekty WHERE nazwa LIKE 'GUIZB\\_%')");
            m.wykonaj("DELETE FROM tbl_projekty WHERE nazwa LIKE 'GUIZB\\_%'");
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUIZB\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIZB\\_%'");
            m.wykonaj("DELETE FROM tbl_katalog WHERE symbol LIKE 'GUIZB\\_%'");
            m.wykonaj("DELETE FROM tbl_ksiazki WHERE tytul LIKE 'GUIZB %'");
        };
        sprzatanie();

        // --- Magazyn: zaznaczenie zakresu i usunięcie
        PanelElementow* pe = ZnajdzTyp<PanelElementow>(okno);
        CHECK(pe != nullptr);
        std::vector<long long> ide;
        for (int i = 1; i <= 4; ++i) {
            Element e; e.symbol = "GUIZB_E" + std::to_string(i); e.iloscDostepna = i;
            auto id = logika.zapiszElement(e);
            CHECK(id.has_value());
            if (id) ide.push_back(*id);
        }
        if (pe && ide.size() == 4) {
            pe->UstawKategorie(std::nullopt);
            pe->Szukaj("GUIZB_E");
            CHECK(pe->Elementy().size() == 4);
            pe->ZaznaczZakres(0, 2);
            CHECK(pe->LiczbaZaznaczonych() == 3);
            // edycja zbiorcza przez logikę z okna: dialog z polami
            {
                DialogEdycjiZbiorczej d(okno, logika, 3);
                                CHECK(d.Zmiany().pusta());
                d.UstawProducenta("ZbiorczyProd");
                d.UstawIloscMinimalna(6);
                d.UstawTagi("gui", true);
                const ZmianyZbiorcze z = d.Zmiany();
                CHECK(z.producent && *z.producent == "ZbiorczyProd" && z.iloscMinimalna && *z.iloscMinimalna == 6 && z.tagi && z.tagiDopisz);
                int zm = 0;
                CHECK(logika.zmienZbiorczo({ide[0], ide[1], ide[2]}, z, &zm) && zm == 3);
                auto e = logika.pobierzElement(ide[1]);
                CHECK(e && e->producent == "ZbiorczyProd" && e->iloscMinimalna == 6);
                auto e4 = logika.pobierzElement(ide[3]);
                CHECK(e4 && e4->producent != "ZbiorczyProd");
            }
            pe->Odswiez();
            pe->ZaznaczZakres(0, 2);
            CHECK(pe->UsunZaznaczone(true) == 3);
            pe->Szukaj("GUIZB_E");
            CHECK(pe->Elementy().size() == 1);
        }

        // --- Projekt: wiele pozycji naraz
        {
            Projekt p; p.nazwa = "GUIZB_Projekt";
            auto idp = lp.zapiszProjekt(p);
            CHECK(idp.has_value());
            std::vector<long long> idel;
            for (int i = 1; i <= 3; ++i) {
                Element e; e.symbol = "GUIZB_P" + std::to_string(i); e.iloscDostepna = 5;
                auto id = logika.zapiszElement(e);
                CHECK(id.has_value());
                if (id && idp) {
                    PozycjaProjektu poz; poz.idProjektu = *idp; poz.idElementu = *id; poz.oznaczenia = "R" + std::to_string(i); poz.wymagana = 1;
                    CHECK(lp.dodajPozycje(poz).has_value());
                }
            }
            PanelProjektow* pp = ZnajdzTyp<PanelProjektow>(okno);
            CHECK(pp != nullptr);
            if (pp && idp) {
                pp->Odswiez();
                pp->WybierzProjekt(*idp);
                pp->ZaznaczWszystkiePozycje();
                CHECK(pp->LiczbaZaznaczonychPozycji() == 3);
                CHECK(pp->UsunPozycje(true) == 3);
                std::vector<PozycjaProjektu> po;
                CHECK(lp.pobierzPozycje(*idp, po) && po.empty());
            }
        }

        // --- Katalog: zaznaczenie wielu i usunięcie; podgląd parametrów z indeksami
        {
            PanelKatalogu* pk = ZnajdzTyp<PanelKatalogu>(okno);
            CHECK(pk != nullptr);
            for (int i = 1; i <= 3; ++i) {
                PozycjaKatalogu k; k.symbol = "GUIZB_K" + std::to_string(i); k.producent = "T";
                k.parametry = {{"V_{CE(sat)}", "0,2 V", "reczny"}, {"Rezystancja", "10^3 om", "reczny"}};
                CHECK(lkat.zapisz(k).has_value());
            }
            if (pk) {
                okno->PokazKatalog();
                pk->Szukaj("GUIZB_K");
                CHECK(pk->Lista().size() == 3);
                pk->ZaznaczWszystkie();
                CHECK(pk->LiczbaZaznaczonych() == 3);
                CHECK(pk->Usun(true) == 3);
                CHECK(pk->Lista().empty());
                PozycjaKatalogu k; k.symbol = "GUIZB_K9"; k.producent = "T";
                k.parametry = {{"V_{CE(sat)}", "0,2 V", "reczny"}, {"hFE", "A: 110-220; B: 200-450", "reczny"}};
                auto id = lkat.zapisz(k);
                CHECK(id.has_value());
                pk->Szukaj("GUIZB_K9");
                if (id) pk->Wybierz(*id);
                const wxString html = pk->HtmlParametrow();
                CHECK(html.Contains("<sub>") && html.Contains("CE(sat)") && html.Contains("hFE"));
            }
        }

        // --- Literatura: zaznaczanie, usuwanie, zmiana zbiorcza
        PanelKsiazek* pks = ZnajdzTyp<PanelKsiazek>(okno);
        CHECK(pks != nullptr);
        if (pks) {
            for (int i = 1; i <= 3; ++i) {
                Ksiazka k; k.tytul = "GUIZB Ksiazka " + std::to_string(i); k.autor = "Stary"; k.tagi = "lampy";
                CHECK(lks.zapisz(k).has_value());
            }
            okno->PokazLiteratura();
            pks->Szukaj("GUIZB");
            CHECK(pks->Lista().size() == 3);
            pks->ZaznaczZakres(0, 1);
            CHECK(pks->LiczbaZaznaczonych() == 2 && pks->ZaznaczoneId().size() == 2);
            CHECK(pks->UsunZaznaczone(true) == 2);
            CHECK(pks->Lista().size() == 1);
            pks->ZaznaczWszystkie();
            CHECK(pks->UsunZaznaczone(true) == 1 && pks->Lista().empty());
        }

        // --- Formularz książki: tryb zbiorczy (pola indywidualne wyszarzone)
        {
            DialogKsiazki d(okno, Ksiazka(), 3);
            CHECK(d.TrybZbiorczy());
                        CHECK(!d.PoleTytulu()->IsEnabled() && !d.PoleRoku()->IsEnabled() && !d.PoleIsbn()->IsEnabled() &&
                  !d.PoleJezyka()->IsEnabled() && !d.PodgladUwag()->IsEnabled() && !d.PoleWydania()->IsEnabled());
            CHECK(d.PoleAutora()->IsEnabled() && d.PoleWydawcy()->IsEnabled() && d.PoleTagow()->IsEnabled());
            CHECK(d.Zmiany().pusta());
            d.PoleAutora()->SetValue("Nowy");
            CHECK(d.ZmianaAutora()->GetValue() && !d.ZmianaWydawcy()->GetValue());
            d.PoleTagow()->SetValue("audio");
            const ZmianyZbiorczeKsiazek z = d.Zmiany();
            CHECK(z.autor && *z.autor == "Nowy" && !z.wydawca && z.tagi && *z.tagi == "audio" && z.tagiDopisz);
        }
        // --- Formularz książki: rok z wklejonego tekstu, Język jako combo, odpowiedź BN, edytor uwag
        {
            DialogKsiazki d(okno, Ksiazka());
            CHECK(!d.TrybZbiorczy());
                        CHECK(d.WklejRok("wyd. 2, [2015 r.]") && d.PoleRoku()->GetValue() == "2015");
            CHECK(!d.WklejRok("brak liczby") && d.PoleRoku()->GetValue() == "2015");   // bez liczby nic się nie zmienia
            CHECK(d.WklejRok("  1999  ") && d.PoleRoku()->GetValue() == "1999");
            wxComboBox* jez = d.PoleJezyka();
            CHECK(jez != nullptr && jez->GetCount() >= 5 && jez->FindString("polski") != wxNOT_FOUND);
            const std::string json = R"({"bibs":[{"title":"Sztuka elektroniki /","author":"Horowitz, Paul.","publisher":"WKL","publicationYear":"2013","language":"polski","isbnIssn":"978-83-206-1500-0"}]})";
            CHECK(d.ZastosujOdpowiedzBN(json, false) == 1);
            CHECK(d.PoleTytulu()->GetValue() == "Sztuka elektroniki" && d.PoleAutora()->GetValue() == "Horowitz, Paul" &&
                  d.PoleRoku()->GetValue() == "2013" && d.PoleIsbn()->GetValue() == "9788320615000" && jez->GetValue() == "polski");
        }
        {
            DialogUwagHTML d(okno, "<b>Wazne</b> V<sub>CE</sub>\n<ul><li>jeden</li><li>dwa</li></ul>");
                        CHECK(d.HtmlPodgladu().Contains("<b>Wazne</b>") && d.HtmlPodgladu().Contains("<sub>CE</sub>"));
            d.UstawZrodlo("linia1\nlinia2");
            CHECK(d.HtmlPodgladu().Contains("<br>"));      // zwykły tekst dostaje <br> w miejscu nowych linii
            d.UstawZrodlo("abc");
            d.Otocz("<i>", "</i>");
            CHECK(d.Zrodlo().Contains("<i></i>"));
        }

        // --- Narzędzia: numer inwentarzowy widoczny w liście wyszukiwania
        {
            PanelNarzedzi* pn = ZnajdzTyp<PanelNarzedzi>(okno);
            CHECK(pn != nullptr);
        }
        sprzatanie();
    }


    // =====================================================================
    // Zapis danych do PDF (ExifTool) i OCR (ocrmypdf): z prawdziwymi programami, gdy są zainstalowane
    // =====================================================================
    void TestOcrIMetadanych(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaKsiazek lks(m);
        Konfiguracja& kfg = Konfiguracja::pobierz();
        const wxString kat = wxFileName::GetTempDir() + "/dm_guiocr";
        wxFileName::Mkdir(kat, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_ksiazki WHERE tytul LIKE 'GUIOC %'");
            for (const char* f : {"/a.pdf", "/a.pdf_original", "/s.pdf", "/s_ocr.pdf", "/s.pdf.bak", "/zepsuty.pdf"}) wxRemoveFile(kat + f);
        };
        sprzatanie();
        PanelKsiazek* pk = ZnajdzTyp<PanelKsiazek>(okno);
        CHECK(pk != nullptr);
        if (!pk) return;
        okno->PokazLiteratura();

        // zwykły PDF z jedną stroną tekstu
        {
            std::string pdf = "%PDF-1.4\n";
            std::vector<size_t> przes;
            const std::string tekst = "BT /F1 18 Tf 20 100 Td (Test zapisu) Tj ET";
            const std::string obj[] = {"<< /Type /Catalog /Pages 2 0 R >>", "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
                "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 300 200] /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>",
                "<< /Length " + std::to_string(tekst.size()) + " >>\nstream\n" + tekst + "\nendstream", "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"};
            for (size_t i = 0; i < 5; ++i) { przes.push_back(pdf.size()); pdf += std::to_string(i + 1) + " 0 obj\n" + obj[i] + "\nendobj\n"; }
            const size_t x = pdf.size();
            pdf += "xref\n0 6\n0000000000 65535 f \n";
            for (size_t o : przes) { char b[32]; snprintf(b, sizeof b, "%010zu 00000 n \n", o); pdf += b; }
            pdf += "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n" + std::to_string(x) + "\n%%EOF\n";
            wxFFile f(kat + "/a.pdf", "wb"); f.Write(pdf.data(), pdf.size());
        }
        {
            wxFFile f(kat + "/zepsuty.pdf", "wb"); f.Write("to nie jest pdf", 15);
        }
        const wxString skan = wxString::FromUTF8(env("DM_TEST_SKAN", "../tests/dane/skan_test.pdf"));
        const bool maSkan = wxFileExists(skan) && wxCopyFile(skan, kat + "/s.pdf", true);

        Ksiazka k; k.tytul = "GUIOC Urządzenia"; k.autor = "Miszczak, Stanisław"; k.wydawca = "WKŁ"; k.rok = 1964; k.wydanie = "wydanie drugie"; k.isbn = "9788320615000";
        k.jezyk = "polski"; k.tagi = "lampy, audio"; k.sciezka = (kat + "/a.pdf").utf8_string();
        auto id = lks.zapisz(k);
        Ksiazka s; s.tytul = "GUIOC Skan"; s.sciezka = (kat + "/s.pdf").utf8_string();
        auto ids = maSkan ? lks.zapisz(s) : std::nullopt;
        Ksiazka z; z.tytul = "GUIOC Zepsuty"; z.sciezka = (kat + "/zepsuty.pdf").utf8_string();
        auto idz = lks.zapisz(z);
        CHECK(id.has_value() && idz.has_value());
        pk->Szukaj("GUIOC");

        // brak programu: wyraźny brak działania, bez zawieszenia
        const std::string staryExif = kfg.pobierz("literatura", "exiftool", ""), staryOcr = kfg.pobierz("literatura", "ocrmypdf", "");
        kfg.ustaw("literatura", "exiftool", "/nie/ma/takiego/exiftool");
        kfg.ustaw("literatura", "ocrmypdf", "/nie/ma/takiego/ocrmypdf");
        if (id) pk->Wybierz(*id);
        CHECK(pk->ZapiszMetadaneDoPliku(true) == -1);
        CHECK(pk->Ocr(PanelKsiazek::TrybOcr::ObokOryginalu, true) == -1);
        if (staryExif.empty()) kfg.usun("literatura", "exiftool"); else kfg.ustaw("literatura", "exiftool", staryExif);
        if (staryOcr.empty()) kfg.usun("literatura", "ocrmypdf"); else kfg.ustaw("literatura", "ocrmypdf", staryOcr);
        const std::string ocrTest = env("DM_TEST_OCRMYPDF", "");
        if (!ocrTest.empty()) kfg.ustaw("literatura", "ocrmypdf", ocrTest);

        const bool maExif = !ProgramyZewnetrzne::Znajdz("exiftool", kfg.pobierz("literatura", "exiftool", "")).empty();
        const bool maOcr = !ProgramyZewnetrzne::Znajdz("ocrmypdf", kfg.pobierz("literatura", "ocrmypdf", "")).empty();
        std::cout << "ExifTool: " << (maExif ? "jest" : "BRAK (pomijam test zapisu)") << ", ocrmypdf: " << (maOcr ? "jest" : "BRAK (pomijam test OCR)") << "\n";

        if (maExif && id) {
            pk->Wybierz(*id);
            CHECK(pk->ZapiszMetadaneDoPliku(true) == 1);
            CHECK(wxFileExists(kat + "/a.pdf_original"));                        // kopia zapasowa ExifTool
            const auto d = MetadaneKsiazek::odczytaj((kat + "/a.pdf").utf8_string());   // odczyt z powrotem tym samym kodem co przy imporcie
            CHECK(d.tytul == "GUIOC Urządzenia" && d.autor == "Miszczak, Stanisław" && d.wydawca == "WKŁ");
            CHECK(d.wydanie == "wydanie drugie");
            CHECK(d.rok == 1964 && d.isbn == "9788320615000" && d.jezyk == "polski" && d.tagi == "lampy, audio");
            // plik uszkodzony: błąd zgłoszony, nie zawieszenie ani wyjątek
            if (idz) { pk->Wybierz(*idz); CHECK(pk->ZapiszMetadaneDoPliku(true) == 0); }
            // książka bez PDF: nic do zrobienia
            if (id) { pk->Wybierz(*id); }
        }
        if (maOcr && ids) {
            pk->Szukaj("GUIOC Skan");
            pk->Wybierz(*ids);
            CHECK(pk->Ocr(PanelKsiazek::TrybOcr::ObokOryginalu, true) == 1);
            CHECK(wxFileExists(kat + "/s_ocr.pdf") && wxFileExists(kat + "/s.pdf"));
            std::string bladPdf;
            const auto st = StronyPDF::pobierz((kat + "/s_ocr.pdf").utf8_string(), bladPdf);
            CHECK(!st.empty() && st[0].find("Lampy") != std::string::npos);
            pk->Szukaj("GUIOC Skan");
            pk->Wybierz(*ids);
            CHECK(pk->Ocr(PanelKsiazek::TrybOcr::ZastapOryginal, true) == 1);
            CHECK(wxFileExists(kat + "/s.pdf.bak") && !wxFileExists(kat + "/s_ocr.pdf"));
            auto o = lks.pobierzKsiazke(*ids);
            CHECK(o && o->strony > 0);                                           // indeks treści zbudowany po OCR
            std::vector<TrafienieKsiazki> tr;
            CHECK(lks.szukajWTresci("pentoda", *ids, tr) && tr.size() == 1);
            pk->Szukaj("GUIOC Zepsuty");
            if (idz) { pk->Wybierz(*idz); CHECK(pk->Ocr(PanelKsiazek::TrybOcr::ObokOryginalu, true) == 0); }   // błąd OCR nie przerywa programu
        }
        if (ocrTest.empty()) { if (staryOcr.empty()) kfg.usun("literatura", "ocrmypdf"); }
        else { if (staryOcr.empty()) kfg.usun("literatura", "ocrmypdf"); else kfg.ustaw("literatura", "ocrmypdf", staryOcr); }
        sprzatanie();
        wxRmdir(kat);
    }

    // =====================================================================
    // Literatura: menu podręczne, karta Info, pole Wydanie, uwagi jako HTML w formularzu
    // =====================================================================
    // =====================================================================
    // Katalog: menu podręczne (usuń z katalogu, OCR datasheetu)
    // =====================================================================
    void TestOcrDatasheetu(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaKatalogu lk(m);
        Konfiguracja& kfg = Konfiguracja::pobierz();
        PanelKatalogu* pk = ZnajdzTyp<PanelKatalogu>(okno);
        CHECK(pk != nullptr);
        if (!pk) return;
        const wxString kat = wxFileName::GetTempDir() + "/dm_guids";
        wxFileName::Mkdir(kat, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_katalog WHERE symbol LIKE 'GUIDS %'");
            for (const char* n : {"GUIDS_Skan.pdf", "GUIDS_Skan.pdf.bak", "GUIDS_Skan_ocr.pdf"}) wxRemoveFile(kat + "/" + n);
        };
        sprzatanie();
        auto us = kfg.pobierzUstawieniaAplikacji();
        const std::string staryKat = us.sciezkaPDFDatasheet;
        us.sciezkaPDFDatasheet = kat.utf8_string();
        kfg.ustawUstawieniaAplikacji(us);
        const std::string staryOcr = kfg.pobierz("literatura", "ocrmypdf", "");
        const std::string ocrTest = env("DM_TEST_OCRMYPDF", "");
        if (!ocrTest.empty()) kfg.ustaw("literatura", "ocrmypdf", ocrTest);
        const bool maOcr = !ProgramyZewnetrzne::Znajdz("ocrmypdf", kfg.pobierz("literatura", "ocrmypdf", "")).empty();

        PozycjaKatalogu a; a.symbol = "GUIDS Skan"; a.producent = "X";
        PozycjaKatalogu b; b.symbol = "GUIDS Bez pliku";
        const auto ida = lk.zapisz(a);
        const auto idb = lk.zapisz(b);
        CHECK(ida.has_value() && idb.has_value());
        const wxString skan = wxString::FromUTF8(env("DM_TEST_SKAN", "../tests/dane/skan_test.pdf"));
        const wxString plikDs = WyszukiwanieDatasheet::sciezkaPliku(kat, "GUIDS Skan");
        CHECK(wxCopyFile(skan, plikDs, true));
        pk->Szukaj("GUIDS");
        if (ida) pk->Wybierz(*ida);

        // menu podręczne: Usuń z katalogu + OCR (aktywny, bo datasheet jest pobrany)
        {
            auto menu = pk->MenuKontekstowe();
            CHECK(menu != nullptr);
            if (menu) {
                wxMenuItem* usun = menu->FindItemByPosition(0);
                wxMenuItem* ocr = menu->FindItemByPosition(2);
                CHECK(usun && usun->GetItemLabelText() == wxString::FromUTF8("Usuń z katalogu") && usun->IsEnabled());
                CHECK(ocr && ocr->GetItemLabelText() == wxString::FromUTF8("Wykonaj OCR datasheetu") && ocr->IsEnabled());
            }
        }
        // pozycja bez pobranego datasheetu: OCR nieaktywny i nic do zrobienia
        if (idb) {
            pk->Wybierz(*idb);
            auto menu = pk->MenuKontekstowe();
            CHECK(menu && !menu->FindItemByPosition(2)->IsEnabled());
            CHECK(pk->OcrDatasheetow(OcrPliku::Tryb::Obok, true) == -1);
        }
        // brak programu: wyraźny brak działania
        if (ida) {
            pk->Wybierz(*ida);
            const std::string chwilowy = kfg.pobierz("literatura", "ocrmypdf", "");
            kfg.ustaw("literatura", "ocrmypdf", "/nie/ma/takiego/ocrmypdf");
            CHECK(pk->OcrDatasheetow(OcrPliku::Tryb::Obok, true) == -1);
            if (chwilowy.empty()) kfg.usun("literatura", "ocrmypdf"); else kfg.ustaw("literatura", "ocrmypdf", chwilowy);
        }
        if (maOcr && ida) {
            pk->Wybierz(*ida);
            CHECK(pk->OcrDatasheetow(OcrPliku::Tryb::Obok, true) == 1);
            const wxString obok = kat + "/GUIDS_Skan_ocr.pdf";
            CHECK(wxFileExists(obok) && wxFileExists(plikDs));
            std::string bladPdf;
            const auto st = StronyPDF::pobierz(obok.utf8_string(), bladPdf);
            CHECK(!st.empty() && st[0].find("Lampy") != std::string::npos);
            pk->Wybierz(*ida);
            CHECK(pk->OcrDatasheetow(OcrPliku::Tryb::Zastap, true) == 1);
            CHECK(wxFileExists(plikDs + ".bak") && !wxFileExists(obok));
            const auto st2 = StronyPDF::pobierz(plikDs.utf8_string(), bladPdf);
            CHECK(!st2.empty() && st2[0].find("Lampy") != std::string::npos);   // datasheet ma teraz warstwę tekstową
        }
        // usuwanie z katalogu (to samo polecenie, co w menu)
        if (idb) { pk->Wybierz(*idb); CHECK(pk->Usun(true) == 1); CHECK(!lk.pobierzPozycje(*idb).has_value()); }

        if (staryOcr.empty()) kfg.usun("literatura", "ocrmypdf"); else kfg.ustaw("literatura", "ocrmypdf", staryOcr);
        us.sciezkaPDFDatasheet = staryKat;
        kfg.ustawUstawieniaAplikacji(us);
        sprzatanie();
        wxRmdir(kat);
    }

    // =====================================================================
    // Katalog: wyszukiwanie datasheetu w Mouserze (odpowiedzi z pliku, bez sieci)
    // =====================================================================
    // Drukarka Zebra: ZPL do pliku, przez gniazdo TCP (lokalny "serwer drukarki") i okno ustawień
    void TestZebry(OknoGlowne* okno, const std::vector<Element>& lista, long long id) {
        const wxString plik = wxFileName::GetTempDir() + "/gui_zebra.zpl";
        {
            wxLogNull cicho;
            wxRemoveFile(plik);
        }
        // zapis do pliku
        std::string blad;
        CHECK(WysylkaZPL::doPliku(plik.utf8_string(), "^XA^XZ", &blad));
        CHECK(!WysylkaZPL::doPliku("", "x", &blad) && !blad.empty());
        CHECK(!WysylkaZPL::doPliku("/nie/ma/takiego/katalogu/a.zpl", "x", &blad) && !blad.empty());
        CHECK(!WysylkaZPL::przezSystem("", "x", &blad));
        CHECK(!WysylkaZPL::przezSystem("/dev/nie_ma_takiego_urzadzenia", "x", &blad));
        CHECK(WysylkaZPL::trybZNazwy("system") == WysylkaZPL::Tryb::System && WysylkaZPL::trybZNazwy("coś") == WysylkaZPL::Tryb::Siec);

        // ustawienia w config.ini
        const UstawieniaZebry stare = UstawieniaZebry::wczytaj();
        UstawieniaZebry u;
        u.wysylka.tryb = WysylkaZPL::Tryb::Plik;
        u.wysylka.plik = plik.utf8_string();
        u.etykieta.szerMm = 80;
        u.etykieta.wysMm = 50;
        u.etykieta.dpi = 300;
        u.uzywaj = true;
        u.zapisz();
        const UstawieniaZebry w = UstawieniaZebry::wczytaj();
        CHECK(w.wysylka.tryb == WysylkaZPL::Tryb::Plik && w.etykieta.dpi == 300 && w.uzywaj && w.wysylka.port == 9100);
        {
            DialogEtykiet dlg(okno, lista, id);
            dlg.WybierzTylkoZaznaczony(true);
            CHECK(dlg.ZebraAktywna());
            const auto ez = dlg.EtykietyZebry();
            CHECK(ez.size() == 1 && ez[0].kod.size() == 13 && !ez[0].symbol.empty());
            const std::string zpl = dlg.PoleceniaZPL();
            CHECK(zpl.find("^XA") == 0 && zpl.find("^PW945") != std::string::npos && zpl.find("^FD" + ez[0].kod + "^FS") != std::string::npos);
            CHECK(dlg.DrukujZebra(true));
            wxFFile f(plik, "rb");
            wxString tresc;
            CHECK(f.IsOpened() && f.ReadAll(&tresc, wxConvUTF8));
            CHECK(tresc.utf8_string() == zpl);
            dlg.UstawZebra(false);
            CHECK(!dlg.ZebraAktywna());
        }
        // okno ustawień: odczyt i zapis pól, etykieta próbna do pliku
        {
            DialogZebra dz(okno, w);
            const UstawieniaZebry o = dz.Ustawienia();
            CHECK(o.wysylka.tryb == WysylkaZPL::Tryb::Plik && o.etykieta.dpi == 300 && o.etykieta.szerMm == 80 && o.etykieta.wysMm == 50);
            CHECK(dz.WyslijProbna());
            wxFFile f(plik, "rb");
            wxString tresc;
            CHECK(f.IsOpened() && f.ReadAll(&tresc, wxConvUTF8) && tresc.Contains("^GB") && tresc.Contains("80 x 50 mm, 300 dpi"));
            dz.UstawTryb(WysylkaZPL::Tryb::Siec);
            dz.PoleHosta()->SetValue(" 127.0.0.1 ");
            dz.WyborDpi()->SetSelection(0);
            const UstawieniaZebry o2 = dz.Ustawienia();
            CHECK(o2.wysylka.tryb == WysylkaZPL::Tryb::Siec && o2.wysylka.host == "127.0.0.1" && o2.etykieta.dpi == 203);
        }
#ifndef _WIN32
        // lokalny "serwer drukarki" na porcie 9100-podobnym: odbiera wszystko do zamknięcia połączenia
        {
            const int serwer = ::socket(AF_INET, SOCK_STREAM, 0);
            CHECK(serwer >= 0);
            sockaddr_in a{};
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            a.sin_port = 0;
            CHECK(::bind(serwer, reinterpret_cast<sockaddr*>(&a), sizeof a) == 0 && ::listen(serwer, 1) == 0);
            socklen_t dl = sizeof a;
            ::getsockname(serwer, reinterpret_cast<sockaddr*>(&a), &dl);
            const int port = ntohs(a.sin_port);
            std::string odebrane;
            std::thread watek([&] {
                const int k = ::accept(serwer, nullptr, nullptr);
                char b[4096];
                ssize_t n;
                while (k >= 0 && (n = ::recv(k, b, sizeof b, 0)) > 0) odebrane.append(b, static_cast<size_t>(n));
                if (k >= 0) ::close(k);
            });
            std::string dane;
            for (int i = 0; i < 400; ++i) dane += "^XA^FO10,10^FDlinia " + std::to_string(i) + "^FS^XZ\n";  // > 4 KB, kilka porcji
            std::string b2;
            const bool ok = WysylkaZPL::przezSiec("127.0.0.1", port, dane, 5, &b2);
            watek.join();
            ::close(serwer);
            CHECK(ok);
            CHECK(odebrane == dane);
            if (!ok) std::cerr << "przezSiec: " << b2 << "\n";
            // port, na którym nikt nie słucha: czytelny błąd
            std::string b3;
            CHECK(!WysylkaZPL::przezSiec("127.0.0.1", port, "x", 2, &b3));
            CHECK(b3.find("Nie można połączyć") != std::string::npos);
            CHECK(!WysylkaZPL::przezSiec("", 9100, "x", 2, &b3));
            CHECK(!WysylkaZPL::przezSiec("127.0.0.1", 0, "x", 2, &b3));
        }
#endif
        // przywrócenie ustawień sprzed testu
        stare.zapisz();
        {
            wxLogNull cicho;
            wxRemoveFile(plik);
        }
    }

    void TestDlawika(OknoGlowne* okno) {
        DialogDlawika d(okno);
        CHECK(d.Wynik("zwoje") == "892" && d.Wynik("vfe") == "343,12" && d.Wynik("lzw") == "21,27" && d.Wynik("opor_m") == "0,1469" && d.Wynik("zalecana") == "225,000" && d.Wynik("szczelina") == "0,000" && d.Wynik("szczelina_ecclab") == "0,335");
        CHECK(d.TrescOstrzezenia().find("mniej niż 50%") != wxString::npos);   // 21,7% < 50%
        CHECK(d.Drut().Contains("razem") && d.Drut().Contains("0,"));            // zestawienie drutu: długość, miedź, do zakupu
        d.UstawPole("L", "20");
        d.UstawKsztaltke("EI66");
        CHECK(d.TrescOstrzezenia().find("ponad 75%") != wxString::npos);       // za mały rdzeń
        d.UstawKsztaltke("EI102");
        d.UstawPole("L", "5");
        CHECK(d.Wynik("zwoje") == "892");
        d.UstawPole("a", "36");                                         // własny wymiar
        CHECK(d.Wynik("zwoje") != "892");
        d.UstawPole("I", "abc");
        CHECK(d.Wynik("zwoje").empty() && !d.Uwagi().empty() && d.Drut().empty());
        d.UstawPole("I", "300"); d.UstawPole("drut", "0,2");
        CHECK(d.Wynik("drut") == "0,20");
        d.Show(); d.Centre(); d.Raise(); wxYield(); wxMilliSleep(150); wxYield();
        ZrzutEkranu("dlawik");
        d.Hide();
    }

    void TestTrafoSieciowy(OknoGlowne* okno) {
        DialogTrafoSieciowy d(okno);
        CHECK(d.Wynik("moc_wtorna") == "41,3" && d.Wynik("zwoje1").size() > 0 && d.Tabela().Contains("pierwotne") && d.Tabela().Contains("wtórne 2"));
        CHECK(d.Drut().Contains("razem") && d.Drut().Contains("W2"));
        d.UstawUzwojenia("300 0,1\n6,3 1,8\n5 2");
        CHECK(d.Wynik("moc_wtorna") == "51,3" && d.Tabela().Contains("wtórne 3"));
        d.UstawUzwojenia("300");                                        // brak prądu
        CHECK(d.Wynik("zwoje1").empty() && !d.Uwagi().empty() && d.Drut().empty());
        d.UstawUzwojenia("300 0,1\n6,3 1,8");
        d.UstawPole("U1", "115");
        CHECK(!d.Wynik("zwoje1").empty());
        d.UstawKsztaltke("EI66");
        CHECK(d.Uwagi().Contains("większy rdzeń") || d.Uwagi().Contains("nie mieszczą"));
        d.UstawKsztaltke("EI102");
        d.Show(); d.Centre(); d.Raise(); wxYield(); wxMilliSleep(150); wxYield();
        ZrzutEkranu("trafo_sieciowy");
        d.Hide();
    }

    void TestTrafoSE(OknoGlowne* okno) {
        DialogTrafoSE d(okno);
        d.UstawPole("S", "10,5"); d.UstawPole("Fe", "17");
        CHECK(d.Wynik("n") == "20,616" && d.Wynik("n_porownanie") == "24,254" && d.Wynik("Li_porownanie") == "4,9020" && d.Wynik("Li") == "5,6172");
        d.UstawSpadek(1);
        CHECK(d.Wynik("Li") != "5,6172");
        d.UstawSpadek(3);
        CHECK(d.Wynik("Li") == "5,6172");
        CHECK(d.Wynik("z1") == "1035" && d.Wynik("z2") == "50" && d.Wynik("d1") == "0,241");
        CHECK(d.Ostrzezenie().Contains("klasie A"));
        d.UstawPole("P", "4");
        CHECK(!d.Ostrzezenie().Contains("klasie A"));
        d.UstawPole("P", "10");
        d.UstawKsztaltke("EI102");
        CHECK(d.PoleTekst("wymA") == "102" && d.PoleTekst("wyma") == "34" && d.PoleTekst("h") == "5,10");   // kształtka wypełnia wymiary blachy
        d.UstawPole("wymc", "20");
        CHECK(d.PoleTekst("a") == "2,00");
        d.UstawKsztaltke("EI102");
        CHECK(d.PoleTekst("wymc") == "17" && d.PoleTekst("a") == "1,70");
        CHECK(!d.Wynik("szczelina").empty() && !d.Wynik("cy_z1").empty() && !d.Wynik("cy_szczelina").empty());
        CHECK(d.WykonaneDostepne() && !d.Wynik("cy_az").empty());
        CHECK(d.Wynik("kr_z23") == "1651" && d.Wynik("kr_z24") == "1239" && d.Wynik("wi_q").Contains("9,5"));
        CHECK(!d.Wynik("kr_szczelina").empty() && !d.Wynik("kr_d1").empty() && d.Wynik("kr_iloczyn").Contains("/"));
        d.UstawA(3);
        CHECK(d.Wynik("kr_iloczyn").Contains("50,0"));
        d.UstawA(0);
        d.UstawPole("Rg", "0");
        CHECK(d.Wynik("Li").empty());
        d.UstawPole("Rg", "4");
        d.Show(); d.Centre(); d.Raise(); wxYield(); wxMilliSleep(150); wxYield();
        ZrzutEkranu("trafo_se");
        d.Hide();
        // uzwojenia wykonane
        d.UstawKsztaltke("EI102");
        d.UstawPole("S", "17,34"); d.UstawPole("Fe", "18,7");
        DialogTrafoSEWykonane* e = d.OtworzWykonane();
        CHECK(e != nullptr);
        if (e) {
            CHECK(!e->Wynik("li").empty() && !e->Wynik("rp").empty() && e->Tabela().Contains("P1") && e->Tabela().Contains("W1"));
            CHECK(e->Drut().Contains("razem") && e->Drut().Contains("P1"));
            CHECK(e->Model() && !e->Model()->Wynik("lp").empty() && !e->Model()->Wynik("bdc").empty() && !e->Model()->Wynik("fd").empty() && e->Model()->PunktowKrzywej() > 50);
            const wxString lp1 = e->Model()->Wynik("lp");
            e->Model()->UstawPole("mu", "1000");
            CHECK(e->Model()->Wynik("lp") != lp1);
            e->Model()->UstawPole("mu", "3000");
            e->UstawUklad(1);
            CHECK(e->Tabela().Contains("P2") && e->Wynik("lr_ukl").Contains("/"));
            e->UstawUklad(2);
            CHECK(e->Tabela().Contains("W2"));
            e->UstawPole("z2", "0");
            CHECK(e->Wynik("li").empty() && !e->Uwagi().empty());
            e->UstawPole("z2", "50");
            e->UstawUklad(0);
            e->Show(); e->Centre(); e->Raise(); wxYield(); wxMilliSleep(150); wxYield();
            ZrzutEkranu("trafo_se_wykonane");
            e->Destroy();
        }
    }

    void TestCewkiAL(OknoGlowne* okno) {
        DialogCewkiAL d(okno);
        CHECK(d.Wynik("n") == "331,66" && d.Wynik("zwoje") == "332" && d.Wynik("L") == "771,568" && d.Wynik("blad") == "0,20");
        d.UstawPole("sprawdz", "320");
        CHECK(d.Wynik("L_sprawdz") == "716,800" && d.Wynik("blad_sprawdz") == "-6,91" && d.Uwagi().Contains("716,8"));
        d.UstawPole("tol", "25");
        CHECK(d.Wynik("zakres").Contains("...") && d.Wynik("B").empty());
        d.UstawPole("Ae", "100"); d.UstawPole("I", "0,1");
        CHECK(!d.Wynik("B").empty());
        d.UstawJednostke(2);
        CHECK(d.Wynik("zwoje") != "332");
        d.UstawJednostke(1);
        d.UstawPole("AL", "0");
        CHECK(d.Wynik("zwoje").empty() && !d.Uwagi().empty());
        d.UstawPole("AL", "7000");
        d.Show(); d.Centre(); d.Raise(); wxYield(); wxMilliSleep(150); wxYield();
        ZrzutEkranu("cewka_al");
        d.Hide();
    }

    void TestTrafoPP(OknoGlowne* okno) {
        DialogTrafoPP d(okno);
        CHECK(d.Wynik("Li") == "11,0284" && d.Wynik("Lr") == "0,1464" && d.Wynik("n") == "19,558" && d.Wynik("iloczyn") == "417,08");
        d.UstawSpadek(2);
        CHECK(d.Wynik("Li") != "11,0284" && d.Wynik("Lr") != "0,1464");
        d.UstawSpadek(3);
        CHECK(d.Wynik("Li") == "11,0284" && d.Wynik("Lr") == "0,1464");
        d.UstawPole("P0", "50"); d.UstawPole("Fd", "100"); d.UstawPole("Fg", "15000"); d.UstawPole("I0", "32");
        CHECK(d.Wynik("Li") == "4,4114" && d.Wynik("Lr") == "0,1952" && d.Wynik("przekroj") == "14,441");
        d.UstawPole("S", "17,34"); d.UstawPole("Fe", "18,7");
        CHECK(d.Wynik("z2") == "46,9" || d.Wynik("z2").StartsWith("46,89"));
        CHECK(d.Wynik("d1") == "0,319" && d.Wynik("d2") == "1,2616");
        CHECK(!d.Wynik("g_L1").empty() && !d.Wynik("g_z1").empty() && !d.Wynik("g_d1").empty());
        // lampa z katalogu: Ri i I0 z jej parametrów, ręczna zmiana wraca do „wpisz ręcznie”
        CHECK(d.LiczbaLamp() >= 8);
        for (const char* sym : {"EL34", "6P14P", "EL84", "6V6", "6L6", "ECL86", "ECL82", "EBL21"}) CHECK(d.UstawLampe(sym));
        CHECK(d.UstawLampe("EL34") && d.PoleTekst("Ri") == "17000" && d.PoleTekst("I0") == "45" && d.PoleTekst("Raa") == "6600");
        CHECK(d.UstawLampe("EL84") && d.PoleTekst("Ri") == "38000" && d.PoleTekst("I0") == "25" && d.PoleTekst("Raa") == "8000" && d.WybranaLampa() == "EL84");
        d.UstawPole("Ri", "12000");
        CHECK(d.WybranaLampa().empty());
        d.UstawPole("Ri", "15000"); d.UstawPole("I0", "32"); d.UstawPole("Raa", "3400");   // jak przed sprawdzeniem lamp
        d.UstawTyp(1);
        CHECK(d.PoleTekst("Fd") == "50" && d.PoleTekst("Fg") == "12000" && d.PoleTekst("B") == "8000");   // gitarowy: pasmo 50 Hz - 12 kHz
        CHECK(d.Wynik("przekroj").size() > 0);
        d.UstawPole("R0", "0");
        CHECK(d.Wynik("Li").empty());
        d.UstawPole("R0", "8");
        d.UstawKsztaltke("EI102");
        CHECK(!d.Wynik("z1").empty());
        // wymiary blachy wypełniane kształtką, ale edytowalne: zmiana h przelicza pola pochodne
        CHECK(d.PoleTekst("wymA") == "102" && d.PoleTekst("wymh") == "51" && d.PoleTekst("h") == "5,10");
        d.UstawPole("wymh", "60");
        CHECK(d.PoleTekst("h") == "6,00" && d.PoleTekst("S") == "17,34" && !d.Wynik("z1").empty());
        d.UstawKsztaltke("EI102");
        CHECK(d.PoleTekst("wymh") == "51" && d.PoleTekst("h") == "5,10");
        d.Show(); d.Centre(); d.Raise(); wxYield(); wxMilliSleep(150); wxYield();
        ZrzutEkranu("trafo_pp");
        d.Hide();
        // Etap 3
        d.UstawTyp(0);
        d.UstawPole("P0", "50"); d.UstawPole("Fd", "100"); d.UstawPole("Fg", "15000"); d.UstawPole("I0", "32");
        d.UstawKsztaltke("EI102");
        CHECK(d.Etap3Dostepny());
        DialogTrafoPPEtap3* e = d.OtworzEtap3();
        CHECK(e != nullptr);
        if (e) {
            CHECK(e->Wynik("raa") != "" && e->Tabela().Contains("A1") && e->Tabela().Contains("W1"));
            CHECK(e->Drut().Contains("razem"));
            e->UstawPole("z1", "917"); e->UstawPole("z2", "47"); e->UstawPole("d1", "0,3"); e->UstawPole("d2", "1,3");
            CHECK(e->Wynik("raa") == "3383,69" && e->Wynik("rw_ecclab").size() > 0 && e->Wynik("li_zad") == "4,4114");
            CHECK(e->Model() && !e->Model()->Wynik("lp").empty() && !e->Model()->Wynik("fd").empty() && !e->Model()->Wynik("fg").empty() && e->Model()->PunktowKrzywej() > 50);
            const wxString fgNieprz = e->Model()->Wynik("fg");
            e->UstawUklad(1);
            CHECK(e->Model()->Wynik("fg") != fgNieprz);
            e->Model()->UstawPole("mu", "x");
            CHECK(e->Model()->Wynik("lp").empty());
            e->Model()->UstawPole("mu", "3000");
            CHECK(e->Tabela().Contains("A2") && e->Uwagi().Contains("nieparzystą"));
            e->UstawPole("z2", "0");
            CHECK(e->Wynik("raa").empty() && !e->Uwagi().empty());
            e->UstawPole("z2", "47");
            e->UstawUklad(3);
            CHECK(e->Sekcje().Contains("|") && e->Tabela().Contains("A2") && !e->Wynik("lr").empty() && !e->Model()->Wynik("fg").empty());
            e->UstawPrzegrode("100");
            CHECK(e->Wynik("lr").empty() && !e->Uwagi().empty());
            e->UstawPrzegrode("2");
            e->UstawSekcje("A1 600\nW1 20\nA2 317\nW2 27\n");           // tabela własna: suma A = 917, suma W = 47
            CHECK(e->Wynik("lr") != "" && e->Tabela().Contains("A2") && e->Tabela().Contains("W2"));
            e->UstawSekcje("A1 600\nW1 20\nQ 1\n");
            CHECK(e->Wynik("lr").empty() && !e->Uwagi().empty());
            e->UstawUklad(2);
            CHECK(e->Tabela().Contains("A3") && e->Tabela().Contains("W2"));
            e->UstawWtorne(true);
            CHECK(!e->Uwagi().empty());           // sekcje W różne (nierówne) przy połączeniu równoległym
            e->UstawWtorne(false);
            {   // Mesa Boogie DR: jeden układ, odczepy 4, 8 i 16 Ω
                e->UstawPole("z1", "1696"); e->UstawPole("d1", "0,3"); e->UstawPole("d2", "0,4"); e->UstawPole("z2", "78");
                e->UstawUklad(5);
                CHECK(e->Sekcje().Contains("g1/1/1") && e->Sekcje().Contains(" e2") && e->Tabela().Contains("W6") && e->Tabela().Contains("A4"));
                CHECK(e->Tabela().Contains("Odczepy") && e->Tabela().Contains("16 Ω"));
                wxString rw[3];
                for (int k = 0; k < 3; k++) {
                    e->UstawOdczep(k);
                    CHECK(!e->Wynik("lr").empty() && !e->Wynik("rw").empty());
                    rw[k] = e->Wynik("rw");
                }
                CHECK(rw[0] != rw[2]);                                  // inny odczep = inne sekcje czynne
                e->UstawPole("z1", "917"); e->UstawPole("z2", "47"); e->UstawPole("d1", "0,3"); e->UstawPole("d2", "1,3");
            }
            e->UstawUklad(0);
            e->Show(); e->Centre(); e->Raise(); wxYield(); wxMilliSleep(150); wxYield();
            ZrzutEkranu("trafo_pp_etap3");
            e->Destroy();
        }
    }

    void TestZnieksztalcen(OknoGlowne* okno) {
        DialogZnieksztalcen d(okno);
        CHECK(!d.WykresOdcinki() && d.OpisWykresu().Contains("Ia max"));   // rysunek zależy od wybranej opcji
        d.UstawTryb(true);
        CHECK(d.WykresOdcinki() && d.OpisWykresu().Contains("a = d+e"));
        d.UstawTryb(false);
        CHECK(d.Wynik("h2") == "10,112" && d.Wynik("h3") == "3,371" && d.Wynik("h4") == "0,749" && d.Wynik("h") == "10,686");
        CHECK(d.Wynik("h2_db") == "-19,9");
        d.UstawPole("imax", "x");
        CHECK(d.Wynik("h").empty() && !d.Uwagi().empty());
        d.UstawPole("imax", "93");
        d.UstawTryb(true);
        CHECK(d.Wynik("h2") == "10,112" || !d.Wynik("h2").empty());
        d.UstawPole("a", "26,5"); d.UstawPole("b", "26,5"); d.UstawPole("c", "26,5");
        d.UstawPole("d", "1"); d.UstawPole("e", "1"); d.UstawPole("f", "1"); d.UstawPole("g", "1");
        CHECK(d.Wynik("h2") == "0" && d.Wynik("h3") == "0" && d.Wynik("h4") == "0");
        d.Wyczysc();
        CHECK(d.Wynik("h").empty() && !d.Uwagi().empty());
    }

    void TestMetodGraficznych(OknoGlowne* okno) {
        DialogMetodyGraficzne d(okno);
        wxImage obraz(1000, 800);
        obraz.SetRGB(wxRect(0, 0, 1000, 800), 255, 255, 255);
        CHECK(!d.Skaluj("500", "300") && !d.Skalibrowany());               // brak obrazu i punktów
        CHECK(d.UstawObraz(obraz));
        CHECK(!d.UstawObraz(wxImage()));
        CHECK(!d.Skaluj("500", "300"));                                   // brak punktów
        d.UstawPunktKalibracji(0, 100, 700);
        d.UstawPunktKalibracji(1, 900, 700);
        d.UstawPunktKalibracji(2, 100, 100);
        CHECK(!d.Skaluj("0", "300") && !d.Skaluj("500", "abc"));
        CHECK(d.Skaluj("500", "300") && d.Skalibrowany());
        CHECK(d.OpisSkali().Contains("0,625") && d.OpisSkali().Contains("0,5"));
        d.PokazStrone(1);
        // prosta początkowa: 60% osi prądu (350 mA) i 60% osi napięcia (562,5 V)
        CHECK(d.Wynik("du") == "337,50" && d.Wynik("di") == "210,00");
        CHECK(d.Wynik("ra") == "1,61" && d.Wynik("raa") == "6,43" && d.Wynik("moc_se") == "8,86" && d.Wynik("moc_pp") == "35,44");
        // przesunięcie końców: 235,7 mA na osi prądu, 419,6 V i 0,9 mA na osi napięcia (jak w ECCLab)
        d.UstawKonieczPradu(0, 235.7);
        d.UstawKoniecNapiecia(419.6, 0.9);
        CHECK(d.Wynik("du") == "419,60" && d.Wynik("di") == "234,80" && d.Wynik("ra") == "1,79");
        // koniec napięciowy nie może wejść na lewo od prądowego
        d.UstawKonieczPradu(100, 235.7);
        d.UstawKoniecNapiecia(50, 0.9);
        CHECK(d.Prosta().p2.u >= d.Prosta().p1.u - 1e-9);
        // blokada Ra
        d.UstawKonieczPradu(0, 200);
        d.UstawKoniecNapiecia(400, 0);
        d.UstawBlokady(true, false);
        d.UstawKonieczPradu(0, 150);
        CHECK(d.Wynik("ra") == "2,00" && d.Wynik("di") == "150,00");
        d.UstawBlokady(false, true);
        d.UstawKonieczPradu(0, 180);
        CHECK(d.Wynik("di") == "150,00");
        d.UstawBlokady(false, false);
        // zniekształcenia z dwukliku: prosta 0 V/200 mA - 400 V/0 mA, punkty co 100 V (Ia1, Ia0, Ia2 = 150, 100, 50 mA)
        d.UstawKonieczPradu(0, 200);
        d.UstawKoniecNapiecia(400, 0);
        CHECK(d.Wynik("zn_h").empty());
        d.Dwuklik(100); d.Dwuklik(200); d.Dwuklik(300);
        CHECK(d.Wynik("ia1") == "150,000" && d.Wynik("ia0") == "100,000" && d.Wynik("ia2") == "50,000");
        CHECK(d.Wynik("zn_h") == "0,000" && d.Wynik("zn_h2") == "0,000");          // prosta liniowa: bez zniekształceń
        d.Dwuklik(100);                                                    // znów Ia1 (kolejność kołowa)
        d.Dwuklik(-50);                                                    // na lewo od prostej: ograniczone do jej początku
        CHECK(d.Wynik("ia0") == "200,000" && !d.Wynik("zn_h2").empty() && d.Wynik("zn_h2") != "0,000");
        // krzywa Pa max i siatka kontrolna
        CHECK(d.PunktowPaMax() == 0);
        d.UstawPaMax("25");
        CHECK(d.PunktowPaMax() == 200);
        d.UstawPaMax("abc");
        CHECK(d.PunktowPaMax() == 0);
        d.UstawSiatke("50", "60");
        d.UstawPaMax("25");
        d.Show(); d.Centre(); d.Raise(); wxYield();
        wxYield(); wxMilliSleep(150); wxYield();
        ZrzutEkranu("grafika_wyniki");
        d.PokazStrone(0);
        wxYield(); wxMilliSleep(100); wxYield();
        ZrzutEkranu("grafika_skalowanie");
        d.Hide();
    }

    void TestAudioLampy(OknoGlowne* okno) {
        DialogAudioLampy d(okno);
        CHECK(d.LiczbaStron() == 7);
        // polaryzacja katodowa: Ia = 1,2 mA, Uk = 1,8 V -> Rk = 1,5 kΩ, Ck 8,2 µF (E12: 10 µF)
        d.UstawPolaKatody({"1,2", "0", "1,8", "1,6", "40"});
        CHECK(d.WynikKatody().Contains("1,5 k") && d.WynikKatody().Contains("Ck dla spadku 3 dB"));
        d.UstawPolaKatody({"x", "0", "1,8", "1,6", "40"});
        CHECK(d.WynikKatody().Contains("Wpisz Ia"));
        // kondensator sprzęgający: Ra 100k, ra 62k, Rg 470k, Cs 22n -> fc ≈ 14,2 Hz
        d.UstawPolaSprzezenia({"100k", "62k", "470k", "22n", "20"}, 1);
        CHECK(d.WynikSprzezenia().Contains("14,2") && d.WynikSprzezenia().Contains("Cs dla fd"));
        d.UstawPolaSprzezenia({"100k", "62k", "0", "22n", "20"}, 1);
        CHECK(d.WynikSprzezenia().Contains("muszą być dodatnie"));
        // regulacja barwy: wybór układu pokazuje tylko jego elementy i potencjometry
        CHECK(d.LiczbaUkladowBarwy() == 14);
        d.UstawBarwe(0, 0.5, 0.5, 0.5);   // Fender Bassman 5F6-A (potencjometry log A: bas i wysokie)
        CHECK(d.PunktowBarwy() > 50 && d.LiczbaPotBarwy() == 3 && d.NazwaPotBarwy(1) == "Bas" && d.WynikBarwy().Contains("1 kHz -16,9"));
        d.UstawTaperPotBarwy(1, 0);
        d.UstawTaperPotBarwy(3, 0);       // liniowe -> inny przebieg
        CHECK(!d.WynikBarwy().Contains("1 kHz -16,9") && d.WynikBarwy().Contains("Najgłębsze wycięcie"));
        d.UstawBarwe(3, 0.3, 0.5, 0.9);   // Vox: tylko bas i wysokie
        CHECK(d.LiczbaPotBarwy() == 2 && d.NazwaPotBarwy(2).empty() && d.NazwaPotBarwy(3) == "Wysokie" && d.PunktowBarwy() > 50);
        d.UstawBarwe(11, 0.5, 0.5, 0.5);  // Marshall 18 W: jeden potencjometr „Barwa”
        CHECK(d.LiczbaPotBarwy() == 1 && d.NazwaPotBarwy(2) == "Barwa");
        d.UstawBarwe(7, 0.5, 0.5, 0.5);   // Bench: z cewkami
        CHECK(d.PunktowBarwy() > 50 && d.LiczbaPotBarwy() == 3 && !d.WynikBarwy().Contains("Wpisz"));
        d.UstawPolaLampy({"250", "100k", "0,8", "-1,5", "", "", "100", "62k5", "1", "20"});
        CHECK(d.WynikPunktuPracy().Contains("Rk = ") && d.WynikPunktuPracy().Contains("Kondensator katodowy"));
        d.UstawPolaLampy({"250", "100k", "0,8", "1,5"});
        CHECK(d.WynikPunktuPracy().Contains("Niemo"));
        d.UstawPolaLampy({"250", "100k", "0,8", "-1,5"});
        // układy lampowe: domyślne pola to przykłady z ECCLab
        d.UstawUklad(0);
        CHECK(d.ElementySchematu().Contains("Rk") && d.SchematMaEtykiete("Cs"));
        d.UstawPolaryzacjeAutomatyczna(false);
        CHECK(!d.SchematMaEtykiete("Rk"));
        d.UstawPolaryzacjeAutomatyczna(true);
        d.UstawUklad(2);
        CHECK(d.ElementySchematu().Contains("R") && d.SchematMaEtykiete("Rs"));
        d.UstawUklad(0);
        CHECK(d.LiczbaPolUkladu() == 10 && d.LiczbaWynikowUkladu() == 4);
        CHECK(d.WynikUkladu(0) == "1,38" && d.WynikUkladu(2) == "42,85" && d.WynikUkladu(3) == "47,12");
        d.UstawPolaryzacjeAutomatyczna(false);
        CHECK(d.WynikUkladu(2) == "56,88" && d.UwagiUkladu().empty());
        d.UstawPolaryzacjeAutomatyczna(true);
        CHECK(!d.UwagiUkladu().empty());
        d.UstawUklad(1);
        CHECK(d.WynikUkladu(0) == "470" && d.WynikUkladu(1) == "0,177" && d.WynikUkladu(2) == "0,703");
        d.UstawUklad(2);
        CHECK(d.WynikUkladu(0) == "14542,75" && d.WynikUkladu(4) == "264,998");
        d.UstawUklad(3);
        CHECK(d.WynikUkladu(0) == "1,1356" && d.WynikUkladu(2) == "28,149");
        for (int nr = 4; nr <= 8; nr++) {
            d.UstawUklad(nr);
            CHECK(d.LiczbaPolUkladu() > 3 && d.LiczbaWynikowUkladu() >= 3);
            for (int i = 0; i < d.LiczbaWynikowUkladu(); i++) CHECK(!d.WynikUkladu(i).empty());
            CHECK(d.UwagiUkladu().empty());
        }
        d.Show(); d.Centre(); d.Raise(); wxYield();
        for (int nr = 0; nr <= 8; nr++) {
            d.UstawUklad(nr);
            wxYield(); wxMilliSleep(150); wxYield();
            ZrzutEkranu(wxString::Format("audio_uklad_%d", nr));
        }
        d.Hide();
        // lampy w katalogu
        d.UstawUklad(0);
        {
            ManagerBazyDanych::pobierz().wykonaj("DELETE FROM tbl_katalog WHERE symbol IN ('ECC81','ECC82','ECC83','ECC85','ECC88','TESTL_X')");
            CHECK(!d.LampaZKatalogu("ECC83"));
            CHECK(d.DodajLampyPrzykladowe() == 5 && d.DodajLampyPrzykladowe() == 0);
            CHECK(d.LampaZKatalogu("ecc88"));
            CHECK(d.LampaDlaPunktuPracy("ecc88") && !d.LampaDlaPunktuPracy("NIEMA_TAKIEJ") && d.WynikPunktuPracy().Contains("Wzmocnienie napięciowe: 32,"));
            auto l = d.DaneLampyZPol();
            CHECK(l && std::abs(l->sa - 12.5) < 1e-9 && std::abs(l->ka - 33) < 1e-9 && std::abs(l->rwew - 2.6) < 1e-9);
            CHECK(l && l->csk && std::abs(*l->csk - 3.1) < 1e-9);
            CHECK(d.WynikUkladu(2).Contains(",") || !d.WynikUkladu(2).empty());
            d.UstawPoleUkladu(7, "40"); d.UstawPoleUkladu(8, "30"); d.UstawPoleUkladu(9, "20");
            CHECK(d.ZapiszLampeDoKatalogu("TESTL_X"));
            d.UstawPoleUkladu(7, "1"); d.UstawPoleUkladu(8, "1"); d.UstawPoleUkladu(9, "1");
            CHECK(d.LampaZKatalogu("TESTL_X"));
            auto l2 = d.DaneLampyZPol();
            CHECK(l2 && std::abs(l2->sa - 40) < 1e-9 && std::abs(l2->ka - 30) < 1e-9 && std::abs(l2->rwew - 20) < 1e-9);
            d.UstawPoleUkladu(8, "x");
            CHECK(!d.DaneLampyZPol() && !d.ZapiszLampeDoKatalogu("TESTL_Y"));
            ManagerBazyDanych::pobierz().wykonaj("DELETE FROM tbl_katalog WHERE symbol IN ('ECC81','ECC82','ECC83','ECC85','ECC88','TESTL_X')");
        }
        d.UstawUklad(3);
        d.UstawPoleUkladu(0, "abc");
        CHECK(d.WynikUkladu(0).empty() && !d.UwagiUkladu().empty());
        d.UstawUklad(0);
        d.UstawNapiecia("100", "50");
        CHECK(d.OpisNapiec().Contains("6,021 dB") && d.OpisNapiec().Contains("0,6931 Np"));
        d.UstawNapiecia("0", "50");
        CHECK(d.OpisNapiec().Contains("większe od zera"));
        d.UstawMoce("100", "1");
        CHECK(d.OpisMocy().Contains("20 dB"));
        d.UstawMiare("2", true);
        CHECK(d.OpisMiary().Contains("3,01 dB"));
        d.UstawMiare("2", false);
        CHECK(d.OpisMiary().Contains("6,021 dB"));
        d.UstawDb("20", false);
        CHECK(d.OpisDb().Contains("10"));
        d.UstawDb("20", true);
        CHECK(d.OpisDb().Contains("100"));
        d.UstawNepery("1");
        CHECK(d.OpisNeperow().Contains("8,6859 dB"));
        d.UstawDbNaNepery("8,686");
        CHECK(d.OpisDbNaNepery().Contains("1 Np") || d.OpisDbNaNepery().Contains("0,99999 Np"));
        d.PokazStrone(1);
        d.UstawRIAA(0, false, "20");
        CHECK(d.OpisPunktuRIAA().Contains("+19,27 dB") && d.OpisPunktuRIAA().Contains(wxString::FromUTF8("\xC2\xB0")) && d.OpisPunktuRIAA().Contains("1 kHz"));
        d.UstawRIAA(0, false, "20k");
        CHECK(d.OpisPunktuRIAA().Contains("-19,62 dB"));
        d.UstawRIAA(1, false, "20k");
        CHECK(d.OpisPunktuRIAA().Contains("+19,62 dB"));
        d.UstawRIAA(0, true, "20");
        CHECK(d.OpisPunktuRIAA().Contains("+") && !d.OpisPunktuRIAA().Contains("19,27"));
        d.UstawRIAA(0, false, "1k");
        d.UstawZakresRIAA("0", "100", "50");
        CHECK(d.LiczbaWierszyRIAA() == 3 && d.KomorkaRIAA(2, 0) == "100" && d.KomorkaRIAA(2, 1).Contains("13,09"));
        d.UstawZakresRIAA("10", "5", "1");
        CHECK(d.LiczbaWierszyRIAA() == 0);
        CHECK(d.PunktowWykresu() > 30);
        d.UstawSiecRIAA("100k", "10k", 4, 2);
        d.Show();
        wxYield();
        d.Centre(); d.Raise(); PrzerwaNaRysowanie(600); ZrzutEkranu("audio_riaa");
        d.PokazStrone(0);
        PrzerwaNaRysowanie(600); ZrzutEkranu("audio_db");
        d.PokazStrone(3);
        PrzerwaNaRysowanie(600); ZrzutEkranu("audio_lampa");
        d.PokazStrone(1);
        CHECK(d.OpisSieciRIAA().Contains("R1 = 900") && d.OpisSieciRIAA().Contains("dB"));
        d.UstawSiecRIAA("", "10k", 4, 2);
        CHECK(d.OpisSieciRIAA().Contains("Wpisz R2"));
    }

    void TestSortowaniaList(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaKatalogu lk(m);
        // porównanie tekstów: wielkość liter, liczby naturalnie, polskie litery
        CHECK(SortowanieList::PorownajTekst("R2", "R10") < 0);
        CHECK(SortowanieList::PorownajTekst("abc", "ABD") < 0);
        CHECK(SortowanieList::PorownajTekst("b", "B") == 0);
        CHECK(SortowanieList::PorownajTekst("z", "\xC5\xBC") < 0);       // z < ż
        CHECK(SortowanieList::PorownajTekst("\xC5\x82", "m") < 0);       // ł < m
        CHECK(SortowanieList::PorownajTekst("l", "\xC5\x82") < 0);       // l < ł
        PanelKatalogu* pk = ZnajdzTyp<PanelKatalogu>(okno);
        CHECK(pk != nullptr);
        if (!pk) return;
        m.wykonaj("DELETE FROM tbl_katalog WHERE symbol LIKE 'GUISORT\\_%'");
        for (const char* s : {"GUISORT_B10", "GUISORT_A", "GUISORT_B2"}) {
            PozycjaKatalogu p; p.symbol = s; p.producent = "X";
            CHECK(lk.zapisz(p).has_value());
        }
        pk->Szukaj("GUISORT_");
        CHECK((pk->Lista().size()) == static_cast<size_t>(3));
        pk->SortujWedlug(0);
        if (pk->Lista().size() == 3) {
            CHECK((pk->Lista()[0].symbol) == std::string("GUISORT_A"));
            CHECK((pk->Lista()[1].symbol) == std::string("GUISORT_B2"));
            CHECK((pk->Lista()[2].symbol) == std::string("GUISORT_B10"));
        }
        CHECK(pk->NaglowekKolumny(0).EndsWith(wxString::FromUTF8("\xE2\x96\xB2")));
        pk->SortujWedlug(0);  // ponowne kliknięcie odwraca kierunek
        if (pk->Lista().size() == 3) CHECK((pk->Lista()[0].symbol) == std::string("GUISORT_B10"));
        CHECK(pk->NaglowekKolumny(0).EndsWith(wxString::FromUTF8("\xE2\x96\xBC")));
        pk->SortujWedlug(1);  // inna kolumna: strzałka znika z poprzedniej
        CHECK(!pk->NaglowekKolumny(0).Contains(wxString::FromUTF8("\xE2\x96\xB2")) &&
              !pk->NaglowekKolumny(0).Contains(wxString::FromUTF8("\xE2\x96\xBC")));
        CHECK(pk->NaglowekKolumny(1).EndsWith(wxString::FromUTF8("\xE2\x96\xB2")));
        pk->Szukaj("");
        m.wykonaj("DELETE FROM tbl_katalog WHERE symbol LIKE 'GUISORT\\_%'");
        pk->Odswiez();
    }

    void TestMousera(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaKatalogu lk(m);
        PanelKatalogu* pk = ZnajdzTyp<PanelKatalogu>(okno);
        CHECK(pk != nullptr);
        if (!pk) return;
        m.wykonaj("DELETE FROM tbl_katalog WHERE symbol LIKE 'GUIMO %'");
        const std::string odp = R"j({"Errors":[],"SearchResults":{"NumberOfResult":2,"Parts":[
          {"DataSheetUrl":"//www.mouser.com/datasheet/2/405/lm358.pdf","Description":"Op Amps Dual","Manufacturer":"Texas Instruments","ManufacturerPartNumber":"LM358DR",
           "MouserPartNumber":"595-LM358DR","ProductAttributes":[{"AttributeName":"Package / Case","AttributeValue":"SOIC-8"}]},
          {"DataSheetUrl":"","Description":"Dual Op Amp DIP","Manufacturer":"ON Semiconductor","ManufacturerPartNumber":"LM358N","MouserPartNumber":"863-LM358N"}]}})j";
        {
            DialogDatasheetMouser d(okno, wxString::FromUTF8("LM358"), "00000000-1111-2222-3333-444444444444");
            CHECK(d.PoleSzukania()->GetValue() == "LM358");
            CHECK(d.ZastosujOdpowiedz(odp) == 2 && d.LiczbaWynikow() == 2);
            CHECK(d.Wybrana() == nullptr);
            CHECK(d.Wybierz(0) && d.Wybrana() && d.Wybrana()->numerProducenta == "LM358DR" && d.Wybrana()->adresDatasheetu == "https://www.mouser.com/datasheet/2/405/lm358.pdf");
            CHECK(!d.Wybierz(5));
            CHECK(d.ZastosujOdpowiedz(R"j({"Errors":[{"Code":"Invalid","Message":"Invalid unique identifier.","PropertyName":"API Key"}]})j") == -1);
            CHECK(d.LiczbaWynikow() == 0 && d.Stan().Contains("klucz"));
            CHECK(d.ZastosujOdpowiedz(R"j({"Errors":[],"SearchResults":{"NumberOfResult":0,"Parts":[]}})j") == 0 && d.Stan().Contains("nie znalazł"));
        }
        // zastosowanie wyniku w katalogu: puste pola uzupełnione, istniejące zostają
        PozycjaKatalogu a; a.symbol = "GUIMO LM358"; a.producent = "Moje TI";
        const auto id = lk.zapisz(a);
        CHECK(id.has_value());
        if (id) {
            pk->Szukaj("GUIMO");
            pk->Wybierz(*id);
            std::vector<Mouser::Czesc> cz; std::string blad;
            CHECK(Mouser::zOdpowiedzi(odp, cz, blad) && cz.size() == 2);
            CHECK(pk->ZastosujWynikMouser(cz[0]));
            auto p = lk.pobierzPozycje(*id);
            CHECK(p && p->producent == "Moje TI" && p->opis == "Op Amps Dual" && p->obudowa == "SOIC-8" && p->datasheetUrl == "https://www.mouser.com/datasheet/2/405/lm358.pdf");
            pk->Wybierz(*id);
            CHECK(pk->ZastosujWynikMouser(cz[1]));   // brak adresu datasheetu w wyniku nie kasuje zapisanego
            auto p2 = lk.pobierzPozycje(*id);
            CHECK(p2 && p2->datasheetUrl == "https://www.mouser.com/datasheet/2/405/lm358.pdf" && p2->obudowa == "SOIC-8");
        }
        m.wykonaj("DELETE FROM tbl_katalog WHERE symbol LIKE 'GUIMO %'");
    }

    void TestOProgramie(OknoGlowne* okno) {
        DialogOProgramie d(okno);
        CHECK(d.Karty() && d.Karty()->GetPageCount() == 3);
        const wxString o = DialogOProgramie::TrescOProgramie();
        CHECK(o.Contains("Magmass") && o.Contains("Adam Massalski") && o.Contains("github.com/amassalski/Magmass") && o.Contains("GNU General Public License"));
        const wxString p = DialogOProgramie::TrescPodziekowan();
        CHECK(p.Contains("wxWidgets") && p.Contains("PostgreSQL") && p.Contains("ExifTool") && p.Contains("OCRmyPDF") && p.Contains("Tesseract"));
        const wxString l = DialogOProgramie::TekstLicencji();
        CHECK(l.Contains("GNU GENERAL PUBLIC LICENSE") && l.Contains("Version 3"));
    }

    void TestMenuIWydania(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaKsiazek lks(m);
        m.wykonaj("DELETE FROM tbl_ksiazki WHERE tytul LIKE 'GUIMN %'");
        PanelKsiazek* pk = ZnajdzTyp<PanelKsiazek>(okno);
        CHECK(pk != nullptr);
        if (!pk) return;
        Ksiazka k; k.tytul = "GUIMN Lampy"; k.wydanie = "wydanie trzecie"; k.uwagi = "<b>Gruby</b> opis <i>kursywa</i>"; k.autor = "A & B";
        auto id = lks.zapisz(k);
        CHECK(id.has_value());
        if (!id) return;
        CHECK(lks.zapiszIndeks(*id, {"Pierwsza strona o triodzie."}));
        pk->Szukaj("GUIMN");
        pk->Wybierz(*id);
        auto zapisana = lks.pobierzKsiazke(*id);
        CHECK(zapisana && zapisana->wydanie == "wydanie trzecie");
        // karta Info: dane i uwagi w HTML
        const wxString info = pk->TrescInfo();
        CHECK(info.Contains("wydanie trzecie") && info.Contains("<b>Gruby</b>") && info.Contains("A &amp; B"));
        // menu podręczne: pięć poleceń, Usuń indeks aktywny (książka ma indeks), zapis/OCR nieaktywne (brak pliku PDF)
        auto menu = pk->MenuKontekstowe();
        CHECK(menu != nullptr);
        if (menu) {
            CHECK(menu->FindItem(menu->FindItem(wxString::FromUTF8("Usuń książkę"))) != nullptr);
            wxMenuItem* ind = menu->FindItemByPosition(1);
            CHECK(ind && ind->GetItemLabelText() == wxString::FromUTF8("Usuń indeks") && ind->IsEnabled());
            wxMenuItem* meta = menu->FindItemByPosition(3);
            wxMenuItem* ocr = menu->FindItemByPosition(4);
            CHECK(meta && !meta->IsEnabled() && ocr && !ocr->IsEnabled());
            wxMenuItem* inf = menu->FindItemByPosition(6);
            CHECK(inf && inf->GetItemLabelText() == "Info" && inf->IsEnabled());
        }
        // usunięcie indeksu: książka zostaje
        CHECK(pk->UsunIndeks(true) == 1);
        auto po = lks.pobierzKsiazke(*id);
        CHECK(po && po->strony == 0 && po->tytul == "GUIMN Lampy");
        { auto m2 = pk->MenuKontekstowe(); CHECK(m2 && !m2->FindItemByPosition(1)->IsEnabled()); }
        CHECK(pk->UsunIndeks(true) == 0);
        // formularz: pole Wydanie i uwagi pokazane jako HTML, edycja dopiero po dwukliku
        {
            DialogKsiazki d(okno, *po);
            CHECK(d.PoleWydania() && d.PoleWydania()->GetValue() == "wydanie trzecie");
            CHECK(d.PodgladUwag() != nullptr && d.PodgladUwag()->ToText().Contains("Gruby opis") && !d.PodgladUwag()->ToText().Contains("<b>"));
            CHECK(d.Uwagi() == wxString::FromUTF8("<b>Gruby</b> opis <i>kursywa</i>"));
            d.UstawUwagi(wxString::FromUTF8("zwykły tekst\ndruga linia"));
            CHECK(d.Uwagi() == wxString::FromUTF8("zwykły tekst\ndruga linia"));
            CHECK(UwagiHTML::zTekstu(d.Uwagi().utf8_string()).find("<br>") != std::string::npos);   // zwykły tekst pokazany jako HTML (łamanie linii)
        }
        m.wykonaj("DELETE FROM tbl_ksiazki WHERE tytul LIKE 'GUIMN %'");
    }

    void TestKsiazek(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        auto sprzatanie = [&] { m.wykonaj("DELETE FROM tbl_ksiazki WHERE tytul LIKE 'GUIKS %' OR sciezka LIKE '%dm_guiks%'"); };
        sprzatanie();
        PanelKsiazek* pk = ZnajdzTyp<PanelKsiazek>(okno);
        CHECK(pk != nullptr);
        if (!pk) return;
        // prosty PDF z dwiema stronami tekstu
        const wxString katalog = wxFileName::GetTempDir() + "/dm_guiks";
        wxFileName::Mkdir(katalog, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        {
            const std::string teksty[] = {"First page about resistors", "Second page: triode ECC83 amplifier"};
            std::string pdf = "%PDF-1.4\n";
            std::vector<size_t> przes;
            auto obiekt = [&](int nr, const std::string& tresc) { przes.push_back(pdf.size()); pdf += std::to_string(nr) + " 0 obj\n" + tresc + "\nendobj\n"; };
            obiekt(1, "<< /Type /Catalog /Pages 2 0 R /Metadata 7 0 R >>");
            obiekt(2, "<< /Type /Pages /Kids [3 0 R 5 0 R] /Count 2 >>");
            for (int i = 0; i < 2; ++i) {
                obiekt(3 + 2 * i, "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents " + std::to_string(4 + 2 * i) +
                       " 0 R /Resources << /Font << /F1 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> >> >> >>");
                const std::string s = "BT /F1 14 Tf 72 700 Td (" + teksty[i] + ") Tj ET";
                obiekt(4 + 2 * i, "<< /Length " + std::to_string(s.size()) + " >>\nstream\n" + s + "\nendstream");
            }
            const std::string xmp =
                "<?xpacket begin='' id='W5M0MpCehiHzreSzNTczkc9d'?><x:xmpmeta xmlns:x=\"adobe:ns:meta/\"><rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">"
                "<rdf:Description rdf:about=\"\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:prism=\"http://prismstandard.org/namespaces/basic/2.0/\">"
                "<dc:title><rdf:Alt><rdf:li xml:lang=\"x-default\">GUIKS Wzmacniacze lampowe (XMP)</rdf:li></rdf:Alt></dc:title>"
                "<dc:creator><rdf:Seq><rdf:li>Jan Xmp</rdf:li></rdf:Seq></dc:creator><dc:date>2012-05-01</dc:date><prism:isbn>978-83-206-1777-1</prism:isbn>"
                "</rdf:Description></rdf:RDF></x:xmpmeta><?xpacket end='w'?>";
            obiekt(7, "<< /Type /Metadata /Subtype /XML /Length " + std::to_string(xmp.size()) + " >>\nstream\n" + xmp + "\nendstream");
            const size_t x = pdf.size();
            pdf += "xref\n0 8\n0000000000 65535 f \n";
            for (size_t o : przes) { char b[32]; snprintf(b, sizeof b, "%010zu 00000 n \n", o); pdf += b; }
            pdf += "trailer\n<< /Size 8 /Root 1 0 R >>\nstartxref\n" + std::to_string(x) + "\n%%EOF\n";
            wxFFile f(katalog + "/GUIKS_wzmacniacze_lampowe.pdf", "wb");
            f.Write(pdf.data(), pdf.size());
        }
        // EPUB z metadanymi OPF
        {
            wxFFileOutputStream wy(katalog + "/GUIKS_ksiazka.epub");
            wxZipOutputStream zip(wy);
            auto wpis = [&](const char* nazwa, const std::string& tresc) {
                zip.PutNextEntry(nazwa);
                zip.Write(tresc.data(), tresc.size());
                zip.CloseEntry();
            };
            wpis("mimetype", "application/epub+zip");
            wpis("META-INF/container.xml", "<?xml version=\"1.0\"?><container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">"
                 "<rootfiles><rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/></rootfiles></container>");
            wpis("OEBPS/content.opf", "<?xml version=\"1.0\"?><package xmlns=\"http://www.idpf.org/2007/opf\" version=\"2.0\"><metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
                 "xmlns:opf=\"http://www.idpf.org/2007/opf\"><dc:title>GUIKS EPUB z OPF</dc:title><dc:creator>Ewa Opf</dc:creator><dc:publisher>Wyd. Test</dc:publisher>"
                 "<dc:date>2018-01-01</dc:date><dc:language>pl</dc:language><dc:subject>lampy</dc:subject></metadata>"
                 "<manifest><item id=\"c1\" href=\"r1.xhtml\" media-type=\"application/xhtml+xml\"/><item id=\"c2\" href=\"r2.xhtml\" media-type=\"application/xhtml+xml\"/></manifest>"
                 "<spine><itemref idref=\"c2\"/><itemref idref=\"c1\"/></spine></package>");
            wpis("OEBPS/r1.xhtml", "<html><body><h1>Rozdzial</h1><p>Opis cewki i kondensatora.</p></body></html>");
            wpis("OEBPS/r2.xhtml", "<html><body><p>Wstep o pentodzie EL84 i transformatorze.</p></body></html>");
            zip.Close();
        }
        okno->PokazLiteratura();
        CHECK(pk->DodajZKatalogu(katalog) == 2);
        CHECK(pk->DodajZKatalogu(katalog) == 0);                  // drugi raz: pominięte
        pk->Szukaj("GUIKS");
        CHECK(pk->Lista().size() == 2);
        {
            const Ksiazka* epub = nullptr; const Ksiazka* pdfk = nullptr;
            for (const Ksiazka& k : pk->Lista()) (k.sciezka.find(".epub") != std::string::npos ? epub : pdfk) = &k;
            CHECK(epub && epub->tytul == "GUIKS EPUB z OPF" && epub->autor == "Ewa Opf" && epub->wydawca == "Wyd. Test" && epub->rok == 2018 && epub->jezyk == "polski");
            CHECK(pdfk && pdfk->tytul == "GUIKS Wzmacniacze lampowe (XMP)" && pdfk->autor == "Jan Xmp" && pdfk->rok == 2012 && pdfk->isbn == "978-83-206-1777-1");
            if (pdfk) pk->Wybierz(pdfk->id);
        }
        if (pk->Wybrana()) {
            CHECK(pk->Wybrana() != nullptr);
            CHECK(pk->Indeksuj(true) == 2);
            CHECK(pk->SzukajWTresci("TRIODE amplifier") == 1);
            CHECK(pk->Trafienia().size() == 1 && pk->Trafienia()[0].strona == 2);
            CHECK(pk->SzukajWTresci("resistors", true) == 1 && pk->Trafienia()[0].strona == 1);
            CHECK(pk->SzukajWTresci("pentode") == 0);
            pk->SzukajWTresci("triode");
            wxYield();
            ZrzutEkranu("23_literatura");
            CHECK(pk->IndeksujWszystkie(true) == 1);              // zostaje tylko EPUB
            CHECK(pk->SzukajWTresci("pentodzie") == 1 && pk->Trafienia()[0].tytul == "GUIKS EPUB z OPF");
            CHECK(pk->SzukajWTresci("kondensatora") == 1);
            CHECK(pk->IndeksujWszystkie(true) == 0);              // nic do zrobienia
        }
        // pożyczanie książki: zapis przez logikę, zwrot i przypomnienie przez panel
        {
            LogikaKsiazek lk(m);
            std::vector<Ksiazka> kk;
            lk.pobierz("GUIKS EPUB", kk);
            CHECK(kk.size() == 1);
            if (kk.size() == 1) {
                CHECK(lk.pozycz(kk[0].id, "Jan Pozyczajacy", "2000-01-01", "2000-01-10", ""));
                pk->Szukaj("GUIKS");
                pk->Wybierz(kk[0].id);
                CHECK(pk->Przypomnij() >= 1);
                CHECK(pk->Zwroc(true));
                CHECK(pk->Przypomnij() == 0);
                auto o = lk.pobierzKsiazke(kk[0].id);
                CHECK(o && !o->pozyczona);
            }
        }
        // import CSV przez panel
        const wxString csv = wxFileName::GetTempDir() + "/guiks.csv";
        {
            wxFFile f(csv, "wb");
            const std::string t = "\xEF\xBB\xBFTytuł;Autor;Rok\nGUIKS Papierowa;Jan Test;2001\n";
            f.Write(t.data(), t.size());
        }
        CHECK(pk->ImportujCSV(csv) == 1);
        wxRemoveFile(csv);
        const wxString out = wxFileName::GetTempDir() + "/guiks_out.csv";
        CHECK(pk->EksportujCSV(out) && wxFileExists(out));
        wxRemoveFile(out);
        pk->Szukaj("");
        okno->PokazKatalog();
        sprzatanie();
        wxRemoveFile(katalog + "/GUIKS_wzmacniacze_lampowe.pdf");
        wxRemoveFile(katalog + "/GUIKS_ksiazka.epub");
        wxFileName::Rmdir(katalog);
    }

    void TestNarzedzi(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaNarzedzi ln(m);
        auto sprzatanie = [&] { m.wykonaj("DELETE FROM tbl_narzedzia WHERE nazwa LIKE 'GUIN %'"); };
        sprzatanie();
        PanelNarzedzi* pn = ZnajdzTyp<PanelNarzedzi>(okno);
        CHECK(pn != nullptr);
        if (!pn) return;
        Narzedzie a; a.nazwa = "GUIN Lutownica"; a.producent = "Weller"; a.model = "WTCP"; a.kategoria = "Lutowanie"; a.miejsce = "Biurko"; a.cena = 450;
        Narzedzie b; b.nazwa = "GUIN Multimetr"; b.model = "UT61E";
        Narzedzie c; c.nazwa = "GUIN Szczypce";
        auto ia = ln.zapisz(a), ib = ln.zapisz(b), ic = ln.zapisz(c);
        CHECK(ia && ib && ic);
        if (!ia || !ib || !ic) return;
        CHECK(ln.pozycz(*ia, "Adam Kowalski", "2000-01-10", "2000-01-20", "z grotem"));   // po terminie
        CHECK(ln.pozycz(*ib, "Ewa", "", "", ""));                                           // bez terminu
        okno->PokazNarzedzia();
        pn->Szukaj("GUIN");
        CHECK(pn->Lista().size() == 3);
        pn->Wybierz(*ia);
        CHECK(pn->Wybrane() && pn->Wybrane()->id == *ia && pn->Wybrane()->przeterminowane);
        CHECK(pn->Historia().size() == 1 && pn->Historia()[0].osoba == "Adam Kowalski");
        pn->UstawFiltr(true, false);
        CHECK(pn->Lista().size() == 2);
        pn->UstawFiltr(true, true);
        CHECK(pn->Lista().size() == 1 && pn->Lista()[0].id == *ia);
        pn->UstawFiltr(false, false);
        CHECK(pn->Przypomnij() >= 1);
        pn->Wybierz(*ia);
        wxYield();
        ZrzutEkranu("24_narzedzia");
        CHECK(pn->Zwroc(true));
        CHECK(pn->Wybrane() && !pn->Wybrane()->pozyczone && pn->Historia().size() == 1 && !pn->Historia()[0].zwrot.empty());
        pn->Wybierz(*ic);
        CHECK(!pn->ZdjeciePokazane());
        {
            const wxString obraz = wxFileName::GetTempDir() + "/guin_zdjecie.png";
            wxImage im(40, 30); im.SetRGB(wxRect(0, 0, 40, 30), 200, 30, 30);
            CHECK(im.SaveFile(obraz, wxBITMAP_TYPE_PNG));
            auto nc = ln.pobierzNarzedzie(*ic);
            CHECK(nc.has_value());
            if (nc) { nc->zdjecie = obraz.utf8_string(); CHECK(ln.zapisz(*nc).has_value()); }
            pn->Szukaj("GUIN");
            pn->Wybierz(*ic);
            CHECK(pn->ZdjeciePokazane());
            wxRemoveFile(obraz);
        }
        CHECK(!pn->Zwroc(true));                                  // nie jest pożyczone
        pn->Szukaj("ewa");                                       // szukanie po osobie
        CHECK(pn->Lista().size() == 1 && pn->Lista()[0].id == *ib);
        const wxString out = wxFileName::GetTempDir() + "/guin_out.csv";
        CHECK(pn->EksportujCSV(out) && wxFileExists(out));
        wxRemoveFile(out);
        pn->Szukaj("");
        okno->PokazKatalog();
        sprzatanie();
    }

    void TestSerwisu(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaSerwisu ls(m);
        LogikaNarzedzi ln(m);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_serwis WHERE urzadzenie LIKE 'GUIS %'");
            m.wykonaj("DELETE FROM tbl_narzedzia WHERE nazwa LIKE 'GUIS %'");
        };
        sprzatanie();
        PanelSerwisu* ps = ZnajdzTyp<PanelSerwisu>(okno);
        CHECK(ps != nullptr);
        if (!ps) return;
        WpisSerwisowy a; a.urzadzenie = "GUIS Router"; a.numerSeryjny = "SN-1"; a.lokalizacja = "Węzeł A"; a.data = "2026-05-01";
        a.status = "w trakcie"; a.rodzaj = "wymiana";
        CzescSerwisowa c; c.nazwa = "Moduł SFP"; c.numerNowy = "SFP-9";
        a.czesci = {c};
        WpisSerwisowy b; b.urzadzenie = "GUIS Switch"; b.data = "2026-04-01";
        auto ia = ls.zapisz(a), ib = ls.zapisz(b);
        CHECK(ia && ib);
        if (!ia || !ib) { sprzatanie(); return; }
        okno->PokazSerwis();
        ps->Szukaj("GUIS");
        CHECK(ps->Lista().size() == 2 && ps->Lista()[0].id == *ia);
        ps->Wybierz(*ia);
        CHECK(ps->Wybrany() && ps->Wybrany()->id == *ia);
        CHECK(ps->Szczegoly() && ps->Szczegoly()->czesci.size() == 1);
        ps->UstawFiltr("", "w trakcie");
        CHECK(ps->Lista().size() >= 1);
        ps->UstawFiltr("", "");
        ps->Szukaj("SFP-9");
        CHECK(ps->Lista().size() == 1 && ps->Lista()[0].id == *ia);
        ps->Szukaj("GUIS");
        ps->Wybierz(*ia);
        const wxString obraz = wxFileName::GetTempDir() + "/guis_zdjecie.png";
        wxImage im(60, 40); im.SetRGB(wxRect(0, 0, 60, 40), 30, 120, 30);
        CHECK(im.SaveFile(obraz, wxBITMAP_TYPE_PNG));
        CHECK(ps->DolaczZdjecie(obraz, "przed"));
        CHECK(ps->Szczegoly() && ps->Szczegoly()->zdjecia.size() == 1 && ps->LiczbaMiniatur() == 1);
        wxYield();
        ZrzutEkranu("25_serwis");
        ps->UsunZdjecie(0, true);
        CHECK(ps->Szczegoly() && ps->Szczegoly()->zdjecia.empty() && ps->LiczbaMiniatur() == 0);
        wxRemoveFile(obraz);
        ps->HistoriaUrzadzenia();
        CHECK(ps->Lista().size() == 1);                            // po numerze seryjnym
        ps->Szukaj("GUIS");
        const wxString out = wxFileName::GetTempDir() + "/guis_out.csv";
        CHECK(ps->EksportujCSV(out) && wxFileExists(out));
        wxRemoveFile(out);
        ps->Wybierz(*ib);
        ps->Usun(true);
        CHECK(ps->Lista().size() == 1);

        // zdjęcie narzędzia (to samo, co robi menu kontekstowe "Dołącz zdjęcie z kamery")
        Narzedzie n; n.nazwa = "GUIS Miernik";
        auto in = ln.zapisz(n);
        CHECK(in.has_value());
        if (in) {
            okno->PokazNarzedzia();
            PanelNarzedzi* pn = ZnajdzTyp<PanelNarzedzi>(okno);
            CHECK(pn != nullptr);
            if (pn) {
                pn->Szukaj("GUIS Miernik");
                pn->Wybierz(*in);
                CHECK(!pn->ZdjeciePokazane());
                const wxString o2 = wxFileName::GetTempDir() + "/guis_n.png";
                CHECK(im.SaveFile(o2, wxBITMAP_TYPE_PNG));
                CHECK(pn->DolaczZdjecie(o2));
                CHECK(pn->ZdjeciePokazane());
                wxRemoveFile(o2);
                pn->Szukaj("");
            }
        }
        okno->PokazKatalog();
        sprzatanie();
    }

    void TestKatalogu(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        LogikaKatalogu lk(m);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUIK2\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIK2\\_%'");
            m.wykonaj("DELETE FROM tbl_katalog WHERE symbol LIKE 'GUIK2\\_%'");
        };
        sprzatanie();
        PanelKatalogu* pk = ZnajdzTyp<PanelKatalogu>(okno);
        CHECK(pk != nullptr);
        if (!pk) return;

        PozycjaKatalogu a; a.symbol = "GUIK2_LM358"; a.producent = "TI"; a.opis = "Dual Op-Amp"; a.obudowa = "DIP-8"; a.rodzina = "Wzmacniacze";
        a.parametry = {{"Napięcie zasilania", "3 V ... 32 V", "reczny"}};
        const auto ida = lk.zapisz(a);
        CHECK(ida.has_value());

        // import CSV przez panel
        const wxString csv = wxFileName::GetTempDir() + "/guik2.csv";
        {
            wxFFile f(csv, "wb");
            const std::string t = "\xEF\xBB\xBFSymbol;Producent;Opis;Obudowa;Rodzina;Parametry\n"
                                  "GUIK2_NE555;TI;Timer;DIP-8;Timery;\"Częstotliwość=100 kHz\"\n"
                                  "GUIK2_BC547;;NPN;TO-92;Tranzystory;\n";
            f.Write(t.data(), t.size());
        }
        CHECK(pk->ImportujCSV(csv) == 2);
        CHECK(pk->ImportujCSV(csv) == 0);                     // drugi raz: wszystko pominięte
        wxRemoveFile(csv);
        pk->Szukaj("GUIK2_");
        CHECK(pk->Lista().size() == 3);
        auto ne = lk.znajdz("GUIK2_NE555", "TI");
        CHECK(ne && ne->parametr("Częstotliwość") && ne->parametr("Częstotliwość")->wartosc == "100 kHz");

        // dodanie do magazynu i powiązanie
        pk->Wybierz(*ida);
        CHECK(pk->Wybrana() && pk->Wybrana()->symbol == "GUIK2_LM358" && pk->Wybrana()->wMagazynie == 0);
        const auto idm = lk.dodajDoMagazynu(logika, *ida, 7, MiejsceMagazynowe{"Kontener 1", "B2", ""}, std::nullopt);
        CHECK(idm.has_value());
        CHECK(!lk.dodajDoMagazynu(logika, *ida, 1, MiejsceMagazynowe{}, std::nullopt).has_value());   // duplikat symbolu
        pk->Odswiez();
        pk->Wybierz(*ida);
        CHECK(pk->Wybrana() && pk->Wybrana()->wMagazynie == 7);
        PanelPodgladu* pp = ZnajdzTyp<PanelPodgladu>(okno);
        PanelElementow* pe = ZnajdzTyp<PanelElementow>(okno);
        if (pp && pe && idm) {
            pe->UstawKategorie(std::nullopt);
            pe->Szukaj("GUIK2_LM358");
            pe->WybierzPierwszy();
            CHECK(pp->Opis().Contains("Z katalogu") && pp->Opis().Contains("3 V ... 32 V"));
            pe->Szukaj("");
        }

        // dialogi
        {
            DialogKatalogu d(okno, *lk.pobierzPozycje(*ida), lk.rodziny());
            CHECK(d.LiczbaParametrow() == 1);
            d.DodajParametr(wxString::FromUTF8("Prąd"), "20 mA");
            const PozycjaKatalogu w = d.Pobierz();
            CHECK(w.symbol == "GUIK2_LM358" && w.parametry.size() == 2 && w.parametry[1].nazwa == "Prąd");
            d.DodajParametr("W_amax", "25 W");
            CHECK(d.OpisParametru(2).find("anody") != std::string::npos && d.OpisParametru(1).empty() && d.OpisParametru(99).empty());
            CallAfter([&] { ZrzutEkranu("20_katalog_pozycja"); d.EndModal(wxID_CANCEL); });
            d.ShowModal();
        }
        {
            DialogWyboruZKatalogu d(okno, lk, "GUIK2_");
            CHECK(d.LiczbaWynikow() == 3);
            d.Szukaj("GUIK2_NE");
            CHECK(d.LiczbaWynikow() == 1 && d.Wybierz(0) && d.Wybrana()->symbol == "GUIK2_NE555");
            DialogElementu de(okno, logika, Element{});
            de.UzupelnijZKatalogu(*d.Wybrana());
            const Element el = de.PobierzElement();
            CHECK(el.symbol == "GUIK2_NE555" && el.producent == "TI" && el.obudowa == "DIP-8" && el.parametry.find("100 kHz") != std::string::npos);
        }

        // odczyt datasheetu z PDF (wymaga pdftotext; bez niego pomijamy)
        {
            const wxString pdf = wxFileName::GetTempDir() + "/guik2.pdf";
            const std::string tresc =
                "BT /F1 11 Tf 50 740 Td 14 TL (LM358 Dual Operational Amplifier) Tj T* (Supply voltage 3 V to 32 V) Tj "
                "T* (Operating temperature -40 C to 85 C) Tj T* (Available in SOIC-8 package) Tj ET";
            std::string pdfTekst = "%PDF-1.4\n1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n2 0 obj<</Type/Pages/Kids[3 0 R]/Count 1>>endobj\n"
                "3 0 obj<</Type/Page/Parent 2 0 R/MediaBox[0 0 612 792]/Contents 4 0 R/Resources<</Font<</F1 5 0 R>>>>>>endobj\n"
                "4 0 obj<</Length " + std::to_string(tresc.size()) + ">>stream\n" + tresc + "\nendstream endobj\n"
                "5 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj\ntrailer<</Root 1 0 R/Size 6>>\n%%EOF\n";
            { wxFFile f(pdf, "wb"); f.Write(pdfTekst.data(), pdfTekst.size()); }
            std::string blad;
            if (!OdczytDatasheetu::tekstZPDF(pdf.utf8_string(), blad).empty()) {
                pk->Wybierz(*ida);
                CHECK(pk->OdczytajDatasheet(pdf, true) >= 2);
                auto po = lk.pobierzPozycje(*ida);
                CHECK(po && po->parametr("Temperatura pracy") && po->parametr("Temperatura pracy")->wartosc == "-40 ... +85 °C");
                bool zrodlo = false;
                if (po) for (const auto& p : po->parametry) if (p.nazwa == "Temperatura pracy" && p.zrodlo == "datasheet") zrodlo = true;
                CHECK(zrodlo);
                CHECK(po && po->parametr("Prąd") == nullptr);                      // nie dopisano niczego z dialogu, który anulowano
                CHECK(po && po->parametr("Napięcie zasilania") != nullptr);
            } else {
                std::cout << "  (pomijam odczyt PDF: " << blad << ")\n";
            }
            wxRemoveFile(pdf);
        }
        // eksport
        const wxString out = wxFileName::GetTempDir() + "/guik2_out.csv";
        CHECK(pk->EksportujCSV(out) && wxFileExists(out));
        wxRemoveFile(out);

        okno->PokazKatalog();
        pk->Szukaj("GUIK2_");
        pk->Wybierz(*ida);
        wxYield();
        ZrzutEkranu("21_zakladka_katalog");
        pk->Szukaj("");
        sprzatanie();
    }

    void TestSklepow(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        LogikaProjektow lp(m, logika);
        LogikaSklepow ls(m);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_pozycje_projektow WHERE id_projektu IN (SELECT id FROM tbl_projekty WHERE nazwa LIKE 'GUIS\\_%')");
            m.wykonaj("DELETE FROM tbl_projekty WHERE nazwa LIKE 'GUIS\\_%'");
            m.wykonaj("DELETE FROM tbl_elementy_sklepy WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUIS\\_%')");
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUIS\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIS\\_%'");
            m.wykonaj("DELETE FROM tbl_sklepy WHERE nazwa LIKE 'GUIS\\_%'");
        };
        sprzatanie();

        // menu Sklepy
        CHECK(okno->GetMenuBar()->FindMenu("Sklepy") != wxNOT_FOUND);

        // --- okno sklepów: dodanie przez okno, lista
        Sklep a; a.nazwa = "GUIS_A"; a.urlBazowy = "https://a.example/"; a.kosztDostawy = 10; a.priorytet = 9;
        Sklep b; b.nazwa = "GUIS_B"; b.urlBazowy = "https://b.example/"; b.kosztDostawy = 15; b.darmowaDostawaOd = 100; b.priorytet = 9;
        auto ia = ls.zapiszSklep(a), ib = ls.zapiszSklep(b);
        CHECK(ia && ib);
        if (!ia || !ib) return;
        {
            DialogSklepow dlg(okno, ls);
            bool jestA = false;
            for (const Sklep& s : dlg.Sklepy()) jestA = jestA || s.nazwa == "GUIS_A";
            CHECK(jestA);
            const size_t przed = dlg.Sklepy().size();
            CallAfter([&] {
                auto* d = dynamic_cast<DialogDodajSklep*>(Gorne());
                CHECK(d != nullptr);
                if (!d) return;
                d->tcNazwa->SetValue("GUIS_C");
                d->tcAdres->SetValue("https://c.example/");
                d->tcSzablon->SetValue("https://c.example/s?q={q}");
                d->tcWaluta->SetValue("usd");
                d->tcDostawa->SetValue("5,5");
                wxWindow* ok = Znajdz(d, "OK");
                CHECK(ok != nullptr);
                if (ok) Kliknij(ok);
            });
            CHECK(dlg.Dodaj());
            CHECK(dlg.Sklepy().size() == przed + 1);
            bool c = false;
            for (const Sklep& s : dlg.Sklepy()) if (s.nazwa == "GUIS_C") c = s.waluta == "USD" && s.kosztDostawy == 5.5 && s.szablonWyszukiwania.find("{q}") != std::string::npos;
            CHECK(c);
            // zła waluta: okno zostaje otwarte (zapis się nie udaje)
            {   // zamykane przed ShowModal, bo Gorne() wskazuje ostatnio utworzone okno
                DialogDodajSklep zle(okno, ls);
                zle.tcNazwa->SetValue("GUIS_Z");
                zle.tcAdres->SetValue("https://z/");
                zle.tcWaluta->SetValue("zl");
                CHECK(zle.Pobierz().waluta == "ZL");           // okno przekazuje wartość, a baza ją odrzuca
                CHECK(!ls.zapiszSklep(zle.Pobierz()));
                CHECK(zle.IdZapisanego() == 0);
            }
            CallAfter([&] { ZrzutEkranu("10_sklepy"); if (auto* d = dynamic_cast<wxDialog*>(Gorne())) d->EndModal(wxID_CANCEL); });
            dlg.ShowModal();
        }
        {
            std::map<std::string, double> k;
            std::string blad;
            CHECK(DialogSklepow::TekstNaKursy("USD = 4,05\n# komentarz\n\neur=4.3\n", k, blad) && k.size() == 2 && k["EUR"] == 4.3 && k["USD"] == 4.05);
            CHECK(!DialogSklepow::TekstNaKursy("USD 4", k, blad) && !blad.empty());
            CHECK(!DialogSklepow::TekstNaKursy("USD = abc", k, blad));
            CHECK(!DialogSklepow::TekstNaKursy("PLN = 2", k, blad));
            CHECK(!DialogSklepow::TekstNaKursy("USD = 0", k, blad));
            CHECK(DialogSklepow::KursyNaTekst({{"PLN", 1}, {"USD", 4.05}}) == "USD = 4,05\n");
        }

        // --- elementy, linki z cenami, projekt z brakami
        Element e1; e1.symbol = "GUIS_E1"; e1.producent = "Test";
        Element e2; e2.symbol = "GUIS_E2";
        auto i1 = logika.zapiszElement(e1), i2 = logika.zapiszElement(e2);
        CHECK(i1 && i2);
        if (!i1 || !i2) { sprzatanie(); return; }
        auto link = [&](long long el, long long sk, double cena) {
            LinkSklepu l; l.idElementu = el; l.idSklepu = sk; l.url = "https://x.example/p"; l.cena = cena;
            CHECK(ls.zapiszLink(l).has_value());
        };
        link(*i1, *ia, 1.00); link(*i1, *ib, 1.20);
        link(*i2, *ia, 2.00); link(*i2, *ib, 1.80);

        Projekt pr; pr.nazwa = "GUIS_Projekt";
        auto ip = lp.zapiszProjekt(pr);
        CHECK(ip.has_value());
        if (!ip) { sprzatanie(); return; }
        PozycjaProjektu p1; p1.idProjektu = *ip; p1.idElementu = *i1; p1.wymagana = 10;
        PozycjaProjektu p2; p2.idProjektu = *ip; p2.idElementu = *i2; p2.wymagana = 5;
        CHECK(lp.dodajPozycje(p1).has_value() && lp.dodajPozycje(p2).has_value());

        {
            DialogZestawienia dlg(okno, lp, ls, *ip);
            CHECK(dlg.Potrzeby().size() == 2);
            // tylko oferty dwóch testowych sklepów; inne sklepy (z cenami) nie wchodzą, bo elementy testowe mają tylko te linki
            const PlanZakupow& najt = dlg.PlanDlaTrybu(TrybPlanu::Najtanszy);
            const PlanZakupow& ceny = dlg.PlanDlaTrybu(TrybPlanu::NajtanszeCeny);
            const PlanZakupow& jeden = dlg.PlanDlaTrybu(TrybPlanu::JedenSklep);
            CHECK(najt.kompletny() && std::abs(najt.razemPLN() - 30.0) < 0.005);    // wszystko w A: 10 + 10 + 10 zł dostawy
            CHECK(std::abs(ceny.razemPLN() - 44.0) < 0.005);                        // E1 w A (10), E2 w B (9), dostawy 10 + 15
            CHECK(jeden.kompletny() && std::abs(jeden.razemPLN() - 30.0) < 0.005);
            CHECK(dlg.TekstPorownania().Contains("30,00") && dlg.TekstPorownania().Contains("44,00"));
            CHECK(dlg.TekstPorownania().Contains("14,00"));                         // oszczędność na łączeniu zakupów

            dlg.WybierzTryb(TrybPlanu::NajtanszeCeny);
            CHECK(std::abs(dlg.Plan().razemPLN() - 44.0) < 0.005);
            const Raport r = dlg.RaportZPlanu();
            CHECK(r.wiersze.size() == 2 && r.naglowki.size() == 6);
            dlg.WybierzTryb(TrybPlanu::Najtanszy);

            // już zamówione: 4 z 10 sztuk E1
            std::vector<PozycjaProjektu> poz;
            CHECK(lp.pobierzPozycje(*ip, poz));
            for (PozycjaProjektu& x : poz) if (x.idElementu == *i1) { x.zamowiona = 10; CHECK(lp.zapiszPozycje(x)); }
            CHECK(dlg.Przelicz());
            CHECK(dlg.Potrzeby().size() == 1 && dlg.Potrzeby()[0].symbol == "GUIS_E2");   // E1 zamówione w całości
            dlg.OdejmujZamowione(false);
            CHECK(dlg.Potrzeby().size() == 2);
            dlg.WybierzProjekt(std::nullopt);                                              // wszystkie aktywne projekty
            CHECK(dlg.Potrzeby().size() >= 2);
            dlg.WybierzProjekt(*ip);

            CallAfter([&] { ZrzutEkranu("11_zestawienie"); if (auto* d = dynamic_cast<wxDialog*>(Gorne())) d->EndModal(wxID_CANCEL); });
            dlg.ShowModal();
        }

        // --- okno linków elementu
        {
            DialogLinkowElementu dlg(okno, ls, logika, *i1);
            bool a1 = false, b1 = false;
            for (const LinkSklepu& l : dlg.Linki()) { a1 = a1 || (l.idSklepu == *ia && l.cena && *l.cena == 1.0); b1 = b1 || l.idSklepu == *ib; }
            CHECK(a1 && b1);
            CHECK(dlg.StatusDatasheetu().Contains("Brak datasheetu"));
            CHECK(!dlg.Sprawdzanie());
            CallAfter([&] { ZrzutEkranu("12_linki_elementu"); if (auto* d = dynamic_cast<wxDialog*>(Gorne())) d->EndModal(wxID_CANCEL); });
            dlg.ShowModal();
        }

        sprzatanie();
    }

    void TestRaportowIEtykiet(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        LogikaProjektow lp(m, logika);
        LogikaRaportow lr(m, logika, lp);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUIE\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIE\\_%'");
        };
        sprzatanie();
        Element e;
        e.symbol = "GUIE_LM358";
        e.wartosc = "LM358";
        e.obudowa = "SO-8";
        e.producent = "Texas Instruments";
        e.opis = "Podwójny wzmacniacz operacyjny";
        e.iloscDostepna = 3;
        e.iloscMinimalna = 10;
        e.cena = 1.5;
        auto id = logika.zapiszElement(e);
        CHECK(id.has_value());
        if (!id) return;

        // --- raport
        {
            DialogRaportu dlg(okno, lr);
            dlg.WybierzRodzaj(RodzajRaportu::NiskieStany);
            CHECK(dlg.Generuj());
            bool jest = false;
            for (const auto& w : dlg.AktualnyRaport().wiersze) jest = jest || w[0] == "GUIE_LM358";
            CHECK(jest);
            dlg.WybierzRodzaj(RodzajRaportu::HistoriaZmian);
            dlg.UstawDaty("2000-01-01", "2999-01-01");
            CHECK(dlg.Generuj());
        }

        // --- etykiety: PNG, a z PNG odczyt kodu (pełna pętla: rysowanie -> obraz -> odczyt)
        std::vector<Element> lista;
        FiltrElementow f;
        f.tekst = "GUIE_";
        CHECK(logika.pobierzElementy(f, lista) && lista.size() == 1);
        const wxString bazowa = wxFileName::GetTempDir() + "/gui_etykiety.png";
        {
            DialogEtykiet dlg(okno, lista, *id);
            dlg.WybierzTylkoZaznaczony(true);
            CHECK(dlg.Etykiety().size() == 1);
            CHECK(dlg.Etykiety()[0].kod.size() == 13);
            const int zapisano = dlg.ZapiszPNG(bazowa, 300);
            if (zapisano != 1) std::cerr << "zapisano=" << zapisano << " sciezka=" << bazowa.utf8_string() << "\n";
            CHECK(zapisano == 1);
            const UstawieniaDruku u = dlg.Ustawienia();
            CHECK(u.format.naStrone() >= 1);
        }
        TestZebry(okno, lista, *id);
        wxString blad;
        const auto kod = OdczytajKodZPliku(bazowa, blad);
        CHECK(kod.has_value());
        if (kod) {
            const int prefiks = Konfiguracja::pobierz().pobierzUstawieniaAplikacji().prefixEAN;
            CHECK(KodKreskowy::idZKodu(*kod, prefiks) == std::optional<long long>(*id));
        }
        wxString blad2;
        CHECK(!OdczytajKodZPliku(wxFileName::GetTempDir() + "/nie_ma_takiego.png", blad2).has_value());
        CHECK(!blad2.empty());

        // --- szukanie po kodzie wskazuje element
        PanelElementow* pe = ZnajdzTyp<PanelElementow>(okno);
        if (pe && kod) {
            pe->Szukaj(wxString::FromUTF8(kod->c_str()));
            CHECK(pe->Elementy().size() == 1);
            CHECK(!pe->Elementy().empty() && pe->Elementy()[0].id == *id);
            CHECK(pe->MaWybor());
            pe->Szukaj("");
        }
        if (!env("DM_TEST_SHOTS", "").empty()) wxCopyFile(bazowa, wxString::FromUTF8(env("DM_TEST_SHOTS", "")) + "/12_etykieta_strona.png");
        wxRemoveFile(bazowa);
        sprzatanie();
    }

    void TestProjektow(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_pozycje_projektow WHERE id_projektu IN (SELECT id FROM tbl_projekty WHERE nazwa LIKE 'GUIP\\_%')");
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_projektu IN (SELECT id FROM tbl_projekty WHERE nazwa LIKE 'GUIP\\_%')");
            m.wykonaj("DELETE FROM tbl_projekty WHERE nazwa LIKE 'GUIP\\_%'");
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUIP\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUIP\\_%'");
        };
        sprzatanie();

        PanelProjektow* pp = ZnajdzTyp<PanelProjektow>(okno);
        wxNotebook* zakladki = ZnajdzTyp<wxNotebook>(okno);
        CHECK(pp != nullptr && zakladki != nullptr);
        if (!pp || !zakladki) return;
        CHECK(zakladki->GetPageCount() == 6);
        CHECK(zakladki->GetPageText(1) == "Projekty");
        CHECK(pp->IsEnabled());

        // element, który program dopasuje po wartości i obudowie
        Element e;
        e.symbol = "GUIP_R10k0805";
        e.wartosc = "10k";
        e.obudowa = "R0805";
        e.iloscDostepna = 4;
        CHECK(logika.zapiszElement(e).has_value());

        const wxString plik = wxFileName::GetTempDir() + "/gui_bom.csv";
        {
            std::ofstream f(plik.utf8_string(), std::ios::binary);
            f << "Reference,Value,Footprint,Qty\n"
                 "\"R1, R2, R3\",10k,Resistor_SMD:R_0805_2012Metric,3\n"
                 "\"C1\",GUIP_1u,Capacitor_SMD:C_0805_2012Metric,1\n";
        }

        zakladki->SetSelection(1);
        pp->Odswiez();

        // --- nowy projekt
        CallAfter([&] {
            wxWindow* dlg = Gorne();
            CHECK(dynamic_cast<DialogProjektu*>(dlg) != nullptr);
            auto* nazwa = ZnajdzTyp<wxTextCtrl>(dlg);
            CHECK(nazwa != nullptr);
            if (nazwa) nazwa->SetValue("GUIP_Zasilacz");
            wxWindow* ok = Znajdz(dlg, "OK");
            CHECK(ok != nullptr);
            if (ok) Kliknij(ok);
        });
        pp->NowyProjekt();
        CHECK(pp->MaProjekt());
        CHECK(!pp->Projekty().empty());
        bool jest = false;
        for (const Projekt& p : pp->Projekty()) jest = jest || p.nazwa == "GUIP_Zasilacz";
        CHECK(jest);

        // --- import BOM z pliku: pierwsza pozycja z magazynu, druga jako nowy element
        CallAfter([&] {
            wxWindow* dlg = Gorne();
            auto* imp = dynamic_cast<DialogImportBOM*>(dlg);
            CHECK(imp != nullptr);
            if (imp) CHECK(imp->LiczbaWierszy() == 2);
            Zrzut(dlg, "08_import_bom");
            auto* plytki = ZnajdzTyp<wxSpinCtrl>(dlg);
            if (plytki) plytki->SetValue(2);
            wxWindow* ok = Znajdz(dlg, wxString::FromUTF8("Importuj"));
            CHECK(ok != nullptr);
            if (ok) Kliknij(ok);
        });
        pp->ImportujBOM(plik);
        CHECK(pp->Pozycje().size() == 2);
        bool rezystory = false, nowy = false;
        for (const PozycjaProjektu& z : pp->Pozycje()) {
            if (z.symbol == "GUIP_R10k0805") rezystory = z.wymagana == 6 && z.brakuje() == 2;
            if (z.symbol == "GUIP_1u C0805" || z.wartosc == "GUIP_1u") nowy = z.wymagana == 2 && z.dostepna == 0;
        }
        CHECK(rezystory);
        CHECK(nowy);
        CHECK(Liczba(m, "SELECT count(*) FROM tbl_elementy WHERE wartosc = 'GUIP_1u'") == 1);
        Zrzut(okno, "09_projekty");

        // --- zmiana stanu w Magazynie odświeża listę części projektu po powrocie na zakładkę Projekty
        {
            m.wykonaj("UPDATE tbl_elementy SET ilosc_dostepna = 10 WHERE symbol = 'GUIP_R10k0805'");
            zakladki->SetSelection(0);
            zakladki->SetSelection(1);
            bool odswiezone = false;
            for (const PozycjaProjektu& z : pp->Pozycje())
                if (z.symbol == "GUIP_R10k0805") odswiezone = z.dostepna == 10 && z.brakuje() == 0;
            CHECK(odswiezone);
            m.wykonaj("UPDATE tbl_elementy SET ilosc_dostepna = 4 WHERE symbol = 'GUIP_R10k0805'");
            zakladki->SetSelection(0);
            zakladki->SetSelection(1);
            bool wrocilo = false;
            for (const PozycjaProjektu& z : pp->Pozycje())
                if (z.symbol == "GUIP_R10k0805") wrocilo = z.dostepna == 4 && z.brakuje() == 2;
            CHECK(wrocilo);
        }

        // --- menu kontekstowe: „Idź do elementu w magazynie”
        {
            CHECK(!pp->Pozycje().empty());
            pp->WybierzPozycje(0);
            const long long idElementu = pp->Pozycje()[0].idElementu;
            PanelElementow* pel = ZnajdzTyp<PanelElementow>(okno);
            CHECK(pel != nullptr);
            pel->Szukaj("zzz_nic_takiego");   // filtr, który ukryłby element
            zakladki->SetSelection(1);
            CHECK(pp->IdzDoElementu());
            CHECK(zakladki->GetSelection() == 0);
            CHECK(pel->Wybrany() != nullptr && pel->Wybrany()->id == idElementu);
            zakladki->SetSelection(1);
        }

        // --- lista zakupów jest policzona z brakujących części
        LogikaProjektow lp(m, logika);
        std::vector<PozycjaProjektu> braki;
        CHECK(lp.pobierzBraki(pp->Projekty()[0].id, braki));
        CHECK(braki.size() == 2);

        // --- wydanie jednej pozycji: okno wyboru ilości (domyślnie tyle, ile można)
        sprzatanie();
        pp->Odswiez();
        zakladki->SetSelection(0);
    }

    void TestMagazynu(OknoGlowne* okno) {
        ManagerBazyDanych& m = ManagerBazyDanych::pobierz();
        LogikaMagazynu logika(m);
        auto sprzatanie = [&] {
            m.wykonaj("DELETE FROM tbl_transakcje WHERE id_elementu IN (SELECT id FROM tbl_elementy WHERE symbol LIKE 'GUI\\_%')");
            m.wykonaj("DELETE FROM tbl_elementy WHERE symbol LIKE 'GUI\\_%'");
            m.wykonaj("DELETE FROM tbl_kategorie WHERE nazwa LIKE 'GUI\\_%' AND id_rodzica IS NOT NULL");
            m.wykonaj("DELETE FROM tbl_kategorie WHERE nazwa LIKE 'GUI\\_%'");
        };
        sprzatanie();

        PanelKategorii* pk = ZnajdzTyp<PanelKategorii>(okno);
        PanelElementow* pe = ZnajdzTyp<PanelElementow>(okno);
        CHECK(pk != nullptr);
        CHECK(pe != nullptr);
        if (!pk || !pe) return;
        pk->Odswiez();
        pe->Odswiez();

        // Menu: Magazyn i Widok istnieją, a pozycje są dostępne po połączeniu
        {
            wxMenuBar* pasek = okno->GetMenuBar();
            int magazyn = -1, widok = -1;
            for (size_t i = 0; i < pasek->GetMenuCount(); ++i) {
                const wxString t = pasek->GetMenuLabelText(i);
                if (t == "Magazyn") magazyn = static_cast<int>(i);
                if (t == "Widok") widok = static_cast<int>(i);
            }
            CHECK(magazyn >= 0);
            CHECK(widok >= 0);
            if (magazyn >= 0) {
                wxMenu* menu = pasek->GetMenu(static_cast<size_t>(magazyn));
                CHECK(menu->GetMenuItemCount() >= 10);
                bool jestNowy = false;
                for (wxMenuItem* pozycja : menu->GetMenuItems()) {
                    if (pozycja->GetItemLabelText().StartsWith("Nowy element")) jestNowy = true;
                }
                CHECK(jestNowy);
            }
        }
        CHECK(pk->WybranaKategoria() == std::nullopt);
        CHECK(pk->IsEnabled() && pe->IsEnabled());
        CHECK(okno->GetStatusBar()->GetStatusText(0).Contains(wxString::FromUTF8("Elementów")));

        // --- kategoria główna przez okienko wpisywania nazwy
        CallAfter([] {
            auto* d = dynamic_cast<wxTextEntryDialog*>(Gorne());
            CHECK(d != nullptr);
            if (d) { d->SetValue(wxString::FromUTF8("GUI_Zażółć")); d->EndModal(wxID_OK); }
        });
        pk->DodajKategorieGlowna();
        CHECK(Liczba(m, "SELECT count(*) FROM tbl_kategorie WHERE nazwa = 'GUI_Zażółć'") == 1);
        CHECK(logika.odswiezKategorie());
        long long idGlowna = 0;
        for (const Kategoria& k : logika.kategorie()) if (k.nazwa == "GUI_Zażółć") idGlowna = k.id;
        CHECK(idGlowna != 0);

        // --- podkategoria (rodzic = zaznaczona kategoria)
        pk->WybierzKategorie(idGlowna);
        CHECK(pk->WybranaKategoria() == std::optional<long long>(idGlowna));
        CHECK(pe->Kategoria() == std::optional<long long>(idGlowna));   // zaznaczenie przekazane do listy
        CallAfter([] {
            auto* d = dynamic_cast<wxTextEntryDialog*>(Gorne());
            CHECK(d != nullptr);
            if (d) { d->SetValue("GUI_Podkategoria"); d->EndModal(wxID_OK); }
        });
        pk->DodajPodkategorie();
        CHECK(Liczba(m, "SELECT count(*) FROM tbl_kategorie k JOIN tbl_kategorie r ON r.id = k.id_rodzica "
                        "WHERE k.nazwa = 'GUI_Podkategoria' AND r.nazwa = 'GUI_Zażółć'") == 1);
        logika.odswiezKategorie();
        long long idPod = 0;
        for (const Kategoria& k : logika.kategorie()) if (k.nazwa == "GUI_Podkategoria") idPod = k.id;
        CHECK(idPod != 0);

        // --- zmiana nazwy
        pk->WybierzKategorie(idPod);
        CallAfter([] {
            auto* d = dynamic_cast<wxTextEntryDialog*>(Gorne());
            CHECK(d != nullptr);
            if (d) { CHECK(d->GetValue() == "GUI_Podkategoria"); d->SetValue("GUI_Pod2"); d->EndModal(wxID_OK); }
        });
        pk->ZmienNazwe();
        CHECK(Liczba(m, "SELECT count(*) FROM tbl_kategorie WHERE nazwa = 'GUI_Pod2'") == 1);

        // --- nowy element: formularz (pierwsze pole tekstowe to symbol, pierwsze pokrętło to ilość)
        pk->WybierzKategorie(idPod);
        CallAfter([&] {
            wxWindow* dlg = Gorne();
            CHECK(dynamic_cast<DialogElementu*>(dlg) != nullptr);
            Zrzut(dlg, "06_dialog_elementu");
            auto* symbol = ZnajdzTyp<wxTextCtrl>(dlg);
            auto* ilosc = ZnajdzTyp<wxSpinCtrl>(dlg);
            CHECK(symbol != nullptr && ilosc != nullptr);
            if (symbol) symbol->SetValue("GUI_Rezystor 10k");
            if (ilosc) ilosc->SetValue(5);
            wxWindow* ok = Znajdz(dlg, "OK");
            CHECK(ok != nullptr);
            if (ok) Kliknij(ok);
        });
        pe->NowyElement();
        CHECK(pe->Elementy().size() == 1);
        CHECK(pe->MaWybor());                                   // nowy element jest zaznaczony
        if (!pe->Elementy().empty()) {
            const Element& e = pe->Elementy()[0];
            CHECK(e.symbol == "GUI_Rezystor 10k" && e.iloscDostepna == 5);
            CHECK(e.idKategorii == std::optional<long long>(idPod));   // kategoria z drzewa
        }
        CHECK(okno->GetStatusBar()->GetStatusText(0).Contains("1"));

        // --- wyszukiwanie
        pe->Szukaj("gui_REZ");
        CHECK(pe->Elementy().size() == 1);
        pe->Szukaj("nie_ma_takiego_xyz");
        CHECK(pe->Elementy().empty());
        CHECK(!pe->MaWybor());
        pe->Szukaj("");
        CHECK(pe->Elementy().size() == 1);
        pe->Szukaj("gui_rez");
        pe->WybierzPierwszy();
        CHECK(pe->MaWybor());

        // --- zmiana stanu: wydanie 3 sztuk
        CallAfter([&] {
            wxWindow* dlg = Gorne();
            CHECK(dynamic_cast<DialogZmianyIlosci*>(dlg) != nullptr);
            Zrzut(dlg, "07_zmiana_stanu");
            auto* wydanie = dynamic_cast<wxRadioButton*>(Znajdz(dlg, wxString::FromUTF8("Wydanie (odejmij od stanu)")));
            auto* ilosc = ZnajdzTyp<wxSpinCtrl>(dlg);
            CHECK(wydanie != nullptr && ilosc != nullptr);
            if (wydanie) wydanie->SetValue(true);
            if (ilosc) ilosc->SetValue(3);
            wxWindow* ok = Znajdz(dlg, "OK");
            CHECK(ok != nullptr);
            if (ok) Kliknij(ok);
        });
        pe->ZmienStanWybranego();
        CHECK(!pe->Elementy().empty() && pe->Elementy()[0].iloscDostepna == 2);
        CHECK(Liczba(m, "SELECT count(*) FROM tbl_transakcje t JOIN tbl_elementy e ON e.id = t.id_elementu "
                        "WHERE e.symbol = 'GUI_Rezystor 10k' AND t.typ_transakcji = 'WYDANIE' AND t.ilosc_zmiana = -3") == 1);

        // --- edycja: zmiana symbolu i ceny (z przecinkiem)
        CallAfter([&] {
            wxWindow* dlg = Gorne();
            CHECK(dynamic_cast<DialogElementu*>(dlg) != nullptr);
            auto* symbol = ZnajdzTyp<wxTextCtrl>(dlg);
            CHECK(symbol != nullptr);
            if (symbol) { CHECK(symbol->GetValue() == "GUI_Rezystor 10k"); symbol->SetValue("GUI_Rezystor 22k"); }
            // pola tekstowe w kolejności tworzenia: symbol, symbol producenta, producent, opis,
            // wartość, jednostka, obudowa, cena
            std::vector<wxTextCtrl*> pola;
            for (wxWindow* d : dlg->GetChildren()) if (auto* t = dynamic_cast<wxTextCtrl*>(d)) pola.push_back(t);
            CHECK(pola.size() >= 8);
            if (pola.size() >= 8) pola[7]->SetValue("0,35");
            wxWindow* ok = Znajdz(dlg, "OK");
            if (ok) Kliknij(ok);
        });
        pe->EdytujWybrany();
        CHECK(Liczba(m, "SELECT count(*) FROM tbl_elementy WHERE symbol = 'GUI_Rezystor 22k' AND cena_jednostkowa = 0.35 "
                        "AND ilosc_dostepna = 2") == 1);
        CHECK(Liczba(m, "SELECT count(*) FROM tbl_elementy WHERE symbol = 'GUI_Rezystor 10k'") == 0);

        // --- przeniesienie kategorii do poziomu głównego (logika używana przez przeciąganie)
        CHECK(logika.przeniesKategorie(idPod, std::nullopt));
        pk->Odswiez();
        pe->Odswiez();
        CHECK(!pe->Elementy().empty() && pe->Elementy()[0].kategoriaPelna == "GUI_Pod2");
        CHECK(logika.przeniesKategorie(idPod, idGlowna));

        // --- dane przykładowe do zrzutu ekranu
        auto idKat = [&](const std::string& pelna) -> std::optional<long long> {
            logika.odswiezKategorie();
            for (const Kategoria& k : logika.kategorie()) if (k.pelnaNazwa == pelna) return k.id;
            return std::nullopt;
        };
        struct Przyklad { const char* symbol; const char* producent; const char* opis; const char* wartosc;
                          const char* obudowa; const char* kat; int ilosc; int min; double cena; };
        const Przyklad przyklady[] = {
            {"GUI_RC0805-10k", "Yageo", "Rezystor 10 kΩ 1% 0,125 W", "10k", "0805", "Elementy bierne / Rezystory / Rezystory SMD", 480, 100, 0.012},
            {"GUI_MF-1k", "Vishay", "Rezystor metalizowany 1 kΩ 1% 0,6 W", "1k", "THT", "Elementy bierne / Rezystory / Rezystory THT", 12, 50, 0.08},
            {"GUI_Elko 100u/25V", "Panasonic", "Kondensator elektrolityczny 100 µF 25 V", "100u", "Ø6,3", "Elementy bierne / Kondensatory / Elektrolityczne", 34, 10, 0.45},
            {"GUI_X7R 100n", "Murata", "Kondensator ceramiczny 100 nF 50 V", "100n", "0603", "Elementy bierne / Kondensatory / Ceramiczne", 900, 200, 0.02},
        };
        for (const Przyklad& p : przyklady) {
            Element e;
            e.symbol = p.symbol; e.producent = p.producent; e.opis = p.opis; e.wartosc = p.wartosc;
            e.obudowa = p.obudowa; e.iloscDostepna = p.ilosc; e.iloscMinimalna = p.min; e.cena = p.cena;
            e.idKategorii = idKat(p.kat);
            CHECK(e.idKategorii.has_value());
            CHECK(logika.zapiszElement(e).has_value());
        }
        pk->Odswiez();
        pe->Szukaj("");
        pk->WybierzKategorie(idKat("Elementy bierne"));
        CHECK(pe->Elementy().size() >= 4);
        Zrzut(okno, "05_okno_z_danymi");
        pe->Szukaj("zolty"); // nic nie pasuje do przykładów
        CHECK(pe->Elementy().empty());
        pe->Szukaj("gui_ kondensator 100");
        CHECK(pe->Elementy().size() == 2);
        Zrzut(okno, "08_wyszukiwanie");
        pe->Szukaj("");

        // --- dziennik zdarzeń: każde zdarzenie zapisane dokładnie raz
        {
            wxString tekstDziennika;
            for (wxWindow* dziecko : okno->GetChildren()) {
                if (auto* t = dynamic_cast<wxTextCtrl*>(dziecko)) tekstDziennika = t->GetValue();
            }
            auto ile = [&](const wxString& fragment) {
                int n = 0;
                for (size_t poz = tekstDziennika.find(fragment); poz != wxString::npos;
                     poz = tekstDziennika.find(fragment, poz + 1)) ++n;
                return n;
            };
            CHECK(ile(wxString::FromUTF8("Dodano kategorię: GUI_Zażółć")) == 1);
            CHECK(ile(wxString::FromUTF8("Dodano podkategorię: GUI_Podkategoria")) == 1);
            CHECK(ile(wxString::FromUTF8("Zmieniono nazwę kategorii na: GUI_Pod2")) == 1);
            CHECK(ile(wxString::FromUTF8("Dodano element: GUI_Rezystor 10k")) == 1);
            CHECK(ile(wxString::FromUTF8("Zmieniono stan elementu GUI_Rezystor 10k: -3")) == 1);
            CHECK(ile(wxString::FromUTF8("Zapisano element: GUI_Rezystor 22k")) == 1);
        }

        // --- sprzątanie
        sprzatanie();
        pk->Odswiez();
        pe->Odswiez();
        CHECK(Liczba(m, "SELECT count(*) FROM tbl_elementy WHERE symbol LIKE 'GUI\\_%'") == 0);
    }

    template <class D> void PokazIZrzuc(D& d, const char* nazwa) {
        d.Show(); d.Centre(); d.Raise(); wxYield(); wxMilliSleep(300); wxYield();
        ZrzutEkranu(nazwa);
        d.Hide();
    }
    // audyt opisów pól: klucze użyte przez więcej niż jedną kontrolkę oraz kontrolki wejściowe bez opisu
    void AudytOkna(wxWindow* d, const char* nazwa) {
        std::map<std::string, int> ile;
        std::vector<std::string> bez;
        std::function<void(wxWindow*)> idz = [&](wxWindow* w) {
            for (wxWindow* ch : w->GetChildren()) {
                const std::string k = PomocGUI::KluczKontrolki(ch);
                const bool wejscie = wxDynamicCast(ch, wxTextCtrl) || wxDynamicCast(ch, wxChoice) || wxDynamicCast(ch, wxComboBox) ||
                                     wxDynamicCast(ch, wxCheckBox) || wxDynamicCast(ch, wxSlider) || wxDynamicCast(ch, wxSpinCtrl) ||
                                     wxDynamicCast(ch, wxRadioButton);
                if (!k.empty()) {
                    ile[k]++;
                    if (std::getenv("DM_AUDYT_LISTA")) {
                        wxString et;
                        const wxWindowList& rodzenstwo = w->GetChildren();
                        wxWindow* poprz = nullptr;
                        for (wxWindow* r : rodzenstwo) { if (r == ch) break; if (wxDynamicCast(r, wxStaticText)) poprz = r; }
                        if (poprz) et = ((wxStaticText*)poprz)->GetLabel();
                        if (wxDynamicCast(ch, wxCheckBox)) et = ((wxCheckBox*)ch)->GetLabel();
                        if (wxDynamicCast(ch, wxRadioButton)) et = ((wxRadioButton*)ch)->GetLabel();
                        std::cerr << "LISTA " << nazwa << " | " << PomocGUI::KluczPola(ch, k) << " | " << wxString(ch->GetClassInfo()->GetClassName()).utf8_string() << " | " << et.utf8_string() << std::endl;
                    }
                }
                else if (wejscie) bez.push_back(wxString(ch->GetClassInfo()->GetClassName()).utf8_string() + " @" + std::to_string(ch->GetPosition().x) + "," + std::to_string(ch->GetPosition().y));
                idz(ch);
            }
        };
        idz(d);
        std::cerr << "AUDYT " << nazwa << ":";
        for (auto& [k, n] : ile) if (n > 1) std::cerr << " " << k << "x" << n;
        std::cerr << "  | bez opisu: " << bez.size();
        for (auto& b : bez) std::cerr << " [" << b << "]";
        std::cerr << std::endl;
    }
    void Audyt(OknoGlowne* okno) {
        { DialogKodow d(okno); AudytOkna(&d, "kody"); }
        { DialogKalkulatorow d(okno); AudytOkna(&d, "kalkulatory"); }
        { DialogKalkulatorow d(okno, true); AudytOkna(&d, "cewki"); }
        { DialogAudioLampy d(okno); AudytOkna(&d, "audio"); }
        { DialogMetodyGraficzne d(okno); AudytOkna(&d, "grafika"); }
        { DialogZnieksztalcen d(okno); AudytOkna(&d, "znieksztalcenia"); }
        { DialogDlawika d(okno); AudytOkna(&d, "dlawik"); }
        { DialogTrafoSieciowy d(okno); AudytOkna(&d, "trafo_sieciowy"); }
        { DialogTrafoPP d(okno); AudytOkna(&d, "trafo_pp"); }
        { DialogTrafoSE d(okno); AudytOkna(&d, "trafo_se"); }
        { DialogCewkiAL d(okno); AudytOkna(&d, "cewka_al"); }
    }
    void Podglad(OknoGlowne* okno) {
        if (env("DM_TEST_PODGLAD", "") == "audyt") { Audyt(okno); return; }
        const std::string co = env("DM_TEST_PODGLAD", "");
        if (co.find("cewki") != std::string::npos) {
            wxDialog d(okno, wxID_ANY, "cewki", wxDefaultPosition, wxSize(560, 260));
            wxBoxSizer* s = new wxBoxSizer(wxHORIZONTAL);
            s->Add(new PanelSchematu(&d, PanelSchematu::Rodzaj::CewkaJedno), 0, wxALL, 5);
            s->Add(new PanelSchematu(&d, PanelSchematu::Rodzaj::CewkaWiele), 0, wxALL, 5);
            d.SetSizerAndFit(s);
            PokazIZrzuc(d, "p_cewki");
        }
        if (co.find("dlawik") != std::string::npos) { DialogDlawika d(okno); PokazIZrzuc(d, "p_dlawik"); }
        if (co.find("trafo") != std::string::npos) { DialogTrafoSieciowy d(okno); PokazIZrzuc(d, "p_trafo"); }
        if (co.find("se3") != std::string::npos) {
            DialogTrafoSE d(okno);
            d.UstawKsztaltke("EI102");
            if (DialogTrafoSEWykonane* e = d.OtworzWykonane()) PokazIZrzuc(*e, "p_trafose3");
        } else if (co.find("se") != std::string::npos) { DialogTrafoSE d(okno); PokazIZrzuc(d, "p_trafose"); }
        if (co.find("pp3") != std::string::npos) {
            DialogTrafoPP d(okno);
            d.UstawTyp(0); d.UstawPole("P0", "50"); d.UstawPole("Fd", "100"); d.UstawPole("Fg", "15000"); d.UstawPole("I0", "32");
            d.UstawKsztaltke("EI102");
            if (DialogTrafoPPEtap3* e = d.OtworzEtap3()) PokazIZrzuc(*e, "p_trafopp3");
        } else if (co.find("pp") != std::string::npos) { DialogTrafoPP d(okno); PokazIZrzuc(d, "p_trafopp"); }
        if (co.find("zas") != std::string::npos) {
            DialogKalkulatorow d(okno); d.WybierzStrone(3);
            PokazIZrzuc(d, "p_zasilacz0");
            d.UstawProstownik(1); PokazIZrzuc(d, "p_zasilacz1");
            d.UstawProstownik(2); PokazIZrzuc(d, "p_zasilacz2");
        }
        if (co.find("zn") != std::string::npos) {
            DialogZnieksztalcen d(okno); PokazIZrzuc(d, "p_zn_prady");
            d.UstawTryb(true); PokazIZrzuc(d, "p_zn_odcinki");
        }
        if (co.find("audio") != std::string::npos) { DialogAudioLampy d(okno); PokazIZrzuc(d, "p_audio"); }
    }

    void Uruchom() {
        // --- Okno główne łączy się samo ---
        auto* okno = new OknoGlowne("Magmass");
        okno->Show();
        for (int i = 0; i < 60 && !ManagerBazyDanych::pobierz().czyPolaczony(); ++i) {
            wxMilliSleep(100);
            wxYield();
        }
        wxYield();
        if (!env("DM_TEST_PODGLAD", "").empty()) {   // tylko zrzuty wybranych okien (podgląd układu), bez testów
            Podglad(okno);
            ExitMainLoop();
            return;
        }
        CHECK(ManagerBazyDanych::pobierz().czyPolaczony());
        CHECK(okno->GetStatusBar()->GetStatusText(1).Contains(wxString::FromUTF8(env("DM_TEST_DB", "dm_test"))));
        Zrzut(okno, "01_okno_glowne");

        // --- Dialog połączenia: test, zapis ---
        wxWindow* dlgPol = nullptr;
        CallAfter([&] {
            dlgPol = wxTopLevelWindows.GetLast()->GetData();
            Zrzut(dlgPol, "02_dialog_polaczenia");
            wxWindow* test = Znajdz(dlgPol, wxString::FromUTF8("Test połączenia"));
            CHECK(test != nullptr);
            if (test) Kliknij(test);
            CHECK(ZnajdzZawierajacy(dlgPol, wxString::FromUTF8("Połączenie udane")) != nullptr);
            Zrzut(dlgPol, "03_dialog_polaczenia_po_tescie");
            wxWindow* zapisz = Znajdz(dlgPol, wxString::FromUTF8("Zapisz ustawienia"));
            CHECK(zapisz != nullptr);
            std::cerr << "klik zapisz" << std::endl;
            if (zapisz) Kliknij(zapisz);
            std::cerr << "po zapisz" << std::endl;
            CHECK(ZnajdzZawierajacy(dlgPol, wxString::FromUTF8("zapisane")) != nullptr);
            wxWindow* ok = Znajdz(dlgPol, "OK");
            CHECK(ok != nullptr);
            std::cerr << "klik ok" << std::endl;
            if (ok) Kliknij(ok);
            std::cerr << "po ok" << std::endl;
        });
        {
            DialogPolaczenia dlg(okno);
            dlg.ShowModal();
        }

        // --- Dialog ustawień aplikacji: zmiana, zapis, odczyt ---
        CallAfter([&] {
            wxWindow* dlg = wxTopLevelWindows.GetLast()->GetData();
            Zrzut(dlg, "04_dialog_ustawien");
            wxWindow* ok = Znajdz(dlg, "OK");
            CHECK(ok != nullptr);
            std::cerr << "klik ok" << std::endl;
            if (ok) Kliknij(ok);
            std::cerr << "po ok" << std::endl;
        });
        {
            Konfiguracja& k = Konfiguracja::pobierz();
            UstawieniaBazowe us = k.pobierzUstawieniaAplikacji();
            us.sciezkaObrazki = "/tmp/zażółć/obrazy";
            us.prefixEAN = 27;
            us.nazwaFontu = "Verdana";
            us.pogrubienie = true;
            us.koloryProgow[3] = "#00FF00";
            k.ustawUstawieniaAplikacji(us);
            k.zapisz();

            DialogUstawien dlg(okno);
            dlg.ShowModal();
            CHECK(!dlg.ZmienionoPolaczenie());
        }
        {
            Konfiguracja& k = Konfiguracja::pobierz();
            k.wczytaj();
            const UstawieniaBazowe w = k.pobierzUstawieniaAplikacji();
            CHECK(w.sciezkaObrazki == "/tmp/zażółć/obrazy");
            CHECK(w.prefixEAN == 27 && w.nazwaFontu == "Verdana" && w.pogrubienie);
            CHECK(w.koloryProgow.count(3) == 1);        // OK nie gubi nieedytowanych ustawień
            // OK w oknie ustawień nie może skasować hasła do bazy z pliku
            ManagerBazyDanych::pobierz().wczytajUstawienia();
            CHECK(ManagerBazyDanych::pobierz().pobierzUstawienia().haslo ==
                  env("DM_TEST_PASSWORD", ""));
        }

        TestMagazynu(okno);
        TestProjektow(okno);
        TestRaportowIEtykiet(okno);
        TestSklepow(okno);
        TestPomocy(okno);
        TestMiejsca(okno);
        TestImportuElementow(okno);
        TestKodow(okno);
        TestKalkulatorow(okno);
        TestKamery(okno);
        TestEagle(okno);
        TestKatalogu(okno);
        TestObudow(okno);
        TestKsiazek(okno);
        TestNarzedzi(okno);
        TestSerwisu(okno);
        TestZbiorczychGUI(okno);
        TestOcrIMetadanych(okno);
        TestMenuIWydania(okno);
        TestOProgramie(okno);
        TestOcrDatasheetu(okno);
        TestMousera(okno);
        TestSortowaniaList(okno);
        TestAudioLampy(okno);
        TestMetodGraficznych(okno);
        TestZnieksztalcen(okno);
        TestDlawika(okno);
        TestTrafoSieciowy(okno);
        TestTrafoPP(okno);
        TestTrafoSE(okno);
        TestCewkiAL(okno);

        {   // obrazy pomocy wkompilowane w program: poprawne PNG, każdy użyty w pomocy
            static const char* wymagane[] = {"rdzen_ei.png", "okno_warstwy.png", "uklady_sekcji.png",
                                             "amperozwoje.png", "mesa_boogie_dr.png"};
            for (const char* nazwa : wymagane) {
                bool jest = false;
                for (const ObrazPomocy& o : ObrazyPomocy())
                    if (std::string(o.nazwa) == nazwa) {
                        jest = o.rozmiar > 1000 && o.dane[1] == 'P' && o.dane[2] == 'N' && o.dane[3] == 'G';
                        wxMemoryInputStream in(o.dane, o.rozmiar);
                        wxImage im(in, wxBITMAP_TYPE_PNG);
                        CHECK(im.IsOk() && im.GetWidth() > 300);
                    }
                CHECK(jest);
            }
        }

        okno->Close(true);
        ExitMainLoop();
    }
};

wxIMPLEMENT_APP_NO_MAIN(TestApp);

int main(int argc, char** argv) {
    const int wynik = wxEntry(argc, argv);
    std::cout << (bledy == 0 ? "GUI: wszystko OK" : "GUI: są błędy") << "\n";
    return wynik != 0 ? wynik : (bledy == 0 ? 0 : 1);
}
