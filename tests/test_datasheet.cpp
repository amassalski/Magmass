#include "../src/Logika/OdczytDatasheetu.h"
#include <cstdio>
#include <string>

static int bledy = 0;
#define SPR(w) do { if (!(w)) { std::printf("BLAD: %s (linia %d)\n", #w, __LINE__); ++bledy; } } while (0)

static std::string param(const OdczytDatasheetu::Wynik& w, const std::string& n) {
    for (const auto& p : w.parametry) if (p.nazwa == n) return p.wartosc;
    return "";
}

int main() {
    const std::string t =
        "LM7805 Positive Voltage Regulator\n"
        "Copyright 2020 Example Corp. All rights reserved\n"
        "Features\n"
        "  Output current up to 1.5 A\n"
        "  Input voltage range 7 V to 25 V\n"
        "  Operating temperature range: \xE2\x88\x92" "40\xC2\xB0" "C to +125\xC2\xB0" "C\n"
        "  Available in TO-220 package, 3-pin\n"
        "  Oscillator frequency 100 kHz\n"
        "  Power dissipation 15 W\n";
    auto w = OdczytDatasheetu::wyciagnij(t);
    SPR(w.opis == "LM7805 Positive Voltage Regulator");
    SPR(w.obudowa == "TO-220");
    SPR(param(w, "Prąd maksymalny") == "1.5 A");
    SPR(param(w, "Napięcie zasilania") == "7 V ... 25 V");
    SPR(param(w, "Temperatura pracy") == "-40 ... +125 °C");
    SPR(param(w, "Liczba wyprowadzeń") == "3");
    SPR(param(w, "Częstotliwość") == "100 kHz");
    SPR(param(w, "Moc maksymalna") == "15 W");
    // tranzystor: sekcje Absolute Maximum Ratings i Electrical Characteristics, hFE, zastosowania
    const std::string tr =
        "BC547 NPN General Purpose Transistor\n"
        "\n"
        "Applications\n"
        "  \xE2\x80\xA2 Audio preamplifiers\n"
        "  \xE2\x80\xA2 General purpose switching\n"
        "\n"
        "Absolute Maximum Ratings (TA = 25\xC2\xB0" "C unless otherwise noted)\n"
        "Rating                              Symbol     Value     Unit\n"
        "Collector-Emitter Voltage           VCEO        45       Vdc\n"
        "Collector-Base Voltage              VCBO        50       Vdc\n"
        "Emitter-Base Voltage                VEBO        6.0      Vdc\n"
        "Collector Current - Continuous      IC         100       mAdc\n"
        "Total Device Dissipation @ TA = 25\xC2\xB0" "C   PD   625   mW\n"
        "Operating and Storage Junction Temperature Range  TJ, Tstg  \xE2\x88\x92" "55 to +150  \xC2\xB0" "C\n"
        "\n"
        "Electrical Characteristics\n"
        "DC Current Gain  hFE   (IC = 2.0 mA, VCE = 5.0 V)    110    -    800\n"
        "Collector-Emitter Saturation Voltage  VCE(sat)  (IC = 10 mA, IB = 0.5 mA)   0.09   0.25   V\n"
        "Current-Gain - Bandwidth Product  fT   (IC = 10 mA, VCE = 5 V, f = 100 MHz)   300   MHz\n"
        "Noise Figure  NF   (IC = 0.2 mA)   2.0   10   dB\n"
        "\n"
        "Ordering Information\n"
        "BC547B  TO-92\n";
    auto t2 = OdczytDatasheetu::wyciagnij(tr);
    SPR(param(t2, "Maks.: Napięcie kolektor-emiter (VCEO)") == "45 V");
    SPR(param(t2, "Maks.: Napięcie kolektor-baza (VCBO)") == "50 V");
    SPR(param(t2, "Maks.: Napięcie emiter-baza (VEBO)") == "6.0 V");
    SPR(param(t2, "Maks.: Prąd kolektora (IC)") == "100 mA");
    SPR(param(t2, "Maks.: Moc admisyjna (Ptot)") == "625 mW");
    SPR(param(t2, "Maks.: Temperatura złącza / przechowywania (Tj, Tstg)") == "-55 ... +150 \xC2\xB0" "C" ||
        param(t2, "Maks.: Temperatura złącza / przechowywania (Tj, Tstg)") == "-55 ... 150 \xC2\xB0" "C");
    SPR(param(t2, "Elektr.: Wzmocnienie prądowe (hFE)") == "110 ... 800");
    SPR(param(t2, "Elektr.: Napięcie nasycenia C-E (VCE(sat))") == "0.09 ... 0.25 V");
    SPR(param(t2, "Elektr.: Częstotliwość graniczna (fT)") == "300 MHz");
    SPR(param(t2, "Elektr.: Współczynnik szumów (NF)") == "2.0 ... 10 dB");
    SPR(t2.uwagi.find("Audio preamplifiers") != std::string::npos && t2.uwagi.find("Zastosowanie") == 0);
    for (const auto& x : t2.parametry) if (x.nazwa.find("Maks.: ") == 0 || x.nazwa.find("Elektr.: ") == 0) std::printf("  %s = %s\n", x.nazwa.c_str(), x.wartosc.c_str());

    // kondensator elektrolityczny (układ tabel pdftotext -layout, kółko nad literą zamiast stopnia)
    const std::string el =
        "ALUMINUM  ELECTROLYTIC  CAPACITORS\n\nURZ\n"
        "                                  Compact & Low-Profile Sized,\n"
        "Specifications\n"
        "Category Temperature Range                            \xE2\x80\x93" "55 to +105\xCB\x9A" "C\n"
        "Rated Voltage Range                                   16 to 100V\n"
        "Rated Capacitance Range                               47 to 6800\xC2\xB5" "F\n"
        "Capacitance Tolerance                                 \xC2\xB1 20% at 120Hz, 20\xCB\x9A" "C\n"
        "                                                       After 1 minute's application of rated voltage at 20\xC2\xB0" "C, leakage current is not more than 0.03CV(\xCE\xBC" "A).\n"
        "Leakage Current                                       After 2 minutes' application of rated voltage at 20\xC2\xB0" "C, leakage current is not more than 0.01CV(\xCE\xBC" "A).\n"
        "Tangent of loss angle (tan \xCE\xB4)    Rated voltage (V)  16  25  35\n"
        "                    tan \xCE\xB4 (max.)       0.20      0.16      0.14      0.12      0.10      0.08\n"
        "voltage is applied for 1000 hours at 105\xC2\xB0" "C.\n"
        "         \xCF\x86" "D               10     12.5     16        18                                                                          Pb-free PET sleeve\n"
        "          P             5.0    5.0      7.5       7.5                                                                             10                     PD\n"
        "capacitor capacitor\n";
    auto we = OdczytDatasheetu::wyciagnij(el);
    SPR(param(we, "Rodzaj kondensatora") == "Elektrolityczny aluminiowy");
    SPR(param(we, "Temperatura pracy") == "-55 ... +105 \xC2\xB0" "C");
    SPR(param(we, "Napięcie znamionowe") == "16 ... 100 V");
    SPR(param(we, "Pojemność") == "47 ... 6800 \xC2\xB5" "F");
    SPR(param(we, "Tolerancja pojemności") == "\xC2\xB1" "20 %");
    SPR(param(we, "Prąd upływu (po 1 min)").find("0.03") != std::string::npos);
    SPR(param(we, "Prąd upływu (po 2 min)").find("0.01") != std::string::npos);
    SPR(param(we, "Tangens kąta strat tg \xCE\xB4").find("0.08 ... 0.2") == 0);
    SPR(param(we, "Trwałość") == "1000 h / 105 \xC2\xB0" "C");
    SPR(param(we, "Rozstaw wyprowadzeń") == "5 ... 7.5 mm");
    SPR(param(we, "Średnica obudowy \xCF\x86" "D") == "10 ... 18 mm");
    SPR(we.opis == "Kondensator elektrolityczny aluminiowy URZ");

    // kondensator foliowy EMI (X2): karta dwukolumnowa
    const std::string fo =
        "Film Capacitors\n\nEMI Suppression Capacitors (MKP)\n\nSeries/Type:          B32921C/D ... B32928C/D\n"
        "EMI suppression capacitors (MKP)                                                 B32921C/D ... B32928C/D\n"
        "X2 / 305 V AC\n\n"
        "Typical applications                                 Dimensional drawings\n"
        "  X2 class for interference suppression              Drawing 1\n"
        "  \"Across the line\" applications\n"
        "Climatic\n"
        "  Max. operating temperature: 110 \xC2\xB0" "C\n"
        "  Climatic category (IEC 60068-1): 40/110/56\n"
        "Terminals\n"
        "  Parallel wire leads, lead-free tinned\n"
        "Lead spacing 10 mm 15 mm 22.5 mm 27.5 mm 37.5 mm 52.5 mm\n"
        "Lead CR Max. dimensions Ordering code\n"
        "spacing w\xC3\x97h\xC3\x97l\n"
        "mm \xC2\xB5" "F mm\n"
        "10 0.010 4.0 \xC3\x97 9.0 \xC3\x97 13.0 B32921C3103+*** 4000 2\n"
        "0.33 6.0 \xC3\x97 15.0 \xC3\x97 26.5 B32923C3334M*** 2720 2\n"
        "52.5 30 35.0 \xC3\x97 50.0 \xC3\x97 57.5 B32928C3306M*** 108 4\n"
        "+ = Capacitance tolerance code:\n"
        "    M = \xC2\xB1" "20%\n    K = \xC2\xB1" "10%\n"
        "Maximum continuous DC voltage VDC 630 V\n"
        "Rated AC voltage (IEC 60384-14)                305 V (50/60 Hz)\n"
        "DC test voltage                                2121 V, 2 s\n"
        "Insulation resistance Rins        CR<=0.33 \xC2\xB5" "F      CR>0.33 \xC2\xB5" "F\n"
        "or time constant                 100 000 M\xCE\xA9 30 000 s\n"
        "capacitors capacitor\n";
    auto wf = OdczytDatasheetu::wyciagnij(fo);
    SPR(param(wf, "Rodzaj kondensatora") == "Foliowy polipropylenowy (MKP)");
    SPR(param(wf, "Napięcie znamionowe AC") == "305 V AC");
    SPR(param(wf, "Klasa przeciwzakłóceniowa") == "X2");
    SPR(param(wf, "Temperatura pracy") == "-40 ... +110 \xC2\xB0" "C");
    SPR(param(wf, "Maks. temperatura pracy") == "110 \xC2\xB0" "C");
    SPR(param(wf, "Pojemność") == "0.01 ... 30 \xC2\xB5" "F");
    SPR(param(wf, "Tolerancja pojemności") == "\xC2\xB1" "10 % / \xC2\xB1" "20 %");
    SPR(param(wf, "Rozstaw wyprowadzeń") == "10 ... 52.5 mm");
    SPR(param(wf, "Maks. napięcie ciągłe DC") == "630 V");
    SPR(param(wf, "Napięcie probiercze DC") == "2121 V / 2 s");
    SPR(param(wf, "Rezystancja izolacji").find("100000") == 0);
    SPR(param(wf, "Wyprowadzenia") == "Radialne");
    SPR(wf.opis == "Kondensator foliowy polipropylenowy (MKP) B32921C/D ... B32928C/D");
    SPR(wf.uwagi == "Zastosowanie: X2 class for interference suppression; \"Across the line\" applications");

    auto p = OdczytDatasheetu::wyciagnij("");
    SPR(p.parametry.empty() && p.opis.empty() && p.obudowa.empty());
    std::printf(bledy ? "BLEDY: %d\n" : "test_datasheet OK\n", bledy);
    return bledy ? 1 : 0;
}
