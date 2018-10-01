#ifndef __COLORSTATE_H_INCLUDED__
#define __COLORSTATE_H_INCLUDED__

class ColorState: public IEffect {

  void applyColor(int inR, int inG, int inB, int inW) {
    // when the light is off it should be off
    if (!stateOn) {
      inR = 0;
      inG = 0;
      inB = 0;
      inW = 0;
    }

    if (CONFIG_INVERT_LED_LOGIC) {
      inR = (255 - inR);
      inG = (255 - inG);
      inB = (255 - inB);
      inW = (255 - inW);
    }

    if (includeRgb) {
      analogWrite(CONFIG_PIN_RED, inR);
      analogWrite(CONFIG_PIN_GREEN, inG);
      analogWrite(CONFIG_PIN_BLUE, inB);
    }

    if (includeWhite) {
      analogWrite(CONFIG_PIN_WHITE, inW);
    }

#ifdef CONFIG_DEBUG
    Serial.print("Setting LEDs: {");
    if (includeRgb) {
      Serial.print("r: ");
      Serial.print(inR);
      Serial.print(" , g: ");
      Serial.print(inG);
      Serial.print(" , b: ");
      Serial.print(inB);
    }

    if (includeWhite) {
      if (includeRgb) {
        Serial.print(", ");
      }
      Serial.print("w: ");
      Serial.print(inW);
    }

    Serial.println("}");
#endif
  }

public:
  //TODO check which variables should be private
  const bool includeRgb = (CONFIG_STRIP == RGB) || (CONFIG_STRIP == RGBW);
  const bool includeWhite = (CONFIG_STRIP == BRIGHTNESS) || (CONFIG_STRIP == RGBW);

  byte red = 255;
  byte green = 255;
  byte blue = 255;
  byte white = 255;

  byte brightness = 255;

  bool stateOn = false;

  void setColor(int inR, int inG, int inB, int inW) {
    red = inR;
    green = inG;
    blue = inB;
    white = inW;
  }

  bool processJson(JsonObject& root) {
    if (root.containsKey("color")) {
      red = root["color"]["r"];
      green = root["color"]["g"];
      blue = root["color"]["b"];
    }

    if (root.containsKey("white_value")) {
      white = root["white_value"];
    }

    if (root.containsKey("brightness")) {
      brightness = root["brightness"];
    }

    return true;
  }

  void populateJson(JsonObject& root) {
    root["state"] = (stateOn) ? CONFIG_MQTT_PAYLOAD_ON : CONFIG_MQTT_PAYLOAD_OFF;
    if (includeRgb) {
      JsonObject& color = root.createNestedObject("color");
      color["r"] = red;
      color["g"] = green;
      color["b"] = blue;
    }

    if (includeWhite) {
      root["white_value"] = white;
    }

    root["brightness"] = brightness;
  }

  void update() { 
    // Update lights
    byte realRed = map(red, 0, 255, 0, brightness);
    byte realGreen = map(green, 0, 255, 0, brightness);
    byte realBlue = map(blue, 0, 255, 0, brightness);
    byte realWhite = map(white, 0, 255, 0, brightness);

    applyColor (realRed, realGreen, realBlue, realWhite);
  }
  void end() {}
};

#endif
