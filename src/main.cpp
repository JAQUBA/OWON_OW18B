// ============================================================================
// OWON OW18B — Aplikacja miernika Bluetooth
// Bazuje na: https://github.com/JAQUBA/JQB_WindowsLib
// ============================================================================

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/Label/Label.h>
#include <UI/Button/Button.h>
#include <UI/Select/Select.h>
#include <UI/TextArea/TextArea.h>
#include <UI/ProgressBar/ProgressBar.h>
#include <UI/Chart/Chart.h>
#include <UI/ValueDisplay/ValueDisplay.h>
#include <UI/CheckBox/CheckBox.h>
#include <IO/BLE/BLE.h>
#include <Util/StringUtils.h>

#include "OW18BParser.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

// ============================================================================
// Globalne komponenty UI
// ============================================================================
SimpleWindow*   window;

// --- Sekcja połączenia ---
Label*          lblTitle;
Label*          lblStatus;
Select*         selDevices;
Button*         btnScan;
Button*         btnConnect;
Button*         btnDisconnect;
ProgressBar*    progressScan;

// --- Sekcja wyświetlania wartości ---
ValueDisplay*   valueDisplay;
Label*          lblMode;
Label*          lblMinMax;

// --- Sekcja wykresu ---
Chart*          chart;
CheckBox*       chkChartEnabled;
Button*         btnClearChart;

// --- Log danych ---
TextArea*       txtLog;
CheckBox*       chkLogRaw;

// ============================================================================
// Globalne obiekty
// ============================================================================
BLE             ble;

// Listy urządzeń
std::vector<std::string> deviceNames;

// Stan aplikacji
bool isConnected   = false;
bool chartEnabled  = true;
bool logRawData    = false;

// Min/Max tracking
double minValue    = 1e18;
double maxValue    = -1e18;
bool   hasMinMax   = false;
std::wstring lastUnit = L"";

// Licznik pomiarów
uint32_t measurementCount = 0;

// ============================================================================
// Funkcje pomocnicze
// ============================================================================
void logMsg(const wchar_t* msg) {
    if (txtLog) {
        txtLog->append(msg);
        txtLog->append(L"\r\n");
    }
}

void logMsg(const std::wstring& msg) {
    logMsg(msg.c_str());
}

void updateMinMaxLabel() {
    if (!hasMinMax || !lblMinMax) return;
    wchar_t buf[128];
    swprintf(buf, 128, L"MIN: %.4f  |  MAX: %.4f  %s", minValue, maxValue, lastUnit.c_str());
    lblMinMax->setText(buf);
}

void resetMinMax() {
    minValue = 1e18;
    maxValue = -1e18;
    hasMinMax = false;
    measurementCount = 0;
    if (lblMinMax) lblMinMax->setText(L"MIN: ---  |  MAX: ---");
}

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
// Obsługa odebranych danych BLE
// ============================================================================
void handleBLEData(const std::vector<uint8_t>& data) {
    // Loguj surowe dane jeśli włączony
    if (logRawData) {
        std::wstring hexStr = L"RX: " + formatHexDump(data);
        logMsg(hexStr);
    }

    // Parsuj dane
    OW18B::Measurement m = OW18B::Parser::parse(data);

    if (!m.isValid) {
        logMsg(L"⚠ Nieprawidłowe dane");
        return;
    }

    measurementCount++;

    // --- Aktualizuj ValueDisplay ---
    if (valueDisplay) {
        if (m.isOL) {
            // Over Limit
            valueDisplay->updateValue(INFINITY, m.prefix, m.unit);
        } else {
            valueDisplay->updateValue(m.value, m.prefix, m.unit);
        }
        valueDisplay->setMode(m.modeStr);
        valueDisplay->setAuto(m.isAuto);
        valueDisplay->setHold(m.isHold);
        valueDisplay->setDelta(m.isDelta);
    }

    // --- Aktualizuj etykietę trybu ---
    if (lblMode) {
        std::wstring modeInfo = L"Tryb: " + m.modeStr;
        if (m.isAuto)  modeInfo += L" | AUTO";
        if (m.isHold)  modeInfo += L" | HOLD";
        if (m.isDelta) modeInfo += L" | DELTA";

        wchar_t countBuf[32];
        swprintf(countBuf, 32, L"  (#%u)", measurementCount);
        modeInfo += countBuf;

        lblMode->setText(modeInfo.c_str());
    }

    // --- Min/Max ---
    if (!m.isOL && std::isfinite(m.value)) {
        if (m.value < minValue) minValue = m.value;
        if (m.value > maxValue) maxValue = m.value;
        hasMinMax = true;
        lastUnit = m.prefix + m.unit;
        updateMinMaxLabel();
    }

    // --- Wykres ---
    if (chartEnabled && chart && !m.isOL && std::isfinite(m.value)) {
        chart->addDataPoint(m.value, m.unit);
    }

    // --- Logowanie wartości ---
    if (!logRawData) {
        wchar_t valBuf[128];
        if (m.isOL) {
            swprintf(valBuf, 128, L"OL  %s  [%s]", 
                     m.unit.c_str(), m.modeStr.c_str());
        } else {
            swprintf(valBuf, 128, L"%.*f %s%s  [%s]",
                     m.precision, m.value,
                     m.prefix.c_str(), m.unit.c_str(),
                     m.modeStr.c_str());
        }
        logMsg(valBuf);
    }
}

