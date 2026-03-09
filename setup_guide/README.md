# XYZ-Switch setup guide

This folder contains everything you need to set up and configure your XYZ-Switch (ESP32) for Govee lights.

---

## Before you start

### 1. Use the correct USB port and switch

The device has **two USB-C ports**:

- **Left port** — **charging only**. Do not use this for setup or programming.
- **Right port** — **device setup and programming**. Use this one to connect to your computer.

There is a **physical switch in the middle** of the device:

- **OFF (left)** — use this position when connecting to your computer to program or configure the device.
- **ON (right)** — normal operation (e.g. after setup).

**To configure the device:** Set the switch to **OFF (left)**, connect a USB cable from the **right** USB-C port to your computer, then run the configuration script.

### 2. Serial is only available when the device is awake

The ESP32 spends most of its time in **deep sleep** to save power. While it is sleeping, the serial connection is **not available**.

- **To make the serial port available:** **Press the button** on the device. This wakes it up. After a short time you will see the serial port in the list and you can connect.
- If you run the configuration script and select a port but nothing happens, **press the button on the device** and try again (you can refresh the port list with **0** and then select the port again).

---

## Get a Govee API key

You need a Govee API key so the device can talk to the Govee cloud and control your lights.

### Method — From the Govee Home app (recommended)

1. Open the **Govee Home** app on your phone.
2. Tap **Profile** (👤 icon).
3. Tap the **Settings** gear (⚙️) in the top-right.
4. Tap **“Apply for API Key”**.
5. Fill in:
   - **Name** (e.g. “XYZ-Switch” or your name).
   - **Reason** (e.g. “Home automation” or “ESP32 integration”).
6. Submit.

Govee will email you the API key. Keep it handy; you’ll enter it in the configuration tool.

---

## Install Python dependencies

From this folder (`setup_guide`), create a virtual environment and install the required packages:

```bash
cd setup_guide
python3 -m venv venv
source venv/bin/activate   # On macOS/Linux
# On Windows:  venv\Scripts\activate
pip install -r requirements.txt
```

Required packages (in `requirements.txt`):

- **pyserial** — list serial ports and talk to the ESP32.

---

## Run the configuration tool

1. **Wake the device** (press the button) so the serial port appears.
2. From the `setup_guide` folder, with the virtual environment activated:

   ```bash
   python3 configure_esp.py
   ```

3. The script will list **available serial ports**. Choose your ESP32 by number.
   - **0** — refresh the list (e.g. after plugging in the device or pressing the button).
   - **q** — quit.

4. After you select a port, you are connected to the ESP32’s **own Serial console**.
   - Press **Enter** to show the ESP32’s menu (if it isn’t already visible).
   - Type menu choices directly (e.g. `4` for WiFi setup).
   - To exit the console: type **`~.`** on its own line, or press **Ctrl+C**.

---

## Recommended setup order

1. **WiFi (4)** — so the device can reach the internet and Govee.
2. **Set API key (1)** — so the device can list and control your Govee devices.
3. **Select devices (3)** — choose which lights the physical button will turn on/off.

After that, set the middle switch to **ON**, disconnect USB if you like, and use the **button** to toggle the selected lights.

---

## Troubleshooting

- **No serial ports listed** — Use the **right** USB-C port and set the middle switch to **OFF**. Press the button to wake the device, then choose **0** to refresh the list.
- **Port opens but nothing happens** — The device may be in deep sleep. Press the button on the device and try the same port again (or refresh with **0** and reselect).
- **WiFi or API key not saving** — Make sure you completed the prompts (e.g. password for WiFi, full API key). Use **Raw serial (6)** if you want to see the device’s own menu and prompts directly.
