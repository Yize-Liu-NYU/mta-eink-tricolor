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

// A program that checks the subwaynow API for the next uptown N train arrivals at Fort Hamilton Parkway and displays it on an e-ink display, along with battery status. The program updates every 60 seconds.
// Note: The ESP8266 does not have a built-in RTC, so the "minutes away" calculation is based on the API's provided timestamp. For accurate timekeeping, consider adding an NTP sync or using an RTC module in a production version of this firmware.
// Even though the hardware supports tri-color e-ink, this sketch only use black and white due to a bug with 

// --- WiFi Configuration (Extracted from firmware dump) ---
const char* ssid     = "HStark-NY";
const char* password = "116208818";

// --- API Configuration ---
const char* api_url = "https://api.subwaynow.app/stops/N03";

// --- Battery Configuration ---
#define BATTERY_PIN A0
// Adjust this multiplier based on your specific voltage divider.
// For NodeMCU v2 built-in divider (0-3.3V): 3.3 / 1023.0
// For custom divider for 4.2V LiPo: 4.2 / 1023.0
const float BATTERY_VOLTAGE_MULTIPLIER = 4.2 / 1023.0; 

EPaperDrive EPD(0, CS, RST, DC, BUSY, CLK, DIN);

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // 60 seconds

// Display a simple message and uptime on the e-ink screen (used for errors and status updates)
void displaySimpleMessage(const char* message) {
  EPD.EPD_init_Full();
  EPD.clearbuffer();
  EPD.fontscale = 2;
  EPD.SetFont(FONT12);
  EPD.DrawUTF(10, 10, message);
  // show uptime in seconds to help with debugging
  unsigned long uptimeSeconds = millis() / 1000;
  char uptimeBuff[40];
  snprintf(uptimeBuff, sizeof(uptimeBuff), "Uptime: %lu sec", uptimeSeconds);
  EPD.fontscale = 1;
  EPD.DrawUTF(10, 60, uptimeBuff);
  EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer, 1);
  EPD.EPD_Update();
  EPD.ReadBusy_long();
  EPD.deepsleep();
}

void updateDisplay() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping update.");
    // Display an error message on the e-ink screen if WiFi is not connected
    displaySimpleMessage("WiFi not connected");
    return;
  }

  Serial.println("Fetching subway data...");
  
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  client->setInsecure(); // Ignore SSL certificate validation for simplicity
  client->setBufferSizes(4096, 1024); // Reduce buffer size to save memory
  HTTPClient http;

  if (http.begin(*client, api_url)) {
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        // Parse JSON directly from stream to save memory
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, http.getStream());

        if (error) {
          Serial.print("deserializeJson() failed: ");
          String errorStr = error.c_str();
          Serial.println(errorStr);
          http.end();
          // Display an error message on the e-ink screen if JSON parsing fails
          String errorMessage = String("JSON parse error: ") + errorStr;
          displaySimpleMessage(errorMessage.c_str());
          return;
        }

        // Extract uptown N train arrivals
        JsonArray northTrips = doc["upcoming_trips"]["north"];
        
        EPD.EPD_init_Full();
        EPD.clearbuffer();
        EPD.fontscale = 2;
        EPD.SetFont(FONT12);

        EPD.DrawUTF(10, 10, "Uptown N Train Arrivals:");
        EPD.DrawUTF(40, 10, "Fort Hamilton Pkwy");

        int yPos = 80;
        int count = 0;
        
        long currentTimestamp = doc["timestamp"];
        
        for (JsonObject trip : northTrips) {
          if (count >= 3) break; // Show up to 3 arrivals
          
          long arrivalTime = trip["estimated_current_stop_arrival_time"];
          if (arrivalTime == 0) {
             arrivalTime = trip["current_stop_arrival_time"];
          }
          
          if (arrivalTime > 0) {
            // Calculate minutes away
            // Note: The ESP8266 doesn't have a built-in RTC synced to NTP by default in this sketch.
            // For accurate "minutes away", we need the current Unix time.
            // The API provides a "timestamp" field we can use as the current time.
            int minutesAway = (arrivalTime - currentTimestamp) / 60;
            
            String arrivalStr = String(minutesAway) + " min";
            if (minutesAway <= 0) {
              arrivalStr = "Now";
            }
            
            EPD.DrawUTF(yPos, 10, arrivalStr);
            yPos += 40;
            count++;
          }
        }
        
        if (count == 0) {
           EPD.DrawUTF(yPos, 10, "No upcoming trains");
        }

        // Draw last updated time down to the second using the API's timestamp for reference
        if (currentTimestamp > 0) {
          time_t now = currentTimestamp;
          struct tm * timeinfo;
          setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1); // New York time
          tzset();
          timeinfo = localtime(&now);
          char timeStringBuff[40];
          strftime(timeStringBuff, sizeof(timeStringBuff), "Updated: %H:%M:%S", timeinfo);
          
          EPD.fontscale = 1;
          EPD.DrawUTF(270, 10, timeStringBuff);
        }

        // Draw battery status
        int rawBattery = analogRead(BATTERY_PIN);
        float batteryVoltage = rawBattery * BATTERY_VOLTAGE_MULTIPLIER;
        
        // Simple percentage calculation for 3.7V nominal LiPo (4.2V max, ~3.3V min)
        int batteryPercent = (batteryVoltage - 3.3) / (4.2 - 3.3) * 100;
        if (batteryPercent > 100) batteryPercent = 100;
        if (batteryPercent < 0) batteryPercent = 0;

        char batteryStringBuff[40];
        // Temporarily displaying the raw analog value (R:%d) to help with calibration
        snprintf(batteryStringBuff, sizeof(batteryStringBuff), "Bat: %.1fV (%d%%) R:%d", batteryVoltage, batteryPercent, rawBattery);
        EPD.fontscale = 1;
        EPD.DrawUTF(250, 10, batteryStringBuff);

        EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer, 1);
        Serial.println("Updating display...");
        EPD.EPD_Update();
        EPD.ReadBusy_long();
        EPD.deepsleep();
        Serial.println("Display update complete.");
      } else {
        Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        // Display an error message on the e-ink screen if HTTP request fails
        displaySimpleMessage("HTTP GET failed");
      }
    } else {
      Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
      // Display an error message on the e-ink screen if HTTP request fails
      displaySimpleMessage("HTTP GET failed");
    }
    http.end();
  } else {
    Serial.println("Unable to connect to API");
    // Display an error message on the e-ink screen if API connection fails
    displaySimpleMessage("API connection failed");
  }
}

void setup() {
  Serial.begin(460800);
  Serial.println("\n\nStarting custom firmware...");

  SPIFFS.begin();
  EPD.SetFS(&SPIFFS);

  // 1. Initialize Display
  Serial.println("Initializing e-ink display...");
  EPD.EPD_Set_Model(OPM42); 
  displaySimpleMessage("Starting...");

  // 2. Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
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


  
  // Initial update
  updateDisplay();
}

void loop() {
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    updateDisplay();
  }
}