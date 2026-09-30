#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "secrets.h"

// OLED dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// I2C addresses
#define OLED_ADDRESS 0x3C
#define BME_ADDRESS 0x76


Adafruit_BME280 bme;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Connecting to WiFi...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected!");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Start I2C
    // SDA = D21
    // SCL = D22
    Wire.begin(21, 22);

    // Start BME280
    if (!bme.begin(BME_ADDRESS, &Wire)) {
        Serial.println("BME280 initialization failed!");

        while (true) {
            delay(1000);
        }
    }

    // Start OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("OLED initialization failed!");

        while (true) {
            delay(1000);
        }
    }

    Serial.println("BME280 and OLED initialized!");

    // Startup message
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(20, 20);
    display.println("READY!");
    display.display();

    delay(2000);
}

void loop() {
    // Read BME280
    float temperatureC = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressureHpa = bme.readPressure() / 100.0F;

    // Convert to imperial
    float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
    float pressureInHg = pressureHpa * 0.02953;

    // --------------------
    // Serial Monitor
    // --------------------

    Serial.print("Temperature: ");
    Serial.print(temperatureF, 1);
    Serial.print(" F / ");
    Serial.print(temperatureC, 1);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity, 1);
    Serial.println(" %");

    Serial.print("Pressure: ");
    Serial.print(pressureInHg, 2);
    Serial.print(" inHg / ");
    Serial.print(pressureHpa, 1);
    Serial.println(" hPa");

    Serial.println("--------------------");

    // --------------------
    // OLED Display
    // --------------------

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    // Title
    display.setCursor(15, 0);
    display.println("ENVIRONMENT");

    // Temperature
    display.setCursor(0, 18);
    display.print("Temp: ");
    display.print(temperatureF, 1);
    display.println(" F");

    // Humidity
    display.setCursor(0, 34);
    display.print("Humidity: ");
    display.print(humidity, 1);
    display.println(" %");

    // Pressure
    display.setCursor(0, 50);
    display.print("Pressure: ");
    display.print(pressureInHg, 2);
    display.println(" inHg");

    // Send everything to the OLED
    display.display();

    // Update every 2 seconds
    delay(2000);
}