# Copilot Instructions — OWON OW18B

## Project Description

Native Windows desktop application (C++) for communicating with the **OWON OW18B** multimeter via Bluetooth Low Energy (BLE). The app receives 6-byte measurement packets, parses them, and displays results in a dark-themed GUI with a real-time chart. It supports remote multimeter control (BLE commands), CSV recording, MIN/MAX/AVG/PEAK statistics, configurable keyboard shortcuts, and an OBS Studio overlay window.

## Architecture

```
OWON_OW18B/
├── include/
│   └── OW18BParser.h        # OW18B protocol parser header (enum, struct, class)
├── src/
│   ├── main.cpp              # Entry point: setup(), loop() — module orchestration
│   ├── AppState.h / .cpp     # Global state, variables, helpers (logMsg, sendCommand, resetStats)
│   ├── AppUI.h / .cpp        # UI component creation (createUI) with dark theme
│   ├── BLEHandler.h / .cpp   # BLE callbacks, data parsing (handleBLEData, setupBLE)
│   ├── MenuHandler.h / .cpp  # Menu bar, command routing
│   ├── MeterOverlay.h / .cpp # OverlayWindow subclass — OBS overlay rendering meter values
│   ├── OW18B_Commands.h      # Multimeter control commands (namespace OW18BCmd)
│   └── OW18BParser.cpp       # BLE protocol parser implementation
├── platformio.ini            # PlatformIO config (platform: native)
├── owon_meter.ini            # User config (shortcuts, settings, auto-saved)
├── resources/
│   └── icon.ico              # Application icon
├── app.manifest              # Windows Common Controls v6 manifest
└── resources.rc              # Windows resource file (icon + manifest)
```

### Module Responsibilities

| Module | Responsibility |
|--------|---------------|
| **main.cpp** | `setup()` / `loop()` — window, menu, UI, BLE, hotkeys, tray icon init; `TraySubclassProc` for SC_MINIMIZE interception; autostart logic |
| **AppState** | All global variable definitions (`window`, `ble`, `stats`, `config`, `hotkeyMgr`, `overlayWindow`, `trayIcon`, `logWindow`), helpers (`logMsg()`, `sendCommand()`, `resetStats()`), shared actions (`doScanBLE()`, `doConnectBLE()`, `doRecordStart()`, `doToggleLogWindow()` etc.), INI load/save |
| **AppUI** | `createUI(SimpleWindow*)` — creates all UI components with dark theme; uses `styleBtn()` helper for compact button creation |
| **BLEHandler** | `setupBLE()` — BLE callback registration and adapter init; `handleBLEData()` — packet parsing, UI/overlay/stats/chart/CSV update |
| **MenuHandler** | `createAppMenu()` — MenuBar-based menu construction with lambda callbacks (uses JQB_WindowsLib `MenuBar` class) |
| **MeterOverlay** | `OverlayWindow` subclass from library — renders measurement value (Consolas) and mode/flags (Segoe UI) in OBS overlay |
| **OW18B_Commands** | Inline functions returning `std::vector<uint8_t>{button_id, press_type}` — 2-byte format |
| **OW18BParser** | Stateless 6-byte protocol parser — `OW18B::Parser::parse()`. MartMet/OW18B bitfield format |

## Tech Stack

- **Language**: C++17
- **Build system**: PlatformIO (`platform = native`, MinGW-w64)
- **UI framework**: [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) — lightweight Win32 UI library
- **Communication**: Windows BLE GATT API (via JQB_WindowsLib `IO/BLE/BLE.h`)
- **Target platform**: Windows 10+ (Bluetooth 4.0+ adapter required)

## Coding Conventions

### Code Style

- Comments and UI variable names in **Polish**
- Class names, methods, and parameters in **English** (namespace `OW18B`, class `Parser`, method `parse`)
- Use `std::wstring` and `L"..."` for displayed UI text (Polish characters and symbols: Ω, µ, °)
- Use section comments `// --- ... ---` for code organization in .cpp files
- Indentation: 4 spaces (no tabs)
- Explicit Wide WinAPI versions: `CreateFontW()`, `CreateWindowExW()`, `MessageBoxW()`, `LoadCursorW()` etc.
- Do not use `std::thread` (use `CreateThread`), `std::to_wstring` (use `jqb_compat::to_wstring` from Core.h)

### Application Pattern

- JQB_WindowsLib defines `setup()` and `loop()` — Arduino-like
- `setup()` → load settings → window + menu → UI → BLE → keyboard shortcuts
- `loop()` → main loop (empty — data handled by callbacks)
- UI components created via `new` and added to `SimpleWindow` via `window->add()`
- **Do not change** the `setup()` and `loop()` signatures — these are framework entry points

