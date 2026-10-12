#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------------------
// OLED settings
// --------------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_ADDRESS 0x3C
#define BME_ADDRESS 0x76

Adafruit_BME280 bme;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Create web server on port 80
WebServer server(80);
unsigned long previousUpdate = 0;
const unsigned long updateInterval = 2000; // Update every 2 seconds


// ======================================================
// MAIN WEBPAGE
// ======================================================

void handleRoot() {

    String webpage = R"rawliteral(
<!DOCTYPE html>
<html>

<head>

    <meta name="viewport" content="width=device-width, initial-scale=1.0">

    <title>Environmental Monitor</title>

    <style>

        body {
            font-family: Arial, sans-serif;
            background-color: #f4f6f8;
            text-align: center;
            margin: 0;
            padding: 20px;
        }

        h1 {
            margin-bottom: 5px;
        }

        .status {
            color: green;
            margin-bottom: 30px;
        }

        .container {
            max-width: 700px;
            margin: auto;
        }

        .card {
            background: white;
            padding: 25px;
            margin: 15px;
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

    <div class="status">
        &#9679; Online
    </div>


    <div class="card">

        <div class="label">
            Temperature
        </div>

        <div class="value">
            <span id="temperature">--</span> &deg;F
        </div>

    </div>


    <div class="card">

        <div class="label">
            Humidity
        </div>

        <div class="value">
            <span id="humidity">--</span> %
        </div>

    </div>


    <div class="card">

        <div class="label">
            Pressure
        </div>

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

function updateData() {

    fetch('/data')

        .then(response => response.json())

        .then(data => {

            document.getElementById("temperature").innerText =
                data.temperature.toFixed(1);

            document.getElementById("humidity").innerText =
                data.humidity.toFixed(1);

            document.getElementById("pressure").innerText =
                data.pressure.toFixed(2);

            document.getElementById("lastUpdate").innerText =
                new Date().toLocaleTimeString();

        })

        .catch(error => {
            console.log("Error:", error);
        });
}


// Get data immediately when page loads
updateData();


// Get new data every 2 seconds
setInterval(updateData, 2000);

</script>


</body>
</html>
)rawliteral";


    server.send(200, "text/html", webpage);
}


// ======================================================
// SENSOR DATA ROUTE
// ======================================================

void handleData() {

    float temperatureC = bme.readTemperature();
    float temperatureF =
        (temperatureC * 9.0 / 5.0) + 32.0;

    float humidity =
        bme.readHumidity();

    float pressureHpa =
        bme.readPressure() / 100.0F;

    float pressureInHg =
        pressureHpa * 0.02953;


    // Build JSON response
    String json = "{";

    json += "\"temperature\":";
    json += String(temperatureF, 1);

    json += ",";

    json += "\"humidity\":";
    json += String(humidity, 1);

    json += ",";

    json += "\"pressure\":";
    json += String(pressureInHg, 2);

    json += "}";


    server.send(200, "application/json", json);
}


// ======================================================
// SETUP
// ======================================================

void setup() {

    Serial.begin(115200);

    delay(1000);


    // --------------------
    // Start I2C
    // --------------------

    Wire.begin(21, 22);


    // --------------------
    // Start BME280
    // --------------------

    if (!bme.begin(BME_ADDRESS, &Wire)) {

        Serial.println("BME280 initialization failed!");

        while (true) {
            delay(1000);
        }
    }


    // --------------------
    // Start OLED
    // --------------------

    if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {

        Serial.println("OLED initialization failed!");

        while (true) {
            delay(1000);
        }
    }


    Serial.println("BME280 and OLED initialized!");


    // --------------------
    // Connect to WiFi
    // --------------------

    Serial.println("Connecting to WiFi...");

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );


    while (WiFi.status() != WL_CONNECTED) {

        delay(500);

        Serial.print(".");
    }


    Serial.println();

    Serial.println("WiFi connected!");

    Serial.print("IP Address: ");

    Serial.println(
        WiFi.localIP()
    );


    // --------------------
    // Start web server
    // --------------------

    server.on("/", handleRoot);

    server.on("/data", handleData);

    server.begin();


    Serial.println(
        "Web server started!"
    );


    // --------------------
    // OLED startup message
    // --------------------

    display.clearDisplay();

    display.setTextColor(
        SSD1306_WHITE
    );

    display.setTextSize(2);

    display.setCursor(20, 20);

    display.println("READY!");

    display.display();


    delay(2000);
}


// ======================================================
// MAIN LOOP
// ======================================================

void loop() {

    // Continuously check for incoming web requests
    server.handleClient();

    // Update readings every 2 seconds without blocking
    if (millis() - previousUpdate >= updateInterval) {

        previousUpdate = millis();

        // --------------------
        // Read BME280
        // --------------------

        float temperatureC = bme.readTemperature();
        float humidity = bme.readHumidity();
        float pressureHpa = bme.readPressure() / 100.0F;

        // Convert to imperial
        float temperatureF = (temperatureC * 9.0F / 5.0F) + 32.0F;
        float pressureInHg = pressureHpa * 0.02953F;

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

        // Send the updated information to OLED
        display.display();
    }
}