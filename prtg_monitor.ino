#
# PRTG_monitor.ino
#
# created by: B. Duijnhouwer
# on: September 1st, 2023
#
#
# This code is for an ESP2866 and monitors your PRTG server for errors and warnings.
# Depending on the status of PRTG it will color an IKEA pencil green, yellow or red.
# This project was created by me to give our helpdesk a visual aid for the status of PRTG.
#
#

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN D4
#define LED_COUNT 24

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "your_wifi_ssid_here";          // Variable for WiFi SSID
const char* password = "your_wifi_password_here";  // Variable for WiFi password
const char* prtgServer = "your_prtg_server_here";  // Variable for PRTG server (hostname or IP address)
const int prtgPort = 443;

//
// To find your passhash, go to:
// - PRTG interface
// - Setup
// - System Administration
// - User Accounts
// - <your PRTG user>
// - <blue button: SHOW PASSHASH>
//

const char* prtg_user = "your_prtg_user_here";     // Variable for PRTG username
const char* passhass = "your_passhass_here";       // Variable for PRTG passhash

String requestURL_error;
String requestURL_warning;

int ErrorValue;
int WarningValue;

String prtg_error_string;
String prtg_warning_string;

void setColor(uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void readPRTG(String& prtg_string, const char* requestURL, int& value) {
  WiFiClientSecure client;
  client.setInsecure();
  
  Serial.print("Connecting to PRTG server...");
  if (client.connect(prtgServer, prtgPort)) {
    Serial.println("connected");

    // Send the request with the dynamic URL
    client.print(String(requestURL) +
                 "Host: " + prtgServer + "\r\n" +
                 "Connection: close\r\n\r\n");

    while (client.connected() || client.available()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        prtg_string = line;
      }
    }

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, prtg_string);

    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
      return;
    }

    value = doc["treesize"];

    client.stop();
  }
}

void setup() {
  strip.begin();
  strip.show();

  ErrorValue = 0;
  WarningValue = 0;
  Serial.begin(115200);

  WiFiManager wifiManager;
  wifiManager.autoConnect("PRTG_monitor");

  Serial.println("Connected to Wi-Fi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Create dynamic URLs for errors and warnings (only once)
  requestURL_error = "GET /api/table.json?content=sensors&output=json&columns=sensor&filter_status=5";
  requestURL_error += "&username=";
  requestURL_error += prtg_user;
  requestURL_error += "&passhash=";
  requestURL_error += passhass;
  requestURL_error += " HTTP/1.1\r\n";
  
  requestURL_warning = "GET /api/table.json?content=sensors&output=json&columns=sensor&filter_status=4";
  requestURL_warning += "&username=";
  requestURL_warning += prtg_user;
  requestURL_warning += "&passhash=";
  requestURL_warning += passhass;
  requestURL_warning += " HTTP/1.1\r\n";
}

void loop() {
  // Use the dynamic URLs created in setup()
  readPRTG(prtg_error_string, requestURL_error.c_str(), ErrorValue);
  readPRTG(prtg_warning_string, requestURL_warning.c_str(), WarningValue);

  if (ErrorValue > 0) {
    flashColor(strip.Color(255, 0, 0), "red");
  } else if (WarningValue > 0) {
    flashColor(strip.Color(255, 255, 0), "yellow");
  } else {
    flashColor(strip.Color(0, 255, 0), "green");
  }

  delay(30000);  // Wait 30 seconds between updates
}

void flashColor(uint32_t color, const char* colorName) {
  Serial.print("Make it ");
  Serial.println(colorName);

  for (int i = 0; i < 3; i++) {
    setColor(strip.Color(0, 0, 0));
    delay(1000);
    setColor(color);
    delay(1000);
  }

  setColor(strip.Color(0, 0, 0));
  delay(1000);
  setColor(color);
}
