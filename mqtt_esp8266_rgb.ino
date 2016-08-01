// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h> 

#include <ESP8266WiFi.h>

// http://pubsubclient.knolleary.net/
#include <PubSubClient.h>

const int redPin = 5;
const int greenPin = 4;
const int bluePin = 2;

const char* ssid = "{WIFI-SSID}";
const char* password = "{WIFI-PASSWORD}";

const char* mqtt_server = "{MQTT-SERVER}";
const char* mqtt_username = "{MQTT-USERNAME}";
const char* mqtt_password = "{MQTT-PASSWORD}";

// Topics
const char* light_state_topic = "home/rgb1";
const char* light_set_topic = "home/rgb1/set";

const char* on_cmd = "ON";
const char* off_cmd = "OFF";

const int BUFFER_SIZE = JSON_OBJECT_SIZE(8);


byte red = 255;
byte green = 255;
byte blue = 255;

byte brightness = 255;

bool state_on = false;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  analogWriteRange(255);

  Serial.begin(115200);
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

  byte redOut;
  byte greenOut;
  byte blueOut;
  if (state_on) {
    // Update lights
    redOut = map(red, 0, 255, 0, brightness);
    greenOut = map(green, 0, 255, 0, brightness);
    blueOut = map(blue, 0, 255, 0, brightness);

    // redOut = map(red, 0, 255, brightness, 0);
    // greenOut = map(green, 0, 255, brightness, 0);
    // blueOut = map(blue, 0, 255, brightness, 0);
  }
  else {
    redOut = 0;
    greenOut = 0;
    blueOut = 0;
  }

  analogWrite(redPin, redOut);
  analogWrite(greenPin, greenOut);
  analogWrite(bluePin, blueOut);

  Serial.println("Setting LEDs:");
  Serial.print("r: ");
  Serial.print(redOut);
  Serial.print(", g: ");
  Serial.print(greenOut);
  Serial.print(", b: ");
  Serial.println(blueOut);

  sendState();
}

bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }

  /*
  SAMPLE PAYLOAD:
    {
      "brightness": 120,
      "color": {
        "r": 255,
        "g": 100,
        "b": 100
      },
      "transition": 5,
      "state": "ON"
    }
  */

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      state_on = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      state_on = false;
    }
  }

  if (root.containsKey("color")) {
    red = root["color"]["r"];
    green = root["color"]["g"];
    blue = root["color"]["b"];
  }

  if (root.containsKey("brightness")) {
    brightness = root["brightness"];
  }

  if (root.containsKey("transition")) {
    // TODO
  }

  return true;
}

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (state_on) ? on_cmd : off_cmd;
  JsonObject& color = root.createNestedObject("color");
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;

  root["brightness"] = brightness;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(light_state_topic, buffer, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_username, mqtt_password)) {
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
  
//  long now = millis();
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, 75, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    client.publish("outTopic", msg);
//  }
}
