/*
 * XYZ-Switch firmware
 * On first boot: AP "XYZ-Switch" and web setup. Then connect to router.
 * Serial (USB): press Enter for menu — API key, list devices, select/deselect. SW1 toggles lights.
 */
#include "ConfigPortal.h"
#include "Govee.h"

const char* AP_NAME = "XYZ-Switch";

#define SW1_GPIO 3
#define SW2_GPIO 4
#define DEBOUNCE_MS 50

static bool sw1_last, sw2_last;
static unsigned long sw1_ts, sw2_ts;

// Serial menu: 0=normal, 1=main, 2=wait API key, 3=wait select number
static int menuState = 0;
static String lineBuf;

static void showMainMenu() {
  Serial.println("\n--- Govee ---");
  Serial.println("1) Set API key");
  Serial.println("2) List devices");
  Serial.println("3) Select / deselect devices");
  Serial.println("0) Back");
  Serial.print("> ");
}

static void showSelectMenu() {
  int n = Govee.fetchDevices();
  if (n == 0) {
    Serial.println("No devices (set API key and WiFi first).");
    Serial.print("0=back> ");
    return;
  }
  Serial.println("\n  [x]=selected");
  for (int i = 0; i < n; i++) {
    String d, m, name;
    Govee.getDeviceAt(i, d, m, name);
    bool sel = Govee.isSelected(d, m);
    Serial.println(String("  ") + (i + 1) + ") [" + (sel ? "x" : " ") + "] " + name);
  }
  Serial.print("Number to toggle, 0=back> ");
}

static void processLine() {
  lineBuf.trim();
  if (menuState == 1) {
    if (lineBuf == "1") {
      Serial.print("API key> ");
      menuState = 2;
    } else if (lineBuf == "2") {
      int n = Govee.fetchDevices();
      if (n == 0) Serial.println("No devices or no API key.");
      else
        for (int i = 0; i < n; i++) {
          String d, m, name;
          Govee.getDeviceAt(i, d, m, name);
          Serial.println(String("  ") + (i + 1) + ") " + name + "  " + d + "  " + m);
        }
      showMainMenu();
    } else if (lineBuf == "3") {
      showSelectMenu();
      menuState = 3;
    } else if (lineBuf == "0") {
      menuState = 0;
      Serial.println("(menu closed; press Enter to reopen)");
    } else
      showMainMenu();
  } else if (menuState == 2) {
    if (lineBuf.length() > 0) {
      Govee.setApiKey(lineBuf);
      Serial.println("API key saved.");
    }
    menuState = 1;
    showMainMenu();
  } else if (menuState == 3) {
    int v = lineBuf.toInt();
    int n = Govee.getLastListCount();
    if (v == 0) {
      menuState = 1;
      showMainMenu();
    } else if (v >= 1 && v <= n) {
      String d, m, name;
      Govee.getDeviceAt(v - 1, d, m, name);
      Govee.toggleSelection(d, m);
      showSelectMenu();
    } else
      showSelectMenu();
  }
}

static void serialMenuStep() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (menuState == 0) {
        menuState = 1;
        lineBuf = "";
        showMainMenu();
      } else {
        processLine();
        lineBuf = "";
      }
    } else if (c >= 32 && c < 127 && lineBuf.length() < 128)
      lineBuf += c;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nXYZ-Switch");

  pinMode(SW1_GPIO, INPUT_PULLUP);
  pinMode(SW2_GPIO, INPUT_PULLUP);

  if (ConfigPortal.tryStoredWifi()) {
    Serial.println("Connected to WiFi.");
    Serial.println(String("SW1=GPIO") + SW1_GPIO + " (toggle lights)  SW2=GPIO" + SW2_GPIO);
    Serial.println("Press Enter for Govee menu (API key, devices).");
    return;
  }

  Serial.print("Starting config portal: ");
  Serial.println(AP_NAME);
  ConfigPortal.begin(AP_NAME);
}

static void pollSwitches() {
  bool s1 = (digitalRead(SW1_GPIO) == LOW);
  bool s2 = (digitalRead(SW2_GPIO) == LOW);

  if (s1 != sw1_last) {
    if (sw1_ts == 0) sw1_ts = millis();
    else if ((unsigned long)(millis() - sw1_ts) >= DEBOUNCE_MS) {
      sw1_last = s1;
      sw1_ts = 0;
      if (s1) Govee.toggleLights();
    }
  } else
    sw1_ts = 0;

  if (s2 != sw2_last) {
    if (sw2_ts == 0) sw2_ts = millis();
    else if ((unsigned long)(millis() - sw2_ts) >= DEBOUNCE_MS) {
      sw2_last = s2;
      sw2_ts = 0;
    }
  } else
    sw2_ts = 0;
}

void loop() {
  if (ConfigPortal.isActive()) {
    ConfigPortal.handleClient();
  } else {
    pollSwitches();
    serialMenuStep();
  }
}
