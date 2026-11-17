#include "Logika/Kalkulatory.h"
#include "Logika/KalkulatoryAnalogowe.h"
#include "Logika/KalkulatoryAudio.h"
#include "Logika/KalkulatoryLampowe.h"
#include "Logika/SchematyLampowe.h"
#include "Logika/MetodyGraficzne.h"
#include "Logika/Znieksztalcenia.h"
#include "Logika/Dlawik.h"
#include "Narzedzia/InfirayUsb.h"
#include "Narzedzia/XthermDll.h"
#include "Logika/TrafoSieciowy.h"
#include "Logika/TrafoPP.h"
#include "Logika/Rogowski.h"
#include "Logika/TrafoSE.h"
#include "Logika/TrafoModel.h"
#include "Logika/CewkaAL.h"
#include "Logika/Termowizja.h"
#include "Logika/MasaDrutu.h"
#include "Logika/CewkaPowietrzna.h"
#include "Logika/StabilizatorRegulowany.h"
#include <algorithm>
#include "Logika/tube_audio.hpp"
#include <cmath>
#include <iostream>

static int bledy = 0, sprawdzenia = 0;
#define CHECK(w) do { ++sprawdzenia; if (!(w)) { ++bledy; std::cerr << "BŁĄD " << __FILE__ << ":" << __LINE__ << "  " #w "\n"; } } while (0)
static bool bl(double a, double b, double tol = 1e-6) { return std::fabs(a - b) <= tol * std::fabs(b) + 1e-12; }

using namespace Kalkulatory;

