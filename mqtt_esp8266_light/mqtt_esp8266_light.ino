/*
 * ESP8266 MQTT Lights for Home Assistant.
 *
 * Created DIY lights for Home Assistant using MQTT and JSON.
 * This project supports single-color, RGB, and RGBW lights.
 *
 * Copy the included `config-sample.h` file to `config.h` and update
 * accordingly for your setup.
 *
 * See https://github.com/corbanmailloux/esp-mqtt-rgb-led for more information.
 */

// Set configuration options for LED type, pins, WiFi, and MQTT in the following file:
#include "config.h"

// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>

// http://pubsubclient.knolleary.net/
#include <PubSubClient.h>

#include "ColorState.h"

#include "IEffect.h"
#include "effects/Flash.h"
#include "effects/ColorFade.h"

ColorState state;

const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

// effects
Flash flashEffect(state);
ColorFade colorFadeEffect(state);
const int effectsCount = 2;
IEffect *effects[] = {
  &flashEffect,
  &colorFadeEffect
};

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  if (state.includeRgb) {
    pinMode(CONFIG_PIN_RED, OUTPUT);
    pinMode(CONFIG_PIN_GREEN, OUTPUT);
    pinMode(CONFIG_PIN_BLUE, OUTPUT);
  }
  if (state.includeWhite) {
    pinMode(CONFIG_PIN_WHITE, OUTPUT);
  }

  // Set the BUILTIN_LED based on the CONFIG_BUILTIN_LED_MODE
  switch (CONFIG_BUILTIN_LED_MODE) {
    case 0:
      pinMode(BUILTIN_LED, OUTPUT);
      digitalWrite(BUILTIN_LED, LOW);
      break;
    case 1:
      pinMode(BUILTIN_LED, OUTPUT);
      digitalWrite(BUILTIN_LED, HIGH);
      break;
    default: // Other options (like -1) are ignored.
      break;
  }

  analogWriteRange(255);

  if (CONFIG_DEBUG) {
    Serial.begin(115200);
  }

  setup_wifi();
  client.setServer(CONFIG_MQTT_HOST, CONFIG_MQTT_PORT);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(CONFIG_WIFI_SSID);

  WiFi.mode(WIFI_STA); // Disable the built-in WiFi access point.
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

  /*
  SAMPLE PAYLOAD (BRIGHTNESS):
    {
      "brightness": 120,
      "flash": 2,
      "transition": 5,
      "state": "ON"
    }

  SAMPLE PAYLOAD (RGBW):
    {
      "brightness": 120,
      "color": {
        "r": 255,
        "g": 100,
        "b": 100
      },
      "white_value": 255,
      "flash": 2,
      "transition": 5,
      "state": "ON",
      "effect": "colorfade_fast"
    }
  */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJson(message)) {
    return;
  }

  if (state.stateOn) {
    // Update lights
    state.realRed = map(state.red, 0, 255, 0, state.brightness);
    state.realGreen = map(state.green, 0, 255, 0, state.brightness);
    state.realBlue = map(state.blue, 0, 255, 0, state.brightness);
    state.realWhite = map(state.white, 0, 255, 0, state.brightness);
  }
  else {
    state.realRed = 0;
    state.realGreen = 0;
    state.realBlue = 0;
    state.realWhite = 0;
  }

  state.startFade = true;
  state.inFade = false; // Kill the current fade

  sendState();
}

bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], CONFIG_MQTT_PAYLOAD_ON) == 0) {
      state.stateOn = true;
    }
    else if (strcmp(root["state"], CONFIG_MQTT_PAYLOAD_OFF) == 0) {
      state.stateOn = false;
    }
  }

  for (int i = 0; i < effectsCount; ++i) {
    if (effects[i]->processJson(root)) {
      return true; // terminate if an effect is activated
    }
  }

  // No effect
  // stop all active effects
  for (int i = 0; i < effectsCount; ++i) {
    effects[i]->end();
  }

  if (state.includeRgb && root.containsKey("color")) {
    state.red = root["color"]["r"];
    state.green = root["color"]["g"];
    state.blue = root["color"]["b"];
  }

  if (state.includeWhite && root.containsKey("white_value")) {
    state.white = root["white_value"];
  }

  if (root.containsKey("brightness")) {
    state.brightness = root["brightness"];
  }

  if (root.containsKey("transition")) {
    state.transitionTime = root["transition"];
  }
  else {
    state.transitionTime = 0;
  }

  return true;
}

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (state.stateOn) ? CONFIG_MQTT_PAYLOAD_ON : CONFIG_MQTT_PAYLOAD_OFF;
  if (state.includeRgb) {
    JsonObject& color = root.createNestedObject("color");
    color["r"] = state.red;
    color["g"] = state.green;
    color["b"] = state.blue;
  }

  root["brightness"] = state.brightness;

  if (state.includeWhite) {
    root["white_value"] = state.white;
  }

  root["effect"] = "null";
  for (int i = 0; i < effectsCount; ++i) {
    if (effects[i]->isRunning()) {
      root["effect"] = effects[i]->getName();
      break;
    }
  }

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(CONFIG_MQTT_TOPIC_STATE, buffer, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CONFIG_MQTT_CLIENT_ID, CONFIG_MQTT_USER, CONFIG_MQTT_PASS)) {
      Serial.println("connected");
      client.subscribe(CONFIG_MQTT_TOPIC_SET);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  // update effects
  for (int i = 0; i < effectsCount; ++i) {
    effects[i]->update();
  }

  state.update();
}
