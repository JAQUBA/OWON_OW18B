# Copilot Instructions — OWON OW18B

## Opis projektu

Natywna aplikacja desktopowa Windows (C++) do komunikacji z multimetrem **OWON OW18B** przez Bluetooth Low Energy (BLE). Aplikacja odbiera 6-bajtowe pakiety pomiarowe, parsuje je i wyświetla wyniki w GUI z wykresem czasu rzeczywistego.

## Architektura

```
OWON_OW18B/
├── include/
│   └── OW18BParser.h        # Nagłówek parsera protokołu OW18B (enum, struct, klasa)
├── src/
│   ├── main.cpp              # Punkt wejścia, GUI (SimpleWindow), logika BLE
│   └── OW18BParser.cpp       # Implementacja parsera protokołu BLE
├── platformio.ini            # Konfiguracja PlatformIO (platforma: native)
├── app.manifest              # Manifest Windows Common Controls v6
└── resources.rc              # Plik zasobów Windows (osadzenie manifestu)
```

## Stack technologiczny

- **Język**: C++17
- **Build system**: PlatformIO (`platform = native`)
- **UI framework**: [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) — lekka biblioteka Win32 UI
- **Komunikacja**: Windows BLE API (poprzez JQB_WindowsLib `IO/BLE/BLE.h`)
- **Platforma docelowa**: Windows 10+ (wymagany adapter Bluetooth 4.0+)

## Konwencje kodowania

### Styl kodu

- Komentarze i nazwy zmiennych UI pisz **po polsku**
- Nazwy klas, metod i parametrów — **po angielsku** (namespace `OW18B`, klasa `Parser`, metoda `parse`)
- Stosuj `std::wstring` i `L"..."` dla tekstów wyświetlanych w UI (obsługa polskich znaków i symboli: Ω, µ, °)
- Używaj sekcji z komentarzami `// ===...===` do organizacji kodu
- Wcięcia: 4 spacje (nie taby)

### Wzorzec aplikacji

- Biblioteka JQB_WindowsLib definiuje `setup()` i `loop()` — analogicznie do Arduino
- `setup()` → inicjalizacja okna, komponentów UI, konfiguracja callbacków BLE
- `loop()` → pętla główna (w tym projekcie pusta — dane obsługiwane przez callbacki)
- Komponenty UI tworzone przez `new` i dodawane do `SimpleWindow` przez `window->add()`

### Protokół BLE OWON OW18B

Pakiet: **6 bajtów**

| Bajt | Opis |
|------|------|
| 0    | Kod funkcji (tryb pomiaru) — enum `MeasurementMode` |
| 1    | Skala / zakres (pozycja kropki dziesiętnej) |
| 2    | Flagi 1 (bit 0: AUTO, bit 1: HOLD, bit 2: DELTA) |
| 3    | Flagi 2 (zarezerwowane) |
| 4-5  | Wartość 16-bit little-endian z kodowaniem znaku w bajcie 5 |

Wartość:
- `data[5] < 128` → dodatnia: `data[5] * 256 + data[4]`
- `data[5] >= 128` → ujemna: `-((data[5] - 128) * 256 + data[4])`
- Over Limit: `rawValue >= 32767` lub `0x7FFF`

### Obsługiwane tryby pomiaru

DC/AC: V, mV, µA, mA, A | Rezystancja Ω | Ciągłość | Dioda | Pojemność F | Częstotliwość Hz | Temperatura °C/°F | Duty Cycle % | hFE | NCV

## Wytyczne dla Copilota

1. **Nowe tryby pomiaru** — dodaj do enum `MeasurementMode`, metod `getModeString()`, `getUnitString()`, `getDivisor()`, `getPrecision()` oraz logikę prefiksu w `parse()`
2. **Nowe komponenty UI** — twórz za pomocą klas z JQB_WindowsLib (`Label`, `Button`, `Select`, `TextArea`, `ProgressBar`, `Chart`, `ValueDisplay`, `CheckBox`), dodawaj do `window`
3. **Callbacki BLE** — rejestruj przez `ble.onXxx()` (onDeviceDiscovered, onScanComplete, onConnect, onDisconnect, onReceive, onError)
4. **Nie zmieniaj sygnatury `setup()` i `loop()`** — to punkty wejścia frameworka
5. **Parser jest bezstanowy** — wszystkie metody `Parser::*` są statyczne
6. **Testowanie parsera** — twórz `std::vector<uint8_t>` z 6 bajtami i wywołuj `Parser::parse()`
7. **Logowanie** — używaj `logMsg()` do logowania w `TextArea`
