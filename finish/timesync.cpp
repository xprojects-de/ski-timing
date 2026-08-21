#include "timesync.h"
#include <esp_timer.h>
#include <math.h>

Timesync::Timesync() {
  _pending = {0, 0, false};
  _samplesCollected = 0;
  _finalMedianOffset = 0;
  _isTimeSynced = false;
  _lastT1 = 0;
  for (int i = 0; i < TIMESYNC_SAMPLES; i++) {
    _offsetSamples[i] = 0;
  }
}

Timesync::~Timesync() {}

// Returns the median of the first `count` elements of _offsetSamples.
// Handles both odd and even counts correctly.
int64_t Timesync::calculateMedian(int count) {
  if (count <= 0) return 0;

  int64_t sorted[TIMESYNC_SAMPLES];
  for (int i = 0; i < count; i++) {
    sorted[i] = _offsetSamples[i];
  }

  // Insertion sort — fast enough for TIMESYNC_SAMPLES ≤ ~20
  for (int i = 1; i < count; i++) {
    int64_t key = sorted[i];
    int j = i - 1;
    while (j >= 0 && sorted[j] > key) {
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = key;
  }

  if (count % 2 == 1) {
    // Odd: exact middle element
    return sorted[count / 2];
  } else {
    // Even: average of the two middle elements
    return (sorted[count / 2 - 1] + sorted[count / 2]) / 2;
  }
}

// ─── Phase 1: finish receives "sync:<t1>" ────────────────────────────────────
// Returns true  → caller must send "syncack:<t1>" back to start.
// Returns false → sender reset detected or already fully synced (no ack needed).
bool Timesync::onSyncMsg(uint64_t t1, uint64_t rxTimestamp, StopWatch* sw) {

  // Detect start-device reboot / counter wrap (t1 jumped backwards)
  if (_lastT1 > 0 && t1 < _lastT1) {
    Serial.println("[TIMESYNC] Start-device reset detected — re-syncing...");
    _pending = {0, 0, false};
    _samplesCollected = 0;
    _isTimeSynced = false;
    // Reset stopwatch offset so no stale value is used during re-sync
    sw->setOffset(0);
    for (int i = 0; i < TIMESYNC_SAMPLES; i++) {
      _offsetSamples[i] = 0;
    }
  }

  _lastT1 = t1;

  if (_isTimeSynced) {
    // Already synced — silently ignore further sync requests
    return false;
  }

  if (_samplesCollected >= TIMESYNC_SAMPLES) {
    // All samples collected but awaiting last syncfin — don't overwrite pending
    return false;
  }

  // Store t1 and t2 (finish-side receive time) for this round-trip
  _pending = {t1, rxTimestamp, true};

  Serial.printf("[TIMESYNC] sync received  t1=%llu  t2=%llu  (sample %d/%d)\n",
                t1, rxTimestamp, _samplesCollected + 1, TIMESYNC_SAMPLES);

  return true;  // Tell caller to send syncack
}

// ─── Phase 3: finish receives "syncfin:<t1>:<half_rtt>" ──────────────────────
void Timesync::onSyncFinMsg(uint64_t t1, int64_t halfRtt, StopWatch* sw) {

  if (!_pending.valid || _pending.t1 != t1) {
    // No matching pending entry — stale or duplicate, ignore
    Serial.printf("[TIMESYNC] syncfin ignored (t1=%llu, no matching pending)\n", t1);
    return;
  }

  // offset = t1 + half_rtt - t2
  // After applying: localTime + offset ≈ start_time_at_same_moment
  int64_t offset = (int64_t)t1 + halfRtt - (int64_t)_pending.t2;

  Serial.printf("[TIMESYNC] RTT sample %d/%d: half_rtt=%lld us  offset=%lld us\n",
                _samplesCollected + 1, TIMESYNC_SAMPLES, halfRtt, offset);

  _offsetSamples[_samplesCollected] = offset;
  _samplesCollected++;
  _pending.valid = false;

  if (_samplesCollected == TIMESYNC_SAMPLES) {
    _finalMedianOffset = calculateMedian(TIMESYNC_SAMPLES);
    sw->setOffset(_finalMedianOffset);

    // Compute stats for diagnostics
    int64_t sum = 0;
    for (int i = 0; i < TIMESYNC_SAMPLES; i++) sum += _offsetSamples[i];
    int64_t avg = sum / TIMESYNC_SAMPLES;

    int64_t variance = 0;
    for (int i = 0; i < TIMESYNC_SAMPLES; i++) {
      int64_t diff = _offsetSamples[i] - avg;
      variance += diff * diff;
    }
    int64_t stddev = (int64_t)sqrt((double)(variance / TIMESYNC_SAMPLES));

    Serial.println("\n[TIMESYNC] === SYNCHRONIZATION COMPLETE (RTT-based) ===");
    Serial.printf("[TIMESYNC] Median Offset : %lld us  (%.2f ms)\n",
                  _finalMedianOffset, _finalMedianOffset / 1000.0);
    Serial.printf("[TIMESYNC] Average Offset: %lld us\n", avg);
    Serial.printf("[TIMESYNC] Std Deviation : %lld us  (%.2f ms)\n",
                  stddev, stddev / 1000.0);
    Serial.println("[TIMESYNC] === Ready for timing! ===\n");

    _isTimeSynced = true;
  }
}

bool Timesync::isInit() const {
  return _isTimeSynced;
}
