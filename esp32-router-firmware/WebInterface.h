#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include "ConfigManager.h"
#include "LeaseManager.h"

extern WebServer server;

inline String getFactoryStaMac() {
  uint8_t mac[6];
  if (esp_efuse_mac_get_default(mac) == ESP_OK) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
  }
  return WiFi.macAddress();
}

static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 NAT Router - Manajemen Jaringan</title>
  <style>
    :root {
      --bg: #0b0f19;
      --card-bg: #151d30;
      --card-border: #23304e;
      --primary: #38bdf8;
      --primary-hover: #0284c7;
      --success: #34d399;
      --warning: #fbbf24;
      --danger: #f87171;
      --text: #f1f5f9;
      --text-muted: #94a3b8;
      --font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: var(--font-family); }
    body { background-color: var(--bg); color: var(--text); padding-bottom: 60px; }
    .container { max-width: 1080px; margin: 0 auto; padding: 20px 16px; }
    
    header { display: flex; justify-content: space-between; align-items: center; padding-bottom: 20px; border-bottom: 1px solid var(--card-border); margin-bottom: 24px; flex-wrap: wrap; gap: 12px; }
    .logo { display: flex; align-items: center; gap: 12px; }
    .logo-icon { width: 38px; height: 38px; background: linear-gradient(135deg, #0284c7, #38bdf8); border-radius: 10px; display: flex; align-items: center; justify-content: center; font-size: 20px; font-weight: bold; color: white; }
    .logo-text h1 { font-size: 20px; font-weight: 700; color: white; }
    .logo-text p { font-size: 12px; color: var(--text-muted); }

    .nav-tabs { display: flex; gap: 8px; margin-bottom: 24px; border-bottom: 1px solid var(--card-border); overflow-x: auto; padding-bottom: 8px; }
    .tab-btn { background: transparent; border: none; color: var(--text-muted); padding: 10px 16px; font-size: 14px; font-weight: 600; cursor: pointer; border-radius: 8px; transition: all 0.2s; white-space: nowrap; display: flex; align-items: center; gap: 8px; }
    .tab-btn:hover { background: rgba(56, 189, 248, 0.1); color: var(--primary); }
    .tab-btn.active { background: var(--primary); color: #0b0f19; font-weight: 700; }

    .tab-pane { display: none; }
    .tab-pane.active { display: block; animation: fadeIn 0.25s ease-in-out; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(4px); } to { opacity: 1; transform: translateY(0); } }

    .grid-2 { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 20px; margin-bottom: 24px; }
    .grid-4 { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 16px; margin-bottom: 24px; }

    .card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 14px; padding: 20px; }
    .card-title { font-size: 16px; font-weight: 700; margin-bottom: 16px; display: flex; justify-content: space-between; align-items: center; color: white; }

    .stat-card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; padding: 16px; display: flex; flex-direction: column; gap: 6px; }
    .stat-label { font-size: 12px; color: var(--text-muted); text-transform: uppercase; font-weight: 600; letter-spacing: 0.5px; }
    .stat-val { font-size: 22px; font-weight: 700; color: white; }
    .stat-desc { font-size: 12px; color: var(--text-muted); }

    .badge { display: inline-flex; align-items: center; gap: 6px; padding: 4px 10px; border-radius: 20px; font-size: 12px; font-weight: 600; }
    .badge-dot { width: 8px; height: 8px; border-radius: 50%; }
    .badge-success { background: rgba(52, 211, 153, 0.15); color: var(--success); }
    .badge-success .badge-dot { background: var(--success); box-shadow: 0 0 8px var(--success); }
    .badge-danger { background: rgba(248, 113, 113, 0.15); color: var(--danger); }
    .badge-danger .badge-dot { background: var(--danger); }
    .badge-primary { background: rgba(56, 189, 248, 0.15); color: var(--primary); }
    .badge-primary .badge-dot { background: var(--primary); }
    .badge-gray { background: rgba(148, 163, 184, 0.15); color: var(--text-muted); }
    .badge-gray .badge-dot { background: var(--text-muted); }

    .info-list { display: flex; flex-direction: column; gap: 12px; font-size: 14px; }
    .info-item { display: flex; justify-content: space-between; border-bottom: 1px dashed rgba(255,255,255,0.06); padding-bottom: 8px; }
    .info-key { color: var(--text-muted); }
    .info-val { font-weight: 600; color: white; text-align: right; }

    .form-group { margin-bottom: 16px; }
    label { display: block; font-size: 13px; font-weight: 600; margin-bottom: 6px; color: var(--text-muted); }
    input[type="text"], input[type="password"] {
      width: 100%; background: #0c1220; border: 1px solid var(--card-border); border-radius: 8px; padding: 12px 14px; font-size: 14px; color: white; outline: none; transition: all 0.2s;
    }
    input[type="text"]:focus, input[type="password"]:focus {
      border-color: var(--primary); box-shadow: 0 0 0 3px rgba(56, 189, 248, 0.2);
    }
    .input-row { display: flex; gap: 8px; align-items: center; }

    .btn { background: var(--primary); color: #0b0f19; border: none; border-radius: 8px; padding: 10px 18px; font-size: 14px; font-weight: 700; cursor: pointer; transition: all 0.2s; display: inline-flex; align-items: center; justify-content: center; gap: 8px; }
    .btn:hover { background: var(--primary-hover); transform: translateY(-1px); }
    .btn-secondary { background: #1e293b; color: white; border: 1px solid var(--card-border); }
    .btn-secondary:hover { background: #334155; }
    .btn-danger { background: rgba(248, 113, 113, 0.2); color: var(--danger); border: 1px solid rgba(248, 113, 113, 0.3); }
    .btn-danger:hover { background: var(--danger); color: #0b0f19; }
    .btn-sm { padding: 6px 12px; font-size: 12px; border-radius: 6px; }

    .banner { background: rgba(56, 189, 248, 0.08); border: 1px solid rgba(56, 189, 248, 0.2); border-radius: 10px; padding: 14px 16px; font-size: 13px; line-height: 1.5; color: #bae6fd; margin-bottom: 20px; }

    /* Tables */
    .table-container { width: 100%; overflow-x: auto; border: 1px solid var(--card-border); border-radius: 10px; }
    table { width: 100%; border-collapse: collapse; font-size: 13px; text-align: left; }
    th { background: #0e1526; padding: 12px 14px; color: var(--text-muted); font-weight: 600; border-bottom: 1px solid var(--card-border); }
    td { padding: 12px 14px; border-bottom: 1px solid rgba(255,255,255,0.05); color: var(--text); }
    tr:last-child td { border-bottom: none; }
    tr:hover td { background: rgba(255,255,255,0.02); }

    .search-bar { display: flex; gap: 10px; margin-bottom: 16px; flex-wrap: wrap; }
    .search-input { flex: 1; min-width: 200px; }

    /* WiFi Scanner List */
    .scan-list { display: flex; flex-direction: column; gap: 8px; max-height: 240px; overflow-y: auto; margin-top: 12px; }
    .scan-item { display: flex; justify-content: space-between; align-items: center; background: #0d1424; border: 1px solid var(--card-border); border-radius: 8px; padding: 10px 14px; cursor: pointer; transition: all 0.2s; }
    .scan-item:hover { border-color: var(--primary); background: rgba(56, 189, 248, 0.05); }

    /* Toast */
    #toast { position: fixed; bottom: 20px; right: 20px; background: #1e293b; color: white; padding: 12px 20px; border-radius: 8px; border: 1px solid var(--card-border); font-size: 14px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); display: none; z-index: 9999; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="logo">
        <div class="logo-icon">&#8644;</div>
        <div class="logo-text">
          <h1>ESP32 NAT Router</h1>
          <p><span id="headerChipModel">ESP32</span> &bull; Deterministic Static DHCP Subnet 192.168.4.0/24</p>
        </div>
      </div>
      <div style="display: flex; gap: 8px; align-items: center;">
        <span id="headerWanBadge" class="badge badge-gray"><span class="badge-dot"></span>Memeriksa WAN...</span>
        <span id="headerApBadge" class="badge badge-primary"><span class="badge-dot"></span>AP 192.168.4.1</span>
      </div>
    </header>

    <div class="nav-tabs">
      <button class="tab-btn active" onclick="openTab('tab-dashboard')">&#128202; Dashboard</button>
      <button class="tab-btn" onclick="openTab('tab-host')">&#128225; WiFi Host (Sumber)</button>
      <button class="tab-btn" onclick="openTab('tab-forward')">&#128246; WiFi Forward (Hotspot)</button>
      <button class="tab-btn" onclick="openTab('tab-leases')">&#128203; Tabel MAC & IP (<span id="storedCountTab">0</span>/254)</button>
      <button class="tab-btn" onclick="openTab('tab-system')">&#9881; Sistem</button>
    </div>

    <!-- TAB 1: DASHBOARD -->
    <div id="tab-dashboard" class="tab-pane active">
      <div class="grid-4">
        <div class="stat-card">
          <span class="stat-label">Status Internet WAN</span>
          <span class="stat-val" id="statWanStatus" style="font-size:18px;">Menghubungkan...</span>
          <span class="stat-desc" id="statWanIp">-</span>
        </div>
        <div class="stat-card">
          <span class="stat-label">Klien Aktif Saat Ini</span>
          <span class="stat-val" id="statActiveClients">0</span>
          <span class="stat-desc">Perangkat Online di Hotspot</span>
        </div>
        <div class="stat-card">
          <span class="stat-label">MAC Tersimpan di Flash</span>
          <span class="stat-val"><span id="statStoredMacs">0</span> <small style="font-size:14px;color:var(--text-muted)">/ 254</small></span>
          <span class="stat-desc">Maks 254 Alamat Unique (FIFO)</span>
        </div>
        <div class="stat-card">
          <span class="stat-label">NAPT Routing Status</span>
          <span class="stat-val" id="statNaptStatus" style="color:var(--success);font-size:18px;">Aktif</span>
          <span class="stat-desc">Hardware IP Forwarding</span>
        </div>
      </div>

      <div class="grid-2">
        <div class="card">
          <div class="card-title">
            <span>&#128225; Info Jaringan Host (Upstream)</span>
            <span id="badgeWanDetail" class="badge badge-gray">-</span>
          </div>
          <div class="info-list">
            <div class="info-item"><span class="info-key">SSID Host</span><span class="info-val" id="valStaSsid">-</span></div>
            <div class="info-item"><span class="info-key">IP Address STA</span><span class="info-val" id="valStaIp">-</span></div>
            <div class="info-item"><span class="info-key">Gateway Host</span><span class="info-val" id="valStaGw">-</span></div>
            <div class="info-item"><span class="info-key">DNS Server</span><span class="info-val" id="valStaDns">-</span></div>
            <div class="info-item"><span class="info-key">Kekuatan Sinyal (RSSI)</span><span class="info-val" id="valStaRssi">-</span></div>
            <div class="info-item"><span class="info-key">MAC STA ESP32</span><span class="info-val" id="valStaMac">-</span></div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">
            <span>&#128246; Info Hotspot Forward (SoftAP)</span>
            <span class="badge badge-success"><span class="badge-dot"></span>Aktif</span>
          </div>
          <div class="info-list">
            <div class="info-item"><span class="info-key">SSID Forward</span><span class="info-val" id="valApSsid">-</span></div>
            <div class="info-item"><span class="info-key">IP Gateway Forward</span><span class="info-val">192.168.4.1</span></div>
            <div class="info-item"><span class="info-key">Subnet Mask</span><span class="info-val">255.255.255.0 (/24)</span></div>
            <div class="info-item"><span class="info-key">Rentang DHCP Otomatis</span><span class="info-val">192.168.4.2 - 192.168.4.254</span></div>
            <div class="info-item"><span class="info-key">Klien Terhubung</span><span class="info-val" id="valApClients">0 Perangkat</span></div>
            <div class="info-item"><span class="info-key">MAC AP ESP32</span><span class="info-val" id="valApMac">-</span></div>
          </div>
        </div>
      </div>
    </div>

    <!-- TAB 2: WIFI HOST (UPSTREAM) -->
    <div id="tab-host" class="tab-pane">
      <div class="card" style="max-width: 600px; margin: 0 auto;">
        <div class="card-title">
          <span>&#128225; Pengaturan WiFi Host (Sumber Internet)</span>
          <button class="btn btn-secondary btn-sm" onclick="scanWifi()">&#128269; Scan WiFi Sekitar</button>
        </div>
        
        <p style="font-size:13px; color:var(--text-muted); margin-bottom: 16px;">
          ESP32 akan terhubung ke WiFi host ini sebagai station untuk meneruskan akses internet ke jaringan hotspot forward.
        </p>

        <div id="scanLoading" style="display:none; margin-bottom:12px; color:var(--primary); font-size:13px;">
          &#9203; Sedang memindai jaringan WiFi 2.4 GHz sekitar...
        </div>
        <div id="scanResults" class="scan-list" style="display:none; margin-bottom:16px;"></div>

        <form id="formSta" onsubmit="saveSta(event)">
          <div class="form-group">
            <label for="sta_ssid">SSID WiFi Host</label>
            <input type="text" id="sta_ssid" required placeholder="Masukkan nama WiFi host">
          </div>
          <div class="form-group">
            <label for="sta_pass">Password WiFi Host</label>
            <div class="input-row">
              <input type="password" id="sta_pass" placeholder="Password WiFi host">
              <button type="button" class="btn btn-secondary btn-sm" onclick="togglePass('sta_pass')">&#128065;</button>
            </div>
          </div>
          <button type="submit" class="btn" style="width:100%;">&#128190; Simpan & Hubungkan Host</button>
        </form>
      </div>
    </div>

    <!-- TAB 3: WIFI FORWARD (SOFTAP) -->
    <div id="tab-forward" class="tab-pane">
      <div class="card" style="max-width: 600px; margin: 0 auto;">
        <div class="card-title">
          <span>&#128246; Pengaturan WiFi Forward (Hotspot ESP32)</span>
        </div>

        <p style="font-size:13px; color:var(--text-muted); margin-bottom: 16px;">
          Pengaturan nama jaringan (SSID) dan kata sandi hotspot yang dipancarkan oleh ESP32.
        </p>

        <form id="formAp" onsubmit="saveAp(event)">
          <div class="form-group">
            <label for="ap_ssid">SSID Hotspot Forward</label>
            <input type="text" id="ap_ssid" required placeholder="Nama WiFi Hotspot">
          </div>
          <div class="form-group">
            <label for="ap_pass">Password Hotspot Forward (Kosongkan jika Open)</label>
            <div class="input-row">
              <input type="password" id="ap_pass" placeholder="Minimal 8 karakter atau kosong">
              <button type="button" class="btn btn-secondary btn-sm" onclick="togglePass('ap_pass')">&#128065;</button>
            </div>
          </div>
          <div class="info-list" style="margin-bottom: 20px;">
            <div class="info-item"><span class="info-key">Subnet IP Hotspot</span><span class="info-val">192.168.4.0/24</span></div>
            <div class="info-item"><span class="info-key">IP Gateway ESP32</span><span class="info-val">192.168.4.1</span></div>
            <div class="info-item"><span class="info-key">Alokasi IP Otomatis</span><span class="info-val">192.168.4.2 - 192.168.4.254</span></div>
          </div>
          <button type="submit" class="btn" style="width:100%;">&#128190; Simpan Pengaturan Hotspot</button>
        </form>
      </div>
    </div>

    <!-- TAB 4: TABEL MAC & IP -->
    <div id="tab-leases" class="tab-pane">
      <div class="banner">
        <strong>&#8505; Sistem Deterministic DHCP & Flash Persistence:</strong><br>
        Setiap alamat MAC baru yang terhubung ke hotspot 192.168.4.0/24 otomatis diberi IP unik dan disimpan permanen ke Flash ESP32 (hingga 254 perangkat + 1 gateway = 255 alamat). Perangkat dengan MAC yang sama selalu dijamin memperoleh IP yang persis sama. Jika kapasitas 254 MAC penuh, MAC paling lama (FIFO) akan otomatis dihapus dan digantikan oleh perangkat baru.
      </div>

      <div class="search-bar">
        <input type="text" id="tableSearch" class="search-input" placeholder="&#128269; Cari berdasarkan IP atau MAC Address..." onkeyup="renderLeasesTable()">
        <button class="btn btn-secondary btn-sm" onclick="toggleShowAll()" id="btnToggleShowAll">Tampilkan Semua (255 IP)</button>
        <button class="btn btn-danger btn-sm" onclick="clearAllLeases()">&#128465; Reset Semua MAC</button>
      </div>

      <div class="table-container">
        <table>
          <thead>
            <tr>
              <th>Alamat IP</th>
              <th>Alamat MAC</th>
              <th>Status</th>
              <th>Antrean FIFO</th>
              <th>Terakhir Terlihat</th>
              <th style="text-align:center;">Aksi</th>
            </tr>
          </thead>
          <tbody id="leasesTableBody">
            <tr><td colspan="6" style="text-align:center;color:var(--text-muted);padding:30px;">Memuat data alokasi IP...</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- TAB 5: SISTEM -->
    <div id="tab-system" class="tab-pane">
      <!-- Card Kustom MAC Address (STA) -->
      <div class="card" style="margin-bottom: 24px;">
        <div class="card-title">
          <span>&#128279; Kustom & Kloning MAC Address (Host WAN / STA)</span>
          <span id="badgeMacStatus" class="badge badge-gray"><span class="badge-dot"></span>Bawaan Pabrik</span>
        </div>
        <div class="banner">
          <strong>Kloning MAC / Bypass Filter WiFi:</strong> Ubah MAC address interface Station (STA) ESP32 agar menyerupai perangkat Anda yang sudah terdaftar di whitelist Wi-Fi Host. Pastikan perangkat asli tidak terhubung bersamaan ke jaringan yang sama untuk mencegah konflik ARP/IP.
        </div>

        <div class="grid-2" style="margin-bottom: 16px;">
          <div class="info-list">
            <div class="info-item">
              <span class="info-key">MAC STA Aktif</span>
              <span class="info-val" id="sysActiveMac" style="font-family:monospace; color:var(--primary); font-size:15px;">-</span>
            </div>
            <div class="info-item">
              <span class="info-key">MAC Bawaan Pabrik (eFuse)</span>
              <span class="info-val" id="sysFactoryMac" style="font-family:monospace; font-size:14px;">-</span>
            </div>
          </div>
          <div class="info-list">
            <div class="info-item">
              <span class="info-key">Status Mode MAC</span>
              <span class="info-val" id="sysMacModeText">Bawaan Pabrik (Default)</span>
            </div>
            <div class="info-item">
              <span class="info-key">Target Interface</span>
              <span class="info-val">Station (Klien Host Wi-Fi)</span>
            </div>
          </div>
        </div>

        <div class="form-group">
          <label for="custom_mac_input">MAC Address Kustom Baru (Format: XX:XX:XX:XX:XX:XX)</label>
          <div class="input-row">
            <input type="text" id="custom_mac_input" placeholder="Contoh: 3C:52:82:11:22:33" maxlength="17" oninput="this.dataset.touched='true'" style="font-family:monospace; letter-spacing:1px; text-transform:uppercase;">
            <button class="btn btn-secondary btn-sm" type="button" onclick="fillFactoryMac()" title="Salin MAC bawaan pabrik ke kolom input">Salin MAC Pabrik</button>
          </div>
          <small style="color:var(--text-muted); font-size:12px; margin-top:6px; display:block;">
            *Catatan: Byte pertama harus bernilai genap (Unicast, bit ke-0 bernilai 0). Router akan restart secara otomatis (±5 dtk) untuk menerapkan MAC baru.
          </small>
        </div>

        <div style="display: flex; gap: 12px; flex-wrap: wrap; margin-top: 20px;">
          <button class="btn" onclick="saveCustomMac()">&#128190; Simpan & Terapkan MAC</button>
          <button class="btn btn-secondary" onclick="revertCustomMac()">&#8634; Kembalikan ke MAC Bawaan Pabrik</button>
        </div>
      </div>

      <div class="grid-2">
        <div class="card">
          <div class="card-title">&#128421; Info Perangkat Keras</div>
          <div class="info-list">
            <div class="info-item"><span class="info-key">Model Chip</span><span class="info-val" id="sysChipModel">ESP32</span></div>
            <div class="info-item"><span class="info-key">Uptime</span><span class="info-val" id="sysUptime">-</span></div>
            <div class="info-item"><span class="info-key">Free Heap RAM</span><span class="info-val" id="sysHeap">-</span></div>
            <div class="info-item"><span class="info-key">Flash Storage Mode</span><span class="info-val">NVS Preferences</span></div>
            <div class="info-item"><span class="info-key">Versi Firmware</span><span class="info-val">v2.0 (NAT Router + DHCP Binding)</span></div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">&#9881; Operasi Router</div>
          <p style="font-size:13px; color:var(--text-muted); margin-bottom: 20px;">
            Restart ulang modul ESP32 untuk memuat ulang seluruh stack jaringan atau kembalikan pengaturan ke default pabrik.
          </p>
          <div style="display: flex; flex-direction: column; gap: 12px;">
            <button class="btn btn-secondary" onclick="rebootEsp()">&#128260; Restart Router Sekarang</button>
            <button class="btn btn-danger" onclick="resetDefaults()">&#9888; Reset ke Pengaturan Default</button>
          </div>
        </div>
      </div>
    </div>
  </div>

  <div id="toast"></div>

  <script>
    let showAll255 = false;
    let cachedLeases = [];
    let apMacGlobal = "";

    function openTab(tabId) {
      document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
      document.querySelectorAll('.tab-pane').forEach(pane => pane.classList.remove('active'));
      event.currentTarget.classList.add('active');
      document.getElementById(tabId).classList.add('active');
      if (tabId === 'tab-leases') loadLeases();
    }

    function showToast(msg, isError = false) {
      const t = document.getElementById('toast');
      t.innerText = msg;
      t.style.display = 'block';
      t.style.background = isError ? '#ef4444' : '#1e293b';
      t.style.borderColor = isError ? '#b91c1c' : '#38bdf8';
      setTimeout(() => { t.style.display = 'none'; }, 3500);
    }

    function togglePass(id) {
      const input = document.getElementById(id);
      input.type = input.type === 'password' ? 'text' : 'password';
    }

    function toggleShowAll() {
      showAll255 = !showAll255;
      document.getElementById('btnToggleShowAll').innerText = showAll255 ? 'Tampilkan Hanya Terdaftar' : 'Tampilkan Semua (255 IP)';
      renderLeasesTable();
    }

    async function loadStatus() {
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        
        apMacGlobal = data.system.mac_ap;

        // WAN Status
        const wanConnected = data.sta.connected;
        const wanBadge = document.getElementById('headerWanBadge');
        const statWanStatus = document.getElementById('statWanStatus');
        const statWanIp = document.getElementById('statWanIp');
        const badgeWanDetail = document.getElementById('badgeWanDetail');

        if (wanConnected) {
          wanBadge.className = 'badge badge-success';
          wanBadge.innerHTML = '<span class="badge-dot"></span>WAN: ' + data.sta.ip;
          statWanStatus.innerText = 'Terhubung';
          statWanStatus.style.color = 'var(--success)';
          statWanIp.innerText = 'IP: ' + data.sta.ip + ' (' + data.sta.ssid + ')';
          badgeWanDetail.className = 'badge badge-success';
          badgeWanDetail.innerHTML = '<span class="badge-dot"></span>Terhubung';
        } else {
          wanBadge.className = 'badge badge-danger';
          wanBadge.innerHTML = '<span class="badge-dot"></span>WAN: Terputus';
          statWanStatus.innerText = 'Terputus';
          statWanStatus.style.color = 'var(--danger)';
          statWanIp.innerText = 'Mencoba terhubung ke: ' + data.sta.ssid;
          badgeWanDetail.className = 'badge badge-danger';
          badgeWanDetail.innerHTML = '<span class="badge-dot"></span>Terputus';
        }

        // Stats
        document.getElementById('statActiveClients').innerText = data.ap.clients_connected;
        document.getElementById('statStoredMacs').innerText = data.leases_summary.total_stored;
        document.getElementById('storedCountTab').innerText = data.leases_summary.total_stored;
        document.getElementById('statNaptStatus').innerText = data.napt ? 'Aktif' : 'Nonaktif';
        document.getElementById('statNaptStatus').style.color = data.napt ? 'var(--success)' : 'var(--danger)';

        // Details
        document.getElementById('valStaSsid').innerText = data.sta.ssid || '-';
        document.getElementById('valStaIp').innerText = data.sta.ip || '-';
        document.getElementById('valStaGw').innerText = data.sta.gw || '-';
        document.getElementById('valStaDns').innerText = data.sta.dns || '-';
        document.getElementById('valStaRssi').innerText = data.sta.rssi ? data.sta.rssi + ' dBm' : '-';
        
        const isSpoofed = data.system.is_mac_spoofed;
        document.getElementById('valStaMac').innerHTML = (data.system.mac_sta || '-') + (isSpoofed ? ' <span style="color:var(--primary);font-size:11px;font-weight:700;">(Kustom)</span>' : '');

        document.getElementById('valApSsid').innerText = data.ap.ssid;
        document.getElementById('valApClients').innerText = data.ap.clients_connected + ' Perangkat';
        document.getElementById('valApMac').innerText = data.system.mac_ap;

        // System & Chip Info
        const chip = data.system.chip_model || 'ESP32';
        const hChip = document.getElementById('headerChipModel');
        if (hChip) hChip.innerText = chip;
        const sChip = document.getElementById('sysChipModel');
        if (sChip) sChip.innerText = chip + ' (Xtensa Dual-Core)';

        // MAC info in System tab
        const activeStaMac = data.system.mac_sta || '-';
        const factoryStaMac = data.system.mac_sta_factory || activeStaMac;
        if (document.getElementById('sysActiveMac')) document.getElementById('sysActiveMac').innerText = activeStaMac;
        if (document.getElementById('sysFactoryMac')) document.getElementById('sysFactoryMac').innerText = factoryStaMac;

        const badgeMac = document.getElementById('badgeMacStatus');
        const modeText = document.getElementById('sysMacModeText');
        if (badgeMac && modeText) {
          if (isSpoofed) {
            badgeMac.className = 'badge badge-primary';
            badgeMac.innerHTML = '<span class="badge-dot"></span>Terkustom (Kloning)';
            modeText.innerHTML = '<span style="color:var(--primary);font-weight:700;">Kustom / Kloning Aktif</span>';
          } else {
            badgeMac.className = 'badge badge-gray';
            badgeMac.innerHTML = '<span class="badge-dot"></span>Bawaan Pabrik';
            modeText.innerText = 'Bawaan Pabrik (Default)';
          }
        }

        const macInput = document.getElementById('custom_mac_input');
        if (macInput && !macInput.dataset.touched) {
          macInput.value = data.system.mac_sta_custom || '';
        }

        const u = data.system.uptime;
        const h = Math.floor(u / 3600);
        const m = Math.floor((u % 3600) / 60);
        const s = u % 60;
        document.getElementById('sysUptime').innerText = `${h}j ${m}m ${s}s`;
        document.getElementById('sysHeap').innerText = Math.round(data.system.free_heap / 1024) + ' KB';

        // Set inputs if empty
        if (!document.getElementById('sta_ssid').value) document.getElementById('sta_ssid').value = data.sta.ssid;
        if (!document.getElementById('ap_ssid').value) document.getElementById('ap_ssid').value = data.ap.ssid;

      } catch (e) {
        console.error("Status fetch error", e);
      }
    }

    async function loadLeases() {
      try {
        const res = await fetch('/api/leases');
        cachedLeases = await res.json();
        renderLeasesTable();
      } catch (e) {
        console.error("Leases fetch error", e);
      }
    }

    function renderLeasesTable() {
      const q = document.getElementById('tableSearch').value.toLowerCase().trim();
      const tbody = document.getElementById('leasesTableBody');
      tbody.innerHTML = '';

      // Prepare list of 1..254 (1 = GW, 2..254 = clients)
      let rows = [];

      // Gateway
      rows.push({
        ip: '192.168.4.1',
        mac: apMacGlobal || 'ESP32 AP MAC',
        status: 'gateway',
        order: '-',
        last_seen: 'Aktif (Router)'
      });

      // Map stored
      let leaseMap = {};
      cachedLeases.forEach(l => {
        leaseMap[l.ip_suffix] = l;
      });

      for (let s = 2; s <= 254; s++) {
        const ipStr = '192.168.4.' + s;
        if (leaseMap[s]) {
          rows.push({
            ip: ipStr,
            mac: leaseMap[s].mac,
            status: leaseMap[s].online ? 'online' : 'stored',
            order: '#' + leaseMap[s].seq,
            seq: leaseMap[s].seq,
            last_seen: leaseMap[s].last_seen,
            ip_suffix: s
          });
        } else if (showAll255) {
          rows.push({
            ip: ipStr,
            mac: '-',
            status: 'free',
            order: '-',
            last_seen: 'Belum Terpakai',
            ip_suffix: s
          });
        }
      }

      // Filter
      const filtered = rows.filter(r => {
        if (!q) return true;
        return r.ip.toLowerCase().includes(q) || r.mac.toLowerCase().includes(q);
      });

      if (filtered.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" style="text-align:center;color:var(--text-muted);padding:24px;">Tidak ada data yang cocok dengan pencarian.</td></tr>';
        return;
      }

      filtered.forEach(r => {
        let badgeHtml = '';
        if (r.status === 'gateway') {
          badgeHtml = '<span class="badge badge-primary"><span class="badge-dot"></span>Gateway</span>';
        } else if (r.status === 'online') {
          badgeHtml = '<span class="badge badge-success"><span class="badge-dot"></span>Online</span>';
        } else if (r.status === 'stored') {
          badgeHtml = '<span class="badge badge-gray"><span class="badge-dot"></span>Tersimpan di Flash</span>';
        } else {
          badgeHtml = '<span class="badge" style="background:rgba(255,255,255,0.04);color:#64748b;">Kosong</span>';
        }

        let actionHtml = '-';
        if (r.status === 'online' || r.status === 'stored') {
          actionHtml = `<button class="btn btn-danger btn-sm" onclick="deleteLease(${r.ip_suffix}, '${r.ip}')">&#128465; Hapus</button>`;
        }

        const tr = document.createElement('tr');
        tr.innerHTML = `
          <td style="font-weight:700; font-family:monospace; font-size:14px; color:${r.status === 'gateway' ? 'var(--primary)' : 'white'}">${r.ip}</td>
          <td style="font-family:monospace; font-size:13px; color:${r.mac !== '-' ? '#e2e8f0' : 'var(--text-muted)'}">${r.mac}</td>
          <td>${badgeHtml}</td>
          <td style="color:var(--text-muted); font-size:12px;">${r.order}</td>
          <td style="color:var(--text-muted); font-size:12px;">${r.last_seen}</td>
          <td style="text-align:center;">${actionHtml}</td>
        `;
        tbody.appendChild(tr);
      });
    }

    async function deleteLease(ipSuffix, ipStr) {
      if (!confirm(`Hapus alokasi MAC untuk IP ${ipStr}? Slot IP ini akan dibebaskan.`)) return;
      try {
        const res = await fetch('/api/leases/delete', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'ip_suffix=' + ipSuffix
        });
        const data = await res.json();
        if (data.success) {
          showToast('Alokasi MAC untuk ' + ipStr + ' berhasil dihapus.');
          loadLeases();
          loadStatus();
        } else {
          showToast('Gagal menghapus lease.', true);
        }
      } catch (e) {
        showToast('Terjadi kesalahan saat menghapus.', true);
      }
    }

    async function clearAllLeases() {
      if (!confirm('PERINGATAN: Apakah Anda yakin ingin menghapus SEMUA MAC address yang tersimpan di Flash? Semua alokasi IP akan di-reset.')) return;
      try {
        const res = await fetch('/api/leases/clear', { method: 'POST' });
        const data = await res.json();
        if (data.success) {
          showToast('Semua MAC address di Flash berhasil dibersihkan.');
          loadLeases();
          loadStatus();
        }
      } catch (e) {
        showToast('Gagal mereset tabel MAC.', true);
      }
    }

    async function scanWifi() {
      const load = document.getElementById('scanLoading');
      const box = document.getElementById('scanResults');
      load.style.display = 'block';
      box.style.display = 'none';
      box.innerHTML = '';

      try {
        const res = await fetch('/api/scan');
        const list = await res.json();
        load.style.display = 'none';

        if (list.length === 0) {
          box.innerHTML = '<div style="color:var(--text-muted);font-size:13px;padding:8px;">Tidak ditemukan jaringan WiFi sekitar.</div>';
        } else {
          list.forEach(net => {
            const div = document.createElement('div');
            div.className = 'scan-item';
            div.onclick = () => {
              document.getElementById('sta_ssid').value = net.ssid;
              document.getElementById('sta_pass').focus();
              box.style.display = 'none';
              showToast('SSID "' + net.ssid + '" dipilih.');
            };
            div.innerHTML = `
              <div style="font-weight:600;color:white;">${net.ssid}</div>
              <div style="font-size:12px;color:var(--text-muted);display:flex;gap:10px;align-items:center;">
                <span>${net.secure ? '&#128274; Terkunci' : '&#128275; Terbuka'}</span>
                <span style="color:var(--primary);font-weight:700;">${net.rssi} dBm</span>
              </div>
            `;
            box.appendChild(div);
          });
        }
        box.style.display = 'flex';
      } catch (e) {
        load.style.display = 'none';
        showToast('Gagal memindai WiFi.', true);
      }
    }

    async function saveSta(e) {
      e.preventDefault();
      const ssid = document.getElementById('sta_ssid').value.trim();
      const pass = document.getElementById('sta_pass').value;

      try {
        const res = await fetch('/api/settings/sta', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: `ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`
        });
        const data = await res.json();
        if (data.success) {
          showToast('Pengaturan WiFi Host disimpan. Menghubungkan ke ' + ssid + '...');
          setTimeout(loadStatus, 2000);
        } else {
          showToast('Gagal menyimpan pengaturan host.', true);
        }
      } catch (e) {
        showToast('Gagal mengirim permintaan.', true);
      }
    }

    async function saveAp(e) {
      e.preventDefault();
      const ssid = document.getElementById('ap_ssid').value.trim();
      const pass = document.getElementById('ap_pass').value;

      if (pass.length > 0 && pass.length < 8) {
        showToast('Password Hotspot minimal 8 karakter!', true);
        return;
      }

      try {
        const res = await fetch('/api/settings/ap', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: `ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`
        });
        const data = await res.json();
        if (data.success) {
          showToast('Pengaturan Hotspot Forward disimpan! Menerapkan perubahan...');
          setTimeout(loadStatus, 2000);
        } else {
          showToast('Gagal menyimpan pengaturan AP.', true);
        }
      } catch (e) {
        showToast('Gagal mengirim permintaan.', true);
      }
    }

    function fillFactoryMac() {
      const fMac = document.getElementById('sysFactoryMac').innerText;
      if (fMac && fMac !== '-') {
        const inp = document.getElementById('custom_mac_input');
        inp.value = fMac;
        inp.dataset.touched = 'true';
        showToast('MAC Bawaan Pabrik disalin ke input.');
      }
    }

    async function saveCustomMac() {
      const inp = document.getElementById('custom_mac_input');
      const macVal = inp.value.trim().toUpperCase();

      if (!macVal) {
        showToast('Masukkan MAC Address yang valid!', true);
        return;
      }

      const macRegex = /^([0-9A-F]{2}[:-]){5}([0-9A-F]{2})$/i;
      if (!macRegex.test(macVal)) {
        showToast('Format MAC tidak valid! Gunakan format XX:XX:XX:XX:XX:XX', true);
        return;
      }

      const firstByte = parseInt(macVal.substring(0, 2), 16);
      if (firstByte & 1) {
        showToast('MAC tidak valid! Byte pertama harus genap (Unicast, bukan Multicast).', true);
        return;
      }

      if (!confirm(`Terapkan MAC Address kustom: ${macVal}?\n\nRouter akan restart otomatis (±5 detik) untuk menerapkan MAC baru ke interface WAN.`)) {
        return;
      }

      try {
        const res = await fetch('/api/settings/mac', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: `action=save&mac=${encodeURIComponent(macVal)}`
        });
        const data = await res.json();
        if (data.success) {
          showToast(data.msg || 'MAC berhasil disimpan! Router sedang restart...');
          inp.dataset.touched = '';
          setTimeout(() => { location.reload(); }, 6500);
        } else {
          showToast(data.msg || 'Gagal menyimpan MAC address.', true);
        }
      } catch (e) {
        showToast('Gagal mengirim permintaan.', true);
      }
    }

    async function revertCustomMac() {
      if (!confirm('Kembalikan MAC Address interface Host/STA ke bawaan pabrik (eFuse)?\n\nRouter akan restart otomatis (±5 detik).')) {
        return;
      }

      try {
        const res = await fetch('/api/settings/mac', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'action=revert'
        });
        const data = await res.json();
        if (data.success) {
          showToast(data.msg || 'MAC dikembalikan ke bawaan pabrik! Router restart...');
          const inp = document.getElementById('custom_mac_input');
          if (inp) {
            inp.value = '';
            inp.dataset.touched = '';
          }
          setTimeout(() => { location.reload(); }, 6500);
        } else {
          showToast(data.msg || 'Gagal mereset MAC address.', true);
        }
      } catch (e) {
        showToast('Gagal mengirim permintaan.', true);
      }
    }

    async function rebootEsp() {
      if (!confirm('Apakah Anda yakin ingin me-restart ESP32 Router? Koneksi akan terputus sementara.')) return;
      try {
        await fetch('/api/reboot', { method: 'POST' });
        showToast('Router sedang restart. Silakan tunggu 10 detik lalu refresh.');
      } catch (e) {
        showToast('Permintaan restart dikirim.');
      }
    }

    async function resetDefaults() {
      if (!confirm('PERINGATAN: Kembalikan seluruh pengaturan ke default pabrik?')) return;
      try {
        const res = await fetch('/api/reset', { method: 'POST' });
        const data = await res.json();
        if (data.success) {
          showToast('Pengaturan dikembalikan ke default. Router akan restart...');
        }
      } catch (e) {
        showToast('Gagal reset pengaturan.', true);
      }
    }

    // Auto-update status every 4 seconds
    loadStatus();
    setInterval(loadStatus, 4000);
  </script>
</body>
</html>
)rawliteral";

