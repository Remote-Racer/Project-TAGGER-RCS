#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_camera.h"
#include "esp_err.h"
#include "esp_http_client.h"

#include "basic_actuation.h"

const char *host = "XPS";

void MDNS_resolve_host() {
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

//CONTROL STREAM SECTION
//const char *CONTROL_STREAM_URL = "http://XPS.local:3000/player/p1/control";
const char *CONTROL_STREAM_URL = "http://192.168.183.89:3000/player/p1/control";

static char response_buffer[128];
static int response_len = 0;

static double x = 0;
static double y = 0;

esp_err_t HTTP_control_stream_handler(esp_http_client_event_t *evt) {

  switch (evt->event_id) {

    case HTTP_EVENT_ON_DATA:

      if (!esp_http_client_is_chunked_response(evt->client)) {
        // Copy the response data into our buffer
        strncpy(response_buffer, (char *)evt->data, evt->data_len);
        response_len = evt->data_len;
      }
      break;
    case HTTP_EVENT_ON_FINISH:

      actuate( x, y );
      break;

    case HTTP_EVENT_ERROR:

      Serial.print("HTTP ERROR DETECTED");

      //MDNS_resolve_host();
      break;
    default: break;
  }

  return ESP_OK;
}

static esp_http_client_config_t CONTROL_STREAM_CONFIG = {
  .url = CONTROL_STREAM_URL,
  .method = HTTP_METHOD_GET,
  .event_handler = HTTP_control_stream_handler,
  .transport_type = HTTP_TRANSPORT_OVER_TCP
};

void GET_CONTROL_STREAM() {

  esp_http_client_handle_t client = esp_http_client_init(&CONTROL_STREAM_CONFIG);

  // Set headers
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_header(client, "Accept", "application/json");

  esp_err_t err = esp_http_client_perform(client);

  if (err != ESP_OK) {

    return;
  }

  if( esp_http_client_get_status_code( client ) != 200 ) {

    return;
  }

  JsonDocument doc;

  DeserializationError json_parse_error = deserializeJson(doc, response_buffer);

  if( json_parse_error ) {

    esp_http_client_cleanup( client );
    return;
  }

  x = doc["x"];
  y = doc["y"];

  Serial.println( response_buffer );
  esp_http_client_cleanup( client );
}


//CAMERA STREAM SECTION
//const char *CAMERA_STREAM_URL = "http://XPS.local:3000/upload/p1/stream";
const char *CAMERA_STREAM_URL = "http://192.168.183.89:3000/upload/p1/stream";

static esp_http_client_config_t CAMERA_STREAM_CONFIG = {
  .url = CAMERA_STREAM_URL,
  .method = HTTP_METHOD_POST,
  .transport_type = HTTP_TRANSPORT_OVER_TCP
};

void POST_CAMERA_STREAM() {

  //Get camera's current frame buffer
  camera_fb_t *fb = esp_camera_fb_get();

  //Check if the camera has prepared a frame buffer
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  //Create http client for communication to server at endpoint
  esp_http_client_handle_t client = esp_http_client_init(&CAMERA_STREAM_CONFIG);

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
  int total_len = strlen(image_part_header) + jpeg_size + strlen(end_boundary);

  char total_len_str[4];
  snprintf(total_len_str, 4, "%d", total_len);

  // Set content length
  esp_http_client_set_header(client, "Content-Length", total_len_str);

  // Open the connection
  esp_http_client_open(client, total_len);

  // Write the image header
  esp_http_client_write(client, image_part_header, strlen(image_part_header));

  // Write the JPEG image data
  esp_http_client_write(client, (const char *)jpeg_data, jpeg_size);

  // Write the final boundary
  esp_http_client_write(client, end_boundary, strlen(end_boundary));

  esp_err_t err = esp_http_client_perform(client);

  //Free memory reserved for camera frame buffer once more
  esp_camera_fb_return(fb);

  // Clean up and close the client
  esp_http_client_cleanup(client);
}