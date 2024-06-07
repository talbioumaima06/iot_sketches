#include <FirebaseESP32.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "SD.h"
#include "FS.h"

// Wi-Fi credentials
const char* ssid = "TOPNET_UKHT";
const char* password = "darhammouda2027";

// Firebase configuration
const char *firebaseHost = "https://litbebe-a66b1-default-rtdb.europe-west1.firebasedatabase.app/";
const char *databaseSecret = "lpbGsp8ScRUPqoFCUn2qhJbBGYxnYYUMOe4N0mP3";
const char *storageBucket = "gs://litbebe-a66b1.appspot.com";

// Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Define the GPIO pin for PWM audio output
#define AUDIO_PIN 2  // Example: Use GPIO2 for PWM output

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi");

  // Initialize Firebase
  config.host = firebaseHost;
  config.signer.tokens.legacy_token = databaseSecret;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  config.max_token_generation_retry = 5;

  Serial.println("Firebase initialized");

  // Initialize PWM pin for audio output
  ledcSetup(0, 1000, 8); // channel 0, frequency 1000Hz, resolution 8-bit
  ledcAttachPin(AUDIO_PIN, 0);

  // Read and play the audio file from Firebase Storage
  readFileFromStorage("/music.wav");
}

void loop() {
  // No need for continuous playback in this example
}

void readFileFromStorage(const char* filePath) {
  Serial.print("Reading file: ");
  Serial.println(filePath);

  HTTPClient http;
  String url = String("https://firebasestorage.googleapis.com/v0/b/") + storageBucket + "/o/" + filePath + "?alt=media&token=" + databaseSecret;
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode > 0) {
      WiFiClient *stream = http.getStreamPtr();
      uint8_t buffer[512];
      int len = 0;

      while (http.connected() && (len = stream->available()) > 0) {
        int c = stream->readBytes(buffer, min(len, (int)sizeof(buffer)));
        for (int i = 0; i < c; i++) {
          ledcWrite(0, buffer[i]);
          delay(5); // Adjust this delay for playback speed
        }
      }

      Serial.println("Finished playing audio");
    }
  } else {
    Serial.printf("GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
