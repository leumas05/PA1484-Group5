## Introduction

* The ESP32-based system connects to Wi-Fi and acquires data from the SMHI APIs for current conditions, 7-day forecasts, and historical trends for a selected Swedish city. It then displays everything using LVGL charts, icons, and text on the touchscreen.

* The device solves the hassle of chasing weather data across multiple apps or browser tabs. Automatic SMHI API polling keeps information fresh without manual refreshes, turning the unit into a dedicated always-on weather screen.

* The project uses an ESP32 with touchscreen, the SMHI API for weather data, PlatformIO for build/deployment, and LVGL as the graphics framework.

* The main functionality is presenting SMHI weather data (current, forecast, and historical) on the LilyGo AMOLED: it periodically fetches data for the selected city/parameter and renders it with charts for quick interpretation.

## Getting started
### Prerequisites
- A computer with Git and Visual Studio Code installed (Windows, macOS, or Linux)
- The PlatformIO Core CLI (automatically installed with the VS Code PlatformIO extension)
- Python 3.10+ on the system path (PlatformIO uses it to build ESP32 projects)
- USB-C cable capable of data transfer
- CP210x/CH9102 USB-to-UART drivers (required for the LilyGo T-Display AMOLED)
- Wi-Fi credentials that the device can use at runtime (edit `project/credentials.h`)

### Installation
1. **Clone the repository**
	```powershell
	git clone https://github.com/leumas05/PA1484-Group5.git
	cd PA1484-Group5
	```
2. **Open the folder in VS Code** and let the PlatformIO extension index the project; it automatically reads `platformio.ini` and installs listed libraries (LVGL, Arduino_GFX, etc.).
3. **Configure Wi-Fi and API parameters** by editing `project/credentials.h` (set SSID, password, and any optional tokens) so the firmware can reach the SMHI endpoints.
4. **Connect the LilyGo T-Display AMOLED** via USB-C and confirm the correct serial port appears (Device Manager on Windows or `ls /dev/tty.*` on macOS/Linux). If the port is missing, install the CP210x/CH9102 driver mentioned above.
5. **Build the firmware** for the provided environment:
	```powershell
	pio run --environment T-Display-AMOLED
	```
6. **Flash the device** once the build succeeds:
	```powershell
	pio run --target upload --environment T-Display-AMOLED
	```
7. **Monitor logs (optional)** to verify Wi-Fi connection and SMHI synchronization:
	```powershell
	pio device monitor --environment T-Display-AMOLED
	```
	Press `Ctrl+T` then `Ctrl+H` in the monitor to toggle timestamps if needed. When the display shows live data, you are ready to iterate on UI or logic changes.

## Building and running
- **Build & Upload:** Use `pio run --environment T-Display-AMOLED` followed by `pio run --target upload --environment T-Display-AMOLED`. PlatformIO handles compilation, dependency resolution, and flashing over USB.
- **Startup sequence:** After reset or power-on, the ESP32 initializes LVGL, shows a boot tile, connects to the configured Wi-Fi, and starts periodic SMHI API polling. Status text indicates Wi-Fi progress.
- **Runtime arguments/modes:** The firmware reads the selected parameter/city from NVS (Preferences). You can change these on Tile #2 using dropdowns; pressing the Save button persists the choice so subsequent boots honor it.
- **Operation:** Swipe between LVGL tiles to access:
	- Tile #1: summary metrics plus Wi-Fi indicator
	- Tile #2: selector UI for city/parameter with reset and save actions
	- Tile #3: 7-day forecast chart with LVGL weather icons mapped to SMHI symbol codes
	- Tile #4: historical temperature graph with slider-controlled window
	Interaction is touch-based; long-press gestures are not used, so taps and swipes suffice.

## Features
- [x] US1.1C: View current weather conditions and icons for a selected Swedish city.
- [x] US1.2C: Display a seven-day temperature forecast with LVGL charts and SMHI symbol icons.
- [x] US2.1: Store preferred city/parameter selections so they persist across reboots.
- [x] US3.1: Show historical temperature data with a scrollable/slidable window.
- [ ] US3.2D: Provide configurable weather alerts/notifications on abnormal conditions.
- [ ] US4.1: Allow editing of SMHI API endpoints or adding custom data sources from the UI.

