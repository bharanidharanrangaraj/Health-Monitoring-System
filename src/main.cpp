#include <Arduino.h>

#include <WiFi.h>
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

// ================= DATA =================
int bpm = 0;
int spo2 = 0;
float bodyTemp = 0;
bool fallDetected = false;
float latitude = 0.0;
float longitude = 0.0;
float accX = 0.0;
float accY = 0.0;
float accZ = 0.0;

// ================= FALL =================
boolean fall = false;     // stores if a fall has occurred
boolean trigger1 = false; // stores if first trigger (lower threshold) has occurred
boolean trigger2 = false; // stores if second trigger (upper threshold) has occurred
boolean trigger3 = false; // stores if third trigger (orientation change) has occurred
byte trigger1count = 0;   // stores the counts past since trigger 1 was set true
byte trigger2count = 0;   // stores the counts past since trigger 2 was set true
byte trigger3count = 0;   // stores the counts past since trigger 3 was set true
int angleChange = 0;

// ================= FUNCTION DECLARATION =================
void sendData();

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
    .container {
      max-width: 1200px;
      margin: 0 auto;
    }
    header {
      text-align: center;
      margin-bottom: 30px;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
    }
    header h1 {
      font-size: 2.5em;
      margin-bottom: 5px;
      font-weight: 700;
    }
    header p {
      font-size: 0.95em;
      opacity: 0.9;
    }
    .main-content {
      display: grid;
      grid-template-columns: 1fr 2fr 1fr;
      gap: 20px;
      margin-bottom: 20px;
    }
    .side-panel {
      display: flex;
      flex-direction: column;
      gap: 15px;
    }
    .card {
      background: rgba(255, 255, 255, 0.1);
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255, 255, 255, 0.2);
      padding: 20px;
      border-radius: 15px;
      box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);
      transition: all 0.3s ease;
    }
    .card:hover {
      background: rgba(255, 255, 255, 0.15);
      transform: translateY(-5px);
      box-shadow: 0 12px 40px 0 rgba(31, 38, 135, 0.5);
    }
    .card-title {
      font-size: 0.85em;
      opacity: 0.8;
      text-transform: uppercase;
      letter-spacing: 1px;
      margin-bottom: 8px;
    }
    .card-value {
      font-size: 2em;
      font-weight: 700;
      line-height: 1.2;
    }
    .card-unit {
      font-size: 0.8em;
      opacity: 0.7;
    }
    #map {
      height: 500px;
      border-radius: 15px;
      box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);
      border: 1px solid rgba(255, 255, 255, 0.2);
    }
    .status-fall {
      padding: 15px;
      border-radius: 10px;
      text-align: center;
      font-weight: 600;
    }
    .status-normal {
      background: rgba(16, 185, 129, 0.2);
      border: 1px solid rgba(16, 185, 129, 0.5);
      color: #a7f3d0;
    }
    .status-alert {
      background: rgba(239, 68, 68, 0.2);
      border: 1px solid rgba(239, 68, 68, 0.5);
      color: #fca5a5;
      animation: pulse 1s infinite;
    }
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.7; }
    }
    .accel-data {
      font-size: 0.85em;
      line-height: 1.6;
      font-family: 'Courier New', monospace;
    }
    .accel-data span {
      display: block;
      margin: 5px 0;
    }
    .accel-x { color: #fca5a5; }
    .accel-y { color: #a7f3d0; }
    .accel-z { color: #bfdbfe; }
    @media (max-width: 1024px) {
      .main-content {
        grid-template-columns: 1fr;
      }
      header h1 {
        font-size: 1.8em;
      }
    }
  </style>
  <link rel="stylesheet" href="https://unpkg.com/leaflet/dist/leaflet.css"/>
</head>
<body>
  <div class="container">
    <header>
      <h1>❤️ Health Monitor</h1>
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
        <div id="map"></div>
        <div class="card" style="margin-top: 15px;">
          <div class="card-title">GPS Coordinates</div>
          <div style="font-size: 0.9em; font-family: 'Courier New', monospace;">
            <div>Lat: <span id="latitude">0.000000</span></div>
            <div>Lon: <span id="longitude">0.000000</span></div>
          </div>
        </div>
      </div>

      <div class="side-panel">
        <div class="card status-fall status-normal" id="fallStatus">
          <div class="card-title">Fall Detection</div>
          <div class="card-value" id="fall">NORMAL</div>
        </div>
        <div class="card">
          <div class="card-title">Acceleration</div>
          <div class="accel-data">
            <span class="accel-x">X: <span id="accX">--</span></span>
            <span class="accel-y">Y: <span id="accY">--</span></span>
            <span class="accel-z">Z: <span id="accZ">--</span></span>
          </div>
        </div>
      </div>
    </div>
  </div>

<script src="https://unpkg.com/leaflet/dist/leaflet.js"></script>
<script>
  var map = L.map('map').setView([11.025310, 77.002599], 15);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);
  var marker = L.marker([11.025310, 77.002599]).addTo(map);

  function updateData(){
    fetch('/data').then(res=>res.json()).then(d=>{
      document.getElementById('hr').innerText = d.hr;
      document.getElementById('spo2').innerText = d.spo2;
      document.getElementById('temp').innerText = d.temp.toFixed(1);
      document.getElementById('fall').innerText = d.fall;
      document.getElementById('accX').innerText = d.accX.toFixed(2);
      document.getElementById('accY').innerText = d.accY.toFixed(2);
      document.getElementById('accZ').innerText = d.accZ.toFixed(2);
      document.getElementById('latitude').innerText = d.lat.toFixed(6);
      document.getElementById('longitude').innerText = d.lon.toFixed(6);
      
      var fallStatus = document.getElementById('fallStatus');
      if(d.fall === 'FALL'){
        fallStatus.classList.remove('status-normal');
        fallStatus.classList.add('status-alert');
      } else {
        fallStatus.classList.remove('status-alert');
        fallStatus.classList.add('status-normal');
      }

      if(d.lat != 0 && d.lon != 0){
        marker.setLatLng([d.lat, d.lon]);
        map.setView([d.lat, d.lon], 15);
      }
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

  // ---------- mDNS ----------
  if (!MDNS.begin("smartband"))
  {
    Serial.println("mDNS failed");
  }

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
  // Convert to g and deg/s
  float ax = a.acceleration.x / 9.81;
  float ay = a.acceleration.y / 9.81;
  float az = a.acceleration.z / 9.81;
  float gx = g.gyro.x * 180.0 / PI;
  float gy = g.gyro.y * 180.0 / PI;
  float gz = g.gyro.z * 180.0 / PI;
  // Calculating Amplitude vector for 3 axis
  float Raw_Amp = sqrt(ax * ax + ay * ay + az * az);
  int Amp = Raw_Amp * 10; // Multiplied by 10 because values are between 0 to 1
  if (Amp <= 5 && trigger2 == false)
  { // if AM breaks lower threshold (0.5g) - lowered for easier flip detection
    trigger1 = true;
  }
  if (trigger1 == true)
  {
    trigger1count++;
    if (Amp >= 8)
    { // if AM breaks upper threshold (0.8g) - lowered for easier flip detection
      trigger2 = true;
      trigger1 = false;
      trigger1count = 0;
      trigger2 = true;
      trigger1 = false;
      trigger1count = 0;
    }
  }
  if (trigger2 == true)
  {
    trigger2count++;
    angleChange = sqrt(gx * gx + gy * gy + gz * gz);
    if (angleChange >= 30 && angleChange <= 400)
    { // if orientation changes by between 30-400 deg/s
      trigger3 = true;
      trigger2 = false;
      trigger2count = 0;
    }
  }
  if (trigger3 == true)
  {
    trigger3count++;
    if (trigger3count >= 10)
    {
      angleChange = sqrt(gx * gx + gy * gy + gz * gz);
      if ((angleChange >= 0) && (angleChange <= 10))
      { // if orientation changes remains between 0-10 deg/s
        fall = true;
        trigger3 = false;
        trigger3count = 0;
      }
      else
      { // user regained normal orientation
        trigger3 = false;
        trigger3count = 0;
      }
    }
  }
  static unsigned long fallTime = 0;
  if (fall == true)
  { // in event of a fall detection
    fallDetected = true;
    fall = false;
    fallTime = millis();
    Serial.println("FALL DETECTED!!!");
  }
  if (fallDetected && millis() - fallTime > 30000)
  {
    fallDetected = false;
    trigger1 = false;
    trigger2 = false;
    trigger3 = false;
    trigger1count = 0;
    trigger2count = 0;
    trigger3count = 0;
  }
  if (trigger2count >= 6)
  { // allow 0.5s for orientation change
    trigger2 = false;
    trigger2count = 0;
  }
  if (trigger1count >= 6)
  { // allow 0.5s for AM to break upper threshold
    trigger1 = false;
    trigger1count = 0;
  }
}

// ================= JSON HANDLER =================
void sendData()
{
  StaticJsonDocument<256> doc;
  doc["hr"] = bpm;
  doc["spo2"] = spo2;
  doc["temp"] = bodyTemp;
  doc["fall"] = fallDetected ? "FALL" : "NORMAL";
  doc["lat"] = latitude;
  doc["lon"] = longitude;
  doc["accX"] = accX;
  doc["accY"] = accY;
  doc["accZ"] = accZ;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}