#include "WifiConfig.h"
#include <WiFi.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <time.h>
#include <string.h>

#define NVS_NS "wifi"
#define MAX_SCAN 24
#define K_SSID "ssid"
#define K_PASS "pass"
#define K_CHANNEL "ch"
#define K_BSSID "bssid"
#define K_IP "ip"
#define K_GATEWAY "gw"
#define K_SUBNET "sn"
#define K_LAST_DHCP "ld"
#define CONNECT_TIMEOUT_MS 15000
#define CONNECT_POLL_MS 30
#define NTP_SERVER "pool.ntp.org"
#define SECONDS_PER_DAY 86400

// Set to 0 to disable static IP and always use DHCP (use if static-IP path causes issues).
#ifndef USE_STATIC_IP
#define USE_STATIC_IP 1
#endif

namespace {
  bool tryConnectInternal(const String& ssid, const String& pass, int32_t channel, const uint8_t* bssid,
                         const String* staticIp, const String* gateway, const String* subnet) {
    WiFi.mode(WIFI_STA);
#if USE_STATIC_IP
    if (staticIp != nullptr && gateway != nullptr && subnet != nullptr &&
        staticIp->length() > 0 && gateway->length() > 0 && subnet->length() > 0) {
      IPAddress ip, gw, sn;
      if (ip.fromString(*staticIp) && gw.fromString(*gateway) && sn.fromString(*subnet))
        WiFi.config(ip, gw, sn, gw);
    }
#endif
    if (channel >= 1 && channel <= 14 && bssid != nullptr)
      WiFi.begin(ssid.c_str(), pass.c_str(), channel, bssid);
    else
      WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t) < CONNECT_TIMEOUT_MS)
      delay(CONNECT_POLL_MS);
    return (WiFi.status() == WL_CONNECTED);
  }

  void saveConnectionInfo() {
    if (WiFi.status() != WL_CONNECTED) return;
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) return;
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putUChar(K_CHANNEL, ap.primary);
    prefs.putBytes(K_BSSID, ap.bssid, 6);
    prefs.end();
  }

  void saveStaticFromCurrent() {
    if (WiFi.status() != WL_CONNECTED) return;
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putString(K_IP, WiFi.localIP().toString());
    prefs.putString(K_GATEWAY, WiFi.gatewayIP().toString());
    prefs.putString(K_SUBNET, WiFi.subnetMask().toString());
    prefs.end();
  }

  void saveLastDhcpEpoch(uint32_t epoch) {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putULong(K_LAST_DHCP, (unsigned long)epoch);
    prefs.end();
  }

  uint32_t getLastDhcpEpoch() {
    Preferences prefs;
    prefs.begin(NVS_NS, true);
    unsigned long v = prefs.getULong(K_LAST_DHCP, 0);
    prefs.end();
    return (uint32_t)v;
  }

  bool ensureNtpThenSaveEpoch() {
    configTime(0, 0, NTP_SERVER);
    struct tm tm;
    for (int i = 0; i < 100; i++) {
      if (getLocalTime(&tm, 100)) {
        time_t now = mktime(&tm);
        if (now >= (time_t)SECONDS_PER_DAY) {
          saveLastDhcpEpoch((uint32_t)now);
          return true;
        }
      }
      delay(50);
    }
    return false;
  }
}

