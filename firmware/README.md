# XYZ-Switch Firmware (ESP32)

Modular Arduino sketch for the ESP32.

**Libraries:** ESP32 Arduino core, **ArduinoJson** (Sketch → Include Library → Manage Libraries → search “ArduinoJson”).

## Behavior

- **WiFi:** On boot, connects using stored credentials (NVS). If none or connection fails, use the Serial menu: **4** WiFi setup → scan lists networks by signal strength; pick by number or type SSID, then password; saves only on successful connect.
- **Serial (USB):** At 115200, press **Enter** for the menu: **1** Set API key, **2** List devices, **3** Select/deselect devices, **4** WiFi setup, **0** Back. All settings stored in NVS.
- **SW1 (GPIO3):** Toggles all selected Govee devices. SW2 (GPIO4) is reserved.

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
  WifiConfig.h/cpp - NVS (ssid, pass), tryStoredWifi, setWifiCredentials, tryConnect
  Govee.h / Govee.cpp - Govee API (NVS: apikey, selected devices, lastOn), fetch/list, control, toggle
```
