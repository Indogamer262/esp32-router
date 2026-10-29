#ifndef LEASE_MANAGER_H
#define LEASE_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_netif.h>
#include "dhcpserver/dhcpserver.h"
#include "dhcpserver/dhcpserver_options.h"
#include <lwip/tcpip.h>

#define TOTAL_CLIENT_LEASES 253 // 192.168.4.2 to 192.168.4.254 (1 is GW, 255 is broadcast)

#pragma pack(push, 1)
struct StoredLeaseRecord {
  uint8_t mac[6];
  uint8_t ip_suffix;  // 2 .. 254
  uint8_t valid;      // 1 if stored, 0 if free
  uint32_t seq;       // FIFO ordering counter (lowest = oldest)
};
#pragma pack(pop)

// ESP-IDF internal struct definitions needed for DHCPS manipulation
typedef struct list_node {
  void *pnode;
  struct list_node *pnext;
} list_node;

struct dhcps_t {
  struct netif *dhcps_netif;
  ip4_addr_t broadcast_dhcps;
  ip4_addr_t server_address;
  ip4_addr_t dns_server[2];
  ip4_addr_t client_address;
  ip4_addr_t client_address_plus;
  ip4_addr_t dhcps_mask;
  list_node *plist;
  bool renew;
  dhcps_lease_t dhcps_poll;
  dhcps_time_t dhcps_lease_time;
  dhcps_offer_t dhcps_offer;
  dhcps_offer_t dhcps_dns;
  dhcps_cb_t dhcps_cb;
  void* dhcps_cb_arg;
  struct udp_pcb *dhcps_pcb;
  int state;
  bool has_declined_ip;
};

struct esp_netif_obj_dummy {
  uint8_t mac[6];
  void* ip_info;
  void* ip_info_old;
  struct netif *lwip_netif;
  void* lwip_init_fn;
  void* lwip_input_fn;
  void* netif_handle;
  void* related_data;
  dhcps_t *dhcps;
};

struct StaticIpInjectCtx {
  uint8_t mac[6];
  uint32_t ip;
};

class LeaseManager {
public:
  StoredLeaseRecord leases[TOTAL_CLIENT_LEASES];
  bool onlineStatus[TOTAL_CLIENT_LEASES];
  uint32_t lastSeenMs[TOTAL_CLIENT_LEASES];
  uint32_t maxSeq;

  LeaseManager() : maxSeq(0) {
    memset(leases, 0, sizeof(leases));
    memset(onlineStatus, 0, sizeof(onlineStatus));
    memset(lastSeenMs, 0, sizeof(lastSeenMs));
  }

  void begin() {
    loadFromFlash();
  }

