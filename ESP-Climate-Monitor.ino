#include <Wire.h>
#include <Adafruit_GFX.h>
#define SSD1306_NO_SPLASH
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 3
#define SCL_PIN 1
#define BUTTON_PIN 2
#define BATTERY_PIN 0 // ADC pin for battery voltage
#define CALIBRATION_FACTOR 0.95 // Коэффициент калибровки (уменьшаем значение на 5%)
const float R1 = 100000.0; // 100kΩ
const float R2 = 100000.0; // 100kΩ
const float correctionFactor = (R1 + R2) / R2; // Корректирующий коэффициент

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_AHTX0 aht;

enum DisplayMode { SHOW_TEMPERATURE, SHOW_HUMIDITY, SHOW_BOTH, SHOW_BATTERY };
DisplayMode currentMode = SHOW_TEMPERATURE;

bool lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);

  // Set the resolution to 12 bits (0-4095)
  analogReadResolution(12);
  
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    while(1);
  }
  //display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();

  pinMode(BUTTON_PIN, INPUT_PULLUP); // Assuming button is connected between GPIO0 and GND
}

void drawBatteryIcon(int level) {
  // Нарисовать рамку батареи
  display.drawRect(10, 10, 35, 20, SSD1306_WHITE);
  display.drawRect(45, 15, 5, 10, SSD1306_WHITE);

  // Нарисовать заполнение в зависимости от уровня заряда
  for (int i = 0; i < level; i++) {
    display.fillRect(12 + i * 6, 12, 5, 16, SSD1306_WHITE);
  }
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && lastButtonState == HIGH) {
    currentMode = static_cast<DisplayMode>((currentMode + 1) % 4);
    delay(200); // Small delay to avoid multiple state changes
  }

  lastButtonState = buttonState;

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  int analogVolts = analogReadMilliVolts(BATTERY_PIN);
  
  float batteryVoltage = analogVolts * correctionFactor / 1000; // Учитываем корректирующий коэффициент
 
  float batteryLevel = map(batteryVoltage * 1000, 3000, 4200, 0, 100);
  batteryLevel = constrain(batteryLevel, 0, 100); // Ограничим процент до 100%

  float batteryIconLevel = map(batteryVoltage * 1000, 3000, 4200, 0, 5);
  batteryIconLevel = constrain(batteryIconLevel, 0, 5);
  
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  switch (currentMode) {
    case SHOW_TEMPERATURE:
      display.print("Temp: "); display.print(temp.temperature); display.println(" C");
      break;
    case SHOW_HUMIDITY:
      display.print("Humidity: "); display.print(humidity.relative_humidity); display.println(" %");
      break;
    case SHOW_BOTH:
      display.print("Temp: "); display.print(temp.temperature); display.println(" C");
      display.print("Humidity: "); display.print(humidity.relative_humidity); display.println(" %");
      break;
    case SHOW_BATTERY:
      //drawBatteryIcon(batteryLevel);

      //display.print("Voltage: "); display.print(batteryVoltage); display.println(" V");
      //display.print("Level: "); display.print(batteryLevel); display.println(" %");

      drawBatteryIcon(batteryIconLevel);
      break;
  }

  display.setCursor(0, 50);
  display.print(batteryLevel); display.println(" %");

  display.display();
  delay(100);
}
