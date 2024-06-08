#include "esp_camera.h"
#include <chawkiForAll.h>
#include "arduino_base64.hpp"

// Select camera model
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"

// WiFi credentials
const char* ssid = "TOPNET_UKHT";
const char* password = "darhammouda2027";

// Firebase Configuration
const char *firebaseHost = "https://litbebe-a66b1-default-rtdb.europe-west1.firebasedatabase.app/";
const char *databaseSecret = "lpbGsp8ScRUPqoFCUn2qhJbBGYxnYYUMOe4N0mP3";
const char *storageBucket = "litbebe-a66b1.appspot.com";

chawkiForAll all;

void startCameraServer();
void setupLedFlash(int pin);

unsigned long previousMillis = 0;
const long interval = 10000;  // Interval in milliseconds 300000(5 minutes) ... 10000(10 sec)

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Use higher resolution and better JPEG quality if PSRAM is available
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // or FRAMESIZE_HD
    config.jpeg_quality = 10; // Lower number means higher quality
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    // Limit the frame size when PSRAM is not available
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // Camera initialization
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }

  all.initWiFi(ssid, password);
  all.connectToWiFi();
  Serial.println("Connected to WiFi!");

  // Initiate Firebase
  all.initFb(firebaseHost, databaseSecret);
  while (!Serial) {
    ; // Wait for Serial to be ready
  }

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  String url = "http://" + WiFi.localIP().toString();
  all.setFbString("/camera/url_streaming", url.c_str());
  Serial.println("' to connect");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    takeAndUploadPicture();
  }
  takeAndUploadPicture();

  // Do nothing. Everything is done in another task by the web server
  delay(10000);
}

void takeAndUploadPicture() {
  Serial.println("Taking picture...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Encode the image to base64
  size_t encodedSize = base64::encodeLength(fb->len);
  char* base64Buffer = new char[encodedSize];
  base64::encode(fb->buf, fb->len, base64Buffer);

  // Convert the encoded base64 buffer to a String
  String base64Image = String(base64Buffer);
  Serial.println(base64Image);

  // Upload the base64 encoded image to Firebase Realtime Database
  all.setFbString("/camera/base64_image", base64Image.c_str());
  Serial.println("Base64 image uploaded to Firebase Realtime Database");

  // Return the frame buffer to the camera library
  esp_camera_fb_return(fb);
}
