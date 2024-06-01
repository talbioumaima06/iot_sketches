#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

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

// Initialize the sensor pin
int CapteurMvt = 15;

void setup() {
  // Set sensor pin as input
  pinMode(CapteurMvt , INPUT);

  // Initialize Serial for debugging
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
  // Read the sensor value
  int sensorValue = digitalRead(CapteurMvt);
  
  // Check sensor status and print message
  if(sensorValue == HIGH) { // The sensor detects movement
    Serial.println("Your baby has moved ");
    
    // Send data to Firebase
    if (Firebase.ready() && signupOK) {
      Firebase.RTDB.setString(&fbdo, "Sensor/Mvt_Status", "Your baby has moved");
    }
  } else { // The sensor does not detect movement
    Serial.println("Your baby is calm ");
    
    // Send data to Firebase
    if (Firebase.ready() && signupOK) {
      Firebase.RTDB.setString(&fbdo, "Sensor/Mvt_Status", "Your baby is calm");
    }
  }

  // Delay for 1 second
  delay(1000);
}
