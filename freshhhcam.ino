#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "base64.h"

// ==== WIFI ====
const char* ssid = "ssid";
const char* password = "password";

// ==== SERVER URL (UPDATED) ====
String serverURL = "http://server ip and port/upload";

// =======================
// BUTTON & FLASH
// =======================
#define BUTTON_PIN 13
#define FLASH_LED_PIN 4

// =======================
// ESP32-CAM (AI Thinker Pins)
// =======================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM     25
#define HREF_GPIO_NUM      23
#define PCLK_GPIO_NUM      22


// =======================
// CAMERA SETUP
// =======================
void startCamera() {
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

  config.frame_size = FRAMESIZE_UXGA;  // 1600x1200 for OCR
  config.jpeg_quality = 8;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("❌ Camera init failed!");
    return;
  }

  Serial.println("📸 Camera ready!");
}


// =======================================================
// CAPTURE + SEND (DOUBLE CAPTURE FIX)
// =======================================================
void captureAndSend() {
  Serial.println("\n--- TAKING FRESH PHOTO ---");

  // Flash ON
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(200);

  // 1️⃣ Throw away OLD frame (always old)
  camera_fb_t *oldfb = esp_camera_fb_get();
  if (oldfb) esp_camera_fb_return(oldfb);

  delay(120);

  // 2️⃣ Capture fresh NEW frame
  camera_fb_t *fb = esp_camera_fb_get();

  // Flash OFF
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!fb) {
    Serial.println("❌ Failed to capture image!");
    return;
  }

  Serial.println("✔ Fresh image captured!");

  // Encode to Base64
  String imgBase64 = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  // Send to server
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"image\":\"" + imgBase64 + "\"}";

  int code = http.POST(body);

  if (code > 0) {
    Serial.println("✔ Server Response:");
    Serial.println(http.getString());
  } else {
    Serial.print("❌ Upload Error: ");
    Serial.println(code);
  }

  http.end();
}


// =======================
// SETUP
// =======================
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  Serial.println("Connecting WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }

  Serial.println("\n✔ WiFi Connected!");
  Serial.println(WiFi.localIP());

  startCamera();
}


// =======================
// LOOP
// =======================
void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    delay(50);  // debounce
    captureAndSend();

    while (digitalRead(BUTTON_PIN) == HIGH);
  }
}
