#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> 
#include <SimpleTimer.h>

// Set Defalt Values
#define DEFAULT_Temperature 0
#define DEFAULT_Humidity 0
#define DHTTYPE DHT11  

const int I2C_ADDR = 0x27;
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

// BLE Credentils
const char *service_name = "ArchiOrganiz";
const char *pop = "12345678";

//GPIO Declaration
static uint8_t gpio_reset = 0;
static uint8_t DHTPIN = 13;
static uint8_t water_level = 36;

bool wifi_connected = 0;

float humid = 0;
float temp = 0;
float waterLevelValue = 0;

DHT dht(DHTPIN, DHTTYPE);

SimpleTimer Timer;

// Device Declaration
static TemperatureSensor temperature("Temperature");
static TemperatureSensor humidity("Humidity");
static TemperatureSensor waterLevel("Water Level");
static Switch temp_switch("Show Temperature");
static Switch humid_switch("Show Humidity");
static Switch water_switch("Show Water Level");


bool showHumidity = false;
bool showTemperature = false;
bool showWaterLevel = false;

//Provisioning
void sysProvEvent(arduino_event_t *sys_event){
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32
      Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on BLE\n", service_name, pop);
      printQR(service_name, pop, "ble");
#else
      Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on SoftAP\n", service_name, pop);
      printQR(service_name, pop, "softap");
#endif
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.printf("\nConnected to Wi-Fi!\n");
      wifi_connected = 1;
      delay(500);
      break;
    case ARDUINO_EVENT_PROV_CRED_RECV: {
        Serial.println("\nReceived Wi-Fi credentials");
        Serial.print("\tSSID : ");
        Serial.println((const char *) sys_event->event_info.prov_cred_recv.ssid);
        Serial.print("\tPassword : ");
        Serial.println((char const *) sys_event->event_info.prov_cred_recv.password);
        break;
      }
    case ARDUINO_EVENT_PROV_INIT:
      wifi_prov_mgr_disable_auto_stop(10000);
      break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("Stopping Provisioning!!!");
      wifi_prov_mgr_stop_provisioning();
      break;
  }
}
  
void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx){
  const char *device_name = device->getDeviceName();
  Serial.println(device_name);
  const char *param_name = param->getParamName();

  if (strcmp(device_name, "Show Temperature") == 0)
  {
    if (strcmp(param_name, "Power") == 0)
    {
      showTemperature = true;
      showHumidity = false;
      showWaterLevel = false;
      param->updateAndReport(val);
    }
  }

  if (strcmp(device_name, "Show Humidity") == 0)
  {
    if (strcmp(param_name, "Power") == 0)
    {
      showTemperature = false;
      showHumidity = true;
      showWaterLevel = false;
      param->updateAndReport(val);
    }
  }

  if (strcmp(device_name, "Show Water Level") == 0)
  {
    if (strcmp(param_name, "Power") == 0)
    {
      showTemperature = false;
      showHumidity = false;
      showWaterLevel = true;
      param->updateAndReport(val);
    }
  }
}

void readSensors() {
  humid = dht.readHumidity();
  temp = dht.readTemperature();
  waterLevelValue = analogRead(water_level);
  Serial.print("Temperature - "); Serial.println(temp);
  Serial.print("Humidity - "); Serial.println(humid);
  Serial.print("Water Level - "); Serial.println(waterLevelValue);

  temperature.updateAndReportParam("Temperature", temp);
  humidity.updateAndReportParam("Temperature", humid);
  waterLevel.updateAndReportParam("Temperature", waterLevelValue);

}

void updateLCD() {
  lcd.clear();
  if (showTemperature) {
    lcd.setCursor(0, 0);
    lcd.print("Temperature:");
    lcd.setCursor(0, 1);
    lcd.print(temp);
    lcd.print(" C");
  }
  if (showHumidity) {
    lcd.setCursor(0, 0);
    lcd.print("Humidity:");
    lcd.setCursor(0, 1);
    lcd.print(humid);
    lcd.print(" %");
  }
  if (showWaterLevel) {
    lcd.setCursor(0, 0);
    lcd.print("Water Level:");
    lcd.setCursor(0, 1);
    lcd.print(waterLevelValue);
  }
}

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  delay(2000); 
  
  pinMode(gpio_reset, INPUT);

  dht.begin();
  delay(2000);

  Node my_node;
  my_node = RMaker.initNode("Archi_Organiz");
  temp_switch.addCb(write_callback);
  humid_switch.addCb(write_callback);
  water_switch.addCb(write_callback);
  my_node.addDevice(temperature);
  my_node.addDevice(humidity);
  my_node.addDevice(waterLevel);
  my_node.addDevice(temp_switch);
  my_node.addDevice(humid_switch);
  my_node.addDevice(water_switch);

  Serial.printf("\nStarting ESP-RainMaker\n");
  RMaker.start();

  // Timer for Sending Sensor's Data
  Timer.setInterval(6000);

  WiFi.onEvent(sysProvEvent);

#if CONFIG_IDF_TARGET_ESP32
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
#else
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, WIFI_PROV_SCHEME_HANDLER_NONE, WIFI_PROV_SECURITY_1, pop, service_name);
#endif

}

void loop() {
  // put your main code here, to run repeatedly:
if (Timer.isReady() && wifi_connected) {                    // Check is ready a second timer
    Serial.println("Sending Sensor's Data");
    readSensors();
    updateLCD();
    Timer.reset();                        // Reset a second timer
  }



  //--------Logic to Reset RainMaker------------

  // Read GPIO0 (external button to reset device
  if (digitalRead(gpio_reset) == LOW) { //Push button pressed
    Serial.printf("Reset Button Pressed!\n");
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_reset) == LOW) delay(50);
    int endTime = millis();

    if ((endTime - startTime) > 10000) {
      // If key pressed for more than 10secs, reset all
      Serial.printf("Reset to factory.\n");
      wifi_connected = 0;
      RMakerFactoryReset(2);
    } else if ((endTime - startTime) > 3000) {
      Serial.printf("Reset Wi-Fi.\n");
      wifi_connected = 0;
      // If key pressed for more than 3secs, but less than 10, reset Wi-Fi
      RMakerWiFiReset(2);
    }
  }
  delay(100);
}


