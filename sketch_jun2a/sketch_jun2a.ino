#include <Arduino.h>
#include <WiFi.h>               //we are using the ESP32
#include <Firebase_ESP_Client.h>
#include <DHT.h>                // Install DHT library by adafruit 1.3.8
#include <Adafruit_NeoPixel.h> // LED package

#include "addons/TokenHelper.h" // Provide the token generation process info.
#include "addons/RTDBHelper.h" // Provide the RTDB payload printing info and other helper functions.

// WiFi Credentials
#define WIFI_SSID "Redmi Note 13"
#define WIFI_PASSWORD "1234567899"


// Firebase Configuration
#define API_KEY "AIzaSyCo1xuU3wb0JSdSPU6XS84OMAqByqpK7FQ"
#define DATABASE_URL "https://login-litbebe-default-rtdb.europe-west1.firebasedatabase.app/"
// Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false; // since we are doing an anonymous sign in



//ULTRASON
#define capteur 19 // GPIO 4 on ESP32, used for analog input
int threshold = 400; // Adjust this value based on the noise level in your environment
unsigned long lastMovementTime = 0; // Variable to store the time of the last movement detection
unsigned long sleepingDelay = 3000; // Delay in milliseconds before considering the baby is sleeping
String currentStatus = ""; // To track the current status of the baby



// MOUVEMENT
int CapteurMvt = 15;



// NeoPixel LED
#define LED_PIN 22
#define NUMPIXELS 100
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
// Token status callback function
//void tokenStatusCallback(TokenInfo info) {
  //Serial.printf("Token Info: type = %d, status = %d\n", info.type, info.status);
//}


//TEMPERATURE
#define DHT_SENSOR_PIN 4
#define DHT_SENSOR_TYPE DHT11
//To provide the ESP32 / ESP8266 with the connection and the sensor type
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
unsigned long sendDataPrevMillis = 0;
int count = 0;



void setup() {
  // Initialize DHT sensor
  dht_sensor.begin();
  
  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(50);  // Set initial brightness

  // Set sensor pin modes
  pinMode(CapteurMvt, INPUT);
  pinMode(capteur, INPUT); // Configure the sensor pin as input

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

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize lastMovementTime to current time
  lastMovementTime = millis();
}

void loop() {
  // Temperature and humidity measurement
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

  // Check for serial input to change RGB color
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    int r, g, b;
    sscanf(input.c_str(), "%d %d %d", &r, &g, &b);
    setColor(strip.Color(r, g, b));
    Firebase.RTDB.setString(&fbdo, "LED/RGB", input);
  }

  // Check for RGB value changes in Firebase
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

  // Movement Sensor
  int sensorValue = digitalRead(CapteurMvt);
  if (sensorValue == HIGH) { // The sensor detects movement
    Serial.println("Your baby has moved ");
    if (Firebase.ready() && signupOK) {
      Firebase.RTDB.setString(&fbdo, "Sensor/Mvt_Status", "Your baby has moved");
    }
  } else { // The sensor does not detect movement
    Serial.println("Your baby is calm ");
    if (Firebase.ready() && signupOK) {
      Firebase.RTDB.setString(&fbdo, "Sensor/Mvt_Status", "Your baby is calm");
    }
  }

  // Sound Sensor
  int soundLevel = analogRead(capteur); // Read the analog value from the sound sensor
  if (soundLevel > threshold) {
    Serial.println("Your baby has woken up!"); // Print the message to the serial monitor
    lastMovementTime = millis(); // Update the last movement detection time
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
      Serial.println("Your baby is sleeping."); // Print the message to the serial monitor
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

  delay(500); // Optional delay to prevent continuous printing
}

void setColor(uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}