### Dark Theme & UI Styling

The app uses a consistent dark color palette defined in `createUI()`:

```cpp
colBtnBg     = RGB(50, 52, 62)      // standard button background
colBtnText   = RGB(200, 210, 225)   // standard button text
colBtnHover  = RGB(65, 68, 80)      // standard hover
colAccent    = RGB(40, 130, 200)    // accent (connect button)
colCtrl      = RGB(45, 90, 90)     // control buttons (SELECT etc.)
colCtrlText  = RGB(150, 240, 220)   // control text (teal)
colRecRed    = RGB(160, 40, 40)    // record button
colDim       = RGB(42, 44, 52)     // secondary buttons
colDimText   = RGB(160, 165, 175)   // secondary text
```

Helper function for button styling:
```cpp
static void styleBtn(SimpleWindow* win, Button* btn,
                     COLORREF bg, COLORREF text, COLORREF hover) {
    btn->setBackColor(bg); btn->setTextColor(text); btn->setHoverColor(hover);
    win->add(btn);
}
```

### Menu Bar

```
Plik → Nagrywaj CSV | Zatrzymaj nagrywanie | Zamknij
Połączenie → Skanuj BLE | Połącz | Rozłącz
Sterowanie → SELECT | HOLD | RANGE | Hz/DUTY | SELECT (długie) | HOLD (długie) | RANGE (długie) | Hz/DUTY (długie)
Widok → Okno OBS (overlay) | Okno logów
Ustawienia → Skróty klawiaturowe... | ✓ Wykres aktywny | ✓ Loguj dane RAW | ✓ Auto-łączenie | ✓ Minimalizuj do zasobnika | ✓ Autostart: zasobnik + overlay | Resetuj statystyki
Pomoc → O programie...
```

- Menu created in `MenuHandler::createAppMenu()` using `MenuBar` class from JQB_WindowsLib
- Each menu uses `menuBar->addMenu(label, lambda)` with `PopupMenu` API: `addItem()`, `addCheckItem()`, `addSeparator()`
- Checkbox items use `addCheckItem(label, boolRef, onChange)` — auto-synced state
- Settings saved to `owon_meter.ini` via `ConfigManager`

### Overlay Window (OBS Studio)

- Class `MeterOverlay` in `MeterOverlay.h/.cpp` — **subclass** of `OverlayWindow` from JQB_WindowsLib
- Opened from menu: View → OBS Overlay Window (`IDM_VIEW_OBS_OVERLAY = 9050`)
- Inherits from library: WinAPI window, always-on-top, double-buffered GDI, context menu (colors, pin), position persistence
- Overrides `onPaint()` — renders measurement value (Consolas bold) and mode/flags (Segoe UI)
- Custom methods: `updateValue()`, `setMode()`, `setFlags()`
- `enablePersistence(config, "overlay")` — auto-save/load position and colors to `owon_meter.ini`
- Data updated in `BLEHandler.cpp` after each received packet
- Base context menu IDs: 9100–9149 (from library), subclass: 9150+

### System Tray Icon

- Uses `TrayIcon` class from JQB_WindowsLib (`UI/TrayIcon/TrayIcon.h`)
- Created in `main.cpp` → `setup()` — `trayIcon->create(hwnd, 101, L"OW18B Meter")`
- Minimize to tray: `TraySubclassProc` intercepts `SC_MINIMIZE` → `ShowWindow(SW_HIDE)` + `trayIcon->show()`
- Restore: `trayIcon->onRestore()` → `ShowWindow(SW_SHOW)` + `SetForegroundWindow()`
- Behavior controlled by `minimizeToTray` setting (checkbox in Ustawienia menu)
- Tray icon always created but `show()` only called when minimizing with `minimizeToTray` enabled

### Log Window

- Uses `LogWindow` class from JQB_WindowsLib (`UI/LogWindow/LogWindow.h`)
- Created on first use in `doToggleLogWindow()` in `AppState.cpp`
- Styled: Consolas 14pt, dark theme (`RGB(22,22,28)` background, `RGB(170,180,195)` text)
- `logMsg()` writes to `logWindow->appendMessage()` instead of inline TextArea
- Position auto-saved via `enablePersistence(config, "logwin")`
- Toggle via menu: Widok → Okno logów

### Autostart Mode (Tray + Overlay)

- Setting `autoStartTray` (checkbox: Ustawienia → Autostart: zasobnik + overlay)
- When enabled: forces `minimizeToTray = true` and `overlayAutoOpen = true`
- On startup: app starts minimized to tray with overlay window open
- Use case: OBS streaming — meter overlay visible, main window hidden in tray

### Keyboard Shortcuts Dialog

