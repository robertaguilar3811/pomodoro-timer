# Pomodoro Timer

A desk productivity timer built from scratch — custom hardware, embedded firmware, and a single-button interface that controls everything.

<p align="center">
  <img src="POMODORO TIMER.JPG" alt="CAD Model" width="550"/>
</p>

---

## Hardware

| Component | Part |
|-----------|------|
| Microcontroller | Arduino Uno R3 |
| Breakout Board | GeeekPi Screw Terminal Hat for Arduino UNO |
| Display | Hosyond 0.96" SSD1306 128×64 OLED (I2C, White) |
| LEDs | WS2812B RGB LED Ring — 7× addressable NeoPixels |
| Power | NightShade Electronics energyShield 2 Basic (rechargeable battery shield) |
| Input | Tactile push button |

---

## How It Works

Set a session length (15 – 90 min) and hold the button to start. The LED ring drains one pixel at a time as the session progresses, shifting **green → orange → red** as time runs low. When the session ends the LEDs flash and a 5-minute break timer starts automatically.

**Everything is controlled with one button:**

| Gesture | Action |
|---------|--------|
| Tap | Cycle preset / Pause / Resume |
| Double tap | Cycle preset backward |
| Hold 3s | Start / Cancel |

---

## Firmware Highlights

- Finite state machine — `MENU → RUNNING → DONE → BREAK`
- Hardware debounce + double-tap detection, non-blocking
- OLED redraws only on second change — no flicker
- Last preset saved to EEPROM across power cycles

---

## Stack

`C++` · `Arduino` · `I2C` · `NeoPixel` · `EEPROM` · `CAD`

---

## Dependencies

- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
