# Rule-Based System Implementation Report
## ESP32 Water Tank Management System

**Project:** Computer Systems Project - Wokwi ESP32 Water Tank Simulation  
**Team:** Group 05  
**Date:** April 2026  
**Objective:** Implement intelligent water tank level monitoring and automated pump control using a rule-based decision system.

---

## 1. Executive Summary

A comprehensive rule-based system has been implemented on an ESP32 microcontroller to manage an automated water tank system. The system monitors water levels, calculates flow rates, detects anomalies (leaks/overflow risks), and automatically controls a water pump based on predefined rules and thresholds.

---

## 2. System Architecture

### 2.1 Hardware Components
- **ESP32-S3 Microcontroller**: Main processing unit
- **Ultrasonic Sensor**: Measures water depth (pins 5 & 18)
- **Water Pump**: Controlled via GPIO pin 4
- **Water Usage Detector**: Monitors active usage (pin 6)
- **MQTT Broker**: Receives sensor data and system state
- **WiFi Network**: Enables remote monitoring and control

### 2.2 Communication
- Protocol: MQTT over WiFi
- Broker IP: `MQTT_SERVER_IP` (from secrets.h)
- Topic: `sensors/group05/watertank/data`
- Update Frequency: 2 seconds

---

## 3. Rule-Based Logic Implementation

### 3.1 Core Rules

#### **Rule 1: Pump Control Based on Water Level**
The system maintains water level within defined bounds using a hysteresis mechanism.

| Rule | Condition | Action |
|------|-----------|--------|
| Rule 1a | `distance >= lower_bound_cm (160 cm)` | Turn Pump **ON** |
| Rule 1b | `distance <= upper_bound_cm (100 cm)` | Turn Pump **OFF** |
| Rule 1c | `upper_bound_cm < distance < lower_bound_cm` | Maintain Previous State (Hysteresis) |

**Purpose:** Prevent rapid pump cycling and maintain optimal tank levels  
**Hysteresis Gap:** 60 cm prevents oscillation between on/off states

**Implementation:**
```cpp
if (distance >= lower_bound_cm) {
  pumpOn = true;
} else if (distance <= upper_bound_cm) {
  pumpOn = false;
}
digitalWrite(pumpPin, pumpOn ? HIGH : LOW);
```

---

#### **Rule 2: Leak Detection**
System monitors water flow rate to detect potential leaks.

| Rule | Condition | Action |
|------|-----------|--------|
| Rule 2 | `water_rate_cm_per_s < -5.0 cm/s` | Set Warning = **"leak_detected"** |

**Purpose:** Alert operators to sudden water loss  
**Threshold:** 5.0 cm/s (rate of water level dropping)  
**Calculation Method:** `delta_distance / delta_time_s`

**Logic:**
```cpp
if (water_rate_cm_per_s < leak_threshold_cm_per_s) {
  warning = "leak_detected";
}
```

---

#### **Rule 3: Overflow Risk Detection**
System warns when tank approaches capacity.

| Rule | Condition | Action |
|------|-----------|--------|
| Rule 3 | `distance <= 20.0 cm` | Set Warning = **"overflow_risk"** |

**Purpose:** Prevent tank overflow  
**Threshold:** 20 cm (sensor distance indicates tank nearly full)

**Logic:**
```cpp
else if (distance <= overflow_risk_distance_cm) {
  warning = "overflow_risk";
}
```

---

### 3.2 Data Processing Rules

#### **Rule 4: Water Rate Calculation**
Convert distance changes into flow rate metrics.

**Formula:**
```
water_rate_cm_per_s = (prev_distance - distance) / time_elapsed_seconds
volume_rate_cm3_per_s = tank_area_cm2 * water_rate_cm_per_s
volume_rate_Lpm = volume_rate_cm3_per_s * 0.001 * 60
```

**Purpose:** Track water consumption and flow dynamics  
**Parameters:**
- Tank cross-sectional area: 100 cm²
- Conversion factor: 0.001 cm³ to liters, 60 seconds to minutes

---

#### **Rule 5: Water Level Translation**
Convert sensor distance to actual water depth.

**Formula:**
```
actual_water_level_cm = bottom_bound_cm - distance
```

**Purpose:** Account for sensor reference point  
**Reference:** `bottom_bound_cm = 200.0 cm` (distance when tank is empty)

---

#### **Rule 6: Usage Status Detection**
Monitor whether water is actively being used.

| Rule | Condition | Status |
|------|-----------|--------|
| Rule 6a | `waterUsePin == HIGH` | "active" |
| Rule 6b | `waterUsePin == LOW` | "idle" |

---

## 4. Configuration Parameters

### 4.1 Tank Parameters
```cpp
const float tank_area_cm2 = 100.0;        // Cross-sectional area
const float bottom_bound_cm = 200.0;      // Empty tank reference
```

### 4.2 Control Thresholds
```cpp
const float lower_bound_cm = 160.0;       // Pump activation threshold
const float upper_bound_cm = 100.0;       // Pump deactivation threshold
```

### 4.3 Warning Thresholds
```cpp
const float leak_threshold_cm_per_s = -5.0;           // Leak detection sensitivity
const float overflow_risk_distance_cm = 20.0;         // Overflow prevention margin
```

### 4.4 Timing Parameters
```cpp
const unsigned long publish_interval = 2000;          // ms (2 seconds)
```

---

