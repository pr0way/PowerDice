#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// --- NETWORK CONFIGURATION ---
const char* ssid = "";          // <--- ENTER YOUR WIFI NAME
const char* password = "";      // <--- ENTER YOUR WIFI PASSWORD
const char* mqtt_server = "192.168.1.X"; // <--- ENTER YOUR MQTT IP
const char* mqtt_topic = "dice/status";

// --- HARDWARE CONFIGURATION ---
#define BATTERY_PIN 2           // GPIO2 for ESP32-C3 (ADC1_CH2)
const float THRESHOLD = 8.0;

// Wake-up Pin (GPIO3 for Seeed Xiao ESP32-C3)
#define ADXL_INT_PIN 3 
#define ADXL345_ADDR 0x53

// Sleep Configuration (Session Mode)
#define INACTIVITY_TIME_SEC 120  // 2 minutes of inactivity = deep sleep

// Heartbeat Configuration (Weekly battery report)
#define HEARTBEAT_INTERVAL_US (7ULL * 24 * 60 * 60 * 1000000ULL) // 7 days

// MQTT Keep-alive (synchronized with inactivity time)
#define MQTT_KEEPALIVE 45  // = 45 seconds

unsigned long lastActivityTime = 0; // Activity timer

// State tracking variables
int lastSentResult = 0;

// Objects
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
WiFiClient espClient;
PubSubClient client(espClient);

// --- BATTERY READ FUNCTION ---
float readBatteryVoltage() {
  uint32_t mv = 0;
  for(int i=0; i<16; i++) {
    mv += analogReadMilliVolts(BATTERY_PIN);
  }
  
  float v_pin_mv = mv / 16.0;
  float v_bat = (v_pin_mv * 2) / 1000.0; // 220k/220k divider = x2
  
  return v_bat;
}

// --- SEND BATTERY ONLY (For Heartbeat) ---
void sendBatteryReport() {
  float vbat = readBatteryVoltage();
  char msg[100];
  snprintf(msg, sizeof(msg), "{\"vbat\": %.2f}", vbat);
  
  Serial.print("HEARTBEAT: ");
  Serial.println(msg);
  client.publish(mqtt_topic, msg);
  
  // Give MQTT time to send
  client.loop();
  delay(500);
}

// --- ADXL INTERRUPT CONFIGURATION (For Wake-up) ---
void configureADXLInterrupts() {
  Wire.beginTransmission(ADXL345_ADDR); 
  Wire.write(0x2D); 
  Wire.write(0x08); 
  Wire.endTransmission(); // Power ON
  
  Wire.beginTransmission(ADXL345_ADDR); 
  Wire.write(0x24); 
  Wire.write(20);   
  Wire.endTransmission(); // Threshold (Sensitivity)
  
  Wire.beginTransmission(ADXL345_ADDR); 
  Wire.write(0x27); 
  Wire.write(0x70); 
  Wire.endTransmission(); // Axes X Y Z
  
  Wire.beginTransmission(ADXL345_ADDR); 
  Wire.write(0x2F); 
  Wire.write(0x00); 
  Wire.endTransmission(); // Map to INT1
  
  Wire.beginTransmission(ADXL345_ADDR); 
  Wire.write(0x2E); 
  Wire.write(0x10); 
  Wire.endTransmission(); // Enable Activity
}

// --- CLEAR INTERRUPT ---
void clearADXLInterrupt() {
  Wire.beginTransmission(ADXL345_ADDR); 
  Wire.write(0x30); 
  Wire.endTransmission();
  Wire.requestFrom(ADXL345_ADDR, 1); 
  if(Wire.available()) Wire.read(); 
}

// --- WIFI SETUP ---
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to: "); 
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
  } else {
    Serial.println(" No WiFi - resetting!");
    ESP.restart();
  }
}

// --- MQTT RECONNECT ---
void reconnect() {
  int attempts = 0;
  while (!client.connected() && attempts < 3) {
    Serial.print("Connecting MQTT...");
    String clientId = "Dice-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
    } else {
      Serial.print("Error, rc="); 
      Serial.print(client.state());
      Serial.println(" try again in 5s");
      delay(5000);
      lastActivityTime = millis(); 
    }
    attempts++;
  }
}

