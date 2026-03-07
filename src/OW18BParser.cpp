#include "OW18BParser.h"
#include <cmath>

namespace OW18B {

// ============================================================================
// Nazwa trybu pomiaru
// ============================================================================
std::wstring Parser::getModeString(MeterMode mode) {
    switch (mode) {
        case MeterMode::DC_Voltage:  return L"DC V";
        case MeterMode::AC_Voltage:  return L"AC V";
        case MeterMode::DC_Ampere:   return L"DC A";
        case MeterMode::AC_Ampere:   return L"AC A";
        case MeterMode::Ohm:         return L"Ω";
        case MeterMode::Farad:       return L"Pojemność";
        case MeterMode::Hz:          return L"Częstotliwość";
        case MeterMode::Percent:     return L"Duty Cycle";
        case MeterMode::Celsius:     return L"Temp °C";
        case MeterMode::Fahrenheit:  return L"Temp °F";
        case MeterMode::Diode:       return L"Dioda";
        case MeterMode::Continuity:  return L"Ciągłość";
        case MeterMode::HFEC:        return L"hFE";
        case MeterMode::NCV:         return L"NCV";
        default:                     return L"---";
    }
}

// ============================================================================
// Jednostka pomiaru
// ============================================================================
std::wstring Parser::getUnitString(MeterMode mode) {
    switch (mode) {
        case MeterMode::DC_Voltage:
        case MeterMode::AC_Voltage:
        case MeterMode::Diode:       return L"V";
        case MeterMode::DC_Ampere:
        case MeterMode::AC_Ampere:   return L"A";
        case MeterMode::Ohm:
        case MeterMode::Continuity:  return L"Ω";
        case MeterMode::Farad:       return L"F";
        case MeterMode::Hz:          return L"Hz";
        case MeterMode::Percent:     return L"%";
        case MeterMode::Celsius:     return L"°C";
        case MeterMode::Fahrenheit:  return L"°F";
        case MeterMode::HFEC:        return L"hFE";
        default:                     return L"";
    }
}

// ============================================================================
// Prefiks jednostki (p, n, µ, m, -, k, M, G)
// ============================================================================
std::wstring Parser::getPrefixString(MeterPrefix prefix) {
    switch (prefix) {
        case MeterPrefix::Pico:  return L"p";
        case MeterPrefix::Nano:  return L"n";
        case MeterPrefix::Micro: return L"µ";
        case MeterPrefix::Milli: return L"m";
        case MeterPrefix::None:  return L"";
        case MeterPrefix::Kilo:  return L"k";
        case MeterPrefix::Mega:  return L"M";
        case MeterPrefix::Giga:  return L"G";
        default:                 return L"";
    }
}

// ============================================================================
// Wartość dzielnika (ile razy dzielić raw value)
// ============================================================================
double Parser::getDivisorValue(MeterDivisor divisor) {
    switch (divisor) {
        case MeterDivisor::D1:     return 1.0;
        case MeterDivisor::D10:    return 10.0;
        case MeterDivisor::D100:   return 100.0;
        case MeterDivisor::D1000:  return 1000.0;
        case MeterDivisor::D10000: return 10000.0;
        default:                   return 1.0;
    }
}

// ============================================================================
// Precyzja (liczba miejsc po przecinku)
// ============================================================================
int Parser::getPrecision(MeterDivisor divisor) {
    switch (divisor) {
        case MeterDivisor::D1:     return 0;
        case MeterDivisor::D10:    return 1;
        case MeterDivisor::D100:   return 2;
        case MeterDivisor::D1000:  return 3;
        case MeterDivisor::D10000: return 4;
        default:                   return 0;
    }
}

// ============================================================================
// Główna funkcja parsowania (format MartMet — pola bitowe w word0)
// ============================================================================
Measurement Parser::parse(const std::vector<uint8_t>& data) {
    Measurement m;
    m.isValid = false;

    if (data.size() < 6) return m;

    // --- Bajty 0-1: word0 (little-endian) — mode, prefix, divisor ---
    uint16_t word0 = data[0] | (data[1] << 8);
    m.rawWord0 = word0;

    uint8_t divBits    = word0 & 0x07;           // bity 0-2: Divisor
    uint8_t prefixBits = (word0 >> 3) & 0x07;    // bity 3-5: Prefix
    uint8_t modeBits   = (word0 >> 6) & 0x0F;    // bity 6-9: Mode

    m.divisor  = static_cast<MeterDivisor>(divBits);
    m.prefixId = static_cast<MeterPrefix>(prefixBits);
    m.mode     = (modeBits <= 13) ? static_cast<MeterMode>(modeBits)
                                  : MeterMode::UNKNOWN;

    // --- Bajt 2: Flagi (JayTee42 format) ---
    uint8_t flags = data[2];
    m.isHold   = (flags & 0x01) != 0;   // bit 0: Data Hold Mode
    m.isDelta  = (flags & 0x02) != 0;   // bit 1: Relative Mode
    m.isAuto   = (flags & 0x04) != 0;   // bit 2: Auto Ranging
    m.isLowBat = (flags & 0x08) != 0;   // bit 3: Low Battery

    // --- Bajty 4-5: Wartość (sign-magnitude 16-bit LE) ---
    // Bit 15 = znak (1=ujemna), bity 0-14 = wartość bezwzględna
    uint16_t rawVal = data[4] | (data[5] << 8);
    bool isNegative = (rawVal & 0x8000) != 0;
    int16_t magnitude = static_cast<int16_t>(rawVal & 0x7FFF);
    int16_t signedVal = isNegative ? -magnitude : magnitude;
    m.rawValue = signedVal;

    // --- OL / UL / ERR ---
    if (m.divisor == MeterDivisor::OL) {
        m.isOL = true;
        m.value = INFINITY;
    } else if (m.divisor == MeterDivisor::UL) {
        m.isUL = true;
        m.value = 0.0;
    } else if (m.divisor == MeterDivisor::ERR) {
        m.isOL = true;
        m.value = INFINITY;
    } else {
        double div = getDivisorValue(m.divisor);
        m.value = static_cast<double>(signedVal) / div;
    }

    // --- Stringi ---
    m.modeStr   = getModeString(m.mode);
    m.unit      = getUnitString(m.mode);
    m.prefix    = getPrefixString(m.prefixId);
    m.precision = getPrecision(m.divisor);

    m.isValid = true;
    return m;
}

} // namespace OW18B
