
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <math.h>

#include "secrets.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======================================================
// HARDWARE SETTINGS
// ======================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_ADDRESS 0x3C
#define BME_ADDRESS 0x76

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_BME280 bme;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

WebServer server(80);

// ======================================================
// TIMING
// ======================================================

// OLED and Serial Monitor: every 2 seconds
unsigned long previousUpdate = 0;
const unsigned long updateInterval = 2000;

// Supabase upload: every 60 seconds
unsigned long previousCloudUpload = 0;
const unsigned long cloudUploadInterval = 60000;

// ======================================================
// SENSOR DATA
// ======================================================

float temperatureC = 0;
float temperatureF = 0;
float humidity = 0;
float pressureHpa = 0;
float pressureInHg = 0;

void readSensor() {

    temperatureC = bme.readTemperature();
    humidity = bme.readHumidity();
    pressureHpa = bme.readPressure() / 100.0F;

    temperatureF = (temperatureC * 9.0F / 5.0F) + 32.0F;
    pressureInHg = pressureHpa * 0.02953F;
}

// ======================================================
// LOCAL WEB DASHBOARD
// ======================================================

void handleRoot() {

    const char* webpage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport"
          content="width=device-width, initial-scale=1.0">

    <title>Environmental Monitor</title>

    <style>
        body {
            font-family: Arial, sans-serif;
            background: #f4f6f8;
            text-align: center;
            margin: 0;
            padding: 20px;
        }

        .container {
            max-width: 700px;
            margin: auto;
        }

        h1 {
            margin-bottom: 5px;
        }

        .status {
            color: green;
            margin-bottom: 30px;
        }

        .card {
            background: white;
            padding: 25px;
            margin: 15px 0;
            border-radius: 12px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.15);
        }

        .label {
            font-size: 18px;
            color: #555;
        }

        .value {
            font-size: 36px;
            font-weight: bold;
            margin-top: 10px;
        }

        .updated {
            margin-top: 25px;
            color: #777;
            font-size: 14px;
        }
    </style>
</head>

<body>
<div class="container">

    <h1>Environmental Monitor</h1>

    <div class="status" id="status">
        &#9679; Connecting...
    </div>

    <div class="card">
        <div class="label">Temperature</div>
        <div class="value">
            <span id="temperature">--</span> &deg;F
        </div>
    </div>

    <div class="card">
        <div class="label">Humidity</div>
        <div class="value">
            <span id="humidity">--</span> %
        </div>
    </div>

    <div class="card">
        <div class="label">Pressure</div>
        <div class="value">
            <span id="pressure">--</span> inHg
        </div>
    </div>

    <div class="updated">
        Last updated:
        <span id="lastUpdate">--</span>
    </div>

</div>

<script>
async function updateData() {
    try {
        const response = await fetch('/data', {
            cache: 'no-store'
        });

        if (!response.ok) {
            throw new Error("Request failed");
        }

        const data = await response.json();

        document.getElementById("temperature").innerText =
            data.temperature.toFixed(1);

        document.getElementById("humidity").innerText =
            data.humidity.toFixed(1);

        document.getElementById("pressure").innerText =
            data.pressure.toFixed(2);

        document.getElementById("lastUpdate").innerText =
            new Date().toLocaleTimeString();

        document.getElementById("status").innerText =
            "● Online";

        document.getElementById("status").style.color =
            "green";

    } catch (error) {

        document.getElementById("status").innerText =
            "● Connection lost";

        document.getElementById("status").style.color =
            "red";

        console.log(error);
    }
}

updateData();
setInterval(updateData, 2000);
</script>

</body>
</html>
)rawliteral";

    server.send(200, "text/html", webpage);
}

// ======================================================
// JSON DATA FOR LOCAL DASHBOARD
// ======================================================

void handleData() {

    if (!isfinite(temperatureF) ||
        !isfinite(humidity) ||
        !isfinite(pressureInHg)) {

        server.send(503, "text/plain", "Invalid sensor data");
        return;
    }

    String json = "{";

    json += "\"temperature\":";
    json += String(temperatureF, 1);

    json += ",\"humidity\":";
    json += String(humidity, 1);

    json += ",\"pressure\":";
    json += String(pressureInHg, 2);

    json += "}";

    server.send(200, "application/json", json);
}