## 5. MQTT Data Payload Structure

### 5.1 JSON Output Format
```json
{
  "water_level_cm": 85.5,
  "water_rate_Lpm": 2.4,
  "usage_status": "active",
  "pump_status": "on",
  "warning": "none"
}
```

### 5.2 Field Descriptions
| Field | Unit | Description |
|-------|------|-------------|
| `water_level_cm` | cm | Actual water depth in tank |
| `water_rate_Lpm` | L/min | Current flow rate (positive = filling, negative = draining) |
| `usage_status` | state | "active" if water is being consumed, "idle" otherwise |
| `pump_status` | state | "on" if pump is running, "off" if stopped |
| `warning` | alert | "none", "leak_detected", or "overflow_risk" |

---

## 6. Decision Flow Diagram

```
┌─────────────────────────────────┐
│  Read Sensor Data               │
│  - Distance (ultrasonic)        │
│  - Water Usage (digital pin)    │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│  Apply Rule 1: Pump Control     │
│  Check: distance vs bounds      │
│  Output: pumpOn (true/false)    │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│  Apply Rules 4-5: Rate & Level  │
│  Calculate: water rates, depth  │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│  Apply Rules 2-3: Anomalies     │
│  Check: leak & overflow risk    │
│  Output: warning state          │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│  Format JSON Payload            │
│  Publish to MQTT Broker         │
│  Log to Serial Console          │
└─────────────────────────────────┘
```

---

## 7. State Machine for Pump Control

```
                    distance >= 160 cm
                    ──────────────────>
                         ┌─────────┐
             ┌──────────>│ Pump ON │<──────────┐
             │           └─────────┘           │
             │               ▲                 │
             │               │                 │
             │        maintain state           │
             │     (160 > distance > 100)      │
             │               │                 │
             │               ▼                 │
             │           ┌─────────┐           │
             └──────────│Pump OFF │───────────>
                        └─────────┘
                    distance <= 100 cm
```

---

## 8. Exception Handling

### 8.1 First Loop Handling
- On first sensor read, `prev_time == 0`
- Water rate calculation is skipped
- System initializes: `prev_distance` and `prev_time` to current values
- Next loop calculates rate normally

**Code:**
```cpp
if (prev_time != 0) {
  // Calculate water_rate_cm_per_s
}
```

### 8.2 Sensor Glitches
- Rate-of-change algorithm filters sudden spikes
- Hysteresis in pump control prevents oscillation

---

## 9. Testing Scenarios

### Scenario 1: Normal Operation
1. Water level at 130 cm (between bounds)
2. Pump OFF (within hysteresis zone)
3. Warning: "none"
4. Usage: "idle"

### Scenario 2: Low Water Refill
1. Water level drops to 165 cm
2. Rule 1a triggers: Pump turns ON
3. Water level rises back to 95 cm
4. Rule 1b triggers: Pump turns OFF
5. Status published with `pump_status: "on"` or `"off"`

### Scenario 3: Leak Detection
1. Water level drops faster than expected
2. `water_rate_cm_per_s` becomes < -5.0
3. Rule 2 triggers: `warning: "leak_detected"`
4. Alert published to MQTT broker

### Scenario 4: Overflow Prevention
1. Water level distance approaches 20 cm
2. Rule 3 triggers: `warning: "overflow_risk"`
3. Manual override or additional control may be needed

---

## 10. Integration with Project

### 10.1 Files Modified
- **`src/main.cpp`**: Core firmware with rule-based logic

### 10.2 Dependencies
- Arduino Framework (ESP32)
- PubSubClient (MQTT)
- ArduinoJson (JSON serialization)
- WiFi support (ESP32 built-in)

### 10.3 Credentials
- WiFi credentials: `WIFI_SSID`, `WIFI_PASSWORD` (from `secrets.h`)
- MQTT credentials: `MQTT_SERVER_IP`, `MQTT_SERVER_PORT` (from `secrets.h`)

---

## 11. Performance Metrics

| Metric | Value | Notes |
|--------|-------|-------|
| Update Frequency | 2 seconds | Sampling rate for all sensors |
| Hysteresis Gap | 60 cm | Prevents pump cycling |
| Leak Detection Sensitivity | -5.0 cm/s | Configurable threshold |
| Overflow Margin | 20 cm | Conservative safety margin |
| MQTT Publish Interval | 2 seconds | Bandwidth efficient |

---

## 12. Future Enhancements

1. **Adaptive Thresholds**: Adjust bounds based on time-of-day or usage patterns
2. **Historical Logging**: Store sensor data for trend analysis
3. **Predictive Control**: Estimate future levels to prevent overflow/underflow
4. **Remote Configuration**: Allow MQTT commands to update thresholds
5. **Multi-Sensor Fusion**: Combine flow meter and ultrasonic data
6. **Alert System**: Email/SMS notifications for critical warnings

---

## 13. Conclusion

A robust rule-based system has been successfully implemented for automated water tank management. The system operates autonomously with clear decision rules for:
- **Automatic pump control** using hysteresis
- **Real-time anomaly detection** (leaks and overflow risks)
- **Flow rate calculation** for consumption monitoring
- **Remote MQTT publishing** for integration with monitoring systems

The implementation follows embedded systems best practices with efficient sensor reading, minimal latency, and reliable state management.

---

**Report Prepared By:** Group 05  
**System Status:** Operational  
**Date:** April 30, 2026
