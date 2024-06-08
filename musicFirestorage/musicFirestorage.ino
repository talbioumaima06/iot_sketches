#include <FirebaseESP32.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"

// Wi-Fi credentials
const char* ssid = "TOPNET_UKHT";
const char* password = "darhammouda2027";

// Firebase configuration
const char *firebaseHost = "https://litbebe-a66b1-default-rtdb.europe-west1.firebasedatabase.app/";
const char *databaseSecret = "lpbGsp8ScRUPqoFCUn2qhJbBGYxnYYUMOe4N0mP3";
const char *storageBucket = "litbebe-a66b1.appspot.com";

// Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Define the GPIO pin for PWM audio output
#define AUDIO_PIN 2  // Example: Use GPIO2 for PWM output

#define BUFFER_SIZE 512 // Buffer size for audio data
uint8_t buffer[BUFFER_SIZE];
int bufferIndex = 0;

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
  ledcSetup(0, 44100, 8); // channel 0, frequency 44100Hz (standard audio sampling rate), resolution 8-bit
  ledcAttachPin(AUDIO_PIN, 0);

  // Read and play the audio file from Firebase Storage
  readFileFromStorage("music.wav");
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
    Serial.println(httpCode);
    if (httpCode == HTTP_CODE_OK) {
      WiFiClient *stream = http.getStreamPtr();
      int len = 0;

      while (http.connected() && (len = stream->available()) > 0) {
        int bytesRead = stream->readBytes(buffer + bufferIndex, min(len, BUFFER_SIZE - bufferIndex));
        bufferIndex += bytesRead;

        // If the buffer is full or the end of the file is reached
        if (bufferIndex >= BUFFER_SIZE || len == 0) {
          // Write buffered data to PWM output
          for (int i = 0; i < bufferIndex; i++) {
            ledcWrite(0, buffer[i]);
            delayMicroseconds(23); // Adjusted delay for 44.1kHz sample rate
          }
          bufferIndex = 0; // Reset buffer index
        }
      }

      // Handle any remaining data in the buffer
      if (bufferIndex > 0) {
        for (int i = 0; i < bufferIndex; i++) {
          ledcWrite(0, buffer[i]);
          delayMicroseconds(23); // Adjusted delay for 44.1kHz sample rate
        }
        bufferIndex = 0; // Reset buffer index
      }

      Serial.println("Finished playing audio");
    }
  } else {
    Serial.printf("GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