- Opened from menu: Settings → Keyboard Shortcuts...
- Modal dialog (blocks main window)
- Displays shortcut list with binding buttons
- Click button → capture mode (`WH_KEYBOARD_LL` hook captures key combination)
- Escape cancels capture
- "Restore Defaults" button
- Bindings auto-saved to `owon_meter.ini` via `HotkeyManager`

### Configuration (owon_meter.ini)

File managed by `ConfigManager` (auto-load in constructor, auto-save in destructor).

| Key | Description | Default |
|-----|-------------|---------|
| `chart_enabled` | Whether chart is active | `1` |
| `log_raw_data` | Log RAW hex data | `0` |
| `shortcut_select` | Shortcut: SELECT | `Ctrl+Alt+1` |
| `shortcut_hold` | Shortcut: HOLD | `Ctrl+Alt+2` |
| `shortcut_range` | Shortcut: RANGE | `Ctrl+Alt+3` |
| `shortcut_hzduty` | Shortcut: Hz/DUTY | `Ctrl+Alt+4` |
| `shortcut_hold_long` | Shortcut: HOLD (long) | `Ctrl+Shift+Alt+2` |
| `overlay_x` | Overlay window X position | `100` |
| `overlay_y` | Overlay window Y position | `100` |
| `overlay_w` | Overlay window width | `420` |
| `overlay_h` | Overlay window height | `160` |
| `overlay_ontop` | Overlay always on top | `1` |
| `overlay_bg` | Overlay background color (COLORREF) | `0` (black) |
| `overlay_text` | Overlay text color (COLORREF) | `65280` (green) |
| `auto_reconnect` | Auto-reconnect to last device | `0` |
| `last_device_address` | Last connected BLE device address | (empty) |
| `last_device_name` | Last connected BLE device name | (empty) |
| `minimize_to_tray` | Minimize to system tray | `1` |
| `overlay_auto_open` | Auto-open overlay on startup | `0` |
| `start_minimized` | Start app minimized | `0` |
| `auto_start_tray` | Autostart: tray + overlay | `0` |
| `logwin_x` | Log window X position | (auto) |
| `logwin_y` | Log window Y position | (auto) |
| `logwin_w` | Log window width | (auto) |
| `logwin_h` | Log window height | (auto) |

### BLE Protocol OWON OW18B

