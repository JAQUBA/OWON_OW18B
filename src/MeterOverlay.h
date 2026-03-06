// ============================================================================
// MeterOverlay — Okno nakładki OBS wyświetlające pomiary multimetru
// ============================================================================
// Podklasa OverlayWindow z biblioteki JQB_WindowsLib.
// Renderuje wartość pomiaru (Consolas) i tryb/flagi (Segoe UI).
// ============================================================================
#ifndef METER_OVERLAY_H
#define METER_OVERLAY_H

#include <UI/OverlayWindow/OverlayWindow.h>
#include <string>

class MeterOverlay : public OverlayWindow {
public:
    MeterOverlay();

    // Aktualizacja danych z parsera
    void updateValue(double value, const std::wstring& prefix,
                     const std::wstring& unit, int precision);
    void setMode(const std::wstring& mode);
    void setFlags(bool isAuto, bool isHold, bool isDelta, bool isOL);

protected:
    void onPaint(HDC memDC, const RECT& clientRect) override;

private:
    // Dane pomiarowe
    double       m_value     = 0.0;
    std::wstring m_prefix;
    std::wstring m_unit;
    std::wstring m_mode;
    int          m_precision = 2;
    bool         m_isAuto    = false;
    bool         m_isHold    = false;
    bool         m_isDelta   = false;
    bool         m_isOL      = false;

    // Kolor trybu
    COLORREF     m_modeColor = RGB(120, 140, 160);
};

#endif // METER_OVERLAY_H
