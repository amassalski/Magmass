#ifndef DIALOGAUDIOLAMPY_H
#define DIALOGAUDIOLAMPY_H

#include "../Logika/KalkulatoryAudio.h"
#include "../Logika/KalkulatoryLampowe.h"
#include "../Logika/LampyKatalog.h"
#include "../Logika/SchematyLampowe.h"

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>

#include <initializer_list>
#include <vector>

/// Wykres krzywej RIAA (oś częstotliwości logarytmiczna 20 Hz..20 kHz, oś dB); opcjonalnie druga krzywa (sieć RC)
class WykresRIAA : public wxPanel {
public:
    explicit WykresRIAA(wxWindow* parent);
    /// Krzywe: idealna i (opcjonalnie) sieć; wartości w dB dla częstotliwości `f`
    void Ustaw(const std::vector<double>& f, const std::vector<double>& idealna, const std::vector<double>& siec);
    size_t LiczbaPunktow() const { return f_.size(); }

private:
    std::vector<double> f_, idealna_, siec_;
    void OnPaint(wxPaintEvent&);
};

/// Schemat układu lampowego; elementy użyte w obliczeniach są wyróżnione kolorem
class SchematUkladu : public wxPanel {
public:
    explicit SchematUkladu(wxWindow* parent);
    void Ustaw(const SchematyLampowe::Schemat& s);
    const SchematyLampowe::Schemat& Schemat() const { return schemat_; }

private:
    SchematyLampowe::Schemat schemat_;
    void OnPaint(wxPaintEvent&);
};

/// Narzędzia audio i lampowe (wzorowane na ECCLab): ECCkalkulator (dB, nepery), krzywa RIAA i dobór sieci korekcyjnej.
/// Kolejne zakładki (układy lampowe, lampy, metody graficzne, elementy indukcyjne) dochodzą w następnych etapach.
class DialogAudioLampy : public wxDialog {
public:
    explicit DialogAudioLampy(wxWindow* parent);

    // --- dostęp dla testów
    void PokazStrone(int nr) { ksiazka_->SetSelection(nr); }
    int LiczbaStron() const { return static_cast<int>(ksiazka_->GetPageCount()); }
    void UstawNapiecia(const wxString& u1, const wxString& u2);
    wxString OpisNapiec() const { return lbNapiecia_->GetLabel(); }
    void UstawMoce(const wxString& p1, const wxString& p2);
    wxString OpisMocy() const { return lbMoce_->GetLabel(); }
    void UstawMiare(const wxString& miara, bool moc);
    wxString OpisMiary() const { return lbMiara_->GetLabel(); }
    void UstawDb(const wxString& db, bool moc);
    wxString OpisDb() const { return lbDb_->GetLabel(); }
    void UstawNepery(const wxString& np);
    wxString OpisNeperow() const { return lbNepery_->GetLabel(); }
    void UstawDbNaNepery(const wxString& db);
    wxString OpisDbNaNepery() const { return lbDbNp_->GetLabel(); }

    /// rodzaj: 0 = odtwarzanie, 1 = nagrywanie
    void UstawRIAA(int rodzaj, bool iec, const wxString& czestotliwosc);
    wxString OpisPunktuRIAA() const { return lbRiaaPunkt_->GetLabel(); }
    void UstawZakresRIAA(const wxString& od, const wxString& doo, const wxString& interwal);
    int LiczbaWierszyRIAA() const { return tabela_->GetItemCount(); }
    wxString KomorkaRIAA(int wiersz, int kolumna) const;
    void UstawSiecRIAA(const wxString& r2, const wxString& r3, int szeregR, int szeregC);
    wxString OpisSieciRIAA() const { return lbSiec_->GetLabel(); }
    size_t PunktowWykresu() const { return wykres_->LiczbaPunktow(); }

    /// Punkt pracy lampy (zakładka 3): pola w kolejności z formularza, "" zostawia puste
    void UstawPolaLampy(std::initializer_list<wxString> wartosci);
    /// Punkt pracy: parametry lampy (μ, ra, Pa max) z katalogu po symbolu; false, gdy brak połączenia albo lampy
    bool LampaDlaPunktuPracy(const wxString& symbol);
    wxString WynikPunktuPracy() const { return lbLM_->GetLabel(); }

