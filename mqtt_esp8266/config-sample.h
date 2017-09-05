/*
 * This is a sample configuration file for the "mqtt_esp8266" light.
 *
 * Change the settings below and save the file as "config.h"
 * You can then upload the code using the Arduino IDE.
 */

// Define the strip type
enum strip { BRIGHTNESS // only one color
           , RGB // RGB LEDs
           , RGBW // RGB LEDs with an extra white LED per LED
           }; 
#define CONFIG_STRIP RGB

// Pins
// In case of BRIGHTNESS: only RED is used
// In case of RGB(W): red, green, blue(, white) is used
// All values need to be present, if they are not needed, just write any value,
// it will be ignored.
#define CONFIG_PIN_RED 0
#define CONFIG_PIN_GREEN 2 // only needed for RGB(W) strips
#define CONFIG_PIN_BLUE 3 // only needed for RGB(W) strips
#define CONFIG_PIN_WHITE 4 // only needed for RGBW strips

// WiFi
#define CONFIG_WIFI_SSID "{WIFI-SSID}"
#define CONFIG_WIFI_PASS "{WIFI-PASSWORD}"

// MQTT
#define CONFIG_MQTT_HOST "{MQTT-SERVER}"
#define CONFIG_MQTT_USER "{MQTT-USERNAME}"
#define CONFIG_MQTT_PASS "{MQTT-PASSWORD}"
#define CONFIG_MQTT_CLIENT_ID "ESP_LED" // Must be unique on the MQTT network

// MQTT Topics
#define CONFIG_MQTT_TOPIC_STATE "home/ESP_LED"
#define CONFIG_MQTT_TOPIC_SET "home/ESP_LED/set"

#define CONFIG_MQTT_PAYLOAD_ON "ON"
#define CONFIG_MQTT_PAYLOAD_OFF "OFF"

// Miscellaneous
// Default number of flashes if no value was given
#define CONFIG_DEFAULT_FLASH_LENGTH 2
// Number of seconds for one transition in colorfade mode
#define CONFIG_COLORFADE_TIME_SLOW 10
#define CONFIG_COLORFADE_TIME_FAST 3

// Reverse the LED logic
// false: 0 (off) - 255 (bright)
// true: 255 (off) - 0 (bright)
#define CONFIG_INVERT_LED_LOGIC false

// Enables Serial and print statements
#define CONFIG_DEBUG false
