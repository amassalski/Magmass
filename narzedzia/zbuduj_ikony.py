#!/usr/bin/env python3
"""Rysuje ikony programu (własny, płaski zestaw, bez cudzych licencji) i zapisuje:
  assets/ikony/<nazwa>_<rozmiar>.png   oraz   src/GUI/IkonyDane.h (PNG-i wkompilowane w program).
Uruchomienie: python3 narzedzia/zbuduj_ikony.py"""
import io, math, os, sys
from PIL import Image, ImageDraw

KAT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
S = 400                       # płótno robocze; współrzędne w skali 0..100
def k(v): return v * S / 100

NIEB, NIEB_J, NIEB_C = "#2f6fb3", "#6fa8e0", "#1d4a7c"
POM, ZIEL, CZERW, ZOLT = "#f08a24", "#3aa655", "#d64545", "#f2c94c"
SZ, SZ_J, BIAL = "#5b6770", "#c9d1d8", "#ffffff"

def nowy(): return Image.new("RGBA", (S, S), (0, 0, 0, 0))

def rr(d, x0, y0, x1, y1, r, fill, outline=None, w=0):
    d.rounded_rectangle([k(x0), k(y0), k(x1), k(y1)], radius=k(r), fill=fill, outline=outline, width=int(k(w)))

def linia(d, pts, kol, w):
    d.line([(k(x), k(y)) for x, y in pts], fill=kol, width=int(k(w)), joint="curve")

def wielokat(d, pts, kol): d.polygon([(k(x), k(y)) for x, y in pts], fill=kol)
def elipsa(d, x0, y0, x1, y1, fill=None, outline=None, w=0):
    d.ellipse([k(x0), k(y0), k(x1), k(y1)], fill=fill, outline=outline, width=int(k(w)))

def ikona_app():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 6, 8, 94, 94, 10, NIEB, NIEB_C, 2)                      # szafka
    for i, y in enumerate((14, 39, 64)):                          # szuflady
        rr(d, 12, y, 88, y + 21, 5, NIEB_J, NIEB_C, 1.5)
        rr(d, 38, y + 7, 62, y + 14, 3.5, BIAL, SZ, 1)            # uchwyt
    # rezystor w środkowej szufladzie jako znak firmowy
    rr(d, 18, 45, 34, 52, 2, "#e2c896", "#8a6a3a", 0.8)
    for x, c in ((21, ZOLT), (25, "#8e44ad"), (29, CZERW)): d.rectangle([k(x), k(45.5), k(x + 2), k(51.5)], fill=c)
    return im

def i_nowy():
    im = nowy(); d = ImageDraw.Draw(im)
    elipsa(d, 8, 8, 92, 92, ZIEL); linia(d, [(50, 26), (50, 74)], BIAL, 13); linia(d, [(26, 50), (74, 50)], BIAL, 13); return im
def i_edytuj():
    im = nowy(); d = ImageDraw.Draw(im)
    wielokat(d, [(66, 12), (88, 34), (38, 84), (12, 90), (16, 62)], POM)
    wielokat(d, [(16, 62), (38, 84), (12, 90)], "#f2d3a0"); linia(d, [(60, 18), (82, 40)], "#b5610f", 4); return im
def i_stan():
    im = nowy(); d = ImageDraw.Draw(im)
    wielokat(d, [(30, 10), (52, 40), (8, 40)], ZIEL); rr(d, 22, 38, 38, 88, 3, ZIEL)
    wielokat(d, [(70, 90), (92, 60), (48, 60)], CZERW); rr(d, 62, 14, 78, 62, 3, CZERW); return im
def i_usun():
    im = nowy(); d = ImageDraw.Draw(im)
    elipsa(d, 8, 8, 92, 92, CZERW)
    linia(d, [(30, 30), (70, 70)], BIAL, 13); linia(d, [(70, 30), (30, 70)], BIAL, 13); return im
