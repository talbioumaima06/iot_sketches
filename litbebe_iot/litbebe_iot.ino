#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// WiFi Credentials
#define WIFI_SSID "Redmi Note 13"
#define WIFI_PASSWORD "1234567899"

// Firebase Configuration
#define API_KEY "AIzaSyCiWthdW9oA6TIm1vBJSCq83Q4IKRGdm7E"
#define DATABASE_URL "https://litbebe-a66b1-default-rtdb.europe-west1.firebasedatabase.app/"

// Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// Ultrasonic Sensor
#define CAPTEUR 19
int threshold = 400;
unsigned long lastMovementTime = 0;
unsigned long sleepingDelay = 3000;
String currentStatus = "";

// Movement Sensor
int CapteurMvt = 15;

// NeoPixel LED
#define LED_PIN 22
#define NUMPIXELS 100
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// DHT Sensor
#define DHT_SENSOR_PIN 4
#define DHT_SENSOR_TYPE DHT11
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
unsigned long sendDataPrevMillis = 0;
int count = 0;

void setup() {
  // Initialize DHT sensor
  dht_sensor.begin();
  
  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(50);

  // Set sensor pin modes
  pinMode(CapteurMvt, INPUT);
  pinMode(CAPTEUR, INPUT);

  // Initialize Serial for user input and debug
  Serial.begin(115200);

  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  //config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize lastMovementTime to current time
  lastMovementTime = millis();
}

void loop() {
  temperatureHumiditySensor();
  //rgbSensor();
  movementSensor();
  soundSensor();
  delay(3000);
}

void temperatureHumiditySensor() {
  float temperature = dht_sensor.readTemperature();
  float humidity = dht_sensor.readHumidity();

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setInt(&fbdo, "Sensor/DHT_11/Temperature", temperature)) {
      Serial.print("Temperature : ");
      Serial.println(temperature);
    } else {
      Serial.println("Failed to Read Temperature from the Sensor");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "Sensor/DHT_11/Humidity", humidity)) {
      Serial.print("Humidity : ");
      Serial.println(humidity);
    } else {
      Serial.println("Failed to Read Humidity from the Sensor");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

void rgbSensor() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    int r, g, b;
    sscanf(input.c_str(), "%d %d %d", &r, &g, &b);
    setColor(strip.Color(r, g, b));
    Firebase.RTDB.setString(&fbdo, "LED/RGB", input);
  }

  if (Firebase.ready() && signupOK) {
    String rgbValues;
    if (Firebase.RTDB.getString(&fbdo, "LED/RGB", &rgbValues)) {
      int r, g, b;
      sscanf(rgbValues.c_str(), "%d %d %d", &r, &g, &b);
      setColor(strip.Color(r, g, b));
    } else {
      Serial.println("Failed to get RGB data");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

void movementSensor() {
  int sensorValue = digitalRead(CapteurMvt);
  if (sensorValue == HIGH) {
    Serial.println("Your baby has moved ");
    if (Firebase.ready() && signupOK) {
      Firebase.RTDB.setString(&fbdo, "Sensor/Mvt_Status", "Your baby has moved");
    }
  } else {
    Serial.println("Your baby is calm ");
    if (Firebase.ready() && signupOK) {
      Firebase.RTDB.setString(&fbdo, "Sensor/Mvt_Status", "Your baby is calm");
    }
  }
}

void soundSensor() {
  int soundLevel = digitalRead(CAPTEUR);
  // Log the sound level
  Serial.print("Sound levelllllllllllllllllllllll: ");
  Serial.println(soundLevel);

  if (soundLevel == 1 ) {
    Serial.println("Your baby has woken up!");
    lastMovementTime = millis();
    if (Firebase.ready() && signupOK && currentStatus != "Your baby has woken up!") {
      if (Firebase.RTDB.setString(&fbdo, "Sensor/sound_status", "Your baby has woken up!")) {
        Serial.println("Data sent: baby is awake");
        currentStatus = "Your baby has woken up!";
      } else {
        Serial.println("Failed to send data");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    }
  } else {
    if (millis() - lastMovementTime >= sleepingDelay) {
      Serial.println("Your baby is sleeping.");
      if (Firebase.ready() && signupOK && currentStatus != "Your baby is sleeping.") {
        if (Firebase.RTDB.setString(&fbdo, "Sensor/sound_status", "Your baby is sleeping.")) {
          Serial.println("Data sent: baby is sleeping");
          currentStatus = "Your baby is sleeping.";
        } else {
          Serial.println("Failed to send data");
          Serial.println("REASON: " + fbdo.errorReason());
        }
      }
    }
  }
}

void setColor(uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}
