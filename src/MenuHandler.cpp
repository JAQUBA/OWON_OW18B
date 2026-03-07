// ============================================================================
// MenuHandler — Budowanie paska menu z użyciem MenuBar (JQB_WindowsLib)
// ============================================================================
#include "MenuHandler.h"
#include "AppState.h"
#include "OW18B_Commands.h"
#include "MeterOverlay.h"

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/MenuBar/MenuBar.h>
#include <Util/HotkeyManager.h>

// ============================================================================
// Tworzenie i podpięcie paska menu
// ============================================================================
void createAppMenu(SimpleWindow* win) {
    MenuBar* menuBar = new MenuBar();

    // --- Plik ---
    menuBar->addMenu(L"Plik", [](PopupMenu& m) {
        m.addItem(L"Nagrywaj CSV",         []() { doRecordStart(); });
        m.addItem(L"Zatrzymaj nagrywanie", []() { doRecordStop(); });
        m.addSeparator();
        m.addItem(L"Zamknij", []() { if (window) window->close(); });
    });

    // --- Połączenie ---
    menuBar->addMenu(L"Połączenie", [](PopupMenu& m) {
        m.addItem(L"Skanuj BLE", []() { doScanBLE(); });
        m.addItem(L"Połącz",     []() { doConnectBLE(); });
        m.addItem(L"Rozłącz",    []() { doDisconnectBLE(); });
    });

    // --- Sterowanie ---
    menuBar->addMenu(L"Sterowanie", [](PopupMenu& m) {
        m.addItem(L"SELECT",  []() { sendCommand(OW18BCmd::SELECT());  logMsg(L"→ SELECT"); });
        m.addItem(L"HOLD",    []() { sendCommand(OW18BCmd::HOLD());    logMsg(L"→ HOLD"); });
        m.addItem(L"RANGE",   []() { sendCommand(OW18BCmd::RANGE());   logMsg(L"→ RANGE"); });
        m.addItem(L"Hz/DUTY", []() { sendCommand(OW18BCmd::HZ_DUTY()); logMsg(L"→ Hz/DUTY"); });
        m.addSeparator();
        m.addItem(L"SELECT (długie)",  []() { sendCommand(OW18BCmd::SELECT_LONG());  logMsg(L"→ SELECT (długie)"); });
        m.addItem(L"HOLD (długie)",    []() { sendCommand(OW18BCmd::HOLD_LONG());    logMsg(L"→ HOLD (długie)"); });
        m.addItem(L"RANGE (długie)",   []() { sendCommand(OW18BCmd::RANGE_LONG());   logMsg(L"→ RANGE (długie)"); });
        m.addItem(L"Hz/DUTY (długie)", []() { sendCommand(OW18BCmd::HZ_DUTY_LONG()); logMsg(L"→ Hz/DUTY (długie)"); });
    });

    // --- Widok ---
    menuBar->addMenu(L"Widok", [](PopupMenu& m) {
        m.addItem(L"Okno OBS (overlay)", []() {
            if (!overlayWindow) {
                overlayWindow = new MeterOverlay();
            }
            if (overlayWindow->isOpen()) {
                overlayWindow->close();
                overlayAutoOpen = false;
                saveSettings();
                logMsg(L"Okno OBS zamkni\u0119te");
            } else {
                HWND parent = window ? window->getHandle() : NULL;
                if (overlayWindow->open(parent)) {
                    overlayAutoOpen = true;
                    saveSettings();
                    logMsg(L"Okno OBS otwarte");
                } else {
                    logMsg(L"\u26a0 Nie uda\u0142o si\u0119 otworzy\u0107 okna OBS");
                }
            }
        });
        m.addItem(L"Okno log\u00f3w", []() {
            doToggleLogWindow();
        });
    });

    // --- Ustawienia ---
    menuBar->addMenu(L"Ustawienia", [](PopupMenu& m) {
        m.addItem(L"Skróty klawiaturowe...", []() {
            if (hotkeyMgr && window)
                hotkeyMgr->showSettingsDialog(window->getHandle());
        });
        m.addSeparator();
        m.addCheckItem(L"Wykres aktywny", chartEnabled, [](bool checked) {
            chartEnabled = checked;
            saveSettings();
            logMsg(checked ? L"Wykres włączony" : L"Wykres wyłączony");
        });
        m.addCheckItem(L"Loguj dane RAW", logRawData, [](bool checked) {
            logRawData = checked;
            saveSettings();
            logMsg(checked ? L"Logowanie RAW włączone" : L"Logowanie RAW wyłączone");
        });
        m.addCheckItem(L"Auto-łączenie z ostatnim urządzeniem", autoReconnect, [](bool checked) {
            autoReconnect = checked;
            saveSettings();
            logMsg(checked ? L"Auto-łączenie włączone" : L"Auto-łączenie wyłączone");
        });
        m.addCheckItem(L"Minimalizuj do zasobnika", minimizeToTray, [](bool checked) {
            minimizeToTray = checked;
            saveSettings();
            logMsg(checked ? L"Minimalizacja do zasobnika włączona" : L"Minimalizacja do zasobnika wyłączona");
        });        m.addCheckItem(L"Autostart: zasobnik + overlay", autoStartTray, [](bool checked) {
            autoStartTray = checked;
            if (checked) {
                minimizeToTray = true;
                overlayAutoOpen = true;
            }
            saveSettings();
            logMsg(checked ? L"Autostart z zasobnikiem i overlay w\u0142\u0105czony" : L"Autostart z zasobnikiem i overlay wy\u0142\u0105czony");
        });        m.addSeparator();
        m.addItem(L"Resetuj statystyki", []() {
            resetStats();
            logMsg(L"Statystyki zresetowane");
        });
    });

    // --- Pomoc ---
    menuBar->addMenu(L"Pomoc", [](PopupMenu& m) {
        m.addItem(L"O programie...", []() {
            if (window) {
                MessageBoxW(window->getHandle(),
                    L"OWON OW18B — Multimetr BLE\n"
                    L"Wersja 1.0\n\n"
                    L"Komunikacja z multimetrem OWON OW18B\n"
                    L"przez Bluetooth Low Energy.\n\n"
                    L"Biblioteka: JQB_WindowsLib",
                    L"O programie", MB_OK | MB_ICONINFORMATION);
            }
        });
    });

    menuBar->attachTo(win);
}