// --- DICE FACE LOGIC ---
int getDiceFace() {
  sensors_event_t event;
  accel.getEvent(&event);
  float x = event.acceleration.x;
  float y = event.acceleration.y;
  float z = event.acceleration.z;

  if (x > THRESHOLD) return 3;
  if (x < -THRESHOLD) return 4;
  if (y > THRESHOLD) return 5;
  if (y < -THRESHOLD) return 2;
  if (z > THRESHOLD) return 1;
  if (z < -THRESHOLD) return 6;
  return 0; 
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Set I2C pins
  Wire.begin(6, 7);  // SDA=D4 (GPIO6), SCL=D5 (GPIO7)
  pinMode(BATTERY_PIN, INPUT);

  // === CHECK WAKEUP REASON ===
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    // --- HEARTBEAT MODE: Send only battery and go back to sleep ---
    Serial.println("Woke up by TIMER (Heartbeat)");
    
    // Initialize minimal hardware
    if(!accel.begin()) {
      Serial.println("ERROR: No ADXL345 - skipping heartbeat");
      ESP.restart();
    }
    
    // Connect to network
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setKeepAlive(MQTT_KEEPALIVE);
    
    if (!client.connected()) {
      reconnect();
    }
    
    // Send battery report
    sendBatteryReport();
    
    // Go back to sleep immediately
    Serial.println("Heartbeat done. Back to sleep...");
    client.disconnect();
    delay(100);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    clearADXLInterrupt();
    configureADXLInterrupts();
    
    uint64_t gpio_mask = 1ULL << ADXL_INT_PIN;
    esp_deep_sleep_enable_gpio_wakeup(gpio_mask, ESP_GPIO_WAKEUP_GPIO_HIGH);
    esp_sleep_enable_timer_wakeup(HEARTBEAT_INTERVAL_US);
    
    Serial.flush();
    esp_deep_sleep_start();
  }
  
  // --- NORMAL MODE: Woke up by shake or first boot ---
  if (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) {
    Serial.println("Woke up by shake!");
  } else {
    Serial.println("Woke up by power/reset");
  }

  // 1. ADXL Initialization
  if(!accel.begin()) {
    Serial.println("ERROR: No ADXL345 found");
    Serial.println("Check wiring: SDA=D4(GPIO6), SCL=D5(GPIO7)");
    while(1) delay(10);
  }
  accel.setRange(ADXL345_RANGE_16_G);

  // 2. Network Connection
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setKeepAlive(MQTT_KEEPALIVE);
  
  Serial.print("MQTT Keep-alive: ");
  Serial.print(MQTT_KEEPALIVE);
  Serial.println(" seconds");
  Serial.print("Inactivity timeout: ");
  Serial.print(INACTIVITY_TIME_SEC);
  Serial.println(" seconds");
  Serial.print("Heartbeat interval: ");
  Serial.print(HEARTBEAT_INTERVAL_US / (1000000ULL * 60 * 60 * 24));
  Serial.println(" days");

  // 3. Wake-up Pin Configuration
  pinMode(ADXL_INT_PIN, INPUT_PULLDOWN);

  // Reset activity timer on startup
  lastActivityTime = millis();
}

void loop() {
  // 1. Maintain MQTT Connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 2. MOTION DETECTION (SESSION MAINTENANCE)
  sensors_event_t event;
  accel.getEvent(&event);
  
  float totalAccel = sqrt(event.acceleration.x * event.acceleration.x + 
                          event.acceleration.y * event.acceleration.y + 
                          event.acceleration.z * event.acceleration.z);
  
  if (totalAccel > 15.0) {
      lastActivityTime = millis(); 
  }

  // 3. THROW LOGIC
  int currentResult = getDiceFace();

  if (currentResult > 0 && currentResult != lastSentResult) {
    
    unsigned long stabilizationStart = millis();
    while(millis() - stabilizationStart < 500) {
      client.loop();
      delay(10);
    }
    
    int doubleCheck = getDiceFace();

    if (doubleCheck == currentResult) {
      float vbat = readBatteryVoltage();
      char msg[100];
      snprintf(msg, sizeof(msg), "{\"result\": %d, \"vbat\": %.2f}", currentResult, vbat);
      
      Serial.print("NEW RESULT: "); 
      Serial.println(msg);
      client.publish(mqtt_topic, msg);

      lastSentResult = currentResult;
      lastActivityTime = millis();
    }
  }

  // 4. INACTIVITY CHECK (AUTO-SLEEP)
  if (millis() - lastActivityTime > (INACTIVITY_TIME_SEC * 1000UL)) {
      Serial.println("No activity. Going to sleep...");
      
      clearADXLInterrupt();
      configureADXLInterrupts();
      
      client.disconnect();
      delay(100);
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(100);

      // Configure BOTH wake-up sources:
      // A. GPIO wake (shake detection)
      uint64_t gpio_mask = 1ULL << ADXL_INT_PIN;
      esp_deep_sleep_enable_gpio_wakeup(gpio_mask, ESP_GPIO_WAKEUP_GPIO_HIGH);
      
      // B. Timer wake (weekly heartbeat)
      esp_sleep_enable_timer_wakeup(HEARTBEAT_INTERVAL_US);
      
      Serial.flush();
      esp_deep_sleep_start();
  }

  delay(50);
}