// ============================================================================
// setup() — Inicjalizacja GUI i BLE
// ============================================================================
void setup() {
    // --- Okno główne ---
    window = new SimpleWindow(800, 680, "OWON OW18B — Multimetr BLE", 0);
    window->init();

    int y = 10;
    int margin = 10;
    int width  = 780;

    // =======================================================================
    // SEKCJA 1: Nagłówek i status połączenia
    // =======================================================================
    lblTitle = new Label(margin, y, width, 24,
        L"OWON OW18B — Multimetr Bluetooth");
    window->add(lblTitle);
    y += 30;

    lblStatus = new Label(margin, y, width, 20,
        L"Status: Oczekuje na inicjalizację BLE...");
    window->add(lblStatus);
    y += 28;

    // --- Pasek postępu skanowania ---
    progressScan = new ProgressBar(margin, y, width, 18);
    window->add(progressScan);
    y += 26;

    // =======================================================================
    // SEKCJA 2: Połączenie BLE
    // =======================================================================
    window->add(new Label(margin, y, 100, 22, L"Urządzenie:"));

    selDevices = new Select(margin + 100, y, 300, 25, "(skanuj najpierw)", nullptr);
    selDevices->link(&deviceNames);
    window->add(selDevices);

    btnScan = new Button(margin + 410, y, 110, 28, "Skanuj BLE", [](Button*) {
        lblStatus->setText(L"Status: Skanowanie BLE...");
        progressScan->setMarquee(true);
        deviceNames.clear();
        selDevices->updateItems();
        logMsg(L"--- Rozpoczynam skanowanie BLE (10s) ---");
        ble.startScan(10);
    });
    window->add(btnScan);

    btnConnect = new Button(margin + 530, y, 110, 28, "Połącz", [](Button*) {
        auto& devices = ble.getDiscoveredDevices();
        int idx = (int)SendMessage(selDevices->getHandle(), CB_GETCURSEL, 0, 0);
        if (idx >= 0 && idx < (int)devices.size()) {
            logMsg(L"Łączenie z urządzeniem...");
            lblStatus->setText(L"Status: Łączenie...");
            ble.connect(devices[idx].address);
        } else {
            logMsg(L"⚠ Wybierz urządzenie z listy!");
        }
    });
    window->add(btnConnect);

    btnDisconnect = new Button(margin + 650, y, 120, 28, "Rozłącz", [](Button*) {
        ble.disconnect();
        logMsg(L"Rozłączanie...");
    });
    window->add(btnDisconnect);

    y += 40;

    // =======================================================================
    // SEKCJA 3: Wyświetlacz wartości (LCD style)
    // =======================================================================
    valueDisplay = new ValueDisplay(margin, y, width, 130);
    window->add(valueDisplay);

    // Konfiguracja wyglądu
    ValueDisplay::DisplayConfig cfg;
    cfg.backgroundColor = RGB(10, 10, 25);
    cfg.textColor       = RGB(0, 230, 50);    // Zielony — styl LCD
    cfg.holdTextColor   = RGB(255, 50, 50);    // Czerwony HOLD
    cfg.deltaTextColor  = RGB(50, 100, 255);   // Niebieski DELTA
    cfg.precision       = 4;
    cfg.fontName        = L"Consolas";
    cfg.valueFontRatio  = 0.55;
    cfg.unitFontRatio   = 0.22;
    cfg.statusFontRatio = 0.14;
    valueDisplay->setConfig(cfg);
    valueDisplay->setMode(L"---");

    y += 138;

    // --- Tryb i Min/Max ---
    lblMode = new Label(margin, y, width / 2 - 5, 20, L"Tryb: ---");
    window->add(lblMode);

    lblMinMax = new Label(margin + width / 2 + 5, y, width / 2 - 5, 20,
                          L"MIN: ---  |  MAX: ---");
    window->add(lblMinMax);
    y += 26;

    // --- Przycisk resetowania Min/Max ---
    window->add(new Button(margin, y, 120, 26, "Reset Min/Max", [](Button*) {
        resetMinMax();
        logMsg(L"Min/Max zresetowane");
    }));
    y += 34;

    // =======================================================================
    // SEKCJA 4: Wykres czasu rzeczywistego
    // =======================================================================
    chart = new Chart(margin, y, width, 180, "Wykres pomiarów");
    window->add(chart);
    chart->setTimeWindow(60);           // 60 sekund widocznych
    chart->setAutoScale(true);
    chart->setRefreshRate(100);         // 10 FPS
    chart->setColors(
        RGB(40, 40, 50),               // Siatka
        RGB(140, 140, 160),            // Osie
        RGB(0, 200, 255)               // Dane
    );
    y += 188;

    // --- Opcje wykresu ---
    chkChartEnabled = new CheckBox(margin, y, 140, 22, "Wykres aktywny", true,
        [](CheckBox* cb, bool checked) { chartEnabled = checked; });
    window->add(chkChartEnabled);

    btnClearChart = new Button(margin + 160, y, 120, 24, "Wyczyść wykres", [](Button*) {
        if (chart) chart->clear();
        logMsg(L"Wykres wyczyszczony");
    });
    window->add(btnClearChart);

    chkLogRaw = new CheckBox(margin + 300, y, 180, 22, "Loguj dane RAW (hex)", false,
        [](CheckBox* cb, bool checked) { logRawData = checked; });
    window->add(chkLogRaw);

    y += 30;

    // =======================================================================
    // SEKCJA 5: Log tekstowy
    // =======================================================================
    window->add(new Label(margin, y, 100, 18, L"Log:"));

    window->add(new Button(margin + 680, y, 90, 20, "Wyczyść log", [](Button*) {
        if (txtLog) txtLog->setText(L"");
        logMsg(L"Log wyczyszczony");
    }));
    y += 22;

    txtLog = new TextArea(margin, y, width, 100);
    window->add(txtLog);

    // =======================================================================
    // Konfiguracja callbacków BLE
    // =======================================================================
    ble.onDeviceDiscovered([](const BLEDevice& dev) {
        std::wstring msg = L"Znaleziono: " + dev.name;
        logMsg(msg);

        std::string nameUtf8 = StringUtils::wideToUtf8(dev.name);
        deviceNames.push_back(nameUtf8);
        selDevices->updateItems();
    });

    ble.onScanComplete([]() {
        progressScan->setMarquee(false);
        progressScan->setProgress(100);

        auto& devices = ble.getDiscoveredDevices();
        wchar_t buf[128];
        swprintf(buf, 128, L"Status: Skanowanie zakończone — znaleziono %d urz.",
                 (int)devices.size());
        lblStatus->setText(buf);
        logMsg(L"--- Skanowanie zakończone ---");
    });

    ble.onConnect([]() {
        isConnected = true;
        lblStatus->setText(L"Status: Połączony z OWON OW18B");
        logMsg(L"✔ Połączono z urządzeniem BLE!");
        resetMinMax();
    });

    ble.onDisconnect([]() {
        isConnected = false;
        lblStatus->setText(L"Status: Rozłączony");
        logMsg(L"✘ Rozłączono");
        if (valueDisplay) {
            valueDisplay->setMode(L"---");
        }
        if (lblMode) {
            lblMode->setText(L"Tryb: ---");
        }
    });

    ble.onReceive([](const std::vector<uint8_t>& data) {
        handleBLEData(data);
    });

    ble.onError([](const std::wstring& errMsg) {
        std::wstring msg = L"BŁĄD: " + errMsg;
        logMsg(msg);
        lblStatus->setText(L"Status: Błąd BLE");
        progressScan->setMarquee(false);
    });

    // =======================================================================
    // Inicjalizacja BLE
    // =======================================================================
    if (ble.init()) {
        lblStatus->setText(L"Status: BLE gotowe — kliknij 'Skanuj BLE'");
        logMsg(L"Adapter BLE zainicjalizowany.");
        logMsg(L"Włącz miernik OWON OW18B i kliknij 'Skanuj BLE'.");
    } else {
        lblStatus->setText(L"Status: BŁĄD — Bluetooth niedostępny!");
        logMsg(L"⚠ Bluetooth niedostępny w systemie!");
        logMsg(L"Upewnij się, że adapter BT jest włączony.");
    }
}

// ============================================================================
// loop() — Pętla główna (pętla komunikatów obsługuje zdarzenia automatycznie)
// ============================================================================
void loop() {
    // Dane z BLE przychodzą przez callback onReceive()
    // Nie trzeba tutaj nic robić — wszystko obsługują callbacki
}
