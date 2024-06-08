#include <chawkiForAll.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <Servo.h>  // Include the Servo library

#include <HTTPClient.h>
#include "FS.h"
#include "driver/i2s.h"

// Firebase Configuration
const char *firebaseHost = "https://litbebe-a66b1-default-rtdb.europe-west1.firebasedatabase.app/";
const char *databaseSecret = "lpbGsp8ScRUPqoFCUn2qhJbBGYxnYYUMOe4N0mP3";
const char *storageBucket = "litbebe-a66b1.appspot.com";

String RGB;
int ledStatus;
chawkiForAll all;

// NeoPixel LED
#define LED_PIN 4
#define NUMPIXELS 15
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

//speaker
#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define CHANNELS 2
#define AUDIO_BUFFER_SIZE 1024
#define I2S_PIN_OUT 25 // Define the GPIO pin for I2S audio output
uint8_t buffer[AUDIO_BUFFER_SIZE]; // Define the audio buffer

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

// Servo Motor
Servo swingServo;
#define SWING_SERVO_PIN 18
String swingStatus;

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
    ; // Wait for Serial to be ready
  }
  Serial.println("Serial initialized");
  strip.begin();
  Serial.println("Strip initialized");
  strip.setBrightness(100);
  strip.fill(strip.Color(0, 0, 255));  // Red color for testing
  strip.show();

  // Initialize lastMovementTime to current time
  lastMovementTime = millis();
  

  //Speaker
  // Initialize servo motor
  swingServo.attach(SWING_SERVO_PIN);
   // Configure I2S
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 2,
      .dma_buf_len = AUDIO_BUFFER_SIZE,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0};
  i2s_pin_config_t pin_config = {
      .bck_io_num = -1,  // Not used (output)
      .ws_io_num = -1,   // Not used (output)
      .data_out_num = I2S_PIN_OUT, // Set the GPIO pin for I2S audio output
      .data_in_num = -1   // Not used (output)
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);
  // Read and play the audio file from Firebase Storage
  readFileFromStorage("music.wav");

}

void loop() {
  led();
  mouvement();
  sound();
  readDHT(); // Renamed the function to readDHT
  controlServo(); // Add this function call to control the servo
  
  delay(2000); // Optional delay to prevent continuous printing
}

void led() {
  // led actuator 
  all.getFbString("LED/RGB", RGB);
  int commaIndex1 = RGB.indexOf(',');
  int commaIndex2 = RGB.lastIndexOf(',');

  String status;
  all.getFbString("LED/status", status);
  ledStatus = status.toInt();
  if (ledStatus == 1) {
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
  }else if (ledStatus == 0) {
    strip.clear();
    strip.show();
    Serial.println("LED turned off.");
  } else {
    Serial.println("Invalid LED status.");
  }
}

void mouvement() {
  // Movement Sensor
  int movement = digitalRead(MOVEMENT_SENSOR_PIN);
  if (movement == HIGH) {
    Serial.println("Your baby has moveddddddddddddddddddddddddd");
    if (signupOK) {
      all.setFbString("Sensor/Mvt_Status", "Your baby has moved");
    } else {
      Serial.println("Failed to send data sound_________acvvvvvvvvvv____");
    }
  } else {
    Serial.println("Your baby is calmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm");
    if (1 == 1) {
      all.setFbString("Sensor/Mvt_Status", "Your baby is calm!!!!!!");
    } else {
      Serial.println("Failed to send data mouvement_______aaaaaaaa______");
    }
  }
}

void sound() {
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
      if (2 == 2) {
        all.setFbString("Sensor/sound_status", "Your baby is sleeping..........");
        Serial.println("Data sent: baby is sleepinggggggggggggggggggggggggggg");
        currentSoundStatus = "Your baby is sleeping.";
      } else {
        Serial.println("Failed to send data soundddddddddddddddddddddddd");
      }
    }
  }
}

void readDHT() { // Renamed the function to readDHT
  // DHT11 Sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (3 == 3) {
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

void controlServo() {
  all.getFbString("Swings", swingStatus);
  int swingPosition = swingStatus.toInt();
  if (swingPosition == 1) {
    // Move to position 0 degrees (simulating -90 degrees)
    swingServo.write(0);
    Serial.println("Swing Servo moved to 0 degrees (simulating -90 degrees).");
    delay(1000); // Wait for 1 second
    
    // Move to position 90 degrees (simulating 0 degrees)
    swingServo.write(90);
    Serial.println("Swing Servo moved to 90 degrees (simulating 0 degrees).");
    delay(1000); // Wait for 1 second

    // Move to position 180 degrees (simulating 90 degrees)
    swingServo.write(180);
    Serial.println("Swing Servo moved to 180 degrees (simulating 90 degrees).");
    delay(1000); // Wait for 1 second

    // Move back to position 90 degrees (simulating 0 degrees)
    swingServo.write(90);
    Serial.println("Swing Servo moved back to 90 degrees (simulating 0 degrees).");
    delay(1000); // Wait for 1 second
  } else if (swingPosition == 0) {
    swingServo.write(0);   // Turn the servo off to 0 degrees
    Serial.println("Swing Servo turned off.");
  } else {
    Serial.println("Invalid swing status.");
  }
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
      size_t bytesRead = 0;

      while (http.connected() && (bytesRead = stream->readBytes(buffer, AUDIO_BUFFER_SIZE)) > 0) {
        size_t bytesWritten = 0;
        i2s_write(I2S_NUM_0, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
      }

      Serial.println("Finished playing audio");
    }
  } else {
    Serial.printf("GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}