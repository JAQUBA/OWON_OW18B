#pragma once
#include <cstdint>
#include <string>
#include <vector>

// ============================================================================
// OWON OW18B BLE Protocol Parser
// ============================================================================
// Protokół: 6 bajtów na pomiar (format MartMet/JayTee42)
//
//   Bajty 0-1 (word0, little-endian):
//     bity 0-2:  Divisor  (pozycja kropki dziesiętnej / OL / UL)
//     bity 3-5:  Prefix   (pico, nano, µ, m, -, k, M, G)
//     bity 6-9:  Mode     (DC_V, AC_V, DC_A, AC_A, Ω, F, Hz, %, °C, °F,
//                           Diode, Continuity, hFE, NCV)
//     bity 10+:  nieużywane
//
//   Bajt 2 (flagi):
//     bit 0: HOLD         (Data Hold Mode)
//     bit 1: REL/DELTA    (Relative Mode)
//     bit 2: AUTO         (Auto Ranging)
//     bit 3: LOW_BAT      (Niska bateria)
//
//   Bajt 3: zarezerwowany
//
//   Bajty 4-5 (value, sign-magnitude 16-bit LE):
//     bit 15 = znak (1=ujemna), bity 0-14 = wartość bezwzględna
//
// Źródła:
//   https://github.com/MartMet/OW18B (Blazor WebBluetooth — format bitowy)
//   https://github.com/JayTee42/ow18b (Linux — opis flag i sign-magnitude)
// ============================================================================

namespace OW18B {

// --- Tryb pomiaru (4 bity, pozycja 6-9 w word0) ---
enum class MeterMode : uint8_t {
    DC_Voltage  = 0,
    AC_Voltage  = 1,
    DC_Ampere   = 2,
    AC_Ampere   = 3,
    Ohm         = 4,
    Farad       = 5,
    Hz          = 6,
    Percent     = 7,
    Celsius     = 8,
    Fahrenheit  = 9,
    Diode       = 10,
    Continuity  = 11,
    HFEC        = 12,
    NCV         = 13,
    UNKNOWN     = 0xFF
};

// --- Prefiks jednostki (3 bity, pozycja 3-5 w word0) ---
enum class MeterPrefix : uint8_t {
    Pico  = 0,
    Nano  = 1,
    Micro = 2,
    Milli = 3,
    None  = 4,
    Kilo  = 5,
    Mega  = 6,
    Giga  = 7
};

// --- Dzielnik / punkt dziesiętny (3 bity, pozycja 0-2 w word0) ---
enum class MeterDivisor : uint8_t {
    D1       = 0,   // wartość × 1
    D10      = 1,   // wartość / 10
    D100     = 2,   // wartość / 100
    D1000    = 3,   // wartość / 1000
    D10000   = 4,   // wartość / 10000
    ERR      = 5,   // Błąd
    UL       = 6,   // Under Limit
    OL       = 7    // Over Limit
};

// --- Struktura wyniku pomiaru ---
struct Measurement {
    MeterMode       mode      = MeterMode::UNKNOWN;
    MeterPrefix     prefixId  = MeterPrefix::None;
    MeterDivisor    divisor   = MeterDivisor::D1;
    double          value     = 0.0;
    std::wstring    unit;        // np. L"V", L"A", L"Ω"
    std::wstring    prefix;      // np. L"k", L"M", L"m", L"µ"
    std::wstring    modeStr;     // np. L"DC V", L"AC A", L"Ω"
    bool            isAuto     = false;
    bool            isHold     = false;
    bool            isDelta    = false;
    bool            isLowBat   = false;
    bool            isOL       = false;
    bool            isUL       = false;
    bool            isValid    = false;
    int             precision  = 0;
    uint16_t        rawWord0   = 0;    // Surowe bytes 0-1 (debug)
    int16_t         rawValue   = 0;    // Surowe bytes 4-5 (debug)
};

// --- Klasa parsera (bezstanowa — wszystkie metody statyczne) ---
class Parser {
public:
    static Measurement parse(const std::vector<uint8_t>& data);
    static std::wstring getModeString(MeterMode mode);
    static std::wstring getUnitString(MeterMode mode);
    static std::wstring getPrefixString(MeterPrefix prefix);
    static double       getDivisorValue(MeterDivisor divisor);
    static int          getPrecision(MeterDivisor divisor);
};

} // namespace OW18B
