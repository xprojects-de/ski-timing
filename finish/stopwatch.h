#ifndef TIME_H
#define TIME_H

#include <Wire.h>
#include "fs.h"

class StopWatch {

private:
  uint64_t _startTime;
  uint64_t _lastTime;
  bool _isRunning;
  uint64_t _offset;

public:
  StopWatch();
  ~StopWatch();

  void setOffset(uint64_t offset);
  uint64_t getSyncedMicros();
  void triggerStart(uint64_t time);
  void triggerFinish(Fs* fs_instance);
  String draw();

  static String formatTime(unsigned long ms);
};

#endif