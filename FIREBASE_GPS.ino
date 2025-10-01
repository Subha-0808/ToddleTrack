#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// Wi-Fi credentials
const char* WIFI_SSID = "Wifi has left the chat";
const char* WIFI_PASSWORD = "jaguar08";

// Firebase Realtime Database URL
String FIREBASE_URL = "https://toddletrack-fd848-default-rtdb.asia-southeast1.firebasedatabase.app/sensor.json";

// GPS configuration
const int GPS_BAUDRATE = 115200;
const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;

// Motion detection threshold (adjust as needed)
const float MOTION_THRESHOLD = 1.5; // in m/s^2

// Cooldown variables
unsigned long last_sent_time = 0;
const unsigned long COOLDOWN_INTERVAL = 10000; // 10 seconds in milliseconds

// Function to send data to Firebase
void putData(String jsonPayload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(FIREBASE_URL);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.PUT(jsonPayload);

    Serial.print("Firebase response: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      Serial.println("Data sent successfully!");
    } else {
      Serial.println("Error sending data to Firebase.");
    }
    http.end();
  } else {
    Serial.println("Wi-Fi not connected");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip!");
    while (1) { delay(10); }
  }
  Serial.println("MPU6050 connected!");

  gpsSerial.begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS module initialized!");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWi-Fi connected!");
}

void loop() {
  // Always check for new GPS data in a non-blocking manner
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Read MPU6050 data
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Check for motion, a valid GPS fix, and if the cooldown period has passed
  if ((abs(a.acceleration.x) > MOTION_THRESHOLD ||
       abs(a.acceleration.y) > MOTION_THRESHOLD ||
       abs(a.acceleration.z) > MOTION_THRESHOLD) &&
      gps.location.isValid() &&
      (millis() - last_sent_time > COOLDOWN_INTERVAL)) {

    Serial.println("--------------------");
    Serial.println("MOTION DETECTED!");
    
    // Print MPU6050 accelerometer data
    Serial.print("Accel X: ");
    Serial.print(a.acceleration.x);
    Serial.print(" m/s^2, Y: ");
    Serial.print(a.acceleration.y);
    Serial.print(" m/s^2, Z: ");
    Serial.print(a.acceleration.z);
    Serial.println(" m/s^2");

    // Print GPS location data
    Serial.print("Location -> Latitude: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(", Longitude: ");
    Serial.println(gps.location.lng(), 6);
    
    Serial.println("--------------------");
    
    // JSON payload with both sensor and GPS data
    String jsonPayload = "{";
    jsonPayload += "\"ax\":" + String(a.acceleration.x) + ",";
    jsonPayload += "\"ay\":" + String(a.acceleration.y) + ",";
    jsonPayload += "\"az\":" + String(a.acceleration.z) + ",";
    jsonPayload += "\"latitude\":" + String(gps.location.lat(), 6) + ",";
    jsonPayload += "\"longitude\":" + String(gps.location.lng(), 6);
    jsonPayload += "}";

    putData(jsonPayload);

    // Update the last sent time
    last_sent_time = millis();
  }

  delay(100); // Short delay to prevent flooding the loop
}