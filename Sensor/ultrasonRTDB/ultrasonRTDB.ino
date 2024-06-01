#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define capteur 4 // GPIO 4 on ESP32, used for analog input
int threshold = 400; // Adjust this value based on the noise level in your environment
unsigned long lastMovementTime = 0; // Variable to store the time of the last movement detection
unsigned long sleepingDelay = 3000; // Delay in milliseconds before considering the baby is sleeping
String currentStatus = ""; // To track the current status of the baby

// Insert your network credentials
#define WIFI_SSID "TT_01C0"
#define WIFI_PASSWORD "d6uqta94b8"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCo1xuU3wb0JSdSPU6XS84OMAqByqpK7FQ"

// Insert RTDB URL
#define DATABASE_URL "https://login-litbebe-default-rtdb.europe-west1.firebasedatabase.app/"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false; // since we are doing an anonymous sign in

void setup() {
  pinMode(capteur, INPUT); // Configure the sensor pin as input
  Serial.begin(115200); // Initialize serial communication with baud rate 115200

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

  // Assign the API key (required)
  config.api_key = API_KEY;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  // Sign up
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long-running token generation task
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize lastMovementTime to current time
  lastMovementTime = millis();
}

void loop() {
  int soundLevel = analogRead(capteur); // Read the analog value from the sound sensor
  
  if (soundLevel > threshold) {
    Serial.println("Your baby has woken up!"); // Print the message to the serial monitor
    lastMovementTime = millis(); // Update the last movement detection time

    // Send data to Firebase if the state changes
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
    // Check if it's time to consider the baby is sleeping
    if (millis() - lastMovementTime >= sleepingDelay) {
      Serial.println("Your baby is sleeping."); // Print the message to the serial monitor

      // Send data to Firebase if the state changes
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
