#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <MAX30105.h>
#include "heartRate.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_MLX90614.h>
#include <TinyGPSPlus.h>

// ================= WIFI =================
const char *ssid = "Admin";
const char *password = "12345678";

// ================= WHATSAPP API =================
const char *apiKey = "cd_hea_170226_acy-Kt";
const char *waHost = "www.circuitdigest.cloud";
const int httpsPort = 443;
const char *phoneNumber = "919486229298";
#define WA_COOLDOWN_MS 60000

WiFiClientSecure waClient;
unsigned long lastWaSentTime = 0;

// ================= OBJECTS =================
WebServer server(80);
MAX30105 max30102;
Adafruit_MPU6050 mpu;
Adafruit_MLX90614 mlx;
TinyGPSPlus gps;

// ================= GPS =================
HardwareSerial gpsSerial(2);
#define GPS_RX 16
#define GPS_TX 17

#define DEFAULT_LAT 11.025310
#define DEFAULT_LON 77.002599

// ================= FALL DETECTION =================
#define FALL_Z_THRESHOLD 0.0
#define FALL_CONFIRM_MS  3000   // Must stay fallen for 3 seconds before alert

// ================= DATA =================
int bpm = 0;
int spo2 = 0;
float bodyTemp = 0;
bool fallDetected = false;
float latitude = DEFAULT_LAT;
float longitude = DEFAULT_LON;
float accX = 0.0;
float accY = 0.0;
float accZ = 0.0;

// ================= FALL TIMING =================
bool fallCandidate = false;          // true = device is currently upside down
unsigned long fallCandidateStart = 0; // when it first flipped
bool alertSentForThisFall = false;   // prevents repeated alerts for same fall event

// ================= FUNCTION DECLARATIONS =================
void sendData();
void sendWhatsAppAlert(float lat, float lon);

