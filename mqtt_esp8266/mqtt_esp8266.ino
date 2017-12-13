/*
 * ESP8266 MQTT Lights for Home Assistant.
 *
 * This file is a joined file for Brightness, RBG and RGBW lights.
 *
 * See https://github.com/corbanmailloux/esp-mqtt-rgb-led
 */

// Set configuration options for pins, WiFi, and MQTT in the following file:
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

const int txPin = BUILTIN_LED; // On-board blue LED

const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASS;

const char* mqtt_server = CONFIG_MQTT_HOST;
const char* mqtt_username = CONFIG_MQTT_USER;
const char* mqtt_password = CONFIG_MQTT_PASS;
const char* client_id = CONFIG_MQTT_CLIENT_ID;

// Topics
const char* light_state_topic = CONFIG_MQTT_TOPIC_STATE;
const char* light_set_topic = CONFIG_MQTT_TOPIC_SET;

const char* on_cmd = CONFIG_MQTT_PAYLOAD_ON;
const char* off_cmd = CONFIG_MQTT_PAYLOAD_OFF;

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
    pinMode(state.redPin, OUTPUT);
    pinMode(state.greenPin, OUTPUT);
    pinMode(state.bluePin, OUTPUT);
  }
  if (state.includeWhite) {
    pinMode(state.whitePin, OUTPUT);
  }

  pinMode(txPin, OUTPUT);
  digitalWrite(txPin, HIGH); // Turn off the on-board LED

  analogWriteRange(255);

  if (state.debug_mode) {
    Serial.begin(115200);
  }

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

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
    if (strcmp(root["state"], on_cmd) == 0) {
      state.stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
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

  root["state"] = (state.stateOn) ? on_cmd : off_cmd;
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

  client.publish(light_state_topic, buffer, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(light_set_topic);
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
