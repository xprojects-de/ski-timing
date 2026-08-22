#include "stopwatch.h"

#define START_TIME_QUEUE_DEPTH 20

StopWatch::StopWatch() {
  _startTime = 0;
  _lastTime = 0;
  _isRunning = false;
  _offset = 0;
  _isContinuousMode = false;
  _startTimeQueue = xQueueCreate(START_TIME_QUEUE_DEPTH, sizeof(int64_t));
}

StopWatch::~StopWatch() {
  if (_startTimeQueue != NULL) {
    vQueueDelete(_startTimeQueue);
    _startTimeQueue = NULL;
  }
}

void StopWatch::setOffset(int64_t offset) {
  _offset = offset;
}

int64_t StopWatch::getSyncedMicros() {
  return (int64_t)((int64_t)esp_timer_get_time() + _offset);
}

void StopWatch::triggerStart(uint64_t time) {

  if (_isContinuousMode) {

    if (!_isRunning) {
      _startTime = (int64_t)time;
      _isRunning = true;
    }
  } else {

    if (_startTimeQueue == NULL) return;

    int64_t startTime = (int64_t)time;
    if (xQueueSend(_startTimeQueue, &startTime, 0) != pdTRUE) {
      Serial.println("[STOPWATCH] WARNING: Start time queue full, start ignored!");
    } else {
      Serial.printf("[STOPWATCH] Added start time to queue. Queue size: %d\n", uxQueueMessagesWaiting(_startTimeQueue));
    }
  }
}

void StopWatch::triggerFinish(CFs* fs_instance) {
  triggerFinishWithTimestamp(fs_instance, (uint64_t)getSyncedMicros());
}

void StopWatch::triggerFinishWithTimestamp(CFs* fs_instance, uint64_t timestamp) {

  // Convert the finish-local timestamp into the start device's time domain
  // by applying the sync offset (same transformation as getSyncedMicros()).
  // Without this, elapsed = finishLocal - startRemote = garbage when devices
  // were booted at different times.
  const int64_t syncedTimestamp = (int64_t)timestamp + _offset;

  if (_isContinuousMode) {

    if (!_isRunning) {

      _startTime = syncedTimestamp;
      _isRunning = true;

    } else {

      int64_t elapsed = syncedTimestamp - _startTime;

      _lastTime = elapsed / 1000;

      fs_instance->append(_lastTime);
    }

  } else {

    if (_startTimeQueue == NULL) return;

    int64_t startTime;
    if (xQueueReceive(_startTimeQueue, &startTime, 0) != pdTRUE) {
      Serial.println("[STOPWATCH] No start time in queue, finish ignored");
      return;
    }

    int64_t elapsed = syncedTimestamp - startTime;

    _lastTime = elapsed / 1000;

    fs_instance->append(_lastTime);

    Serial.printf("[STOPWATCH] Finish recorded: %lld ms. Queue size: %d\n", _lastTime, uxQueueMessagesWaiting(_startTimeQueue));
  }
}

String StopWatch::formatTime(int64_t ms) {

  if (ms < 0) ms = 0;

  // Use a static buffer to avoid heap allocation on every call
  static char buf[16];
  int seconds = (int)(ms / 1000);
  int hundredths = (int)((ms % 1000) / 10);
  snprintf(buf, sizeof(buf), "%d.%02d", seconds, hundredths);
  return String(buf);
}

String StopWatch::draw() {

  int64_t currentElapsed = 0;

  if (_isContinuousMode) {

    if (_isRunning) {
      int64_t currentTime = getSyncedMicros();
      if (currentTime > _startTime) {
        currentElapsed = (currentTime - _startTime) / 1000;
      }
    }
  } else {

    if (_startTimeQueue != NULL) {

      int64_t oldestStartTime;
      if (xQueuePeek(_startTimeQueue, &oldestStartTime, 0) == pdTRUE) {
        int64_t currentTime = getSyncedMicros();
        if (currentTime > oldestStartTime) {
          currentElapsed = (currentTime - oldestStartTime) / 1000;
        }
      }
    }
  }

  // Use a static buffer to avoid two String temporaries per call
  static char drawBuf[32];
  int lSec = (int)(_lastTime / 1000);
  int lHun = (int)((_lastTime % 1000) / 10);
  int cSec = (int)(currentElapsed / 1000);
  int cHun = (int)((currentElapsed % 1000) / 10);
  snprintf(drawBuf, sizeof(drawBuf), "%d.%02d / %d.%02d", lSec, lHun, cSec, cHun);
  return String(drawBuf);
}

void StopWatch::setContinuousMode(bool continuousMode) {
  _isContinuousMode = continuousMode;
}

bool StopWatch::isContinuousMode() {
  return _isContinuousMode;
}

bool StopWatch::reset() {
  _lastTime = 0;
  _isRunning = false;
  _startTime = 0;

  if (_startTimeQueue != NULL) {
    xQueueReset(_startTimeQueue);
  }

  return true;
}

int StopWatch::getQueueSize() {
  if (_startTimeQueue == NULL) return 0;
  return uxQueueMessagesWaiting(_startTimeQueue);
}
