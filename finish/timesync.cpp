#include "timesync.h"

#define TIMESYNC_COUNT 5

Timesync::Timesync() {
  _isTimeSynced = false;
}

Timesync::~Timesync() {}

void Timesync::syncTime(uint64_t remoteTime, StopWatch* stopWatch) {

  if (remoteTime < _lastRemoteTime) {

    Serial.println("Sender Reset detected! Sync again...");

    _timeSyncReceived = 0;
    _totalOffset = 0;
    _isTimeSynced = false;
  }

  _lastRemoteTime = remoteTime;

  if (_timeSyncReceived >= TIMESYNC_COUNT) {
    return;
  }

  int64_t localTime = esp_timer_get_time();
  int64_t offset = (int64_t)(remoteTime - localTime);
  _totalOffset += offset;
  _timeSyncReceived++;

  Serial.printf("Times local/remote %lld/%lld us\n", localTime, remoteTime);
  Serial.printf("Sync %d/%d: Diff = %lld us\n", _timeSyncReceived, TIMESYNC_COUNT, offset);

  if (_timeSyncReceived == TIMESYNC_COUNT) {

    _finalAverageOffset = _totalOffset / TIMESYNC_COUNT;
    stopWatch->setOffset(_finalAverageOffset);
    Serial.printf("Avarage-Offset: %lld us\n", _finalAverageOffset);
    _isTimeSynced = true;
  }
}

bool Timesync::isInit() {
  return _isTimeSynced;
}