    /// Układy lampowe: 0 = sprzężenie RC, 1 = wtórnik anodowy, 2 = wtórnik katodowy, 3 = inwerter różnicowy
    void UstawUklad(int nr);
    int LiczbaPolUkladu() const;
    void UstawPoleUkladu(int nr, const wxString& tekst);
    wxString EtykietaPolaUkladu(int nr) const;
    void UstawPolaryzacjeAutomatyczna(bool autom);
    int LiczbaWynikowUkladu() const;
    wxString WynikUkladu(int nr) const;
    /// Etykiety elementów schematu użytych w obliczeniach, rozdzielone spacją (np. "Ra Rs Cs Rk")
    wxString ElementySchematu() const;
    bool SchematMaEtykiete(const wxString& e) const { return schemat_->Schemat().maEtykiete(e.utf8_string()); }
    wxString UwagiUkladu() const { return lbUwagi_->GetLabel(); }

    /// Wpisuje dane lampy w pola Sa, Ka, Rwew (oraz Cak, Cas, Csk, jeśli układ je ma) bieżącego układu
    void ZastosujLampe(const LampyKatalog::DaneLampy& d);
    /// Dane lampy odczytane z pól bieżącego układu (nullopt, gdy Ka albo Rwew nie są poprawne)
    std::optional<LampyKatalog::DaneLampy> DaneLampyZPol() const;
    /// Bez okien dialogowych: lampa o symbolu z katalogu (wymaga połączenia z bazą); false, gdy jej nie ma
    bool LampaZKatalogu(const wxString& symbol);
    /// Zapisuje dane lampy z pól do katalogu pod podanym symbolem (nowa pozycja albo aktualizacja parametrów)
    bool ZapiszLampeDoKatalogu(const wxString& symbol);
    /// Dodaje do katalogu lampy przykładowe, których jeszcze nie ma; zwraca liczbę dodanych (-1 przy błędzie bazy)
    int DodajLampyPrzykladowe();

private:
    wxNotebook* ksiazka_ = nullptr;
    wxTextCtrl *tcU1_ = nullptr, *tcU2_ = nullptr, *tcP1_ = nullptr, *tcP2_ = nullptr, *tcMiara_ = nullptr, *tcDb_ = nullptr,
               *tcNp_ = nullptr, *tcDbNp_ = nullptr;
    wxChoice *chMiara_ = nullptr, *chDb_ = nullptr;
    wxStaticText *lbNapiecia_ = nullptr, *lbMoce_ = nullptr, *lbMiara_ = nullptr, *lbDb_ = nullptr, *lbNepery_ = nullptr,
                 *lbDbNp_ = nullptr;

    wxChoice *chRodzaj_ = nullptr, *chSzeregR_ = nullptr, *chSzeregC_ = nullptr;
    wxCheckBox* cbIec_ = nullptr;
    wxTextCtrl *tcF_ = nullptr, *tcOd_ = nullptr, *tcDo_ = nullptr, *tcInterwal_ = nullptr, *tcR2_ = nullptr, *tcR3_ = nullptr;
    wxStaticText *lbRiaaPunkt_ = nullptr, *lbSiec_ = nullptr, *lbZakres_ = nullptr;
    wxListCtrl* tabela_ = nullptr;
    WykresRIAA* wykres_ = nullptr;

    static constexpr int kMaksPol = 14, kMaksWynikow = 6;
    wxChoice* chUklad_ = nullptr;
    wxCheckBox* cbAutom_ = nullptr;
    wxStaticText* etPola_[kMaksPol] = {};
    wxTextCtrl* tcPola_[kMaksPol] = {};
    wxStaticText *etWyn_[kMaksWynikow] = {}, *lbWyn_[kMaksWynikow] = {}, *lbUwagi_ = nullptr;
    wxPanel* panelUkladu_ = nullptr;
    SchematUkladu* schemat_ = nullptr;
    std::string roleP_[kMaksPol];   ///< rola pola: "Sa", "Ka", "Rwew", "Cak", "Cas", "Csk" albo pusta
    void OnLampaZKatalogu();
    void OnLampaDlaPunktuPracy();
    std::optional<LampyKatalog::DaneLampy> WybierzLampeZKatalogu();
    void ZastosujLampeDoPunktuPracy(const LampyKatalog::DaneLampy& d);
    void OnZapiszLampe();

    std::vector<wxTextCtrl*> poleLM_;
    wxStaticText* lbLM_ = nullptr;
    wxPanel* StronaPunktuPracy();
    void PrzeliczLampe();

    wxPanel* StronaUklady();
    void UkladZmieniony();
    void PrzeliczUklad();
    wxPanel* StronaDecybeli();
    wxPanel* StronaRIAA();
    void PrzeliczDecybele();
    void PrzeliczRIAA();
    void PrzeliczTabele();
    void PrzeliczSiec();
};

#endif  // DIALOGAUDIOLAMPY_H
