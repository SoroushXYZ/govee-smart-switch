#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>

class WifiConfigClass {
public:
  // Try to connect using stored credentials. Returns true if connected.
  bool tryStoredWifi();

  // Save SSID and password to NVS.
  void setWifiCredentials(const String& ssid, const String& pass);

  // Attempt to connect. Returns true if connected within timeout. Does not save.
  bool tryConnect(const String& ssid, const String& pass);

  // Scan; dedupe by SSID (best RSSI), sort by RSSI descending. Returns count.
  int scanNetworks();

  // Access last scan. i in [0, scanNetworks()-1].
  String getScannedSsid(int i);
  int getScannedRssi(int i);
};

extern WifiConfigClass WifiConfig;

#endif
