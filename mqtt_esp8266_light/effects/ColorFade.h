#ifndef __FADE_H_INCLUDED__
#define __FADE_H_INCLUDED__

class ColorFade: public IEffect {
  const char* nameSlow = "colorfade_slow";
  const char* nameFast = "colorfade_fast";

  ColorState& state;
  Transition& transition;

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
  ColorFade(ColorState& s, Transition& t):
    state(s),
    transition(t)
  {
  }

  bool processJson(JsonObject& root) {
    if (state.includeRgb && root.containsKey("effect") &&
      (strcmp(root["effect"], nameSlow) == 0 || strcmp(root["effect"], nameFast) == 0)) {
      running = true;
      currentColor = 0;
      if (strcmp(root["effect"], nameSlow) == 0) {
        transition.time = CONFIG_COLORFADE_TIME_SLOW; //TODO preserve original transition time for returning to other effect
      }
      else {
        transition.time = CONFIG_COLORFADE_TIME_FAST;
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
  void populateJson(JsonObject& root) {
    if (running) {
      root["effect"] = getName();
    }
  }

  void update() {
    if (running && !transition.running) {
      byte red = colors[currentColor][0];
      byte green = colors[currentColor][1];
      byte blue = colors[currentColor][2];
      byte white = colors[currentColor][3];

      currentColor = (currentColor + 1) % numColors;

      transition.start(red, green, blue, white);
    }
  }
  void end() {
#ifdef CONFIG_DEBUG
    Serial.println("stopping colorfade");
#endif
    running = false;
  }

  const char* getName() {
    if (transition.time == CONFIG_COLORFADE_TIME_SLOW) {
      return nameSlow;
    }
    return nameFast;
  }
};

#endif
