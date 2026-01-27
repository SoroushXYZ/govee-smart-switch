#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <Arduino.h>

class ConfigPortalClass {
public:
  // Try to connect using stored credentials. Returns true if connected.
  bool tryStoredWifi();

  // Start AP and web server (captive portal). Call when no/wrong credentials.
  void begin(const char* apName);

  // Call from loop() while portal is active.
  void handleClient();

  // Whether the portal is running (AP + server).
  bool isActive() const { return _active; }

private:
  void handleRoot();
  void handleScan();
  void handleConnect();
  void handleNotFound();
  void stop();

  bool _active = false;
  String _apName;
};

extern ConfigPortalClass ConfigPortal;

#endif
