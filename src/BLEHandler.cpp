// ============================================================================
// BLEHandler — Obsługa komunikacji BLE z multimetrem OWON OW18B
// ============================================================================
#include "BLEHandler.h"
#include "AppState.h"
#include "OW18BParser.h"
#include "MeterOverlay.h"

#include <Core.h>
#include <IO/BLE/BLE.h>
#include <UI/Label/Label.h>
#include <UI/Select/Select.h>
#include <UI/ProgressBar/ProgressBar.h>
#include <UI/ValueDisplay/ValueDisplay.h>
#include <UI/Chart/Chart.h>
#include <Util/Statistics.h>
#include <Util/DataLogger.h>
#include <Util/StringUtils.h>

#include <cstdio>
#include <cmath>

// ============================================================================
// Obsługa odebranych danych BLE
// ============================================================================
void handleBLEData(const std::vector<uint8_t>& data) {
    // --- Loguj surowy pakiet ---
    if (logRawData) {
        std::wstring hexStr = L"RX " + formatHexDump(data);
        logMsg(hexStr);
    }

    // Parsuj dane
    OW18B::Measurement m = OW18B::Parser::parse(data);

    if (!m.isValid) {
        logMsg(L"⚠ Nieprawidłowe dane");
        return;
    }

    // --- DEBUG: Sparsowane pola bitowe ---
    if (logRawData) {
        wchar_t dbg[256];
        swprintf(dbg, 256,
            L"  word0=0x%04X mode=%d(%ls) prefix=%ls div=%d val=%d → %.*f %ls%ls%ls%ls%ls",
            m.rawWord0,
            (int)m.mode, m.modeStr.c_str(),
            m.prefix.empty() ? L"-" : m.prefix.c_str(),
            (int)m.divisor, (int)m.rawValue,
            m.precision, m.value,
            m.prefix.c_str(), m.unit.c_str(),
            m.isAuto ? L" AUTO" : L"",
            m.isHold ? L" HOLD" : L"",
            m.isDelta ? L" DELTA" : L"");
        logMsg(dbg);
    }

    measurementCount++;

    // --- Aktualizuj ValueDisplay ---
    if (valueDisplay) {
        if (m.isOL) {
            valueDisplay->updateValue(INFINITY, m.prefix, m.unit);
        } else {
            valueDisplay->updateValue(m.value, m.prefix, m.unit);
        }
        valueDisplay->setMode(m.modeStr);
        valueDisplay->setAuto(m.isAuto);
        valueDisplay->setHold(m.isHold);
        valueDisplay->setDelta(m.isDelta);
    }

    // --- Aktualizuj okno overlay (OBS) ---
    if (overlayWindow && overlayWindow->isOpen()) {
        if (m.isOL) {
            overlayWindow->updateValue(INFINITY, m.prefix, m.unit, m.precision);
        } else {
            overlayWindow->updateValue(m.value, m.prefix, m.unit, m.precision);
        }
        overlayWindow->setMode(m.modeStr);
        overlayWindow->setFlags(m.isAuto, m.isHold, m.isDelta, m.isOL);
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

    // --- Statystyki (Min/Max/Avg/Peak) ---
    if (!m.isOL && std::isfinite(m.value)) {
        stats.addSample(m.value);
        lastUnit = m.prefix + m.unit;
        updateStatsLabel();
    }

    // --- Wykres ---
    if (chartEnabled && chart && !m.isOL && std::isfinite(m.value)) {
        chart->addDataPoint(m.value, m.unit);
    }

    // --- DataLogger (CSV) ---
    if (dataLogger.isRecording()) {
        char valBuf[32];
        if (m.isOL) {
            snprintf(valBuf, sizeof(valBuf), "OL");
        } else {
            snprintf(valBuf, sizeof(valBuf), "%.6f", m.value);
        }
        std::string unitStr = StringUtils::wideToUtf8(m.prefix + m.unit);
        std::string modeStr = StringUtils::wideToUtf8(m.modeStr);
        dataLogger.addRow({
            valBuf, unitStr, modeStr,
            m.isOL    ? "1" : "0",
            m.isAuto  ? "1" : "0",
            m.isHold  ? "1" : "0",
            m.isDelta ? "1" : "0"
        });
        updateRecordLabel();
    }

    // --- Logowanie wartości (jeśli nie tryb RAW) ---
    if (!logRawData) {
        wchar_t valBuf[128];
        if (m.isOL) {
            swprintf(valBuf, 128, L"OL  %ls  [%ls]",
                     m.unit.c_str(), m.modeStr.c_str());
        } else {
            swprintf(valBuf, 128, L"%.*f %ls%ls  [%ls]",
                     m.precision, m.value,
                     m.prefix.c_str(), m.unit.c_str(),
                     m.modeStr.c_str());
        }
        logMsg(valBuf);
    }
}

// ============================================================================
// Konfiguracja callbacków BLE i inicjalizacja
// ============================================================================
void setupBLE() {
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
        lblStatus->setTextColor(RGB(160, 170, 180));
        logMsg(L"--- Skanowanie zakończone ---");
    });

    ble.onConnect([]() {
        isConnected = true;
        lblStatus->setText(L"Status: Połączony z OWON OW18B");
        lblStatus->setTextColor(RGB(50, 220, 80));
        logMsg(L"✔ Połączono z urządzeniem BLE!");
        resetStats();
    });

    ble.onDisconnect([]() {
        isConnected = false;
        lblStatus->setText(L"Status: Rozłączony");
        lblStatus->setTextColor(RGB(180, 140, 50));
        logMsg(L"✘ Rozłączono");
        if (valueDisplay) valueDisplay->setMode(L"---");
        if (lblMode) lblMode->setText(L"Tryb: ---");
    });

    ble.onReceive([](const std::vector<uint8_t>& data) {
        handleBLEData(data);
    });

    ble.onError([](const std::wstring& errMsg) {
        std::wstring msg = L"BŁĄD: " + errMsg;
        logMsg(msg);
        lblStatus->setText(L"Status: Błąd BLE");
        lblStatus->setTextColor(RGB(255, 60, 60));
        progressScan->setMarquee(false);
    });

    // Inicjalizacja adaptera BLE
    if (ble.init()) {
        ble.setServiceUUID(OWON_SERVICE_UUID);
        ble.setNotifyCharacteristicUUID(OWON_NOTIFY_CHARACTERISTIC_UUID);
        ble.setWriteCharacteristicUUID(OWON_WRITE_CHARACTERISTIC_UUID);

        ble.addPriorityFilter(L"owon");
        ble.addPriorityFilter(L"ow18b");

        lblStatus->setText(L"Status: BLE gotowe — kliknij 'Skanuj BLE'");
        lblStatus->setTextColor(RGB(100, 180, 255));
        logMsg(L"Adapter BLE zainicjalizowany.");
        logMsg(L"Włącz miernik OWON OW18B i kliknij 'Skanuj BLE'.");
    } else {
        lblStatus->setText(L"Status: BŁĄD — Bluetooth niedostępny!");
        lblStatus->setTextColor(RGB(255, 60, 60));
        logMsg(L"⚠ Bluetooth niedostępny w systemie!");
        logMsg(L"Upewnij się, że adapter BT jest włączony.");
    }
}