def i_kategoria():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 6, 18, 42, 38, 5, "#e0a82e"); rr(d, 6, 28, 94, 84, 7, ZOLT, "#b8891b", 1.5); return im
def i_import():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 10, 64, 90, 90, 6, SZ); wielokat(d, [(50, 62), (24, 34), (76, 34)], NIEB); rr(d, 42, 8, 58, 36, 3, NIEB); return im
def i_eksport():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 10, 64, 90, 90, 6, SZ); wielokat(d, [(50, 8), (24, 38), (76, 38)], POM); rr(d, 42, 36, 58, 64, 3, POM); return im
def i_etykieta():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 6, 18, 94, 82, 7, BIAL, SZ, 2.5)
    x = 16
    for w in (3, 6, 3, 4, 7, 3, 5, 3, 6, 4, 3):
        d.rectangle([k(x), k(28), k(x + w * 0.9), k(72)], fill="#222"); x += w + 2.2
    return im
def i_szukaj():
    im = nowy(); d = ImageDraw.Draw(im)
    elipsa(d, 8, 8, 64, 64, fill="#dcecfb", outline=NIEB, w=9); linia(d, [(56, 56), (88, 88)], NIEB_C, 14); return im
def i_odswiez():
    im = nowy(); d = ImageDraw.Draw(im)
    d.arc([k(14), k(14), k(86), k(86)], 40, 340, fill=NIEB, width=int(k(12)))
    wielokat(d, [(86, 14), (88, 46), (58, 36)], NIEB); return im
def i_projekt():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 8, 14, 92, 86, 6, "#2e8b57", "#1e5c39", 2)
    linia(d, [(20, 34), (46, 34), (56, 48), (80, 48)], "#bfe8cf", 3.5); linia(d, [(20, 64), (40, 64), (50, 54)], "#bfe8cf", 3.5)
    rr(d, 52, 58, 82, 76, 2.5, "#222"); [d.rectangle([k(54 + i * 8), k(78), k(56.5 + i * 8), k(83)], fill=SZ_J) for i in range(4)]
    return im
def i_raport():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 8, 8, 92, 92, 6, BIAL, SZ, 2.5)
    for x, h, c in ((20, 28, NIEB), (40, 48, POM), (60, 36, ZIEL), (76, 56, CZERW)): rr(d, x, 82 - h, x + 11, 82, 2, c)
    return im
def i_sklep():
    im = nowy(); d = ImageDraw.Draw(im)
    linia(d, [(6, 14), (22, 14), (34, 62), (80, 62)], SZ, 7); wielokat(d, [(26, 26), (90, 26), (80, 54), (32, 54)], POM)
    elipsa(d, 32, 72, 46, 86, fill=SZ); elipsa(d, 66, 72, 80, 86, fill=SZ); return im
def i_polacz():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 30, 8, 42, 34, 3, SZ); rr(d, 58, 8, 70, 34, 3, SZ); rr(d, 22, 32, 78, 62, 8, SZ_J, SZ, 2)
    linia(d, [(50, 62), (50, 92)], SZ, 9); return im
def i_pomoc():
    im = nowy(); d = ImageDraw.Draw(im)
    elipsa(d, 6, 6, 94, 94, NIEB)
    d.arc([k(32), k(18), k(68), k(52)], 180, 410, fill=BIAL, width=int(k(10)))
    linia(d, [(50, 52), (50, 62)], BIAL, 10); elipsa(d, 44, 70, 56, 82, fill=BIAL); return im
def i_narzedzia():
    im = nowy(); d = ImageDraw.Draw(im)
    linia(d, [(4, 50), (22, 50)], SZ, 5); linia(d, [(78, 50), (96, 50)], SZ, 5)
    rr(d, 18, 28, 82, 72, 18, "#e2c896", "#8a6a3a", 2)
    for x, c in ((30, ZOLT), (42, "#8e44ad"), (54, CZERW), (68, "#d4af37")): d.rectangle([k(x), k(29), k(x + 6), k(71)], fill=c)
    return im
def i_generator():
    im = nowy(); d = ImageDraw.Draw(im)
    for r in range(3):
        for c in range(3):
            rr(d, 10 + c * 29, 10 + r * 29, 31 + c * 29, 31 + r * 29, 4, (NIEB, NIEB_J, POM)[(r + c) % 3])
    return im
def i_zamknij():
    im = nowy(); d = ImageDraw.Draw(im)
    linia(d, [(20, 20), (80, 80)], SZ, 12); linia(d, [(80, 20), (20, 80)], SZ, 12); return im


