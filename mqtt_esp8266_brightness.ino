// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h> 

#include <ESP8266WiFi.h>

// http://pubsubclient.knolleary.net/
#include <PubSubClient.h>

const int redPin = 5; // Use only red for brightness only
const int greenPin = 4;
const int bluePin = 2;

const char* ssid = "{WIFI-SSID}";
const char* password = "{WIFI-PASSWORD}";

const char* mqtt_server = "{MQTT-SERVER}";
const char* mqtt_username = "{MQTT-USERNAME}";
const char* mqtt_password = "{MQTT-PASSWORD}";

const char* client_id = "ESPBrightnessLED"; // Must be unique on the MQTT network

// Topics
const char* light_state_topic = "home/rgb1";
const char* light_set_topic = "home/rgb1/set";

const char* on_cmd = "ON";
const char* off_cmd = "OFF";

const int BUFFER_SIZE = JSON_OBJECT_SIZE(8);

// Maintained state for reporting to HA
byte red = 255;
// byte green = 0;
// byte blue = 0;
byte brightness = 255;

// Real values to write to the LEDs (ex. including brightness and state)
byte realRed = 0;
// byte realGreen = 0;
// byte realBlue = 0;

bool state_on = false;

// Globals for fade/transitions
bool startFade = false;
long lastLoop = 0;
int wait = 0;
bool inFade = false;
int loopCount = 0;
int stepR; //, stepG, stepB;
int prevR = 0;
// int prevG = 0;
// int prevB = 0;
int redVal = 0;
// int grnVal = 0;
// int bluVal = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(redPin, OUTPUT);
  // pinMode(greenPin, OUTPUT);
  // pinMode(bluePin, OUTPUT);

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

  if (state_on) {
    // Update lights
    realRed = map(red, 0, 255, 0, brightness);
    // realGreen = map(green, 0, 255, 0, brightness);
    // realBlue = map(blue, 0, 255, 0, brightness);
  }
  else {
    realRed = 0;
    // realGreen = 0;
    // realBlue = 0;
  }

  startFade = true;

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
      state_on = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      state_on = false;
    }
  }

  // if (root.containsKey("color")) {
  //   red = root["color"]["r"];
  //   green = root["color"]["g"];
  //   blue = root["color"]["b"];
  // }

  if (root.containsKey("brightness")) {
    brightness = root["brightness"];
  }

  if (root.containsKey("transition")) {
    wait = root["transition"];
  }
  else {
    wait = 0;
  }

  return true;
}

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (state_on) ? on_cmd : off_cmd;
  // JsonObject& color = root.createNestedObject("color");
  // color["r"] = red;
  // color["g"] = green;
  // color["b"] = blue;

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

void setLight(int inR) {
  analogWrite(redPin, inR);
  // analogWrite(greenPin, inG);
  // analogWrite(bluePin, inB);

  Serial.print("Setting light: ");
  Serial.println(inR);
  // Serial.println("Setting LEDs:");
  // Serial.print("r: ");
  // Serial.print(inR);
  // Serial.print(", g: ");
  // Serial.print(inG);
  // Serial.print(", b: ");
  // Serial.println(inB);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (startFade) {
    // If we don't want to fade, skip it.
    if (wait == 0) {
      setLight(realRed);
      prevR = realRed;
      // prevG = realGreen;
      // prevB = realBlue;

      redVal = realRed;
      // grnVal = realGreen;
      // bluVal = realBlue;

      startFade = false;
    }
    else {
      loopCount = 0;
      stepR = calculateStep(prevR, realRed);
      // stepG = calculateStep(prevG, realGreen); 
      // stepB = calculateStep(prevB, realBlue);

      inFade = true;
    }
  }

  if (inFade) {
    startFade = false;
    long now = millis();
    if (now - lastLoop > wait) {
      if (loopCount <= 1020) {
        lastLoop = now;
        
        redVal = calculateVal(stepR, redVal, loopCount);
        // grnVal = calculateVal(stepG, grnVal, loopCount);
        // bluVal = calculateVal(stepB, bluVal, loopCount);
        
        setLight(redVal); // Write current values to LED pins

        loopCount++;
      }
      else {
        // Update current values for next loop
        prevR = redVal; 
        // prevG = grnVal; 
        // prevB = bluVal;

        inFade = false;
      }
    }
  }
}

