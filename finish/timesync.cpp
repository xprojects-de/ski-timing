#include "timesync.h"
#include <esp_timer.h>

// LoRa Airtime for SF10for small packages: ~450-500ms
// Latenz is symmetric (for both directions)
#define LORA_ESTIMATED_LATENCY_US 475000

Timesync::Timesync() {
  _isTimeSynced = false;
  _timeSyncReceived = 0;
  _finalMedianOffset = 0;
  for (int i = 0; i < TIMESYNC_SAMPLES; i++) {
    _offsetSamples[i] = 0;
  }
}

Timesync::~Timesync() {}

int64_t Timesync::calculateMedian() {

  int64_t sorted[TIMESYNC_SAMPLES];
  for (int i = 0; i < TIMESYNC_SAMPLES; i++) {
    sorted[i] = _offsetSamples[i];
  }

  // Bubble Sort
  for (int i = 0; i < TIMESYNC_SAMPLES - 1; i++) {
    for (int j = 0; j < TIMESYNC_SAMPLES - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        int64_t temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }

  // Median
  return sorted[TIMESYNC_SAMPLES / 2];
}

void Timesync::syncTime(uint64_t remoteTime, StopWatch* stopWatch) {

  if (remoteTime < _lastRemoteTime) {
    Serial.println("[TIMESYNC] Sender Reset detected! Re-sync...");
    _timeSyncReceived = 0;
    _isTimeSynced = false;
    for (int i = 0; i < TIMESYNC_SAMPLES; i++) {
      _offsetSamples[i] = 0;
    }
  }

  _lastRemoteTime = remoteTime;

  if (_timeSyncReceived >= TIMESYNC_SAMPLES) {
    return;  // still syncronized
  }

  int64_t localTime = esp_timer_get_time();

  // Calculate Offset for LoRa-Latenz compensation
  // use: Message was send before ~237ms (half Airtime)
  int64_t offset = (int64_t)(remoteTime - localTime) + (LORA_ESTIMATED_LATENCY_US / 2);

  _offsetSamples[_timeSyncReceived] = offset;
  _timeSyncReceived++;

  Serial.printf("[TIMESYNC] %d/%d: local=%lld, remote=%lld, offset=%lld us (LoRa-compensated)\n",
                _timeSyncReceived, TIMESYNC_SAMPLES, localTime, remoteTime, offset);

  if (_timeSyncReceived == TIMESYNC_SAMPLES) {
    _finalMedianOffset = calculateMedian();
    stopWatch->setOffset(_finalMedianOffset);

    // Standardabweichung
    int64_t sum = 0;
    for (int i = 0; i < TIMESYNC_SAMPLES; i++) {
      sum += _offsetSamples[i];
    }
    int64_t average = sum / TIMESYNC_SAMPLES;

    int64_t variance = 0;
    for (int i = 0; i < TIMESYNC_SAMPLES; i++) {
      int64_t diff = _offsetSamples[i] - average;
      variance += diff * diff;
    }
    int64_t stddev = (int64_t)sqrt(variance / TIMESYNC_SAMPLES);

    Serial.println("\n[TIMESYNC] === SYNCHRONIZATION COMPLETE ===");
    Serial.printf("[TIMESYNC] Median Offset: %lld us (%.1f ms)\n", _finalMedianOffset, _finalMedianOffset / 1000.0);
    Serial.printf("[TIMESYNC] Average Offset: %lld us\n", average);
    Serial.printf("[TIMESYNC] Std Deviation: %lld us\n", stddev);
    Serial.printf("[TIMESYNC] LoRa Latency Compensation: %d us\n", LORA_ESTIMATED_LATENCY_US / 2);
    Serial.println("[TIMESYNC] === Ready for timing! ===\n");

    _isTimeSynced = true;
  }
}

bool Timesync::isInit() {
  return _isTimeSynced;
}

