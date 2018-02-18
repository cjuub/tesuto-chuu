
#include <Wire.h>
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <Ethernet.h>


#define BUTTON_PIN 14
#define LED_PIN 12
#define NUM_PIXELS 1


const char* WIFI_SSID_1 = "ssid1";
const char* WIFI_PASSWORD_1 = "pass1";
const char* WIFI_SSID_2 = "ssid2";
const char* WIFI_PASSWORD_2 = "pass2";


Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_RGB + NEO_KHZ400);
WiFiServer server(13337);
SSD1306 display(0x3c, 4, 5); //0x3C being the usual address of the OLED


bool isTestInProgress = false;
bool isButtonPressed = false;
String name = "";


ESP8266WiFiMulti wifi;

bool isConnected = false;
time_t startTime = millis();

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  
  strip.begin();
  server.begin();
  Serial.begin(115200);

  strip.setPixelColor(0, getRgb(0, 0, 255));
  strip.show();
  
  delay(5000);

  display.init();
  display.flipScreenVertically();

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  // An IP must be set here otherwise wifi.run() will fail later.    
  IPAddress ip(10, 0, 0, 115);
  IPAddress netmask(255, 255, 255, 0);
  IPAddress gateway(10, 0, 0, 1);

  WiFi.config(ip, netmask, gateway);

  // Uncomment this to clear remembered APs
//  WiFi.disconnect();
  
  wifi.addAP(WIFI_SSID_1, WIFI_PASSWORD_1);
  wifi.addAP(WIFI_SSID_2, WIFI_PASSWORD_2);

  if (wifi.run() == WL_CONNECTED) {
    isConnected = true;
    selectIp();
  }

  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();

  if (!isTestInProgress && time_t(startTime + 30000) < millis()) {
    if (!isConnected) {
      strip.setPixelColor(0, getRgb(64, 0, 0));
      strip.show();
    }

    if (wifi.run() == WL_CONNECTED) {
      isConnected = true;
      printOnScreen("");
      selectIp();
    } else {
      isConnected = false;
      delay(1000);
    }

    startTime = millis();
  }
  
  if (isTestInProgress) {
    for (int i = 0; i < 50; i++) {
      strip.setPixelColor(0, getRgb(i, 0, 0));
      strip.show();
      
      updateButtonPressed();
      updateScreen();
      handleClient();

      if (!isTestInProgress) {
        break;
      }

      delay(10);
    } 
  } else {
    updateButtonPressed();
    handleClient();

    if (isConnected) {
      strip.setPixelColor(0, getRgb(0, 20, 0));
      updateScreen();
    } else {
      strip.setPixelColor(0, getRgb(32, 16, 0));
      printOnScreen("Conn Err.");
    }
    
    strip.show();
  }

  delay(10);
}

void selectIp() {
  if (WiFi.SSID() == WIFI_SSID_2) {
      IPAddress ip(192, 168, 1, 115);
      IPAddress netmask(255, 255, 255, 0);
      IPAddress gateway(192, 168, 1, 1); 
      WiFi.config(ip, netmask, gateway);
      WiFi.reconnect();
    } else if (WiFi.SSID() == WIFI_SSID_1) {
      IPAddress ip(10, 0, 0, 115);
      IPAddress netmask(255, 255, 255, 0);
      IPAddress gateway(10, 0, 0, 1); 
      WiFi.config(ip, netmask, gateway);
      WiFi.reconnect();
    }
}

void handleClient() {
  WiFiClient client = server.available();
  if (client) {
    String testMode = client.readStringUntil(',');

    if (testMode == "ON") {
      isTestInProgress = true;
      name = client.readStringUntil('\n');
    } else if (testMode == "OFF") {
      isTestInProgress = false;
    }

    client.flush();
  }
}

void updateButtonPressed() {
  if (digitalRead(BUTTON_PIN) && !isButtonPressed) {
    isButtonPressed = true;
    isTestInProgress = !isTestInProgress;
    name = "";
  } else if (!digitalRead(BUTTON_PIN) && isButtonPressed) {
    isButtonPressed = false;
  }
}

void updateScreen() {
  display.clear();

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  if (isTestInProgress) {
    display.drawString(64, 0, "Test pågår!");
    display.drawString(64, 36, name);
  }

  display.display();
}

void printOnScreen(String str) {
  display.clear();

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(64, 0, str);
  
  display.display();
}

uint32_t getRgb(uint8_t r, uint8_t g, uint8_t b) {
  uint32_t color = (uint32_t(r) << 16) + (uint32_t(g) << 8) + (uint32_t(b));
  return color;
}

void clearStrip() {
  for( int i = 0; i < NUM_PIXELS; i++){
    strip.setPixelColor(i, 0x000000); strip.show();
  }
}

