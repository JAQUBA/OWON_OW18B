#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

// ============================================================================
// OWON OW18B BLE Protocol Parser
// ============================================================================
// Protokół: 6 bajtów na pomiar
//   Bajt 0:   Kod funkcji (tryb pomiaru)
//   Bajt 1:   Skala / zakres (pozycja kropki dziesiętnej)
//   Bajt 2-3: Flagi (AUTO, HOLD, itp.)
//   Bajt 4-5: Wartość pomiaru (16-bit little-endian, signed)
//             data[5] < 128  → wartość dodatnia = data[5]*256 + data[4]
//             data[5] >= 128 → wartość ujemna   = -((data[5]-128)*256 + data[4])
// ============================================================================

namespace OW18B {

// --- Tryby pomiaru (function codes) ---
enum class MeasurementMode : uint8_t {
    UNKNOWN         = 0x00,
    DC_uA           = 0x11,   // Prąd stały µA
    DC_mA           = 0x12,   // Prąd stały mA
    DC_A            = 0x13,   // Prąd stały A
    AC_uA           = 0x15,   // Prąd zmienny µA
    AC_mA           = 0x16,   // Prąd zmienny mA
    AC_A            = 0x17,   // Prąd zmienny A
    DC_mV           = 0x19,   // Napięcie stałe mV
    AC_mV           = 0x1A,   // Napięcie zmienne mV
    AC_V            = 0x21,   // Napięcie zmienne V
    DC_V            = 0x22,   // Napięcie stałe V
    RESISTANCE      = 0x23,   // Rezystancja Ω
    CONTINUITY      = 0x24,   // Ciągłość (buzzer)
    DIODE           = 0x25,   // Test diody
    CAPACITANCE     = 0x26,   // Pojemność F
    FREQUENCY       = 0x27,   // Częstotliwość Hz
    TEMPERATURE_C   = 0x28,   // Temperatura °C
    TEMPERATURE_F   = 0x29,   // Temperatura °F
    DUTY_CYCLE      = 0x2A,   // Współczynnik wypełnienia %
    HFE             = 0x2D,   // Wzmocnienie tranzystora hFE
    NCV             = 0x30,   // Non-Contact Voltage
};

// --- Struktura wyniku pomiaru ---
struct Measurement {
    MeasurementMode mode    = MeasurementMode::UNKNOWN;
    double          value   = 0.0;
    std::wstring    unit;       // np. L"V", L"mV", L"Ω"
    std::wstring    prefix;     // np. L"k", L"M", L"m", L"µ", L"n"
    std::wstring    modeStr;    // np. L"DC V", L"AC mA"
    uint8_t         scale   = 0;
    uint8_t         flags1  = 0;
    uint8_t         flags2  = 0;
    bool            isAuto  = false;
    bool            isHold  = false;
    bool            isDelta = false;
    bool            isOL    = false;   // Over Limit (przepełnienie)
    bool            isValid = false;
    int             precision = 2;     // Liczba miejsc po przecinku
};

// --- Klasa parsera ---
class Parser {
public:
    // Parsuj surowe dane BLE (minimum 6 bajtów)
    static Measurement parse(const std::vector<uint8_t>& data);

    // Pobierz nazwę trybu jako wide string
    static std::wstring getModeString(MeasurementMode mode);

    // Pobierz jednostkę jako wide string
    static std::wstring getUnitString(MeasurementMode mode);

    // Pobierz prefiks jednostki
    static std::wstring getPrefixString(MeasurementMode mode);

private:
    // Dekoduj wartość z bajtów 4-5
    static double decodeValue(uint8_t low, uint8_t high, bool& isNegative);

    // Oblicz dzielnik na podstawie trybu i skali
    static double getDivisor(MeasurementMode mode, uint8_t scale);

    // Oblicz precyzję wyświetlania
    static int getPrecision(MeasurementMode mode, uint8_t scale);
};

} // namespace OW18B
