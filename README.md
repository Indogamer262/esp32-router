# ESP32 NAT Router & Wi-Fi Extender Firmware v2.0

Firmware router jaringan portabel berbasis ESP32 dengan fitur **NAPT (*Network Address Port Translation*)**, **Manajemen Web UI Modern (*Responsive Dark Mode*)**, **Captive Portal**, **DHCP Binding Statis Deterministik ke Flash NVS**, serta **Kloning / Kustom MAC Address (STA)** untuk mem-bypass pembatasan *MAC filtering* pada jaringan Wi-Fi sumber.

---

## 🚀 Fitur Utama

- **NAPT Wi-Fi Repeater / Range Extender**: Meneruskan koneksi internet dari Wi-Fi sumber (Host) ke jaringan lokal baru (Hotspot) menggunakan translasi alamat NAPT lwIP.
- **Modern Responsive Web Interface**:
  - UI bertema gelap (*Dark Mode*), bersih, dan intuitif.
  - Berjalan 100% *offline / air-gapped* langsung dari Flash ESP32 (`PROGMEM`), tanpa dependensi CDN eksternal (CDN-free).
  - Monitoring metrik *real-time* (Status WAN, RSSI dBm, IP, DNS, Gateway, Uptime, Free Heap, dan Jumlah Klien).
- **Captive Portal Otomatis**: Dilengkapi DNS Server internal (*port 53*) yang menangkap *probe* koneksi perangkat (Android, iOS/macOS, Windows) dan mengarahkan otomatis ke antarmuka `http://192.168.4.1/`.
- **Tabel MAC & IP Statis Deterministik (Lease Manager)**:
  - Subnet lokal terisolasi: `192.168.4.0/24` (Gateway: `192.168.4.1`, Rentang Klien: `192.168.4.2` – `192.168.4.254`).
  - Mengikat (*bind*) MAC address perangkat klien secara permanen ke Flash NVS (Preferences).
  - Setiap perangkat yang pernah terhubung akan selalu mendapatkan IP lokal yang persis sama meski router di-restart.
  - Mekanisme **FIFO Eviction** otomatis jika kapasitas 253 klien penuh.
  - Tabel interaktif untuk mencari, memantau perangkat online/offline, dan menghapus alokasi lease secara individual atau massal.
- **Kloning & Kustom MAC Address (STA)**:
  - Mengganti MAC Address interface Station (klien WAN) untuk menduplikasi identitas perangkat yang telah di-*whitelist* oleh administrator Wi-Fi target.
  - Validasi otomatis format alamat Unicast (bit ke-0 byte pertama harus bernilai genap).
  - Dilengkapi fitur **Revert ke MAC Pabrik** yang membaca hardware MAC permanen dari eFuse chip.
- **Wi-Fi Scanner**: Pemindai jaringan Wi-Fi sekitar secara langsung dari antarmuka web untuk kemudahan konfigurasi SSID & kata sandi host.
- **Dukungan Multi-Chip**: Kompatibel penuh dengan keluarga **ESP32 (WROOM-32)** dan **ESP32-S3 (termasuk varian N16R8)**.

---

## 🛠️ Kompatibilitas Hardware

Firmware ini dirancang secara modular dan dapat digunakan pada modul ESP32 generasi manapun:

| Parameter | ESP32-WROOM-32 | ESP32-S3-N16R8 |
| :--- | :--- | :--- |
| **Arsitektur CPU** | Xtensa Dual-Core 32-bit LX6 (240 MHz) | Xtensa Dual-Core 32-bit LX7 (240 MHz) |
| **Flash Memory** | 4 MB SPI Flash | 16 MB Octal/Quad SPI Flash |
| **RAM Internal** | 520 KB SRAM | 512 KB SRAM |
| **PSRAM Eksternal** | Tidak ada | 8 MB Octal SPI PSRAM |
| **Status Kompatibilitas** | ✅ Kompatibel Penuh | ✅ Kompatibel Penuh (Performa & buffer lebih tinggi) |

