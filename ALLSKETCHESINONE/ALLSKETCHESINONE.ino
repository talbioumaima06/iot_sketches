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

// Speaker
#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define CHANNELS 2
#define AUDIO_BUFFER_SIZE 1024
#define I2S_PIN_OUT 25 // Define the GPIO pin for I2S audio output
uint8_t buffer[AUDIO_BUFFER_SIZE]; // Define the audio buffer

// Sound Sensor
#define SOUND_SENSOR_PIN 5

// Movement Sensor
#define MOVEMENT_SENSOR_PIN 23

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

// State variable for music playing
bool isPlayingMusic = false;
String currentMusicFile = "";
WiFiClient *audioStream = nullptr; // Pointer to the audio stream
HTTPClient http;
size_t bytesRead = 0;

void setup() {
  // Initialize sensors and actuators
  pinMode(SOUND_SENSOR_PIN, INPUT);
  pinMode(MOVEMENT_SENSOR_PIN, INPUT);
  dht.begin();
  Serial.begin(115200);

  // WiFi
  all.initWiFi("TOPNET_UKHT", "darhammouda2027");
  all.connectToWiFi();
  Serial.println("Connected to WiFi!");

  // Initialize Firebase
  all.initFb(firebaseHost,databaseSecret);
  while (!Serial) {
    ; // Wait for Serial to be ready
  }
  Serial.println("Serial initialized");
  strip.begin();
  Serial.println("Strip initialized");
  strip.setBrightness(100);
  strip.fill(strip.Color(0, 0, 255));  // Blue color for testing
  strip.show();

  // Initialize servo motor
  swingServo.attach(SWING_SERVO_PIN);

  // Configure I2S for Speaker
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

  signupOK = true;
}

void loop() {
  static unsigned long lastLedTime = 0;
  static unsigned long lastMovementTime = 0;
  static unsigned long lastSoundTime = 0;
  static unsigned long lastDHTTime = 0;
  static unsigned long lastServoTime = 0;
  static unsigned long lastMusicCheckTime = 0;

  unsigned long currentTime = millis();
  Serial.println("-------------------Loop--------------------------------");
  Serial.println(currentTime);
  Serial.println(lastLedTime);
  Serial.println(lastMovementTime);
  Serial.println(lastSoundTime);
  Serial.println(lastDHTTime);
  Serial.println(lastServoTime);
  Serial.println("---------------------------------------------------");

  // Check if it's time to execute the LED function
  if (currentTime - lastLedTime >= 10000) { // 10 seconds
    lastLedTime = currentTime;
    led();
  }

  // Check if it's time to execute the movement function
  if (currentTime - lastMovementTime >= 10000) { // 10 seconds
    lastMovementTime = currentTime;
    mouvement();
  }

  // Check if it's time to execute the sound function
  if (currentTime - lastSoundTime >= 10000) { // 10 seconds
    lastSoundTime = currentTime;
    sound();
  }

  // Check if it's time to execute the DHT function
  if (currentTime - lastDHTTime >= 5000) { // 5 seconds
    lastDHTTime = currentTime;
    readDHT();
  }

  // Check if it's time to execute the servo function
  if (currentTime - lastServoTime >= 1000) { // 1 second
    lastServoTime = currentTime;
    controlServo();
  }

  // Check if it's time to check for music updates
  if (currentTime - lastMusicCheckTime >= 1000) { // 1 second
    lastMusicCheckTime = currentTime;
    checkAndPlayMusic();
  }

  // Process audio data if music is playing
  if (isPlayingMusic) {
    processMusicChunk();
  }

  delay(50); // Optional small delay to prevent excessive looping
}

void led() {
  Serial.println("-------------------LED--------------------------------");
  // LED actuator 
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
        } else {
            Serial.println("Invalid input. RGB values must be between 0 and 255.");
        }
    } else {
        Serial.println("Invalid input format. Use R,G,B format.");
    }
  } else if (ledStatus == 0) {
    strip.clear();
    strip.show();
    Serial.println("LED turned off.");
  } else {
    Serial.println("Invalid LED status.");
  }
  Serial.println("---------------------------------------------------");
}