// From https://www.arduino.cc/en/Tutorial/ColorCrossfader
/* BELOW THIS LINE IS THE MATH -- YOU SHOULDN'T NEED TO CHANGE THIS FOR THE BASICS
* 
* The program works like this:
* Imagine a crossfade that moves the red LED from 0-10, 
*   the green from 0-5, and the blue from 10 to 7, in
*   ten steps.
*   We'd want to count the 10 steps and increase or 
*   decrease color values in evenly stepped increments.
*   Imagine a + indicates raising a value by 1, and a -
*   equals lowering it. Our 10 step fade would look like:
* 
*   1 2 3 4 5 6 7 8 9 10
* R + + + + + + + + + +
* G   +   +   +   +   +
* B     -     -     -
* 
* The red rises from 0 to 10 in ten steps, the green from 
* 0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.
* 
* In the real program, the color percentages are converted to 
* 0-255 values, and there are 1020 steps (255*4).
* 
* To figure out how big a step there should be between one up- or
* down-tick of one of the LED values, we call calculateStep(), 
* which calculates the absolute gap between the start and end values, 
* and then divides that gap by 1020 to determine the size of the step  
* between adjustments in the value.
*/
int calculateStep(int prevValue, int endValue) {
    int step = endValue - prevValue; // What's the overall gap?
    if (step) {                      // If its non-zero, 
        step = 1020/step;            //   divide by 1020
    }
    
    return step;
}

/* The next function is calculateVal. When the loop value, i,
*  reaches the step size appropriate for one of the
*  colors, it increases or decreases the value of that color by 1. 
*  (R, G, and B are each calculated separately.)
*/
int calculateVal(int step, int val, int i) {
    if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
        if (step > 0) {              //   increment the value if step is positive...
            val += 1;           
        } 
        else if (step < 0) {         //   ...or decrement it if step is negative
            val -= 1;
        } 
    }
    
    // Defensive driving: make sure val stays in the range 0-255
    if (val > 255) {
        val = 255;
    } 
    else if (val < 0) {
        val = 0;
    }
    
    return val;
}

/* crossFade() converts the percentage colors to a 
*  0-255 range, then loops 1020 times, checking to see if  
*  the value needs to be updated each time, then writing
*  the color values to the correct pins.
*/
// void crossFade(int color[3], int wait) {
//     // Convert to 0-255
//     // int R = (color[0] * 255) / 100;
//     // int G = (color[1] * 255) / 100;
//     // int B = (color[2] * 255) / 100;
    
//     int R = color[0];
//     int G = color[1];
//     int B = color[2];
    
//     // Spark.publish("crossFade", String(R) + "," + String(G) + "," + String(B));
    
//     int stepR = calculateStep(prevR, R);
//     int stepG = calculateStep(prevG, G); 
//     int stepB = calculateStep(prevB, B);

//     for (int i = 0; i <= 1020; i++) {
//         if (interruptFade) {
//           break;
//         }
        
//         redVal = calculateVal(stepR, redVal, i);
//         grnVal = calculateVal(stepG, grnVal, i);
//         bluVal = calculateVal(stepB, bluVal, i);
        
//         setColor(redVal, grnVal, bluVal); // Write current values to LED pins
        
//         delay(wait); // Pause for 'wait' milliseconds before resuming the loop
//     }
    
//     // Update current values for next loop
//     prevR = redVal; 
//     prevG = grnVal; 
//     prevB = bluVal;
//     delay(hold); // Pause for optional 'wait' milliseconds before resuming the loop
// }

