#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <Wire.h>
#include "customFilesystem.h"
#include <freertos/queue.h>

class StopWatch {

private:
  int64_t _lastTime;
  int64_t _offset;
  bool _isContinuousMode;
  QueueHandle_t _startTimeQueue;
  uint64_t _lastFinishTimestamp;

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
  int getQueueSize();
  bool discardOldestStart();

  static String formatTime(int64_t ms);
};

#endif