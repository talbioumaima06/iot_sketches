#include <chawkiForAll.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>

// Firebase Configuration
const char *firebaseHost = "https://litbebe-a66b1-default-rtdb.europe-west1.firebasedatabase.app/";
const char *databaseSecret = "lpbGsp8ScRUPqoFCUn2qhJbBGYxnYYUMOe4N0mP3";

String RGB;
chawkiForAll all;

// NeoPixel LED
#define LED_PIN 4
#define NUMPIXELS 15
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Sound Sensor
#define SOUND_SENSOR_PIN 5
int soundThreshold = 400; // Adjust based on your environment
unsigned long lastMovementTime = 0;
unsigned long sleepingDelay = 3000;
String currentSoundStatus = "";

// Movement Sensor
#define MOVEMENT_SENSOR_PIN 13

// DHT11 Sensor
#define DHT_SENSOR_PIN 21
#define DHT_SENSOR_TYPE DHT11
DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

void tokenStatusCallback(TokenInfo info) {
  Serial.printf("Token Info: type = %d, status = %d\n", info.type, info.status);
}

void setup() {
  // Initialize sensors and actuators
  pinMode(SOUND_SENSOR_PIN, INPUT);
  pinMode(MOVEMENT_SENSOR_PIN, INPUT);
  dht.begin();
  Serial.begin(115200);

  // wifi
  all.initWiFi("TOPNET_UKHT", "darhammouda2027");
  all.connectToWiFi();
  Serial.println("Connected to WiFi!");

  // initiate firebase
  all.initFb(firebaseHost,databaseSecret);
  while (!Serial) {
    signupOK = true;
      ; // Wait for Serial to be ready
  }
  Serial.println("Serial initialized");
  strip.begin();
  Serial.println("Strip initialized");
  strip.setBrightness(100);
  strip.fill(strip.Color(0, 0, 255));  // Red color for testing
  strip.show();
  //Serial.println("Strip set to red. Setup complete.");
  //Serial.println("Enter RGB values as R,G,B:");

  // Initialize lastMovementTime to current time
  lastMovementTime = millis();
}

void loop() {

  led();
  mouvement();
  sound();
  readdht();
  
  delay(2000); // Optional delay to prevent continuous printing
}

void led(){
  // led actuator 
  all.getFbString("LED/RGB", RGB);
  int commaIndex1 = RGB.indexOf(',');
  int commaIndex2 = RGB.lastIndexOf(',');

  if (commaIndex1 > 0 && commaIndex2 > commaIndex1) {
      int r = RGB.substring(0, commaIndex1).toInt();
      int g = RGB.substring(commaIndex1 + 1, commaIndex2).toInt();
      int b = RGB.substring(commaIndex2 + 1).toInt();

      if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
          strip.fill(strip.Color(r, g, b));
          strip.show();
          //Serial.print("Updated color to: R=");
          //Serial.print(r);
          //Serial.print(", G=");
          //Serial.print(g);
          //Serial.print(", B=");
          //Serial.println(b);
      } else {
          Serial.println("Invalid input. RGB values must be between 0 and 255.");
      }
  } else {
      Serial.println("Invalid input format. Use R,G,B format.");
  }
}

void mouvement(){

  // Movement Sensor
  int movement = digitalRead(MOVEMENT_SENSOR_PIN);
  if (movement == HIGH) {
    Serial.println("Your baby has moveddddddddddddddddddddddddd");
    if (signupOK) {
      all.setFbString("Sensor/Mvt_Status", "Your baby has moved");
    }else{
      Serial.println("Failed to send data sound_________acvvvvvvvvvv____");
    }
  } else {
    Serial.println("Your baby is calmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm");
    if (1==1) {
      all.setFbString("Sensor/Mvt_Status", "Your baby is calm!!!!!!");
    }else{
      Serial.println("Failed to send data mouvement_______aaaaaaaa______");
    }
  }
}

void sound(){

  // Sound Sensor
  int soundLevel = digitalRead(SOUND_SENSOR_PIN);
  if (soundLevel > soundThreshold) {
    Serial.println("Your baby has woken up!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    lastMovementTime = millis();

    if (signupOK && currentSoundStatus != "Your baby has woken up!") {
      all.setFbString("Sensor/sound_status", "Your baby has woken up!");
      Serial.println("Data sent: baby is awake");
      currentSoundStatus = "Your baby has woken up!";
    } else {
      Serial.println("Failed to send data sound_____________");
    }
  } else {
    if (millis() - lastMovementTime >= sleepingDelay) {
      Serial.println("Your baby is sleeping......................................");
      if (2==2) {
        all.setFbString("Sensor/sound_status", "Your baby is sleeping..........");
        Serial.println("Data sent: baby is sleepinggggggggggggggggggggggggggg");
        currentSoundStatus = "Your baby is sleeping.";
      } else {
        Serial.println("Failed to send data soundddddddddddddddddddddddd");
      }
    }
  }
}

void readdht(){
  // DHT11 Sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (3==3) {
    if (all.setFbFloat("Sensor/DHT_11/Temperature", temperature)) {
      Serial.print("Temperature: ");
      Serial.println(temperature);
    } else {
      Serial.println("Failed to send temperature data");
    }
    if (all.setFbFloat("Sensor/DHT_11/Humidity", humidity)) {
      Serial.print("Humidity: ");
      Serial.println(humidity);
    } else {
      Serial.println("Failed to send humidity data");
    }
  }
}