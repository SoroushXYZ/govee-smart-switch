#include "WifiConfig.h"
#include <WiFi.h>
#include <Preferences.h>
#include <string.h>

#define NVS_NS "wifi"
#define MAX_SCAN 24
#define K_SSID "ssid"
#define K_PASS "pass"
#define CONNECT_TIMEOUT_MS 15000

bool WifiConfigClass::tryStoredWifi() {
  Preferences prefs;
  prefs.begin(NVS_NS, true);
  String ssid = prefs.getString(K_SSID, "");
  String pass = prefs.getString(K_PASS, "");
  prefs.end();
  if (ssid.length() == 0) return false;
  return tryConnect(ssid, pass);
}

void WifiConfigClass::setWifiCredentials(const String& ssid, const String& pass) {
  Preferences prefs;
  prefs.begin(NVS_NS, false);
  prefs.putString(K_SSID, ssid);
  prefs.putString(K_PASS, pass);
  prefs.end();
}

void WifiConfigClass::clearAll() {
  Preferences prefs;
  prefs.begin(NVS_NS, false);
  prefs.clear();
  prefs.end();
}

bool WifiConfigClass::tryConnect(const String& ssid, const String& pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t) < CONNECT_TIMEOUT_MS)
    delay(200);
  return (WiFi.status() == WL_CONNECTED);
}

namespace {
  struct { char ssid[33]; int rssi; } scanList[MAX_SCAN];
  int scanCount = 0;
}

int WifiConfigClass::scanNetworks() {
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks();
  scanCount = 0;
  for (int i = 0; i < n && scanCount < MAX_SCAN; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue;
    int r = WiFi.RSSI(i);
    int j = 0;
    for (; j < scanCount; j++)
      if (s.equals(scanList[j].ssid)) {
        if (r > scanList[j].rssi) scanList[j].rssi = r;
        break;
      }
    if (j == scanCount) {
      s.toCharArray(scanList[scanCount].ssid, 33);
      scanList[scanCount].rssi = r;
      scanCount++;
    }
  }
  for (int i = 0; i < scanCount - 1; i++)
    for (int j = 0; j < scanCount - 1 - i; j++)
      if (scanList[j].rssi < scanList[j + 1].rssi) {
        struct { char ssid[33]; int rssi; } t;
        memcpy(&t, &scanList[j], sizeof(t));
        memcpy(&scanList[j], &scanList[j + 1], sizeof(t));
        memcpy(&scanList[j + 1], &t, sizeof(t));
      }
  return scanCount;
}

String WifiConfigClass::getScannedSsid(int i) {
  if (i < 0 || i >= scanCount) return "";
  return String(scanList[i].ssid);
}

int WifiConfigClass::getScannedRssi(int i) {
  if (i < 0 || i >= scanCount) return 0;
  return scanList[i].rssi;
}

WifiConfigClass WifiConfig;
