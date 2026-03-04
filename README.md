# OWON OW18B — Multimetr Bluetooth (Windows)

Natywna aplikacja desktopowa Windows do komunikacji z multimetrem **OWON OW18B** przez Bluetooth Low Energy (BLE). Odbiera pomiary w czasie rzeczywistym, wyświetla je w GUI z wykresem i logowaniem danych.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![Windows 10+](https://img.shields.io/badge/Windows-10%2B-0078D6)
![PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange)
![BLE](https://img.shields.io/badge/BLE-4.0%2B-blueviolet)

---

## Funkcje

- **Skanowanie BLE** — automatyczne wykrywanie multimetrów OWON OW18B w zasięgu
- **Wyświetlacz LCD** — duży, czytelny wyświetlacz wartości w stylu multimetru (Consolas, zielony na czarnym tle)
- **Wykres czasu rzeczywistego** — 60-sekundowe okno z autoskalowaniem i odświeżaniem 10 FPS
- **Min/Max** — ciągłe śledzenie wartości minimalnej i maksymalnej
- **Tryby pomiaru** — pełna obsługa wszystkich trybów OW18B:

| Kategoria | Tryby |
|-----------|-------|
| Napięcie | DC V, AC V, DC mV, AC mV |
| Prąd | DC µA, AC µA, DC mA, AC mA, DC A, AC A |
| Rezystancja | Ω (z prefiksami k/M) |
| Inne | Ciągłość, Dioda, Pojemność (nF/µF/mF), Częstotliwość (Hz/kHz/MHz) |
| Specjalne | Temperatura °C/°F, Duty Cycle %, hFE, NCV |

- **Flagi** — obsługa AUTO, HOLD, DELTA, Over Limit (OL)
- **Logowanie** — log pomiarów z opcjonalnym widokiem surowych danych hex

---

## Wymagania

| Wymaganie | Minimum |
|-----------|---------|
| System operacyjny | Windows 10 (build 1809+) |
| Adapter Bluetooth | BLE 4.0+ |
| Build system | [PlatformIO](https://platformio.org/) (CLI lub wtyczka VS Code) |
| Kompilator | MSVC / MinGW z obsługą C++17 |

---

## Instalacja i budowanie

### 1. Klonowanie repozytorium

```bash
git clone https://github.com/<twoj-user>/OWON_OW18B.git
cd OWON_OW18B
```

### 2. Budowanie z PlatformIO

```bash
# PlatformIO CLI
pio run -e app

# Lub w VS Code z wtyczką PlatformIO — kliknij "Build"
```

Zależność [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) zostanie pobrana automatycznie przez PlatformIO.

### 3. Uruchomienie

```bash
pio run -e app -t exec

# Lub uruchom bezpośrednio plik wykonywalny:
.pio\build\app\program.exe
```

---

## Użytkowanie

1. **Włącz multimetr** OWON OW18B i upewnij się, że Bluetooth jest aktywny
2. **Uruchom aplikację** — pojawi się okno „OWON OW18B — Multimetr BLE"
3. **Kliknij „Skanuj BLE"** — aplikacja szuka urządzeń BLE przez 10 sekund
4. **Wybierz urządzenie** z listy rozwijanej
5. **Kliknij „Połącz"** — dane pomiarowe pojawią się na wyświetlaczu i wykresie
6. **Kliknij „Rozłącz"** aby zakończyć sesję

### Opcje

- **Wykres aktywny** — włącz/wyłącz zapis na wykresie
- **Loguj dane RAW (hex)** — pokaż surowe 6-bajtowe pakiety BLE w logu
- **Reset Min/Max** — wyzeruj śledzenie wartości ekstremalnych
- **Wyczyść wykres / log** — usuń historyczne dane

---

## Architektura projektu

```
OWON_OW18B/
├── .github/
│   └── copilot-instructions.md   # Instrukcje dla GitHub Copilot
├── include/
│   └── OW18BParser.h             # Nagłówek parsera protokołu OW18B
├── src/
│   ├── main.cpp                  # GUI (SimpleWindow), logika BLE, punkt wejścia
│   └── OW18BParser.cpp           # Implementacja parsera protokołu BLE
├── platformio.ini                # Konfiguracja PlatformIO (native, JQB_WindowsLib)
├── app.manifest                  # Manifest Windows Common Controls v6
├── resources.rc                  # Plik zasobów (osadzenie manifestu)
└── README.md                     # Ten plik
```

### Moduły

| Moduł | Opis |
|-------|------|
| **OW18BParser** | Bezstanowy parser 6-bajtowego protokołu BLE. Dekoduje tryb pomiaru, skalę, flagi, wartość 16-bit i zwraca strukturę `Measurement`. |
| **main.cpp (GUI)** | Okno Win32 z komponentami JQB_WindowsLib: ValueDisplay (LCD), Chart (wykres), TextArea (log), Select (lista urządzeń), przyciski i checkboxy. |
| **BLE** | Komunikacja przez `BLE` z JQB_WindowsLib. Callbacki: `onDeviceDiscovered`, `onConnect`, `onDisconnect`, `onReceive`, `onError`. |

---

## Protokół BLE OWON OW18B

Multimetr wysyła **6-bajtowe pakiety** przez BLE Notify:

```
Bajt:  [0]        [1]      [2]       [3]       [4]       [5]
       Funkcja    Skala    Flagi1    Flagi2    Val_Lo    Val_Hi
```

### Dekodowanie wartości (bajty 4–5)

```
Jeśli data[5] < 128:  wartość =  data[5] * 256 + data[4]
Jeśli data[5] >= 128: wartość = -((data[5] - 128) * 256 + data[4])
Over Limit:           rawValue >= 32767 (0x7FFF)
```

### Flagi (bajt 2)

| Bit | Flaga |
|-----|-------|
| 0   | AUTO  |
| 1   | HOLD  |
| 2   | DELTA |

### Kody funkcji (bajt 0)

| Kod | Tryb | Kod | Tryb |
|-----|------|-----|------|
| `0x11` | DC µA | `0x22` | DC V |
| `0x12` | DC mA | `0x23` | Rezystancja Ω |
| `0x13` | DC A | `0x24` | Ciągłość |
| `0x15` | AC µA | `0x25` | Dioda |
| `0x16` | AC mA | `0x26` | Pojemność F |
| `0x17` | AC A | `0x27` | Częstotliwość Hz |
| `0x19` | DC mV | `0x28` | Temperatura °C |
| `0x1A` | AC mV | `0x29` | Temperatura °F |
| `0x21` | AC V | `0x2A` | Duty Cycle % |
| | | `0x2D` | hFE |
| | | `0x30` | NCV |

---

## Zależności

- [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) — lekka biblioteka Win32 UI (automatycznie pobierana przez PlatformIO)
   - Komponenty UI: `SimpleWindow`, `Label`, `Button`, `Select`, `TextArea`, `ProgressBar`, `Chart`, `ValueDisplay`, `CheckBox`
   - BLE: `IO/BLE/BLE.h`
   - Narzędzia: `Util/StringUtils.h`

---

## Rozwijanie projektu

### Dodawanie nowego trybu pomiaru

1. Dodaj stałą do `enum class MeasurementMode` w [include/OW18BParser.h](include/OW18BParser.h)
2. Uzupełnij metody w [src/OW18BParser.cpp](src/OW18BParser.cpp):
   - `getModeString()` — nazwa trybu (wide string)
   - `getUnitString()` — jednostka
   - `getDivisor()` — dzielnik wartości dla każdej skali
   - `getPrecision()` — liczba miejsc po przecinku
   - `parse()` — logika prefiksu (k/M/m/µ/n) jeśli potrzebna

### Dodawanie komponentów UI

Twórz obiekty klas z JQB_WindowsLib i dodawaj do okna:

```cpp
auto* btn = new Button(x, y, w, h, "Etykieta", [](Button*) {
    // callback kliknięcia
});
window->add(btn);
```

### Konwencje

- **Polski** — komentarze, teksty UI, nazwy zmiennych UI
- **Angielski** — nazwy klas, metod, parametrów
- `std::wstring` / `L"..."` — wszystkie teksty wyświetlane (Unicode: Ω, µ, °)
- Wcięcia: 4 spacje
- Sekcje kodu oddzielane komentarzami `// ===...===`

---

## Licencja

Ten projekt jest udostępniony na licencji MIT. Szczegóły w pliku [LICENSE](LICENSE).

---

## Autor

Projekt oparty na bibliotece [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) autorstwa [JAQUBA](https://github.com/JAQUBA).
