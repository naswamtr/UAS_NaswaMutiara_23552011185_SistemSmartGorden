# UAS_NaswaMutiara_23552011185_SistemSmartGorden

# Smart Gorden Berbasis ESP32

# Link Video Demo : https://youtu.be/u03DVq8Cbj8

# 1. Deskripsi
Proyek ini merupakan sistem smart gorden otomatis yang dapat dikontrol secara manual melalui web dashboard atau jarak jauh melalui MQTT. ESP32 digunakan sebagai controller utama, terhubung ke jaringan WiFi, dan dapat beroperasi dalam STA mode (terhubung ke router) atau AP mode (akses langsung ke ESP32). Sistem juga menampilkan status LED sebagai indikator gorden terbuka.
Fitur utama:
1. Kontrol gorden Ruang Keluarga dan Kamar Tidur.
2. Kontrol manual melalui web browser.
3. Kontrol jarak jauh via MQTT (HiveMQ).
4. LED indikator menyala jika gorden terbuka.
5. Sistem stabil dan responsif menggunakan FreeRTOS task dan interrupt.

# 2. Alur Sistem
1. ESP32 booting dan membaca konfigurasi WiFi dari Preferences.
2. Menghubungkan ke WiFi (STA mode). Jika gagal, membuat Access Point sendiri.
3. Menginisialisasi MQTT untuk berlangganan topik:
- smarthome/esp32/gorden/keluarga
- smarthome/esp32/gorden/kamar
4. Menjalankan web server pada port 80 dengan dashboard interaktif:
- Status WiFi, IP, SSID
- Status gorden dan LED
- Kontrol buka/tutup gorden
- Form input WiFi
5. Interaksi:
- Tekan tombol di web → ESP32 mengubah status gorden & publish MQTT.
- Terima pesan MQTT → ESP32 mengubah status gorden sesuai perintah.
- LED menyala jika salah satu gorden terbuka.
