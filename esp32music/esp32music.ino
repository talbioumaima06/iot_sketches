#include <FirebaseESP32.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "driver/i2s.h"

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

#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define CHANNELS 2
#define AUDIO_BUFFER_SIZE 1024

// Define the GPIO pin for I2S audio output
#define I2S_PIN_OUT 25

// Define the audio buffer
uint8_t buffer[AUDIO_BUFFER_SIZE];

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