void mouvement() {
  Serial.println("-------------------MOUVEMENT--------------------------------");
  // Movement Sensor
  int movement = digitalRead(MOVEMENT_SENSOR_PIN);
  Serial.println(movement);
  if (movement == HIGH) {
    Serial.println("Your baby has moved");
    if (all.setFbString("Sensor/Mvt_Status", "Your baby has moved")) {
      Serial.println("Data sent: baby is moving");
    } else {
      Serial.println("Failed to send data sound");
    }
  } else {
    Serial.println("Your baby is calm");
    if (all.setFbString("Sensor/Mvt_Status", "Your baby is calm")) {
      Serial.println("Data sent: baby is calm");
    } else {
      Serial.println("Failed to senddata mouvement");
    }
  }
  Serial.println("---------------------------------------------------");
}

void sound() {
  Serial.println("-------------------SOUND--------------------------------");
  // Sound Sensor
  int soundStatus = digitalRead(SOUND_SENSOR_PIN);
  if (soundStatus == HIGH) {
    Serial.println("Your baby has woken up!");
    if (all.setFbString("Sensor/sound_status", "Your baby has woken up!")) {
      Serial.println("Data sent: baby is awake");
    } else {
      Serial.println("No need to send data sound");
    }
  } else {
    Serial.println("Your baby is sleeping");
    if (all.setFbString("Sensor/sound_status", "Your baby is sleeping.")) {
      Serial.println("Data sent: baby is sleeping");
    } else {
      Serial.println("No need to send data sound");
    }
  }
  Serial.println("---------------------------------------------------");
}

void readDHT() {
  Serial.println("-------------------DHT--------------------------------");
  // DHT11 Sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (signupOK) {
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
  Serial.println("---------------------------------------------------");
}

void controlServo() {
  Serial.println("-------------------MOTOR--------------------------------");
  all.getFbString("Swings", swingStatus);
  int swingPosition = swingStatus.toInt();
  if (swingStatus == "true") {
    swingServo.write(90);
    Serial.println("Swing Servo moved to 45 degrees.");
    delay(500); // Wait for 0.5 second

    swingServo.write(15);
    Serial.println("Swing Servo moved to 135 degrees.");
    delay(500); // Wait for 0.5 second
  } else if (swingStatus == "false") {
    swingServo.write(45);   // Turn the servo off to 0 degrees
    Serial.println("Swing Servo turned off.");
  } else {
    Serial.println("Invalid swing status.");
  }
  Serial.println("---------------------------------------------------");
}

void checkAndPlayMusic() {
  String playing;
  all.getFbString("/music/playing", playing);

  if (playing == "1" && !isPlayingMusic) {
    all.getFbString("/music/current_playing", currentMusicFile);
    isPlayingMusic = true;
    playMusic(currentMusicFile.c_str());
  }
}

void playMusic(const char* filePath) {
  Serial.println("-------------------Play Music--------------------------------");
  Serial.print("Playing file: ");
  Serial.println(filePath);
  String url = String("https://firebasestorage.googleapis.com/v0/b/") + storageBucket + "/o/" + filePath + "?alt=media&token=" + databaseSecret;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    audioStream = http.getStreamPtr();
  } else {
    Serial.printf("Failed to get file: %d\n", httpCode);
    http.end();
    isPlayingMusic = false; // Reset the state
    return;
  }
}

void processMusicChunk() {
  if (audioStream && audioStream->available()) {
    size_t bytesRead = audioStream->readBytes(buffer, AUDIO_BUFFER_SIZE);
    size_t bytesWritten = 0;
    i2s_write(I2S_NUM_0, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
  } else {
    Serial.println("Finished playing audio");
    http.end();
    isPlayingMusic = false; // Reset the state
    all.setFbString("/music/playing", "0");
  }
}