def kolo_zeb(d, cx, cy, r_zew, r_wew, n, kol):
    for i in range(n):
        a = 2 * math.pi * i / n
        ux, uy, nx, ny = math.cos(a), math.sin(a), -math.sin(a), math.cos(a)
        w = r_zew * 0.2
        wielokat(d, [(cx + ux * (r_wew - 2) + nx * w, cy + uy * (r_wew - 2) + ny * w), (cx + ux * r_zew + nx * w * 0.7, cy + uy * r_zew + ny * w * 0.7),
                     (cx + ux * r_zew - nx * w * 0.7, cy + uy * r_zew - ny * w * 0.7), (cx + ux * (r_wew - 2) - nx * w, cy + uy * (r_wew - 2) - ny * w)], kol)
    elipsa(d, cx - r_wew, cy - r_wew, cx + r_wew, cy + r_wew, kol)

def i_rozlacz():
    im = i_polacz(); d = ImageDraw.Draw(im); linia(d, [(14, 86), (86, 14)], CZERW, 9); return im
def i_ustawienia():
    im = nowy(); d = ImageDraw.Draw(im); kolo_zeb(d, 50, 50, 44, 31, 8, SZ); elipsa(d, 36, 36, 64, 64, BIAL); return im
def walec(d, x0, x1, y0, y1, kol, jasny):
    h = (y1 - y0) * 0.16
    rr(d, x0, y0 + h / 2, x1, y1 - h / 2, 1, kol)
    elipsa(d, x0, y1 - h, x1, y1, fill=kol); elipsa(d, x0, y0, x1, y0 + h, fill=jasny)
def i_baza():
    im = nowy(); d = ImageDraw.Draw(im)
    for y0 in (52, 30, 8): walec(d, 14, 86, y0, y0 + 36, NIEB, NIEB_J)
    return im
def i_baza_struktura():
    im = i_baza(); d = ImageDraw.Draw(im); kolo_zeb(d, 72, 72, 24, 17, 8, POM); elipsa(d, 64, 64, 80, 80, BIAL); return im
def i_info():
    im = nowy(); d = ImageDraw.Draw(im); elipsa(d, 6, 6, 94, 94, NIEB); elipsa(d, 43, 18, 57, 32, fill=BIAL); rr(d, 43, 40, 57, 80, 3, BIAL); return im
def i_aparat():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 6, 26, 94, 84, 9, SZ); rr(d, 30, 14, 58, 30, 4, SZ); elipsa(d, 28, 33, 72, 77, fill=SZ_J, outline="#222", w=3); elipsa(d, 40, 45, 60, 65, fill=NIEB); return im
def i_wydaj():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 6, 40, 62, 90, 4, "#c9a063", "#8a6a3a", 2); rr(d, 6, 40, 62, 54, 3, "#e0bd82")
    wielokat(d, [(94, 40), (66, 14), (66, 28), (40, 28), (40, 52), (66, 52), (66, 66)], POM); return im
def i_do_magazynu():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 8, 50, 92, 92, 5, "#c9a063", "#8a6a3a", 2); rr(d, 8, 50, 92, 62, 3, "#e0bd82")
    rr(d, 42, 8, 58, 36, 3, ZIEL); wielokat(d, [(50, 52), (26, 28), (74, 28)], ZIEL); return im
def dokument(d, kol_ramka=SZ, wypelnienie=BIAL):
    wielokat(d, [(18, 6), (64, 6), (84, 26), (84, 94), (18, 94)], wypelnienie)
    linia(d, [(18, 6), (64, 6), (84, 26), (84, 94), (18, 94), (18, 6)], kol_ramka, 2.5); wielokat(d, [(64, 6), (64, 26), (84, 26)], SZ_J)
def i_szablon():
    im = nowy(); d = ImageDraw.Draw(im); dokument(d)
    for y in (42, 56, 70): linia(d, [(26, y), (76, y)], NIEB_J, 4)
    linia(d, [(44, 36), (44, 82)], NIEB_J, 3); return im
def i_pdf():
    im = nowy(); d = ImageDraw.Draw(im); dokument(d); rr(d, 18, 50, 84, 74, 3, CZERW)
    for x in (28, 44, 60): rr(d, x, 55, x + 10, 69, 2, BIAL)
    return im
def i_pdf_odczyt():
    im = i_pdf(); d = ImageDraw.Draw(im); elipsa(d, 44, 40, 82, 78, fill="#dcecfb", outline=NIEB, w=6); linia(d, [(76, 72), (94, 92)], NIEB_C, 9); return im