int main() {
    // szeregi
    CHECK(bl(najblizsza(4800, Szereg::E24), 4700) || bl(najblizsza(4800, Szereg::E24), 5100));
    CHECK(bl(najblizsza(4700, Szereg::E24), 4700));
    CHECK(bl(najblizszaWieksza(4800, Szereg::E24), 5100));
    CHECK(bl(najblizszaWieksza(470, Szereg::E12), 470));
    CHECK(bl(najblizszaWieksza(0.5, Szereg::E12), 0.56));
    CHECK(najblizsza(-1, Szereg::E12) == 0);

    // dzielnik
    auto u = napiecieDzielnika(12, 10000, 10000);
    CHECK(u && bl(*u, 6));
    CHECK(!napiecieDzielnika(12, 1000, 0));
    auto d = dobierzDzielnik(5, 3.3, Szereg::E24);
    CHECK(d && std::fabs(d->bladProc) < 1.0);
    if (d) CHECK(bl(d->uwy, 5 * d->r2 / (d->r1 + d->r2)) && d->r1 >= 1e3 && d->r1 <= 1e6);
    auto d96 = dobierzDzielnik(5, 3.3, Szereg::E96);
    CHECK(d96 && std::fabs(d96->bladProc) < 0.5);
    if (d) CHECK(d->r1 + d->r2 >= 5e3 && d->r1 + d->r2 <= 100e3);   // rozsądny prąd, nie megaomy
    CHECK(!dobierzDzielnik(5, 6, Szereg::E24));
    CHECK(!dobierzDzielnik(5, 0, Szereg::E24));

    // LED: 5 V, 2 V, 20 mA -> 150 Ω
    auto l = rezystorLED(5, 2, 20);
    CHECK(l && bl(l->rIdealny, 150) && bl(l->rZSzeregu, 150) && bl(l->pradRzeczywistyMA, 20));
    if (l) CHECK(bl(l->mocRezystoraMW, 60) && bl(l->mocLEDMW, 40));
    auto l2 = rezystorLED(5, 2.1, 20);                      // 145 Ω -> 150 Ω
    CHECK(l2 && bl(l2->rZSzeregu, 150) && l2->pradRzeczywistyMA < 20);
    CHECK(!rezystorLED(2, 2, 20) && !rezystorLED(3, 2, 0));

    // RC: 10 k i 100 nF -> 159,15 Hz, tau 1 ms
    CHECK(bl(czestotliwoscGranicznaRC(10e3, 100e-9), 159.1549, 1e-4));
    CHECK(bl(stalaCzasowaRC(10e3, 100e-9), 1e-3));
    CHECK(bl(rDlaFiltru(159.1549, 100e-9), 10e3, 1e-4));
    CHECK(bl(cDlaFiltru(159.1549, 10e3), 100e-9, 1e-4));
    CHECK(czestotliwoscGranicznaRC(0, 1e-9) == 0);

    CHECK(formatCzestotliwosci(159.15) == "159 Hz");
    CHECK(formatCzestotliwosci(1591.5) == "1,59 kHz");
    CHECK(formatCzestotliwosci(15915) == "15,9 kHz");
    CHECK(formatCzestotliwosci(2.2e6) == "2,2 MHz");
    CHECK(formatCzasu(0.001) == "1 ms");
    CHECK(formatCzasu(4.7e-6) == "4,7 \xC2\xB5s");
    CHECK(formatCzasu(1.2) == "1,2 s");

    // zasilacz: 18 V wtórne, mostek, 1 A, 4700 uF, 50 Hz
    auto z = zasilaczZTrafa(18, 1.0, 4700e-6, Prostownik::Mostek);
    CHECK(z && bl(z->uSzczytowa, 18 * std::sqrt(2.0) - 1.4) && bl(z->czestTetnien, 100) && bl(z->tetnieniaPP, 1.0 / (100 * 4700e-6)));
    if (z) CHECK(bl(z->uSrednia, z->uSzczytowa - z->tetnieniaPP / 2) && z->mocTrafoVA > 18 && z->pradSkokowyDiodyA > 1.0);
    auto zj = zasilaczZTrafa(18, 1.0, 4700e-6, Prostownik::Jednopolowkowy);
    CHECK(zj && bl(zj->czestTetnien, 50) && zj->tetnieniaPP > z->tetnieniaPP * 1.9);
    CHECK(!zasilaczZTrafa(0, 1, 1e-3, Prostownik::Mostek) && !zasilaczZTrafa(1, 1, 1e-3, Prostownik::Mostek));   // 1 V: szczyt ujemny
    auto uw = napiecieWtorneDlaWyjscia(12, 1.0, Prostownik::Mostek);
    CHECK(uw && bl(*uw, (12 + 0.5 + 1.4) / std::sqrt(2.0)));
    CHECK(bl(kondensatorFiltra(1.0, 1.0, Prostownik::Mostek), 0.01));
    CHECK(bl(kondensatorFiltra(1.0, 1.0, Prostownik::Jednopolowkowy), 0.02));
    CHECK(kondensatorFiltra(0, 1, Prostownik::Mostek) == 0);

    // termowizja: układ ramki, temperatury, statystyki, palety
    {
        using namespace Termowizja;
        const KlatkaSurowa k = klatkaTestowa(0);
        const auto u = rozpoznaj(k);
        CHECK(u.uklad == Uklad::ObrazIDane && u.obrazSzer == 256 && u.obrazWys == 192);
        const auto d = daneRadiometryczne(k, u);
        CHECK(d.size() == 256u * 192u);
        CHECK(bl(naCelsjusza(static_cast<uint16_t>(std::lround((36.6 + 273.15) * 64)), 64, 273.15), 36.6, 1e-3));
        CHECK(naCelsjusza(100, 0) == 0);
        const auto st = statystyki(d, 256, 192);
        CHECK(st.ok && st.minC > 19.9 && st.minC < 21 && st.maxC > 55 && st.maxC < 85 && st.maxC > st.minC);
        CHECK(st.sredniaC > st.minC && st.sredniaC < st.maxC && st.xMin < 5);        // najchłodniej po lewej
        const auto st2 = statystyki(d, 256, 192, 128.0);                              // zły dzielnik: inne wartości
        CHECK(st2.ok && std::fabs(st2.maxC - st.maxC) > 10);
        CHECK(!statystyki(d, 256, 192, 0).ok && !statystyki({}, 256, 192).ok);
        KlatkaSurowa k2; k2.szer = 320; k2.wys = 240; k2.kanaly = 3; k2.dane.assign(320u * 240u * 3u, 7);
        CHECK(rozpoznaj(k2).uklad == Uklad::TylkoObraz && daneRadiometryczne(k2, rozpoznaj(k2)).empty());
        KlatkaSurowa k3; k3.szer = 256; k3.wys = 384; k3.kanaly = 3; k3.dane.assign(256u * 384u * 3u, 1);
        CHECK(rozpoznaj(k3).uklad == Uklad::TylkoObraz);                              // 3 bajty na piksel: to nie YUYV
        CHECK(rozpoznaj(KlatkaSurowa()).uklad == Uklad::TylkoObraz);
        unsigned char c[3];
        kolorPalety(Paleta::Szara, 0, c); CHECK(c[0] == 0);
        kolorPalety(Paleta::Szara, 2, c); CHECK(c[0] == 255);
        kolorPalety(Paleta::SzaraOdwrocona, 1, c); CHECK(c[0] == 0);
        kolorPalety(Paleta::Teczowa, 0, c); CHECK(c[2] == 255 && c[0] == 0);          // zimno: niebieski
        kolorPalety(Paleta::Teczowa, 1, c); CHECK(c[0] == 255 && c[2] == 0);          // gorąco: czerwony
        kolorPalety(Paleta::Zelazo, 0, c); CHECK(c[0] == 0 && c[1] == 0 && c[2] == 0);
        kolorPalety(Paleta::Zelazo, 1, c); CHECK(c[0] == 255 && c[2] == 255);
        CHECK(obrazZDanych(d, 256, 192, Paleta::Zelazo).size() == 256u * 192u * 3u);
        CHECK(obrazZJasnosci(k, u, Paleta::Teczowa).size() == 256u * 192u * 3u);
        CHECK(obrazZJasnosci(k2, rozpoznaj(k2), Paleta::Szara).size() == 320u * 240u * 3u);
        const std::string r = raportDiagnostyczny(k);
        CHECK(r.find("256 x 384") != std::string::npos && r.find("°C") != std::string::npos && r.find("Pierwsze 16 bajtów") != std::string::npos);
        CHECK(raportDiagnostyczny(KlatkaSurowa()).find("Brak") != std::string::npos);
        CHECK(raportDiagnostyczny(k2).find("nie wygląda") != std::string::npos);
        CHECK(klatkaTestowa(5).dane != klatkaTestowa(0).dane);                        // gorący punkt się porusza
    }

    // ESR: tg delta typowy i ESR = tgd / (2 pi f C)
    CHECK(bl(tgDeltaTypowy(16), 0.16) && bl(tgDeltaTypowy(1), 0.22) && bl(tgDeltaTypowy(1000), 0.20));
    CHECK(bl(tgDeltaTypowy(30), 0.13));                       // srodek miedzy 25 V (0,14) i 35 V (0,12)
    CHECK(bl(esrZTgDelta(0.16, 1000e-6), 0.16 / (2 * 3.14159265358979 * 120 * 1000e-6), 1e-9));
    CHECK(bl(esrTypowy(1000e-6, 16), 0.2122, 2e-3));
    CHECK(esrZTgDelta(0.1, 0) == 0 && esrZTgDelta(0, 1e-6) == 0 && tgDeltaTypowy(0) == 0);
    CHECK(esrTypowy(2000e-6, 16) < esrTypowy(1000e-6, 16));   // wieksze C: mniejszy ESR
    CHECK(bl(mocStratESR(2, 0.25), 1.0) && mocStratESR(-1, 1) == 0);

    // tranzystor: Vcc 12, R1 47k, R2 10k, RC 2k2, RE 470, beta 100
    auto tr = punktPracyTranzystora(12, 47e3, 10e3, 2200, 470, 100);
    CHECK(tr && tr->stan == StanTranzystora::Aktywny);
    if (tr) {
        CHECK(bl(tr->vceV, 12 - tr->icMA * 1e-3 * 2200 - tr->veV, 1e-6) || bl(tr->vceV, 12 - tr->icMA * 1e-3 * 2200 - (tr->icMA * 1e-3 + tr->ibUA * 1e-6) * 470, 1e-6));
        CHECK(tr->icMA > 0.5 && tr->icMA < 3 && tr->vceV > 1 && tr->vceV < 12 && bl(tr->vbV, tr->veV + 0.65));
    }
    auto tn = punktPracyTranzystora(12, 4.7e3, 10e3, 2200, 10, 100);   // za duzy prad bazy -> nasycenie
    CHECK(tn && tn->stan == StanTranzystora::Nasycenie && bl(tn->vceV, 0.2));
    auto tz = punktPracyTranzystora(12, 47e3, 1e3, 2200, 470, 100);    // Vb < Vbe -> zatkany
    CHECK(tz && tz->stan == StanTranzystora::Zatkany && tz->icMA == 0 && bl(tz->vceV, 12));
    CHECK(!punktPracyTranzystora(12, 0, 10e3, 2200, 470, 100));
    auto pt = dobierzTranzystor(12, 2.0, 150);
    CHECK(pt && pt->wynik.stan == StanTranzystora::Aktywny);
    if (pt) CHECK(std::fabs(pt->wynik.icMA - 2.0) < 0.5 && std::fabs(pt->wynik.vceV - 6.0) < 1.5);
    CHECK(!dobierzTranzystor(12, 0, 100));

    // lampa: trioda ECC83: B+ 250 V, Ra 100 k, Ia 0,8 mA, Ug -1,5 V, mi 100, ra 62,5 k
    DaneLampy dl; dl.bPlus = 250; dl.ra = 100e3; dl.iaMA = 0.8; dl.ugV = -1.5; dl.mi = 100; dl.rWewnAnodowaOm = 62.5e3; dl.paMaxW = 1.0; dl.fDolnaHz = 20;
    auto lp = punktPracyLampy(dl);
    CHECK(lp && bl(lp->rKatodyOm, 1.5 / 0.8e-3) && bl(lp->uAnodyV, 250 - 80 - 1.5) && !lp->przekroczonaMocAnody);
    if (lp) CHECK(bl(lp->wzmocnienie, 100 * 100.0 / 162.5) && lp->ckF > 1e-6 && lp->ckF < 1e-3 && bl(lp->iaMaxMA, 2.5) && lp->rEkranuOm == 0);
    // pentoda EL84: B+ 250, Ra 5k2, Ia 48 mA, Ig2 5 mA, Ug -7,3, Ug2 250
    DaneLampy dp; dp.bPlus = 300; dp.ra = 5200; dp.iaMA = 48; dp.ig2MA = 5; dp.ugV = -7.3; dp.ug2V = 250; dp.paMaxW = 12;
    auto lpp = punktPracyLampy(dp);
    CHECK(lpp && bl(lpp->rKatodyOm, 7.3 / 0.053) && lpp->rEkranuOm > 0 && bl(lpp->rEkranuOm, (300 - 250 - 7.3) / 0.005));
    if (lpp) CHECK(bl(lpp->mocAnodyW, lpp->uAnodyV * 0.048) && !lpp->przekroczonaMocAnody);
    dp.iaMA = 80; dp.ra = 1000;
    auto lpm = punktPracyLampy(dp);
    CHECK(lpm && lpm->przekroczonaMocAnody);
    DaneLampy zla = dl; zla.ra = 500e3;                                // spadek > B+
    CHECK(!punktPracyLampy(zla));
    zla = dl; zla.ugV = 1;
    CHECK(!punktPracyLampy(zla));


    // --- ECCkalkulator: decybele, nepery
    {
        using namespace KalkulatoryAudio;
        CHECK(bl(*dbNapiec(100, 50), 6.0206, 1e-4));
        CHECK(bl(*dbNapiec(10, 1), 20, 1e-9));
        CHECK(bl(*dbMocy(100, 1), 20, 1e-9));
        CHECK(bl(*dbMocy(2, 1), 3.0103, 1e-4));
        CHECK(bl(*neperyNapiec(std::exp(1.0), 1), 1, 1e-9));
        CHECK(bl(*neperyMocy(std::exp(2.0), 1), 1, 1e-9));
        CHECK(bl(dbZNeperow(1), 8.685890, 1e-6) && bl(neperyZDb(8.685890), 1, 1e-6));
        CHECK(bl(neperyZDb(dbZNeperow(0.37)), 0.37, 1e-12));
        CHECK(!dbNapiec(0, 1) && !dbNapiec(1, -1) && !dbMocy(1, 0) && !dbZMiaryLiniowej(0, false));
        CHECK(bl(*dbZMiaryLiniowej(2, false), 6.0206, 1e-4) && bl(*dbZMiaryLiniowej(2, true), 3.0103, 1e-4));
        CHECK(bl(miaraLiniowaZDb(20, false), 10, 1e-9) && bl(miaraLiniowaZDb(20, true), 100, 1e-9));
        CHECK(bl(miaraLiniowaZDb(*dbZMiaryLiniowej(3.7, false), false), 3.7, 1e-9));

        // --- krzywa RIAA: wartości z normy (odtwarzanie, względem 1 kHz), dokładność ok. 0,1 dB
        auto r = [](double f) { return tlumienieRIAAdB(f, true, false); };
        CHECK(std::fabs(r(1000)) < 1e-9);
        CHECK(std::fabs(r(20) - 19.274) < 0.05);
        CHECK(std::fabs(r(50) - 16.946) < 0.05);
        CHECK(std::fabs(r(100) - 13.088) < 0.05);
        CHECK(std::fabs(r(500) - 2.648) < 0.05);
        CHECK(std::fabs(r(2000) - (-2.588)) < 0.05);
        CHECK(std::fabs(r(5000) - (-8.21)) < 0.05);
        CHECK(std::fabs(r(10000) - (-13.734)) < 0.05);
        CHECK(std::fabs(r(20000) - (-19.62)) < 0.05);
        // nagrywanie jest odwrotnością odtwarzania
        for (double f : {20.0, 100.0, 1000.0, 7000.0, 20000.0})
            CHECK(std::fabs(tlumienieRIAAdB(f, true) + tlumienieRIAAdB(f, false)) < 1e-9);
        // IEC: dodatkowe tłumienie poniżej 20 Hz, bez zmian powyżej kilkuset Hz
        CHECK(tlumienieRIAAdB(20, true, true) < tlumienieRIAAdB(20, true, false) - 2.5);
        CHECK(std::fabs(tlumienieRIAAdB(1000, true, true)) < 1e-9);
        CHECK(std::fabs(tlumienieRIAAdB(2000, true, true) - tlumienieRIAAdB(2000, true, false)) < 0.1);
        // faza i tabela
        const auto tab = tabelaRIAA(0, 100, 10);
        CHECK(tab.size() == 11 && tab.front().fHz == 0 && tab.back().fHz == 100);
        CHECK(tab.front().db > 19.5 && tab.front().db < 21.0);
        CHECK(std::fabs(tab.back().db - r(100)) < 1e-9);
        CHECK(tabelaRIAA(10, 5, 1).empty() && tabelaRIAA(0, 100, 0).empty() && tabelaRIAA(0, 1e9, 1).size() == 5000);
        const auto lg = czestotliwosciLogarytmiczne(20, 20000, 10);
        CHECK(lg.size() == 31 && lg.front() == 20 && std::fabs(lg.back() - 20000) < 1e-6);
        CHECK(czestotliwosciLogarytmiczne(0, 100, 10).empty());

        // --- bierna sieć RIAA: układ wynikowy ma dokładnie krzywą RIAA (po wyrównaniu przy 1 kHz)
        const auto s = projektujSiecRIAA(100e3, 10e3);
        CHECK(s.has_value());
        if (s) {
            CHECK(bl(s->r1, 900e3) && bl(s->c1, kTau2 / 900e3) && bl(s->c3, 7.5e-9));
            CHECK(maksOdchylkaSieci(*s) < 1e-6);
            // półka 3180/318 µs tłumi 20 dB przy małych f; krzywa RIAA przy 1 kHz leży ok. 20 dB niżej, więc sieć daje tu ok. -40 dB
            CHECK(std::fabs(std::abs(odpowiedzSieci(*s, 1.0)) - 0.1) < 0.002);
            const double t = tlumienieSieciPrzy1kHz(*s);
            CHECK(t < -39.0 && t > -41.0);
            // po zaokrągleniu do szeregów: odchyłka rośnie, ale E96/E24 daje < 0,5 dB, a E12/E6 mniej dokładnie
            const SiecRIAA z96 = zaokraglijSiec(*s, Szereg::E96, Szereg::E24);
            const SiecRIAA z12 = zaokraglijSiec(*s, Szereg::E12, Szereg::E12);
            CHECK(maksOdchylkaSieci(z96) < 0.5);
            CHECK(maksOdchylkaSieci(z96) > 1e-6);
            CHECK(maksOdchylkaSieci(z12) >= maksOdchylkaSieci(z96) - 1e-9);
            // niezależna kontrola: szereg-bocznik policzone wprost z impedancji dla kilku częstotliwości
            for (double f : {20.0, 500.0, 2122.0, 20000.0}) {
                const std::complex<double> sj(0, 2 * 3.14159265358979323846 * f);
                const auto z1 = 1.0 / (1.0 / s->r1 + sj * s->c1);
                const auto z2 = 1.0 / (1.0 / s->r2 + sj * s->c2);
                const auto h = z2 / (z1 + z2) / (1.0 + sj * s->r3 * s->c3);
                CHECK(std::abs(h - odpowiedzSieci(*s, f)) < 1e-12);
            }
        }
        CHECK(!projektujSiecRIAA(0, 10e3) && !projektujSiecRIAA(100e3, -1));
        // różne R2: kształt ten sam
        for (double r2 : {10e3, 47e3, 220e3, 1e6}) {
            const auto s2 = projektujSiecRIAA(r2, 4.7e3);
            CHECK(s2 && maksOdchylkaSieci(*s2) < 1e-6 && s2->c2 > 0 && s2->c1 > 0);
        }
    }

    // ---------------------------------------------------------------- układy lampowe (przykłady z ECCLab 7.2)
    {
        using namespace KalkulatoryLampowe;
        auto bl = [](double a, double b, double tol) { return std::abs(a - b) <= tol; };
        const Trioda e83{1.6, 100, 62.5};
        // sprzężenie RC: 3 przykłady
        {
            auto r = sprzezenieRC({0.45, 1.1, 1.6, 220, 0.47, 100, 470, e83, true});
            CHECK(r && bl(r->ku, 42.85, 0.005) && bl(r->rwyk, 47.12, 0.005) && bl(r->fdHz, 1.38, 0.005) && r->fgPrzyblizona);
            r = sprzezenieRC({0.45, 1.1, 1.6, 100, 0.47, 220, 470, e83, true});
            CHECK(r && bl(r->ku, 57.68, 0.005) && bl(r->rwyk, 63.43, 0.005) && bl(r->fdHz, 2.93, 0.005));
            r = sprzezenieRC({0.45, 1.1, 1.6, 10, 0.47, 100, 470, e83, false});
            CHECK(r && bl(r->ku, 56.88, 0.005) && bl(r->rwyk, 35.55, 0.005) && bl(r->fdHz, 31.30, 0.005) &&
                  bl(r->fgKHz, 67.00, 0.05) && !r->fgPrzyblizona);
            CHECK(!sprzezenieRC({0.45, 1.1, 1.6, 0, 0.47, 100, 470, e83, true}));   // Cs = 0
        }
        // wtórnik katodowy: 4 lampy (Csk, Sa, Ka, Rwew) -> Rwe, Rwy, K, fg, Umax
        {
            struct P { double csk; Trioda l; double rwe, rwy, ku, fg, umax; } p[] = {
                {1.6, {1.6, 100, 62.5}, 14542.75, 0.625, 0.98, 161.30, 264.998},
                {1.8, {2.2, 17, 7.7}, 6413.13, 0.455, 0.94, 196.44, 93.764},
                {2.3, {5.5, 60, 11}, 15778.55, 0.182, 0.98, 382.15, 298.273},
                {3.1, {12.5, 33, 2.6}, 11578.96, 0.080, 0.97, 643.02, 193.718}};
            for (const auto& e : p) {
                auto r = wtornikKatodowy({e.csk, 0.47, 47, 470, e.l, 6});
                CHECK(r && bl(r->rwe, e.rwe, 0.006) && bl(r->rwyk, e.rwy, 0.0006) && bl(r->ku, e.ku, 0.006) &&
                      bl(r->umax, e.umax, 0.001) && std::abs(r->fgMHz / e.fg - 1) < 1e-3);
            }
        }
        // wtórnik anodowy: 4 lampy
        {
            struct P { Trioda l; double rwy, ku; } p[] = {{{5.5, 60, 11}, 0.177, 0.703}, {{2.2, 17, 7.7}, 0.405, 0.480},
                                                          {{1.6, 100, 62.5}, 0.613, 0.426}, {{12.5, 33, 2.6}, 0.074, 0.814}};
            for (const auto& e : p) {
                auto r = wtornikAnodowy({470, 0.47, e.l});
                CHECK(r && bl(r->rwe, 470, 1e-9) && bl(r->rwyk, e.rwy, 0.0006) && bl(r->ku, e.ku, 0.0006));
            }
        }
        // inwerter różnicowy: 4 lampy
        {
            struct P { double ka, rw, q, sr, k1, k2; } p[] = {{60, 11, 1.1356, 26.469, 28.149, 24.789},
                                                              {17, 7.7, 1.4432, 7.774, 9.184, 6.364},
                                                              {100, 62.5, 1.1270, 28.442, 30.140, 26.743},
                                                              {55, 9, 1.1445, 24.794, 26.465, 23.123}};
            for (const auto& e : p) {
                auto r = inwerterRoznicowy({11.3, 100, 470, {0, e.ka, e.rw}});
                CHECK(r && bl(r->q, e.q, 6e-5) && bl(r->kuSr, e.sr, 6e-4) && bl(r->ku1, e.k1, 6e-4) && bl(r->ku2, e.k2, 6e-4));
            }
        }

        // układy złożone z modułu tube_audio (jednostki kΩ / pF), własności i zgodność z funkcjami modułu
        {
            const Trioda t{0, 60, 11};
            auto m = muWtornik({t, t, 2.2, 0, 1.6, 1.6, 10});
            auto m2 = muWtornik({t, t, 2.2, 100, 1.6, 1.6, 10});
            CHECK(m && m2 && m->ku < t.ka && m->ku > 0.8 * t.ka && m2->ku < m->ku && bl(m->rwyk, (11 + 2.2) / 61, 1e-9));
            CHECK(!muWtornik({{0, 1, 11}, t, 2.2, 0, 1, 1, 1}));                   // Ka <= 1
            const Trioda u{0, 20, 7.7};
            auto sr = srpp({u, 0, 0});
            CHECK(sr && bl(sr->ku0, 10.0, 1e-9) && bl(sr->ku, sr->ku0, 1e-9));   // bez Rk: mu/2
            auto sr2 = srpp({u, 1, 100});
            CHECK(sr2 && sr2->ku < sr2->ku0 && sr2->rlOpt > 0 && sr2->rwyk > 0);
            auto wh = wtornikWhite({u, u, 10});
            const auto wh0 = tube::calcWhiteCathodeFollower(tube::TriodeParams::fromKaRho(20, 7.7e3),
                                                            tube::TriodeParams::fromKaRho(20, 7.7e3), 10e3);
            CHECK(wh && bl(wh->rwyk, wh0.Rwy / 1e3, 1e-9) && wh->ku > 0.9 && wh->ku < 1);
            auto ks = wtornikKaskodowy({t, t, 10, 100});
            CHECK(ks && ks->ku < 1 && ks->ku > 0.9 && ks->ku <= ks->ku0 && ks->rwyk < 0.1);
            auto s1 = superwtornik({t, 10, 1});
            auto s2 = superwtornik({t, 10, 2});
            CHECK(s1 && s2 && s2->ku1 > s1->ku1 && s2->rwyk < s1->rwyk && s1->ku1 < 1);
            CHECK(!superwtornik({t, 10, 0}) && !superwtornik({t, 10, 7}) && !superwtornik({t, 0, 1}));
        }
        // moduł tube_audio: LTP bez obciążenia = ścisłe rozwiązanie węzłowe (poprawiony licznik napięcia katody)
        {
            const auto L = tube::TriodeParams::fromKaRho(60, 11e3);
            const auto t = tube::calcLTPTriode(L, L, 100e3, 100e3, 11.3e3);
            CHECK(bl(t.ku1, 29.041, 0.001) && bl(t.ku2, 25.013, 0.001));
            const auto d = tube::calcLTPDiff(t);
            CHECK(d.Ad > 0 && d.Acm > 0 && d.CMRR > 1);
            // SRPP bez Rk: ku0 = mu/2
            const auto sr = tube::calcSRPP(L, L, 0.0);
            CHECK(bl(sr.ku0, 30.0, 1e-9));
        }
    }

    // schematy układów lampowych: każdy układ ma schemat, a wyróżnione są elementy z pól wejściowych
    {
        using namespace SchematyLampowe;
        auto ma = [](const Schemat& sc, const char* e) {
            auto u = sc.uzyte();
            return std::find(u.begin(), u.end(), e) != u.end();
        };
        for (int i = 0; i <= 8; i++) {
            const auto sc = zbuduj(i);
            CHECK(sc.elementy.size() > 15 && !sc.podpis.empty());
            const int triod = static_cast<int>(std::count_if(sc.elementy.begin(), sc.elementy.end(),
                                                              [](const ElementRysunku& e) { return e.typ == Typ::Trioda; }));
            CHECK(triod == (i == 1 || i == 3 || (i >= 4 && i <= 8) ? 2 : 1));
            for (const auto& e : sc.elementy)   // wszystko mieści się w polu rysunku
                CHECK(e.x1 >= 0 && e.x1 <= kSzer && e.y1 >= 0 && e.y1 <= kWys && e.x2 >= 0 && e.x2 <= kSzer && e.y2 >= 0 && e.y2 <= kWys);
        }
        CHECK(zbuduj(99).elementy.empty());
        const auto rc = zbuduj(0, true), rcs = zbuduj(0, false);
        CHECK(ma(rc, "Ra") && ma(rc, "Rs") && ma(rc, "Cs") && ma(rc, "Rk"));
        CHECK(!rcs.maEtykiete("Rk") && ma(rcs, "Ra"));                          // polaryzacja stała: bez Rk
        CHECK(ma(zbuduj(1), "Rs") && ma(zbuduj(1), "Ro") && ma(zbuduj(1), "Cs") && zbuduj(1).maEtykiete("Ck") && !ma(zbuduj(1), "Ck"));
        CHECK(ma(zbuduj(2), "Rs") && ma(zbuduj(2), "Rk") && ma(zbuduj(2), "R"));
        CHECK(ma(zbuduj(3), "Ra") && ma(zbuduj(3), "Rs") && ma(zbuduj(3), "Rk"));
        CHECK(ma(zbuduj(4), "Rk2") && ma(zbuduj(4), "R0") && ma(zbuduj(5), "RL") && ma(zbuduj(6), "RL"));
        CHECK(ma(zbuduj(7), "Ra") && ma(zbuduj(7), "R0") && ma(zbuduj(8), "Ra") && !zbuduj(8).maEtykiete("R0"));
    }

    // metody graficzne: skalowanie obrazu, prosta obciążenia, Pa max
    {
        using namespace MetodyGraficzne;
        auto bl = [](double a, double b, double t) { return std::abs(a - b) <= t; };
        // obraz 1000 x 800: (0,0) w (100,700); 500 V w x = 900; 300 mA w y = 100
        const auto k = kalibruj({100, 700}, {900, 650}, 500, {95, 100}, 300);
        CHECK(k && bl(k->voltNaPx, 0.625, 1e-12) && bl(k->mANaPx, 0.5, 1e-12));
        const auto u = k->wartosc({500, 400});
        CHECK(bl(u.u, 250, 1e-9) && bl(u.i, 150, 1e-9));
        const auto px = k->piksel({250, 150});
        CHECK(bl(px.x, 500, 1e-9) && bl(px.y, 400, 1e-9));
        CHECK(!kalibruj({100, 700}, {50, 700}, 500, {100, 100}, 300));    // napięcie na lewo od zera
        CHECK(!kalibruj({100, 700}, {900, 700}, 500, {100, 800}, 300));   // prąd pod zerem
        CHECK(!kalibruj({100, 700}, {900, 700}, 0, {100, 100}, 300));
        // wynik jak w przykładzie ECCLab: 235,7 mA na osi prądu, 419,6 V i 0,9 mA na osi napięcia
        const auto w = policz({{0, 235.7}, {419.6, 0.9}});
        CHECK(w && bl(w->deltaU, 419.6, 1e-9) && bl(w->deltaI, 234.8, 1e-9) && bl(w->raA, 1.787, 1e-3) && w->u1PoLewejOdOsi);
        CHECK(w && bl(w->raaB, 4 * w->raA, 1e-12) && bl(w->mocSE, 12.315, 5e-3) && bl(w->mocPP, 4 * w->mocSE, 1e-12));
        // 1 kΩ, ΔI = 8,9 mA: moc SE około 0,01 W (jak w oknie ECCLab)
        const auto w2 = policz({{0, 8.9}, {8.9, 0}});
        CHECK(w2 && bl(w2->raA, 1.0, 1e-12) && bl(w2->mocSE, 0.0099, 1e-4) && bl(w2->raaB, 4.0, 1e-12));
        CHECK(!policz({{100, 50}, {80, 0}}) && !policz({{0, 10}, {50, 10}}));
        // przesuwanie: ograniczenia
        Prosta p{{0, 200}, {400, 0}};
        auto a = przesun(p, Koniec::Napieciowy, {-50, 30}, {});
        CHECK(bl(a.p2.u, 0, 1e-12) && bl(a.p2.i, 30, 1e-12));                // nie na lewo od końca prądowego
        a = przesun(p, Koniec::Napieciowy, {300, 500}, {});
        CHECK(bl(a.p2.i, 200, 1e-12));                                       // prąd nie większy niż końca prądowego
        a = przesun(p, Koniec::Pradowy, {450, 100}, {});
        CHECK(bl(a.p1.u, 400, 1e-12));                                       // koniec prądowy nie za końcem napięciowym
        // blokada Ra: nachylenie 2 kΩ zostaje
        a = przesun(p, Koniec::Pradowy, {0, 150}, {true, false});
        auto wa = policz(a);
        CHECK(wa && bl(wa->raA, 2.0, 1e-9) && bl(a.p1.i, 150, 1e-12));
        a = przesun(p, Koniec::Napieciowy, {300, 20}, {true, false});
        wa = policz(a);
        CHECK(wa && bl(wa->raA, 2.0, 1e-9) && bl(a.p2.u, 300, 1e-12));
        // blokada amplitudy: ΔI = 200 mA zostaje
        a = przesun(p, Koniec::Pradowy, {0, 260}, {false, true});
        CHECK(bl(a.p1.i - a.p2.i, 200, 1e-9));
        // obie blokady: sztywne przesunięcie w prądzie
        a = przesun(p, Koniec::Pradowy, {0, 260}, {true, true});
        wa = policz(a);
        CHECK(wa && bl(wa->raA, 2.0, 1e-9) && bl(wa->deltaI, 200, 1e-9));
        // Pa max: 25 W przechodzi przez (100 V, 250 mA) i (500 V, 50 mA)
        const auto pm = krzywaPaMax(25, 100, 500, 5);
        CHECK(pm.size() == 5 && bl(pm.front().i, 250, 1e-9) && bl(pm.back().i, 50, 1e-9));
        CHECK(krzywaPaMax(0, 100, 500, 5).empty() && krzywaPaMax(25, 500, 100, 5).empty() && krzywaPaMax(25, 100, 500, 1).empty());
    }

    // zniekształcenia: metoda pięciu rzędnych (przykład z formularza ECCLab: 93; 67,5; 41; 20; 7 mA)
    {
        using namespace Znieksztalcenia;
        auto bl = [](double a, double b, double t) { return std::abs(a - b) <= t; };
        const auto z = zPradow(93, 67.5, 41, 20, 7);
        CHECK(z && bl(z->i0, 45.8333, 1e-3) && bl(z->i1, 44.5, 1e-9) && bl(z->i2, 4.5, 1e-9) && bl(z->i3, -1.5, 1e-9) && bl(z->i4, -1.0 / 3, 1e-9));
        CHECK(z && bl(z->h2, 10.1124, 1e-3) && bl(z->h3, 3.3708, 1e-3) && bl(z->h4, 0.7491, 1e-3) && bl(z->h, 10.6857, 1e-3));
        CHECK(bl(Harmoniczne::dB(10), -20, 1e-9) && std::isinf(Harmoniczne::dB(0)));
        // rekonstrukcja: szereg z wyliczonych amplitud daje z powrotem pięć prądów
        for (int k = 0; z && k < 5; k++) {
            const double wt = std::acos(1.0) + (k == 0 ? 0 : k == 1 ? 3.14159265358979 / 3 : k == 2 ? 3.14159265358979 / 2 : k == 3 ? 2 * 3.14159265358979 / 3 : 3.14159265358979);
            const double i = z->i0 + z->i1 * std::cos(wt) + z->i2 * std::cos(2 * wt) + z->i3 * std::cos(3 * wt) + z->i4 * std::cos(4 * wt);
            const double oczek[5] = {93, 67.5, 41, 20, 7};
            CHECK(bl(i, oczek[k], 1e-9));
        }
        // charakterystyka liniowa: same pierwsze harmoniczne; symetryczna: tylko nieparzyste
        const auto lin = zPradow(100, 75, 50, 25, 0);
        CHECK(lin && bl(lin->h, 0, 1e-9));
        const auto sym = zPradow(80, 40, 0, -40, -80);
        CHECK(sym && bl(sym->h2, 0, 1e-9) && bl(sym->h4, 0, 1e-9));
        CHECK(!zPradow(5, 5, 5, 5, 5) && !zPradow(1, 2, NAN, 3, 4));
        // z odcinków: te same punkty (a = d+e, b = f+g, c = e+f) dają ten sam wynik co z prądów
        const double d = 93 - 67.5, e = 67.5 - 41, f = 41 - 20, g = 20 - 7;
        const auto zo = zOdcinkow(d + e, f + g, e + f, d, e, f, g);
        CHECK(zo && z && bl(zo->h2, z->h2, 1e-9) && bl(zo->h3, z->h3, 1e-9) && bl(zo->h4, z->h4, 1e-9));
        CHECK(!zOdcinkow(0, 0, 0, 1, 1, 1, 1));
    }

    // dławik zasilacza: przykład z formularza ECCLab (EI102, pakiet 51 mm, blacha 0,5 mm, 5 H, 300 mA, mi = 500)
    {
        using namespace Dlawik;
        auto bl = [](double a, double b, double t) { return std::abs(a - b) <= t; };
        const auto r = ksztaltkaZNazwy("EI102", 51, 0.5);
        CHECK(r && bl(r->A, 102, 1e-9) && bl(r->B, 85, 1e-9) && bl(r->a, 34, 1e-9) && bl(r->c, 17, 1e-9) && bl(r->d, 17, 1e-9) && bl(r->h, 51, 1e-9));
        CHECK(!ksztaltkaZNazwy("XX102", 51) && !ksztaltkaZNazwy("EI", 51) && !ksztaltkaZNazwy("EI1x", 51));
        Wejscie w; w.rdzen = *r; w.indukcyjnoscH = 5; w.pradMa = 300;
        const auto o = oblicz(w);
        // liczby kontrolne z formularza ECCLab (EI102, pakiet 51 mm): Qfe 16,82 cm², Vfe 343,12 cm³, lz 21,27 cm, zalecana objętość 225
        CHECK(o && bl(o->przekrojCzynnyCm2, 16.82, 0.005) && bl(o->objetoscCzynnaCm3, 343.12, 0.005) && bl(o->sredniaDlugoscZwojuCm, 21.27, 0.005));
        CHECK(o && bl(o->zalecanaObjetoscCm3, 225.0, 1e-9) && bl(o->energiaMJ, 225, 1e-9));
        CHECK(o && bl(o->drutGolyMm, 0.39, 1e-12) && bl(o->drutEmaliaMm, 0.42, 1e-12) && bl(o->oporNaMetrOm, 0.1469, 5e-5));
        CHECK(o && o->zwojeNaCm2 == 475 && o->pradDlaGestosciMa == 304 && bl(o->spadekV, 0.3 * o->opornoscOm, 1e-9));
        CHECK(o && o->zwoje == 892 && o->wypelnieniePct > 20 && o->wypelnieniePct < 23 && o->ostrzezenie == Wynik::Ostrzezenie::zaMaloWypelnione);
        // porównanie ze wzorem Krize'a [38] / ECCLab (886 zw., 300 mA -> 0,332 mm); szczelina fizyczna jest od niego mniejsza o lFe/μ
        CHECK(o && bl(o->szczelinaKrizeMm, o->zwoje * 300 / 8.0 * 1e-5, 1e-9) && bl(886 * 300 / 8.0 * 1e-5, 0.332, 5e-4));
        CHECK(o && o->szczelinaMm == 0 && o->indukcyjnoscRzeczH < 5.0);       // μ = 500: sama droga w żelazie przekracza wymaganą reluktancję
        Wejscie wf = w; wf.bMaxT = 0.5;       // mniejsza indukcja = więcej zwojów i szczelina niezerowa
        const auto of = oblicz(wf);
        CHECK(of && of->zwoje > o->zwoje && of->szczelinaMm > 0.5 && bl(of->przekladkaMm * 2, of->szczelinaMm, 1e-12));
        if (of) {   // niezależna kontrola fizyki: L = μ0 Qfe N² / (szczelina + lFe/μ)
            const double Q = of->przekrojCzynnyCm2 * 1e-4, N = of->zwoje;
            const double lt = of->szczelinaMm * 1e-3 + of->dlugoscStrumieniaCm * 1e-2 / 500.0;
            CHECK(bl(4e-7 * 3.14159265358979 * Q * N * N / lt, 5.0, 0.02) && bl(5 * 0.3 / (N * Q), 0.5, 2e-3));
            CHECK(of->szczelinaKrizeMm > of->szczelinaMm);
        }
        // przykład z książki Krize'a: L = 30 H, I0 = 60 mA -> objętość rdzenia >= 54 cm³
        Wejscie wk; wk.rdzen = *ksztaltkaZNazwy("EI102", 51, 0.5); wk.indukcyjnoscH = 30; wk.pradMa = 60;
        CHECK(oblicz(wk) && bl(oblicz(wk)->zalecanaObjetoscCm3, 54.0, 1e-9));
        // mniejszy rdzeń = nie mieści się (ponad 75%)
        Wejscie w3 = w; w3.rdzen = *ksztaltkaZNazwy("EI66", 30); w3.indukcyjnoscH = 20;
        CHECK(oblicz(w3) && !oblicz(w3)->miesciSie && oblicz(w3)->ostrzezenie == Wynik::Ostrzezenie::zaDuzoWypelnione);
        // wymuszona średnica drutu: cieńszy drut = większa rezystancja
        Wejscie w4 = w; w4.drutMm = 0.2;
        CHECK(oblicz(w4) && oblicz(w4)->opornoscOm > o->opornoscOm && bl(oblicz(w4)->drutGolyMm, 0.2, 1e-12));
        Wejscie zle = w; zle.pradMa = 0;
        CHECK(!oblicz(zle));
        zle = w; zle.mi = -5;
        CHECK(!oblicz(zle));
    }

    // SDK IRay ładowane dynamicznie: bez biblioteki nic nie działa, ale nie ma awarii; słowa 16-bitowe little endian
    {
        InfirayUsb::Sdk sdk; std::string bl;
        CHECK(!sdk.zaladuj("/nie/ma/takiego/USBSDK.dll", bl) && !bl.empty() && !sdk.zaladowana());
        CHECK(sdk.szukaj().empty() && !sdk.otwarte() && sdk.typRdzenia() == 0);
        InfirayUsb::Klatka k; k.szer = 2; k.wys = 1; k.dane = {0x34, 0x12, 0xFF, 0x00};
        const auto s = InfirayUsb::slowa16(k);
        CHECK(s.size() == 2 && s[0] == 0x1234 && s[1] == 0x00FF);
        CHECK(!InfirayUsb::opisTypuRdzenia(4).empty());
    }

    {   // biblioteka Xtherm: bez pliku DLL nie ma awarii
        XthermLib::Biblioteka xb; std::string bl;
        CHECK(!xb.zaladuj("/nie/ma/XthermDll.dll", bl) && !bl.empty() && !xb.zaladowana());
        unsigned char kl[16] = {0};
        CHECK(!xb.oblicz(kl, sizeof(kl), 2, 4, true, true).ok);
    }

    // transformator sieciowy wg Krize'a: przykład 700 V/0,1 A, 6,3 V/1,75 A, 5 V/2 A z sieci 127 V na rdzeniu Ш-25 (a=25, b=35, c=25, h=60 mm)
    {
        using namespace TrafoSieciowy;
        auto bl = [](double a, double b, double t) { return std::abs(a - b) <= t; };
        Wejscie w;
        w.rdzen.a = 25; w.rdzen.b = 35; w.rdzen.c = 25; w.rdzen.h = 60; w.rdzen.A = 100; w.rdzen.B = 85; w.rdzen.d = 12.5;
        w.napiecieSieciV = 127; w.bGs = 1e4; w.sprawnosc = 0.87; w.gestoscAmm2 = 2; w.kOkna = 0.3; w.kFe = 1.0; w.srDlugoscZwojuCm = 24;
        w.wtorne = {{700, 0.1}, {6.3, 1.75}, {5, 2}};
        const auto o = oblicz(w);
        CHECK(o && o->pierwotne.zwoje == 688 && o->wtorne.size() == 3);
        CHECK(o && bl(o->wtorne[0].zwoje, 4232, 2) && o->wtorne[1].zwoje == 38 && o->wtorne[2].zwoje == 30);
        CHECK(o && bl(o->przekrojZelazaCm2, 8.75, 1e-9) && bl(o->iloczynPosiadanyCm4, 131.25, 1e-9));
        CHECK(drutNormalny(0.67) == 0.69 && drutNormalny(1.13) == 1.16 && bl(drutWEmalii(0.69), 0.73, 1e-9));
        CHECK(o && o->pierwotne.drutMm > 0.3 && o->wtorne[0].drutMm == 0.25 && o->wtorne[0].warstwy >= 1);
        CHECK(!typowe(5).bGs == false && typowe(60).bGs == 8750 && typowe(200).sprawnosc == 0.9);
        { Wejscie m0 = w, m1 = w; m0.marginesWarstwyMm = 0; m1.marginesWarstwyMm = 5;
          const auto a0 = oblicz(m0), a1 = oblicz(m1);
          CHECK(a0 && a1 && a1->pierwotne.warstwy >= a0->pierwotne.warstwy && a1->pierwotne.gruboscMm >= a0->pierwotne.gruboscMm); }
        Wejscie zle = w; zle.wtorne.clear();
        CHECK(!oblicz(zle));
        zle = w; zle.wtorne[0].pradA = 0;
        CHECK(!oblicz(zle));
        zle = w; zle.rdzen.c = 5;   // okno za wąskie
        CHECK(oblicz(zle) && !oblicz(zle)->miesciSie);
    }

    // transformator głośnikowy SE: zrzut użytkownika (10 VA, Ra 2000, Ri 15000, 85 mA, 50 Hz, 4 om, η 0,85, 17 cm, 10,5 cm²)
    {
        auto bl = [](double a, double b, double t) { return std::abs(a - b) <= t; };
        TrafoSE::Wejscie w;
        const auto r = TrafoSE::oblicz(w);
        CHECK(r && bl(r->przekladniaPorownanie, 24.253562, 1e-6));                // wartość ze zrzutu: √(Ra/(η·Rg))
        CHECK(r && bl(r->przekladnia, std::sqrt(0.85 * 2000 / 4.0), 1e-9));      // fizycznie: Ra = n²·Rg/η
        CHECK(r && bl(r->liPorownanieH, 4.9019607, 1e-6) && bl(r->liH, 1764.70588 / (2 * 3.14159265358979 * 50), 1e-4));
        CHECK(r && bl(r->przekrojOrientacyjnyCm2, 7.906, 0.001));                 // zrzut: 8
        CHECK(r && r->przesterowanie && bl(r->mocMaxKlasaAVA, 0.085 * 0.085 * 1000 * 0.85, 1e-9));
        // niezależna kontrola: z·B·S = L·I, a szczelina z reluktancji daje z powrotem L
        const double mu0 = 4e-7 * 3.14159265358979, S = 10.5e-4;
        CHECK(r && bl(r->zwojePierwotne * 1.0 * S * 1.0, r->liH * r->pradSzczytowyMa / 1000.0, 0.5 * S));
        CHECK(r && !r->brakSzczeliny && r->szczelinaMm > 0 && bl(r->amperozwojeNaCm, r->zwojePierwotne * 0.085 / 17.0, 1e-9));
        CHECK(r && bl(mu0 * S * r->zwojePierwotne * r->zwojePierwotne / (r->szczelinaMm * 1e-3 + 0.17 / 1000.0), r->liH, 0.01 * r->liH));
        CHECK(r && r->indukcjaSzczytowaT > 0.99 && r->indukcjaSzczytowaT < 1.01 && r->indukcjaStalaT < r->indukcjaSzczytowaT);
        CHECK(r && r->zwojeWtorne == std::lround(r->zwojePierwotne / r->przekladnia));
        TrafoSE::Wejscie z = w; z.mocVA = 4;
        const auto r2 = TrafoSE::oblicz(z);
        CHECK(r2 && !r2->przesterowanie);
        {   // literatura: przykład z Krize (pentoda 6F6: Qs 6,5, ls 15, I0 35 mA, L1 20 H) i Witort
            TrafoSE::Wejscie k; k.mocVA = 2.5; k.raOm = 7400; k.riOm = 1e9; k.iaMa = 35; k.fdHz = 50; k.rgOm = 10; k.eta = 0.8;
            k.przekrojCm2 = 6.5; k.drogaCm = 15; k.dlugoscZwojuCm = 15;
            const auto pk = TrafoSE::oblicz(k);
            TrafoSE::Wynik pp = *pk; pp.liH = 20;                         // przykład Krize przyjmuje L1 = 20 H
            const auto lk = TrafoSE::literatura(k, pp);
            CHECK(bl(lk.krizeZwoje23, 4950, 10) && bl(lk.krizeZwoje24, 4846, 1));
            CHECK(bl(lk.krizeN, 1.0 / 27.2, 5e-4) && bl(lk.krizeSzczelinaMm, 0.2166, 0.01) && bl(lk.krizePrzekladkaMm, 0.108, 0.005));
            CHECK(bl(lk.krizeR1Om, 7400.0 / 2 * 0.2, 1e-9) && bl(lk.krizeDrutMinMm, 0.148, 0.001));
            CHECK(bl(lk.krizeDrutPierwotnyMm, 0.015 * std::sqrt(15.0 * lk.krizeZwoje23 / 740.0), 1e-9) && bl(lk.krizeDrutPierwotnyMm, 0.15, 0.01));
            CHECK(bl(lk.krizeIloczynWymaganyCm4, 50, 1e-9) && lk.krizeIloczynRdzeniaCm4 == 0);
            // Witort: n1 = 9000·√(L1·lr/(500·Q)), Q = (3÷5)·√P
            CHECK(bl(lk.witortQMinCm2, 3 * std::sqrt(2.5), 1e-9) && bl(lk.witortQMaxCm2, 5 * std::sqrt(2.5), 1e-9));
            CHECK(bl(lk.witortZwoje, 9000 * std::sqrt(20.0 * 15 / (500 * 6.5)), 1e-6));
            // niezależna kontrola szczeliny Krize: μ0·N·I/B przy B = 1 T
            const double mu0 = 4e-7 * 3.14159265358979;
            CHECK(bl(lk.krizeSzczelinaMm, mu0 * lk.krizeZwoje23 * 0.035 * 1e3, 0.01));
            // moduł fizyczny: przekładnia Witorta p = √(η·Ror/Zob) jest taka sama jak w programie
            CHECK(bl(pk->przekladnia, std::sqrt(0.8 * 7400 / 10.0), 1e-9));
        }
        {   // uzwojenia wykonane na rdzeniu EI102 (okno 17 x 51 mm, zwój przy rdzeniu 21,3 cm)
            TrafoSE::Wejscie we = w; we.dlugoscZwojuCm = 21.27; we.wysokoscOknaCm = 5.1; we.szerokoscOknaCm = 1.7;
            TrafoSE::Wykonane x; x.zwojePierwotne = 1035; x.zwojeWtorne = 50; x.drutPierwotnyMm = 0.25; x.drutWtornyMm = 0.9; x.szczelinaMm = 0.0816;
            const auto o = TrafoSE::wykonane(we, x);
            CHECK(o && bl(o->liH, r->liH, 0.01 * r->liH));                         // szczelina z Etapu 1 daje zadaną indukcyjność
            CHECK(o && o->indukcjaSzczytowaT > 0.98 && o->indukcjaSzczytowaT < 1.02);
            CHECK(o && o->rpOm > 0 && o->rwOm > 0 && o->rwOm < o->rpOm && o->sprawnosc > 0.5 && o->sprawnosc < 1);
            CHECK(o && bl(o->spadekStalyV, 0.085 * o->rpOm, 1e-12) && bl(o->stratyStaleW, 0.085 * 0.085 * o->rpOm, 1e-12));
            CHECK(o && bl(o->raWidzianeOm, o->rpOm + std::pow(1035.0 / 50, 2) * (4 + o->rwOm), 1e-9));
            // przeplatanie zmniejsza rozproszenie, układ wybrany = jedna z trzech wartości
            CHECK(o && o->lrUkladH[1] < o->lrUkladH[0] && o->lrUkladH[2] < o->lrUkladH[0] && o->sekcje.size() == 2);
            TrafoSE::Wykonane y = x; y.uklad = TrafoSE::Wykonane::Uklad::pierwotnePodzielone;
            const auto o3 = TrafoSE::wykonane(we, y);
            CHECK(o3 && o3->sekcje.size() == 3 && bl(o3->lrH, o3->lrUkladH[1], 1e-15) && o3->sekcje[0].zwoje + o3->sekcje[2].zwoje == 1035);
            // niezależna kontrola energii pola: dwie sekcje, F rośnie 0..1 w P, maleje 1..0 w W, izolacja F = 1
            // sprzężenie stałe: Lr = μ0·k·z²·∫l·F²dx/h daje wartość o rzędzie 1e-3..1e-1 H dla tego uzwojenia
            CHECK(o && o->lrH > 1e-4 && o->lrH < 1.0);
            TrafoSE::Wykonane zz = x; zz.zwojeWtorne = 1;
            CHECK(!TrafoSE::wykonane(we, zz));
            TrafoSE::Wejscie bez = w;   // brak wymiarów okna
            CHECK(!TrafoSE::wykonane(bez, x));
        }
        TrafoSE::Wejscie zle = w; zle.rgOm = 0;
        CHECK(!TrafoSE::oblicz(zle));
    }

    {   // cewka na rdzeniu z AL: N = √(L/AL); przykład AL = 7000 nH/zw², L = 770 mH
        CewkaAL::Wejscie w; w.alNH = 7000; w.indukcyjnoscH = 0.77; w.tolerancjaAlProc = 25; w.zwojeSprawdz = 320; w.aeMm2 = 100; w.pradA = 0.1; w.bNasyceniaT = 0.3;
        const auto o = CewkaAL::oblicz(w);
        CHECK(o && std::fabs(o->zwojeDokladne - 331.6625) < 1e-3 && o->zwoje == 332);
        CHECK(o && std::fabs(o->indukcyjnoscWykonanaH - 7e-6 * 332.0 * 332.0) < 1e-12 && std::fabs(o->bladProc - 0.2037) < 1e-3);
        CHECK(o && std::fabs(o->zwojeMin - 296.64) < 0.05 && std::fabs(o->zwojeMax - 383.0) < 0.1);
        CHECK(o && std::fabs(o->indukcyjnoscSprawdzH - 0.7168) < 1e-9 && std::fabs(o->bladSprawdzProc + 6.9091) < 1e-3);
        CHECK(o && std::fabs(o->bPoprawkaT - 0.771568 * 0.1 / (332 * 1e-4)) < 1e-6 && o->przekroczonaIndukcja);
        CHECK(!CewkaAL::oblicz(CewkaAL::Wejscie{}));
        w.tolerancjaAlProc = 100;
        CHECK(!CewkaAL::oblicz(w));
    }
    {   // wspólny model fizyczny (TrafoModel): sprawdzenie z granicami analitycznymi
        using namespace TrafoModel;
        auto bw = [](double a, double b, double w) { return std::abs(a - b) <= w * std::abs(b); };
        Rdzen r; r.przekrojCm2 = 14; r.drogaCm = 20; r.muPoczatkowa = 3000; r.szczelinaStykuMm = 0; r.stratyZelazaWkg = 1e-9;
        Uzwojenia u; u.zwojePierwotne = 1500; u.zwojeWtorne = 60; u.rpOm = 50; u.rwOm = 0.2; u.lrH = 0.03; u.cdF = 0;
        Zrodlo z; z.przeciwsobny = true; z.riOm = 10000; z.rgOm = 8; z.mocW = 30; z.pradStalyA = 0; z.spadekDb = 3; z.fMocyHz = 40;
        const auto o = analiza(r, u, z);
        CHECK(o.has_value());
        if (o) {
            const double n = 25, rl = n * n * (z.rgOm + u.rwOm), rs = z.riOm + u.rpOm;
            const double lp = 4e-7 * 3.14159265358979 * 3000 * 14e-4 * 1500.0 * 1500.0 / 0.2;
            CHECK(bw(o->lpH, lp, 1e-9) && bw(o->lpBezPolaryzacjiH, lp, 1e-9));                         // L = μ0μ·S·z²/l
            CHECK(bw(o->fdHz, rs * rl / (rs + rl) / (2 * 3.14159265358979 * lp), 0.01));                // pole dolny: (Rs∥R')/(2πLp)
            CHECK(bw(o->fgHz, (rs + rl) / (2 * 3.14159265358979 * u.lrH), 0.01));                       // pole górne: (Rs+R')/(2πLr)
            CHECK(bw(o->raWidzianeOm, u.rpOm + rl, 0.002));
            CHECK(bw(o->sprawnoscMiedz, n * n * z.rgOm / (rl + u.rpOm), 1e-12));
            const double bIdeal = std::sqrt(2 * z.mocW * z.rgOm) * n / (2 * 3.14159265358979 * 40 * 1500 * 14e-4);
            CHECK(o->bZmiennaT > 0.8 * bIdeal && o->bZmiennaT < 1.0 * bIdeal);                          // napięcie w węźle < napięcie obciążenia + spadki
        }
        // spadek na krańcach pasma 1/2/3 dB: f_d = (Rs∥R')/(2π·Lp·k), f_g = k·(Rs+R')/(2π·Lr), k = √(M²−1)
        CHECK(bw(wspolczynnikSpadku(3), 1.0, 1e-12) && bw(wspolczynnikSpadku(2), 0.7648, 5e-4) && bw(wspolczynnikSpadku(1), 0.5087, 5e-4));
        for (double d : {1.0, 2.0, 3.0}) {
            Zrodlo zd = z; zd.spadekDb = d;
            const auto od = analiza(r, u, zd);
            const double nn = 25, rl2 = nn * nn * (zd.rgOm + u.rwOm), rs2 = zd.riOm + u.rpOm, k = wspolczynnikSpadku(d);
            CHECK(od && bw(od->fdHz, rs2 * rl2 / (rs2 + rl2) / (2 * 3.14159265358979 * od->lpH * k), 0.015));
            CHECK(od && bw(od->fgHz, k * (rs2 + rl2) / (2 * 3.14159265358979 * u.lrH), 0.015));
        }
        // polaryzacja i szczelina: przy dużej szczelinie B = μ0·z·I/lg
        Rdzen rs; rs.przekrojCm2 = 10; rs.drogaCm = 15; rs.muPoczatkowa = 5000; rs.szczelinaStykuMm = 0; rs.stratyZelazaWkg = 1e-9;
        Zrodlo zs; zs.riOm = 5000; zs.rgOm = 8; zs.mocW = 3; zs.pradStalyA = 0.05;
        Uzwojenia us; us.zwojePierwotne = 1200; us.zwojeWtorne = 60; us.rpOm = 120; us.rwOm = 0.3; us.lrH = 0.01; us.szczelinaCelowaMm = 2;
        const auto os = analiza(rs, us, zs);
        CHECK(os && bw(os->bStalaT, 4e-7 * 3.14159265358979 * 1200 * 0.05 / 2e-3, 0.03) && os->muPrzyrostowa < 5000 && os->lpH < os->lpBezPolaryzacjiH * 1.0001);
        // odwracanie: szczelina dla zadanej indukcyjności i zwoje dla zadanej indukcyjności
        const auto g = szczelinaDlaIndukcyjnosci(rs, 1200, zs, 4.0);
        CHECK(g.has_value());
        if (g) { us.szczelinaCelowaMm = *g; const auto o4 = analiza(rs, us, zs); CHECK(o4 && bw(o4->lpH, 4.0, 1e-6)); }
        const auto zw = zwojeDlaIndukcyjnosci(rs, 0.5, zs, 4.0);
        CHECK(zw.has_value());
        if (zw) { Uzwojenia u5 = us; u5.zwojePierwotne = *zw; u5.szczelinaCelowaMm = 0.5; const auto o5 = analiza(rs, u5, zs); CHECK(o5 && bw(o5->lpH, 4.0, 1e-6)); }
        // większy prąd => mniejsza indukcyjność przyrostowa
        Zrodlo z2 = zs; z2.pradStalyA = 0.12;
        us.szczelinaCelowaMm = 0.3;
        const auto oa = analiza(rs, us, zs), ob = analiza(rs, us, z2);
        CHECK(oa && ob && ob->lpH < oa->lpH && ob->bStalaT > oa->bStalaT);
        // pojemność własna: jedna sekcja 3 warstwy -> 4/3·(N−1)/N²·C_warstw
        SekcjaPojemnosci sp; sp.pierwotna = true; sp.warstwy = 3; sp.zwoje = 900; sp.dlugoscZwojuCm = 20; sp.wysokoscCm = 5; sp.izolacjaWarstwMm = 0.1;
        const double cw = 8.8541878128e-12 * 3 * 0.2 * 0.05 / 1e-4;
        CHECK(bw(pojemnoscWlasna({sp}, 25, 0.5, 0), 4.0 / 3.0 * 2 / 9 * cw, 1e-9));
        {   // pojemność PP z potencjałów warstw: P (1 warstwa) wewnątrz, W (1 warstwa) na zewnątrz, φ∈[−½,½], ψ∈[0,1]
            SekcjaPP p1, w1;
            p1.pierwotna = true; p1.zwoje = 100; p1.zwojePrzed = 0; p1.warstwy = 1; p1.zwojeWWarstwie = 100; p1.zwojeOstatniejWarstwy = 100; p1.dlugoscZwojuCm = 20; p1.wysokoscCm = 5;
            w1 = p1; w1.pierwotna = false; w1.zwoje = 10; w1.zwojeWWarstwie = 10; w1.zwojeOstatniejWarstwy = 10;
            const double cps = 8.8541878128e-12 * 3 * 0.2 * 0.05 / 0.5e-3;
            CHECK(bw(pojemnoscPP({p1, w1}, 100, 10, 10, 0.1, 0.5), cps * (0.25 + 0.25 + 0.25) / 3.0 * 0 + cps * (2.25 - 0.75 + 0.25) / 3.0, 1e-9));
            CHECK(pojemnoscPP({p1, w1}, 100, 10, 10, 0.1, 0.5) > pojemnoscPP({p1, w1}, 100, 10, 10, 0.1, 1.0));
            w1.komora = 1;
            CHECK(bw(pojemnoscPP({p1, w1}, 100, 10, 10, 0.1, 0.5), 0.0, 1e-12));      // inne komory: brak pojemności P–S
        }
        // dwie połówki niosą po ¼ pojemności jednej sekcji o tym samym C (udział napięcia²)
        SekcjaPojemnosci s1 = sp, s2 = sp; s1.zwoje = s2.zwoje = 450;
        CHECK(bw(pojemnoscWlasna({s1, s2}, 25, 0.5, 0), 2 * 0.25 * 4.0 / 3.0 * 2 / 9 * cw, 1e-9));
        CHECK(!analiza(r, Uzwojenia{}, z) && !szczelinaDlaIndukcyjnosci(rs, 0, zs, 4));
    }

    // transformator głośnikowy PP: liczby kontrolne ze zrzutów programu Laboratorium2005
    {
        using namespace TrafoPP;
        auto bl = [](double a, double b, double t) { return std::abs(a - b) <= t; };
        Wymagania w;   // 8 om, Raa 3400, 40 W, 40...20000 Hz, Ri 15000, I0 30 mA
        const auto p = etap1(w);
        CHECK(p && bl(p->liH, 11.0284, 5e-4) && bl(p->lrH, 0.1464, 5e-5) && bl(p->przekladnia, 19.557, 2e-3));
        CHECK(p && bl(p->iloczynCm4, 417.08, 0.01) && bl(p->przekrojCm2, 20.422, 0.002));
        {   // Li i Lr dla spadku 1/2/3 dB (3 dB = k = 1: wartości jak dotąd)
            Wymagania wd = w; wd.spadekDb = 2;
            const auto pd = etap1(wd);
            CHECK(pd && bl(pd->liH, p->liH / 0.76481, 2e-3) && bl(pd->lrH, p->lrH * 0.76481, 5e-4));
            wd.spadekDb = 1;
            const auto p1d = etap1(wd);
            CHECK(p1d && p1d->liH > pd->liH && p1d->lrH < pd->lrH);
            wd.spadekDb = 0;
            CHECK(!etap1(wd));
        }
        Wymagania w2 = w; w2.mocW = 50; w2.fdHz = 100; w2.fgHz = 15000; w2.i0Ma = 32;
        const auto p2 = etap1(w2);
        CHECK(p2 && bl(p2->liH, 4.4114, 5e-4) && bl(p2->lrH, 0.1952, 5e-5) && bl(p2->iloczynCm4, 208.54, 0.01) && bl(p2->przekrojCm2, 14.441, 0.002));
        Rdzen r; r.przekrojCm2 = 17.34; r.drogaStrumieniaCm = 18.7; r.dlugoscZwojuCm = 21.7; r.wysokoscUzwojeniaCm = 5.1; r.szerokoscOknaCm = 1.7;
        const auto u = etap2(w2, r);
        CHECK(u && bl(u->zwojePierwotne, 917.1557, 0.1) && bl(u->zwojeWtorne, 46.8951, 0.01));
        CHECK(u && bl(u->drutPierwotnyMm, 0.319, 5e-4) && bl(u->drutWtornyMm, 1.2616, 5e-5));
        {   // metoda gitarowa (guitaramplifiers.prv.pl): EL34 AB1, 55 W, Raa 3400, Ri 30000, R2 = 5 Ω, 50 Hz, 8000 Gs, μ 500, rdzeń 13,6 cm² x 21 cm
            Wymagania wg; wg.rObciazeniaOm = 5; wg.raaOm = 3400; wg.mocW = 55; wg.fdHz = 50; wg.riOm = 30000; wg.i0Ma = 30; wg.bGs = 8000;
            wg.mi = 500; wg.eta = 0.9; wg.alfaFe = 1; wg.sigma = 2;
            Rdzen rg; rg.przekrojCm2 = 13.6; rg.drogaStrumieniaCm = 21;
            const auto g = gitarowe(wg, rg);
            CHECK(g && bl(g->l1H, 12.96, 0.05) && bl(g->um1V, 611.6, 0.2) && bl(g->objetoscCm3, 287, 3) && bl(g->zwoje1, 1785, 5));
            CHECK(g && bl(g->imaxA, 0.1799, 5e-4) && bl(g->i1A, 0.0948, 5e-4) && bl(g->d1Mm, 0.2457, 1e-3) && bl(g->zwoje2, 72.1, 0.3) && bl(g->d2Mm, 1.222, 0.01));
            CHECK(g && bl(g->zwoje2Zl[0], g->zwoje1 * std::sqrt(4 / (wg.eta * wg.raaOm)), 1e-6) && g->zwoje2Zl[1] > g->zwoje2Zl[0] && g->zwoje2Zl[2] > g->zwoje2Zl[1]
                  && bl(g->zwoje2Zl[2] / g->zwoje2Zl[0], 2, 1e-9) && bl(g->d2ZlMm[0] / g->d2ZlMm[2], std::sqrt(2), 1e-9));
            { auto w4 = wg; w4.rObciazeniaOm = 4; const auto g4 = gitarowe(w4, rg);   // 4, 8 i 16 Ω nie zależą od wpisanego obciążenia
              CHECK(g && g4 && bl(g4->zwoje2Zl[1], g->zwoje2Zl[1], 1e-9) && bl(g4->zwoje2, g->zwoje2Zl[0], 1e-9)); }
            const auto ga = gitarowe(wg, rg, 1.25, true);
            CHECK(ga && ga->i1A < g->i1A && !gitarowe(wg, rg, 1.0));
        }
        // Etap 3 (uzwojenia wykonane 917/47 zw., drut 0,3/1,3 mm) - zrzut 3 programu: Li 4,4099, Raa 3383,68, F 2,8225 cm², Fx 8,67 cm²
        Wykonane x; x.zwojePierwotne = 917; x.zwojeWtorne = 47; x.drutPierwotnyMm = 0.3; x.drutWtornyMm = 1.3;
        x.sciankaKarkasuMm = 0; x.marginesWarstwyMm = 0;   // zrzut programu liczył okno idealne (bez karkasu i marginesów)
        const auto e3 = etap3(w2, r, x);
        CHECK(e3 && bl(e3->liH, 4.4099, 1e-3) && bl(e3->raaOm, 3383.68, 0.05) && bl(e3->zajeteCm2, 2.8225, 0.01) && bl(e3->miejsceCm2, 8.67, 1e-9) && e3->miesciSie);
        CHECK(e3 && bl(e3->rwEcclabOm, 0.1839, 5e-5) && bl(e3->lrKsiazkaH, 0.02495, 5e-5));     // porównanie z książką/ECCLab
        {   // tabela sekcji: parser, szablony, komory, równowaga amperozwojów
            std::vector<SekcjaWej> s; std::string bl2;
            CHECK(parsujSekcje("A1 600\nW1 100\n|\nA2 300\nw2: 100\n", s, bl2) && s.size() == 4 && s[0].pierwotna && !s[1].pierwotna && s[2].komora == 1 && s[3].zwoje == 100 && s[3].komora == 1);
            CHECK(formatujSekcje(s) == "A1 600\nW1 100\n|\nA2 300\nW2 100\n");
            CHECK(!parsujSekcje("", s, bl2) && !parsujSekcje("X1 5", s, bl2) && !parsujSekcje("A1 abc", s, bl2) && !parsujSekcje("|\nA1 5", s, bl2) && !parsujSekcje("A1 5\n|", s, bl2) && !parsujSekcje("A1 0", s, bl2));
            for (int nr = 0; nr < 5; nr++) {
                const auto ps = presetSekcji(nr, 917, 47);
                int a = 0, ww = 0;
                for (const auto& q : ps) (q.pierwotna ? a : ww) += q.zwoje;
                CHECK(a == 917 && ww == 47);
                Wykonane xt = x; xt.sekcjeWej = ps;
                const auto et = etap3(w2, r, xt);
                CHECK(et && !et->niezrownowazone && et->sekcje.size() == ps.size());
                if (et && nr == 0) CHECK(bl(et->lrH, e3->lrH, 1e-9) && bl(et->rpOm, e3->rpOm, 1e-9) && bl(et->zajeteCm2, e3->zajeteCm2, 1e-9));   // zgodne z układem P-S
                if (et && nr == 1) { Wykonane xp = x; xp.pierwotnePodzielone = true; const auto ep = etap3(w2, r, xp); CHECK(ep && bl(et->lrH, ep->lrH, 1e-3)); }
                if (et && nr == 3) CHECK(et->sekcje[2].komora == 1 && et->sekcje[2].wysokoscKomoryMm < 5.1 * 10 / 2 + 1e-9);
            }
            {   // Mesa Boogie DR (Rys.15): jedno trafo, 10 warstw, odczepy 4, 8 i 16 Ω jednocześnie; pierwotne 1-7-środek-4-10
                const auto ps = presetSekcji(5, 1696, 78);
                int a = 0, w = 0;
                for (const auto& q : ps) (q.pierwotna ? a : w) += q.zwoje;
                CHECK(ps.size() == 10 && a == 1696 && maOdczepy(ps) && w == 78 * 4 + 33 * 2);
                CHECK(ps[0].pierwotna && ps[3].pierwotna && ps[6].pierwotna && ps[9].pierwotna && ps[0].zwoje == 318 && ps[3].zwoje == 530 && ps[9].zwoje == 318);
                // zwoje odczepów: 78, 78 + 33 = 111, 78 + 78 = 156
                CHECK(zwojeWtorneZSekcji(ps, false, 1) == 78 && zwojeWtorneZSekcji(ps, false, 2) == 111 && zwojeWtorneZSekcji(ps, false, 3) == 156);
                Wykonane xd = x; xd.zwojePierwotne = 1696; xd.zwojeWtorne = 78; xd.sekcjeWej = ps; xd.drutPierwotnyMm = 0.3; xd.drutWtornyMm = 0.4;
                WynikWykonane wyn[3];
                for (int k = 1; k <= 3; k++) {
                    xd.odczep = k;
                    const auto ed = etap3(w2, r, xd);
                    CHECK(ed && ed->sekcje.size() == 10 && !ed->niezrownowazone && ed->rwOm > 0);
                    if (ed) {
                        wyn[k - 1] = *ed;
                        // kolejność elektryczna pierwotnych 1-7-4-10: zwojePrzed sekcji 7 = 318, sekcji 4 = 848, sekcji 10 = 1378
                        CHECK(bl(ed->sekcje[0].zwojePrzed, 0, 1e-9) && bl(ed->sekcje[6].zwojePrzed, 318, 1e-9) && bl(ed->sekcje[3].zwojePrzed, 848, 1e-9) && bl(ed->sekcje[9].zwojePrzed, 1378, 1e-9));
                    }
                }
                CHECK(wyn[2].rwOm > 1.5 * wyn[0].rwOm);      // 16 Ω: szeregowo dopinka 78 zw., więcej miedzi w obiegu niż samo 4 Ω
                {   // zestawienie odczepów: każdy dla własnego obciążenia; Raa wszystkich zbliżone (zwoje ∝ √Z)
                    const auto zo = odczepy(w2, r, xd);
                    CHECK(zo.size() == 3 && zo[0].omy == 4 && zo[1].omy == 8 && zo[2].omy == 16 && zo[0].zwojeWtorne == 78 && zo[1].zwojeWtorne == 111 && zo[2].zwojeWtorne == 156);
                    if (zo.size() == 3) CHECK(zo[1].raaOm > 0.97 * zo[0].raaOm && zo[1].raaOm < 1.03 * zo[0].raaOm && zo[2].raaOm > 0.97 * zo[0].raaOm && zo[2].raaOm < 1.03 * zo[0].raaOm);
                    CHECK(odczepy(w2, r, x).empty());          // zwykła tabela: brak odczepów
                }
                const std::string tekst = formatujSekcje(ps);
                CHECK(tekst.find("g1/1/1") != std::string::npos && tekst.find("g-/2/-") != std::string::npos && tekst.find("g-/-/2") != std::string::npos && tekst.find("e2") != std::string::npos);
                std::vector<SekcjaWej> s2; std::string b2;
                CHECK(parsujSekcje(tekst, s2, b2) && s2.size() == ps.size());
                for (size_t i = 0; i < ps.size() && s2.size() == ps.size(); i++) CHECK(s2[i].grupaOdczep == ps[i].grupaOdczep && s2[i].kolejnosc == ps[i].kolejnosc && s2[i].zwoje == ps[i].zwoje);
                // z2 (zwoje na 4 Ω) przeskalowane: 84 zw. -> odczepy 84 / 84+36 / 168
                const auto p84 = presetSekcji(5, 1696, 84);
                CHECK(zwojeWtorneZSekcji(p84, false, 1) == 84 && zwojeWtorneZSekcji(p84, false, 3) == 168);
                // błędy: nierówne sekcje w grupie, mieszanie z sekcjami bez grup, powtórzona kolejność, zły zapis
                Wykonane xe = xd; xe.odczep = 1; xe.sekcjeWej[1].zwoje = 79;
                CHECK(!etap3(w2, r, xe));
                xe = xd; xe.odczep = 1; xe.sekcjeWej[3].kolejnosc = 1;
                CHECK(!etap3(w2, r, xe));
                std::vector<SekcjaWej> sb; std::string bb;
                CHECK(!parsujSekcje("A1 5 g1", sb, bb) && !parsujSekcje("W1 5 e1", sb, bb) && !parsujSekcje("W1 5 g0", sb, bb) && !parsujSekcje("W1 5 qq", sb, bb));
                CHECK(!parsujSekcje("W1 5 g1/2", sb, bb) && !parsujSekcje("W1 5 g1/2/3/4", sb, bb) && !parsujSekcje("W1 5 g1/0/2", sb, bb));
                CHECK(parsujSekcje("A1 5 e1\nW1 3 G2\nW2 3 X", sb, bb) && sb[1].grupa == 2 && sb[2].grupa == -1 && sb[0].kolejnosc == 1);
                CHECK(parsujSekcje("W1 3 g1/-/2", sb, bb) && sb[0].grupaOdczep[0] == 1 && sb[0].grupaOdczep[1] == -1 && sb[0].grupaOdczep[2] == 2);
            }
            Wykonane xn = x; xn.sekcjeWej = {{true, 917, 0}, {false, 47, 0}, {true, 0 + 1, 1}};   // suma A != z1 -> błąd
            CHECK(!etap3(w2, r, xn));
            Wykonane xb = x; xb.sekcjeWej = {{true, 800, 0}, {false, 47, 0}, {true, 117, 1}};     // komora 2 bez wtórnego: niezrównoważona (F = 0,13)
            const auto eb = etap3(w2, r, xb);
            CHECK(eb && eb->niezrownowazone);
            Wykonane xr = x; xr.zwojeWtorne = 24; xr.wtorneRownolegle = true; xr.sekcjeWej = {{true, 917, 0}, {false, 24, 0}, {false, 24, 0}};   // równolegle: dwie równe sekcje
            const auto er = etap3(w2, r, xr);
            CHECK(er && er->rwOm > 0);
            xr.sekcjeWej[2].zwoje = 25;
            CHECK(!etap3(w2, r, xr));
        }
        CHECK(e3 && e3->sekcje.size() == 2 && e3->sekcje[0].pierwotna && e3->sekcje[0].warstwy == 6 && e3->sekcje[1].warstwy == 2);
        CHECK(e3 && e3->sprawnosc > 0.95 && e3->sprawnosc < 0.99 && e3->rpOm > 40 && e3->rpOm < 60);
        {   // poprawka brzegowa k: automatycznie ze współczynnika Rogowskiego albo ręcznie
            const double K = wspolczynnikRogowskiego(0.0064, 0.045);
            CHECK(bl(K, 1.0 - 1.0 / (3.14159265358979323846 * 0.045 / 0.0064), 1e-9) && K > 0.95 && K < 0.96);   // exp(-a) pomijalnie małe dla a ≈ 22
            CHECK(bl(wspolczynnikRogowskiego(1e-9, 0.05), 1.0, 1e-6) && bl(wspolczynnikRogowskiego(1.0, 0.01), 3.14159265358979 * 0.01 / 2, 2e-3));   // δ << h: 1; δ >> h: π·h/(2δ)
            Wykonane xa = x; xa.kEfektyBrzegowe = 0;      // auto
            Wykonane xm = x; xm.kEfektyBrzegowe = 1.0;    // ręcznie: model idealny
            const auto ea = etap3(w2, r, xa), em = etap3(w2, r, xm);
            CHECK(ea && em && ea->kUzyte > 0.5 && ea->kUzyte < 1.0 && bl(em->kUzyte, 1.0, 1e-12) && bl(ea->lrH / em->lrH, ea->kUzyte, 1e-9));
            Wykonane xb = x; xb.kEfektyBrzegowe = -0.1;
            CHECK(!etap3(w2, r, xb));
        }
        {   // ścianki karkasu i margines warstwy zmniejszają wysokość uzwojenia i szerokość okna
            Wykonane xd = x; xd.sciankaKarkasuMm = 1.5; xd.marginesWarstwyMm = 1.5;
            const auto ed = etap3(w2, r, xd);
            CHECK(e3 && ed && ed->sekcje[0].zwojeWWarstwie < e3->sekcje[0].zwojeWWarstwie && ed->lrH > e3->lrH);
            CHECK(ed && bl(ed->miejsceCm2, (r.szerokoscOknaCm - 0.15) * (r.wysokoscUzwojeniaCm - 0.3), 1e-9));
            CHECK(ed && bl(ed->sekcje[0].wysokoscKomoryMm, r.wysokoscUzwojeniaCm * 10.0 - 6.0, 1e-9));   // długość warstwy: h − 2·ścianka − 2·margines
            Wykonane xz = xd; xz.sciankaKarkasuMm = 40;
            CHECK(!etap3(w2, r, xz));   // ścianki zjadają całe okno
        }
        // rozproszenie: przeplatanie zmniejsza je co najmniej 2-krotnie; model fizyczny (rzeczywista grubość uzwojeń) daje mniej niż Δ = szerokość okna
        CHECK(e3 && e3->lrPrzeplatanyH < e3->lrNieprzeplatanyH / 2 && e3->lrNieprzeplatanyH < e3->lrKsiazkaH && bl(e3->lrH, e3->lrNieprzeplatanyH, 1e-12));
        Wykonane xp = x; xp.pierwotnePodzielone = true;
        const auto e3p = etap3(w2, r, xp);
        CHECK(e3p && e3p->sekcje.size() == 3 && bl(e3p->lrH, e3->lrPrzeplatanyH, 1e-12) && e3p->sekcje[0].nieparzysteWarstwy);   // 459 zw. / 159 = 3 warstwy
        // kontrola całki: przy bardzo długim zwoju (l prawie stałe) wynik = μ0·k·z²·l·(δ + (t1+t2)/3)/h
        {
            Rdzen rd = r; rd.dlugoscZwojuCm = 1e5;
            Wykonane xk = x; xk.kEfektyBrzegowe = 1.0;
            const auto ek = etap3(w2, rd, xk);
            if (ek) {
                const double dl = rd.dlugoscZwojuCm * 0.01, tt = (ek->sekcje[0].gruboscMm + ek->sekcje[1].gruboscMm) * 1e-3 / 3.0;
                const double wz = 4e-7 * 3.14159265358979 * 917.0 * 917.0 * dl * (w2.przekladkaCm * 1e-2 + tt) / (r.wysokoscUzwojeniaCm * 1e-2);
                CHECK(bl(ek->lrNieprzeplatanyH / wz, 1.0, 2e-3));
            }
        }
        CHECK(!etap3(w2, r, Wykonane{}));
        Wymagania zle = w; zle.eta = 1.5;
        CHECK(!etap1(zle) && !etap2(zle, r) && !etap3(zle, r, x));
        CHECK(!etap2(w, Rdzen{}));
    }

    // kamera 384x292 (InfiRay T3 Pro na macOS): 288 wierszy obrazu + 4 wiersze nagłówka
    {
        using namespace Termowizja;
        KlatkaSurowa k;
        k.szer = 384; k.wys = 292; k.kanaly = 2; k.bajtyNaKanal = 1;
        k.dane.assign(static_cast<size_t>(384) * 292 * 2, 0);
        for (size_t i = 0; i < k.dane.size(); i += 2) { k.dane[i] = static_cast<unsigned char>(60 + (i / 2) % 90); k.dane[i + 1] = 0x80; }
        auto u = rozpoznaj(k);
        CHECK(u.uklad == Uklad::ObrazZNaglowkiem && u.obrazSzer == 384 && u.obrazWys == 288);
        CHECK(daneRadiometryczne(k, u).empty());
        CHECK(obrazZJasnosci(k, u, Paleta::Szara).size() == static_cast<size_t>(384) * 288 * 3);
        // nagłówek: float 25.0 (0x41C80000) w wierszu 4 na pozycji 0
        const size_t p3 = static_cast<size_t>(291) * 384 * 2;
        k.dane[p3 + 32] = 0xC8; k.dane[p3 + 33] = 0x41;   // float 25,0 jako T odbita (bajt 26 + 4)
        CHECK(opisNaglowka(k, u).find("T odbita 25") != std::string::npos);
        // tryb surowy: chrominancja nie jest stała, dane 16-bitowe little endian
        for (size_t i = 0; i < static_cast<size_t>(288) * 384 * 2; i += 2) { k.dane[i] = 0x34; k.dane[i + 1] = 0x12; }
        k.dane[1] = 0x13;
        u = rozpoznaj(k);
        const auto d = daneRadiometryczne(k, u);
        CHECK(u.uklad == Uklad::Surowe16 && d.size() == static_cast<size_t>(384) * 288 && d[0] == 0x1334 && d[1] == 0x1234);
        const std::string r = raportDiagnostyczny(k);
        CHECK(r.find("bez kalibracji") != std::string::npos && r.find("°C, max") == std::string::npos);
        // 256x384 nadal jest obrazem i danymi; 384x288 bez nagłówka to tylko obraz
        CHECK(rozpoznaj(klatkaTestowa(0)).uklad == Uklad::ObrazIDane);
        KlatkaSurowa m; m.szer = 384; m.wys = 288; m.kanaly = 2; m.bajtyNaKanal = 1; m.dane.assign(384 * 288 * 2, 0x80);
        CHECK(rozpoznaj(m).uklad == Uklad::TylkoObraz);
    }

    // kalibracja użytkownika trybu surowego
    {
        using namespace Termowizja;
        KalibracjaUzytkownika kal;
        CHECK(!kal.gotowa() && kal.naCelsjusza(5000) == 0);
        kal.dodaj(4800, 0);
        CHECK(!kal.gotowa());
        kal.dodaj(5300, 100);
        CHECK(kal.gotowa() && std::fabs(kal.naCelsjusza(5050) - 50) < 1e-9 && std::fabs(kal.naCelsjusza(5550) - 150) < 1e-9 && std::fabs(kal.naCelsjusza(4550) + 50) < 1e-9);
        kal.dodaj(5300.3, 90);                                            // ten sam odczyt (±0,5): zastępuje punkt
        CHECK(kal.liczba() == 2 && std::fabs(kal.naCelsjusza(5300.3) - 90) < 1e-6);
        kal.dodaj(5000, 40);                                              // trzeci punkt: łamana
        CHECK(kal.liczba() == 3 && std::fabs(kal.naCelsjusza(5000) - 40) < 1e-9 && std::fabs(kal.naCelsjusza(4900) - 20) < 1e-9);
        // klatka testowa trybu surowego: rozpoznanie, nagłówek zgodny z danymi, statystyki z kalibracji
        const auto kt = klatkaTestowaSurowa(0);
        const auto ut = rozpoznaj(kt);
        CHECK(ut.uklad == Uklad::Surowe16 && ut.obrazWys == 288);
        const auto dt = daneRadiometryczne(kt, ut);
        const std::string nag = opisNaglowka(kt, ut);
        CHECK(nag.find("Słowo 4 = największy odczyt: zgadza się") != std::string::npos && nag.find("słowo 7 = najmniejszy odczyt: zgadza się") != std::string::npos);
        CHECK(nag.find("emisyjność 0.98") != std::string::npos && nag.find("odległość 1") != std::string::npos);
        KalibracjaUzytkownika k2;
        k2.dodaj(4800, 20); k2.dodaj(5400, 80);                           // 0,1 °C na odczyt
        const auto st = statystykiKalibrowane(dt, 384, 288, k2);
        const auto mmm = std::minmax_element(dt.begin(), dt.end());
        CHECK(st.ok && std::fabs(st.minC - (20 + (*mmm.first - 4800) * 0.1)) < 1e-9 && std::fabs(st.maxC - (20 + (*mmm.second - 4800) * 0.1)) < 1e-9 && st.maxC > st.minC);
        CHECK(!statystykiKalibrowane(dt, 384, 288, KalibracjaUzytkownika()).ok);
        CHECK(std::fabs(sredniaSurowych(std::vector<uint16_t>(100, 7), 10, 10, 0, 0, 3) - 7) < 1e-12);
    }

    // zestawienie drutu na wagę
    {
        using namespace MasaDrutu;
        // 1 mm × 100 m: π/4 · 1 mm² · 100 m = 78,54 cm³ · 8,96 g/cm³ = 703,7 g
        CHECK(std::fabs(masaMiedziG(1.0, 100.0) - 703.7) < 0.1);
        CHECK(masaMiedziG(0, 10) == 0 && masaMiedziG(1, -1) == 0);
        const auto z = zestawienie({{"P1", 0.5, 1000, 10.0}, {"W1", 0.2, 200, 12.0}, {"P2", 0.5, 500, 10.0}, {"W2", 0.0, 100, 10.0}, {"X", 0.3, 0, 10.0}});
        CHECK(z.wiersze.size() == 2 && z.wiersze[0].drutMm == 0.2 && z.wiersze[1].drutMm == 0.5);   // od najcieńszego; zerowe pominięte
        CHECK(std::fabs(z.wiersze[1].dlugoscM - 150.0) < 1e-9 && z.wiersze[1].uzwojenia == "P1, P2");
        CHECK(std::fabs(z.wiersze[1].masaCuG - masaMiedziG(0.5, 150)) < 1e-9);
        CHECK(std::fabs(z.wiersze[1].masaZakupG - z.wiersze[1].masaCuG * 1.03 * 1.10) < 1e-9);
        CHECK(std::fabs(z.razemCuG - (z.wiersze[0].masaCuG + z.wiersze[1].masaCuG)) < 1e-9 && std::fabs(z.razemDlugoscM - 174.0) < 1e-9);
        CHECK(zestawienie({}).puste() && zestawienie({{"a", 0, 1, 1}}).puste());
        Dlawik::Wynik dw; dw.drutGolyMm = 0.4; dw.zwoje = 1000; dw.sredniaDlugoscZwojuCm = 20;
        const auto zd = zestawienie(pozycje(dw));
        CHECK(zd.wiersze.size() == 1 && std::fabs(zd.wiersze[0].dlugoscM - 200.0) < 1e-9);
        TrafoSieciowy::Wynik tw; tw.srDlugoscZwojuCm = 20;
        tw.pierwotne.drutMm = 0.3; tw.pierwotne.zwoje = 1000; tw.pierwotne.gruboscMm = 4;
        TrafoSieciowy::WynikUzwojenia w1; w1.drutMm = 0.3; w1.zwoje = 100; w1.gruboscMm = 4; tw.wtorne.push_back(w1);
        const auto zt = zestawienie(pozycje(tw));
        CHECK(zt.wiersze.size() == 1 && zt.wiersze[0].uzwojenia == "P, W1");
        // uzwojenie zewnętrzne ma dłuższy zwój niż wewnętrzne (±2π·2 mm = ±1.257 cm): 1000·(20−c) + 100·(20+c) cm
        CHECK(std::fabs(zt.wiersze[0].dlugoscM - (1000 * (20 - 2 * 3.14159265358979 * 0.2) + 100 * (20 + 2 * 3.14159265358979 * 0.2)) / 100.0) < 1e-9 && zt.wiersze[0].dlugoscM < 220.0);   // = 208.69 m
    }


    // cewki powietrzne
    {
        using namespace CewkaPowietrzna;
        const double pi = 3.14159265358979323846;
        CHECK(std::fabs(calkaK(0.5) - 1.6857503548) < 1e-9 && std::fabs(calkaE(0.5) - 1.4674622093) < 1e-9);
        CHECK(std::fabs(calkaK(0) - pi / 2) < 1e-12 && std::fabs(calkaE(0) - pi / 2) < 1e-12);
        // dipol: dwa małe pierścienie daleko od siebie: M = μ0·π·a²·b² / (2 z³)
        {
            const double a = 1e-3, z = 0.2;
            const double mDip = 4e-7 * pi * pi * a * a * a * a / (2.0 * z * z * z);
            CHECK(std::fabs(indukcyjnoscWzajemnaPierscieni(a, a, z) / mDip - 1.0) < 1e-3);
        }
        // granica cewki "walcowej" (arkusz prądowy): współczynnik Nagaoki K = L·l / (μ0 N² A): 2a/l = 1 -> 0,6884, 2a/l = 2 -> 0,5255
        for (auto zz : {std::pair<double, double>{1.0, 0.6884}, {2.0, 0.5255}}) {
            const int N = 600;
            const double D = 20.0, l = D / zz.first;                 // [mm], zwoje bardzo cienkim drutem
            Parametry p; p.srednicaKarkasuMm = D; p.drutMm = 0.001; p.drutZIzolacjaMm = 0.001; p.skokMm = l / N; p.zwojeNaWarstwe = N; p.warstwy = 1;
            // średnica pierścieni = D + dz ≈ D
            const auto w = oblicz(p);
            const double a = D / 2 * 1e-3, A = pi * a * a, lm = l * 1e-3;
            const double K = w->indukcyjnoscUH * 1e-6 * lm / (4e-7 * pi * double(N) * N * A);
            CHECK(w && std::fabs(K - zz.second) < 0.01);
        }
        Parametry p; p.srednicaKarkasuMm = 10; p.drutMm = 0.5; p.zwojeNaWarstwe = 50; p.warstwy = 1;
        auto w = oblicz(p);
        CHECK(w && std::fabs(w->indukcyjnoscUH - 8.511) < 0.01 && w->dokladny && std::fabs(w->wheelerUH / w->indukcyjnoscUH - 1.0) < 0.05);
        CHECK(w && w->zwojeRazem == 50 && std::fabs(w->dlugoscUzwojeniaMm - 26.5) < 1e-9);
        CHECK(w && std::fabs(w->dlugoscDrutuM - 50 * pi * 10.53 * 1e-3) < 1e-9 && std::fabs(w->opor20Ohm - 0.1452) < 0.001);
        p.warstwy = 10;
        w = oblicz(p);
        CHECK(w && std::fabs(w->indukcyjnoscUH - 1331.9) < 0.5 && std::fabs(w->wheelerUH / w->indukcyjnoscUH - 1.0) < 0.05);
        CHECK(w && std::fabs(w->gruboscUzwojeniaMm - 5.3) < 1e-9 && std::fabs(w->sredniPromienMm - 7.65) < 1e-9);
        // L rośnie z N^2 (zwoje obok siebie, rzędu): podwojenie warstw daje wyraźnie > 3x
        Parametry p2 = p; p2.warstwy = 20;
        CHECK(oblicz(p2)->indukcyjnoscUH > 3.0 * w->indukcyjnoscUH);
        // nieprawidłowe dane
        Parametry zle = p; zle.drutMm = 0;
        CHECK(!oblicz(zle)); zle = p; zle.drutZIzolacjaMm = 0.3; CHECK(!oblicz(zle)); zle = p; zle.skokMm = 0.4; CHECK(!oblicz(zle));
        zle = p; zle.zwojeNaWarstwe = 0; CHECK(!oblicz(zle));
        // bardzo duża cewka: wzór Wheelera i znacznik
        Parametry duza = p; duza.zwojeNaWarstwe = 3000; duza.warstwy = 100;
        const auto wd = oblicz(duza);
        CHECK(wd && !wd->dokladny && wd->indukcyjnoscUH == wd->wheelerUH);
        // dobór
        p.warstwy = 1;
        auto d = zwojeDlaIndukcyjnosci(p, 10.0);
        CHECK(d && d->zwoje == 58 && d->indukcyjnoscUH >= 10.0 && d->poprzedniaUH < 10.0);
        p.warstwy = 5; p.zwojeNaWarstwe = 30;
        auto dw = warstwyDlaIndukcyjnosci(p, 500.0);
        CHECK(dw && dw->indukcyjnoscUH >= 500.0 && dw->poprzedniaUH < 500.0);
        CHECK(!zwojeDlaIndukcyjnosci(p, 0) && !zwojeDlaIndukcyjnosci(p, 1e12));
        // Wheeler: 10 mm, 50 zwojów, długość 26,5 mm
        CHECK(std::fabs(wheelerJednowarstwowaUH(10.53, 26.5, 50) - 10.53 * 10.53 * 2500.0 / (25.4 * (18 * 10.53 + 40 * 26.5))) < 1e-9);
    }

    // LM317 / LM337
    {
        using namespace StabilizatorRegulowany;
        Parametry p = typowe(Typ::LM317);
        CHECK(p.iMinA == 3.5e-3 && typowe(Typ::LM337).iMinA == 2.5e-3);
        // 1,25·(1 + 2200/240) + 50 µA·2200 = 12,708 + 0,11 = 12,818 V
        CHECK(std::fabs(napiecieWyjsciowe(Typ::LM317, 240, 2200, p) - 12.8183) < 1e-3);
        CHECK(napiecieWyjsciowe(Typ::LM337, 240, 2200, p) < 0 && std::fabs(napiecieWyjsciowe(Typ::LM337, 240, 2200, p) + 12.8183) < 1e-3);
        CHECK(std::fabs(napiecieWyjsciowe(Typ::LM317, 240, 0, p) - 1.25) < 1e-12);
        const auto r2 = r2DlaNapiecia(12.0, 240, p);
        CHECK(r2 && std::fabs(*r2 - 2044.3) < 0.5 && std::fabs(napiecieWyjsciowe(Typ::LM317, 240, *r2, p) - 12.0) < 1e-9);
        CHECK(!r2DlaNapiecia(1.25, 240, p) && !r2DlaNapiecia(5, 0, p) && r2DlaNapiecia(-12.0, 240, p));
        const Zakres z = rozrzutNapiecia(240, 2200, p);
        CHECK(z.min < z.typ && z.typ < z.maks && std::fabs(z.min - 1.20 * (1 + 2200.0 / 240)) < 1e-9);
        const Moc m = bilans(18, 12, 0.5, 240, p);
        CHECK(std::fabs(m.stratyW - 3.0) < 1e-9 && std::fabs(m.wejscieMinV - 15.0) < 1e-9 && m.pradMinimalnyOk && std::fabs(m.pradDzielnikaA - 1.25 / 240) < 1e-12);
        CHECK(!bilans(18, 12, 0.0001, 24000, p).pradMinimalnyOk);
    }

    std::cerr << sprawdzenia << " sprawdzeń, " << bledy << " błędów\n";
    return bledy ? 1 : 0;
}
