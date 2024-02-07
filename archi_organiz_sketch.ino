//NOTE: There are other ways how to do the webserver but this is the most basic one, feel free to add your touch for this

#include <Wire.h>
#include <LiquidCrystal_I2C.h> // Install LiquidCrystal I2C by Frank de Brabander or any counterpart for lcds with no I2C adapter
#include <DHT.h> // Install DHT sensor library by Adafruit
#include <WiFi.h>
#include <ESPAsyncWebServer.h> // Install ESPAsyncWebServer by lacamera 

// Set the LCD address (you may need to change this depending on your LCD module)
const int I2C_ADDR = 0x27;

// Set the LCD dimensions (16x2 characters)
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

// Create an instance of the LCD class - this will differ to the soldered lcd with no I2C adapter
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

// DHT sensor configuration
#define DHTPIN 13       // Replace with your chosen GPIO pin for DHT11
#define DHTTYPE DHT11  

DHT dht(DHTPIN, DHTTYPE);

// Analog pin connected to the water level sensor
const int waterLevelPin = 36; // DO NOT change - 36 responds to VP or a Sensor GPIO

// Web Server
AsyncWebServer server(80);

//WiFi Connection - much better if you use Mobile Hotspot rather than your WiFi
const char* ssid = "Nyenyenye"; //Change with your own SSID (WiFi name you want to connect)
const char* password = "hatdog123"; //Change with the password of that WiFi

// Variable Declaration
bool showHumidity = false;
bool showTemperature = false;
bool showWaterLevel = false;

void setup() {
  Serial.begin(115200);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  delay(2000); // Wait for the LCD to initialize

  // Connect to Wi-Fi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Printing the IP Address to the LCD
  lcd.setCursor(0, 0);
  lcd.print("Your IP Address is");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  // Initialize the DHT sensor
  dht.begin();
  delay(2000); // Wait for the DHT sensor to initialize

  // New endpoint to update the LCD
  server.on("/updateLCD", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("lcdData")) {
      String lcdData = request->getParam("lcdData")->value();
      lcd.clear();
      lcd.setCursor(0, 0);
      if (lcdData.startsWith("Temperature")) {
        lcd.print("Temperature");
      } else if (lcdData.startsWith("Humidity")) {
        lcd.print("Humidity");
      } else if (lcdData.startsWith("Water Level")) {
        lcd.print("Water Level");
      }
      lcd.setCursor(0, 1);
      lcd.print(lcdData.substring(lcdData.indexOf(' ') + 1)); // Print the data without the sensor type
    }
    request->send(200, "text/plain", "LCD updated");
  });

  // Initialize the web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String content = "<!DOCTYPE html><html><head><title>ESP32 Sensor Data</title></head><body>";
    content += "<h1>ESP32 Sensor Data</h1>";
    content += "<form>";
    content += "<input type='radio' name='sensor' value='humidity' onchange='displayData(this.value)'> Humidity";
    content += "<input type='radio' name='sensor' value='temperature' onchange='displayData(this.value)'> Temperature";
    content += "<input type='radio' name='sensor' value='waterLevel' onchange='displayData(this.value)'> Water Level";
    content += "</form>";
    content += "<div id='sensorData'></div>";
    content += "<script>";
    content += "function displayData(sensor) {";
    content += "var xhttp = new XMLHttpRequest();";
    content += "xhttp.onreadystatechange = function() {";
    content += "if (this.readyState == 4 && this.status == 200) {";
    content += "document.getElementById('sensorData').innerHTML = this.responseText;";
    content += "if (sensor == 'humidity') {";
    content += "updateLCD('Humidity: ' + this.responseText);";
    content += "} else if (sensor == 'temperature') {";
    content += "updateLCD('Temperature: ' + this.responseText);";
    content += "} else if (sensor == 'waterLevel') {";
    content += "updateLCD('Water Level: ' + this.responseText);";
    content += "}";
    content += "}";
    content += "};";
    content += "xhttp.open('GET', '/data?sensor=' + sensor, true);";
    content += "xhttp.send();";
    content += "}";
    content += "function updateLCD(data) {";
    content += "var lcdData = data;";
    content += "var xhttp = new XMLHttpRequest();";
    content += "xhttp.open('GET', '/updateLCD?lcdData=' + encodeURIComponent(lcdData), true);";
    content += "xhttp.send();";
    content += "}";
    content += "</script>";
    content += "</body></html>";
    request->send(200, "text/html", content);
  });

    // New endpoint to provide sensor data based on selected sensor type
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("sensor")) {
      String sensorType = request->getParam("sensor")->value();
      String data;
      if (sensorType == "humidity") {
        data = String(dht.readHumidity()) + " %";
      } else if (sensorType == "temperature") {
        data = String(dht.readTemperature()) + " C"; // Unicode for degree symbol
      } else if (sensorType == "waterLevel") {
        data = String(analogRead(waterLevelPin));
      }
      request->send(200, "text/plain", data);
    } else {
      request->send(400, "text/plain", "Error: Sensor type not specified");
    }
  });

  server.begin();
}

void loop() {
  // Read sensor data and print it continuously - Testing Purposes
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int waterLevelValue = analogRead(waterLevelPin);

  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Water Level Percentage: ");
  Serial.println(waterLevelValue);
  
  delay(2000); // Adjust delay as needed
}