def i_kamera_termo():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 6, 20, 94, 80, 8, SZ); rr(d, 60, 8, 84, 22, 3, SZ)
    for i, kol in enumerate((NIEB, "#3aa6a6", ZIEL, ZOLT, POM, CZERW)):
        d.pieslice([k(26), k(28), k(74), k(76)], 60 * i, 60 * i + 60, fill=kol)
    elipsa(d, 40, 42, 60, 62, fill=BIAL); return im
def i_kalkulator():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 16, 6, 84, 94, 8, SZ); rr(d, 24, 14, 76, 32, 3, "#bfe8cf")
    for r in range(4):
        for cc in range(3): rr(d, 24 + cc * 18, 40 + r * 13, 36 + cc * 18, 50 + r * 13, 2, POM if cc == 2 else SZ_J)
    return im
def i_audio():
    im = nowy(); d = ImageDraw.Draw(im)
    wielokat(d, [(8, 36), (28, 36), (52, 14), (52, 86), (28, 64), (8, 64)], SZ)
    d.arc([k(44), k(32), k(70), k(68)], 300, 60, fill=NIEB, width=int(k(6))); d.arc([k(48), k(18), k(86), k(82)], 305, 55, fill=NIEB, width=int(k(6))); return im
def i_wykres():
    im = nowy(); d = ImageDraw.Draw(im)
    linia(d, [(14, 8), (14, 86), (94, 86)], SZ, 5)
    linia(d, [(18, 72), (34, 58), (48, 64), (62, 30), (78, 22), (92, 14)], POM, 5)
    for x, y in ((34, 58), (62, 30), (92, 14)): elipsa(d, x - 4, y - 4, x + 4, y + 4, fill=NIEB); return im
def i_zniekszt():
    im = nowy(); d = ImageDraw.Draw(im)
    pts = [(6 + i, 50 - 30 * math.sin(i / 88 * 4 * math.pi)) for i in range(0, 89, 3)]
    linia(d, pts, NIEB_J, 5)
    pts = [(6 + i, 50 - 30 * max(-0.7, min(0.7, 1.3 * math.sin(i / 88 * 4 * math.pi)))) for i in range(0, 89, 3)]
    linia(d, pts, CZERW, 6); return im
def zwoje(d, cx, y0, y1, n, kol, grub=5, szer=14, lewo=True):
    h = (y1 - y0) / n
    for i in range(n):
        if lewo: d.arc([k(cx - szer), k(y0 + i * h), k(cx + szer), k(y0 + (i + 1) * h + h * 0.2)], 90, 270, fill=kol, width=int(k(grub)))
        else: d.arc([k(cx - szer), k(y0 + i * h), k(cx + szer), k(y0 + (i + 1) * h + h * 0.2)], 270, 90, fill=kol, width=int(k(grub)))
def i_dlawik():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 30, 6, 90, 94, 4, SZ_J, SZ, 2); rr(d, 40, 18, 52, 82, 1, BIAL); rr(d, 68, 18, 80, 82, 1, BIAL)
    zwoje(d, 46, 18, 82, 6, POM, 5, 16, True); linia(d, [(8, 20), (46, 20)], POM, 4); linia(d, [(8, 80), (46, 80)], POM, 4); return im
def i_trafo():
    im = nowy(); d = ImageDraw.Draw(im)
    rr(d, 28, 6, 72, 94, 4, SZ_J, SZ, 2); rr(d, 40, 16, 60, 84, 1, BIAL) if False else None
    rr(d, 44, 14, 56, 86, 1, SZ)
    zwoje(d, 44, 16, 84, 6, POM, 5, 20, True); zwoje(d, 56, 16, 84, 6, NIEB, 5, 20, False); return im
def i_trafo_pp():
    im = i_trafo(); d = ImageDraw.Draw(im); elipsa(d, 4, 46, 14, 56, fill=SZ); elipsa(d, 86, 46, 96, 56, fill=SZ); return im
def i_trafo_se():
    im = i_trafo(); d = ImageDraw.Draw(im); rr(d, 44, 48, 56, 52, 0, "#222"); return im
