/*
 * This is a sample configuration file for the "mqtt_esp8266_rgb" light.
 *
 * Change the settings below and save the file as "config.h"
 * You can then upload the code using the Arduino IDE.
 */

// Pins
#define CONFIG_PIN_RED 0
#define CONFIG_PIN_GREEN 2
#define CONFIG_PIN_BLUE 3

// WiFi
#define CONFIG_WIFI_SSID "{WIFI-SSID}"
#define CONFIG_WIFI_PASS "{WIFI-PASSWORD}"

// MQTT
#define CONFIG_MQTT_HOST "{MQTT-SERVER}"
#define CONFIG_MQTT_USER "{MQTT-USERNAME}"
#define CONFIG_MQTT_PASS "{MQTT-PASSWORD}"

#define CONFIG_MQTT_CLIENT_ID "ESPRGBLED" // Must be unique on the MQTT network

// MQTT Topics
#define CONFIG_MQTT_TOPIC_STATE "home/rgb1"
#define CONFIG_MQTT_TOPIC_SET "home/rgb1/set"

#define CONFIG_MQTT_PAYLOAD_ON "ON"
#define CONFIG_MQTT_PAYLOAD_OFF "OFF"
