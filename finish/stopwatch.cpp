#include "stopwatch.h"

StopWatch::StopWatch() {
  _startTime = 0;
  _lastTime = 0;
  _isRunning = false;
  _offset = 0;
  _isContinuousMode = false;
}

StopWatch::~StopWatch() {}

void StopWatch::setOffset(int64_t offset) {
  _offset = offset;
}

int64_t StopWatch::getSyncedMicros() {
  return (int64_t)((int64_t)esp_timer_get_time() + _offset);
}

void StopWatch::triggerStart(uint64_t time) {

  if (!_isRunning) {

    _startTime = (int64_t)time;
    _isRunning = true;
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

    if (!_isRunning) return;

    int64_t elapsed = syncedTimestamp - _startTime;

    _lastTime = elapsed / 1000;
    _isRunning = false;
    _startTime = 0;

    fs_instance->append(_lastTime);
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

  if (_isRunning) {
    int64_t currentTime = getSyncedMicros();
    if (currentTime > _startTime) {
      currentElapsed = (currentTime - _startTime) / 1000;
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

  return true;
}
