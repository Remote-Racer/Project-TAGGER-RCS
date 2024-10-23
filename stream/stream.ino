#include <WiFi.h>
#include <ESPmDNS.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_client.h"

/*Put your SSID & Password*/
const char *ssid = "RemoteRacer";   // Enter SSID here
const char *password = "12345678";  //Enter Password here

const char *host = "XPS";

const char *URL = "http://XPS.local:3000/upload/player1";

static const char *TAG = "HTTP_CLIENT";

esp_http_client_handle_t client;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("[Esp32] Welcome");
  connectWifi(ssid, password);
  setupmDNS();

  initCamera();

  init_HTTP_JPEG_Stream();
}

void loop() {

  http_post_jpeg_image();
}

void connectWifi(const char *ssid, const char *pwd) {
  Serial.print("[Wifi] Connection found on: ");
  Serial.print(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
  Serial.print("\n[Wifi] Connecting to WiFi ..");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println("\n[Wifi] Wifi Connected");

  Serial.print("[Wifi] RRSI: ");
  Serial.println(WiFi.RSSI());
  IPAddress ip = WiFi.localIP();
  Serial.print("[Wifi] IP ADDRESS: ");
  Serial.println(ip);
}

void setupmDNS() {
  int totalTry = 5;

  while (!MDNS.begin(host) && totalTry > 0) {
    Serial.println(".");
    delay(1000);
    totalTry--;
  }

  Serial.println("[Wifi] mDNS responder started");
  Serial.print("[Wifi] You can now connect to: http://");
  Serial.print(host);
  Serial.println(".local");
}

void initCamera() {

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26;
  config.pin_sccb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.pin_xclk = 0;
  config.xclk_freq_hz = 16000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;  // Lower resolution like QVGA (320x240)
  config.jpeg_quality = 32;            // Lower image quality

  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();

  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
}

void init_HTTP_JPEG_Stream() {

  esp_http_client_config_t config = {
    .url = URL,  // Replace with your actual server URL
    .method = HTTP_METHOD_POST,
    .transport_type = HTTP_TRANSPORT_OVER_TCP
  };

  client = esp_http_client_init(&config);
}

void http_post_jpeg_image() {


  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Define boundary for multipart form data
  const char *boundary = "----ESP32Boundary";

  // Prepare multipart form body
  const char *image_part_header = "------ESP32Boundary\r\n"
                                  "Content-Disposition: form-data; name=\"frame\"; filename=\"stream.jpg\"\r\n"
                                  "Content-Type: image/jpeg\r\n\r\n";


  const char *end_boundary = "\r\n------ESP32Boundary--\r\n";

  // Read the JPEG image
  uint8_t *jpeg_data = fb->buf;
  int jpeg_size = fb->len;

  // Set headers
  char content_type[100];
  snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
  esp_http_client_set_header(client, "Content-Type", content_type);

  // Calculate total body length
  //int total_len = strlen(part1) + strlen(image_part_header) + jpeg_size + strlen(end_boundary);
  int total_len = strlen(image_part_header) + jpeg_size + strlen(end_boundary);

  char total_len_str[4];
  snprintf(total_len_str, 4, "%d", total_len);

  // Set content length
  esp_http_client_set_header(client, "Content-Length", total_len_str);
  //ESP_LOGI(TAG, "HTTP HEADERS %s", "SET");

  // Open the connection
  esp_http_client_open(client, total_len);
  //ESP_LOGI(TAG, "HTTP CLIENT %s", "OPENED");

  // Write the first part (text field)
  //esp_http_client_write(client, part1, strlen(part1));

  // Write the second part (image header)
  esp_http_client_write(client, image_part_header, strlen(image_part_header));
  //ESP_LOGI(TAG, "HTTP CLIENT WRITE %s", "IMAGE PART START BOUNDARY");

  // Write the JPEG image data
  esp_http_client_write(client, (const char *)jpeg_data, jpeg_size);
  //ESP_LOGI(TAG, "HTTP CLIENT WRITE %s", "IMAGE DATA");

  // Write the final boundary
  esp_http_client_write(client, end_boundary, strlen(end_boundary));
  //ESP_LOGI(TAG, "HTTP CLIENT WRITE %s", "END BOUNDARY");

  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    ///ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %" PRId64,
             //esp_http_client_get_status_code(client),
             //esp_http_client_get_content_length(client));
  } else {
    //ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
  }

  esp_camera_fb_return(fb);

  // Fetch and handle the server response
  /*
  esp_http_client_fetch_headers(client);
  int content_length = esp_http_client_get_content_length(client);
  if (content_length > 0) {
    char *buffer = (char *)malloc(content_length + 1);
    esp_http_client_read(client, buffer, content_length);
    buffer[content_length] = 0;
    ESP_LOGI(TAG, "Response: %s", buffer);
    free(buffer);
  }
  */

  // Clean up and close the client
  //esp_http_client_close(client);
  //esp_http_client_cleanup(client);
}