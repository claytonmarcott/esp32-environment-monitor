#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Start I2C
    // SDA = D21
    // SCL = D22
    Wire.begin(21, 22);

    Serial.println("Starting BME280...");

    // New BME280 was detected at address 0x76
    if (!bme.begin(0x76, &Wire)) {
        Serial.println("BME280 initialization failed!");

        while (true) {
            delay(1000);
        }
    }

    Serial.println("BME280 initialized successfully!");
}

void loop() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");

    Serial.println("--------------------");

    delay(2000);
}