
#include <Arduino.h>

void setup() {
  // Initialize serial communication at 115200 baud
  Serial.begin(115200);
  
  // Wait a moment for the serial connection to establish (important for S3 native USB)
  delay(1000); 
  
  Serial.println("ESP32-S3 is awake!");
}

void loop() {
  Serial.println("Hello from PlatformIO!");
  delay(2000);
}
