#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Rysuje ilustracje do pomocy (rdzeń EI, okno i warstwy, układy uzwojeń, rozkład amperozwojów, Mesa Boogie DR)
i wbudowuje je w program: pomoc/obrazy/*.png oraz src/Pomoc/ObrazyPomocy.cpp (dane PNG jako tablice bajtów).

Rysunki są własne (wykonane tym skryptem): wartości liczbowe pochodzą z modelu programu, schemat połączeń DR odtwarza
opis ze strony guitaramplifiers.prv.pl (sam rysunek ze strony nie jest kopiowany).
Użycie: python3 narzedzia/zbuduj_obrazy_pomocy.py   (z katalogu projektu; wymaga matplotlib)
"""
import io
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, FancyArrowPatch, Arc, Polygon

ROOT = Path(__file__).resolve().parent.parent
KAT = ROOT / "pomoc" / "obrazy"
CPP = ROOT / "src" / "Pomoc" / "ObrazyPomocy.cpp"

CZERW, CZERW_J = "#c0392b", "#f1b5ad"
NIEB, NIEB_J = "#1f5fa8", "#b9d1ee"
ZIEL, FIOL = "#1d8a5b", "#7b3fa0"
SZARY, SZARY_J = "#555555", "#d9d9d9"
plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 10, "axes.linewidth": 0.8})


def zapisz(fig, nazwa):
    KAT.mkdir(parents=True, exist_ok=True)
    fig.savefig(KAT / nazwa, dpi=96, facecolor="white", bbox_inches="tight", pad_inches=0.15)
    plt.close(fig)


def strzalka(ax, p, q, tekst=None, kolor="black", dx=0, dy=0, ha="center", va="center", rozmiar=9):
    ax.add_patch(FancyArrowPatch(p, q, arrowstyle="<->", mutation_scale=9, color=kolor, lw=0.9, shrinkA=0, shrinkB=0))
    if tekst:
        ax.text((p[0] + q[0]) / 2 + dx, (p[1] + q[1]) / 2 + dy, tekst, ha=ha, va=va, fontsize=rozmiar, color=kolor,
                bbox=dict(fc="white", ec="none", pad=0.6))


# ------------------------------------------------------------------ 1. rdzeń EI
def rdzen_ei():
    fig, ax = plt.subplots(figsize=(8.2, 4.6))
    ax.set_aspect("equal"); ax.axis("off")
    a = 20.0                       # kolumna środkowa (typowa blacha: boczne a/2, okno a/2 x 3a/2, jarzmo a/2)
    d, c, h, j = a / 2, a / 2, 1.5 * a, a / 2
    A = a + 2 * c + 2 * d
    B = h + 2 * j
    ax.add_patch(Rectangle((0, 0), A, B, fc=SZARY_J, ec=SZARY, lw=1.2))
    for x0 in (d, d + c + a):
        ax.add_patch(Rectangle((x0, j), c, h, fc="white", ec=SZARY, lw=1.2))          # okna
    ax.plot([0, A], [B - j + 0.0, B - j], color="#888", lw=0.8, ls=":")                # granica E / I
    ax.text(A + 13, B - j / 2, "płytka I", va="center", fontsize=9, color=SZARY)
    ax.text(A + 13, j + h / 2, "płytka E\n(trzy kolumny)", va="center", fontsize=9, color=SZARY)
    # uzwojenia w oknach (przekrój)
    for x0, jasne in ((d, True), (d + c + a, False)):
        ax.add_patch(Rectangle((x0 + (c * 0.0), j + 2), c, h - 4, fc="none", ec="none"))
    xl = d + c                      # lewe okno przylega do środkowej kolumny po prawej stronie
    ax.add_patch(Rectangle((d + 1.2, j + 1.5), c - 1.2, h - 3, fc=NIEB_J, ec=NIEB, lw=0.9))
    ax.add_patch(Rectangle((d + c + a, j + 1.5), c - 1.2, h - 3, fc=NIEB_J, ec=NIEB, lw=0.9))
    ax.text(d + c / 2 + 0.6, j + h / 2, "uzwojenia", rotation=90, ha="center", va="center", fontsize=8, color=NIEB)
    ax.text(d + c + a + c / 2 - 0.6, j + h / 2, "uzwojenia", rotation=90, ha="center", va="center", fontsize=8, color=NIEB)
    # strumień
    for xs, kier in ((d + c + a / 2, 1),):
        ax.add_patch(FancyArrowPatch((xs, j + 3), (xs, B - j - 3), arrowstyle="-|>", mutation_scale=12, color=CZERW, lw=1.6))
    xm = d + c + a / 2
    ax.add_patch(Rectangle((d / 2, j / 2), xm - d / 2, B - j, fc="none", ec=CZERW, lw=1.1, ls="--"))
    ax.add_patch(Rectangle((xm, j / 2), A - d / 2 - xm, B - j, fc="none", ec=CZERW, lw=1.1, ls="--"))
    ax.text(xm, -0.1 + j / 2 - 0.0, "", fontsize=1)
    ax.text(A / 2, B + 22, "przerywane: droga strumienia lż\n(dwie pętle: kolumna środkowa + boczna)", ha="center", va="center",
            fontsize=8.5, color=CZERW)
    # wymiary
    strzalka(ax, (0, -4), (A, -4), "A (szerokość blachy)", dy=-3.0)
    strzalka(ax, (-5, 0), (-5, B), "B", dx=-2.6)
    strzalka(ax, (d + c, B + 4), (d + c + a, B + 4), "a", dy=3)
    strzalka(ax, (0, B + 4), (d, B + 4), "d", dy=3)
    strzalka(ax, (d, B + 11), (d + c, B + 11), "c", dy=3)
    strzalka(ax, (A + 5, j), (A + 5, j + h), "h", dx=3)
    # przekrój rdzenia (widok z boku)
    ox = A + 42
    ax.add_patch(Rectangle((ox, 14), 30, 40, fc=SZARY_J, ec=SZARY, hatch="////", lw=1.0))
    ax.text(ox + 15, 62, "przekrój S = a · b", ha="center", fontsize=9.5)
    strzalka(ax, (ox, 7), (ox + 30, 7), "a", dy=-3)
    ax.text(ox + 40, 34, "b = grubość\npakietu", va="center", fontsize=9)
    ax.text(ox + 15, 2, "(przekrój środkowej kolumny, przez którą\nprzechodzi cały strumień)", ha="center", va="top", fontsize=8, color=SZARY)
    ax.set_xlim(-14, ox + 62); ax.set_ylim(-14, B + 30)
    zapisz(fig, "rdzen_ei.png")


# ------------------------------------------------------------------ 2. okno i warstwy
def okno_warstwy():
    fig, ax = plt.subplots(figsize=(8.2, 4.4))
    ax.set_aspect("equal"); ax.axis("off")
    W, H = 70.0, 46.0              # okno (c x h), jednostki umowne
    ax.add_patch(Rectangle((-14, -6), 14, H + 12, fc=SZARY_J, ec=SZARY))           # kolumna środkowa
    ax.text(-7, H / 2, "kolumna\nśrodkowa", rotation=90, ha="center", va="center", fontsize=9, color=SZARY)
    ax.add_patch(Rectangle((0, 0), W, H, fc="white", ec=SZARY, lw=1.4))              # okno
    sc, mg = 3.0, 3.5              # ścianka karkasu, margines warstwy
    ax.add_patch(Rectangle((0, 0), 2.2, H, fc="#e8dcc2", ec="#8b7a4f"))              # policzek od rdzenia
    ax.add_patch(Rectangle((0, 0), W, sc, fc="#e8dcc2", ec="#8b7a4f"))
    ax.add_patch(Rectangle((0, H - sc), W, sc, fc="#e8dcc2", ec="#8b7a4f"))
    ax.text(W - 2, sc / 2, "policzek karkasu (ścianka)", ha="right", va="center", fontsize=8, color="#6b5b30")
    x = 2.2
    grub, prz = 5.0, 0.9
    kolory = [(CZERW_J, CZERW)] * 3 + [(NIEB_J, NIEB)] * 2
    nazwy = ["A", "A", "A", "W", "W"]
    for i, (jas, ciem) in enumerate(kolory):
        ax.add_patch(Rectangle((x, sc + mg), grub, H - 2 * sc - 2 * mg, fc=jas, ec=ciem, lw=0.9))
        for yy in range(int((H - 2 * sc - 2 * mg) / 2.4)):
            ax.add_patch(plt.Circle((x + grub / 2, sc + mg + 1.2 + yy * 2.4), 0.95, fc=ciem, ec="none", alpha=0.75))
        ax.text(x + grub / 2, H + 2.2, nazwy[i] + str(i + 1 if i < 3 else i - 2), ha="center", fontsize=8.5, color=ciem)
        x += grub
        if i < len(kolory) - 1:
            ax.add_patch(Rectangle((x, sc + mg - 1), prz, H - 2 * sc - 2 * mg + 2, fc="#f6f1c4", ec="none"))
            x += prz
    x_war = 2.2 + 0.5 * grub
    # opisy
    xa = W + 4
    ax.plot([x, xa + 1], [sc + mg, sc + mg], color="#999", lw=0.6, ls=":")
    ax.plot([x, xa + 1], [H - sc - mg, H - sc - mg], color="#999", lw=0.6, ls=":")
    strzalka(ax, (xa, sc + mg), (xa, H - sc - mg), None)
    ax.text(xa + 2.5, H / 2, "długość warstwy\n= h − 2·ścianka\n   − 2·margines", ha="left", va="center", fontsize=8.5)
    strzalka(ax, (W + 3, 0), (W + 3, 0.001), None)
    ax.annotate("margines warstwy", (7.5, sc + mg / 2), (44, 12), fontsize=8.5, ha="center",
                arrowprops=dict(arrowstyle="-", color="#666", lw=0.8))
    ax.annotate("przekładka między warstwami", (2.2 + grub + prz / 2, H - sc - mg + 1), (44, H + 8), fontsize=8.5, ha="center",
                arrowprops=dict(arrowstyle="-", color="#666", lw=0.8))
    strzalka(ax, (2.2, -4.5), (x, -4.5), "grubość uzwojeń = Σ warstwy · (d + przekładka)", dy=-3.6, rozmiar=8.5)
    strzalka(ax, (0, H + 13.5), (W, H + 13.5), "szerokość okna c", dy=2.7, rozmiar=8.5)
    ax.text(48, H / 2 + 2, "wolne miejsce\nw oknie", ha="center", va="center", fontsize=9, color="#999")
    ax.text(-16, -16, "Zwojów w warstwie = długość warstwy / (średnica drutu z lakierem);\nwarstw = zwoje sekcji / zwojów w warstwie (w górę).",
            fontsize=8.3, color="#333", ha="left")
    ax.set_xlim(-16, W + 36); ax.set_ylim(-19, H + 18)
    zapisz(fig, "okno_warstwy.png")


# ------------------------------------------------------------------ 3. układy uzwojeń
def uklady():
    wiersze = [
        ("P-S (nieprzeplatane)", [("A", 60), ("W", 28)], []),
        ("P/2-S-P/2", [("A", 30), ("W", 28), ("A", 30)], []),
        ("gitarowy: A 40% W 50% A 20% W 50% A 40%", [("A", 24), ("W", 14), ("A", 12), ("W", 14), ("A", 24)], []),
        ("HiFi: karkas dwukomorowy (A W | A W)", [("A", 30), ("W", 14), ("|", 0), ("A", 30), ("W", 14)], []),
        ("HiFi: dwukomorowy przeplatany (A W A | A W A)", [("A", 15), ("W", 14), ("A", 15), ("|", 0), ("A", 15), ("W", 14), ("A", 15)], []),
        ("Mesa Boogie DR (10 warstw)", [("A", 12), ("W", 8), ("W", 4), ("A", 20), ("W", 8), ("W", 8), ("A", 20), ("W", 8), ("W", 4), ("A", 12)], []),
    ]
    fig, ax = plt.subplots(figsize=(8.2, 5.0))
    ax.axis("off")
    n = len(wiersze)
    for i, (tytul, bloki, _) in enumerate(wiersze):
        y = (n - 1 - i) * 1.3
        ax.text(0, y + 0.82, tytul, fontsize=9.5, ha="left", va="center", fontweight="bold")
        ax.add_patch(Rectangle((-5, y - 0.2), 5, 0.78, fc=SZARY_J, ec=SZARY))
        ax.text(-2.5, y + 0.19, "rdzeń", ha="center", va="center", fontsize=6.5, color=SZARY, rotation=0)
        x = 0.0
        nA = nW = 0
        for typ, szer in bloki:
            if typ == "|":
                ax.add_patch(Rectangle((x, y - 0.2), 2.2, 0.78, fc="#e8dcc2", ec="#8b7a4f"))
                x += 2.2
                continue
            jas, ciem = (CZERW_J, CZERW) if typ == "A" else (NIEB_J, NIEB)
            ax.add_patch(Rectangle((x, y - 0.2), szer, 0.78, fc=jas, ec=ciem, lw=0.9))
            if typ == "A":
                nA += 1; et = f"A{nA}"
            else:
                nW += 1; et = f"W{nW}"
            ax.text(x + szer / 2, y + 0.19, et, ha="center", va="center", fontsize=8, color=ciem)
            x += szer
        ax.add_patch(Rectangle((x, y - 0.2), 1.4, 0.78, fc="#e8dcc2", ec="#8b7a4f"))
    ax.text(0, -0.55, "Od lewej: od kolumny środkowej rdzenia na zewnątrz okna.\nCzerwone: A (pierwotne, anodowe), niebieskie: W (wtórne),\n"
            "beżowe: karkas (policzek, przegroda |). Szerokości są poglądowe.", fontsize=8.3, color="#333", va="top")
    ax.set_xlim(-6, 105); ax.set_ylim(-1.5, n * 1.3 + 0.1)
    zapisz(fig, "uklady_sekcji.png")


# ------------------------------------------------------------------ 4. rozkład amperozwojów (powód przeplatania)
def mmf():
    fig, axes = plt.subplots(2, 1, figsize=(8.2, 5.0), sharex=True)
    ukl = [("P-S: pierwotne obok wtórnego", [("A", 60), ("W", 28)], "Lr ≈ 12,5 mH"),
           ("P/2-S-P/2: wtórne między połówkami pierwotnego", [("A", 30), ("W", 28), ("A", 30)], "Lr ≈ 4,1 mH")]
    for ax, (tytul, bloki, lr) in zip(axes, ukl):
        calk = sum(s for t, s in bloki)
        x, f = 0.0, 0.0
        xs, ys = [0.0], [0.0]
        wA = sum(s for t, s in bloki if t == "A")
        wW = sum(s for t, s in bloki if t == "W")
        for typ, s in bloki:
            for _ in range(1):
                if typ == "A":
                    f += s / wA                 # amperozwoje rosną liniowo wzdłuż grubości uzwojenia
                else:
                    f -= s / wW
                x += s
                xs.append(x); ys.append(f)
            # rysujemy bloki
        x = 0.0
        for typ, s in bloki:
            jas, ciem = (CZERW_J, CZERW) if typ == "A" else (NIEB_J, NIEB)
            ax.add_patch(Rectangle((x, -0.78), s, 0.1, fc=jas, ec=ciem, lw=0.9, clip_on=False))
            x += s
        ax.plot(xs, ys, color="#222", lw=1.8)
        ax.fill_between(xs, ys, 0, color="#f7e19a", alpha=0.6)
        ax.axhline(0, color="#888", lw=0.7)
        ax.set_ylim(-0.8, 1.15)
        ax.set_yticks([0, 0.5, 1.0]); ax.set_yticklabels(["0", "½", "1"])
        ax.set_ylabel("amperozwoje\n(względnie)", fontsize=8.5)
        ax.set_title(tytul + "   →   " + lr + " (przykład z pomocy)", fontsize=9.5, loc="left")
        for sp in ("top", "right"):
            ax.spines[sp].set_visible(False)
    axes[1].set_xlabel("położenie w oknie od kolumny środkowej na zewnątrz (czerwone A, niebieskie W)", fontsize=8.5)
    axes[1].set_xticks([])
    fig.text(0.5, 0.005, "Indukcyjność rozproszenia jest proporcjonalna do całki z kwadratu amperozwojów po oknie: mniejsze „górki” = mniejsze Lr.",
             ha="center", fontsize=8.3, color="#333")
    fig.tight_layout(rect=(0, 0.03, 1, 1))
    zapisz(fig, "amperozwoje.png")


# ------------------------------------------------------------------ 5. Mesa Boogie DR
def cewka(ax, x, y0, y1, kolor, n=4, r=0.16, strona=1):
    """Symbol cewki (pętle) wzdłuż pionowego odcinka x, od y0 do y1."""
    h = (y1 - y0) / n
    for k in range(n):
        ax.add_patch(Arc((x, y0 + h * (k + 0.5)), 2 * r, h, theta1=-90, theta2=90 if strona > 0 else 90, color=kolor, lw=1.6)
                     if strona > 0 else Arc((x, y0 + h * (k + 0.5)), 2 * r, h, theta1=90, theta2=270, color=kolor, lw=1.6))


def dr():
    fig = plt.figure(figsize=(8.4, 6.2))
    gs = fig.add_gridspec(2, 1, height_ratios=[1.05, 1.5], hspace=0.12)
    # --- góra: przekrój okna
    ax = fig.add_subplot(gs[0]); ax.axis("off")
    war = [(1, "A", 318), (2, "W", 78), (3, "W", 33), (4, "A", 530), (5, "W", 78), (6, "W", 78), (7, "A", 530), (8, "W", 78), (9, "W", 33), (10, "A", 318)]
    grupa = {2: NIEB, 5: NIEB, 8: NIEB, 3: ZIEL, 9: ZIEL, 6: FIOL}
    jas = {NIEB: NIEB_J, ZIEL: "#b7e4cf", FIOL: "#dcc6ec"}
    szer = {318: 10, 530: 16, 78: 7, 33: 4}
    ax.add_patch(Rectangle((-8, -0.25), 8, 1.5, fc=SZARY_J, ec=SZARY)); ax.text(-4, 0.5, "rdzeń", ha="center", va="center", fontsize=8, color=SZARY)
    x = 0.0
    for nr, typ, z in war:
        s = szer[z]
        if typ == "A":
            fc, ec = CZERW_J, CZERW
        else:
            ec = grupa[nr]; fc = jas[ec]
        ax.add_patch(Rectangle((x, -0.1), s, 1.2, fc=fc, ec=ec, lw=1.1))
        ax.text(x + s / 2, 0.5, f"{nr}", ha="center", va="center", fontsize=10, color=ec, fontweight="bold")
        ax.text(x + s / 2, -0.5, f"{z}", ha="center", va="center", fontsize=8, color=ec)
        x += s + 0.8
    ax.text(0, 1.55, "Przekrój okna (od rdzenia na zewnątrz): numery warstw, pod nimi liczba zwojów", fontsize=9.5, ha="left", fontweight="bold")
    ax.text(x - 0.8, -1.15, "A: pierwotne (czerwone);  niebieskie 2, 5, 8: odczep 4 Ω;  zielone 3, 9: dopinka 8 Ω;  fioletowa 6: dopinka 16 Ω",
            fontsize=8.3, ha="right", color="#333")
    ax.set_xlim(-9, x + 1); ax.set_ylim(-1.5, 1.9)
    # --- dół: schemat elektryczny
    bx = fig.add_subplot(gs[1]); bx.axis("off"); bx.set_xlim(0, 20); bx.set_ylim(-1.4, 10.4)
    bx.text(0.2, 10.0, "Połączenia elektryczne (wszystkie zwoje nawinięte w tym samym kierunku)", fontsize=9.5, fontweight="bold", va="center")
    # pierwotne: szeregowo 1 - 7 - (+HT) - 4 - 10
    px = 2.6
    kol = [(0.8, 2.2, "1", "318 zw."), (2.2, 3.6, "7", "530 zw."), (4.4, 5.8, "4", "530 zw."), (5.8, 7.2, "10", "318 zw.")]
    bx.plot([px, px], [0.3, 0.8], color=CZERW, lw=1.6)
    bx.text(px + 0.5, 0.35, "A1 (anoda)", fontsize=8.5, color=CZERW, va="center")
    y = 0.8
    seq = [("1", 1.4, "318"), ("7", 2.2, "530")]
    bx.plot([px, px], [0.3, 1.0], color=CZERW, lw=1.6)
    ys = 1.0
    for nr, wys, z in [("1", 1.5, "318"), ("7", 2.4, "530")]:
        cewka(bx, px, ys, ys + wys, CZERW, n=5, strona=1)
        bx.text(px - 0.5, ys + wys / 2, nr, ha="right", va="center", fontsize=10, color=CZERW, fontweight="bold")
        bx.text(px + 0.45, ys + wys / 2 + (0.45 if ys > 3 and wys > 2 and nr == "7" else 0), z + " zw.", ha="left", va="center", fontsize=8, color=CZERW)
        ys += wys
    bx.plot([px, px], [ys, ys + 0.3], color=CZERW, lw=1.6)
    bx.plot([px, px + 2.0], [ys + 0.15, ys + 0.15], color=CZERW, lw=1.6)
    bx.plot(px + 2.0, ys + 0.15, "o", color=CZERW, ms=5)
    bx.text(px + 2.3, ys + 0.15, "+HT (B+, odczep środkowy)", fontsize=8.5, va="center", color=CZERW)
    ys += 0.3
    for nr, wys, z in [("4", 2.4, "530"), ("10", 1.5, "318")]:
        cewka(bx, px, ys, ys + wys, CZERW, n=5, strona=1)
        bx.text(px - 0.5, ys + wys / 2, nr, ha="right", va="center", fontsize=10, color=CZERW, fontweight="bold")
        bx.text(px + 0.45, ys + wys / 2 + (0.45 if ys > 3 and wys > 2 and nr == "7" else 0), z + " zw.", ha="left", va="center", fontsize=8, color=CZERW)
        ys += wys
    bx.plot([px, px], [ys, ys + 0.4], color=CZERW, lw=1.6)
    bx.text(px + 0.5, ys + 0.4, "A2 (anoda)", fontsize=8.5, color=CZERW, va="center")
    # wtórne: dolna sekcja 2 || 5 || 8 (4 Ω), górne 3 || 9 (8 Ω), 6 (16 Ω)
    x0 = 9.0
    xs = [x0, x0 + 1.9, x0 + 3.8]
    yd0, yd1 = 1.2, 3.8       # 0 -> 4R
    yg0, yg1 = 3.8, 6.4       # 4R -> 8R / 16R
    bx.plot([xs[0] - 0.7, xs[2] + 0.7], [yd0 - 0.4, yd0 - 0.4], color="#333", lw=1.4)
    for xx, nr in zip(xs, (2, 5, 8)):
        bx.plot([xx, xx], [yd0 - 0.4, yd0], color=NIEB, lw=1.6)
        cewka(bx, xx, yd0, yd1, NIEB, n=4, strona=-1)
        bx.plot([xx, xx], [yd1, yd1 + 0.4], color=NIEB, lw=1.6)
        bx.text(xx - 0.45, (yd0 + yd1) / 2, str(nr), ha="right", va="center", fontsize=10, color=NIEB, fontweight="bold")
    bx.plot([xs[0], xs[2]], [yd1 + 0.4, yd1 + 0.4], color=NIEB, lw=1.6)
    bx.plot(xs[1], yd0 - 0.4, "v", color="#333", ms=0)
    bx.text(xs[1], yd0 - 0.75, "0 (masa)", ha="center", va="top", fontsize=8.5)
    bx.plot(xs[2] + 0.7, yd0 - 0.4, "o", color="#333", ms=5)
    # wyprowadzenie 4R
    n4 = yd1 + 0.4
    bx.plot([xs[2], xs[2] + 2.6], [n4, n4], color=NIEB, lw=1.6); bx.plot(xs[2] + 2.6, n4, "o", color=NIEB, ms=5)
    bx.text(xs[2] + 2.9, n4, "4 Ω   (78 zw.: 2∥5∥8)", fontsize=8.8, va="center", color=NIEB, fontweight="bold")
    # 3 || 9 (od węzła 4R w górę do 8R)
    for xx, nr in zip((xs[0], xs[1]), (3, 9)):
        bx.plot([xx, xx], [n4, n4 + 0.35], color=ZIEL, lw=1.6)
        cewka(bx, xx, n4 + 0.35, n4 + 2.2, ZIEL, n=3, strona=-1)
        bx.plot([xx, xx], [n4 + 2.2, n4 + 2.55], color=ZIEL, lw=1.6)
        bx.text(xx - 0.45, n4 + 1.3, str(nr), ha="right", va="center", fontsize=10, color=ZIEL, fontweight="bold")
    bx.plot([xs[0], xs[1]], [n4 + 2.55, n4 + 2.55], color=ZIEL, lw=1.6)
    bx.plot([xs[1], xs[1] + 0.6], [n4 + 2.55, n4 + 2.55], color=ZIEL, lw=1.6); bx.plot(xs[1] + 0.6, n4 + 2.55, "o", color=ZIEL, ms=5)
    bx.text(xs[1] + 0.9, n4 + 2.55, "8 Ω   (78 + 33 = 111 zw.: 2∥5∥8 i 3∥9)", fontsize=8.8, va="center", color=ZIEL, fontweight="bold", zorder=5,
            bbox=dict(fc="white", ec="none", pad=0.5))
    # 6 (od 4R w górę do 16R)
    bx.plot([xs[2], xs[2]], [n4, n4 + 0.35], color=FIOL, lw=1.6)
    cewka(bx, xs[2], n4 + 0.35, n4 + 2.2, FIOL, n=3, strona=-1)
    bx.plot([xs[2], xs[2]], [n4 + 2.2, n4 + 3.35], color=FIOL, lw=1.6)
    bx.plot([xs[2], xs[2] + 0.9], [n4 + 3.35, n4 + 3.35], color=FIOL, lw=1.6); bx.plot(xs[2] + 0.9, n4 + 3.35, "o", color=FIOL, ms=5)
    bx.text(xs[2] - 0.45, n4 + 1.3, "6", ha="right", va="center", fontsize=10, color=FIOL, fontweight="bold")
    bx.text(xs[2] + 1.2, n4 + 3.35, "16 Ω   (78 + 78 = 156 zw.: 2∥5∥8 i 6)", fontsize=8.8, va="center", color=FIOL, fontweight="bold")
    bx.text(10.5, 0.35 - 0.0, "", fontsize=8)
    bx.text(19.8, -0.9, "Uwaga: warstwy 3 i 9 oraz 6 przewodzą tylko przy swoim odczepie; przy pozostałych leżą w oknie bez prądu.",
            fontsize=8.0, ha="right", color="#333")
    zapisz(fig, "mesa_boogie_dr.png")


def zbuduj_cpp():
    pliki = sorted(KAT.glob("*.png"))
    out = io.StringIO()
    out.write('// Plik wygenerowany przez narzedzia/zbuduj_obrazy_pomocy.py: nie edytuj ręcznie.\n')
    out.write('#include "ObrazyPomocy.h"\n\nnamespace {\n')
    for p in pliki:
        dane = p.read_bytes()
        out.write(f"const unsigned char kDane_{p.stem}[] = {{\n")
        for i in range(0, len(dane), 24):
            out.write("    " + ",".join(str(b) for b in dane[i:i + 24]) + ",\n")
        out.write("};\n")
    out.write("} // namespace\n\nconst std::vector<ObrazPomocy>& ObrazyPomocy() {\n    static const std::vector<ObrazPomocy> v = {\n")
    for p in pliki:
        out.write(f'        {{"{p.name}", kDane_{p.stem}, sizeof(kDane_{p.stem})}},\n')
    out.write("    };\n    return v;\n}\n")
    CPP.write_text(out.getvalue(), encoding="utf-8")


if __name__ == "__main__":
    rdzen_ei(); okno_warstwy(); uklady(); mmf(); dr()
    zbuduj_cpp()
    print("OK:", ", ".join(p.name for p in sorted(KAT.glob("*.png"))))
