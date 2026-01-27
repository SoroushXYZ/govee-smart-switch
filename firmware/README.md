# XYZ-Switch Firmware (ESP32)

Modular Arduino sketch for the ESP32. Uses only the ESP32 Arduino core (no extra libraries).

## Behavior

- **First boot (no stored WiFi):** ESP creates AP `XYZ-Switch`. Connect to it; a captive portal redirects to the setup page. Enter router SSID and password, then Connect. On success, credentials are saved and the page suggests closing after 3 seconds; the ESP keeps the AP up ~5 s then switches to STA. On failure, an error is shown with a link to try again.
- **Later boots (stored WiFi):** Tries to connect with saved credentials. If it fails, you need to clear NVS (e.g. erase flash or add a “reset config” action) to get back to the portal; for now, re-flashing or full erase brings back the setup portal.

## Build and upload

1. Install [ESP32 Arduino support](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).
2. In Arduino IDE: **File → Open** → `firmware/XYZSwitch/XYZSwitch.ino`.
3. **Tools → Board** → your ESP32 (e.g. ESP32 Dev Module).
4. **Tools → Port** → your ESP32’s port.
5. **Sketch → Upload**.

## Layout

```
XYZSwitch/
  XYZSwitch.ino   - setup/loop, starts stored WiFi or ConfigPortal
  ConfigPortal.h  - ConfigPortal API
  ConfigPortal.cpp - AP, DNS captive portal, WebServer, NVS save/load, / and /connect
```
