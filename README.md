# OWON OW18B — Bluetooth Multimeter (Windows)

Native Windows desktop application for communicating with the **OWON OW18B** multimeter via Bluetooth Low Energy (BLE). Receives real-time measurements, displays them in a dark-themed GUI with a chart, and supports remote control, CSV recording, statistics, and an OBS overlay.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![Windows 10+](https://img.shields.io/badge/Windows-10%2B-0078D6)
![PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange)
![BLE](https://img.shields.io/badge/BLE-4.0%2B-blueviolet)

---

## Features

- **BLE scanning** — automatic detection of OWON OW18B multimeters in range
- **LCD display** — large, readable value display in multimeter style (Consolas, green on black)
- **Real-time chart** — 60-second window with auto-scaling and 10 FPS refresh
- **MIN/MAX/AVG/PEAK statistics** — continuous tracking with reset option
- **Remote control** — send commands to the multimeter via BLE (SELECT, HOLD, RANGE, Hz/DUTY)
- **CSV recording** — save measurements to CSV files with auto-timestamp
- **OBS overlay window** — customizable always-on-top overlay for streaming
- **Keyboard shortcuts** — configurable global hotkeys with a settings dialog
- **Dark theme** — consistent dark color palette across all UI elements
- **Measurement modes** — full support for all OW18B modes:

| Category | Modes |
|----------|-------|
| Voltage | DC V, AC V, DC mV, AC mV |
| Current | DC µA, AC µA, DC mA, AC mA, DC A, AC A |
| Resistance | Ω (with k/M prefixes) |
| Other | Continuity, Diode, Capacitance (nF/µF/mF), Frequency (Hz/kHz/MHz) |
| Special | Temperature °C/°F, Duty Cycle %, hFE, NCV |

- **Flags** — AUTO, HOLD, DELTA, LOW_BAT, Over Limit (OL)

---

## Requirements

| Requirement | Minimum |
|-------------|---------|
| Operating system | Windows 10 (build 1809+) |
| Bluetooth adapter | BLE 4.0+ |
| Build system | [PlatformIO](https://platformio.org/) (CLI or VS Code extension) |
| Compiler | MinGW GCC with C++17 support |

---

## Installation & Build

### 1. Clone the repository

```bash
git clone https://github.com/<your-user>/OWON_OW18B.git
cd OWON_OW18B
```

### 2. Build with PlatformIO

```bash
# PlatformIO CLI
pio run -e app

# Or in VS Code with the PlatformIO extension — click "Build"
```

The [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) dependency will be downloaded automatically by PlatformIO.

### 3. Run

```bash
pio run -e app -t exec

# Or run the executable directly:
.pio\build\app\program.exe
```

---

## Usage

1. **Turn on** the OWON OW18B multimeter and ensure Bluetooth is active
2. **Launch the application** — the "OWON OW18B — BLE Multimeter" window appears
3. **Click "Scan BLE"** — the app searches for BLE devices for 10 seconds
4. **Select a device** from the dropdown list
5. **Click "Connect"** — measurement data appears on the display and chart
6. **Click "Disconnect"** to end the session

### Menu Bar

```
File → Record CSV | Stop Recording | Close
Connection → Scan BLE | Connect | Disconnect
Control → SELECT | HOLD | RANGE | Hz/DUTY
View → OBS Overlay Window
Settings → Keyboard Shortcuts... | ✓ Chart Active | ✓ Log RAW Data | Reset Statistics
Help → About...
```

### Options

- **Chart Active** — enable/disable chart recording
- **Log RAW Data (hex)** — show raw 6-byte BLE packets in the log
- **Reset Statistics** — reset MIN/MAX/AVG/PEAK tracking
- **OBS Overlay Window** — open a customizable overlay for OBS Studio
- **Keyboard Shortcuts** — configure hotkeys for remote multimeter control

---

## Project Architecture

```
OWON_OW18B/
├── .github/
│   └── copilot-instructions.md   # GitHub Copilot instructions
├── include/
│   └── OW18BParser.h             # OW18B protocol parser header
├── src/
│   ├── main.cpp                  # Entry point: setup(), loop() — module orchestration
│   ├── AppState.h / .cpp         # Global state, helpers, shared actions, INI config
│   ├── AppUI.h / .cpp            # UI component creation (dark theme)
│   ├── BLEHandler.h / .cpp       # BLE callbacks, data parsing, fan-out pipeline
│   ├── MenuHandler.h / .cpp      # Menu bar, command routing
│   ├── MeterOverlay.h / .cpp     # OBS overlay window (OverlayWindow subclass)
│   ├── OW18B_Commands.h          # Multimeter control commands (2-byte BLE write)
│   └── OW18BParser.cpp           # BLE protocol parser implementation
├── platformio.ini                # PlatformIO config (platform: native)
├── owon_meter.ini                # User config (shortcuts, settings, auto-saved)
├── app.manifest                  # Windows Common Controls v6 manifest
├── resources.rc                  # Windows resource file (icon + manifest)
└── resources/
    └── icon.ico                  # Application icon
```

### Modules

| Module | Responsibility |
|--------|---------------|
| **main.cpp** | `setup()` / `loop()` — window, menu, UI, BLE, hotkeys initialization (~65 lines) |
| **AppState** | Global variables, helpers (`logMsg()`, `sendCommand()`, `resetStats()`), shared actions (`doScanBLE()`, `doConnectBLE()`, `doRecordStart()` etc.), INI load/save |
| **AppUI** | `createUI()` — all UI components with dark theme; `styleBtn()` helper for consistent button styling |
| **BLEHandler** | `setupBLE()` — BLE callback registration; `handleBLEData()` — packet parsing → UI/overlay/stats/chart/CSV/log fan-out |
| **MenuHandler** | `createAppMenu()` — HMENU creation; `handleMenuCommand()` — command routing; `updateMenuChecks()` |
| **MeterOverlay** | `OverlayWindow` subclass — renders measurement value (Consolas) and mode/flags (Segoe UI) for OBS |
| **OW18B_Commands** | Inline functions returning `std::vector<uint8_t>{button_id, press_type}` for 2-byte commands |
| **OW18BParser** | Stateless 6-byte protocol parser — `OW18B::Parser::parse()` (MartMet/OW18B bitfield format) |

### Data Pipeline

```
BLE.onReceive() → handleBLEData()
    ├── OW18B::Parser::parse()     → Measurement struct
    ├── valueDisplay->updateValue() → LCD display
    ├── overlayWindow->updateValue()→ OBS overlay
    ├── stats.addSample()          → MIN/MAX/AVG/PEAK
    ├── chart->addDataPoint()      → real-time chart
    ├── dataLogger.addRow()        → CSV file
    └── logMsg()                   → text log
```

---

## BLE Protocol — OWON OW18B

The multimeter sends **6-byte packets** via BLE Notify (bitfield format — source: [MartMet/OW18B](https://github.com/MartMet/OW18B)):

| Byte(s) | Description |
|---------|-------------|
| 0-1 | Word0 (16-bit LE) — bitfields: bits 0-2 = Divisor, bits 3-5 = Prefix, bits 6-9 = Mode |
| 2 | Flags: bit 0 = HOLD, bit 1 = REL/DELTA, bit 2 = AUTO, bit 3 = LOW_BAT |
| 3 | Reserved |
| 4-5 | Value: signed int16_t little-endian |

### Divisor (3 bits: 0-2)

| Value | Meaning |
|-------|---------|
| 0-4 | D1, D10, D100, D1000, D10000 (value divisor) |
| 5 | ERR |
| 6 | Under Limit |
| 7 | Over Limit |

### Prefix (3 bits: 3-5)

| Value | Prefix |
|-------|--------|
| 0 | pico | 1 | nano | 2 | µ (micro) | 3 | milli |
| 4 | none | 5 | kilo | 6 | Mega | 7 | Giga |

### Mode (4 bits: 6-9)

| Value | Mode | Value | Mode |
|-------|------|-------|------|
| 0 | DC V | 7 | % (Duty) |
| 1 | AC V | 8 | °C |
| 2 | DC A | 9 | °F |
| 3 | AC A | 10 | Diode |
| 4 | Ω | 11 | Continuity |
| 5 | F (Capacitance) | 12 | hFE |
| 6 | Hz | 13 | NCV |

### BLE UUIDs

| Role | UUID |
|------|------|
| Service | `0000fff0-0000-1000-8000-00805f9b34fb` |
| Notify (data) | `0000fff4-0000-1000-8000-00805f9b34fb` |
| Write (commands) | `0000fff3-0000-1000-8000-00805f9b34fb` |

### Control Commands (BLE Write)

**Format:** 2 bytes `{button_id, press_type}` — short press = `0x01`, long press = `0x00`

| Command | Bytes |
|---------|-------|
| SELECT | `{0x01, 0x01}` |
| HOLD | `{0x03, 0x01}` |
| RANGE | `{0x02, 0x01}` |
| Hz/DUTY | `{0x04, 0x01}` |

---

## Dependencies

- [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) — lightweight Win32 UI library (auto-downloaded by PlatformIO)
   - UI: `SimpleWindow`, `OverlayWindow`, `Label`, `Button`, `Select`, `TextArea`, `ProgressBar`, `Chart`, `ValueDisplay`, `CheckBox`
   - BLE: `IO/BLE/BLE.h`
   - Util: `StringUtils`, `ConfigManager`, `DataLogger`, `HotkeyManager`, `Statistics`

---

## Configuration — `owon_meter.ini`

| Key | Description | Default |
|-----|-------------|---------|
| `chart_enabled` | Chart active | `1` |
| `log_raw_data` | Log RAW hex data | `0` |
| `shortcut_select` | Shortcut: SELECT | `Ctrl+Alt+1` |
| `shortcut_hold` | Shortcut: HOLD | `Ctrl+Alt+2` |
| `shortcut_range` | Shortcut: RANGE | `Ctrl+Alt+3` |
| `shortcut_hzduty` | Shortcut: Hz/DUTY | `Ctrl+Alt+4` |
| `shortcut_hold_long` | Shortcut: HOLD (long) | `Ctrl+Shift+Alt+2` |
| `overlay_*` | Overlay position, size, colors | (various) |

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

## Author

Built with [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) by [JAQUBA](https://github.com/JAQUBA).
