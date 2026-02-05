# UAS_NaswaMutiara_23552011185_SistemSmartGorden

# Smart Gorden Berbasis ESP32
1. Deskripsi

Proyek ini merupakan sistem smart gorden otomatis yang dapat dikontrol secara manual melalui web dashboard atau jarak jauh melalui MQTT. ESP32 digunakan sebagai controller utama, terhubung ke jaringan WiFi, dan dapat beroperasi dalam STA mode (terhubung ke router) atau AP mode (akses langsung ke ESP32). Sistem juga menampilkan status LED sebagai indikator gorden terbuka.

Fitur utama:

Kontrol gorden Ruang Keluarga dan Kamar Tidur.

Kontrol manual melalui web browser.

Kontrol jarak jauh via MQTT (HiveMQ).

LED indikator menyala jika gorden terbuka.

Sistem stabil dan responsif menggunakan FreeRTOS task dan interrupt.

Alur Sistem

ESP32 booting dan membaca konfigurasi WiFi dari Preferences.

Menghubungkan ke WiFi (STA mode). Jika gagal, membuat Access Point sendiri.

Menginisialisasi MQTT untuk berlangganan topik:

smarthome/esp32/gorden/keluarga

smarthome/esp32/gorden/kamar

Menjalankan web server pada port 80 dengan dashboard interaktif:

Status WiFi, IP, SSID

Status gorden dan LED

Kontrol buka/tutup gorden

Form input WiFi

Interaksi:

Tekan tombol di web → ESP32 mengubah status gorden & publish MQTT.

Terima pesan MQTT → ESP32 mengubah status gorden sesuai perintah.

LED menyala jika salah satu gorden terbuka.

Diagram Sistem
[Web Browser] <----> [ESP32 Controller] <----> [Motor Gorden]
                 |
                 v
               [MQTT Broker]

Potongan Kode Utama

1. Library

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>


2. Pin dan PWM

#define LED_PIN 2
#define BTN_PIN 0

#define LED_CHANNEL 0
#define LED_FREQ    5000
#define LED_RES     8


3. Status Global & Pointer

bool gordenKeluargaState = false;
bool gordenKamarState     = false;

bool *gordenKeluarga = &gordenKeluargaState;
bool *gordenKamar    = &gordenKamarState;


4. Interrupt Tombol

void IRAM_ATTR toggleGordenKeluarga() {
  *gordenKeluarga = !(*gordenKeluarga);
  Serial.println("[INTERRUPT] Toggle Gorden Ruang Keluarga");
}


5. Callback MQTT

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (uint8_t i = 0; i < length; i++) msg += (char)payload[i];

  if (String(topic) == "smarthome/esp32/gorden/keluarga") {
    *gordenKeluarga = (msg == "OPEN");
  }
  if (String(topic) == "smarthome/esp32/gorden/kamar") {
    *gordenKamar = (msg == "OPEN");
  }
}


6. FreeRTOS Task untuk MQTT

void taskMQTT(void *pv) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqttClient.connected()) connectMQTT();
      mqttClient.loop();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


7. Web Server Dashboard

server.on("/gorden/keluarga/open", HTTP_POST, [](){
  *gordenKeluarga=true;
  mqttClient.publish("smarthome/esp32/gorden/keluarga","OPEN");
  server.send(200);
});


8. Manajemen WiFi

void connectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(),password.c_str());
  unsigned long t=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-t<15000){
    delay(300);
  }
  if(WiFi.status()!=WL_CONNECTED) startAPMode();
}

void startAPMode(){
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Config");
}

Cara Penggunaan

Upload kode ke ESP32.

Konfigurasi WiFi melalui web dashboard atau AP mode.

Buka dashboard di browser untuk kontrol manual.

Hubungkan ke MQTT broker untuk kontrol jarak jauh.

LED akan menyala jika gorden terbuka.

Lisensi

Proyek ini bebas digunakan untuk pembelajaran, referensi, atau modifikasi pribadi.
