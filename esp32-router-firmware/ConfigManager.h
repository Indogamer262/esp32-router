#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class ConfigManager {
public:
  String sta_ssid;
  String sta_pass;
  String ap_ssid;
  String ap_pass;
  String custom_sta_mac;

  ConfigManager() : 
    sta_ssid("iot"),
    sta_pass("iot12345*"),
    ap_ssid("ESP32-ROUTER"),
    ap_pass(""),
    custom_sta_mac("") {}

  void begin() {
    Preferences prefs;
    if (prefs.begin("router_cfg", true)) {
      sta_ssid = prefs.getString("sta_ssid", "iot");
      sta_pass = prefs.getString("sta_pass", "iot12345*");
      ap_ssid  = prefs.getString("ap_ssid", "ESP32-ROUTER");
      ap_pass  = prefs.getString("ap_pass", "");
      custom_sta_mac = prefs.getString("sta_mac", "");
      prefs.end();
    }
  }

  bool saveSta(const String &ssid, const String &pass) {
    Preferences prefs;
    if (!prefs.begin("router_cfg", false)) return false;
    prefs.putString("sta_ssid", ssid);
    prefs.putString("sta_pass", pass);
    prefs.end();
    sta_ssid = ssid;
    sta_pass = pass;
    return true;
  }

  bool saveAp(const String &ssid, const String &pass) {
    Preferences prefs;
    if (!prefs.begin("router_cfg", false)) return false;
    prefs.putString("ap_ssid", ssid);
    prefs.putString("ap_pass", pass);
    prefs.end();
    ap_ssid = ssid;
    ap_pass = pass;
    return true;
  }

  bool saveCustomStaMac(const String &mac) {
    Preferences prefs;
    if (!prefs.begin("router_cfg", false)) return false;
    prefs.putString("sta_mac", mac);
    prefs.end();
    custom_sta_mac = mac;
    return true;
  }

  bool revertStaMac() {
    Preferences prefs;
    if (!prefs.begin("router_cfg", false)) return false;
    prefs.remove("sta_mac");
    prefs.end();
    custom_sta_mac = "";
    return true;
  }

  bool resetDefaults() {
    Preferences prefs;
    if (!prefs.begin("router_cfg", false)) return false;
    prefs.clear();
    prefs.end();
    sta_ssid = "iot";
    sta_pass = "iot12345*";
    ap_ssid = "ESP32-ROUTER";
    ap_pass = "";
    custom_sta_mac = "";
    return true;
  }

  bool hasCustomStaMac() const {
    return custom_sta_mac.length() > 0;
  }

  bool getCustomStaMacBytes(uint8_t mac[6]) const {
    return parseMacString(custom_sta_mac.c_str(), mac);
  }

  static bool parseMacString(const char *str, uint8_t mac[6]) {
    if (!str || strlen(str) < 12) return false;
    unsigned int m[6];
    int count = sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
    if (count != 6) {
      count = sscanf(str, "%02x-%02x-%02x-%02x-%02x-%02x", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
    }
    if (count != 6) return false;

    // Bit 0 of the first byte must be 0 (Unicast address)
    if (m[0] & 0x01) return false;

    // Check not all zeroes or all 0xFF
    bool allZero = true;
    bool allFF = true;
    for (int i = 0; i < 6; i++) {
      if (m[i] != 0x00) allZero = false;
      if (m[i] != 0xFF) allFF = false;
      mac[i] = (uint8_t)m[i];
    }
    if (allZero || allFF) return false;

    return true;
  }

  static String formatMac(const uint8_t mac[6]) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
  }
};

extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H
