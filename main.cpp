#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"      
#define BLYNK_DEVICE_NAME "ESP32 Controller"      
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"        

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// --- Credential Setup ---
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "YOUR_WIFI_NAME";       
char pass[] = "YOUR_WIFI_PASSWORD";  
// --- Pin Definitions ---
#define DHTPIN 27
#define DHTTYPE DHT11
const int relay1 = 18; // Assume Fan
const int relay2 = 19; // Assume Exhaust/Alarm
const int irSensor = 23;
const int mq2do = 33;
const int mq2ao = 34;

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

// --- State Variables ---
int wifiButtonState = 0; 
bool autoMode = false;   // V4 toggle state
float currentTemp = 0.0;

// --- Sync App States on Boot/Reconnect ---
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V3); // Sync Relay Button
  Blynk.syncVirtual(V4); // Sync Auto/Manual Toggle
}

// --- App Button Handlers ---
BLYNK_WRITE(V3) {
  wifiButtonState = param.asInt(); // Relay control
}

BLYNK_WRITE(V4) {
  autoMode = param.asInt(); // Auto/Manual toggle
  if(autoMode) Serial.println("Switched to AUTO Mode");
  else Serial.println("Switched to MANUAL Mode");
}

// --- Sensor Reading ---
void sendSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(t)) currentTemp = t; // Update global temp for relay logic

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  int smokeLevel = analogRead(mq2ao);
  int smokeDigital = digitalRead(mq2do);

  Blynk.virtualWrite(V0, currentTemp);          
  Blynk.virtualWrite(V1, h);          
  Blynk.virtualWrite(V2, smokeLevel); 

  Serial.print("Temp: "); Serial.print(currentTemp);
  Serial.print(" | Hum: "); Serial.print(h);
  Serial.print(" | Smoke: "); Serial.println(smokeLevel);

  if (smokeDigital == HIGH) {
    Blynk.logEvent("smoke_alert", "Smoke Detected!"); 
  }
}

// --- Relay Logic (Auto vs Manual) ---
void controlRelays() {
  if (autoMode) {
    // AUTO MODE: Controlled by environment
    bool smokeDetected = (digitalRead(mq2do) == HIGH);
    
    // Auto Temp logic: Turn on Relay 1 (Fan) if Temp > 30C
    if (currentTemp > 30.0) digitalWrite(relay1, HIGH);
    else digitalWrite(relay1, LOW);

    // Auto Smoke logic: Turn on Relay 2 (Exhaust) if Smoke Detected
    if (smokeDetected) digitalWrite(relay2, HIGH);
    else digitalWrite(relay2, LOW);
    
  } else {
    // MANUAL MODE: Controlled by IR or App
    bool irActive = (digitalRead(irSensor) == HIGH);
    bool wifiActive = (wifiButtonState == 1);

    if (irActive || wifiActive) {
      digitalWrite(relay1, HIGH);
      digitalWrite(relay2, HIGH);
    } else {
      digitalWrite(relay1, LOW);
      digitalWrite(relay2, LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(irSensor, INPUT);
  pinMode(mq2do, INPUT);
  
  dht.begin();
  Blynk.begin(auth, ssid, pass);

  timer.setInterval(1000L, sendSensorData);
  timer.setInterval(100L, controlRelays);
}

void loop() {
  Blynk.run(); 
  timer.run(); 
}
