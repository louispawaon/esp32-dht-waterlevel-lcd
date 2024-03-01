#define BLYNK_TEMPLATE_ID "TMPL6VUy9_Dpt"
#define BLYNK_TEMPLATE_NAME "ArchiOrganiz"
#define BLYNK_AUTH_TOKEN "qtPsEWcIWkDKjn8upjD7MuGL3PUtiV3z"

#include <Wire.h>
#include <LiquidCrystal_I2C.h> 
#include <DHT.h> 
#include <WiFi.h>
#include <BlynkSimpleEsp32.h> // Include the Blynk library
#include <ESPAsyncWebServer.h> 

const int I2C_ADDR = 0x27;
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

#define DHTPIN 13       
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

const int waterLevelPin = 36;

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

char ssid[] = "Nyenyenye";
char pass[] = "hatdog123";

BlynkTimer timer;

bool showHumidity = false;
bool showTemperature = false;
bool showWaterLevel = false;

float humidity = 0;
float temperature = 0;
int waterLevelValue = 0;

BLYNK_WRITE(V3) {
  int switchState = param.asInt();
  if (switchState == 0) {
    showTemperature = false;
    showHumidity = false;
    showWaterLevel = false;
  } else if (switchState == 1) {
    showTemperature = true;
    showHumidity = false;
    showWaterLevel = false;
  } else if (switchState == 2) {
    showTemperature = false;
    showHumidity = true;
    showWaterLevel = false;
  } else if (switchState == 3) {
    showTemperature = false;
    showHumidity = false;
    showWaterLevel = true;
  }
}

void readSensors() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  waterLevelValue = analogRead(waterLevelPin);
}

void updateLCD() {
  lcd.clear();
  if (showTemperature) {
    lcd.setCursor(0, 0);
    lcd.print("Temperature:");
    lcd.setCursor(0, 1);
    lcd.print(temperature);
    lcd.print(" C");
  }
  if (showHumidity) {
    lcd.setCursor(0, 0);
    lcd.print("Humidity:");
    lcd.setCursor(0, 1);
    lcd.print(humidity);
    lcd.print(" %");
  }
  if (showWaterLevel) {
    lcd.setCursor(0, 0);
    lcd.print("Water Level:");
    lcd.setCursor(0, 1);
    lcd.print(waterLevelValue);
  }
}

void sendSensorDataToBlynk() {
  if (showTemperature) {
    Blynk.virtualWrite(V0, temperature); // Update temperature gauge widget
  }
  if (showHumidity) {
    Blynk.virtualWrite(V1, humidity); // Update humidity gauge widget
  }
  if (showWaterLevel) {
    Blynk.virtualWrite(V2, waterLevelValue); // Update water level label widget
  }
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  delay(2000); 

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); // Initialize Blynk

  dht.begin();
  delay(2000); 

  timer.setInterval(2000L, readSensors); // Read sensors every 2 seconds
  timer.setInterval(2000L, updateLCD); // Update LCD every 2 seconds
  timer.setInterval(2000L, sendSensorDataToBlynk); // Send sensor data to Blynk every 2 seconds
}

void loop() {
  Blynk.run(); // Run Blynk
  timer.run(); // Run timers for sensor readings and updates
}
