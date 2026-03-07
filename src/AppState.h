// ============================================================================
// AppState — Globalny stan aplikacji, zmienne współdzielone, funkcje pomocnicze
// ============================================================================
#ifndef APP_STATE_H
#define APP_STATE_H

#include <string>
#include <vector>
#include <cstdint>

// Forward declarations — klasy z JQB_WindowsLib
class SimpleWindow;
class Label;
class Select;
class Button;
class ProgressBar;
class Chart;
class ValueDisplay;
class CheckBox;
class BLE;
class HotkeyManager;
class ConfigManager;
class DataLogger;
class MeterOverlay;
class TrayIcon;
class LogWindow;
struct Statistics;

// ============================================================================
// Komponenty UI (globalne — dostępne z wielu modułów)
// ============================================================================
extern SimpleWindow*   window;
extern Label*          lblStatus;
extern Select*         selDevices;
extern ProgressBar*    progressScan;
extern ValueDisplay*   valueDisplay;
extern Label*          lblMode;
extern Label*          lblMinMax;
extern Chart*          chart;
extern Label*          lblRecordStatus;
extern MeterOverlay*   overlayWindow;
extern TrayIcon*       trayIcon;
extern LogWindow*      logWindow;

// ============================================================================
// Obiekty aplikacji
// ============================================================================
extern BLE             ble;
extern Statistics      stats;
extern DataLogger      dataLogger;
extern ConfigManager   config;
extern HotkeyManager*  hotkeyMgr;

// ============================================================================
// Stan aplikacji
// ============================================================================
extern bool            isConnected;
extern bool            chartEnabled;
extern bool            logRawData;
extern bool            autoReconnect;
extern bool            minimizeToTray;
extern bool            overlayAutoOpen;
extern bool            startMinimized;
extern bool            autoStartTray;
extern std::wstring    lastUnit;
extern std::wstring    lastDeviceAddress;
extern std::wstring    lastDeviceName;
extern uint32_t        measurementCount;
extern std::vector<std::string> deviceNames;

// ============================================================================
// UUID BLE OWON OW18B
// ============================================================================
extern const std::wstring OWON_SERVICE_UUID;
extern const std::wstring OWON_NOTIFY_CHARACTERISTIC_UUID;
extern const std::wstring OWON_WRITE_CHARACTERISTIC_UUID;

// ============================================================================
// Funkcje pomocnicze
// ============================================================================
void logMsg(const wchar_t* msg);
void logMsg(const std::wstring& msg);
void sendCommand(const std::vector<uint8_t>& cmd);
void updateStatsLabel();
void resetStats();
void updateRecordLabel();
void loadSettings();
void saveSettings();
std::wstring formatHexDump(const std::vector<uint8_t>& data);

// Akcje współdzielone (używane przez przyciski UI i menu)
void doScanBLE();
void doConnectBLE();
void doDisconnectBLE();
void doRecordStart();
void doRecordStop();
void doAutoReconnect();
void doToggleLogWindow();

#endif // APP_STATE_H