// ================= HTML =================
const char MAIN_page[] PROGMEM = R"====(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Health Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: #fff;
      min-height: 100vh;
      padding: 20px;
    }
    .container { max-width: 1200px; margin: 0 auto; }
    header { text-align: center; margin-bottom: 30px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }
    header h1 { font-size: 2.5em; margin-bottom: 5px; font-weight: 700; }
    header p { font-size: 0.95em; opacity: 0.9; }
    .main-content { display: grid; grid-template-columns: 1fr 2fr 1fr; gap: 20px; margin-bottom: 20px; }
    .side-panel { display: flex; flex-direction: column; gap: 15px; }
    .card {
      background: rgba(255,255,255,0.1);
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.2);
      padding: 20px; border-radius: 15px;
      box-shadow: 0 8px 32px 0 rgba(31,38,135,0.37);
      transition: all 0.3s ease;
    }
    .card:hover { background: rgba(255,255,255,0.15); transform: translateY(-5px); }
    .card-title { font-size: 0.85em; opacity: 0.8; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 8px; }
    .card-value { font-size: 2em; font-weight: 700; line-height: 1.2; }
    .card-unit { font-size: 0.8em; opacity: 0.7; }
    #map { height: 500px; border-radius: 15px; box-shadow: 0 8px 32px 0 rgba(31,38,135,0.37); border: 1px solid rgba(255,255,255,0.2); }
    .status-fall { padding: 15px; border-radius: 10px; text-align: center; font-weight: 600; }
    .status-normal { background: rgba(16,185,129,0.2); border: 1px solid rgba(16,185,129,0.5); color: #a7f3d0; }
    .status-alert { background: rgba(239,68,68,0.2); border: 1px solid rgba(239,68,68,0.5); color: #fca5a5; animation: pulse 1s infinite; }
    @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.7} }
    .accel-data { display: flex; gap: 8px; justify-content: space-between; }
    .accel-box { flex: 1; text-align: center; padding: 8px 5px; border-radius: 8px; font-family: 'Courier New', monospace; font-size: 0.85em; }
    .accel-x { background: rgba(252,165,165,0.2); border: 1px solid rgba(252,165,165,0.4); color: #fca5a5; }
    .accel-y { background: rgba(167,243,208,0.2); border: 1px solid rgba(167,243,208,0.4); color: #a7f3d0; }
    .accel-z { background: rgba(191,219,254,0.2); border: 1px solid rgba(191,219,254,0.4); color: #bfdbfe; }
    .accel-label { font-size: 0.75em; opacity: 0.7; margin-bottom: 4px; }
    .accel-val { font-weight: 700; font-size: 1em; }
    @media(max-width:1024px){ .main-content{grid-template-columns:1fr} header h1{font-size:1.8em} }
  </style>
  <link rel="stylesheet" href="https://unpkg.com/leaflet/dist/leaflet.css"/>
</head>
<body>
  <div class="container">
    <header>
      <h1>Health Monitor</h1>
      <p>Real-time Health & Location Tracking</p>
    </header>
    <div class="main-content">
      <div class="side-panel">
        <div class="card">
          <div class="card-title">Heart Rate</div>
          <div class="card-value"><span id="hr">--</span><span class="card-unit"> BPM</span></div>
        </div>
        <div class="card">
          <div class="card-title">Blood Oxygen</div>
          <div class="card-value"><span id="spo2">--</span><span class="card-unit">%</span></div>
        </div>
        <div class="card">
          <div class="card-title">Temperature</div>
          <div class="card-value"><span id="temp">--</span><span class="card-unit">°C</span></div>
        </div>
      </div>
      <div>
        <div class="card-title" style="margin-bottom:10px; text-align:center; letter-spacing:1px;">DEVICE LOCATION</div>
        <div id="map"></div>
      </div>
      <div class="side-panel">
        <div class="card status-fall status-normal" id="fallStatus">
          <div class="card-title">Fall Detection</div>
          <div class="card-value" id="fall">NORMAL</div>
        </div>
        <div class="card">
          <div class="card-title">Acceleration</div>
          <div class="accel-data">
            <div class="accel-box accel-x">
              <div class="accel-label">X</div>
              <div class="accel-val"><span id="accX">--</span></div>
            </div>
            <div class="accel-box accel-y">
              <div class="accel-label">Y</div>
              <div class="accel-val"><span id="accY">--</span></div>
            </div>
            <div class="accel-box accel-z">
              <div class="accel-label">Z</div>
              <div class="accel-val"><span id="accZ">--</span></div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
<script src="https://unpkg.com/leaflet/dist/leaflet.js"></script>
<script>
  var map = L.map('map').setView([11.025310,77.002599],15);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);
  var marker = L.marker([11.025310,77.002599]).addTo(map);

  function updateData(){
    fetch('/data').then(res=>res.json()).then(d=>{
      document.getElementById('hr').innerText=d.hr;
      document.getElementById('spo2').innerText=d.spo2;
      document.getElementById('temp').innerText=d.temp.toFixed(1);
      document.getElementById('fall').innerText=d.fall;
      document.getElementById('accX').innerText=d.accX.toFixed(2);
      document.getElementById('accY').innerText=d.accY.toFixed(2);
      document.getElementById('accZ').innerText=d.accZ.toFixed(2);

      var fs=document.getElementById('fallStatus');
      if(d.fall==='FALL'){fs.classList.remove('status-normal');fs.classList.add('status-alert');}
      else{fs.classList.remove('status-alert');fs.classList.add('status-normal');}

      marker.setLatLng([d.lat,d.lon]);
      map.setView([d.lat,d.lon],15);
    });
  }
  setInterval(updateData,1000);
</script>
</body>
</html>
)====";

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("smartband"))
    Serial.println("mDNS failed");

  waClient.setInsecure();
  waClient.setTimeout(10000);

  max30102.begin(Wire, I2C_SPEED_STANDARD);
  max30102.setup();
  max30102.setPulseAmplitudeIR(0x3F);
  max30102.setPulseAmplitudeRed(0x3F);

  mpu.begin();
  mlx.begin();

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  server.on("/", []()
            { server.send(200, "text/html", MAIN_page); });
  server.on("/data", sendData);
  server.begin();
}

