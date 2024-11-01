#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_err.h"
#include "esp_http_client.h"

const char *ssid = "RemoteRacer";   // Enter SSID here
const char *password = "12345678";  //Enter Password here

void connect_wifi() {

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(100);
  }
}

//WIFI EVENT HANDLERS
void WIFI_DISCONNECTION( WiFiEvent_t event ) {

  connect_wifi();
}

void add_wifi_event_handlers() {

  WiFi.onEvent( WIFI_DISCONNECTION, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED );
}

//SETUP CODE
void setup_wifi() {

  connect_wifi();

  add_wifi_event_handlers();
}