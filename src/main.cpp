// ============================================================================
// OWON OW18B — Aplikacja miernika Bluetooth
// Bazuje na: https://github.com/JAQUBA/JQB_WindowsLib
// ============================================================================

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <Util/HotkeyManager.h>
#include <Util/ConfigManager.h>

#include "AppState.h"
#include "AppUI.h"
#include "BLEHandler.h"
#include "MenuHandler.h"
#include "OW18B_Commands.h"
#include "MeterOverlay.h"

#include <UI/TrayIcon/TrayIcon.h>
#include <UI/LogWindow/LogWindow.h>
#include <commctrl.h>

// ============================================================================
// Subclass proc — przechwycenie minimalizacji do zasobnika
// ============================================================================
static LRESULT CALLBACK TraySubclassProc(HWND hwnd, UINT msg, WPARAM wParam,
                                          LPARAM lParam, UINT_PTR, DWORD_PTR) {
    // Przekaż komunikaty zasobnika
    if (trayIcon && trayIcon->processMessage(msg, wParam, lParam))
        return 0;

    // Minimalizacja → zasobnik
    if (msg == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_MINIMIZE && minimizeToTray) {
        ShowWindow(hwnd, SW_HIDE);
        if (trayIcon) trayIcon->show();
        return 0;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// ============================================================================
// setup() — Inicjalizacja aplikacji
// ============================================================================
void setup() {
    // Wczytaj ustawienia z pliku INI
    loadSettings();

    // --- Okno główne (z ikoną) ---
    window = new SimpleWindow(800, 570, "OWON OW18B — Multimetr BLE", 101);
    window->init();

    // --- Ikona zasobnika systemowego ---
    trayIcon = new TrayIcon();
    trayIcon->create(window->getHandle(), 101, L"OWON OW18B — Multimetr BLE");
    trayIcon->onRestore([]() {
        ShowWindow(window->getHandle(), SW_SHOW);
        ShowWindow(window->getHandle(), SW_RESTORE);
        SetForegroundWindow(window->getHandle());
        trayIcon->hide();
    });
    SetWindowSubclass(window->getHandle(), TraySubclassProc, 1, 0);

    // --- Pasek menu ---
    createAppMenu(window);

    // --- Zapis ustawień przy zamknięciu ---
    window->onClose([]() {
        // Zapamiętaj stan overlay
        if (overlayWindow && overlayWindow->isOpen()) {
            overlayAutoOpen = true;
            overlayWindow->close();  // savePosition() wewnątrz close()
        } else {
            overlayAutoOpen = false;
        }

        // Zapamiętaj stan minimalizacji do tray
        startMinimized = !IsWindowVisible(window->getHandle());

        if (trayIcon) trayIcon->remove();
        if (logWindow) logWindow->close();
        saveSettings();
    });

    // --- Komponenty UI ---
    createUI(window);

    // --- Callbacki i inicjalizacja BLE ---
    setupBLE();

    // --- Auto-łączenie z ostatnim urządzeniem ---
    doAutoReconnect();

    // --- Auto-otwarcie okna overlay ---
    if (overlayAutoOpen || autoStartTray) {
        if (!overlayWindow) overlayWindow = new MeterOverlay();
        overlayWindow->open(window->getHandle());
        logMsg(L"Overlay otwarty automatycznie.");
    }

    // --- Start zminimalizowany do tray ---
    if ((startMinimized && minimizeToTray) || autoStartTray) {
        ShowWindow(window->getHandle(), SW_HIDE);
        if (trayIcon) trayIcon->show();
    }

    // --- Skróty klawiaturowe ---
    hotkeyMgr = new HotkeyManager(config);

    hotkeyMgr->addHotkey("shortcut_select", "SELECT",
                          "Ctrl+Alt+1", []() { sendCommand(OW18BCmd::SELECT()); });
    hotkeyMgr->addHotkey("shortcut_hold", "HOLD",
                          "Ctrl+Alt+2", []() { sendCommand(OW18BCmd::HOLD()); });
    hotkeyMgr->addHotkey("shortcut_range", "RANGE",
                          "Ctrl+Alt+3", []() { sendCommand(OW18BCmd::RANGE()); });
    hotkeyMgr->addHotkey("shortcut_hzduty", "Hz/DUTY",
                          "Ctrl+Alt+4", []() { sendCommand(OW18BCmd::HZ_DUTY()); });
    hotkeyMgr->addHotkey("shortcut_select_long", "SELECT (długie)",
                          "Ctrl+Shift+Alt+1", []() { sendCommand(OW18BCmd::SELECT_LONG()); });
    hotkeyMgr->addHotkey("shortcut_hold_long", "HOLD (długie)",
                          "Ctrl+Shift+Alt+2", []() { sendCommand(OW18BCmd::HOLD_LONG()); });
    hotkeyMgr->addHotkey("shortcut_range_long", "RANGE (długie)",
                          "Ctrl+Shift+Alt+3", []() { sendCommand(OW18BCmd::RANGE_LONG()); });
    hotkeyMgr->addHotkey("shortcut_hzduty_long", "Hz/DUTY (długie)",
                          "Ctrl+Shift+Alt+4", []() { sendCommand(OW18BCmd::HZ_DUTY_LONG()); });

    hotkeyMgr->loadFromConfig();
    hotkeyMgr->installHook();
    logMsg(L"Skróty klawiaturowe aktywne.");
}

// ============================================================================
// loop() — Pętla główna (zdarzenia BLE obsługiwane przez callbacki)
// ============================================================================
void loop() {
}
