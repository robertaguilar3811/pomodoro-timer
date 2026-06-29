# Pomodoro Timer

A self-contained productivity timer built on Arduino Uno, featuring a 7-LED NeoPixel progress ring, SSD1306 OLED display, and single-button interface. Designed to be placed on a desk and operated without looking away from your work.

<p align="center">
  <img src="POMODORO TIMER.JPG" alt="CAD Model" width="300"/>
</p>

<p align="center">
  <!-- Add real photo here -->
</p>

---

## Features

- **6 time presets** — 15, 30, 45, 50, 60, and 90 minutes
- **LED progress ring** — 7 NeoPixels drain one by one as time passes; color shifts green → orange → red as urgency increases
- **OLED countdown** — large MM:SS display with a shrinking progress bar
- **Automatic break timer** — 5-minute break starts after each session with a blue LED ring
- **Flashing done alert** — LEDs flash red at 1 Hz when the session ends
- **Pause / resume** — single tap while running pauses the timer mid-session
- **Persistent preset** — last selected duration saved to EEPROM, survives power cycles

---

## Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | Arduino Uno |
| Display | SSD1306 128×64 OLED (I2C) |
| LEDs | Adafruit NeoPixel strip — 7 pixels, pin 6 |
| Input | Tactile push button with internal pull-up, pin 2 |

### Wiring

| Arduino Pin | Connected To |
|-------------|-------------|
| 5V / GND | OLED VCC / GND |
| A4 (SDA) | OLED SDA |
| A5 (SCL) | OLED SCL |
| 6 | NeoPixel data in |
| 2 | Button (other leg to GND) |

---

## Button Controls

The entire device is controlled with a single button using three gesture types:

| Gesture | Context | Action |
|---------|---------|--------|
| Single tap | Menu | Cycle preset forward |
| Double tap | Menu | Cycle preset backward |
| Hold 3s | Menu | Start timer |
| Single tap | Running | Pause |
| Single tap | Paused | Resume |
| Single tap | Done alert | Dismiss and start break |
| Single tap | Break | Skip break |
| Hold 3s | Running / Paused / Break | Cancel and return to menu |

---

## LED Behavior

| Color | Condition |
|-------|-----------|
| Green (breathing) | > 50% time remaining |
| Orange (solid) | 25 – 50% remaining |
| Red (solid) | < 25% remaining |
| Blue (breathing) | Break in progress |
| Red (flashing 1 Hz) | Session complete |

---

## Software Design

**State machine** — the firmware runs a clean `MENU → RUNNING → DONE → BREAK → MENU` loop with `PAUSED` as a side state off `RUNNING`. Each state owns its display output and LED behavior, making it straightforward to add new states.

**Debouncing** — a 50 ms stability window on the raw GPIO reading eliminates contact bounce before any edge is processed, preventing phantom double-taps.

**Double-tap detection** — a 400 ms window after the first release accumulates tap count before resolving intent, distinguishing single tap, double tap, and hold without blocking the main loop.

**Non-blocking rendering** — the OLED only redraws when the displayed second changes, eliminating flicker and keeping the loop responsive.

**EEPROM wear leveling** — `EEPROM.update()` only writes when the value actually changes, extending flash lifetime.

---

## Dependencies

Install via Arduino Library Manager:

- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)

---

## Build & Flash

1. Open `Code/Arduino/pomodoro-timer/pomodoro-timer.ino` in the Arduino IDE
2. Select **Board:** Arduino Uno and the correct **Port**
3. Click **Upload**

---

## Project Structure

```
pomodoro-timer/
├── Code/
│   └── Arduino/
│       └── pomodoro-timer/
│           └── pomodoro-timer.ino
├── CAD/               # Enclosure files
└── README.md
```