Packet: **6 bytes** (bitfield format — source: [MartMet/OW18B](https://github.com/MartMet/OW18B))

| Byte(s) | Description |
|---------|-------------|
| 0-1 | Word0 (16-bit LE) — bitfields: bits 0-2 = Divisor, bits 3-5 = Prefix, bits 6-9 = Mode |
| 2 | Flags: bit 0 = HOLD, bit 1 = REL/DELTA, bit 2 = AUTO, bit 3 = LOW_BAT |
| 3 | Reserved |
| 4-5 | Value: signed int16_t little-endian |

Divisor (3 bits):
- 0-4: D1, D10, D100, D1000, D10000 (value divisor)
- 5: ERR, 6: Under Limit, 7: Over Limit

Prefix (3 bits):
- 0: pico, 1: nano, 2: µ, 3: milli, 4: none, 5: kilo, 6: Mega, 7: Giga

Mode (4 bits):
- 0: DC V, 1: AC V, 2: DC A, 3: AC A, 4: Ω, 5: F, 6: Hz, 7: %, 8: °C, 9: °F, 10: Diode, 11: Continuity, 12: hFE, 13: NCV

### Multimeter Control Commands (BLE write)

**Format: 2 bytes** `{button_id, press_type}` (source: [MartMet/OW18B](https://github.com/MartMet/OW18B))

```cpp
namespace OW18BCmd {
    // Button identifiers
    BTN_SELECT = 0x01;  BTN_RANGE = 0x02;  BTN_HOLD = 0x03;  BTN_HZDUTY = 0x04;
    // Press types
    PRESS_SHORT = 0x01;  PRESS_LONG = 0x00;

    // Inline functions returning std::vector<uint8_t>
    SELECT()      → { 0x01, 0x01 }
    SELECT_LONG() → { 0x01, 0x00 }
    HOLD()        → { 0x03, 0x01 }
    HOLD_LONG()   → { 0x03, 0x00 }
    RANGE()       → { 0x02, 0x01 }
    RANGE_LONG()  → { 0x02, 0x00 }
    HZ_DUTY()     → { 0x04, 0x01 }
    HZ_DUTY_LONG()→ { 0x04, 0x00 }
}
```

Sending: `ble.write(cmd)` (2 bytes). Wrapper: `sendCommand(const std::vector<uint8_t>& cmd)` in AppState.

### BLE UUIDs OWON OW18B

| Role | UUID |
|------|------|
| Service | `0000fff0-0000-1000-8000-00805f9b34fb` |
| Notify (data reception) | `0000fff4-0000-1000-8000-00805f9b34fb` |
| Write (commands) | `0000fff3-0000-1000-8000-00805f9b34fb` |

### Supported Measurement Modes

DC/AC: V, mV, µA, mA, A | Resistance Ω | Continuity | Diode | Capacitance F | Frequency Hz | Temperature °C/°F | Duty Cycle % | hFE | NCV

## Copilot Guidelines

### Adding New Measurement Modes
Add to enum `MeterMode` in `OW18BParser.h`, then to `getModeString()`, `getUnitString()` in `OW18BParser.cpp`. Prefix and Divisor are generic — no per-mode changes needed.

### Adding New UI Components
Create in `AppUI.cpp` → `createUI()`. Classes: `Label`, `Button`, `Select`, `TextArea`, `ProgressBar`, `Chart`, `ValueDisplay`, `CheckBox`. Add to `window` via `window->add()`. Use `styleBtn()` helper for consistent button styling. If a component needs to be accessed from other modules, declare it `extern` in `AppState.h` and define it in `AppState.cpp`.

### Adding New Menu Commands
1. In `createAppMenu()` in `MenuHandler.cpp`, add item via `m.addItem(label, lambda)` or `m.addCheckItem(label, boolRef, onChange)` inside the appropriate menu lambda
2. If command is shared with UI — create `doXxx()` function in `AppState.h/.cpp`
3. MenuBar class handles all ID management and routing — no manual `IDM_` defines needed

### Adding New Settings (INI)
1. Add `extern` variable in `AppState.h`, define in `AppState.cpp`
2. Load in `loadSettings()` and save in `saveSettings()` in `AppState.cpp`
3. Optionally add menu item in `MenuHandler.cpp` with `MF_CHECKED`

### Adding New Keyboard Shortcuts
Add `hotkeyMgr->addHotkey(iniKey, label, defaultBind, action)` in `main.cpp` → `setup()`. The shortcuts dialog will automatically display the new entry.

### BLE Callbacks
Register in `BLEHandler.cpp` → `setupBLE()` via `ble.onXxx()` (onDeviceDiscovered, onScanComplete, onConnect, onDisconnect, onReceive, onError). Configure UUIDs and priority filters **after** `ble.init()` but **before** `ble.connect()`.

### Extending the Overlay Window
- Class `MeterOverlay` in `MeterOverlay.h/.cpp` — subclass of `OverlayWindow` from JQB_WindowsLib
- Rendering in `onPaint(HDC memDC, const RECT& rc)` — double-buffered GDI, Consolas/Segoe UI fonts
- Extending context menu: override `onBuildContextMenu(HMENU)` + `onMenuCommand(int)`, IDs from `9150+`
- Data updated from `BLEHandler.cpp` — see block `// --- Update overlay window ---`
- Position/colors auto-saved via `enablePersistence(config, "overlay")`

### Second Window (beyond SimpleWindow)
SimpleWindow is a **singleton** (`s_instance`) — do not create a second one!
For additional windows use `OverlayWindow` from the library (subclass with `virtual onPaint()`)
or raw WinAPI pattern with `GWLP_USERDATA`:
1. `RegisterClassW()` with custom `WndProc`
2. `CreateWindowExW()` with `lpParam = this`
3. In `WM_NCCREATE` → `SetWindowLongPtrW(hwnd, GWLP_USERDATA, self)`
4. In other msgs → `GetWindowLongPtrW(hwnd, GWLP_USERDATA)` → cast to `self`

### Modal Dialogs
Pattern: register `WNDCLASSW`, create window with `CreateWindowExW()` using `WS_EX_DLGMODALFRAME`, block main window via `EnableWindow(parent, FALSE)`, unblock in `WM_DESTROY`. See: `HotkeyManager::showSettingsDialog()` in the library.

### Parser is Stateless
All `OW18B::Parser::*` methods are static. Test: `OW18B::Parser::parse(std::vector<uint8_t>{...})`.

### BLE Data Pipeline (fan-out)
One received packet updates multiple consumers in `handleBLEData()`:
1. `OW18B::Parser::parse()` — parsing
2. `valueDisplay->updateValue()` — LCD display
3. `overlayWindow->updateValue()` — OBS window
4. `stats.addSample()` — MIN/MAX/AVG/PEAK statistics
5. `chart->addDataPoint()` — real-time chart
6. `dataLogger.addRow()` — CSV recording
7. `logMsg()` — text log

### Logging
Use `logMsg(const wchar_t*)` or `logMsg(const std::wstring&)` from `AppState.h`. Logs are displayed in a separate `LogWindow` (standalone window from JQB_WindowsLib), not in the main window.
