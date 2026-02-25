#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <EPaperDrive.h>
#include "FS.h"
#include <time.h>

#define CS 15
#define RST 2
#define DC 0
#define BUSY 4
#define CLK 14
#define DIN 13
#define BATTERY_PIN A0

// A program that checks the subwaynow API for the next uptown N train arrivals at Fort Hamilton Parkway and displays it on an e-ink display, along with battery status. The program updates every 60 seconds.
// Note: The ESP8266 does not have a built-in RTC, so the "minutes away" calculation is based on the API's provided timestamp. For accurate timekeeping, consider adding an NTP sync or using an RTC module in a production version of this firmware.
// Even though the hardware supports tri-color e-ink, this sketch only use black and white due to a bug with 

// --- WiFi Configuration (Extracted from firmware dump) ---
const char* ssid     = "HStark-NY";
const char* password = "116208818";

// --- API Configuration ---
const char* api_url = "https://api.subwaynow.app/stops/N03";

EPaperDrive EPD(0, CS, RST, DC, BUSY, CLK, DIN);

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // 60 seconds

// Display a simple message and uptime on the e-ink screen (used for errors and status updates), using full refresh
void displaySimpleMessage(const char* message) {
  EPD.EPD_init_Full();
  EPD.clearbuffer();
  EPD.fontscale = 2;
  EPD.SetFont(FONT12);
  EPD.DrawUTF(10, 10, message);
  EPD.fontscale = 1;
  EPD.DrawUTF(40, 10, "Uptime: " + String(millis() / 1000) + "s");
  EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer, 1);
  EPD.EPD_Update();
  EPD.ReadBusy_long();
  EPD.deepsleep();
}

void drawTrainData(DynamicJsonDocument& doc) {
  EPD.EPD_init_Full();
  EPD.clearbuffer();
  EPD.fontscale = 2;
  EPD.SetFont(FONT12);

  EPD.DrawUTF(10, 10, "Uptown N Train Arrivals:");
  EPD.DrawUTF(40, 10, "Fort Hamilton Pkwy");

  int yPos = 80;
  int count = 0;
  long currentTimestamp = doc["timestamp"];
  
  for (JsonObject trip : doc["upcoming_trips"]["north"].as<JsonArray>()) {
    if (count >= 3) break; // Show up to 3 arrivals
    
    long arrivalTime = trip["estimated_current_stop_arrival_time"];
    if (arrivalTime == 0) arrivalTime = trip["current_stop_arrival_time"];
    
    if (arrivalTime > 0) {
      int minutesAway = (arrivalTime - currentTimestamp) / 60;
      EPD.DrawUTF(yPos, 10, minutesAway <= 0 ? "Now" : String(minutesAway) + " min");
      yPos += 40;
      count++;
    }
  }
  
  if (count == 0) EPD.DrawUTF(yPos, 10, "No upcoming trains");

  // Draw last updated time down to the second using the API's timestamp for reference
  if (currentTimestamp > 0) {
    time_t now = currentTimestamp;
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1); // New York time
    tzset();
    char timeStringBuff[40];
    strftime(timeStringBuff, sizeof(timeStringBuff), "Updated: %H:%M:%S", localtime(&now));
    EPD.fontscale = 1;
    EPD.DrawUTF(270, 10, timeStringBuff);
  }

  // Draw battery status
  int rawBattery = analogRead(BATTERY_PIN);
  int batteryPercent = (rawBattery - 200) * 100 / (780 - 200);
  char batteryStringBuff[40];
  snprintf(batteryStringBuff, sizeof(batteryStringBuff), "Bat: (%d%%) R:%d", batteryPercent, rawBattery);
  
  EPD.fontscale = 1;
  EPD.DrawUTF(250, 10, batteryStringBuff);

  EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer, 1);
  Serial.println("Updating display...");
  EPD.EPD_Update();
  EPD.ReadBusy_long();
  EPD.deepsleep();
  Serial.println("Display update complete.");
}

void updateTrainStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping update.");
    displaySimpleMessage("WiFi not connected");
    return;
  }

  Serial.println("Fetching subway data...");
  Serial.printf("Free Heap before client: %d\n", ESP.getFreeHeap());

  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  if (!client) {
     Serial.println("Unable to create client");
     return;
  }
  
  client->setInsecure(); // Ignore SSL certificate validation
  client->setBufferSizes(8192, 512); // Increasing RX buffer to 8KB to prevent IncompleteInput errors
  client->setTimeout(20000); // 20 seconds timeout
  
  Serial.printf("Free Heap after client: %d\n", ESP.getFreeHeap());

  HTTPClient http;
  http.useHTTP10(true); // Use HTTP/1.0 to reduce memory usage
  http.setTimeout(20000); // Increase HTTP timeout to match client

  Serial.println("Starting HTTP request...");
  if (http.begin(*client, api_url)) {
    Serial.println("Connected to server, sending GET...");
    int httpCode = http.GET();
    Serial.printf("HTTP GET finished. Code: %d\n", httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
      // Print content length for debugging
      int len = http.getSize();
      Serial.printf("Content-Length: %d\n", len);

      StaticJsonDocument<200> filter;
      filter["upcoming_trips"]["north"][0]["estimated_current_stop_arrival_time"] = true;
      filter["upcoming_trips"]["north"][0]["current_stop_arrival_time"] = true;
      filter["timestamp"] = true;

      // Use heap allocation for doc, 3KB should be enough for the filtered result and some overhead
      DynamicJsonDocument doc(3072);
      
      // Deserialize directly from stream using the filter to minimize memory usage
      DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));

      if (!error) {
        drawTrainData(doc);
      } else {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
      displaySimpleMessage("HTTP GET failed");
    }
    http.end();
  } else {
    Serial.println("Unable to connect to API");
    displaySimpleMessage("API connection failed");
  }
}

void setup() {
  Serial.begin(74880); // Use 74880 to match boot log baud rate to see crash dumps
  delay(2000); 
  Serial.println("\n\nStarting custom firmware...");
  Serial.flush();

  WiFi.setOutputPower(15); // 0-20.5dBm, 10 is enough for close range
  
  Serial.println(SPIFFS.begin() ? "SPIFFS Mounted Successfully" : "SPIFFS Mount Failed");
  EPD.SetFS(&SPIFFS);

  Serial.println("Initializing e-ink display...");
  EPD.EPD_Set_Model(OPM42); 
  displaySimpleMessage("Starting...");

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries++ < 20) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    displaySimpleMessage("WiFi connected!");
  } else {
    Serial.println("\nWiFi connection failed.");
    displaySimpleMessage("WiFi connection failed");
  }

  updateTrainStatus();
}

void loop() {
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    updateTrainStatus();
  }
}