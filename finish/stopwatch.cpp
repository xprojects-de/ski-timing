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

  if (_isContinuousMode) {

    if (!_isRunning) {

      _startTime = (int64_t)timestamp;
      _isRunning = true;

    } else {

      int64_t elapsed = (int64_t)((int64_t)timestamp - _startTime);

      _lastTime = (int64_t)(elapsed / 1000);

      fs_instance->append(_lastTime);
    }

  } else {

    if (!_isRunning) return;

    int64_t elapsed = (int64_t)((int64_t)timestamp - _startTime);

    _lastTime = (int64_t)(elapsed / 1000);
    _isRunning = false;
    _startTime = 0;

    fs_instance->append(_lastTime);
  }
}

String StopWatch::formatTime(int64_t ms) {

  if (ms < 0) ms = 0;

  int seconds = ms / 1000;
  int hundredths = (ms % 1000) / 10;
  String res = String(seconds) + ".";
  if (hundredths < 10) res += "0";
  res += String(hundredths);

  return res;
}

String StopWatch::draw() {

  int64_t currentElapsed = 0;

  if (_isRunning) {
    int64_t currentTime = getSyncedMicros();
    if (currentTime > _startTime) {
      currentElapsed = (currentTime - _startTime) / 1000;
    } else {
      currentElapsed = 0;
    }
  }

  return formatTime(_lastTime) + " / " + formatTime(currentElapsed);
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
