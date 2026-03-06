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

// ============================================================================
// setup() — Inicjalizacja aplikacji
// ============================================================================
void setup() {
    // Wczytaj ustawienia z pliku INI
    loadSettings();

    // --- Okno główne (z ikoną) ---
    window = new SimpleWindow(800, 790, "OWON OW18B — Multimetr BLE", 101);
    window->init();

    // --- Pasek menu ---
    createAppMenu(window);

    // --- Zapis ustawień przy zamknięciu ---
    window->onClose([]() {
        saveSettings();
    });

    // --- Komponenty UI ---
    createUI(window);

    // --- Callbacki i inicjalizacja BLE ---
    setupBLE();

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
