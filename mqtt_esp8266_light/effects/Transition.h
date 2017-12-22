#ifndef __TRANSITION_H_INCLUDED__
#define __TRANSITION_H_INCLUDED__

class Transition: public IEffect {
  ColorState& state;

  unsigned long lastLoop = 0;
  int loopCount = 0;
  int stepR, stepG, stepB, stepW;

public:
  bool running = false;
  int time = 0;

  Transition(ColorState& s):
    state(s)
  {
  }

  bool processJson(JsonObject& root) {
    if (root.containsKey("transition")) {
      byte 
        red = 0, 
        grn = 0, 
        blu = 0, 
        wht = 0;

      if (root.containsKey("color")) {
        red = root["color"]["r"];
        grn = root["color"]["g"];
        blu = root["color"]["b"];
      }

      if (root.containsKey("white_value")) {
        wht = root["white_value"];
      }

      // should the brighness also be transitioned?
      if (root.containsKey("brightness")) {
        state.brightness = root["brightness"];
      }

      time = root["transition"];

      start (red, grn, blu, wht);

      return true;
    }
    return false;
  }

  void start(int inR, int inG, int inB, int inW) {
      // If we don't want to fade, skip it.
      if (time == 0) {
        state.setColor(inR, inG, inB, inW);
      }
      else {
        loopCount = 0;
        stepR = calculateStep(state.red, inR);
        stepG = calculateStep(state.green, inG);
        stepB = calculateStep(state.blue, inB);
        stepW = calculateStep(state.white, inW);

        running = true;
      }
  }

  void populateJson(JsonObject& root) {
  }

  void update() { 
    if (running) {
      unsigned long now = millis();
      if (now - lastLoop > time) {
        if (loopCount <= 1020) {
          lastLoop = now;

          state.red = calculateVal(stepR, state.red, loopCount);
          state.green = calculateVal(stepG, state.green, loopCount);
          state.blue = calculateVal(stepB, state.blue, loopCount);
          state.white = calculateVal(stepW, state.white, loopCount);

          //Serial.print("Loop count: ");
          //Serial.println(loopCount);
          loopCount++;
        }
        else {
          running = false;
        }
      }
    }
  }

  void end() {
    running = false;
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
};

#endif
