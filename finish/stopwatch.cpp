#include "stopwatch.h"

// Konstruktor: Initialisiert die Werte
StopWatch::StopWatch() {
  _startTime = 0;
  _lastTime = 0;
  _isRunning = false;
  _offset = 0;
}

StopWatch::~StopWatch() {}

void StopWatch::setOffset(uint64_t offset) {
  _offset = offset;
}

uint64_t StopWatch::getSyncedMicros() {
  return (uint64_t)((int64_t)micros() + _offset);
}

void StopWatch::triggerStart(uint64_t time) {

  if (!_isRunning) {

   _startTime = time;
    _isRunning = true;
  }
}

void StopWatch::triggerFinish(Fs* fs_instance) {

  if (!_isRunning) return;

  unsigned long currentTime = getSyncedMicros();
  unsigned long elapsed = currentTime - _startTime;

  _lastTime = elapsed / 1000;
  _isRunning = false;
  _startTime = 0;

  fs_instance->append(_lastTime);
}

String StopWatch::formatTime(unsigned long ms) {

  if (ms < 0) ms = 0;

  int seconds = ms / 1000;
  int hundredths = (ms % 1000) / 10;
  String res = String(seconds) + ".";
  if (hundredths < 10) res += "0";
  res += String(hundredths);

  return res;
}

String StopWatch::draw() {
  unsigned long currentElapsed = _isRunning ? ((getSyncedMicros() - _startTime) / 1000) : 0;
  return formatTime(_lastTime) + " / " + formatTime(currentElapsed);
}
