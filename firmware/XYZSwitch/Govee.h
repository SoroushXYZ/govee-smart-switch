#ifndef GOVEE_H
#define GOVEE_H

#include <Arduino.h>

class GoveeClass {
public:
  // API key
  void setApiKey(const String& key);
  String getApiKey();

  // Last on/off state (flip on each SW1 toggle)
  void setLastOn(bool on);
  bool getLastOn();

  // Fetch devices from API. Fills internal list; returns count. 0 on fail.
  int fetchDevices();

  // Number of entries in last-fetched list (0 until fetchDevices succeeds).
  int getLastListCount();

  // Access last-fetched list. i in [0, getLastListCount()-1].
  void getDeviceAt(int i, String& device, String& model, String& name);

  // Selection (stored in NVS as "id|model;id|model")
  bool isSelected(const String& device, const String& model);
  void toggleSelection(const String& device, const String& model);
  int getSelectedCount();

  // Control: set all selected to on/off. Returns true if all succeeded.
  bool controlAll(bool on);

  // Toggle: flip getLastOn(), controlAll, setLastOn. Handles no-key, no-devices, no-WiFi.
  void toggleLights();

  // Clear API key, selected devices, and last-on state from NVS.
  void clearAll();
};

extern GoveeClass Govee;

#endif