  int getStoredCount() const {
    int count = 0;
    for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
      if (leases[i].valid) count++;
    }
    return count;
  }

  int getOnlineCount() const {
    int count = 0;
    for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
      if (leases[i].valid && onlineStatus[i]) count++;
    }
    return count;
  }

  IPAddress getOrAssignIp(const uint8_t mac[6]) {
    // 1. Check if MAC is already registered
    for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
      if (leases[i].valid && memcmp(leases[i].mac, mac, 6) == 0) {
        lastSeenMs[i] = millis();
        onlineStatus[i] = true;
        return IPAddress(192, 168, 4, leases[i].ip_suffix);
      }
    }

    // 2. Not registered. Check if there is free slot
    int freeSlot = -1;
    for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
      if (!leases[i].valid) {
        freeSlot = i;
        break;
      }
    }

    if (freeSlot != -1) {
      // Find an unused IP suffix between 2 and 254
      bool usedIps[256] = { false };
      usedIps[0] = true;
      usedIps[1] = true; // Gateway
      usedIps[255] = true; // Broadcast

      for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
        if (leases[i].valid) {
          usedIps[leases[i].ip_suffix] = true;
        }
      }

      uint8_t newSuffix = 2;
      for (int s = 2; s <= 254; s++) {
        if (!usedIps[s]) {
          newSuffix = s;
          break;
        }
      }

      leases[freeSlot].valid = 1;
      leases[freeSlot].ip_suffix = newSuffix;
      leases[freeSlot].seq = ++maxSeq;
      memcpy(leases[freeSlot].mac, mac, 6);
      onlineStatus[freeSlot] = true;
      lastSeenMs[freeSlot] = millis();

      saveToFlash();
      Serial.printf("[LeaseManager] Assigned NEW IP 192.168.4.%d to %02X:%02X:%02X:%02X:%02X:%02X (Slot %d, Seq %u)\n",
        newSuffix, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], freeSlot, maxSeq);

      return IPAddress(192, 168, 4, newSuffix);
    }

    // 3. Table is FULL (TOTAL_CLIENT_LEASES reached)!
    // FIFO Eviction: Find the entry with minimum seq (oldest MAC)
    int oldestSlot = 0;
    uint32_t minSeq = leases[0].seq;
    for (int i = 1; i < TOTAL_CLIENT_LEASES; i++) {
      if (leases[i].seq < minSeq) {
        minSeq = leases[i].seq;
        oldestSlot = i;
      }
    }

    uint8_t oldMac[6];
    memcpy(oldMac, leases[oldestSlot].mac, 6);
    uint8_t reusedSuffix = leases[oldestSlot].ip_suffix;

    leases[oldestSlot].valid = 1;
    leases[oldestSlot].seq = ++maxSeq;
    memcpy(leases[oldestSlot].mac, mac, 6);
    onlineStatus[oldestSlot] = true;
    lastSeenMs[oldestSlot] = millis();

    saveToFlash();
    Serial.printf("[LeaseManager] Table FULL! Evicted oldest MAC %02X:%02X:%02X:%02X:%02X:%02X. Reassigned IP 192.168.4.%d to NEW MAC %02X:%02X:%02X:%02X:%02X:%02X (Seq %u)\n",
      oldMac[0], oldMac[1], oldMac[2], oldMac[3], oldMac[4], oldMac[5],
      reusedSuffix, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], maxSeq);

    return IPAddress(192, 168, 4, reusedSuffix);
  }

  void markOnline(const uint8_t mac[6], IPAddress ip) {
    for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
      if (leases[i].valid && memcmp(leases[i].mac, mac, 6) == 0) {
        onlineStatus[i] = true;
        lastSeenMs[i] = millis();
        // ensure suffix matches
        leases[i].ip_suffix = ip[3];
        return;
      }
    }
  }

  void markOffline(const uint8_t mac[6]) {
    for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
      if (leases[i].valid && memcmp(leases[i].mac, mac, 6) == 0) {
        onlineStatus[i] = false;
        lastSeenMs[i] = millis();
        return;
      }
    }
  }

  bool deleteLease(uint8_t ip_suffix) {
    for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
      if (leases[i].valid && leases[i].ip_suffix == ip_suffix) {
        leases[i].valid = 0;
        onlineStatus[i] = false;
        memset(leases[i].mac, 0, 6);
        leases[i].seq = 0;
        saveToFlash();
        return true;
      }
    }
    return false;
  }

  void clearAll() {
    memset(leases, 0, sizeof(leases));
    memset(onlineStatus, 0, sizeof(onlineStatus));
    memset(lastSeenMs, 0, sizeof(lastSeenMs));
    maxSeq = 0;
    saveToFlash();
  }

  static void tcpip_inject_lease_cb(void *ctx) {
    auto *data = (StaticIpInjectCtx *)ctx;
    esp_netif_t *netif = WiFi.AP.netif();
    if (!netif) return;
    auto *obj = (esp_netif_obj_dummy *)netif;
    dhcps_t *dhcps = obj->dhcps;
    if (!dhcps) return;

    list_node *curr = dhcps->plist;
    while (curr) {
      auto *pool = (dhcps_pool *)curr->pnode;
      if (pool && memcmp(pool->mac, data->mac, 6) == 0) {
        pool->ip.addr = data->ip;
        pool->lease_timer = 86400;
        return;
      }
      curr = curr->pnext;
    }

    auto *new_pool = (dhcps_pool *)malloc(sizeof(dhcps_pool));
    if (!new_pool) return;
    new_pool->ip.addr = data->ip;
    memcpy(new_pool->mac, data->mac, 6);
    new_pool->lease_timer = 86400;

    auto *new_node = (list_node *)malloc(sizeof(list_node));
    if (!new_node) {
      free(new_pool);
      return;
    }
    new_node->pnode = new_pool;
    new_node->pnext = dhcps->plist;
    dhcps->plist = new_node;
  }

  void injectLease(const uint8_t mac[6], IPAddress ip) {
    StaticIpInjectCtx ctx;
    memcpy(ctx.mac, mac, 6);
    ctx.ip = static_cast<uint32_t>(ip);
    tcpip_callback_wait(tcpip_inject_lease_cb, &ctx);
  }

  void configureDhcpRange() {
    dhcps_lease_t lease;
    lease.enable = true;
    lease.start_ip.addr = static_cast<uint32_t>(IPAddress(192, 168, 4, 2));
    lease.end_ip.addr = static_cast<uint32_t>(IPAddress(192, 168, 4, 254));
    esp_netif_dhcps_stop(WiFi.AP.netif());
    esp_netif_dhcps_option(WiFi.AP.netif(), ESP_NETIF_OP_SET, ESP_NETIF_REQUESTED_IP_ADDRESS, &lease, sizeof(dhcps_lease_t));
    esp_netif_dhcps_start(WiFi.AP.netif());
  }

private:
  void loadFromFlash() {
    Preferences prefs;
    if (!prefs.begin("mac_leases", true)) return;
    
    maxSeq = prefs.getUInt("max_seq", 0);
    
    int half = TOTAL_CLIENT_LEASES / 2; // 126
    size_t sz1 = half * sizeof(StoredLeaseRecord);
    size_t sz2 = (TOTAL_CLIENT_LEASES - half) * sizeof(StoredLeaseRecord);

    prefs.getBytes("chunk0", &leases[0], sz1);
    prefs.getBytes("chunk1", &leases[half], sz2);
    prefs.end();

    // Verify maxSeq consistency
    for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
      if (leases[i].valid && leases[i].seq > maxSeq) {
        maxSeq = leases[i].seq;
      }
    }
    Serial.printf("[LeaseManager] Loaded %d stored leases from Flash. maxSeq: %u\n", getStoredCount(), maxSeq);
  }

  void saveToFlash() {
    Preferences prefs;
    if (!prefs.begin("mac_leases", false)) return;

    prefs.putUInt("max_seq", maxSeq);

    int half = TOTAL_CLIENT_LEASES / 2;
    size_t sz1 = half * sizeof(StoredLeaseRecord);
    size_t sz2 = (TOTAL_CLIENT_LEASES - half) * sizeof(StoredLeaseRecord);

    prefs.putBytes("chunk0", &leases[0], sz1);
    prefs.putBytes("chunk1", &leases[half], sz2);
    prefs.end();
  }
};

extern LeaseManager leaseManager;

#endif // LEASE_MANAGER_H