void setupWebRoutes() {
  // Main Dashboard
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  // Captive Portal Endpoints
  auto redirectRoot = []() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  };
  server.on("/generate_204", HTTP_GET, redirectRoot);
  server.on("/hotspot-detect.html", HTTP_GET, redirectRoot);
  server.on("/canonical.html", HTTP_GET, redirectRoot);
  server.on("/connecttest.txt", HTTP_GET, redirectRoot);
  server.on("/ncsi.txt", HTTP_GET, redirectRoot);

  // API Status
  server.on("/api/status", HTTP_GET, []() {
    String json = "{";
    
    // STA info
    json += "\"sta\":{";
    bool staConnected = (WiFi.status() == WL_CONNECTED);
    json += "\"connected\":" + String(staConnected ? "true" : "false") + ",";
    json += "\"ssid\":\"" + configManager.sta_ssid + "\",";
    json += "\"ip\":\"" + (staConnected ? WiFi.localIP().toString() : "") + "\",";
    json += "\"gw\":\"" + (staConnected ? WiFi.gatewayIP().toString() : "") + "\",";
    json += "\"dns\":\"" + (staConnected ? WiFi.dnsIP().toString() : "") + "\",";
    json += "\"rssi\":" + String(staConnected ? WiFi.RSSI() : 0);
    json += "},";

    // AP info
    json += "\"ap\":{";
    json += "\"ssid\":\"" + configManager.ap_ssid + "\",";
    json += "\"ip\":\"192.168.4.1\",";
    json += "\"clients_connected\":" + String(WiFi.softAPgetStationNum());
    json += "},";

    // NAPT & summary
    json += "\"napt\":" + String(staConnected ? "true" : "false") + ",";
    json += "\"leases_summary\":{";
    json += "\"total_stored\":" + String(leaseManager.getStoredCount()) + ",";
    json += "\"active_now\":" + String(leaseManager.getOnlineCount());
    json += "},";

    // System
    json += "\"system\":{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"chip_model\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"mac_ap\":\"" + WiFi.softAPmacAddress() + "\",";
    json += "\"mac_sta\":\"" + WiFi.macAddress() + "\",";
    json += "\"mac_sta_factory\":\"" + getFactoryStaMac() + "\",";
    json += "\"mac_sta_custom\":\"" + configManager.custom_sta_mac + "\",";
    json += "\"is_mac_spoofed\":" + String(configManager.hasCustomStaMac() ? "true" : "false");
    json += "}";

    json += "}";
    server.send(200, "application/json", json);
  });

  // API WiFi Scan
  server.on("/api/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks(false, false, false, 150);
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      json += "{";
      json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
      json += "}";
    }
    json += "]";
    WiFi.scanDelete();
    server.send(200, "application/json", json);
  });

  // API Settings STA
  server.on("/api/settings/sta", HTTP_POST, []() {
    if (!server.hasArg("ssid")) {
      server.send(400, "application/json", "{\"success\":false,\"msg\":\"Missing ssid\"}");
      return;
    }
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    configManager.saveSta(ssid, pass);

    WiFi.disconnect(false, false);
    WiFi.begin(ssid.c_str(), pass.c_str());

    server.send(200, "application/json", "{\"success\":true}");
  });

  // API Settings AP
  server.on("/api/settings/ap", HTTP_POST, []() {
    if (!server.hasArg("ssid")) {
      server.send(400, "application/json", "{\"success\":false,\"msg\":\"Missing ssid\"}");
      return;
    }
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    configManager.saveAp(ssid, pass);

    // Apply SoftAP changes
    WiFi.softAP(ssid.c_str(), pass.length() > 0 ? pass.c_str() : NULL);
    leaseManager.configureDhcpRange();

    server.send(200, "application/json", "{\"success\":true}");
  });

  // API Settings Custom STA MAC
  server.on("/api/settings/mac", HTTP_POST, []() {
    if (!server.hasArg("action")) {
      server.send(400, "application/json", "{\"success\":false,\"msg\":\"Argumen action diperlukan!\"}");
      return;
    }

    String action = server.arg("action");
    if (action == "revert") {
      configManager.revertStaMac();
      server.send(200, "application/json", "{\"success\":true,\"msg\":\"MAC Address dikembalikan ke bawaan pabrik! Router sedang restart...\"}");
      delay(600);
      ESP.restart();
      return;
    }

    if (action == "save") {
      if (!server.hasArg("mac")) {
        server.send(400, "application/json", "{\"success\":false,\"msg\":\"MAC Address diperlukan!\"}");
        return;
      }
      String macInput = server.arg("mac");
      macInput.trim();

      uint8_t parsedMac[6];
      if (!ConfigManager::parseMacString(macInput.c_str(), parsedMac)) {
        server.send(400, "application/json", "{\"success\":false,\"msg\":\"Format MAC tidak valid! Pastikan format XX:XX:XX:XX:XX:XX dan bit ke-0 bernilai genap (Unicast).\"}");
        return;
      }

      String canonicalMac = ConfigManager::formatMac(parsedMac);

      if (canonicalMac.equalsIgnoreCase(WiFi.softAPmacAddress())) {
        server.send(400, "application/json", "{\"success\":false,\"msg\":\"MAC STA tidak boleh sama dengan MAC SoftAP router!\"}");
        return;
      }

      configManager.saveCustomStaMac(canonicalMac);

      server.send(200, "application/json", "{\"success\":true,\"msg\":\"MAC Address " + canonicalMac + " berhasil disimpan! Router sedang restart...\"}");
      delay(600);
      ESP.restart();
      return;
    }

    server.send(400, "application/json", "{\"success\":false,\"msg\":\"Aksi tidak dikenal!\"}");
  });

  // API Leases List
  server.on("/api/leases", HTTP_GET, []() {
    String json = "[";
    bool first = true;
    for (int i = 0; i < TOTAL_CLIENT_LEASES; i++) {
      if (leaseManager.leases[i].valid) {
        if (!first) json += ",";
        first = false;

        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
          leaseManager.leases[i].mac[0], leaseManager.leases[i].mac[1],
          leaseManager.leases[i].mac[2], leaseManager.leases[i].mac[3],
          leaseManager.leases[i].mac[4], leaseManager.leases[i].mac[5]);

        uint32_t diffSec = (millis() - leaseManager.lastSeenMs[i]) / 1000;
        String relTime;
        if (leaseManager.onlineStatus[i]) {
          relTime = "Online sekarang";
        } else if (diffSec < 60) {
          relTime = String(diffSec) + " dtk lalu";
        } else if (diffSec < 3600) {
          relTime = String(diffSec / 60) + " mnt lalu";
        } else {
          relTime = String(diffSec / 3600) + " jam lalu";
        }

        json += "{";
        json += "\"ip_suffix\":" + String(leaseManager.leases[i].ip_suffix) + ",";
        json += "\"mac\":\"" + String(macStr) + "\",";
        json += "\"online\":" + String(leaseManager.onlineStatus[i] ? "true" : "false") + ",";
        json += "\"seq\":" + String(leaseManager.leases[i].seq) + ",";
        json += "\"last_seen\":\"" + relTime + "\"";
        json += "}";
      }
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  // API Delete Lease
  server.on("/api/leases/delete", HTTP_POST, []() {
    if (!server.hasArg("ip_suffix")) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    uint8_t suffix = server.arg("ip_suffix").toInt();
    bool ok = leaseManager.deleteLease(suffix);
    server.send(200, "application/json", ok ? "{\"success\":true}" : "{\"success\":false}");
  });

  // API Clear All Leases
  server.on("/api/leases/clear", HTTP_POST, []() {
    leaseManager.clearAll();
    server.send(200, "application/json", "{\"success\":true}");
  });

  // API Reboot
  server.on("/api/reboot", HTTP_POST, []() {
    server.send(200, "application/json", "{\"success\":true}");
    delay(500);
    ESP.restart();
  });

  // API Factory Reset
  server.on("/api/reset", HTTP_POST, []() {
    configManager.resetDefaults();
    leaseManager.clearAll();
    server.send(200, "application/json", "{\"success\":true}");
    delay(500);
    ESP.restart();
  });

  // 404 / Captive Portal Fallback
  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
  Serial.println("[Web] HTTP Web Server started on port 80");
}

#endif // WEB_INTERFACE_H
