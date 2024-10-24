#include <Wire.h>
#include <Adafruit_GFX.h>
#define SSD1306_NO_SPLASH
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>
#include "esp_sleep.h"
#include "driver/gpio.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 3
#define SCL_PIN 1
#define BUTTON_PIN 2
#define WAKE_UP_BUTTON_GPIO 0
#define WAKE_UP_BUTTON_GPIO_BITMASK 0b000001
#define BATTERY_PIN 0 // ADC pin for battery voltage
#define CALIBRATION_FACTOR 0.95 // Коэффициент калибровки (уменьшаем значение на 5%)
#define DEBOUNCE_TIME 100
#define WAKEUP_INTERVAL 15000000 // 15 секунд в микросекундах

const float R1 = 100000.0; // 100kΩ
const float R2 = 100000.0; // 100kΩ
const float correctionFactor = (R1 + R2) / R2; // Корректирующий коэффициент

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_AHTX0 aht;

enum DisplayMode { SHOW_TEMPERATURE, SHOW_HUMIDITY, SHOW_BOTH, SHOW_BATTERY };
DisplayMode currentMode = SHOW_TEMPERATURE;
bool lastButtonState = HIGH;
volatile bool buttonPressed = false;

void IRAM_ATTR handleButtonPress() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }

  display.clearDisplay();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(WAKE_UP_BUTTON_GPIO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WAKE_UP_BUTTON_GPIO), handleButtonPress, FALLING);

  esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
  if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woken up from deep sleep by GPIO");
  } else if (wakeup_cause == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Woken up from deep sleep by timer");
  } else {
    Serial.println("Power on or reset");
  }
}

void drawBatteryIcon(int level) {
  display.drawRect(10, 10, 35, 20, SSD1306_WHITE);
  display.drawRect(45, 15, 5, 10, SSD1306_WHITE);

  for (int i = 0; i < level; i++) {
    display.fillRect(12 + i * 6, 12, 5, 16, SSD1306_WHITE);
  }
}

void enterDeepSleep(uint64_t sleepDurationUs) {
  Serial.println("Going to deep sleep...");
  esp_deep_sleep_enable_gpio_wakeup(WAKE_UP_BUTTON_GPIO_BITMASK, ESP_GPIO_WAKEUP_GPIO_HIGH);
  esp_sleep_enable_timer_wakeup(sleepDurationUs);
  esp_deep_sleep_start();
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    currentMode = static_cast<DisplayMode>((currentMode + 1) % 4);
    delay(200);
  }
  lastButtonState = buttonState;

  if (buttonPressed) {
    buttonPressed = false;
    delay(DEBOUNCE_TIME);

    display.ssd1306_command(SSD1306_DISPLAYON);
    Serial.println("Display On");

    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    int analogVolts = analogReadMilliVolts(BATTERY_PIN);
    float batteryVoltage = analogVolts * correctionFactor / 1000;
    float batteryLevel = map(batteryVoltage * 1000, 3000, 4200, 0, 100);
    batteryLevel = constrain(batteryLevel, 0, 100);
    float batteryIconLevel = map(batteryVoltage * 1000, 3000, 4200, 0, 5);
    batteryIconLevel = constrain(batteryIconLevel, 0, 5);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    switch (currentMode) {
      case SHOW_TEMPERATURE:
        display.print("Temp: ");
        display.print(temp.temperature);
        display.println(" C");
        break;
      case SHOW_HUMIDITY:
        display.print("Humidity: ");
        display.print(humidity.relative_humidity);
        display.println(" %");
        break;
      case SHOW_BOTH:
        display.print("Temp: ");
        display.print(temp.temperature);
        display.println(" C");
        display.print("Humidity: ");
        display.print(humidity.relative_humidity);
        display.println(" %");
        break;
      case SHOW_BATTERY:
        drawBatteryIcon(batteryIconLevel);
        break;
    }

    display.setCursor(0, 50);
    display.print(batteryLevel);
    display.println(" %");
    display.display();

    delay(10000); // Активное время после пробуждения перед переходом в сон
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  } else {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    Serial.print("Temp: ");
    Serial.print(temp.temperature);
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(humidity.relative_humidity);
    Serial.println(" %");
    enterDeepSleep(WAKEUP_INTERVAL);
  }
}
