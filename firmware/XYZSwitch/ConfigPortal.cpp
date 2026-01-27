#include "ConfigPortal.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <string.h>

namespace {
  WebServer server(80);
  DNSServer dnsServer;
  Preferences prefs;

  const char* NVS_NS = "wifi";
  const char* K_SSID = "ssid";
  const char* K_PASS = "pass";

  const unsigned long CONNECT_TIMEOUT_MS = 15000;
  const unsigned long SUCCESS_DELAY_MS  = 5000;

  const char STYLE[] PROGMEM = R"raw(
*{box-sizing:border-box} body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;background:linear-gradient(160deg,#1a1a2e 0%,#16213e 100%);margin:0;padding:20px;min-height:100vh;display:flex;align-items:center;justify-content:center;color:#eee}
.card{background:rgba(255,255,255,.96);color:#1a1a1a;border-radius:16px;box-shadow:0 12px 40px rgba(0,0,0,.25);padding:28px;width:100%;max-width:380px}
h1{font-size:1.5rem;margin:0 0 6px;font-weight:700;color:#1a1a2e}
.sub{color:#555;font-size:.9rem;margin-bottom:20px}
label{display:block;font-size:.8rem;font-weight:600;color:#333;margin-bottom:6px;text-transform:uppercase;letter-spacing:.04em}
select,input[type=password]{width:100%;padding:12px 14px;font-size:1rem;border:1px solid #ddd;border-radius:10px;margin-bottom:14px;background:#fff}
select:focus,input:focus{outline:none;border-color:#0d6efd;box-shadow:0 0 0 3px rgba(13,110,253,.15)}
.btns{display:flex;gap:10px;margin-top:20px;flex-wrap:wrap}
button{font-family:inherit;padding:12px 20px;font-size:1rem;font-weight:600;border:none;border-radius:10px;cursor:pointer;transition:opacity .15s}
button:active{opacity:.9}
.btn-p{background:#0d6efd;color:#fff}
.btn-s{background:#e9ecef;color:#495057}
.btn-s:hover{background:#dee2e6}
a{color:#0d6efd;text-decoration:none;font-weight:500}
a:hover{text-decoration:underline}
)raw";

  const char FORM_HTML[] PROGMEM = R"raw(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>XYZ-Switch WiFi</title><style>)raw";

  const char FORM_HTML2[] PROGMEM = R"raw(</style></head><body><div class="card"><h1>XYZ-Switch</h1><p class="sub">Choose your Wi&#8209;Fi network</p>
<form method="POST" action="/connect"><label>Network</label>
<select name="ssid" id="ssid" required><option value="">Loading…</option></select>
<label>Password</label><input type="password" name="password" placeholder="(empty if open)">
<div class="btns"><button type="submit" class="btn-p">Connect</button><button type="button" class="btn-s" id="rescan">Rescan</button></div></form></div>
<script>
function load(){ fetch('/scan').then(function(r){return r.json()}).then(function(arr){
  var s=document.getElementById('ssid'); s.innerHTML='<option value="">— Select network —</option>';
  for(var i=0;i<arr.length;i++){ var o=document.createElement('option'); o.value=arr[i].ssid; o.textContent=arr[i].ssid+' ('+arr[i].rssi+' dBm)'; s.appendChild(o) }
})}
document.getElementById('rescan').onclick=function(){ var s=document.getElementById('ssid'); s.innerHTML='<option value="">Scanning…</option>'; load() };
load();
</script></body></html>
)raw";

  const char SUCCESS_HTML[] PROGMEM = R"raw(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Connected</title><style>)raw";

  const char SUCCESS_HTML2[] PROGMEM = R"raw(</style></head><body><div class="card"><h1>Connection successful</h1><p class="sub">You can close this tab. If it doesn’t close in 3 seconds, close it manually.</p></div>
<script>setTimeout(function(){ window.close() }, 3000)</script></body></html>
)raw";

  String failHtml(const char* reason) {
    return String(F("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Failed</title><style>")) + String(FPSTR(STYLE)) +
      F("</style></head><body><div class=\"card\"><h1>Connection failed</h1><p class=\"sub\">") + reason +
      F("</p><p><a href=\"/\">Try again</a></p></div></body></html>");
  }

  const char* wifiStatusToStr(wl_status_t s) {
    switch (s) {
      case WL_NO_SSID_AVAIL: return "Network not found";
      case WL_CONNECT_FAILED: return "Wrong password or weak signal";
      case WL_CONNECTION_LOST: return "Connection lost";
      case WL_DISCONNECTED:   return "Disconnected";
      default:                return "Unknown error";
    }
  }
}

bool ConfigPortalClass::tryStoredWifi() {
  prefs.begin(NVS_NS, true);
  String ssid = prefs.getString(K_SSID, "");
  String pass = prefs.getString(K_PASS, "");
  prefs.end();

  if (ssid.length() == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t) < CONNECT_TIMEOUT_MS) {
    delay(200);
  }
  return (WiFi.status() == WL_CONNECTED);
}

void ConfigPortalClass::begin(const char* apName) {
  _apName = apName;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName);

  IPAddress apip = WiFi.softAPIP();
  dnsServer.start(53, "*", apip);
  server.on("/", [this]() { handleRoot(); });
  server.on("/scan", HTTP_GET, [this]() { handleScan(); });
  server.on("/connect", HTTP_POST, [this]() { handleConnect(); });
  server.onNotFound([this]() { handleNotFound(); });
  server.begin();
  _active = true;
  Serial.println("AP IP: " + apip.toString());
}

void ConfigPortalClass::handleClient() {
  dnsServer.processNextRequest();
  server.handleClient();
}

void ConfigPortalClass::handleRoot() {
  String body = String(FPSTR(FORM_HTML)) + String(FPSTR(STYLE)) + String(FPSTR(FORM_HTML2));
  server.send(200, "text/html", body);
}

void ConfigPortalClass::handleScan() {
  int n = WiFi.scanNetworks();
  struct { char ssid[33]; int rssi; } nets[24];
  int count = 0;

  for (int i = 0; i < n && count < 24; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue;
    int r = WiFi.RSSI(i);
    int j = 0;
    for (; j < count; j++)
      if (s.equals(nets[j].ssid)) {
        if (r > nets[j].rssi) nets[j].rssi = r;
        break;
      }
    if (j == count) {
      s.toCharArray(nets[count].ssid, 33);
      nets[count].rssi = r;
      count++;
    }
  }

  for (int i = 0; i < count - 1; i++)
    for (int j = 0; j < count - 1 - i; j++)
      if (nets[j].rssi < nets[j + 1].rssi) {
        struct { char ssid[33]; int rssi; } t;
        memcpy(&t, &nets[j], sizeof(t));
        memcpy(&nets[j], &nets[j + 1], sizeof(t));
        memcpy(&nets[j + 1], &t, sizeof(t));
      }

  String json = "[";
  for (int i = 0; i < count; i++) {
    if (i) json += ",";
    json += "{\"ssid\":\"";
    for (const char* p = nets[i].ssid; *p; p++) {
      if (*p == '\\') json += "\\\\";
      else if (*p == '"') json += "\\\"";
      else json += *p;
    }
    json += "\",\"rssi\":";
    json += String(nets[i].rssi);
    json += "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void ConfigPortalClass::handleConnect() {
  if (!server.hasArg("ssid")) {
    server.send(400, "text/html", failHtml("SSID is required."));
    return;
  }
  String ssid = server.arg("ssid");
  String pass = server.arg("password");
  if (ssid.length() == 0) {
    server.send(400, "text/html", failHtml("Please select a network."));
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t) < CONNECT_TIMEOUT_MS) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    prefs.begin(NVS_NS, false);
    prefs.putString(K_SSID, ssid);
    prefs.putString(K_PASS, pass);
    prefs.end();

    String body = String(FPSTR(SUCCESS_HTML)) + String(FPSTR(STYLE)) + String(FPSTR(SUCCESS_HTML2));
    server.send(200, "text/html", body);
    delay(SUCCESS_DELAY_MS);
    stop();
    return;
  }

  server.send(200, "text/html", failHtml(wifiStatusToStr(WiFi.status())));
}

void ConfigPortalClass::handleNotFound() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

void ConfigPortalClass::stop() {
  server.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  _active = false;
  Serial.println("Config portal stopped.");
}

ConfigPortalClass ConfigPortal;
