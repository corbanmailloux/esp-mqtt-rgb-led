#ifndef __FLASH_H_INCLUDED__
#define __FLASH_H_INCLUDED__

class Flash: public IEffect {
  const char* name = "flash";
  
  ColorState& state;

  bool running = false;
  bool start = false;
  unsigned long startTime = 0;
  int flashLength = 0;

  byte flashRed = 0;
  byte flashGreen = 0;
  byte flashBlue = 0;
  byte flashWhite = 0;
  byte flashBrightness = 0;

public:
  Flash(ColorState& s):
    state(s)
  {
  }

  virtual bool processJson(JsonObject& root) {
    if (root.containsKey(name) ||
       (root.containsKey("effect") && strcmp(root["effect"], name) == 0)) {

      if (root.containsKey(name)) {
        flashLength = (int)root[name] * 1000;
      }
      else {
        flashLength = CONFIG_DEFAULT_FLASH_LENGTH * 1000;
      }

      if (root.containsKey("brightness")) {
        flashBrightness = root["brightness"];
      }
      else {
        flashBrightness = state.brightness;
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

      // map the color brightness
      flashRed = map(flashRed, 0, 255, 0, flashBrightness);
      flashGreen = map(flashGreen, 0, 255, 0, flashBrightness);
      flashBlue = map(flashBlue, 0, 255, 0, flashBrightness);
      flashWhite = map(flashWhite, 0, 255, 0, flashBrightness);

      running = true;
      start = true;
      return true;
    }
    return false;
  }
  virtual void update() {
    if (running) {
      if (start) {
        start = false;
        startTime = millis();
      }

      if ((millis() - startTime) <= flashLength) {
        if ((millis() - startTime) % 1000 <= 500) {
          state.setColor(flashRed, flashGreen, flashBlue, flashWhite);
        }
        else {
          state.setColor(0, 0, 0, 0);
          // If you'd prefer the flashing to happen "on top of"
          // the current color, uncomment the next line.
          // setColor(realRed, realGreen, realBlue, realWhite);
        }
      }
      else {
        running = false;
        state.setColor(state.realRed, state.realGreen, state.realBlue, state.realWhite);
      }
    }
  }
  virtual void end() {
    Serial.println("stopping flash");
    running = false;
  }

  virtual bool isRunning() {
    return running;
  }
  virtual const char* getName() {
    return name;
  }
};

#endif
