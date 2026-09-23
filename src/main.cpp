#include <Arduino.h>

const int LED_PIN = 23;

bool ledState = LOW;
unsigned long previousMillis = 0;
const unsigned long interval = 1000;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);

        if (ledState == HIGH) {
            Serial.println("LED ON");
        } else {
            Serial.println("LED OFF");
        }
    }
}