#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <EEPROM.h>
#include <math.h>

#define PIN       6
#define NUMPIXELS 7
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_PIN 2
#define HOLD_TIME         3000UL
#define DOUBLE_TAP_WINDOW  400UL
#define DEBOUNCE_DELAY      50UL
#define EEPROM_PRESET_ADDR    0

// --- Button state ---
bool rawLastReading   = HIGH;
unsigned long debounceTimer = 0;
bool debouncedState   = HIGH;
bool lastDebouncedState = HIGH;
unsigned long buttonPressTime = 0;
unsigned long lastReleaseTime = 0;
int  tapCount = 0;
bool waitingForDoubleTap = false;

// --- Presets ---
const int presets[]  = {10, 30, 45, 50, 60, 90};
const int numPresets = sizeof(presets) / sizeof(presets[0]);
int currentPreset = 0;

// --- Timer state ---
enum State { MENU, RUNNING, PAUSED, BREAK, DONE };
State state = MENU;

unsigned long totalTime      = 0;
unsigned long startTime      = 0;
unsigned long pausedRemaining = 0;

const unsigned long BREAK_DURATION = 5UL * 60UL * 1000UL;

// --- Display ---
int lastDisplayedSecond = -1;

// --- Breathing effect ---
float breathPhase = 0.0f;
unsigned long lastBreathUpdate = 0;

// --- Done flash ---
unsigned long lastFlashTime = 0;
bool flashOn = false;

// ------------------------------------------------------------------ helpers

void setAllColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUMPIXELS; i++)
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  pixels.show();
}

// Progress ring with optional breathing brightness.
// fraction: 1.0 = full time left, 0.0 = time up
void updateProgressRing(float fraction, uint8_t r, uint8_t g, uint8_t b, bool breathe) {
  unsigned long now = millis();
  uint8_t bright;

  if (breathe) {
    breathPhase += 0.002f * (float)(now - lastBreathUpdate);
    if (breathPhase > 2.0f * PI) breathPhase -= 2.0f * PI;
    float bf = 0.5f + 0.5f * sinf(breathPhase);
    bright = (uint8_t)(40 + 90 * bf); // 40-130
  } else {
    bright = 255;
  }
  lastBreathUpdate = now;

  int litLEDs = (int)ceilf(fraction * NUMPIXELS);
  litLEDs = constrain(litLEDs, 0, NUMPIXELS);

  for (int i = 0; i < NUMPIXELS; i++) {
    if (i < litLEDs)
      pixels.setPixelColor(i, pixels.Color(
        (uint8_t)(r * bright / 255),
        (uint8_t)(g * bright / 255),
        (uint8_t)(b * bright / 255)));
    else
      pixels.setPixelColor(i, 0);
  }
  pixels.show();
}

void doneAnimation() {
  // Chase pattern
  for (int rep = 0; rep < 3; rep++) {
    for (int i = 0; i < NUMPIXELS; i++) {
      setAllColor(0, 0, 0);
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      pixels.show();
      delay(60);
    }
  }
  // Flash
  for (int f = 0; f < 4; f++) {
    setAllColor(255, 0, 0);
    delay(150);
    setAllColor(0, 0, 0);
    delay(150);
  }
}

// ------------------------------------------------------------------ display

void showMenu() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Select:");

  display.setTextSize(2);
  display.setCursor(0, 25);
  display.print(presets[currentPreset]);
  display.println(" min");

  display.display();
}

// remaining in ms, total in ms
void showTime(unsigned long remaining, unsigned long total, bool paused) {
  int secs = (int)(remaining / 1000);
  int minutes = secs / 60;
  int seconds = secs % 60;

  if (seconds == lastDisplayedSecond && !paused) return;
  lastDisplayedSecond = seconds;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Countdown — large
  display.setTextSize(3);
  display.setCursor(10, 20);
  if (minutes < 10) display.print("0");
  display.print(minutes);
  display.print(":");
  if (seconds < 10) display.print("0");
  display.print(seconds);

  // Progress bar
  float fraction = (total > 0) ? (float)remaining / (float)total : 0.0f;
  int barWidth = (int)(114.0f * fraction);
  display.drawRect(7, 48, 114, 8, SSD1306_WHITE);
  if (barWidth > 0)
    display.fillRect(7, 48, barWidth, 8, SSD1306_WHITE);

  if (paused) {
    display.setTextSize(1);
    display.setCursor(43, 57);
    display.print("PAUSED");
  }

  display.display();
}

void showDone() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(16, 20);
  display.println("DONE");
  display.display();
}

void showBreak(unsigned long remaining) {
  int secs = (int)(remaining / 1000);
  int minutes = secs / 60;
  int seconds = secs % 60;

  if (seconds == lastDisplayedSecond) return;
  lastDisplayedSecond = seconds;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(18, 0);
  display.println("BREAK!");

  display.setTextSize(2);
  display.setCursor(25, 22);
  if (minutes < 10) display.print("0");
  display.print(minutes);
  display.print(":");
  if (seconds < 10) display.print("0");
  display.print(seconds);

  float fraction = (float)remaining / (float)BREAK_DURATION;
  int barWidth = (int)(114.0f * fraction);
  display.drawRect(7, 48, 114, 8, SSD1306_WHITE);
  if (barWidth > 0)
    display.fillRect(7, 48, barWidth, 8, SSD1306_WHITE);

  display.display();
}

