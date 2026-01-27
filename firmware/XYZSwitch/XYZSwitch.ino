/*
 * XYZ-Switch firmware
 * On first boot: AP "XYZ-Switch" and web setup. Then connect to router.
 */
#include "ConfigPortal.h"

const char* AP_NAME = "XYZ-Switch";

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nXYZ-Switch");

  if (ConfigPortal.tryStoredWifi()) {
    Serial.println("Connected to WiFi.");
    return;
  }

  Serial.print("Starting config portal: ");
  Serial.println(AP_NAME);
  ConfigPortal.begin(AP_NAME);
}

void loop() {
  if (ConfigPortal.isActive()) {
    ConfigPortal.handleClient();
  }
}