def i_cewka_al():
    im = nowy(); d = ImageDraw.Draw(im)
    elipsa(d, 8, 22, 92, 80, fill=SZ_J, outline=SZ, w=3); elipsa(d, 32, 40, 68, 62, fill=BIAL, outline=SZ, w=2)
    for i in range(9):
        a = math.pi * (0.1 + 0.1 * i) * 2 * 0.5 * 1.8 + 0.1
        x1, y1 = 50 + 18 * math.cos(a), 51 + 10 * math.sin(a); x2, y2 = 50 + 40 * math.cos(a), 51 + 28 * math.sin(a)
        linia(d, [(x1, y1), (x2, y2)], POM, 5)
    return im
def i_cewka_pow():
    im = nowy(); d = ImageDraw.Draw(im)
    for i in range(7): linia(d, [(14 + i * 12, 70), (26 + i * 12, 30)], POM, 6)
    linia(d, [(4, 70), (14, 70)], POM, 5); linia(d, [(86, 30), (96, 30)], POM, 5); return im

IKONY = {"app": ikona_app, "nowy": i_nowy, "edytuj": i_edytuj, "stan": i_stan, "usun": i_usun, "kategoria": i_kategoria,
         "import": i_import, "eksport": i_eksport, "etykieta": i_etykieta, "szukaj": i_szukaj, "odswiez": i_odswiez,
         "projekt": i_projekt, "raport": i_raport, "sklep": i_sklep, "polacz": i_polacz, "pomoc": i_pomoc,
         "narzedzia": i_narzedzia, "generator": i_generator, "zamknij": i_zamknij,
         "rozlacz": i_rozlacz, "ustawienia": i_ustawienia, "baza": i_baza, "baza_struktura": i_baza_struktura, "info": i_info, "aparat": i_aparat,
         "wydaj": i_wydaj, "do_magazynu": i_do_magazynu, "szablon": i_szablon, "pdf": i_pdf, "pdf_odczyt": i_pdf_odczyt,
         "kamera_termo": i_kamera_termo, "kalkulator": i_kalkulator, "audio": i_audio, "wykres": i_wykres, "zniekszt": i_zniekszt,
         "dlawik": i_dlawik, "trafo": i_trafo, "trafo_pp": i_trafo_pp, "trafo_se": i_trafo_se, "cewka_al": i_cewka_al, "cewka_pow": i_cewka_pow}
ROZMIARY = {"app": (16, 24, 32, 48, 64, 128, 256)}
DOMYSLNE = (16, 24, 32)

def png(im, r):
    b = io.BytesIO(); im.resize((r, r), Image.LANCZOS).save(b, "PNG", optimize=True); return b.getvalue()

def main():
    kat = os.path.join(KAT, "assets", "ikony"); os.makedirs(kat, exist_ok=True)
    wpisy = []
    for nazwa, f in IKONY.items():
        im = f()
        for r in ROZMIARY.get(nazwa, DOMYSLNE):
            dane = png(im, r)
            open(os.path.join(kat, f"{nazwa}_{r}.png"), "wb").write(dane)
            wpisy.append((nazwa, r, dane))
    # arkusz poglądowy
    arkusz = Image.new("RGBA", (len(IKONY) * 72, 72), "#f4f4f4")
    for i, (n, f) in enumerate(IKONY.items()): arkusz.alpha_composite(f().resize((64, 64), Image.LANCZOS), (i * 72 + 4, 4))
    arkusz.save(os.path.join(kat, "przeglad.png"))
    open(os.path.join(kat, "magmass.png"), "wb").write(png(ikona_app(), 256))
    with open(os.path.join(KAT, "src", "GUI", "IkonyDane.h"), "w") as h:
        h.write("// Wygenerowane przez narzedzia/zbuduj_ikony.py: nie edytuj ręcznie.\n#ifndef IKONYDANE_H\n#define IKONYDANE_H\n\n")
        h.write("struct DaneIkony { const char* nazwa; int rozmiar; const unsigned char* dane; unsigned dlugosc; };\n\n")
        for i, (n, r, d) in enumerate(wpisy):
            h.write(f"static const unsigned char ikona_{i}[] = {{" + ",".join(str(b) for b in d) + "};\n")
        h.write("\nstatic const DaneIkony kIkony[] = {\n")
        for i, (n, r, d) in enumerate(wpisy): h.write(f'    {{"{n}", {r}, ikona_{i}, {len(d)}}},\n')
        h.write("};\n\n#endif // IKONYDANE_H\n")
    print("OK:", len(IKONY), "ikon,", len(wpisy), "plików PNG")

if __name__ == "__main__": main()
