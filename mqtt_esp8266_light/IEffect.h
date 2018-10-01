#ifndef __IEFFECT_H_INCLUDED__
#define __IEFFECT_H_INCLUDED__

class IEffect {
public:
  // process the json from the message and return true when activated
  virtual bool processJson(JsonObject& root) = 0;
  virtual void populateJson(JsonObject& root) = 0;

  virtual void update() = 0;
  virtual void end() = 0;

  virtual ~IEffect() {}
};

#endif