// ================= LOOP =================
void loop()
{
  server.handleClient();

  while (gpsSerial.available())
    gps.encode(gpsSerial.read());

  if (gps.location.isValid())
  {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
  }

  long ir = max30102.getIR();
  static unsigned long lastBeat = 0;
  if (ir > 50000 && checkForBeat(ir))
  {
    bpm = 60 * 1000 / (millis() - lastBeat);
    lastBeat = millis();
  }

  spo2 = map(ir, 5000, 60000, 90, 100);
  spo2 = constrain(spo2, 90, 100);

  bodyTemp = mlx.readObjectTempC();

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  accX = a.acceleration.x;
  accY = a.acceleration.y;
  accZ = a.acceleration.z;

  // ================= FALL DETECTION =================
  bool currentlyFallen = (accZ < FALL_Z_THRESHOLD);

  if (currentlyFallen)
  {
    // Device is upside down
    fallDetected = true;

    if (!fallCandidate)
    {
      // Just flipped — start the timer
      fallCandidate = true;
      fallCandidateStart = millis();
      alertSentForThisFall = false;
      Serial.println("Fall candidate started...");
    }

    // If stayed fallen for 3 seconds and alert not yet sent
    if (!alertSentForThisFall && (millis() - fallCandidateStart >= FALL_CONFIRM_MS))
    {
      Serial.println("FALL CONFIRMED — sending WhatsApp alert!");
      unsigned long now = millis();
      if (now - lastWaSentTime > WA_COOLDOWN_MS)
      {
        sendWhatsAppAlert(latitude, longitude);
        lastWaSentTime = now;
      }
      alertSentForThisFall = true; // Don't send again until device recovers and falls again
    }
  }
  else
  {
    // Device is upright — reset everything
    if (fallDetected)
      Serial.println("Device upright — fall cleared.");

    fallDetected = false;
    fallCandidate = false;
    fallCandidateStart = 0;
    alertSentForThisFall = false;
  }
}

// ================= WHATSAPP ALERT =================
void sendWhatsAppAlert(float lat, float lon)
{
  Serial.println("=== sendWhatsAppAlert called ===");
  Serial.print("Lat: "); Serial.println(lat, 6);
  Serial.print("Lon: "); Serial.println(lon, 6);
  Serial.print("Free heap: "); Serial.println(ESP.getFreeHeap());

  Serial.println("Attempting HTTPS connect...");
  int retry = 0;
  while (!waClient.connect(waHost, httpsPort))
  {
    retry++;
    Serial.print("Connection attempt "); Serial.print(retry); Serial.println(" failed");
    if (retry >= 3)
    {
      Serial.println("All retries failed. Aborting.");
      return;
    }
    delay(1000);
  }

  Serial.println("HTTPS Connected!");

  String mapsLink = "https://maps.google.com/?q=" + String(lat, 6) + "," + String(lon, 6);

  String payload =
      "{"
      "\"phone_number\":\"" + String(phoneNumber) + "\","
      "\"template_id\":\"threshold_violation_alert\","
      "\"variables\":{"
      "\"device_name\":\"SmartBand\","
      "\"parameter\":\"Fall Detection\","
      "\"measured_value\":\"Fall Detected\","
      "\"limit\":\"N/A\","
      "\"location\":\"" + mapsLink + "\""
      "}"
      "}";

  Serial.print("Payload length: "); Serial.println(payload.length());
  Serial.println("Payload: " + payload);

  waClient.println("POST /api/v1/whatsapp/send HTTP/1.0");
  waClient.println("Host: www.circuitdigest.cloud");
  waClient.println("Authorization: " + String(apiKey));
  waClient.println("Content-Type: application/json");
  waClient.print("Content-Length: ");
  waClient.println(payload.length());
  waClient.println("Connection: close");
  waClient.println();
  waClient.print(payload);

  Serial.println("Request sent. Waiting for response...");

  unsigned long timeout = millis();
  while (waClient.available() == 0)
  {
    if (millis() - timeout > 15000)
    {
      Serial.println("Timeout waiting for response!");
      waClient.stop();
      return;
    }
  }

  Serial.println("=== API Response ===");
  while (waClient.available())
  {
    String line = waClient.readStringUntil('\n');
    Serial.println(line);
  }

  waClient.stop();
  Serial.println("=== Done ===");
}
// ================= JSON HANDLER =================
void sendData()
{
  StaticJsonDocument<256> doc;
  doc["hr"]   = bpm;
  doc["spo2"] = spo2;
  doc["temp"] = bodyTemp;
  doc["fall"] = fallDetected ? "FALL" : "NORMAL";
  doc["lat"]  = latitude;
  doc["lon"]  = longitude;
  doc["accX"] = accX;
  doc["accY"] = accY;
  doc["accZ"] = accZ;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}