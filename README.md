# Industrial Safety Monitoring System using ESP32, Raspberry Pi & Firebase

This project presents a real-time Industrial Safety Monitoring System that uses multiple sensors to detect hazardous environmental conditions. It utilizes an ESP32 microcontroller for sensor interfacing, Firebase for cloud storage, a Raspberry Pi for local ML-based decision-making, and alert systems like SMS, voice feedback, and visual indicators.

---

## 🔧 Features

- Monitors Temperature, Humidity, Gas, and Flame
- Real-time data logging to Firebase
- ML model running on Raspberry Pi classifies environment as SAFE or DANGER
- OLED Display for live status
- SMS alert via CircuitDigest API
- Voice alert for danger
- Automatic fan activation for ventilation
- Power/WiFi status indicators

  ---

## 🔧 Components Used

| Component       | Description                                             |
|----------------|---------------------------------------------------------|
| ESP32           | Microcontroller for interfacing with sensors and Firebase |
| Raspberry Pi    | Runs ML model, handles local logging and decision logic |
| DHT11 Sensor    | Measures temperature and humidity                       |
| MQ2 Sensor      | Detects gas/smoke levels                                |
| Flame Sensor    | Detects flame or fire                                   |
| Green LED       | Indicates safe environment                              |
| Red LED         | Indicates dangerous environment                         |
| Relay Module    | Controls fan (acts as exhaust system)                   |
| Fan (Exhaust)   | Activated during high gas levels                        |
| OLED Display    | Displays sensor values and system status                |

---

## ⚙️ How to Run

### 🧠 ESP32 Setup

1. Open `ESP32_Code/esp32_monitoring.ino` in Arduino IDE or PlatformIO.
2. Configure your:
   - Wi-Fi credentials
   - Firebase credentials
3. Connect:
   - DHT11 to GPIO 4
   - MQ2 to GPIO 34 (Analog)
   - Flame sensor to GPIO 14
   - Buzzer, LEDs, Fan, Relay as per diagram
4. Upload the code and open Serial Monitor for logs.

<details>
<summary>📄 <strong>ESP32 Code</strong> (click to expand)</summary>

```cpp
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>

// Firebase Credentials
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "https://your-project-id.firebaseio.com"
#define USER_EMAIL "your-email@example.com"
#define USER_PASSWORD "your-password"

// WiFi Credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Sensor Pins
#define DHTPIN 4
#define DHTTYPE DHT11
#define FLAME_SENSOR_PIN 14
#define MQ2_SENSOR_PIN 34

// Output Pins
#define BUZZER_PIN 12
#define RED_PIN 25
#define GREEN_PIN 26
#define BLUE_PIN 27
#define RELAY_FAN_PIN 33

DHT dht(DHTPIN, DHTTYPE);
FirebaseData fbdo;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;
bool firebaseInitialized = false;

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(MQ2_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(RELAY_FAN_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(RELAY_FAN_PIN, HIGH);

  Serial.print("🔌 Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    digitalWrite(BLUE_PIN, HIGH);
    delay(300);
    digitalWrite(BLUE_PIN, LOW);
    delay(300);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected");
    digitalWrite(BLUE_PIN, HIGH);
    firebaseConfig.api_key = API_KEY;
    firebaseConfig.database_url = DATABASE_URL;
    firebaseAuth.user.email = USER_EMAIL;
    firebaseAuth.user.password = USER_PASSWORD;
    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Firebase.reconnectWiFi(true);
    firebaseInitialized = true;
  } else {
    Serial.println("\n❌ WiFi Connection Failed");
    digitalWrite(BLUE_PIN, LOW);
    firebaseInitialized = false;
  }
}

void checkWiFiAndFirebase() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🔄 Reconnecting WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(5000);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("✅ WiFi Reconnected");
      digitalWrite(BLUE_PIN, HIGH);
      if (!firebaseInitialized) {
        firebaseConfig.api_key = API_KEY;
        firebaseConfig.database_url = DATABASE_URL;
        firebaseAuth.user.email = USER_EMAIL;
        firebaseAuth.user.password = USER_PASSWORD;
        Firebase.begin(&firebaseConfig, &firebaseAuth);
        Firebase.reconnectWiFi(true);
        firebaseInitialized = true;
        Serial.println("✅ Firebase Initialized");
      }
    } else {
      Serial.println("❌ WiFi still not connected");
      digitalWrite(BLUE_PIN, LOW);
      firebaseInitialized = false;
    }
  }
}

void sendToFirebase(float temp, float hum, bool flame, int gas) {
  if (WiFi.status() == WL_CONNECTED) {
    Firebase.setFloat(fbdo, "/SensorData/temperature", temp);
    Firebase.setFloat(fbdo, "/SensorData/humidity", hum);
    Firebase.setBool(fbdo, "/SensorData/flame", flame);
    Firebase.setInt(fbdo, "/SensorData/gas", gas);
    Serial.println("📡 Data sent to Firebase");
  } else {
    Serial.println("❌ WiFi not connected");
  }
}

void alertBlink(int durationMillis) {
  unsigned long start = millis();
  while (millis() - start < durationMillis) {
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(250);
    digitalWrite(RED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(250);
  }
}

void loop() {
  checkWiFiAndFirebase();
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  bool flameDetected = digitalRead(FLAME_SENSOR_PIN) == LOW;
  int gasValue = analogRead(MQ2_SENSOR_PIN);

  if (isnan(temp) || isnan(hum)) {
    Serial.println("❌ DHT Read Failed!");
    delay(5000);
    return;
  }

  Serial.printf("🌡 Temp: %.2f °C\n", temp);
  Serial.printf("💧 Hum: %.2f %%\n", hum);
  Serial.printf("🔥 Flame: %s\n", flameDetected ? "YES" : "NO");
  Serial.printf("🌫 Gas: %d\n", gasValue);

  sendToFirebase(temp, hum, flameDetected, gasValue);

  if (flameDetected || gasValue > 1500) {
    Serial.println("⚠ ALERT: Flame or Gas Detected");
    digitalWrite(RELAY_FAN_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    alertBlink(5000);
  } else {
    Serial.println("✅ Safe");
    digitalWrite(RELAY_FAN_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RED_PIN, LOW);
  }

  delay(2000);
}
```
---
## 📦 Dependencies (Install These on Raspberry Pi)

Run the following commands on your Raspberry Pi terminal:

```bash
sudo apt update
sudo apt install python3-pip espeak -y
```
## Install required Python libraries
```bash
pip3 install firebase-admin pandas joblib requests pillow adafruit-circuitpython-ssd1306
```
