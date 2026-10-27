// --- Blynk Credentials ---
// (Replace the placeholder credentials (BLYNK_TEMPLATE_ID, BLYNK_TEMPLATE_NAME, BLYNK_AUTH_TOKEN) with yours from Blynk) 
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Smart Water Tank System"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN_HERE"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Wokwi Virtual Wi-Fi Credentials
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// Pin Definitions
#define TRIG_PIN 5
#define ECHO_PIN 18
#define RELAY_PIN 23
#define BUZZER_PIN 19

// Tank Geometry (in cm)
const float TANK_TOTAL_DEPTH = 120.0; // Distance to bottom when empty
const float SENSOR_OFFSET    = 20.0;  // Minimum distance when 100% full

// Thresholds
const int OVERFLOW_THRESHOLD = 90;    // Auto-cutoff (%)
const int REFILL_THRESHOLD   = 20;    // Auto-on (%)

// OLED Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

BlynkTimer timer;
bool pumpState = false;

// Read ultrasonic sensor distance
float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return -1;
  return (duration * 0.0343) / 2.0;
}

// Convert distance to level percentage
int computeLevelPercentage(float dist) {
  if (dist < 0) return -1;
  float usableHeight = TANK_TOTAL_DEPTH - SENSOR_OFFSET;
  float currentWaterHeight = TANK_TOTAL_DEPTH - dist;
  int pct = (int)((currentWaterHeight / usableHeight) * 100.0);
  return constrain(pct, 0, 100);
}

// Set pump state and sync to Blynk V2
void setPump(bool state) {
  pumpState = state;
  digitalWrite(RELAY_PIN, pumpState ? LOW : HIGH); // Active-LOW relay
  Blynk.virtualWrite(V2, pumpState ? 1 : 0);
}

// Remote control from Blynk Dashboard Switch (V2)
BLYNK_WRITE(V2) {
  int value = param.asInt();
  if (value == 1) {
    // Prevent manual start if tank is already full
    float dist = readDistanceCm();
    int level = computeLevelPercentage(dist);
    if (level >= OVERFLOW_THRESHOLD) {
      Serial.println("[BLYNK] Blocked: Cannot start pump when tank is full!");
      Blynk.virtualWrite(V2, 0); // Revert switch
      return;
    }
    setPump(true);
    Serial.println("[BLYNK] Pump turned ON via Dashboard.");
  } else {
    setPump(false);
    Serial.println("[BLYNK] Pump turned OFF via Dashboard.");
  }
}

// Periodic routine to monitor level, execute safety logic, and send to Blynk
void monitorAndSync() {
  float distance = readDistanceCm();
  int level = computeLevelPercentage(distance);

  if (level < 0) {
    Serial.println("[ERROR] Sensor reading invalid.");
    return;
  }

  String systemStatus = "Normal";

  // 1. Overflow Prevention Logic
  if (level >= OVERFLOW_THRESHOLD) {
    if (pumpState) {
      setPump(false);
      Serial.println("[CRITICAL] Overflow threshold hit! Pump cut off automatically.");
    }
    digitalWrite(BUZZER_PIN, HIGH);
    systemStatus = "OVERFLOW WARNING!";
    Blynk.logEvent("tank_overflow", "Water level exceeded 90%! Pump stopped."); // Optional Event
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // 2. Auto-Refill Logic
  if (level <= REFILL_THRESHOLD && !pumpState) {
    setPump(true);
    systemStatus = "Refilling (Low Level)";
    Serial.println("[INFO] Low water level detected. Auto-refill initiated.");
  }

  if (pumpState && level < OVERFLOW_THRESHOLD && level > REFILL_THRESHOLD) {
    systemStatus = "Pumping Water...";
  }

  // 3. Push Telemetry to Blynk IoT
  Blynk.virtualWrite(V0, level);
  Blynk.virtualWrite(V1, distance);
  Blynk.virtualWrite(V3, systemStatus);

  // 4. Update Local OLED Display
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("IOT WATER MONITOR");

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(level);
  display.print("%");

  // Visual Bar
  display.drawRect(55, 16, 70, 15, SSD1306_WHITE);
  int barWidth = map(level, 0, 100, 0, 66);
  display.fillRect(57, 18, barWidth, 11, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("Dist: %.1f cm", distance);
  display.setCursor(0, 52);
  display.printf("Pump: %s", pumpState ? "RUNNING" : "OFF");
  display.display();

  Serial.printf("Level: %d%% | Dist: %.1f cm | Pump: %s | Status: %s\n", 
                level, distance, pumpState ? "ON" : "OFF", systemStatus.c_str());
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  setPump(false);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize OLED
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(15, 25);
    display.print("Connecting Blynk...");
    display.display();
  }

  // Connect to Blynk cloud via Wokwi simulated Wi-Fi
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Read sensor and sync every 1.5 seconds
  timer.setInterval(1500L, monitorAndSync);
}

void loop() {
  Blynk.run();
  timer.run();
}