bool WifiConfigClass::tryStoredWifi() {
  Preferences prefs;
  prefs.begin(NVS_NS, true);
  String ssid = prefs.getString(K_SSID, "");
  String pass = prefs.getString(K_PASS, "");
  uint8_t channel = prefs.getUChar(K_CHANNEL, 0);
  uint8_t bssid[6];
  bool haveBssid = (prefs.getBytes(K_BSSID, bssid, 6) == 6);
  String ip = prefs.getString(K_IP, "");
  String gateway = prefs.getString(K_GATEWAY, "");
  String subnet = prefs.getString(K_SUBNET, "");
  prefs.end();
  if (ssid.length() == 0) return false;

#if USE_STATIC_IP
  const String* pip = (ip.length() > 0) ? &ip : nullptr;
  const String* pgw = (gateway.length() > 0) ? &gateway : nullptr;
  const String* psn = (subnet.length() > 0) ? &subnet : nullptr;
  bool useStatic = (pip != nullptr && pgw != nullptr && psn != nullptr);
#else
  const String* pip = nullptr;
  const String* pgw = nullptr;
  const String* psn = nullptr;
  bool useStatic = false;
#endif

  bool ok;
  if (channel >= 1 && channel <= 14 && haveBssid)
    ok = tryConnectInternal(ssid, pass, (int32_t)channel, bssid, pip, pgw, psn);
  else
    ok = tryConnectInternal(ssid, pass, 0, nullptr, pip, pgw, psn);

#if USE_STATIC_IP
  if (!ok && useStatic) {
    Preferences prefsClear;
    prefsClear.begin(NVS_NS, false);
    prefsClear.remove(K_IP);
    prefsClear.remove(K_GATEWAY);
    prefsClear.remove(K_SUBNET);
    prefsClear.remove(K_LAST_DHCP);
    prefsClear.end();
    if (channel >= 1 && channel <= 14 && haveBssid)
      ok = tryConnectInternal(ssid, pass, (int32_t)channel, bssid, nullptr, nullptr, nullptr);
    else
      ok = tryConnectInternal(ssid, pass, 0, nullptr, nullptr, nullptr, nullptr);
    useStatic = false;
  }
#endif

  if (!ok) return false;
  saveConnectionInfo();
#if USE_STATIC_IP
  if (!useStatic) {
    saveStaticFromCurrent();
    ensureNtpThenSaveEpoch();
  }
#endif
  return true;
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

#define MIN_UPTIME_BEFORE_REFRESH_MS 60000

void WifiConfigClass::runDailyRefreshIfNeeded() {
#if !USE_STATIC_IP
  return;
#endif
  if (WiFi.status() != WL_CONNECTED) return;
  static unsigned long first_connected_at = 0;
  if (first_connected_at == 0) first_connected_at = millis();
  if ((unsigned long)(millis() - first_connected_at) < MIN_UPTIME_BEFORE_REFRESH_MS) return;

  Preferences prefs;
  prefs.begin(NVS_NS, true);
  String ip = prefs.getString(K_IP, "");
  String ssid = prefs.getString(K_SSID, "");
  String pass = prefs.getString(K_PASS, "");
  uint8_t channel = prefs.getUChar(K_CHANNEL, 0);
  uint8_t bssid[6];
  bool haveBssid = (prefs.getBytes(K_BSSID, bssid, 6) == 6);
  prefs.end();
  if (ip.length() == 0 || ssid.length() == 0) return;

  static bool ntp_requested = false;
  if (!ntp_requested) {
    configTime(0, 0, NTP_SERVER);
    ntp_requested = true;
    return;
  }
  struct tm tm;
  if (!getLocalTime(&tm, 0)) return;
  time_t now = time(nullptr);
  if (now < (time_t)SECONDS_PER_DAY) return;
  uint32_t last = getLastDhcpEpoch();
  if (last == 0) {
    saveLastDhcpEpoch((uint32_t)now);
    return;
  }
  if ((uint32_t)now - last < SECONDS_PER_DAY) return;

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.config(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0));
  bool ok = (channel >= 1 && channel <= 14 && haveBssid)
    ? tryConnectInternal(ssid, pass, (int32_t)channel, bssid, nullptr, nullptr, nullptr)
    : tryConnectInternal(ssid, pass, 0, nullptr, nullptr, nullptr, nullptr);
  if (ok) {
    saveStaticFromCurrent();
    ensureNtpThenSaveEpoch();
    saveConnectionInfo();
    first_connected_at = millis();
  }
}

bool WifiConfigClass::tryConnect(const String& ssid, const String& pass) {
  bool ok = tryConnectInternal(ssid, pass, 0, nullptr, nullptr, nullptr, nullptr);
  if (ok) {
    saveConnectionInfo();
#if USE_STATIC_IP
    saveStaticFromCurrent();
    ensureNtpThenSaveEpoch();
#endif
  }
  return ok;
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
