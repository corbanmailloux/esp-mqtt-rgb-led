#ifndef __COLORSTATE_H_INCLUDED__
#define __COLORSTATE_H_INCLUDED__

class ColorState {
public:
  //TODO check which variables should be private
  const bool includeRgb = (CONFIG_STRIP == RGB) || (CONFIG_STRIP == RGBW);
  const bool includeWhite = (CONFIG_STRIP == BRIGHTNESS) || (CONFIG_STRIP == RGBW);

  // Maintained state for reporting to HA
  byte red = 255;
  byte green = 255;
  byte blue = 255;
  byte white = 255;
  byte brightness = 255;

  // Real values to write to the LEDs (ex. including brightness and state)
  byte realRed = 0;
  byte realGreen = 0;
  byte realBlue = 0;
  byte realWhite = 0;

  bool stateOn = false;

  // Globals for fade/transitions
  bool startFade = false;
  unsigned long lastLoop = 0;
  int transitionTime = 0;
  bool inFade = false;
  int loopCount = 0;
  int stepR, stepG, stepB, stepW;
  int redVal, grnVal, bluVal, whtVal;

  void setColor(int inR, int inG, int inB, int inW) {
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

    if (CONFIG_DEBUG) {
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
    }
  }

  void update() { 
    if (startFade) {
      // If we don't want to fade, skip it.
      if (transitionTime == 0) {
        setColor(realRed, realGreen, realBlue, realWhite);

        redVal = realRed;
        grnVal = realGreen;
        bluVal = realBlue;
        whtVal = realWhite;

        startFade = false;
      }
      else {
        loopCount = 0;
        stepR = calculateStep(redVal, realRed);
        stepG = calculateStep(grnVal, realGreen);
        stepB = calculateStep(bluVal, realBlue);
        stepW = calculateStep(whtVal, realWhite);

        inFade = true;
      }
    }

    if (inFade) {
      startFade = false;
      unsigned long now = millis();
      if (now - lastLoop > transitionTime) {
        if (loopCount <= 1020) {
          lastLoop = now;

          redVal = calculateVal(stepR, redVal, loopCount);
          grnVal = calculateVal(stepG, grnVal, loopCount);
          bluVal = calculateVal(stepB, bluVal, loopCount);
          whtVal = calculateVal(stepW, whtVal, loopCount);

          setColor(redVal, grnVal, bluVal, whtVal); // Write current values to LED pins

          Serial.print("Loop count: ");
          Serial.println(loopCount);
          loopCount++;
        }
        else {
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
};

#endif
