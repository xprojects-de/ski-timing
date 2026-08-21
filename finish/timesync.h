#ifndef TIMESYNC_H
#define TIMESYNC_H

#include <Wire.h>
#include <Arduino.h>
#include "stopwatch.h"

// Number of completed RTT round-trips used for the final median offset.
// MUST be an odd number (enforced by static_assert below).
#define TIMESYNC_SAMPLES 15

static_assert(TIMESYNC_SAMPLES % 2 == 1, "TIMESYNC_SAMPLES must be odd for correct median calculation");

class Timesync {

private:
  // --- RTT sync state ---
  // A "pending" entry is created when we receive a sync packet and
  // completed when the matching syncfin arrives from the start device.
  struct SyncPending {
    uint64_t t1;    // start-device timer at send time (from sync msg)
    uint64_t t2;    // finish-device timer at receive time (captured in OnRxDone)
    bool valid;
  } _pending;

  int64_t  _offsetSamples[TIMESYNC_SAMPLES];
  int      _samplesCollected = 0;
  int64_t  _finalMedianOffset = 0;
  bool     _isTimeSynced = false;
  uint64_t _lastT1 = 0;   // For start-device reset detection

  // Returns the median of _offsetSamples[0.._samplesCollected-1].
  // Works correctly for both odd and even counts.
  int64_t calculateMedian(int count);

public:
  Timesync();
  ~Timesync();

  // Call when a "sync:<t1>" message is received.
  // rxTimestamp = Lora::getRxTimestamp() — the finish-side receive time (t2).
  // sw is only needed for reset-recovery (offset reset to 0).
  // Returns true if the caller should send a "syncack:<t1>" back to start.
  bool onSyncMsg(uint64_t t1, uint64_t rxTimestamp, StopWatch* sw);

  // Call when a "syncfin:<t1>:<half_rtt>" message is received.
  void onSyncFinMsg(uint64_t t1, int64_t halfRtt, StopWatch* sw);

  bool isInit() const;
};


#endif