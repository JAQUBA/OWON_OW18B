// ============================================================================
// MeterOverlay — Podklasa OverlayWindow dla multimetru OWON OW18B
// ============================================================================
#include "MeterOverlay.h"
#include "AppState.h"

#include <Util/ConfigManager.h>

#include <cstdio>
#include <cmath>

// ============================================================================
// Konstruktor
// ============================================================================
MeterOverlay::MeterOverlay()
    : OverlayWindow(L"OW18B_Overlay", L"OW18B \u2014 OBS Overlay")
{
    enablePersistence(config, "overlay");
}

// ============================================================================
// Aktualizacja danych
// ============================================================================
void MeterOverlay::updateValue(double value, const std::wstring& prefix,
                               const std::wstring& unit, int precision) {
    m_value     = value;
    m_prefix    = prefix;
    m_unit      = unit;
    m_precision = precision;
    invalidate();
}

void MeterOverlay::setMode(const std::wstring& mode) {
    m_mode = mode;
    invalidate();
}

void MeterOverlay::setFlags(bool isAuto, bool isHold, bool isDelta, bool isOL) {
    m_isAuto  = isAuto;
    m_isHold  = isHold;
    m_isDelta = isDelta;
    m_isOL    = isOL;
    invalidate();
}

// ============================================================================
// Rysowanie (podklasa — tylko treść, double-buffer obsługiwany przez bazę)
// ============================================================================
void MeterOverlay::onPaint(HDC memDC, const RECT& clientRect) {
    int cw = clientRect.right;
    int ch = clientRect.bottom;

    // --- Wartość pomiaru ---
    int valueFontSize = ch * 55 / 100;
    if (valueFontSize < 16) valueFontSize = 16;

    HFONT valueFont = CreateFontW(
        -valueFontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Consolas");
    HFONT oldFont = (HFONT)SelectObject(memDC, valueFont);

    // Kolor tekstu — zmiana przy HOLD/DELTA
    COLORREF valColor = m_textColor;
    if (m_isHold)  valColor = RGB(255, 80, 80);
    if (m_isDelta) valColor = RGB(80, 140, 255);
    SetTextColor(memDC, valColor);

    // Formatuj wartość
    wchar_t valueBuf[64];
    if (m_isOL) {
        swprintf(valueBuf, 64, L"OL");
    } else if (std::isinf(m_value)) {
        swprintf(valueBuf, 64, L"OL");
    } else {
        swprintf(valueBuf, 64, L"%.*f", m_precision, m_value);
    }

    // Wartość + jednostka
    std::wstring displayStr = valueBuf;
    displayStr += L" ";
    displayStr += m_prefix;
    displayStr += m_unit;

    RECT valRect = { 8, 2, cw - 8, ch * 70 / 100 };
    DrawTextW(memDC, displayStr.c_str(), -1, &valRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(memDC, oldFont);
    DeleteObject(valueFont);

    // --- Tryb + flagi (dolna linia) ---
    int modeFontSize = ch * 22 / 100;
    if (modeFontSize < 10) modeFontSize = 10;

    HFONT modeFont = CreateFontW(
        -modeFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT prevFont = (HFONT)SelectObject(memDC, modeFont);
    SetTextColor(memDC, m_modeColor);

    std::wstring modeLine = m_mode;
    if (m_isAuto)  modeLine += L"  AUTO";
    if (m_isHold)  modeLine += L"  HOLD";
    if (m_isDelta) modeLine += L"  \x0394"; // Δ

    RECT modeRect = { 10, ch * 68 / 100, cw - 10, ch - 4 };
    DrawTextW(memDC, modeLine.c_str(), -1, &modeRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(memDC, prevFont);
    DeleteObject(modeFont);
}
