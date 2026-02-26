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

/**
 * Program Summary:
 * This software drives an e-ink display (ESP8266-based) to show real-time NYC Subway arrival times 
 * for the N train at Fort Hamilton Pkwy (Stop ID: N03).
 * 
 * Key Features:
 * - Connects to WiFi to fetch data from the subwaynow.app API.
 * - Displays service status (e.g., "Good Service", "Delays") and alert summaries.
 * - Lists upcoming northbound train arrivals in minutes.
 * - Shows last update time and battery status.
 * - Optimized for ESP8266 memory constraints using JSON filtering.
 * 
 * Hardware:
 * - ESP8266 Microcontroller
 * - E-ink Display (utilizing EPaperDrive library, configured for model OPM42)
 * 
 * Note:
 * - The display is currently restricted to black and white mode due to issues with the red channel.
 * - The code uses specific memory optimizations (buffer sizing, JSON filtering) to handle HTTPS responses on the constrained ESP8266.
 */

// --- WiFi Configuration ---
const char* ssid     = "HStark-NY";
const char* password = "116208818";

// --- API Configuration ---
const char* api_url = "https://api.subwaynow.app/stops/N03";
const char* status_api_url = "https://api.subwaynow.app/routes/N";

String northStatus = "Unknown";
String northSummary = "";

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

void checkServiceStatus() {
  Serial.println("Checking service status...");
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  if (!client) {
     Serial.println("Unable to create secure client");
     return;
  }
  
  client->setInsecure();
  // Using 4KB rx buffer to save heap, expecting the status field to be early in the JSON.
  // 8KB might be pushing the limits if we have other things allocated.
  client->setBufferSizes(4096, 512); 
  client->setTimeout(10000);
  
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(10000);
  
  // For the status check, we only care about the "direction_statuses" and "service_irregularity_summaries" fields, so we can use a filter to minimize memory usage.
  if (http.begin(*client, status_api_url)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      StaticJsonDocument<200> filter;
      filter["direction_statuses"]["north"] = true;
      filter["service_irregularity_summaries"]["north"] = true;
      
      DynamicJsonDocument doc(1536); 
      // If the input is truncated, we might still have the data we need in the doc.
      DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
      
      // If IncompleteInput, we might still have partial data.
      if (!error || error == DeserializationError::IncompleteInput) {
        if (doc.containsKey("direction_statuses")) {
            northStatus = doc["direction_statuses"]["north"].as<String>();
        }
        if (doc.containsKey("service_irregularity_summaries")) {
            northSummary = doc["service_irregularity_summaries"]["north"].as<String>();
        }
        
        if (northStatus == "null") northStatus = "Unknown";
        if (northSummary == "null") northSummary = "";
        
        Serial.println("Status: " + northStatus);
        Serial.println("Summary: " + northSummary);
      } else {
        Serial.print("Status deserializeJson() failed: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.printf("Status HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("Unable to connect to Status API");
  }
}

// Draw the MTA N-train subway circle icon: solid black circle with a white "N".
// The display axes: x = vertical (rows from top), y = horizontal (cols from left).
// Placed in the blank right-center area of the 400×300 display.
void drawNTrainIcon() {
  const int cx = 155;  // center row  (vertical axis)
  const int cy = 240;  // center col  (horizontal axis)
  const int r  = 52;   // circle radius

  // 1. Solid black filled circle
  EPD.DrawCircle(cx, cy, r, true);

  // FONT32 glyphs are 32 px tall. "N" is ~20 px wide at scale 1.
  // Top-left of glyph placed at (cx-16, cy-10) so it's centered in the circle.
  const int gx = cx - 16;  // glyph top row
  const int gy = cy - 10;  // glyph left col

  // 2. Inverse the N bounding rectangle (all-black from filled circle → all-white)
  EPD.Inverse(gx - 2, gx + 34, gy - 2, gy + 22);

  // 3. Draw "N" → black N on now-white rectangle
  EPD.SetFont(FONT32);
  EPD.fontscale = 1;
  EPD.DrawUTF(gx, gy, "N");

  // 4. Inverse the same rectangle again → white N on black rectangle,
  //    which is fully inside the circle so it merges seamlessly with the black fill.
  EPD.Inverse(gx - 2, gx + 34, gy - 2, gy + 22);

  // Restore drawing state
  EPD.SetFont(FONT12);
  EPD.fontscale = 2;
}

void drawTrainData(DynamicJsonDocument& doc) {
  EPD.EPD_init_Full();
  EPD.clearbuffer();
  EPD.fontscale = 2;
  EPD.SetFont(FONT12);

  // Layout: Header with Status
  String headerText = "N Train: " + (northStatus.length() > 0 ? northStatus : "Unknown");
  EPD.DrawUTF(10, 10, headerText);
  
  // Station Name
  EPD.DrawUTF(40, 10, "Fort Hamilton Pkwy");

  // N-train circle icon in the blank right-center area of the display
  drawNTrainIcon();

  int yPos = 80;
  
  // Check for service alerts
  if (northSummary.length() > 0 && northSummary != "null" && northSummary != "") {
    // If there's an alert, display it prominently
    // We might need to reduce font size or truncate if it's very long
    EPD.fontscale = 1;
    // Simple truncation for now to fit one line, or maybe two
    // A full summary can be long. Let's try to fit 2 lines if needed.
    // 4.2 inch fits roughly 30-40 chars per line at scale 1?
    if (northSummary.length() > 35) {
       EPD.DrawUTF(yPos, 10, northSummary.substring(0, 35));
       yPos += 20;
       int len = northSummary.length();
       int end = len < 70 ? len : 70;
       EPD.DrawUTF(yPos, 10, northSummary.substring(35, end));
    } else {
       EPD.DrawUTF(yPos, 10, northSummary);
    }
    yPos += 30; // Spacing after alert
    EPD.fontscale = 2; // Restore font scale for times
  }

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
    
    // Set Timezone
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1); 
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
  
  // Check service status first
  checkServiceStatus();

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