/*
 * XYZ-Switch firmware
 * On first boot: AP "XYZ-Switch" and web setup. Then connect to router.
 */
#include "ConfigPortal.h"

const char* AP_NAME = "XYZ-Switch";

#define SW1_GPIO 3
#define SW2_GPIO 4
#define DEBOUNCE_MS 50

static bool sw1_last, sw2_last;
static unsigned long sw1_ts, sw2_ts;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nXYZ-Switch");

  pinMode(SW1_GPIO, INPUT_PULLUP);
  pinMode(SW2_GPIO, INPUT_PULLUP);

  if (ConfigPortal.tryStoredWifi()) {
    Serial.println("Connected to WiFi.");
    Serial.println(String("Switches: SW1=GPIO") + SW1_GPIO + " SW2=GPIO" + SW2_GPIO + " (LOW=pressed)");
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
      Serial.println(s1 ? "SW1 pressed" : "SW1 released");
    }
  } else
    sw1_ts = 0;

  if (s2 != sw2_last) {
    if (sw2_ts == 0) sw2_ts = millis();
    else if ((unsigned long)(millis() - sw2_ts) >= DEBOUNCE_MS) {
      sw2_last = s2;
      sw2_ts = 0;
      Serial.println(s2 ? "SW2 pressed" : "SW2 released");
    }
  } else
    sw2_ts = 0;
}

void loop() {
  if (ConfigPortal.isActive()) {
    ConfigPortal.handleClient();
  } else {
    pollSwitches();
  }
}
