#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_NeoPixel.h>

// NeoPixel LED
#define LED_PIN 22
#define NUMPIXELS 100
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// WiFi Credentials
#define WIFI_SSID "TT_01C0"
#define WIFI_PASSWORD "d6uqta94b8"

// Firebase Configuration
#define API_KEY "AIzaSyCo1xuU3wb0JSdSPU6XS84OMAqByqpK7FQ"
#define DATABASE_URL "https://login-litbebe-default-rtdb.europe-west1.firebasedatabase.app/"

// Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// Token status callback function
void tokenStatusCallback(TokenInfo info) {
  Serial.printf("Token Info: type = %d, status = %d\n", info.type, info.status);
}

void setup() {
  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(50);  // Set initial brightness

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
}

void loop() {
  // Check for serial input to change RGB color
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    int r, g, b;
    sscanf(input.c_str(), "%d %d %d", &r, &g, &b);
    setColor(strip.Color(r, g, b));

    // Send RGB values to Firebase
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
}

void setColor(uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}
