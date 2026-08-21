#ifndef TIMESYNC_H
#define TIMESYNC_H

#include <Wire.h>
#include <Arduino.h>
#include "stopwatch.h"

#define TIMESYNC_SAMPLES 15

class Timesync {

private:
  int64_t _offsetSamples[TIMESYNC_SAMPLES];
  int _timeSyncReceived = 0;
  int64_t _finalMedianOffset = 0;
  bool _isTimeSynced = false;
  uint64_t _lastRemoteTime = 0;

  int64_t calculateMedian();

public:
  Timesync();
  ~Timesync();

  void syncTime(uint64_t remoteTime, StopWatch* stopWatch);
  bool isInit();
};


#endif