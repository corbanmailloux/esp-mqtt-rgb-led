/*
* ESP8266 MQTT Lights for Home Assistant.
* See https://github.com/corbanmailloux/esp-mqtt-rgb-led
* Last edit 2016-08-08 - chrom3
*/

#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <PubSubClient.h>       // http://pubsubclient.knolleary.net/

const int redPin                = D1;   // change this to the correct pin
const int greenPin              = D2;   // change this to the correct pin
const int bluePin               = D3;   // change this to the correct pin

const char* ssid                = "wifi ssid";
const char* password            = "wifi password";

const char* mqtt_server         = "mqtt server ip";
const char* mqtt_username       = "mqtt username";
const char* mqtt_password       = "mqtt password";

const char* client_id           = "ESPRGBLED";  // Must be unique on the MQTT network

// Topics
const char* light_state_topic   = "home/rgb1";
const char* light_set_topic     = "home/rgb1/set";

const char* on_cmd              = "ON";
const char* off_cmd             = "OFF";

const int BUFFER_SIZE           = JSON_OBJECT_SIZE(8);

byte red                        = 255;          // Maintained state for reporting to HA
byte green                      = 255;
byte blue                       = 255;
byte brightness                 = 255;

byte realRed                    = 0;            // Real values to write to the LEDs (ex. including brightness and state)
byte realGreen                  = 0;
byte realBlue                   = 0;

bool state_on                   = false;

bool startFade                  = false;        // Globals for fade/transitions
unsigned long lastLoop          = 0;
int  transition_time            = 0;            // transition time
bool inFade                     = false;
int  loopCount                  = 0;
int  stepR, stepG, stepB;
int  prevR, prevG, prevB;
int  redVal, grnVal, bluVal;

WiFiClient espClient;
PubSubClient client(espClient);

bool processJson(char* message)
{
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    
    JsonObject& root = jsonBuffer.parseObject(message);
    
    if (!root.success())
    {
        Serial.println("parseObject() failed");
        return false;
    }

    if (root.containsKey("state"))
    {
        if (strcmp(root["state"], on_cmd) == 0)
        {
            state_on = true;
        }
        else if (strcmp(root["state"], off_cmd) == 0)
        {
            state_on = false;
        }
    }
    
    if (root.containsKey("color"))
    {
        red     = root["color"]["r"];
        green   = root["color"]["g"];
        blue    = root["color"]["b"];
    }
    
    if (root.containsKey("brightness"))
    {
        brightness = root["brightness"];
    }
    
    if (root.containsKey("transition"))
    {
        transition_time = root["transition"];
    }
    else
    {
        transition_time = 0;
    }
    
    return true;
}

void setup_wifi()
{
    delay(10);
    Serial.println();
    
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);         // We start by connecting to a WiFi network
    
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println( WiFi.localIP() );
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

void sendState()
{
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

void callback(char* topic, byte* payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    
    char message[length + 1];
    for (int i = 0; i < length; i++)
    {
        message[i] = (char)payload[i];
    }
    message[length] = '\0';
    Serial.println(message);
    
    if (!processJson(message))
    {
        return;
    }
    
    if (state_on)                           // Update lights
    {
        realRed     = map(red,   0, 255, 0, brightness);
        realGreen   = map(green, 0, 255, 0, brightness);
        realBlue    = map(blue,  0, 255, 0, brightness);
    }
    else
    {
        realRed     = 0;
        realGreen   = 0;
        realBlue    = 0;
    }
    
    startFade       = true;
    inFade          = false;
    
    sendState();
}

void reconnect()
{
    while (!client.connected())                                             // Loop until we're reconnected
    {
        Serial.print("Attempting MQTT connection...");
        
        if (client.connect(client_id, mqtt_username, mqtt_password))        // Attempt to connect
        {
            Serial.println("connected");
            client.subscribe(light_set_topic);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" trying again in 5 seconds.");
            
            delay(5000);                                                    // Wait 5 seconds before retrying
        }
    }
}

void setColor(int inR, int inG, int inB)
{
    analogWrite(redPin,     inR);
    analogWrite(greenPin,   inG);
    analogWrite(bluePin,    inB);
    /*
    Serial.print("Setting LEDs: R: ");
    Serial.print(inR);
    Serial.print(", G: ");
    Serial.print(inG);
    Serial.print(", B: ");
    Serial.println(inB);
    */
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
int calculateStep(int prevValue, int endValue)
{
    int step = endValue - prevValue;        // What's the overall gap?
    if (step)                               // If its non-zero, 
    {
        step /= 1020;                       //   divide by 1020
    }
    
    return step;
}

/* The next function is calculateVal. When the loop value, i,
*  reaches the step size appropriate for one of the
*  colors, it increases or decreases the value of that color by 1. 
*  (R, G, and B are each calculated separately.)
*/
int calculateVal(int step, int val, int i)
{
    if ((step) && i % step == 0)    // If step is non-zero and its time to change a value,
    {
        if (step > 0)               //   increment the value if step is positive...
        {
            val += 1;           
        } 
        else if (step < 0)          //   ...or decrement it if step is negative
        {
            val -= 1;
        }
    }
    
    val = constrain(val, 0, 255);   // Defensive driving: make sure val stays in the range 0-255
    
    return val;
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
    
    if (startFade)
    {
        if (transition_time == 0)    // If we don't want to fade, skip it.
        {
            setColor(realRed, realGreen, realBlue);
            prevR = realRed;
            prevG = realGreen;
            prevB = realBlue;
            
            redVal = realRed;
            grnVal = realGreen;
            bluVal = realBlue;
            
            startFade = false;
        }
        else
        {
            loopCount = 0;
            
            stepR = calculateStep(redVal, realRed);
            stepG = calculateStep(grnVal, realGreen);
            stepB = calculateStep(bluVal, realBlue);
            
            inFade = true;
        }
    }
    
    if (inFade)
    {
        startFade = false;
        unsigned long now = millis();
        if (now - lastLoop > transition_time)
        {
            if (loopCount <= 1020)
            {
                lastLoop = now;
                
                redVal = calculateVal(stepR, redVal, loopCount);
                grnVal = calculateVal(stepG, grnVal, loopCount);
                bluVal = calculateVal(stepB, bluVal, loopCount);
                
                setColor(redVal, grnVal, bluVal);       // Write current values to LED pins

                /*
                Serial.print("Loop count: ");
                Serial.println(loopCount);
                */
                loopCount++;
            }
            else                                        // Update current values for next loop
            {
                prevR = redVal;
                prevG = grnVal; 
                prevB = bluVal;
                
                inFade = false;
            }
        }
    }
}

void setup()
{
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    
    analogWriteRange(255);
    
    Serial.begin(115200);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}
