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
                logMsg(L"Okno OBS zamknięte");
            } else {
                HWND parent = window ? window->getHandle() : NULL;
                if (overlayWindow->open(parent))
                    logMsg(L"Okno OBS otwarte");
                else
                    logMsg(L"⚠ Nie udało się otworzyć okna OBS");
            }
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
        m.addSeparator();
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
