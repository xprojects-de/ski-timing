#ifndef TIMESYNC_H
#define TIMESYNC_H

#include <Wire.h>
#include <Arduino.h>
#include "stopwatch.h"

class Timesync {

private:
  int64_t _totalOffset = 0;
  int _timeSyncReceived = 0;
  int64_t _finalAverageOffset = 0;
  bool _isTimeSynced = false;
  uint64_t _lastRemoteTime = 0;

public:
  Timesync();
  ~Timesync();

  void syncTime(uint64_t remoteTime, StopWatch* stopWatch);
  bool isInit();
};

#endif