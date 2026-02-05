#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

/* ================= PIN ================= */
#define LED_PIN 2
#define BTN_PIN 0

/* ================= PWM ================= */
#define LED_CHANNEL 0
#define LED_FREQ    5000
#define LED_RES     8   // 8-bit (0–255)

/* ================= MQTT ================= */
const char* mqttServer = "public.cloud.shiftr.io";
const int   mqttPort   = 1883;
const char* mqttUser   = "public";
const char* mqttPass   = "public";

/* ================= OBJECT ================= */
WebServer server(80);
Preferences prefs;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

/* ================= GLOBAL STATE ================= */
String ssid, password;

bool gordenKeluargaState = false;
bool gordenKamarState     = false;

bool *gordenKeluarga = &gordenKeluargaState;
bool *gordenKamar    = &gordenKamarState;

/* ================= INTERRUPT ================= */
void IRAM_ATTR toggleGordenKeluarga() {
  *gordenKeluarga = !(*gordenKeluarga);
  Serial.println("[INTERRUPT] Toggle Gorden Ruang Keluarga");
}

/* ================= MQTT CALLBACK ================= */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (uint8_t i = 0; i < length; i++) msg += (char)payload[i];

  Serial.printf("[MQTT] %s => %s\n", topic, msg.c_str());

  if (String(topic) == "smarthome/esp32/gorden/keluarga") {
    *gordenKeluarga = (msg == "OPEN");
    Serial.println("[MQTT] Gorden Ruang Keluarga " + msg);
  }

  if (String(topic) == "smarthome/esp32/gorden/kamar") {
    *gordenKamar = (msg == "OPEN");
    Serial.println("[MQTT] Gorden Kamar " + msg);
  }
}

/* ================= MQTT CONNECT ================= */
void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("[MQTT] Connecting...");
    if (mqttClient.connect("ESP32_SMARTHOME", mqttUser, mqttPass)) {
      Serial.println("[MQTT] Connected");
      mqttClient.subscribe("smarthome/esp32/gorden/keluarga");
      mqttClient.subscribe("smarthome/esp32/gorden/kamar");
    } else {
      delay(2000);
    }
  }
}

/* ================= RTOS TASK: MQTT ================= */
void taskMQTT(void *pv) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqttClient.connected()) connectMQTT();
      mqttClient.loop();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/* ================= RTOS TASK: LED (PWM) ================= */