// ======================================================
// OLED DISPLAY
// ======================================================

void updateOLED() {

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(15, 0);
    display.println("ENVIRONMENT");

    display.setCursor(0, 18);
    display.print("Temp: ");
    display.print(temperatureF, 1);
    display.println(" F");

    display.setCursor(0, 34);
    display.print("Humidity: ");
    display.print(humidity, 1);
    display.println(" %");

    display.setCursor(0, 50);
    display.print("Pressure: ");
    display.print(pressureInHg, 2);
    display.println(" inHg");

    display.display();
}

// ======================================================
// SERIAL MONITOR
// ======================================================

void printReadings() {

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
}

// ======================================================
// SUPABASE CLOUD UPLOAD
// ======================================================

void uploadToSupabase() {

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Cloud upload skipped: WiFi disconnected");
        return;
    }

    if (!isfinite(temperatureC) ||
        !isfinite(humidity) ||
        !isfinite(pressureHpa)) {

        Serial.println("Cloud upload skipped: Invalid sensor data");
        return;
    }

    // TLS verification needs the correct current time.
    struct tm timeInfo;

    if (!getLocalTime(&timeInfo, 1000)) {
        Serial.println("Cloud upload skipped: Clock not synchronized");
        return;
    }

    // Build JSON using the latest sensor readings
    String json = "{";

    json += "\"device_id\":\"esp32-bedroom\",";

    json += "\"temperature_c\":";
    json += String(temperatureC, 2);

    json += ",\"humidity\":";
    json += String(humidity, 2);

    json += ",\"pressure_hpa\":";
    json += String(pressureHpa, 2);

    json += "}";

    WiFiClientSecure secureClient;

    // Use the public root CA certificate stored in secrets.h.
    secureClient.setCACert(SUPABASE_ROOT_CA);

    HTTPClient http;
    http.setTimeout(5000);

    Serial.println("Uploading to Supabase...");

    if (!http.begin(secureClient, SUPABASE_FUNCTION_URL)) {
        Serial.println("Failed to initialize HTTPS request");
        return;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-device-secret", DEVICE_UPLOAD_SECRET);

    int responseCode = http.POST(json);

    Serial.print("Supabase response: ");
    Serial.println(responseCode);

    if (responseCode == 200) {
        Serial.println("Cloud upload successful!");
    } else if (responseCode > 0) {
        Serial.println(http.getString());
    } else {
        Serial.print("HTTP error: ");
        Serial.println(http.errorToString(responseCode));
    }

    http.end();
}

// ======================================================
// SETUP - RUNS ONCE
// ======================================================

void setup() {

    Serial.begin(115200);
    delay(1000);

    // Start I2C
    Wire.begin(SDA_PIN, SCL_PIN);

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

    // First sensor reading
    readSensor();
    updateOLED();
    printReadings();

    // Connect to WiFi
    Serial.println("Connecting to WiFi...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected!");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Synchronize the clock for HTTPS certificate checks
    configTime(0, 0, "pool.ntp.org", "time.google.com");

    // Start local web server
    server.on("/", handleRoot);
    server.on("/data", handleData);

    server.begin();

    Serial.println("Web server started!");

    // Start timing from now
    previousUpdate = millis();
    previousCloudUpload = millis();
}

// ======================================================
// MAIN LOOP - RUNS CONTINUOUSLY
// ======================================================

void loop() {

    // Always check for incoming local web requests
    server.handleClient();

    unsigned long currentTime = millis();

    // ----------------------------------
    // Update sensor, OLED and Serial
    // every 2 seconds
    // ----------------------------------

    if (currentTime - previousUpdate >= updateInterval) {

        previousUpdate = currentTime;

        readSensor();
        updateOLED();
        printReadings();
    }

    // ----------------------------------
    // Upload to Supabase every 60 seconds
    // ----------------------------------

    if (currentTime - previousCloudUpload >= cloudUploadInterval) {

        previousCloudUpload = currentTime;

        uploadToSupabase();
    }
}
