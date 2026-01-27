# XYZ-Switch Firmware (ESP32)

Modular Arduino sketch for the ESP32.

**Libraries:** ESP32 Arduino core, **ArduinoJson** (Sketch → Include Library → Manage Libraries → search “ArduinoJson”).

## Behavior

- **First boot (no stored WiFi):** ESP creates AP `XYZ-Switch`. Connect to it; a captive portal redirects to the setup page. Enter router SSID and password, then Connect. On success, credentials are saved and the page suggests closing after 3 seconds; the ESP keeps the AP up ~5 s then switches to STA. On failure, an error is shown with a link to try again.
- **Later boots (stored WiFi):** Tries to connect with saved credentials. If it fails, you need to clear NVS (e.g. erase flash or add a “reset config” action) to get back to the portal; for now, re-flashing or full erase brings back the setup portal.
- **Serial (USB):** At 115200, press **Enter** to open the Govee menu: **1** Set API key, **2** List devices, **3** Select/deselect devices (by number), **0** Back. Selections and API key are stored in NVS.
- **SW1 (GPIO3):** Toggles all selected Govee devices. Flips stored on/off and sends to every selected device; no per-device state. SW2 (GPIO4) is reserved.

## Build and upload

1. Install [ESP32 Arduino support](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).
2. In Arduino IDE: **File → Open** → `firmware/XYZSwitch/XYZSwitch.ino`.
3. **Tools → Board** → your ESP32 (e.g. ESP32 Dev Module).
4. **Tools → Port** → your ESP32’s port.
5. **Sketch → Upload**.

## Layout

```
XYZSwitch/
  XYZSwitch.ino   - setup/loop, switches, serial menu, SW1→Govee.toggleLights
  ConfigPortal.h  - ConfigPortal API
  ConfigPortal.cpp - AP, DNS captive portal, WebServer, NVS save/load
  Govee.h / Govee.cpp - Govee API (NVS: apikey, selected devices, lastOn), fetch/list, control, toggle
```
