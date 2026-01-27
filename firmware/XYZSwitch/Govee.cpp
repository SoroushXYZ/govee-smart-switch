#include "Govee.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define GOVEE_NS "govee"
#define K_APIKEY "apikey"
#define K_DEVICES "devices"
#define K_LASTON "laston"
#define BASE "https://developer-api.govee.com"
#define MAX_LIST 16
#define MAX_SEL 8

namespace {
  Preferences prefs;
  struct { String device, model, name; } lastList[MAX_LIST];
  int lastListCount = 0;

  struct { String device, model; } sel[MAX_SEL];
  int selCount = 0;

  void loadSelected() {
    prefs.begin(GOVEE_NS, true);
    String s = prefs.getString(K_DEVICES, "");
    prefs.end();
    selCount = 0;
    int i = 0;
    while (selCount < MAX_SEL && i < (int)s.length()) {
      int j = s.indexOf('|', i);
      if (j < 0) break;
      int k = s.indexOf(';', j + 1);
      if (k < 0) k = s.length();
      sel[selCount].device = s.substring(i, j);
      sel[selCount].model  = s.substring(j + 1, k);
      selCount++;
      i = k + 1;
    }
  }

  void saveSelected() {
    String s;
    for (int i = 0; i < selCount; i++) {
      if (i) s += ';';
      s += sel[i].device + '|' + sel[i].model;
    }
    prefs.begin(GOVEE_NS, false);
    prefs.putString(K_DEVICES, s);
    prefs.end();
  }
}

void GoveeClass::setApiKey(const String& key) {
  prefs.begin(GOVEE_NS, false);
  prefs.putString(K_APIKEY, key);
  prefs.end();
}

String GoveeClass::getApiKey() {
  prefs.begin(GOVEE_NS, true);
  String s = prefs.getString(K_APIKEY, "");
  prefs.end();
  return s;
}

void GoveeClass::setLastOn(bool on) {
  prefs.begin(GOVEE_NS, false);
  prefs.putString(K_LASTON, on ? "1" : "0");
  prefs.end();
}

bool GoveeClass::getLastOn() {
  prefs.begin(GOVEE_NS, true);
  String s = prefs.getString(K_LASTON, "0");
  prefs.end();
  return (s == "1");
}

int GoveeClass::fetchDevices() {
  String key = getApiKey();
  if (key.length() == 0) return 0;
  if (WiFi.status() != WL_CONNECTED) return 0;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, BASE "/v1/devices");
  http.addHeader("Govee-API-Key", key);
  http.setTimeout(10000);
  int code = http.GET();
  String body = http.getString();
  http.end();

  if (code != 200) return 0;

  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, body) != DeserializationError::Ok) return 0;
  JsonArray arr = doc["data"]["devices"].as<JsonArray>();
  lastListCount = 0;
  for (JsonObject o : arr) {
    if (lastListCount >= MAX_LIST) break;
    lastList[lastListCount].device = o["device"].as<const char*>();
    lastList[lastListCount].model  = o["model"].as<const char*>();
    lastList[lastListCount].name   = o["deviceName"].as<const char*>();
    lastListCount++;
  }
  return lastListCount;
}

int GoveeClass::getLastListCount() {
  return lastListCount;
}

void GoveeClass::getDeviceAt(int i, String& device, String& model, String& name) {
  if (i < 0 || i >= lastListCount) { device = ""; model = ""; name = ""; return; }
  device = lastList[i].device;
  model  = lastList[i].model;
  name   = lastList[i].name;
}

bool GoveeClass::isSelected(const String& device, const String& model) {
  loadSelected();
  for (int i = 0; i < selCount; i++)
    if (sel[i].device == device && sel[i].model == model) return true;
  return false;
}

void GoveeClass::toggleSelection(const String& device, const String& model) {
  loadSelected();
  for (int i = 0; i < selCount; i++) {
    if (sel[i].device == device && sel[i].model == model) {
      for (int j = i; j < selCount - 1; j++) sel[j] = sel[j + 1];
      selCount--;
      saveSelected();
      return;
    }
  }
  if (selCount >= MAX_SEL) return;
  sel[selCount].device = device;
  sel[selCount].model  = model;
  selCount++;
  saveSelected();
}

int GoveeClass::getSelectedCount() {
  loadSelected();
  return selCount;
}

bool GoveeClass::controlAll(bool on) {
  String key = getApiKey();
  if (key.length() == 0) return false;
  loadSelected();
  if (selCount == 0) return false;
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  const char* val = on ? "on" : "off";
  bool ok = true;

  for (int i = 0; i < selCount; i++) {
    http.begin(client, BASE "/v1/devices/control");
    http.addHeader("Govee-API-Key", key);
    http.addHeader("Content-Type", "application/json");
    StaticJsonDocument<256> doc;
    doc["device"] = sel[i].device;
    doc["model"]  = sel[i].model;
    doc["cmd"]["name"]  = "turn";
    doc["cmd"]["value"] = val;
    String body;
    serializeJson(doc, body);
    int code = http.PUT(body);
    http.end();
    if (code != 200) ok = false;
    delay(120);
  }
  return ok;
}

void GoveeClass::toggleLights() {
  if (getApiKey().length() == 0) {
    Serial.println("Govee: Set API key via Serial menu (press Enter).");
    return;
  }
  if (getSelectedCount() == 0) {
    Serial.println("Govee: Select devices via Serial menu (press Enter).");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Govee: WiFi not connected.");
    return;
  }
  bool next = !getLastOn();
  if (controlAll(next)) {
    setLastOn(next);
    Serial.println(next ? "Govee: ON" : "Govee: OFF");
  } else
    Serial.println("Govee: control failed");
}

GoveeClass Govee;
