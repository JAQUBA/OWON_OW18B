// ============================================================================
// AppUI — Tworzenie komponentów interfejsu użytkownika
// ============================================================================
#include "AppUI.h"
#include "AppState.h"
#include "OW18B_Commands.h"

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/Label/Label.h>
#include <UI/Button/Button.h>
#include <UI/Select/Select.h>
#include <UI/TextArea/TextArea.h>
#include <UI/ProgressBar/ProgressBar.h>
#include <UI/Chart/Chart.h>
#include <UI/ValueDisplay/ValueDisplay.h>
#include <IO/BLE/BLE.h>
#include <Util/DataLogger.h>
#include <Util/StringUtils.h>

#include <cstdio>

// Pomocnicze funkcje UI
static void styleBtn(SimpleWindow* win, Button* btn,
                     COLORREF bg, COLORREF text, COLORREF hover) {
    btn->setBackColor(bg);
    btn->setTextColor(text);
    btn->setHoverColor(hover);
    win->add(btn);
}

static Label* addSectionHeader(SimpleWindow* win, int x, int y, int w,
                                const wchar_t* text) {
    Label* lbl = new Label(x, y, w, 16, text);
    win->add(lbl);
    lbl->setFont(L"Segoe UI", 10, true);
    lbl->setTextColor(RGB(120, 135, 155));
    return lbl;
}

