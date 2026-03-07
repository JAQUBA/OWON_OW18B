// ============================================================================
// AppState — Definicje zmiennych globalnych i funkcje pomocnicze
// ============================================================================
#include "AppState.h"
#include "MeterOverlay.h"

#include <Core.h>
#include <IO/BLE/BLE.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/TrayIcon/TrayIcon.h>
#include <UI/LogWindow/LogWindow.h>
#include <UI/Label/Label.h>
#include <UI/Select/Select.h>
#include <UI/ProgressBar/ProgressBar.h>
#include <UI/ValueDisplay/ValueDisplay.h>
#include <UI/Chart/Chart.h>
#include <Util/Statistics.h>
#include <Util/DataLogger.h>
#include <Util/HotkeyManager.h>
#include <Util/ConfigManager.h>
#include <Util/StringUtils.h>

#include <cstdio>
#include <cmath>

// ============================================================================
// Definicje zmiennych globalnych
// ============================================================================

// Komponenty UI
SimpleWindow*   window          = nullptr;
Label*          lblStatus       = nullptr;
Select*         selDevices      = nullptr;
ProgressBar*    progressScan    = nullptr;
ValueDisplay*   valueDisplay    = nullptr;
Label*          lblMode         = nullptr;
Label*          lblMinMax       = nullptr;
Chart*          chart           = nullptr;
Label*          lblRecordStatus = nullptr;
MeterOverlay*   overlayWindow    = nullptr;
TrayIcon*       trayIcon         = nullptr;
LogWindow*      logWindow        = nullptr;

// Obiekty aplikacji
BLE             ble;
Statistics      stats;
DataLogger      dataLogger("owon");
ConfigManager   config("owon_meter.ini");
HotkeyManager*  hotkeyMgr       = nullptr;

// Stan
bool            isConnected      = false;
bool            chartEnabled     = true;
bool            logRawData       = false;
bool            autoReconnect    = true;
bool            minimizeToTray   = true;
bool            overlayAutoOpen  = false;
bool            startMinimized   = false;
bool            autoStartTray    = false;
std::wstring    lastUnit         = L"";
std::wstring    lastDeviceAddress = L"";
std::wstring    lastDeviceName    = L"";
uint32_t        measurementCount = 0;
std::vector<std::string> deviceNames;

// UUID BLE
const std::wstring OWON_SERVICE_UUID               = L"0000fff0-0000-1000-8000-00805f9b34fb";
const std::wstring OWON_NOTIFY_CHARACTERISTIC_UUID  = L"0000fff4-0000-1000-8000-00805f9b34fb";
const std::wstring OWON_WRITE_CHARACTERISTIC_UUID   = L"0000fff3-0000-1000-8000-00805f9b34fb";

// ============================================================================
// Funkcje pomocnicze — logowanie
// ============================================================================

void logMsg(const wchar_t* msg) {
    if (logWindow) {
        logWindow->appendMessage(msg);
    }
}

void logMsg(const std::wstring& msg) {
    logMsg(msg.c_str());
}

// ============================================================================
// Funkcje pomocnicze — komendy BLE
// ============================================================================

void sendCommand(const std::vector<uint8_t>& cmd) {
    wchar_t buf[128];
    if (!isConnected) {
        logMsg(L"⚠ Nie połączono — komenda odrzucona");
        return;
    }
    bool ok = ble.write(cmd);
    if (cmd.size() == 2) {
        swprintf(buf, 128, L"CMD [%02X %02X] → %ls", cmd[0], cmd[1], ok ? L"OK" : L"FAIL");
    } else {
        swprintf(buf, 128, L"CMD (%d B) → %ls", (int)cmd.size(), ok ? L"OK" : L"FAIL");
    }
    logMsg(buf);
}

// ============================================================================
// Funkcje pomocnicze — statystyki
// ============================================================================

void updateStatsLabel() {
    if (!stats.hasData || !lblMinMax) return;
    wchar_t buf[200];
    swprintf(buf, 200, L"MIN: %.4f  |  MAX: %.4f  |  AVG: %.4f  |  PEAK: %.4f  %ls",
             stats.min, stats.max, stats.getAvg(), stats.peak, lastUnit.c_str());
    lblMinMax->setText(buf);
}

void resetStats() {
    stats.reset();
    measurementCount = 0;
    if (lblMinMax) lblMinMax->setText(L"MIN: ---  |  MAX: ---  |  AVG: ---");
}

// ============================================================================
// Funkcje pomocnicze — nagrywanie CSV
// ============================================================================

void updateRecordLabel() {
    if (!lblRecordStatus) return;
    if (dataLogger.isRecording()) {
        wchar_t buf[128];
        swprintf(buf, 128, L"● REC: %d próbek, %ds",
                 dataLogger.getSampleCount(), dataLogger.getElapsedSeconds());
        lblRecordStatus->setText(buf);
    } else {
        lblRecordStatus->setText(L"");
    }
}

// ============================================================================
// Funkcje pomocnicze — formatowanie
// ============================================================================

