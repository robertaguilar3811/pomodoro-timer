#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define PIN        6
#define NUMPIXELS  7
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_PIN 2
const unsigned long HOLD_TIME = 3000; // 3s long press
bool lastButtonState = HIGH;
unsigned long buttonPressTime = 0;

const int presets[] = {15,30,45,50,60,90};
const int numPresets = sizeof(presets)/sizeof(presets[0]);
int currentPreset = 0;

unsigned long totalTime = presets[currentPreset] * 60UL * 1000UL;
unsigned long startTime = 0;
bool timerRunning = false;

void setColor(uint8_t r, uint8_t g, uint8_t b){
  for(int i=0;i<NUMPIXELS;i++){
    pixels.setPixelColor(i,pixels.Color(r,g,b));
  }
  pixels.show();
}

void showMenu(){
  display.clearDisplay();
  display.setTextSize(2);          // Same size as countdown
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Select Time:");

  display.setCursor(0,30);
  display.print(presets[currentPreset]);
  display.println(" min");
  display.display();
}

void showTime(unsigned long remaining){
  int seconds = remaining/1000;
  int minutes = seconds/60;
  seconds %= 60;

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10,20);

  if(minutes<10) display.print("0");
  display.print(minutes);
  display.print(":");
  if(seconds<10) display.print("0");
  display.print(seconds);

  display.display();
}

void startTimer(){
  totalTime = presets[currentPreset]*60UL*1000UL;
  startTime = millis();
  timerRunning = true;
}

void setup(){
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pixels.begin();
  pixels.setBrightness(64);
  setColor(0,255,0);

  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C)) for(;;);

  showMenu();
}

void loop(){
  int reading = digitalRead(BUTTON_PIN);

  // --- Button press/release handling ---
  if(lastButtonState == HIGH && reading == LOW){
    // Button just pressed
    buttonPressTime = millis();
  }

  if(lastButtonState == LOW && reading == HIGH){
    // Button just released
    unsigned long pressDuration = millis() - buttonPressTime;

    if(pressDuration >= HOLD_TIME){
      // Long press → start timer
      startTimer();
    } else {
      // Short press → cycle preset if timer not running
      if(!timerRunning){
        currentPreset = (currentPreset+1)%numPresets;
        showMenu();
      }
    }
  }

  lastButtonState = reading;

  // --- Timer logic ---
  if(timerRunning){
    unsigned long elapsed = millis() - startTime;
    unsigned long remaining = (elapsed<totalTime)?(totalTime-elapsed):0;

    if(remaining<=totalTime/4) setColor(255,0,0);
    else if(remaining<=totalTime/2) setColor(255,100,0);
    else setColor(0,255,0);

    showTime(remaining);

    if(elapsed>=totalTime){
      timerRunning = false;
      setColor(255,0,0);
      showTime(0);
      showMenu(); // Show menu again after timer ends
    }
  }
}
