/*
 * This is a sample configuration file for the "mqtt_esp8266" light.
 *
 * Change the settings below and save the file as "config.h"
 * You can then upload the code using the Arduino IDE.
 */

// Module type
// Uncomment the line with the correct module name
#define ESP8266
//#define ENC28J60
//#define W5100

// Network mac address, not used by ESP8266, change to something unique within your network
#ifndef ESP8266
uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
#endif

// Leave this here. These are the choices for CONFIG_STRIP below.
enum strip {
  BRIGHTNESS, // only one color/only white
  RGB,        // RGB LEDs
  RGBW        // RGB LEDs with an extra white LED per LED
};

#define CONFIG_STRIP RGB // Choose one of the options from above.

// Pins
// In case of BRIGHTNESS: only WHITE is used
// In case of RGB(W): red, green, blue(, white) is used
// All values need to be present, if they are not needed, set to -1,
// it will be ignored.
#define CONFIG_PIN_RED   0  // For RGB(W)
#define CONFIG_PIN_GREEN 2  // For RGB(W)
#define CONFIG_PIN_BLUE  3  // For RGB(W)
#define CONFIG_PIN_WHITE -1 // For BRIGHTNESS and RGBW

#define CONFIG_PIN_BUTTON -1 // change to actual pin number of the button or comment if there is no button

// WiFi
#define CONFIG_WIFI_SSID "{WIFI-SSID}"
#define CONFIG_WIFI_PASS "{WIFI-PASSWORD}"

// MQTT
#define CONFIG_MQTT_HOST "{MQTT-SERVER}"
#define CONFIG_MQTT_PORT 1883 // Usually 1883
#define CONFIG_MQTT_USER "{MQTT-USERNAME}"
#define CONFIG_MQTT_PASS "{MQTT-PASSWORD}"
#define CONFIG_MQTT_CLIENT_ID "ESP_LED" // Must be unique on the MQTT network

// MQTT Topics
#define CONFIG_MQTT_TOPIC_STATE "home/ESP_LED"
#define CONFIG_MQTT_TOPIC_SET "home/ESP_LED/set"
#define CONFIG_MQTT_TOPIC_BUTTON "home/ESP_LED/button"

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

// Set the mode for the built-in LED on some boards.
// -1 = Do nothing. Leave the pin in its default state.
//  0 = Explicitly set the BUILTIN_LED to LOW.
//  1 = Explicitly set the BUILTIN_LED to HIGH. (Off for Wemos D1 Mini)
#define CONFIG_BUILTIN_LED_MODE -1

// Do the native defined actions of the button. These actions are preformed
//  on the esp module itself without communicating with home assistent.
#define CONFIG_BUTTON_NATIVE true
// Send button changes to the specified MQTT topic
#define CONFIG_BUTTON_MQTT true

// Enables Serial and print statements, comment out to disable debug logs
#define CONFIG_DEBUG