---

## 📂 Struktur Proyek

```text
esp32-router/
├── README.md
└── esp32-router-firmware/
    ├── esp32-router-firmware.ino  # Program utama setup(), loop(), dan Network Events
    ├── ConfigManager.h            # Pengelola konfigurasi SSID/Pass & MAC Kustom (NVS)
    ├── LeaseManager.h             # Pengelola injeksi DHCP statis & tabel 253 klien (NVS)
    └── WebInterface.h             # Server web, API JSON, antarmuka HTML/CSS/JS (PROGMEM)
```

---

## 💻 Panduan Kompilasi & Flashing

### Persyaratan Perangkat Lunak
1. [Arduino IDE](https://www.arduino.cc/en/software) v2.x (atau VS Code dengan PlatformIO / Arduino-CLI).
2. **ESP32 Arduino Core v3.0.0 atau lebih baru** (ESP-IDF v5.x based).
   - Buka *File* > *Preferences* > *Additional boards manager URLs*:
     ```text
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
   - Buka *Tools* > *Board* > *Boards Manager*, cari `esp32` oleh Espressif Systems dan pasang versi **3.x.x**.

---

### Pengaturan Board Arduino IDE

#### 1. Untuk ESP32-WROOM-32 / DevKit v1:
- **Board**: `ESP32 Dev Module` (atau `DOIT ESP32 DEVKIT V1`)
- **Upload Speed**: `921600` (atau `115200`)
- **CPU Frequency**: `240MHz (WiFi/BT)`
- **Flash Frequency**: `80MHz`
- **Flash Mode**: `QIO`
- **Flash Size**: `4MB (32Mb)`
- **Partition Scheme**: `Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)` atau `Minimal SPIFFS (1.9MB APP)`
- **Core Debug Level**: `None`

#### 2. Untuk ESP32-S3-N16R8:
- **Board**: `ESP32S3 Dev Module`
- **USB CDC On Boot**: `Enabled` *(Wajib aktif jika Serial Monitor melalui port USB native Type-C)*
- **CPU Frequency**: `240MHz (WiFi)`
- **Flash Size**: `16MB (128Mb)`
- **Flash Mode**: `OPI 80MHz` (atau `QIO 80MHz`)
- **PSRAM**: `OPI PSRAM`
- **Partition Scheme**: `16M Flash (3MB APP/9.9MB FATFS)`
- **Upload Mode**: `UART0 / Hardware CDC`

---

## 📖 Petunjuk Penggunaan

### 1. Menghubungkan ke Router Pertama Kali
1. Nyalakan modul ESP32 setelah di-flash.
2. Hubungkan perangkat (Smartphone / Laptop) ke jaringan Wi-Fi bawaan:
   - **SSID**: `ESP32-ROUTER`
   - **Password**: *(Kosong / Terbuka)*
3. Pop-up Captive Portal akan otomatis muncul. Jika tidak muncul otomatis, buka browser dan ketik alamat IP:
   ```text
   http://192.168.4.1/
   ```

### 2. Mengatur Wi-Fi Sumber (Host)
1. Buka tab **"WiFi Host (Sumber)"**.
2. Klik tombol **"Pindai Jaringan Sekitar"** untuk melihat daftar Wi-Fi yang tersedia, atau ketik SSID dan password secara manual.
3. Klik **"Simpan & Hubungkan"**.
4. ESP32 akan menyambungkan diri ke router sumber dan mengaktifkan fungsionalitas NAPT (Internet Sharing).

### 3. Mengatur Wi-Fi Lokal (Hotspot Forward)
1. Buka tab **"WiFi Forward (Hotspot)"**.
2. Ubah nama SSID dan kata sandi WPA2 sesuai kebutuhan (minimal 8 karakter).
3. Klik **"Simpan & Terapkan"**.

### 4. Menggunakan Fitur Kloning MAC Address
Fitur ini berguna jika Anda berada di jaringan Wi-Fi yang menerapkan *MAC Filtering / Whitelist*:
1. Buka tab **"Sistem"**.
2. Pada panel **"Kustom & Kloning MAC Address (Host WAN / STA)"**, perhatikan:
   - **MAC STA Aktif**: MAC address yang sedang digunakan interface Station.
   - **MAC Bawaan Pabrik**: Alamat hardware asli dari eFuse chip.
3. Masukkan MAC address perangkat Anda yang sudah terdaftar di whitelist pada kolom input (contoh: `3C:52:82:11:22:33`).
4. Klik **"Simpan & Terapkan MAC"**.
5. ESP32 akan menyimpan nilai ke Flash dan otomatis melakukan restart dalam ~5 detik untuk memuat identitas jaringan yang baru.
6. **Untuk kembali ke bawaan pabrik**: Klik tombol **"Kembalikan ke MAC Bawaan Pabrik"**.

> [!NOTE]
> Pastikan perangkat asli Anda **tidak terhubung ke Wi-Fi sumber yang sama secara bersamaan** dengan ESP32 untuk menghindari konflik ARP dan tabrakan alokasi IP di router utama.

### 5. Memantau & Mengelola Tabel DHCP Statis
1. Buka tab **"Tabel MAC & IP"**.
2. Anda dapat melihat seluruh perangkat yang pernah terhubung beserta IP statis yang dialokasikan (192.168.4.2 – 192.168.4.254).
3. Tombol **"Hapus"** dapat digunakan untuk membebaskan alokasi slot IP tertentu.
4. Tombol **"Hapus Semua Data Flash"** akan membersihkan seluruh riwayat lease MAC address dari memori Flash.

---

## 🔌 Dokumentasi REST API

Seluruh fungsionalitas router dapat dikendalikan secara terprogram melalui endpoint HTTP REST:

| Endpoint | Metode | Parameter | Deskripsi |
| :--- | :---: | :--- | :--- |
| `/api/status` | `GET` | - | Mengambil status lengkap router (WAN, AP, Lease, Uptime, Memory, Chip, dan MAC). |
| `/api/scan` | `GET` | - | Melakukan pemindaian jaringan Wi-Fi sekitar dan mengembalikan daftar SSID, RSSI, dan status enkripsi. |
| `/api/settings/sta` | `POST` | `ssid`, `pass` | Menyimpan konfigurasi Wi-Fi sumber (Host) dan memicu koneksi ulang. |
| `/api/settings/ap` | `POST` | `ssid`, `pass` | Mengubah SSID dan password Wi-Fi Hotspot lokal. |
| `/api/settings/mac` | `POST` | `action` (`save`/`revert`), `mac` | Mengatur MAC kustom atau mengembalikan ke MAC bawaan pabrik (memicu auto-reboot). |
| `/api/leases` | `GET` | - | Mengambil seluruh daftar alokasi MAC-to-IP statis dari Flash. |
| `/api/leases/delete`| `POST` | `ip_suffix` (2-254) | Menghapus alokasi MAC untuk IP suffix tertentu. |
| `/api/leases/clear` | `POST` | - | Menghapus seluruh tabel alokasi lease dari Flash memory. |
| `/api/reboot` | `POST` | - | Me-restart modul ESP32. |
| `/api/reset` | `POST` | - | Melakukan factory reset (menghapus konfigurasi jaringan & tabel lease). |

---

## 🔒 Keamanan & Kebijakan

- Fitur kloning MAC address disediakan untuk fleksibilitas administrasi jaringan pribadi dan pengujian kompatibilitas perangkat IoT.
- Selalu patuhi regulasi dan kebijakan administrator jaringan tempat Anda mengoperasikan perangkat.

---

## 📄 Lisensi

Proyek ini dirilis di bawah lisensi [MIT License](LICENSE). Bebas digunakan, dimodifikasi, dan didistribusikan untuk keperluan personal maupun komersial.