// ------------------------------------------------------------------ actions

void startTimer() {
  totalTime       = (unsigned long)presets[currentPreset] * 60UL * 1000UL;
  startTime       = millis();
  pausedRemaining = 0;
  lastDisplayedSecond = -1;
  breathPhase     = 0.0f;
  lastBreathUpdate = millis();
  state = RUNNING;
}

void startBreak() {
  totalTime       = BREAK_DURATION;
  startTime       = millis();
  pausedRemaining = 0;
  lastDisplayedSecond = -1;
  breathPhase     = 0.0f;
  lastBreathUpdate = millis();
  state = BREAK;
}

void returnToMenu() {
  state = MENU;
  setAllColor(0, 255, 0);
  showMenu();
}

// ------------------------------------------------------------------ setup / loop

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pixels.begin();
  pixels.setBrightness(64);
  setAllColor(0, 255, 0);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for (;;);

  uint8_t saved = EEPROM.read(EEPROM_PRESET_ADDR);
  if (saved < numPresets) currentPreset = saved;

  lastBreathUpdate = millis();
  showMenu();
}

void loop() {
  unsigned long now = millis();
  bool rawReading = digitalRead(BUTTON_PIN);

  // --- Debounce ---
  if (rawReading != rawLastReading) {
    debounceTimer = now;
    rawLastReading = rawReading;
  }
  if ((now - debounceTimer) >= DEBOUNCE_DELAY) {
    debouncedState = rawReading;
  }

  // --- Button edge detection on debounced signal ---
  if (lastDebouncedState == HIGH && debouncedState == LOW) {
    buttonPressTime = now;
  }

  if (lastDebouncedState == LOW && debouncedState == HIGH) {
    unsigned long pressDuration = now - buttonPressTime;

    if (pressDuration >= HOLD_TIME) {
      if (state == MENU) {
        startTimer();
      } else {
        returnToMenu();
      }
      tapCount = 0;
      waitingForDoubleTap = false;
    } else {
      tapCount++;
      lastReleaseTime = now;
      waitingForDoubleTap = true;
    }
  }

  lastDebouncedState = debouncedState;

  // --- Resolve taps after double-tap window ---
  if (waitingForDoubleTap && (now - lastReleaseTime) > DOUBLE_TAP_WINDOW) {
    if (tapCount == 1) {
      if (state == MENU) {
        // Forward through presets
        currentPreset = (currentPreset + 1) % numPresets;
        EEPROM.update(EEPROM_PRESET_ADDR, currentPreset);
        showMenu();
      } else if (state == RUNNING) {
        // Pause
        pausedRemaining = totalTime - (now - startTime);
        lastDisplayedSecond = -1;
        state = PAUSED;
        setAllColor(255, 165, 0);
      } else if (state == PAUSED) {
        // Resume
        startTime = now - (totalTime - pausedRemaining);
        lastDisplayedSecond = -1;
        state = RUNNING;
      } else if (state == DONE) {
        startBreak();
      } else if (state == BREAK) {
        // Skip break
        returnToMenu();
      }
    } else if (tapCount >= 2) {
      if (state == MENU) {
        // Double-tap goes backwards through presets
        currentPreset = (currentPreset + numPresets - 1) % numPresets;
        EEPROM.update(EEPROM_PRESET_ADDR, currentPreset);
        showMenu();
      }
    }
    tapCount = 0;
    waitingForDoubleTap = false;
  }

  // --- State machine ---
  if (state == RUNNING) {
    unsigned long elapsed   = now - startTime;
    unsigned long remaining = (elapsed < totalTime) ? (totalTime - elapsed) : 0;
    float fraction = (float)remaining / (float)totalTime;

    uint8_t r, g, b;
    bool breathe;
    if (remaining <= 5UL * 60UL * 1000UL) {
      r = 255; g = 0;   b = 0; breathe = false; // red — last 5 min
    } else if (fraction > 0.5f) {
      r = 0;   g = 255; b = 0; breathe = true;  // green
    } else {
      r = 255; g = 100; b = 0; breathe = false; // orange
    }

    updateProgressRing(fraction, r, g, b, breathe);
    showTime(remaining, totalTime, false);

    if (elapsed >= totalTime) {
      doneAnimation();
      showDone();
      lastFlashTime = millis();
      flashOn = true;
      state = DONE;
    }

  } else if (state == PAUSED) {
    showTime(pausedRemaining, totalTime, true);

  } else if (state == DONE) {
    if (now - lastFlashTime >= 2000UL) {
      lastFlashTime = now;
      flashOn = !flashOn;
      flashOn ? setAllColor(255, 0, 0) : setAllColor(0, 0, 0);
    }

  } else if (state == BREAK) {
    unsigned long elapsed   = now - startTime;
    unsigned long remaining = (elapsed < BREAK_DURATION) ? (BREAK_DURATION - elapsed) : 0;
    float fraction = (float)remaining / (float)BREAK_DURATION;

    updateProgressRing(fraction, 0, 0, 255, true); // blue progress ring for break
    showBreak(remaining);

    if (elapsed >= BREAK_DURATION) {
      returnToMenu();
    }
  }
}
