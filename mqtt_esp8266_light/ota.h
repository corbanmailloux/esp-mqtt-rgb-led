#ifndef ota_h
#define ota_h

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/**
 * @brief mDNS and OTA Constants
 * @{
 */
#define HOSTNAME OTA_HOSTNAME ///< Hostname. The setup function adds the Chip ID at the end.
/// @}


void setupOta() {

  delay(100);

  Serial.println("\r\n");
  Serial.print("Chip ID: 0x");
  Serial.println(ESP.getChipId(), HEX);

  // Set Hostname.
  String hostname(HOSTNAME);
  hostname += String("-OTA-");
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);

  // Print hostname.
  Serial.println("Hostname: " + hostname);

  // Start OTA server.
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
}

#endif
