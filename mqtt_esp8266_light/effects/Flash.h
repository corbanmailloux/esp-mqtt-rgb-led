#ifndef __FLASH_H_INCLUDED__
#define __FLASH_H_INCLUDED__

class Flash: public IEffect {
  const char* name = "flash";
  
  ColorState& state;

  bool running = false;
  unsigned long startTime = 0;
  int flashLength = 0;

  byte flashRed = 0;
  byte flashGreen = 0;
  byte flashBlue = 0;
  byte flashWhite = 0;
  byte flashBrightness = 0;

  byte oriRed = 0;
  byte oriGreen = 0;
  byte oriBlue = 0;
  byte oriWhite = 0;

public:
  Flash(ColorState& s):
    state(s)
  {
  }

  bool processJson(JsonObject& root) {
    if (root.containsKey(name) ||
       (root.containsKey("effect") && strcmp(root["effect"], name) == 0)) {

      if (root.containsKey(name)) {
        flashLength = (int)root[name] * 1000;
      }
      else {
        flashLength = CONFIG_DEFAULT_FLASH_LENGTH * 1000;
      }

      oriRed = state.red;
      oriGreen = state.green;
      oriBlue = state.blue;
      oriWhite = state.white;

      if (root.containsKey("brightness")) {
        state.brightness = root["brightness"];
      }

      if (root.containsKey("color")) {
        flashRed = root["color"]["r"];
        flashGreen = root["color"]["g"];
        flashBlue = root["color"]["b"];
      }
      else {
        flashRed = state.red;
        flashGreen = state.green;
        flashBlue = state.blue;
      }

      if (root.containsKey("white_value")) {
        flashWhite = root["white_value"];
      }
      else {
        flashWhite = state.white;
      }

      startTime = millis();
      running = true;
      return true;
    }
    return false;
  }
  void populateJson(JsonObject& root) {
    if (running) {
      root["effect"] = name;
    }
  }

  void update() {
    if (running) {
      if ((millis() - startTime) <= flashLength) {
        if ((millis() - startTime) % 1000 <= 500) {
          state.setColor(flashRed, flashGreen, flashBlue, flashWhite);
        }
        else {
          state.setColor(0, 0, 0, 0);
          // If you'd prefer the flashing to happen "on top of"
          // the current color, uncomment the next line.
          // state.setColor(oriRed, oriGreen, oriBlue, oriWhite);
        }
      }
      else {
        running = false;
        state.setColor(oriRed, oriGreen, oriBlue, oriWhite);
      }
    }
  }
  void end() {
#ifdef CONFIG_DEBUG
    Serial.println("stopping flash");
#endif
    running = false;
  }
};

#endif
