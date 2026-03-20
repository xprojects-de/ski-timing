#ifndef TIME_H
#define TIME_H

#include <Wire.h>
#include "fs.h"

class StopWatch {

private:
  int64_t _startTime;
  int64_t _lastTime;
  bool _isRunning;
  int64_t _offset;

public:
  StopWatch();
  ~StopWatch();

  void setOffset(int64_t offset);
  int64_t getSyncedMicros();
  void triggerStart(uint64_t time);
  void triggerFinish(Fs* fs_instance);
  String draw();

  static String formatTime(int64_t ms);
};

#endif