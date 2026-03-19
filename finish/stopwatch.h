#ifndef TIME_H
#define TIME_H

#include <Wire.h>
#include "fs.h"

class StopWatch {

private:
  unsigned long _startTime;
  unsigned long _lastTime;
  bool _isRunning;

public:
  StopWatch();
  ~StopWatch();

  void trigger(Fs* fs_instance);
  String draw();

  static String formatTime(unsigned long ms);
};

#endif