#include <Arduino.h>
#include "esp_now_driver.h"

EspNowDriver radio;

void setup() {
    Serial.begin(115200);
    if (radio.init()) {
        Serial.println("ESP-NOW Initialized successfully!");
    } else {
        Serial.println("ESP-NOW Initialization failed!");
    }
}

void loop() {
    // Main loop logic
}