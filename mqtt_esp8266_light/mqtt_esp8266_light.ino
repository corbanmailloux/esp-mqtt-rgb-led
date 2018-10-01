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

#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined ENC28J60
#include <UIPEthernet.h>
#else
#include <Ethernet.h>
#endif

// http://pubsubclient.knolleary.net/
#include <PubSubClient.h>

#include "IEffect.h"
#include "ColorState.h"
#include "effects/Transition.h"
#include "effects/Flash.h"
#include "effects/ColorFade.h"

#ifdef CONFIG_PIN_BUTTON
// https://github.com/depuits/AButt
#include <AButt.h>

AButt button(CONFIG_PIN_BUTTON, true);
#endif

const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

ColorState state;

// effects
Transition transition(state);
Flash flash(state);
ColorFade colorFade(state, transition);
const int effectsCount = 4;
IEffect *effects[] = {
  &flash,
  &colorFade,
  &transition,
  &state // must be the last effect
};

#ifdef ESP8266
WiFiClient ethClient;
#else
EthernetClient ethClient;
#endif

PubSubClient client(ethClient);

#ifdef CONFIG_PIN_BUTTON
void clicked(unsigned short clicks) {
  if (CONFIG_BUTTON_NATIVE && clicks == 1) {
    //toggle light state
    state.stateOn = !state.stateOn;
    sendState();
  }

  if (CONFIG_BUTTON_MQTT) {
    char cstr[2];
    itoa(clicks, cstr, 10);
    client.publish(CONFIG_MQTT_TOPIC_BUTTON, cstr);
  }
}

void holdStart() {
  if (CONFIG_BUTTON_MQTT) {
    client.publish(CONFIG_MQTT_TOPIC_BUTTON, "hold", true);
  }
}
void holdEnd() {
  if (CONFIG_BUTTON_MQTT) {
    client.publish(CONFIG_MQTT_TOPIC_BUTTON, "release", true);
  }
}
#endif

void setup() {
  if (state.includeRgb) {
    pinMode(CONFIG_PIN_RED, OUTPUT);
    pinMode(CONFIG_PIN_GREEN, OUTPUT);
    pinMode(CONFIG_PIN_BLUE, OUTPUT);
  }
  if (state.includeWhite) {
    pinMode(CONFIG_PIN_WHITE, OUTPUT);
  }

#ifdef CONFIG_PIN_BUTTON
  if (CONFIG_PIN_BUTTON != -1) {
    pinMode(CONFIG_PIN_BUTTON, INPUT_PULLUP);

    button.onClick(clicked);
    button.onHold(holdStart, holdEnd);
  }
#endif

// BUILTIN_LED is only defined when using esp8266
#ifdef BUILTIN_LED
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
#endif

#ifdef ESP8266
  analogWriteRange(255);
#endif

#ifdef CONFIG_DEBUG
  Serial.begin(115200);
#endif

  setup_network();

  client.setServer(CONFIG_MQTT_HOST, CONFIG_MQTT_PORT);
  client.setCallback(callback);
}

void setup_network() {
  delay(10);

#ifdef ESP8266
  // We start by connecting to a WiFi network
#ifdef CONFIG_DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(CONFIG_WIFI_SSID);
#endif

  WiFi.mode(WIFI_STA); // Disable the built-in WiFi access point.
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

#ifdef CONFIG_DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif
#else
  // setup ethernet communication using DHCP
  if(Ethernet.begin(mac) == 0) 
  {
#ifdef CONFIG_DEBUG
    Serial.println("Ethernet configuration using DHCP failed");
#endif

    while(true); // TODO better error handling
  }
#endif
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
#ifdef CONFIG_DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
#endif

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

#ifdef CONFIG_DEBUG
  Serial.println(message);
#endif

  if (!processJson(message)) {
    return;
  }

  sendState();
}

bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
#ifdef CONFIG_DEBUG
    Serial.println("parseObject() failed");
#endif
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

  // should effect be able to run when the state is off?
  for (int i = 0; i < effectsCount; ++i) {
    if (effects[i]->processJson(root)) {
      //we must make sure no other effects are still running
      for (int j = 0; j < effectsCount; ++j) {
        if (i != j) { // we do not want to stop the effect that just started
          effects[j]->end();
        }
      }
      
      return true; // terminate if an effect is activated
    }
  }

#ifdef CONFIG_DEBUG
  Serial.println("no effect applied");
#endif
  return false; // something was wrong, the default effect should've been activated
}

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  for (int i = 0; i < effectsCount; ++i) {
    effects[i]->populateJson(root);
  }

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

#ifdef CONFIG_DEBUG
  Serial.println("sending state:");
  Serial.println(buffer);
#endif
  client.publish(CONFIG_MQTT_TOPIC_STATE, buffer, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
#ifdef CONFIG_DEBUG
    Serial.print("Attempting MQTT connection...");
#endif
    // Attempt to connect
    if (client.connect(CONFIG_MQTT_CLIENT_ID, CONFIG_MQTT_USER, CONFIG_MQTT_PASS)) {  
#ifdef CONFIG_DEBUG
      Serial.println("connected");
#endif
      client.subscribe(CONFIG_MQTT_TOPIC_SET);
    } else {
#ifdef CONFIG_DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
#endif
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

#ifdef CONFIG_PIN_BUTTON
  button.update();
#endif

  // update effects
  for (int i = 0; i < effectsCount; ++i) {
    effects[i]->update();
  }
}
