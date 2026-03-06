// ============================================================================
// BLEHandler — Obsługa komunikacji BLE z multimetrem OWON OW18B
// ============================================================================
#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <vector>
#include <cstdint>

void setupBLE();
void handleBLEData(const std::vector<uint8_t>& data);

#endif // BLE_HANDLER_H
