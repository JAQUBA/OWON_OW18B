// ============================================================================
// Komendy sterujące multimetrem OWON OW18B
// Format: 2 bajty — {button_id, press_type}
//   press_type: 0x01 = krótkie, 0x00 = długie
//   Źródło: https://github.com/MartMet/OW18B (Blazor WebBluetooth)
// ============================================================================
#ifndef OW18B_COMMANDS_H
#define OW18B_COMMANDS_H

#include <cstdint>
#include <vector>

namespace OW18BCmd {
    // Identyfikatory przycisków
    constexpr uint8_t BTN_SELECT = 0x01;
    constexpr uint8_t BTN_RANGE  = 0x02;
    constexpr uint8_t BTN_HOLD   = 0x03;
    constexpr uint8_t BTN_HZDUTY = 0x04;

    // Typy naciśnięcia
    constexpr uint8_t PRESS_SHORT = 0x01;
    constexpr uint8_t PRESS_LONG  = 0x00;

    // Komendy gotowe do wysłania
    inline std::vector<uint8_t> SELECT()      { return { BTN_SELECT, PRESS_SHORT }; }
    inline std::vector<uint8_t> SELECT_LONG() { return { BTN_SELECT, PRESS_LONG  }; }
    inline std::vector<uint8_t> HOLD()        { return { BTN_HOLD,   PRESS_SHORT }; }
    inline std::vector<uint8_t> HOLD_LONG()   { return { BTN_HOLD,   PRESS_LONG  }; }
    inline std::vector<uint8_t> RANGE()       { return { BTN_RANGE,  PRESS_SHORT }; }
    inline std::vector<uint8_t> RANGE_LONG()  { return { BTN_RANGE,  PRESS_LONG  }; }
    inline std::vector<uint8_t> HZ_DUTY()     { return { BTN_HZDUTY, PRESS_SHORT }; }
    inline std::vector<uint8_t> HZ_DUTY_LONG(){ return { BTN_HZDUTY, PRESS_LONG  }; }
}

#endif // OW18B_COMMANDS_H
