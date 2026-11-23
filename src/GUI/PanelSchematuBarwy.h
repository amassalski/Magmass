#ifndef PANELSCHEMATUBARWY_H
#define PANELSCHEMATUBARWY_H

#include "../Logika/SchematBarwy.h"

#include <wx/panel.h>

/// Rysunek układu regulacji barwy (patrz SchematBarwy); skaluje się do rozmiaru panelu, kolory z motywu systemu.
class PanelSchematuBarwy : public wxPanel {
public:
    explicit PanelSchematuBarwy(wxWindow* rodzic);
    void Ustaw(SchematBarwy::Rysunek rysunek);
    size_t LiczbaPrymitywow() const { return rysunek_.prymy.size(); }
    const SchematBarwy::Rysunek& Rysunek() const { return rysunek_; }

private:
    void OnPaint(wxPaintEvent&);
    SchematBarwy::Rysunek rysunek_;
};

#endif
