#include "SchematyLampowe.h"

#include <algorithm>

namespace SchematyLampowe {
namespace {

struct B {
    Schemat s;
    void lin(double x1, double y1, double x2, double y2, bool kr = false) {
        ElementRysunku e; e.typ = Typ::Linia; e.x1 = x1; e.y1 = y1; e.x2 = x2; e.y2 = y2; e.kreska = kr; s.elementy.push_back(e);
    }
    void el(Typ t, double x, double y, double x2, double y2, const std::string& txt, bool u, bool kr = false) {
        ElementRysunku e; e.typ = t; e.x1 = x; e.y1 = y; e.x2 = x2; e.y2 = y2; e.tekst = txt; e.uzyty = u; e.kreska = kr;
        s.elementy.push_back(e);
    }
    void rez(double x, double y1, double y2, const std::string& n, bool u) { el(Typ::RezystorPion, x, y1, 0, y2, n, u); }
    void kondPoz(double x1, double x2, double y, const std::string& n, bool u, bool kr = false) { el(Typ::KondPoz, x1, y, x2, y, n, u, kr); }
    void kondPion(double x, double y1, double y2, const std::string& n, bool u, bool kr = false) { el(Typ::KondPion, x, y1, 0, y2, n, u, kr); }
    void trioda(double cx, double cy) { el(Typ::Trioda, cx, cy, 0, 0, "", true); }
    void masa(double x, double y) { el(Typ::Masa, x, y, 0, 0, "", false); }
    void zas(double x, double y, const std::string& n = "+Ua") { el(Typ::Zasilanie, x, y, 0, 0, n, false); }
    void zacisk(double x, double y, const std::string& n) { el(Typ::Zacisk, x, y, 0, 0, n, false); }
    void kropka(double x, double y) { el(Typ::Kropka, x, y, 0, 0, "", false); }
    void tekst(double x, double y, const std::string& n, bool u = false) { el(Typ::Tekst, x, y, 0, 0, n, u); }
};

Schemat rc(bool autom) {
    B b;
    b.zas(100, 6);
    b.lin(100, 6, 100, 12);
    b.rez(100, 12, 40, "Ra", true);
    b.lin(100, 40, 100, 65);
    b.trioda(100, 95);
    b.kropka(100, 55);
    b.kondPoz(100, 165, 55, "Cs", true);
    b.lin(165, 55, 200, 55);
    b.zacisk(205, 55, "wy");
    b.kropka(185, 55);
    b.rez(185, 55, 100, "Rs", true);
    b.lin(185, 100, 185, 150);
    b.masa(185, 150);
    b.zacisk(25, 95, "we");
    b.lin(30, 95, 70, 95);
    if (autom) {
        b.lin(100, 125, 100, 135);
        b.rez(100, 135, 165, "Rk", true);
        b.lin(100, 165, 100, 175);
        b.masa(100, 175);
    } else {
        b.lin(100, 125, 100, 160);
        b.masa(100, 160);
        b.tekst(25, 115, "-Us (polaryzacja stała)");
    }
    b.s.podpis = "Cak, Cas, Csk – pojemności międzyelektrodowe lampy (Sa, Ka, Rwew – dane lampy)";
    return b.s;
}

Schemat anodowy() {
    B b;
    b.zas(100, 4);
    b.lin(100, 4, 100, 15);
    b.trioda(100, 45);                 // górna: anoda 15, siatka (70,45), katoda 75
    b.lin(70, 45, 45, 45);
    b.masa(45, 45);
    b.lin(100, 75, 100, 95);
    b.kropka(100, 85);
    b.trioda(100, 125);                // dolna: anoda 95, siatka (70,125), katoda 155
    b.kondPoz(100, 150, 85, "Cs", true);
    b.lin(150, 85, 190, 85);
    b.zacisk(195, 85, "wy");
    b.kropka(172, 85);
    b.rez(172, 85, 135, "Ro", true);
    b.lin(172, 135, 172, 170);
    b.masa(172, 170);
    b.zacisk(15, 125, "we");
    b.lin(20, 125, 70, 125);
    b.kropka(40, 125);
    b.rez(40, 125, 165, "Rs", true);
    b.masa(40, 165);
    b.lin(100, 155, 100, 162);
    b.rez(100, 162, 190, "Rk", false);
    b.masa(100, 190);
    b.kondPion(72, 162, 190, "Ck", false, true);
    b.lin(72, 162, 100, 162, true);
    b.lin(72, 190, 100, 190, true);
    b.s.podpis = "Górna trioda jest dynamicznym obciążeniem dolnej; Cak, Cas, Csk – pojemności lamp";
    return b.s;
}

Schemat katodowy() {
    B b;
    b.zas(100, 6);
    b.lin(100, 6, 100, 55);
    b.trioda(100, 85);                 // anoda 55, siatka (70,85), katoda 115
    b.zacisk(15, 85, "we");
    b.lin(20, 85, 70, 85);
    b.kropka(40, 85);
    b.rez(40, 85, 140, "Rs", true);    // Rs od siatki do węzła Rk/R
    b.lin(40, 140, 40, 150);
    b.lin(40, 150, 100, 150);
    b.lin(100, 115, 100, 120);
    b.rez(100, 120, 140, "Rk", true);
    b.lin(100, 140, 100, 150);
    b.kropka(100, 150);
    b.rez(100, 150, 190, "R", true);
    b.masa(100, 190);
    b.kropka(100, 117);
    b.lin(100, 117, 170, 117);
    b.zacisk(175, 117, "wy");
    b.s.podpis = "Rs włączony między siatkę a węzeł Rk–R; Csk – pojemność siatka–katoda lampy";
    return b.s;
}

Schemat inwerter() {
    B b;
    b.zas(140, 2);
    b.lin(140, 2, 140, 12);
    b.lin(100, 12, 185, 12);
    const double c1 = 100, c2 = 185;
    for (double cx : {c1, c2}) {
        b.lin(cx, 12, cx, 14);
        b.rez(cx, 14, 42, "Ra", true);
        b.lin(cx, 42, cx, 55);
        b.trioda(cx, 85);
        b.kropka(cx, 50);
    }
    b.lin(c1, 50, 20, 50);
    b.zacisk(20, 50, "wy1");
    b.rez(20, 50, 110, "Rs", true);
    b.lin(20, 110, 20, 118);
    b.masa(20, 118);
    b.lin(c2, 50, 235, 50);
    b.zacisk(235, 50, "wy2");
    b.rez(235, 50, 110, "Rs", true);
    b.lin(235, 110, 235, 118);
    b.masa(235, 118);
    b.zacisk(55, 85, "we");
    b.lin(60, 85, 70, 85);
    b.lin(155, 85, 140, 85);
    b.lin(140, 85, 140, 100);
    b.masa(140, 100);
    b.lin(c1, 115, c1, 135);
    b.lin(c2, 115, c2, 135);
    b.lin(c1, 135, c2, 135);
    b.kropka(142, 135);
    b.lin(142, 135, 142, 150);
    b.rez(142, 150, 195, "Rk", true);
    b.lin(142, 195, 142, 200);
    b.masa(142, 200);
    b.s.podpis = "Ra – rezystory anodowe, Rs – obciążenia anod, Rk – wspólny rezystor katod";
    return b.s;
}

Schemat mi() {
    B b;
    b.zas(100, 4);
    b.lin(100, 4, 100, 15);
    b.trioda(100, 45);
    b.lin(100, 75, 100, 78);
    b.kropka(100, 78);
    b.lin(100, 78, 160, 78);
    b.zacisk(165, 78, "wy");
    b.kropka(145, 78);
    b.rez(145, 78, 130, "R0", true);
    b.masa(145, 175);
    b.lin(145, 130, 145, 175);
    b.rez(100, 80, 105, "Rk2", true);
    b.lin(100, 105, 100, 112);
    b.kropka(100, 112);
    b.lin(70, 45, 45, 45);
    b.lin(45, 45, 45, 112);
    b.lin(45, 112, 100, 112);
    b.lin(100, 112, 100, 120);
    b.trioda(100, 150);                // anoda 120, siatka (70,150), katoda 180
    b.zacisk(15, 150, "we");
    b.lin(20, 150, 70, 150);
    b.lin(100, 180, 100, 190);
    b.masa(100, 190);
    b.s.podpis = "Dolna trioda wzmacnia, górna jest wtórnikiem katodowym i dynamicznym obciążeniem anody";
    return b.s;
}

Schemat srpp() {
    B b;
    b.zas(100, 4);
    b.lin(100, 4, 100, 20);
    b.trioda(100, 50);                 // anoda 20, siatka (70,50), katoda 80
    b.lin(100, 80, 100, 84);
    b.rez(100, 84, 108, "Rk", true);
    b.lin(100, 108, 100, 115);
    b.kropka(100, 115);
    b.lin(100, 115, 165, 115);
    b.zacisk(170, 115, "wy");
    b.kropka(150, 115);
    b.rez(150, 115, 165, "RL", true);
    b.masa(150, 190);
    b.lin(150, 165, 150, 190);
    b.lin(100, 115, 100, 120);
    b.trioda(100, 150);                // anoda 120, siatka (70,150), katoda 180
    b.zacisk(15, 150, "we");
    b.lin(20, 150, 70, 150);
    b.lin(100, 180, 100, 184);
    b.kropka(100, 184);
    b.rez(100, 184, 208, "Rk", true);
    b.masa(100, 215);
    b.lin(100, 208, 100, 215);
    b.lin(70, 50, 40, 50);
    b.kondPion(40, 50, 184, "", false);
    b.lin(40, 184, 100, 184);
    b.s.podpis = "Obie triody jednakowe, Rk w katodzie każdej; górna siatka sprzężona z dolną katodą";
    return b.s;
}

Schemat white() {
    B b;
    b.zas(100, 4);
    b.lin(100, 4, 100, 8);
    b.rez(100, 8, 34, "Ra", true);
    b.lin(100, 34, 100, 40);
    b.kropka(100, 38);
    b.trioda(100, 70);                 // anoda 40, siatka (70,70), katoda 100
    b.zacisk(15, 70, "we");
    b.lin(20, 70, 70, 70);
    b.lin(100, 100, 100, 108);
    b.kropka(100, 108);
    b.lin(100, 108, 100, 108);
    b.trioda(100, 138);                // anoda 108, siatka (70,138), katoda 168
    b.lin(100, 168, 100, 190);
    b.masa(100, 190);
    b.lin(100, 38, 40, 38, false);
    b.kondPion(40, 38, 138, "C", false);
    b.lin(40, 138, 70, 138);
    b.lin(100, 108, 160, 108);
    b.zacisk(165, 108, "wy");
    b.kropka(145, 108);
    b.rez(145, 108, 165, "RL", true);
    b.lin(145, 165, 145, 190);
    b.masa(145, 190);
    b.s.podpis = "Górna trioda – wtórnik katodowy, dolna – aktywne obciążenie katody; Ra dobierany optymalnie";
    return b.s;
}

Schemat kaskodowy(bool super) {
    B b;
    b.zas(100, 4);
    b.lin(100, 4, 100, 15);
    b.trioda(100, 45);                 // L2: anoda 15, siatka (70,45), katoda 75
    b.lin(100, 75, 100, 95);
    b.kropka(100, 85);
    b.trioda(100, 125);                // L1: anoda 95, siatka (70,125), katoda 155
    b.zacisk(15, 125, "we");
    b.lin(20, 125, 70, 125);
    b.lin(100, 155, 100, 165);
    b.kropka(100, 165);
    b.lin(100, 165, 170, 165);
    b.zacisk(175, 165, "wy");
    if (!super) {
        b.kropka(150, 165);
        b.rez(150, 165, 205, "R0", true);
        b.masa(150, 210);
        b.lin(150, 205, 150, 210);
    } else {
        b.masa(100, 200);
        b.rez(100, 165, 195, "Rk", false);
        b.lin(100, 195, 100, 200);
    }
    b.lin(70, 45, 40, 45);
    b.rez(40, 45, 120, "Ra", true);
    b.lin(40, 120, 40, 165);
    b.lin(40, 165, 100, 165);
    b.s.podpis = super ? "Superwtórnik rzędu n: n stopni kompensujących anodę (rysunek pokazuje pierwszy stopień)"
                       : "Dolna trioda – wtórnik, górna – kaskoda; Ra w pętli sprzężenia zwrotnego";
    return b.s;
}

} // namespace

std::vector<std::string> Schemat::uzyte() const {
    std::vector<std::string> w;
    for (const auto& e : elementy)
        if (e.uzyty && !e.tekst.empty() && std::find(w.begin(), w.end(), e.tekst) == w.end()) w.push_back(e.tekst);
    return w;
}

bool Schemat::maEtykiete(const std::string& n) const {
    return std::any_of(elementy.begin(), elementy.end(), [&](const ElementRysunku& e) { return e.tekst == n; });
}

Schemat zbuduj(int uklad, bool automatyczna) {
    switch (uklad) {
    case 0: return rc(automatyczna);
    case 1: return anodowy();
    case 2: return katodowy();
    case 3: return inwerter();
    case 4: return mi();
    case 5: return srpp();
    case 6: return white();
    case 7: return kaskodowy(false);
    case 8: return kaskodowy(true);
    default: return {};
    }
}

} // namespace SchematyLampowe
