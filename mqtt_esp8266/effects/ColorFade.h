#ifndef __FADE_H_INCLUDED__
#define __FADE_H_INCLUDED__

class ColorFade: public IEffect {
  const char* nameSlow = "colorfade_slow";
  const char* nameFast = "colorfade_fast";

  ColorState& state;

  bool running = false;
  int currentColor = 0;
  // {red, grn, blu, wht}
  const int numColors = 7;
  const byte colors[7][4] = {
    {255, 0, 0, 0},
    {0, 255, 0, 0},
    {0, 0, 255, 0},
    {255, 80, 0, 0},
    {163, 0, 255, 0},
    {0, 255, 255, 0},
    {255, 255, 0, 0}
  };

public:
  ColorFade(ColorState s):
    state(s)
  {
  }

  virtual bool processJson(JsonObject& root) {
    if (state.includeRgb && root.containsKey("effect") &&
      (strcmp(root["effect"], nameSlow) == 0 || strcmp(root["effect"], nameFast) == 0)) {
      //TODO disable flash: flash = false;
      running = true;
      currentColor = 0;
      if (strcmp(root["effect"], nameSlow) == 0) {
        state.transitionTime = CONFIG_COLORFADE_TIME_SLOW; //TODO preserve original transition time for returning to other effect
      }
      else {
        state.transitionTime = CONFIG_COLORFADE_TIME_FAST;
      }
      return true;
    }
    else if (running && !root.containsKey("color") && root.containsKey("brightness")) {
      // Adjust brightness during colorfade
      // (will be applied when fading to the next color)
      state.brightness = root["brightness"];
      return true;
    }

    return false;
  }
  virtual void update() {
    if (state.includeRgb && running && !state.inFade) {
      state.realRed = map(colors[currentColor][0], 0, 255, 0, state.brightness);
      state.realGreen = map(colors[currentColor][1], 0, 255, 0, state.brightness);
      state.realBlue = map(colors[currentColor][2], 0, 255, 0, state.brightness);
      state.realWhite = map(colors[currentColor][3], 0, 255, 0, state.brightness);
      currentColor = (currentColor + 1) % numColors;
      state.startFade = true;
    }
  }
  virtual void end() {
    running = false;
  }

  virtual bool isRunning() {
    return running;
  }
  virtual const char* getName() {
    if (state.transitionTime == CONFIG_COLORFADE_TIME_SLOW) {
      return nameSlow;
    }
    return nameFast;
  }
};

#endif
