
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

// --- Pin Definitions ---
const int trigPin = 5;
const int echoPin = 18;
const int pumpPin = 4;
const int waterUsePin = 6;

// --- Network & MQTT Settings ---
const char *ssid = "Wokwi-GUEST";
const char *password = "";

// IMPORTANT: Change this to your Arch Linux machine's local LAN IP (e.g.,
// 192.168.x.x)
const char *mqtt_server = "192.168.8.199";
const int mqtt_port = 1883;
const char *mqtt_topic = "sensors/group05/watertank/data";

WiFiClient espClient;
PubSubClient client(espClient);

long duration;
float distance;
int isUsingWater;

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

    // --- Create JSON Payload ---
    StaticJsonDocument<200> doc;
    doc["water_level_cm"] = distance;
    doc["status"] = (isUsingWater == HIGH) ? "using_water" : "idle";

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);

    // --- Publish to Broker ---
    Serial.print("Publishing message: ");
    Serial.println(jsonBuffer);
    client.publish(mqtt_topic, jsonBuffer);
  }
}
