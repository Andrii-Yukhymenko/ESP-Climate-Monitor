#include "esp_sleep.h"
#include "driver/gpio.h"

#define WAKE_UP_BUTTON_GPIO 0
#define WAKE_UP_BUTTON_GPIO_BITMASK 0b000001
#define SLEEP_DURATION 10000000 // 10 seconds in microseconds
#define DEBOUNCE_TIME 1000

volatile bool buttonPressed = false;

void IRAM_ATTR handleButtonPress() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting...");

  gpio_set_direction(gpio_num_t(WAKE_UP_BUTTON_GPIO), GPIO_MODE_INPUT);

  pinMode(WAKE_UP_BUTTON_GPIO, INPUT);

  attachInterrupt(digitalPinToInterrupt(WAKE_UP_BUTTON_GPIO), handleButtonPress, RISING);

  // esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
  // if (wakeup_cause == ESP_SLEEP_WAKEUP_GPIO) {
  //   Serial.println("Woken up from deep sleep by GPIO");
  // } else if (wakeup_cause == ESP_SLEEP_WAKEUP_TIMER) {
  //   Serial.println("Woken up from deep sleep by timer");
  // } else {
  //   Serial.println("Power on or reset");
  // }

  Serial.println("Woken up. Start from setup");
}

void loop() {
  if (buttonPressed) {
    buttonPressed = false;
    delay(DEBOUNCE_TIME);
    enterDeepSleep();
  }
}

void enterDeepSleep() {
  Serial.println("Going to deep sleep...");
  esp_deep_sleep_enable_gpio_wakeup(WAKE_UP_BUTTON_GPIO_BITMASK, ESP_GPIO_WAKEUP_GPIO_HIGH);
  //esp_sleep_enable_timer_wakeup(sleepDurationUs);
  esp_deep_sleep_start();
}