std::wstring formatHexDump(const std::vector<uint8_t>& data) {
    std::wstring result = L"[";
    result += std::to_wstring(data.size());
    result += L" B] ";
    for (size_t i = 0; i < data.size(); i++) {
        wchar_t hex[8];
        swprintf(hex, 8, L"%02X ", data[i]);
        result += hex;
    }
    return result;
}

// ============================================================================
// Ustawienia — zapis/odczyt z pliku INI
// ============================================================================

void loadSettings() {
    chartEnabled    = config.getValue("chart_enabled", "1") == "1";
    logRawData      = config.getValue("log_raw_data", "0") == "1";
    autoReconnect   = config.getValue("auto_reconnect", "1") == "1";
    minimizeToTray  = config.getValue("minimize_to_tray", "1") == "1";
    overlayAutoOpen = config.getValue("overlay_auto_open", "0") == "1";
    startMinimized  = config.getValue("start_minimized", "0") == "1";
    autoStartTray   = config.getValue("auto_start_tray", "0") == "1";
    lastDeviceAddress = StringUtils::utf8ToWide(config.getValue("last_device_address", ""));
    lastDeviceName    = StringUtils::utf8ToWide(config.getValue("last_device_name", ""));
}

void saveSettings() {
    config.setValue("chart_enabled",      chartEnabled    ? "1" : "0");
    config.setValue("log_raw_data",       logRawData      ? "1" : "0");
    config.setValue("auto_reconnect",     autoReconnect   ? "1" : "0");
    config.setValue("minimize_to_tray",    minimizeToTray  ? "1" : "0");
    config.setValue("overlay_auto_open",   overlayAutoOpen ? "1" : "0");
    config.setValue("start_minimized",     startMinimized  ? "1" : "0");
    config.setValue("auto_start_tray",      autoStartTray   ? "1" : "0");
    config.setValue("last_device_address", StringUtils::wideToUtf8(lastDeviceAddress));
    config.setValue("last_device_name",    StringUtils::wideToUtf8(lastDeviceName));
}

// ============================================================================
// Akcje współdzielone (przyciski UI + menu)
// ============================================================================

void doScanBLE() {
    lblStatus->setText(L"Status: Skanowanie BLE...");
    lblStatus->setTextColor(RGB(220, 200, 50));
    progressScan->setMarquee(true);
    deviceNames.clear();
    selDevices->updateItems();
    logMsg(L"--- Rozpoczynam skanowanie BLE (10s) ---");
    ble.startScan(10);
}

void doConnectBLE() {
    auto& devices = ble.getDiscoveredDevices();
    int idx = (int)SendMessageW(selDevices->getHandle(), CB_GETCURSEL, 0, 0);
    if (idx >= 0 && idx < (int)devices.size()) {
        logMsg(L"Łączenie z urządzeniem...");
        lblStatus->setText(L"Status: Łączenie...");
        lblStatus->setTextColor(RGB(220, 200, 50));

        // Zapisz dane urządzenia do auto-reconnect
        lastDeviceAddress = devices[idx].address;
        lastDeviceName = devices[idx].name;

        ble.connect(devices[idx].address);
    } else {
        logMsg(L"⚠ Wybierz urządzenie z listy!");
    }
}

void doDisconnectBLE() {
    ble.disconnect();
    logMsg(L"Rozłączanie...");
}

void doRecordStart() {
    if (!dataLogger.isRecording()) {
        if (dataLogger.startRecording(
                {"Value", "Unit", "Mode", "OL", "Auto", "Hold", "Delta"})) {
            logMsg(L"● Rozpoczęto nagrywanie: " +
                   StringUtils::utf8ToWide(dataLogger.getFilename()));
        } else {
            logMsg(L"⚠ Nie można otworzyć pliku CSV!");
        }
    }
}

void doRecordStop() {
    if (dataLogger.isRecording()) {
        wchar_t buf[128];
        swprintf(buf, 128, L"■ Zakończono nagrywanie: %d próbek",
                 dataLogger.getSampleCount());
        dataLogger.stopRecording();
        logMsg(buf);
        updateRecordLabel();
    }
}

void doAutoReconnect() {
    if (!autoReconnect || lastDeviceAddress.empty()) return;

    std::wstring msg = L"Auto-łączenie z: " + lastDeviceName;
    logMsg(msg);
    lblStatus->setText(L"Status: Auto-łączenie...");
    lblStatus->setTextColor(RGB(220, 200, 50));
    ble.connect(lastDeviceAddress);
}
void doToggleLogWindow() {
    if (!logWindow) {
        logWindow = new LogWindow();
        logWindow->setTitle(L"OW18B \u2014 Logi");
        logWindow->enablePersistence(config, "logwin");
    }
    if (logWindow->isOpen()) {
        logWindow->close();
        logMsg(L"Okno log\u00f3w zamkni\u0119te");
    } else {
        HWND parent = window ? window->getHandle() : NULL;
        if (logWindow->open(parent))
            logMsg(L"Okno log\u00f3w otwarte");
    }
}