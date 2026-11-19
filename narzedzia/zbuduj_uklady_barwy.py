#!/usr/bin/env python3
"""Generuje src/Logika/UkladyBarwyDane.inc z katalogu circuits projektu tonestack
(https://github.com/olivergardiner/tonestack, GPL-3.0, oparty na Duncan's Tone Stack Calculator).

Używane są wyłącznie schematy połączeń (netlisty .cir) i wartości katalogowe (tonestacks.tsc): topologie biernych układów
regulacji barwy. Silnik obliczeń (równania węzłowe) jest własny, w src/Logika/KalkulatoryWzmacniaczy.cpp.

Użycie: python3 narzedzia/zbuduj_uklady_barwy.py <katalog tonestack>/circuits
"""
import json, os, re, sys

NAZWY = {   # plik, nazwa wyświetlana
    "fender": "Fender (Twin Reverb, katalog tonestack)",
    "marshall": "Marshall JCM800 (katalog tonestack)",
    "vox": "Vox AC30",
    "james": "James",
    "baxandall": "Baxandall",
    "bandmaster": "Fender Bandmaster",
    "bench": "Bench (z cewkami)",
    "voigt": "Voigt",
    "boneray": "Bone Ray",
    "tubedriver": "Tube Driver",
    "marshall18": "Marshall 18 W",
    "bigmuff": "Big Muff",
    "moonlight": "Moonlight",
}
FUNKCJE = {"Bass": "Bas", "Mid": "Środek", "Treble": "Wysokie", "Tilt": "Nachylenie", "Tone": "Barwa"}
TAPERY = {0: "Liniowy", 1: "Log", 2: "LogA", 3: "OdwrotnyLog"}

def wartosc(tekst):
    m = re.fullmatch(r"([0-9.]+)(MEG|[PNUMKGRH]?)", tekst.upper())
    if not m: raise ValueError(tekst)
    mn = {"": 1, "R": 1, "H": 1, "P": 1e-12, "N": 1e-9, "U": 1e-6, "M": 1e-3, "K": 1e3, "MEG": 1e6, "G": 1e9}[m.group(2)]
    return float(m.group(1)) * mn

def wczytaj(dir_):
    katalog = json.load(open(os.path.join(dir_, "tonestacks.tsc"), encoding="utf-8"))["circuits"]
    wyniki = []
    for nazwa_pliku, nazwa in NAZWY.items():
        cir = [l.split() for l in open(os.path.join(dir_, nazwa_pliku + ".cir"), encoding="utf-8") if l.strip() and not l.startswith(("*", "."))][1:]
        kat = next(c for c in katalog if os.path.basename(c["command"].split()[-1]).replace(".cir", "") == nazwa_pliku)
        skl = []
        for t in cir:
            n = t[0].upper()
            if n in ("VIN", "R98", "R99"): continue
            a, b = int(t[1]), int(t[2])
            m = re.fullmatch(r"RP(\d)(LO|HI)", n)
            if m: skl.append(("PotLo" if m.group(2) == "LO" else "PotHi", int(m.group(1)), a, b)); continue
            m = re.fullmatch(r"([RCL])(\d+)", n)
            if m: skl.append((m.group(1), int(m.group(2)), a, b, wartosc(t[3]))); continue
            raise ValueError(n)
        r = {}; c = {}; l = {}
        for x in kat.get("resistors", []): r[x["id"]] = x["value"]
        for x in kat.get("capacitors", []): c[x["id"]] = x["value"] * 1e-12
        for x in kat.get("inductors", []): l[x["id"]] = x["value"] * 1e-6
        for s in skl:   # brakujące wartości z netlisty
            if len(s) == 5:
                d = {"R": r, "C": c, "L": l}[s[0]]
                d.setdefault(s[1], s[4])
        pots = {}
        for k in ("pot1", "pot2", "pot3"):
            if k in kat:
                p = kat[k]; pots[int(k[3])] = (FUNKCJE[p["function"]], float(p["value"]), TAPERY[p["type"]])
        wyniki.append(dict(nazwa=nazwa, skl=[s[:4] for s in skl], r=r, c=c, l=l, pots=pots, rs=float(kat["rS"]["value"]), rl=float(kat["rL"]["value"])))
    return wyniki

def cpp(u):
    sk = ", ".join('{Typ::%s, %d, %d, %d}' % ({"R": "R", "C": "C", "L": "L"}.get(s[0], s[0]), s[1], s[2], s[3]) for s in u["skl"])
    def tab(d, n): return "{0, " + ", ".join(repr(float(d.get(i, 0))) for i in range(1, n)) + "}"
    pots = ", ".join('{"%s", %r, Taper::%s}' % (u["pots"][i][0], u["pots"][i][1], u["pots"][i][2]) if i in u["pots"] else '{"", 0, Taper::Liniowy}' for i in (1, 2, 3))
    return '    {"%s", {%s}, %s, %s, %s, %r, %r, {{}, %s}},\n' % (u["nazwa"], sk, tab(u["r"], 6), tab(u["c"], 6), tab(u["l"], 3), u["rs"], u["rl"], pots)

if __name__ == "__main__":
    dane = wczytaj(sys.argv[1])
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Logika", "UkladyBarwyDane.inc")
    with open(out, "w", encoding="utf-8") as f:
        f.write("// GENEROWANE przez narzedzia/zbuduj_uklady_barwy.py z katalogu circuits projektu tonestack (GPL-3.0). Nie edytuj ręcznie.\n")
        for u in dane: f.write(cpp(u))
    print("OK:", len(dane), "układów")
