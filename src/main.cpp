
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "include/secrets.h" // Include your WiFi and MQTT credentials

// --- Pin Definitions ---
const int trigPin = 5;
const int echoPin = 18;
const int pumpPin = 4;
const int waterUsePin = 6;

// --- Tank Configuration ---
const float tank_area_cm2 = 100.0; // Customize: Cross-sectional area in cm²
const float lower_bound_cm = 160.0; // Pump ON when distance reaches this low-water threshold
const float upper_bound_cm = 100.0; // Pump OFF when distance reaches this high-water threshold

// Warning thresholds
const float leak_threshold_cm_per_s = -5.0; // Sudden drop indicating leak
const float overflow_risk_distance_cm = 20.0; // Low distance means tank is full

// --- Network & MQTT Settings ---
// Using credentials from secrets.h
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *mqtt_server = MQTT_SERVER_IP;
const int mqtt_port = MQTT_SERVER_PORT;
const char *mqtt_topic = "sensors/group05/watertank/data";

WiFiClient espClient;
PubSubClient client(espClient);

long duration;
float distance;
int isUsingWater;
bool pumpOn = false;

// Previous measurements for rate calculation
// We initialize prev_distance to 0.0 and prev_time to 0 to handle the first loop iteration gracefully.
// On the first loop, since prev_time is 0, we will skip rate calculation and just set prev_distance and prev_time to the current values.
float prev_distance = 0.0;
unsigned long prev_time = 0;

// Timer to avoid flooding the MQTT broker
unsigned long lastMsg = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32S3Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect (no username/password needed for our local config)
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(waterUsePin, INPUT);

  // Start with the pump off
  digitalWrite(pumpPin, LOW);

  // Initialize Network
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Keep MQTT connection alive

  unsigned long now = millis();
  // Publish data every 2 seconds
  if (now - lastMsg > 2000) {
    lastMsg = now;

    // --- Sensor Reading Logic ---
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;
    isUsingWater = digitalRead(waterUsePin);

    // --- Pump control based on level bounds ---
    if (distance >= lower_bound_cm) {
      pumpOn = true;
    } else if (distance <= upper_bound_cm) {
      pumpOn = false;
    }
    digitalWrite(pumpPin, pumpOn ? HIGH : LOW);

    // --- Calculate Water Rate ---
    float water_rate_cm_per_s = 0.0;
    if (prev_time != 0) {
      float delta_distance = prev_distance - distance; // Negative when water level drops
      float delta_time_s = (now - prev_time) / 1000.0;
      water_rate_cm_per_s = delta_distance / delta_time_s;
    }
    prev_distance = distance;
    prev_time = now;

    // --- Calculate Volume Rate ---
    float volume_rate_cm3_per_s = tank_area_cm2 * water_rate_cm_per_s;

    // --- Determine Warning ---
    String warning = "none";
    if (water_rate_cm_per_s < leak_threshold_cm_per_s) {
      warning = "leak_detected";
    } else if (distance <= overflow_risk_distance_cm) {
      warning = "overflow_risk";
    }

    // --- Create JSON Payload ---
    StaticJsonDocument<200> doc;
    doc["water_level_cm"] = distance;
    doc["water_rate_Lpm"] = volume_rate_cm3_per_s * 0.001 * 60; // Convert cm³/s to L/min
    doc["usage_status"] = (isUsingWater == HIGH) ? "active" : "idle";
    doc["pump_status"] = pumpOn ? "on" : "off";
    doc["warning"] = warning;

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);

    // --- Publish to Broker ---
    Serial.print("Publishing message: ");
    Serial.println(jsonBuffer);
    Serial.print("Pump state: ");
    Serial.println(pumpOn ? "ON" : "OFF");
    Serial.print("Water rate: ");
    Serial.print(volume_rate_cm3_per_s * 0.001 * 60);
    Serial.println(" L/min");
    Serial.print("Warning: ");
    Serial.println(warning);
    client.publish(mqtt_topic, jsonBuffer);
  }
}
