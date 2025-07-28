#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>

// Firebase Credentials (Replace with your credentials in a separate secrets.h file for real use)
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "https://your-project-id.firebaseio.com"
#define USER_EMAIL "your-email@example.com"
#define USER_PASSWORD "your-password"

// WiFi Credentials (Replace with your credentials in a separate secrets.h file for real use)
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

// Objects
DHT dht(DHTPIN, DHTTYPE);
FirebaseData fbdo;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

// Firebase init flag
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

  // Initial States
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(RELAY_FAN_PIN, HIGH);  // Fan OFF

  // Connect to WiFi
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
    Serial.println("🔄 Attempting to reconnect WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(5000); // Allow time for reconnection

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
    Serial.println("❌ WiFi not connected, data not sent");
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
    Serial.println("❌ Failed to read from DHT sensor!");
    delay(5000);
    return;
  }

  // Log values
  Serial.printf("🌡 Temp: %.2f °C\n", temp);
  Serial.printf("💧 Hum: %.2f %%\n", hum);
  Serial.printf("🔥 Flame: %s\n", flameDetected ? "YES" : "NO");
  Serial.printf("🌫 Gas: %d\n", gasValue);

  // 1. Send data to Firebase
  sendToFirebase(temp, hum, flameDetected, gasValue);

  // 2. Conditions for alerts
  if (flameDetected || gasValue > 1500) {
    Serial.println("⚠ ALERT: Flame or Gas Detected");
    digitalWrite(RELAY_FAN_PIN, LOW);  // Turn fan ON
    digitalWrite(GREEN_PIN, LOW);
    alertBlink(5000);
  } else {
    Serial.println("✅ Normal - No Flame/Gas Detected");
    digitalWrite(RELAY_FAN_PIN, HIGH);  // Fan OFF
    digitalWrite(GREEN_PIN, HIGH);      // Green ON
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RED_PIN, LOW);
  }

  delay(2000); // Wait before next loop
}
