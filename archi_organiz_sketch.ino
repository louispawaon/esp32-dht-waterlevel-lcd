#include <Wire.h>
#include <LiquidCrystal_I2C.h> 
#include <DHT.h> 
#include <WiFi.h>
#include <ESPAsyncWebServer.h> 

const int I2C_ADDR = 0x27;
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

#define DHTPIN 13       
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

const int waterLevelPin = 36; 
AsyncWebServer server(80);

const char* ssid = "Nyenyenye"; 
const char* password = "hatdog123"; 

bool showHumidity = false;
bool showTemperature = false;
bool showWaterLevel = false;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  delay(2000); 

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

  lcd.setCursor(0, 0);
  lcd.print("Your IP Address is");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  dht.begin();
  delay(2000); 

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
      lcd.print(lcdData.substring(lcdData.indexOf(' ') + 1)); 
    }
    request->send(200, "text/plain", "LCD updated");
  });

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
    content += "function updateSensorData() {";
    content += "displayData(document.querySelector('input[name=\"sensor\"]:checked').value);"; // Get the value of the checked radio button and call displayData
    content += "}";
    content += "setInterval(updateSensorData, 500);"; // Call updateSensorData every 5 seconds
    content += "</script>";
    content += "</body></html>";
    request->send(200, "text/html", content);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("sensor")) {
      String sensorType = request->getParam("sensor")->value();
      String data;
      if (sensorType == "humidity") {
        data = String(dht.readHumidity()) + " %";
      } else if (sensorType == "temperature") {
        data = String(dht.readTemperature()) + " C"; 
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
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int waterLevelValue = analogRead(waterLevelPin);

  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Water Level Percentage: ");
  Serial.println(waterLevelValue);
  
  delay(2000); 
}