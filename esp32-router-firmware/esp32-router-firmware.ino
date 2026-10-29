#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include "ConfigManager.h"
#include "LeaseManager.h"
#include "WebInterface.h"

ConfigManager configManager;
LeaseManager leaseManager;
WebServer server(80);
DNSServer dnsServer;

IPAddress ap_ip(192, 168, 4, 1);
IPAddress ap_mask(255, 255, 255, 0);
IPAddress ap_leaseStart(192, 168, 4, 2);
IPAddress ap_dns(8, 8, 4, 4);

void onEvent(arduino_event_id_t event, arduino_event_info_t info);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=============================================");
  Serial.println("  ESP32 NAT Router - Starting Firmware v2.0   ");
  Serial.println("=============================================");

  // 1. Load saved configs and leases from Flash
  configManager.begin();
  leaseManager.begin();

  // 2. Set up event handler
  Network.onEvent(onEvent);

  // Initialize Wi-Fi in dual mode (AP + STA)
  WiFi.mode(WIFI_AP_STA);

  // Apply Custom STA MAC if configured in NVS
  if (configManager.hasCustomStaMac()) {
    uint8_t customMac[6];
    if (configManager.getCustomStaMacBytes(customMac)) {
      esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, customMac);
      if (err != ESP_OK) {
        esp_wifi_stop();
        err = esp_wifi_set_mac(WIFI_IF_STA, customMac);
        esp_wifi_start();
      }
      if (err == ESP_OK) {
        Serial.printf("[MAC] STA MAC successfully configured to custom: %s\n", configManager.custom_sta_mac.c_str());
      } else {
        Serial.printf("[MAC] Failed to set custom STA MAC: 0x%X. Using hardware MAC.\n", err);
      }
    }
  } else {
    Serial.printf("[MAC] Using factory default hardware STA MAC: %s\n", WiFi.macAddress().c_str());
  }

  // 3. Start SoftAP
  WiFi.AP.begin();
  WiFi.AP.config(ap_ip, ap_ip, ap_mask, ap_leaseStart, ap_dns);
  WiFi.AP.create(configManager.ap_ssid.c_str(), configManager.ap_pass.length() > 0 ? configManager.ap_pass.c_str() : NULL);
  
  if (!WiFi.AP.waitStatusBits(ESP_NETIF_STARTED_BIT, 2000)) {
    Serial.println("[ERROR] Failed to start SoftAP!");
  } else {
    Serial.printf("[AP] Hotspot Started: SSID '%s' (IP: %s)\n", configManager.ap_ssid.c_str(), ap_ip.toString().c_str());
  }

  // 4. Configure DHCP range to 192.168.4.2 - 192.168.4.254
  leaseManager.configureDhcpRange();

  // 5. Start DNS Server for Captive Portal
  dnsServer.start(53, "*", ap_ip);
  Serial.println("[DNS] Captive Portal DNS Server started on port 53");

  // 6. Start Web Interface
  setupWebRoutes();

  // 7. Connect to Host WiFi (STA)
  if (configManager.sta_ssid.length() > 0) {
    Serial.printf("[STA] Connecting to Host WiFi: '%s'...\n", configManager.sta_ssid.c_str());
    WiFi.begin(configManager.sta_ssid.c_str(), configManager.sta_pass.c_str());
  } else {
    Serial.println("[STA] No Host SSID configured. Waiting for configuration via Web UI...");
  }
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  delay(2);
}

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("[STA] WiFi Station Started");
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("[STA] Connected to Host WiFi");
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      Serial.print("[STA] Got IP: ");
      Serial.println(WiFi.localIP());
      Serial.printf("[STA] Gateway: %s, DNS: %s\n", WiFi.gatewayIP().toString().c_str(), WiFi.dnsIP().toString().c_str());
      
      // Enable NAPT router forwarding
      WiFi.AP.enableNAPT(true);
      Serial.println("[NAPT] Router NAT Forwarding ENABLED");
      break;
    }

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("[STA] Lost IP from Host");
      WiFi.AP.enableNAPT(false);
      Serial.println("[NAPT] Router NAT Forwarding DISABLED");
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("[STA] Disconnected from Host WiFi");
      WiFi.AP.enableNAPT(false);
      Serial.println("[NAPT] Router NAT Forwarding DISABLED");
      break;

    case ARDUINO_EVENT_WIFI_STA_STOP:
      Serial.println("[STA] Station Stopped");
      break;

    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("[AP] SoftAP Started");
      break;

    case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
      uint8_t *mac = info.wifi_ap_staconnected.mac;
      // Get deterministic static IP for this MAC (or assign new/FIFO from Flash)
      IPAddress assignedIp = leaseManager.getOrAssignIp(mac);
      
      // Inject deterministic lease into DHCPS before client asks for IP
      leaseManager.injectLease(mac, assignedIp);

      Serial.printf("[AP] Client Connected: %02X:%02X:%02X:%02X:%02X:%02X -> Target IP: %s\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], assignedIp.toString().c_str());
      break;
    }

    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED: {
      uint8_t *mac = info.wifi_ap_staipassigned.mac;
      IPAddress ip(info.wifi_ap_staipassigned.ip.addr);
      leaseManager.markOnline(mac, ip);

      Serial.printf("[AP] IP Assigned: %s to %02X:%02X:%02X:%02X:%02X:%02X\n",
        ip.toString().c_str(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      break;
    }

    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: {
      uint8_t *mac = info.wifi_ap_stadisconnected.mac;
      leaseManager.markOffline(mac);

      Serial.printf("[AP] Client Disconnected: %02X:%02X:%02X:%02X:%02X:%02X\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      break;
    }

    case ARDUINO_EVENT_WIFI_AP_STOP:
      Serial.println("[AP] SoftAP Stopped");
      break;

    default:
      break;
  }
}