void createUI(SimpleWindow* win) {
    // Kolor tła okna — ciemny
    win->setBackgroundColor(RGB(30, 30, 38));

    // Paleta kolorów
    const COLORREF colBtnBg     = RGB(50, 52, 62);     // tło przycisków
    const COLORREF colBtnText   = RGB(200, 210, 225);   // tekst przycisków
    const COLORREF colBtnHover  = RGB(65, 68, 80);      // hover przycisków
    const COLORREF colAccent    = RGB(40, 130, 200);     // akcent (connect)
    const COLORREF colAccentH   = RGB(55, 150, 220);     // hover akcentu
    const COLORREF colCtrl      = RGB(45, 90, 90);       // sterowanie
    const COLORREF colCtrlH     = RGB(55, 110, 110);     // hover sterowania
    const COLORREF colCtrlText  = RGB(150, 240, 220);    // tekst sterowania
    const COLORREF colRecRed    = RGB(160, 40, 40);      // nagrywanie
    const COLORREF colRecRedH   = RGB(190, 50, 50);
    const COLORREF colDim       = RGB(42, 44, 52);       // przyciski secondary
    const COLORREF colDimH      = RGB(55, 58, 68);
    const COLORREF colDimText   = RGB(160, 165, 175);

    int y = 6;
    int m = 10;
    int w  = 780;

    // --- Status + progress ---
    lblStatus = new Label(m, y, 480, 18,
        L"Status: Oczekuje na inicjalizację BLE...");
    win->add(lblStatus);
    lblStatus->setFont(L"Segoe UI", 11, false);
    lblStatus->setTextColor(RGB(160, 170, 180));

    progressScan = new ProgressBar(m + 490, y, w - 490, 16);
    win->add(progressScan);
    progressScan->setColor(colAccent);
    progressScan->setBackColor(RGB(40, 42, 50));
    y += 22;

    // --- Połączenie BLE ---
    auto* lblDevice = new Label(m, y + 3, 80, 20, L"Urządzenie:");
    win->add(lblDevice);
    lblDevice->setFont(L"Segoe UI", 11, false);
    lblDevice->setTextColor(RGB(160, 170, 180));

    selDevices = new Select(m + 85, y, 260, 24, "(skanuj najpierw)", nullptr);
    selDevices->link(&deviceNames);
    win->add(selDevices);

    styleBtn(win, new Button(m + 355, y, 100, 26, "Skanuj BLE",
        [](Button*) { doScanBLE(); }), colBtnBg, colBtnText, colBtnHover);

    styleBtn(win, new Button(m + 465, y, 90, 26, "Połącz",
        [](Button*) { doConnectBLE(); }), colAccent, RGB(240, 245, 255), colAccentH);

    styleBtn(win, new Button(m + 565, y, 90, 26, "Rozłącz",
        [](Button*) { doDisconnectBLE(); }), colBtnBg, colBtnText, colBtnHover);

    y += 32;

    // --- Wyświetlacz wartości (LCD) ---
    valueDisplay = new ValueDisplay(m, y, w, 130);
    win->add(valueDisplay);

    ValueDisplay::DisplayConfig cfg;
    cfg.backgroundColor = RGB(10, 10, 25);
    cfg.textColor       = RGB(0, 230, 50);
    cfg.holdTextColor   = RGB(255, 50, 50);
    cfg.deltaTextColor  = RGB(50, 100, 255);
    cfg.precision       = 4;
    cfg.fontName        = L"Consolas";
    cfg.valueFontRatio  = 0.55;
    cfg.unitFontRatio   = 0.22;
    cfg.statusFontRatio = 0.14;
    valueDisplay->setConfig(cfg);
    valueDisplay->setMode(L"---");
    y += 132;

    // --- Tryb i statystyki ---
    lblMode = new Label(m, y, 350, 18, L"Tryb: ---");
    win->add(lblMode);
    lblMode->setFont(L"Segoe UI", 10, false);
    lblMode->setTextColor(RGB(180, 190, 200));

    lblMinMax = new Label(m + 360, y, w - 360, 18,
                          L"MIN: ---  |  MAX: ---  |  AVG: ---");
    win->add(lblMinMax);
    lblMinMax->setFont(L"Segoe UI", 10, false);
    lblMinMax->setTextColor(RGB(130, 200, 180));
    y += 22;

    // --- Sterowanie + Akcje ---
    int bx = m, bw = 78, bh = 26, gap = 4;

    styleBtn(win, new Button(bx, y, bw, bh, "SELECT",
        [](Button*) { sendCommand(OW18BCmd::SELECT()); logMsg(L"→ SELECT"); },
        [](Button*) { sendCommand(OW18BCmd::SELECT_LONG()); logMsg(L"→ SELECT (długie)"); }),
        colCtrl, colCtrlText, colCtrlH);
    bx += bw + gap;

    styleBtn(win, new Button(bx, y, bw, bh, "HOLD",
        [](Button*) { sendCommand(OW18BCmd::HOLD()); logMsg(L"→ HOLD"); },
        [](Button*) { sendCommand(OW18BCmd::HOLD_LONG()); logMsg(L"→ HOLD (długie)"); }),
        colCtrl, colCtrlText, colCtrlH);
    bx += bw + gap;

    styleBtn(win, new Button(bx, y, bw, bh, "RANGE",
        [](Button*) { sendCommand(OW18BCmd::RANGE()); logMsg(L"→ RANGE"); },
        [](Button*) { sendCommand(OW18BCmd::RANGE_LONG()); logMsg(L"→ RANGE (długie)"); }),
        colCtrl, colCtrlText, colCtrlH);
    bx += bw + gap;

    styleBtn(win, new Button(bx, y, bw, bh, "Hz/DUTY",
        [](Button*) { sendCommand(OW18BCmd::HZ_DUTY()); logMsg(L"→ Hz/DUTY"); },
        [](Button*) { sendCommand(OW18BCmd::HZ_DUTY_LONG()); logMsg(L"→ Hz/DUTY (długie)"); }),
        colCtrl, colCtrlText, colCtrlH);
    bx += bw + gap + 16;

    styleBtn(win, new Button(bx, y, 80, bh, "Reset stat.", [](Button*) {
        resetStats(); logMsg(L"Statystyki zresetowane");
    }), colDim, colDimText, colDimH);
    bx += 84 + gap;

    styleBtn(win, new Button(bx, y, 90, bh, "\xE2\x97\x8F Nagrywaj",
        [](Button*) { doRecordStart(); }),
        colRecRed, RGB(255, 200, 200), colRecRedH);
    bx += 94 + gap;

    styleBtn(win, new Button(bx, y, 80, bh, "\xE2\x96\xA0 Stop",
        [](Button*) { doRecordStop(); }),
        colDim, colDimText, colDimH);
    bx += 84 + gap;

    lblRecordStatus = new Label(bx, y + 3, w - bx + m, 20, L"");
    win->add(lblRecordStatus);
    lblRecordStatus->setFont(L"Segoe UI", 10, false);
    lblRecordStatus->setTextColor(RGB(255, 80, 80));
    y += 30;

    // --- Wykres ---
    addSectionHeader(win, m, y, 200, L"WYKRES");
    styleBtn(win, new Button(m + 700, y - 1, 70, 16, "Wyczyść", [](Button*) {
        if (chart) chart->clear(); logMsg(L"Wykres wyczyszczony");
    }), colDim, colDimText, colDimH);
    y += 18;

    chart = new Chart(m, y, w, 220, "");
    win->add(chart);
    chart->setTimeWindow(60);
    chart->setAutoScale(true);
    chart->setRefreshRate(100);
    chart->setColors(RGB(45, 45, 55), RGB(120, 125, 140), RGB(0, 200, 255));
    y += 224;
}
