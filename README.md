# MTA E-Ink Tricolor Subway Display

This project is a custom ESP8266-powered e-paper display that shows live New York City subway arrival information for the N train at Fort Hamilton Parkway, northbound. It connects to Wi-Fi, fetches data from the subwaynow API, draws the status and upcoming departures on a black-and-white e-paper panel, and refreshes automatically on a timed loop. This is a personal project aimed to streamline the transit experience for my apartment. 

The firmware is built with PlatformIO and uses the Arduino framework. It includes a local copy of the EPaperDrive library and a small set of dependency libraries for graphics, JSON parsing, and Wi-Fi communication.

## Features

- ESP8266 NodeMCU v2 firmware using PlatformIO
- Real-time subway data for the N train
- Service status and irregularity summary display
- Upcoming trip countdowns for northbound service
- Last update timestamp, battery level, and uptime
- Wi-Fi reconnect and error-state display handling
- Full-screen e-paper refresh with low-power-friendly updates
- Designed around the OPM42 e-paper model from the bundled EPaperDrive library

## Hardware

### Core board

- ESP8266 NodeMCU v2
- Operating voltage: 3.3V logic
- Flash and filesystem configured for SPIFFS

### Display

- E-paper module: OPM42
- Driver library model target: `OPM42`
- Panel type: monochrome / black-and-white in current firmware
- The code notes that the red channel is currently problematic, so the project currently runs in black-and-white mode

### Wiring used in firmware

The pin mapping is defined in `src/main.cpp`:

- `CS` = GPIO15
- `RST` = GPIO2
- `DC` = GPIO0
- `BUSY` = GPIO4
- `CLK` = GPIO14
- `DIN` = GPIO13
- Battery sense = `A0`

## Software stack

- PlatformIO
- Arduino framework
- ESP8266 core
- Adafruit GFX Library
- Adafruit BusIO
- ArduinoJson v6
- Local library: `EPaperDrive-main` (bundled in the workspace)

## Project layout

- `src/main.cpp` — main firmware logic and rendering
- `platformio.ini` — PlatformIO project configuration
- `data/` — local data files and fonts used by the display system
- `EPaperDrive-main/` — local EPaperDrive library source and documentation
- `lib/` — optional library directory for custom Arduino libs
- `include/` — project header includes
- `test/` — test folder placeholder

## Build and run

### 1. Install dependencies

Install PlatformIO and its VS Code extension or use the PlatformIO CLI:

```bash
pio run
```

### 2. Build the firmware

From the project root:

```bash
pio run
```

### 3. Upload to the device

```bash
pio run --target upload
```

### 4. Monitor serial output

```bash
pio device monitor
```

## Configuration

### Wi-Fi credentials

The firmware currently contains hardcoded Wi-Fi values in `src/main.cpp`:

```cpp
const char* ssid = "HStark-NY";
const char* password = "116208818";
```

You must change these to match your local network before deployment.

### API endpoints

The project calls the `subwaynow.app` API:

```cpp
const char* api_url = "https://api.subwaynow.app/stops/N03";
const char* status_api_url = "https://api.subwaynow.app/routes/N";
```

This is tied to the N train service data and the Fort Hamilton Parkway station stop ID `N03`.

### Update interval

The refresh period is set in `src/main.cpp`:

```cpp
const unsigned long updateInterval = 60000; // 60 seconds
```

This means the display refreshes once per minute when the device remains active.

## Functional behavior

During startup, the firmware:

1. Initializes serial logging
2. Mounts SPIFFS
3. Sets the display model to `OPM42`
4. Connects to Wi-Fi
5. Fetches service status from the subway API
6. Fetches upcoming northbound train arrival times
7. Draws the display content on the e-paper panel
8. Sleeps after transferring the image
9. Repeats the update loop every minute

The display primarily shows:

- route name and location header
- current service status
- service summary / alerts
- next arrival times
- updated timestamp
- battery percentage and uptime

## Important notes

### API and network assumptions

This project is intentionally specialized for the subwaynow API and a specific station/route. If the upstream API structure or endpoint changes, the JSON parsing logic will need to be updated.

### ESP8266 memory constraints

The firmware is carefully tuned for the ESP8266's limited RAM. It uses:

- filtered JSON parsing
- moderate buffer sizing
- HTTP/1.0 mode
- reduced data extraction to only required fields

This is important because the display code and JSON parsing happen in a constrained environment.

### Red-channel limitation

The code comments explicitly note that the red channel is not stable or functional for the current display configuration. As a result, it currently renders only in black/white mode.

## Local library notes

The workspace includes a copy of EPaperDrive under `EPaperDrive-main/`. This library provides the display driver methods and configuration for supported e-paper panels, including the `OPM42` model used here.

The project also relies on the library’s fonts stored in the `data/` directories.

## Example usage workflow

1. Connect the ESP8266 to the e-paper panel using the pins above.
2. Set your Wi-Fi SSID/password in `src/main.cpp`.
3. Ensure the API is reachable from the device.
4. Build and upload the firmware.
5. Watch the display update with current service and arrival information.

## Troubleshooting

### Device does not connect to Wi-Fi

- Verify the SSID and password in `src/main.cpp`
- Check GPIO0 / boot mode requirements on the ESP8266
- Confirm the antenna and board power supply are stable

### Display remains blank

- Confirm the correct e-paper model is set with `EPD_Set_Model(OPM42)`
- Verify the SPI and power wiring
- Check BUSY, RST, DC, CS, CLK, DIN pin connections
- Confirm SPIFFS is mounted correctly and fonts are available

### API request fails

- Confirm the device has internet access
- Verify the URL still responds
- Check the serial monitor for HTTP errors
- Review the upstream JSON response format

### Memory or parsing errors

- Review the filtered JSON document size and buffer settings
- Reduce the number of fields requested
- Keep the refresh interval reasonable
- Check for a truncated or malformed API payload

## Project status

This is a personal hardware project for a specific MTA display use case. It is functional for its intended station and route setup, but some values are hardcoded and should be adjusted for reuse in another environment.

## License

This project includes a bundled library from the EPaperDrive project, which is distributed with its own license and documentation in the included library folder. The main firmware itself is a custom project and should be treated as project-specific code unless otherwise stated by the original authors.

## Credits

This project builds on the EPaperDrive e-paper driver library and uses the subwaynow API for live subway data.
