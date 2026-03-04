#include "OW18BParser.h"
#include <cmath>

namespace OW18B {

// ============================================================================
// Dekodowanie wartości z bajtów 4-5 (16-bit little-endian, signed via byte 5)
// ============================================================================
double Parser::decodeValue(uint8_t low, uint8_t high, bool& isNegative) {
    isNegative = (high >= 128);
    if (isNegative) {
        return static_cast<double>((high - 128) * 256 + low);
    }
    return static_cast<double>(high * 256 + low);
}

// ============================================================================
// Dzielnik wartości zależny od trybu i bajtu skali
// ============================================================================
double Parser::getDivisor(MeasurementMode mode, uint8_t scale) {
    switch (mode) {
        // --- Napięcie DC/AC V ---
        case MeasurementMode::DC_V:
        case MeasurementMode::AC_V:
            switch (scale) {
                case 0: return 10000.0;   // 0.xxxx V
                case 1: return 1000.0;    // x.xxx V
                case 2: return 100.0;     // xx.xx V
                case 3: return 10.0;      // xxx.x V
                case 4: return 1.0;       // xxxx V
                default: return 100.0;
            }

        // --- Napięcie DC/AC mV ---
        case MeasurementMode::DC_mV:
        case MeasurementMode::AC_mV:
            switch (scale) {
                case 0: return 100.0;     // x.xx mV
                case 1: return 10.0;      // xx.x mV
                case 2: return 1.0;       // xxx mV
                default: return 10.0;
            }

        // --- Prąd µA ---
        case MeasurementMode::DC_uA:
        case MeasurementMode::AC_uA:
            switch (scale) {
                case 0: return 100.0;     // x.xx µA
                case 1: return 10.0;      // xx.x µA
                case 2: return 1.0;       // xxx µA
                default: return 10.0;
            }

        // --- Prąd mA ---
        case MeasurementMode::DC_mA:
        case MeasurementMode::AC_mA:
            switch (scale) {
                case 0: return 1000.0;    // x.xxx mA
                case 1: return 100.0;     // xx.xx mA
                case 2: return 10.0;      // xxx.x mA
                case 3: return 1.0;       // xxxx mA
                default: return 100.0;
            }

        // --- Prąd A ---
        case MeasurementMode::DC_A:
        case MeasurementMode::AC_A:
            switch (scale) {
                case 0: return 10000.0;
                case 1: return 1000.0;
                case 2: return 100.0;
                case 3: return 10.0;
                default: return 1000.0;
            }

        // --- Rezystancja Ω ---
        case MeasurementMode::RESISTANCE:
        case MeasurementMode::CONTINUITY:
            switch (scale) {
                case 0: return 100.0;     // x.xx Ω
                case 1: return 10.0;      // xx.x Ω / kΩ
                case 2: return 1.0;       // xxx Ω / kΩ / MΩ
                case 3: return 1000.0;    // x.xxx kΩ
                case 4: return 100.0;     // xx.xx kΩ / MΩ
                default: return 100.0;
            }

        // --- Pojemność F ---
        case MeasurementMode::CAPACITANCE:
            switch (scale) {
                case 0: return 1000.0;    // x.xxx nF
                case 1: return 100.0;     // xx.xx nF/µF
                case 2: return 10.0;      // xxx.x nF/µF
                case 3: return 1.0;       // xxxx nF/µF
                default: return 100.0;
            }

        // --- Częstotliwość Hz ---
        case MeasurementMode::FREQUENCY:
            switch (scale) {
                case 0: return 100.0;     // xx.xx Hz
                case 1: return 10.0;      // xxx.x Hz
                case 2: return 1.0;       // xxxx Hz
                case 3: return 1000.0;    // x.xxx kHz
                case 4: return 100.0;     // xx.xx kHz
                default: return 100.0;
            }

        // --- Dioda ---
        case MeasurementMode::DIODE:
            return 10000.0;  // x.xxxx V

        // --- Temperatura ---
        case MeasurementMode::TEMPERATURE_C:
        case MeasurementMode::TEMPERATURE_F:
            switch (scale) {
                case 0: return 10.0;      // xxx.x °C
                case 1: return 1.0;       // xxxx °C  
                default: return 10.0;
            }

        // --- Duty Cycle ---
        case MeasurementMode::DUTY_CYCLE:
            switch (scale) {
                case 0: return 100.0;     // xx.xx %
                case 1: return 10.0;      // xxx.x %
                default: return 10.0;
            }

        // --- hFE ---
        case MeasurementMode::HFE:
            return 1.0;

        // --- NCV ---
        case MeasurementMode::NCV:
            return 1.0;

        default:
            return 100.0;
    }
}

// ============================================================================
// Precyzja (liczba miejsc po przecinku)
// ============================================================================
int Parser::getPrecision(MeasurementMode mode, uint8_t scale) {
    double divisor = getDivisor(mode, scale);
    if (divisor >= 10000.0) return 4;
    if (divisor >= 1000.0)  return 3;
    if (divisor >= 100.0)   return 2;
    if (divisor >= 10.0)    return 1;
    return 0;
}

// ============================================================================
// Nazwa trybu pomiaru
// ============================================================================
std::wstring Parser::getModeString(MeasurementMode mode) {
    switch (mode) {
        case MeasurementMode::DC_V:         return L"DC V";
        case MeasurementMode::AC_V:         return L"AC V";
        case MeasurementMode::DC_mV:        return L"DC mV";
        case MeasurementMode::AC_mV:        return L"AC mV";
        case MeasurementMode::DC_uA:        return L"DC µA";
        case MeasurementMode::AC_uA:        return L"AC µA";
        case MeasurementMode::DC_mA:        return L"DC mA";
        case MeasurementMode::AC_mA:        return L"AC mA";
        case MeasurementMode::DC_A:         return L"DC A";
        case MeasurementMode::AC_A:         return L"AC A";
        case MeasurementMode::RESISTANCE:   return L"Ω";
        case MeasurementMode::CONTINUITY:   return L"Ciągłość";
        case MeasurementMode::DIODE:        return L"Dioda";
        case MeasurementMode::CAPACITANCE:  return L"Pojemność";
        case MeasurementMode::FREQUENCY:    return L"Częstotliwość";
        case MeasurementMode::TEMPERATURE_C:return L"Temp °C";
        case MeasurementMode::TEMPERATURE_F:return L"Temp °F";
        case MeasurementMode::DUTY_CYCLE:   return L"Duty Cycle";
        case MeasurementMode::HFE:          return L"hFE";
        case MeasurementMode::NCV:          return L"NCV";
        default:                            return L"---";
    }
}

// ============================================================================
// Jednostka pomiaru
// ============================================================================
std::wstring Parser::getUnitString(MeasurementMode mode) {
    switch (mode) {
        case MeasurementMode::DC_V:
        case MeasurementMode::AC_V:         return L"V";
        case MeasurementMode::DC_mV:
        case MeasurementMode::AC_mV:        return L"mV";
        case MeasurementMode::DC_uA:
        case MeasurementMode::AC_uA:        return L"µA";
        case MeasurementMode::DC_mA:
        case MeasurementMode::AC_mA:        return L"mA";
        case MeasurementMode::DC_A:
        case MeasurementMode::AC_A:         return L"A";
        case MeasurementMode::RESISTANCE:
        case MeasurementMode::CONTINUITY:   return L"Ω";
        case MeasurementMode::DIODE:        return L"V";
        case MeasurementMode::CAPACITANCE:  return L"F";
        case MeasurementMode::FREQUENCY:    return L"Hz";
        case MeasurementMode::TEMPERATURE_C:return L"°C";
        case MeasurementMode::TEMPERATURE_F:return L"°F";
        case MeasurementMode::DUTY_CYCLE:   return L"%";
        case MeasurementMode::HFE:          return L"hFE";
        case MeasurementMode::NCV:          return L"";
        default:                            return L"";
    }
}

// ============================================================================
// Prefiks jednostki (k, M, m, µ, n)
// ============================================================================
std::wstring Parser::getPrefixString(MeasurementMode mode) {
    // Prefiks jest zazwyczaj pusty — bazowa jednostka już zawiera m/µ
    // Prefiks stosowany jest dla Ω (kΩ, MΩ) i F (nF, µF, mF) i Hz (kHz, MHz)
    // Ale to zależy od zakresu, więc tutaj zwracamy pusty
    return L"";
}

// ============================================================================
// Główna funkcja parsowania
// ============================================================================
Measurement Parser::parse(const std::vector<uint8_t>& data) {
    Measurement m;
    m.isValid = false;

    if (data.size() < 6) {
        return m;
    }

    // --- Bajt 0: Kod funkcji ---
    m.mode = static_cast<MeasurementMode>(data[0]);

    // --- Bajt 1: Skala ---
    m.scale = data[1];

    // --- Bajty 2-3: Flagi ---
    m.flags1 = data[2];
    m.flags2 = data[3];

    // Dekoduj flagi (typowe bity)
    m.isAuto  = (m.flags1 & 0x01) != 0;
    m.isHold  = (m.flags1 & 0x02) != 0;
    m.isDelta = (m.flags1 & 0x04) != 0;

    // --- Bajty 4-5: Wartość ---
    bool isNegative = false;
    double rawValue = decodeValue(data[4], data[5], isNegative);

    // Sprawdź Over Limit (OL) — typowo 0x7FFF lub bardzo duża wartość
    if (rawValue >= 32767.0 || (data[4] == 0xFF && data[5] == 0x7F)) {
        m.isOL = true;
        m.value = INFINITY;
    } else {
        double divisor = getDivisor(m.mode, m.scale);
        m.value = rawValue / divisor;
        if (isNegative) m.value = -m.value;
    }

    // --- Prefiks dla zakresów wielości ---
    m.prefix = L"";
    switch (m.mode) {
        case MeasurementMode::RESISTANCE:
        case MeasurementMode::CONTINUITY:
            if (m.scale >= 3 && m.scale <= 4) {
                m.prefix = L"k";
            } else if (m.scale >= 5) {
                m.prefix = L"M";
            }
            break;

        case MeasurementMode::CAPACITANCE:
            if (m.scale <= 1) {
                m.prefix = L"n";
            } else if (m.scale <= 3) {
                m.prefix = L"µ";
            } else {
                m.prefix = L"m";
            }
            break;

        case MeasurementMode::FREQUENCY:
            if (m.scale >= 3 && m.scale <= 4) {
                m.prefix = L"k";
            } else if (m.scale >= 5) {
                m.prefix = L"M";
            }
            break;

        default:
            break;
    }

    // --- Metadane ---
    m.modeStr   = getModeString(m.mode);
    m.unit      = getUnitString(m.mode);
    m.precision = getPrecision(m.mode, m.scale);
    m.isValid   = (m.mode != MeasurementMode::UNKNOWN);

    return m;
}

} // namespace OW18B