void taskLED(void *pv) {
  for (;;) {
    int duty = (*gordenKeluarga || *gordenKamar) ? 0 : 255;
    ledcWrite(LED_CHANNEL, 0);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

/* ================= WEB PAGE ================= */
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Dashboard Controller</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{font-family:Arial;background:linear-gradient(135deg,#ffe4e1,#fff0f5);padding:20px}
.card{max-width:420px;margin:auto;background:#fff;padding:18px;border-radius:14px;box-shadow:0 6px 18px rgba(0,0,0,.15)}
.header{border:1px solid #f4c2c2;background:#ffc1c1;padding:12px;border-radius:10px;text-align:center;font-weight:bold;font-size:20px}
.box{border:1px solid #ddd;border-radius:10px;padding:12px;margin-bottom:14px;background:#f8fafc}
button,input{width:100%;padding:12px;margin:6px 0;border-radius:6px;border:none}
.open{background:#2ecc71;color:#fff}
.close{background:#e74c3c;color:#fff}
.save{background:#64748b;color:#fff}
</style>
</head>
<body>
<div class="card">
<div class="header">Dashboard Controller</div>

<div class="box">
<h3>Status</h3>
<p>WiFi : <b id="wifi">-</b></p>
<p>IP : <b id="ip">-</b></p>
<p>Connected To : <b id="ssid">-</b></p>
</div>

<div class="box">
<h3>Gorden Status</h3>
<p>Gorden Ruang Keluarga : <b id="keluarga">-</b></p>
<p>Gorden Kamar Tidur : <b id="kamar">-</b></p>
<p>LED ESP32 : <b id="led">-</b></p>
</div>

<div class="box">
<h3>WiFi Configuration</h3>
<input id="ssidInput" placeholder="SSID">
<input id="password" type="password" placeholder="Password">
<button class="save" onclick="saveWiFi()">Save WiFi</button>
</div>

<h3>Kontrol Gorden</h3>
<button class="open" onclick="fetch('/gorden/keluarga/open',{method:'POST'})">Buka Gorden Ruang Keluarga</button>
<button class="close" onclick="fetch('/gorden/keluarga/close',{method:'POST'})">Tutup Gorden Ruang Keluarga</button>
<button class="open" onclick="fetch('/gorden/kamar/open',{method:'POST'})">Buka Gorden Kamar Tidur</button>
<button class="close" onclick="fetch('/gorden/kamar/close',{method:'POST'})">Tutup Gorden Kamar Tidur</button>

</div>

<script>
function refresh(){
 fetch('/status').then(r=>r.json()).then(d=>{
  wifi.innerText=d.wifi;
  ip.innerText=d.ip;
  ssid.innerText=d.ssid;
  keluarga.innerText=d.keluarga?'OPEN':'CLOSE';
  kamar.innerText=d.kamar?'OPEN':'CLOSE';
  led.innerText=d.led?'ON':'OFF';
 });
}
function saveWiFi(){
 fetch('/wifi',{
  method:'POST',
  headers:{'Content-Type':'application/json'},
  body:JSON.stringify({ssid:ssidInput.value,password:password.value})
 });
 alert('WiFi disimpan');
}
setInterval(refresh,2000);refresh();
</script>
</body>
</html>
)rawliteral";

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 BOOTING ===");

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  attachInterrupt(BTN_PIN, toggleGordenKeluarga, FALLING);

  /* ===== PWM INIT ===== */
  ledcAttach(LED_PIN, LED_FREQ, LED_RES);

  prefs.begin("wifi", false);
  ssid = prefs.getString("ssid","");
  password = prefs.getString("password","");

  if (ssid == "") startAPMode();
  else connectWiFi();

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  server.on("/", [](){ server.send_P(200,"text/html",webpage); });

  server.on("/wifi", HTTP_POST, [](){
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    prefs.putString("ssid",ssid);
    prefs.putString("password",password);
    Serial.println("[WIFI] Credentials saved");
    Serial.println("[WIFI] SSID: " + ssid);
    server.send(200,"application/json","{\"status\":\"ok\"}");
    connectWiFi();
  });

  server.on("/gorden/keluarga/open", HTTP_POST, [](){
    *gordenKeluarga=true;
    mqttClient.publish("smarthome/esp32/gorden/keluarga","OPEN");
    Serial.println("[WEB] Gorden Ruang Keluarga OPEN");
    server.send(200);
  });

  server.on("/gorden/keluarga/close", HTTP_POST, [](){
    *gordenKeluarga=false;
    mqttClient.publish("smarthome/esp32/gorden/keluarga","CLOSE");
    Serial.println("[WEB] Gorden Ruang Keluarga CLOSE");
    server.send(200);
  });

  server.on("/gorden/kamar/open", HTTP_POST, [](){
    *gordenKamar=true;
    mqttClient.publish("smarthome/esp32/gorden/kamar","OPEN");
    Serial.println("[WEB] Gorden Kamar Tidur OPEN");
    server.send(200);
  });

  server.on("/gorden/kamar/close", HTTP_POST, [](){
    *gordenKamar=false;
    mqttClient.publish("smarthome/esp32/gorden/kamar","CLOSE");
    Serial.println("[WEB] Gorden Kamar Tidur CLOSE");
    server.send(200);
  });

  server.on("/status", [](){
    DynamicJsonDocument doc(256);
    doc["wifi"] = WiFi.status()==WL_CONNECTED?"Connected":"AP Mode";
    doc["ip"]   = WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():WiFi.softAPIP().toString();
    doc["ssid"] = WiFi.status()==WL_CONNECTED?WiFi.SSID():"ESP32-Config";
    doc["keluarga"]= *gordenKeluarga;
    doc["kamar"]   = *gordenKamar;
    doc["led"]     = (*gordenKeluarga || *gordenKamar);
    String res;
    serializeJson(doc,res);
    server.send(200,"application/json",res);
  });

  server.begin();

  xTaskCreatePinnedToCore(taskMQTT,"MQTT",4096,NULL,1,NULL,1);
  xTaskCreatePinnedToCore(taskLED,"LED",2048,NULL,1,NULL,1);
}

/* ================= LOOP ================= */
void loop() {
  server.handleClient();
}

/* ================= WIFI ================= */
void connectWiFi(){
  Serial.println("[WIFI] Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(),password.c_str());
  unsigned long t=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-t<15000){
    delay(300);
  }
  if(WiFi.status()==WL_CONNECTED){
    Serial.println("[WIFI] Connected to: "+WiFi.SSID());
    Serial.println("[IP] "+WiFi.localIP().toString());
  } else {
    Serial.println("[WIFI] Failed, switching to AP");
    startAPMode();
  }
}

void startAPMode(){
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Config");
  Serial.println("[AP MODE] ESP32-Config");
  Serial.println("[IP] "+WiFi.softAPIP().toString());
}
