# Health Monitoring System

An ESP32-based real-time health monitoring system with fall detection, GPS tracking, and WhatsApp emergency alerts. The system monitors vital health parameters and provides a responsive web dashboard for real-time visualization.

![ESP32 Health Monitor](https://img.shields.io/badge/Platform-ESP32-blue)
![PlatformIO](https://img.shields.io/badge/Built%20with-PlatformIO-orange)
![License](https://img.shields.io/badge/License-MIT-green)

## Features

### Health Monitoring
- **Heart Rate (BPM)**: Real-time heart rate monitoring using MAX30102 pulse oximeter sensor
- **Blood Oxygen (SpO2)**: Oxygen saturation level measurement (90-100%)
- **Body Temperature**: Non-contact temperature measurement using MLX90614 infrared sensor

### Fall Detection
- **Accelerometer-based Detection**: Uses MPU6050 6-axis gyroscope and accelerometer
- **Smart Alert System**: 3-second confirmation period to prevent false positives
- **Automatic WhatsApp Alerts**: Sends emergency alerts with GPS location when fall is confirmed
- **Visual Status Indicator**: Real-time fall status on web dashboard

### Location Tracking
- **GPS Integration**: Real-time location tracking using TinyGPS Plus
- **Interactive Map**: Live location visualization on OpenStreetMap
- **Emergency Location Sharing**: Automatic location sharing via WhatsApp during fall events

### Web Dashboard
- **Real-time Updates**: Live data refresh every second
- **Responsive Design**: Modern glassmorphism UI with gradient backgrounds
- **Interactive Map**: Leaflet-powered map with live marker updates
- **Multi-panel Layout**: Organized display of all health metrics and sensor data

## Hardware Requirements

### Microcontroller
- **ESP32 Development Board** (esp32dev)

### Sensors
1. **MAX30102/MAX30105**: Pulse oximeter and heart rate sensor
2. **MPU6050**: 6-axis gyroscope and accelerometer (fall detection)
3. **MLX90614**: Non-contact infrared temperature sensor
4. **GPS Module**: Compatible with TinyGPS Plus library (connected to UART2)

### Pin Configuration
```
I2C (Sensors):
- SDA: GPIO 21
- SCL: GPIO 22

GPS (UART2):
- RX: GPIO 16
- TX: GPIO 17
```

## Software Dependencies

### PlatformIO Libraries
All dependencies are automatically managed by PlatformIO:

```ini
- bblanchon/ArduinoJson@^7.4.2
- devxplained/MAX3010x Sensor Library@^1.0.5
- mbed-arturogasca/HeartRate@0.0.0+sha.18957c930b4c
- adafruit/Adafruit MPU6050@^2.2.6
- mikalhart/TinyGPSPlus@^1.1.0
- sparkfun/SparkFun MAX3010x Pulse and Proximity Sensor Library@^1.1.2
- adafruit/Adafruit MLX90614 Library@^2.1.5
```

## Installation & Setup

### 1. Clone the Repository
```bash
git clone https://github.com/bharanidharanrangaraj/Health-Monitoring-System.git
cd Health-Monitoring-System
```

### 2. Configure WiFi and WhatsApp API
Edit the following credentials in `src/main.cpp`:

```cpp
// WiFi Configuration
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// WhatsApp API Configuration
const char *apiKey = "YOUR_API_KEY";
const char *phoneNumber = "YOUR_PHONE_NUMBER"; // Format: 919486229298
```
### 3. Build and Upload

#### Using PlatformIO CLI:
```bash
pio run --target upload
```

#### Using PlatformIO IDE (VS Code):
1. Open the project folder in VS Code
2. Click the PlatformIO icon in the sidebar
3. Select "Upload" from the project tasks

### 4. Monitor Serial Output
```bash
pio device monitor
```

## Usage

### Accessing the Web Dashboard

1. **Connect to WiFi**: After uploading, the ESP32 will connect to your WiFi network
2. **Find the IP Address**: Check the serial monitor for the ESP32's IP address
3. **Open Dashboard**: Navigate to `http://<ESP32_IP_ADDRESS>` in your web browser
4. **mDNS Access** (optional): Use `http://smartband.local` if your network supports mDNS

### Dashboard Features

The web interface displays:
- **Left Panel**: Heart rate, SpO2, and body temperature
- **Center Panel**: Interactive map with real-time GPS location
- **Right Panel**: Fall detection status and 3-axis accelerometer data (X, Y, Z)

### Fall Detection

The system uses accelerometer Z-axis monitoring:
- **Detection Threshold**: Z-axis < 0.0 (device upside down)
- **Confirmation Period**: 3 seconds of sustained fall position
- **Alert Cooldown**: 60 seconds between WhatsApp alerts
- **Automatic Reset**: Clears when device returns to normal orientation

### Emergency Alerts

When a fall is confirmed:
1. Fall status changes to "FALL" on the dashboard (red alert with pulse animation)
2. WhatsApp message is sent with:
   - Device name: "SmartBand"
   - Event: "Fall Detection"
   - Status: "Fall Detected"
   - Google Maps link with current GPS coordinates

## API Endpoints

- `GET /` - Main dashboard HTML page
- `GET /data` - JSON endpoint for sensor data

### Data Response Format
```json
{
  "hr": 75,
  "spo2": 98,
  "temp": 36.5,
  "fall": "NORMAL",
  "lat": 11.025310,
  "lon": 77.002599,
  "accX": 0.12,
  "accY": -0.05,
  "accZ": 9.81
}
```

## Configuration Parameters

### Fall Detection
```cpp
#define FALL_Z_THRESHOLD 0.0      // Z-axis threshold for fall detection
#define FALL_CONFIRM_MS  3000     // Confirmation time in milliseconds
```

### WhatsApp Alerts
```cpp
#define WA_COOLDOWN_MS 60000      // Cooldown between alerts (60 seconds)
```

## Troubleshooting

### WiFi Connection Issues
- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure 2.4GHz WiFi is enabled (ESP32 doesn't support 5GHz)

### Sensor Issues
- Verify I2C connections (SDA: GPIO21, SCL: GPIO22)
- Check sensor power supply (3.3V)
- Monitor serial output for sensor initialization errors

### GPS Not Working
- Ensure clear view of the sky for GPS signal
- Verify GPS UART connections (RX: GPIO16, TX: GPIO17)
- Check GPS baud rate (9600)

### WhatsApp Alerts Not Sending
- Verify API key and phone number format
- Check internet connectivity
- Monitor serial output for HTTPS connection status
- Ensure CircuitDigest cloud service is accessible

## Project Structure

```
Health-Monitoring-System/
├── src/
│   └── main.cpp              # Main application code
├── include/
│   └── README                # Include directory info
├── lib/
│   └── README                # Library directory info
├── test/
│   └── README                # Test directory info
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## Technologies Used

- **Framework**: Arduino (ESP32)
- **Build System**: PlatformIO
- **Web Server**: ESP32 WebServer
- **Protocol**: HTTPS (WhatsApp API), HTTP (Dashboard)
- **Data Format**: JSON (ArduinoJson)
- **Frontend**: HTML5, CSS3, JavaScript
- **Mapping**: Leaflet.js + OpenStreetMap
- **Design**: Glassmorphism UI with gradient backgrounds

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## License

This project is open source and available under the [MIT License](LICENSE).

## Acknowledgments

- CircuitDigest Cloud for WhatsApp API service
- Adafruit, SparkFun, and other library authors
- OpenStreetMap and Leaflet.js communities

---

**Note**: This device is intended for educational and monitoring purposes only. It is not a certified medical device and should not be used as a substitute for professional medical advice, diagnosis, or treatment.
