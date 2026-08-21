#ifndef TIME_H
#define TIME_H

#include <Wire.h>
#include "customFilesystem.h"

class StopWatch {

private:
  int64_t _startTime;
  int64_t _lastTime;
  bool _isRunning;
  int64_t _offset;
  bool _isContinuousMode;

public:
  StopWatch();
  ~StopWatch();

  void setOffset(int64_t offset);
  int64_t getSyncedMicros();
  void triggerStart(uint64_t time);
  void triggerFinish(CFs* fs_instance);
  void triggerFinishWithTimestamp(CFs* fs_instance, uint64_t timestamp);
  String draw();
  void setContinuousMode(bool continuousMode);
  bool isContinuousMode();
  bool reset();

  static String formatTime(int64_t ms);
};

#endif