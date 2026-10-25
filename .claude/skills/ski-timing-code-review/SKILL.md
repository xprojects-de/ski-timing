---
name: ski-timing-code-review
description: "Full non-diff, multi-agent review of the current state of the ski-timing firmware (start and finish sketch): the button/trigger path from ISR to LoRa packet or recorded time, the LoRa layer (FreeRTOS task, TX/RX queues, ST- prefix, burst and duplicate filter), the RTT time synchronization (sync/syncack/syncfin, median of 15 offsets, reset detection), the stopwatch (start-time queue, normal and continuous mode, debounce, time-domain conversion, integer ms), the CSV on LittleFS and the finish unit's web server and page, the access point, display, battery task and deep sleep - and the drift between the duplicated modules of both sketches. Walks every race-day failure case (packet lost, start unit rebooted, three racers on course, fall, helper reloading the page, battery low, hours of uptime). Asks which parts to review. Follows the shared review base (no hallucination, no over-optimisation, decision register, at most 10 verified findings, fix packages with a board test each). Use when the user invokes /ski-timing-code-review or asks for a code review of the start/finish firmware - NOT a diff/branch/PR comparison."
---

## What this does

A read-only review of the **whole current firmware**, both sketches, as it would behave at a race: a helper presses
the start gate, the start unit's ISR takes a µs timestamp, the packet `ST-time:<µs>` goes out three times over LoRa,
the finish unit drops the duplicates, keeps the start in a queue of three, the light barrier fires, the finish
timestamp is moved into the start unit's time domain with the synced offset, the elapsed ms are appended to
`/times.csv` and shown on the display and the web page a helper's phone has open. Before any of that works, 15
sync round-trips must have produced a median offset. Everything runs on battery for hours, with a button that also
means "fall", "mode toggle" and "shutdown" depending on how long it is held - see [CLAUDE.md](../../../CLAUDE.md)
"How it works".

Several parallel agents each read the **full files** of their part - not excerpts, not `git diff` - and judge them
against the same key questions and failure cases. The findings are pooled, verified, cut to what holds and grouped into
fix packages, each with the button sequence or race scenario that proves the fix on the board.

**It follows [.claude/review-base.md](../../review-base.md)**: rules, the decision register, no hallucination, no
over-optimisation, what counts as a finding, the verifier, the budget, fix packages and the report. Read it first; this
skill adds parts, paths, key questions, failure cases and its own report sections.

What this review is not: a diff review (`/code-review` does that), a hardware or RF review (antenna, range, the
light barrier's electrical behaviour are "Offen/unklar" with what to measure), and not a security review of the open
LoRa link or the AP without login - both are documented decisions in [SECURITY.md](../../../SECURITY.md).

## Step A - choose the parts

- If the arguments name parts (`teil=1,3`, `alle` ...), take them.
- Otherwise **ask** with `AskUserQuestion` - one question, `multiSelect: true`, header "Teile", question "Welche Teile
  der Firmware soll das Review prüfen?", options:
  - "1: Start-Einheit" - `start.ino`: ISR, Tasten-Zustandsautomat, Burst und Cooldown, Sync-Initiator (sync/syncfin), Display, Deep Sleep
  - "2: Ziel: Funk & Zeitsync" - `finish/lora.*` (Duplikat-Cache), `timesync.*` (15 Samples, Median, Reset-Erkennung), `finish.ino` Nachrichten-Dispatch
  - "3: Ziel: Stoppuhr & Speicher" - `stopwatch.*` (Queue, Modi, Debounce, Zeitdomäne), `customFilesystem.*` (LittleFS CSV), Tasten-Zustandsautomat in `finish.ino`
  - "4: Ziel: Web & gemeinsame Module" - `customWebserver.*`, `website.h`, `accesspoint.*`, `display.*`, `batteryCheck.*`, Deep Sleep, Drift zwischen den doppelten Modulen beider Sketche
  Nothing chosen (or "Other: alle") means all four.

## Scope

Two independent Arduino sketches, `start/` and `finish/`, no shared folder. Board: Heltec WiFi LoRa 32 V4
(ESP32-S3), `REGION_EU868`, partition scheme with SPIFFS/LittleFS - [README.md](../../../README.md) "Arduino IDE".
Serial log tags: `[LORA]`, `[SYNC]`, `[TIMESYNC]`, `[STOPWATCH]`, `[BTN]`, `[HEAP]`, `[POWER]`.

| Part | Where | Special care |
|---|---|---|
| 1 Start-Einheit | `start/start.ino`, `start/lora.{h,cpp}` | `handleButtonPress` ISR and the 64-bit `buttonPressMicros` read under `noInterrupts()`; `handleButtonStateMachine` (30 ms / 7.5 s thresholds, `BTN_DEBOUNCE_RELEASE`); `BURST_COOLDOWN_MS` suppressing both a second start and the periodic sync; `sendData(..., burst=true)` with `BURST_COUNT 3` and `TX_QUEUE_DEPTH 8`; `syncack` → `halfRtt = (t4 - t1) / 2` → `syncfin`; `lastSyncTime` initialised to fire at once; `powerDownAndSleep` (`detachInterrupt`, `VextOFF`, ext0 wake on GPIO 0) |
| 2 Ziel: Funk & Zeitsync | `finish/lora.{h,cpp}`, `finish/timesync.{h,cpp}`, `finish/finish.ino` (the `tryGetPacket` dispatch for `syncreq:` / `syncfin:` / `time:`) | `_timestampCache` duplicate filter inside `OnRxDone` (ISR/driver context: `Serial.printf` there, `strtoull` there); `TX_QUEUE_DEPTH 4`; `Timesync::onSyncMsg` (reset detection `t1 < _lastT1`, pending overwritten by a second `sync` before its `syncfin`, `_samplesCollected >= TIMESYNC_SAMPLES` branch); `onSyncFinMsg` (`sscanf("%llu:%lld")`, offset sign, the median only once, `_isTimeSynced` never re-armed except by reset); `time:` ignored while `!isInit()` - what the display shows then |
| 3 Ziel: Stoppuhr & Speicher | `finish/stopwatch.{h,cpp}`, `finish/customFilesystem.{h,cpp}`, `finish/finish.ino` (`handleButtonStateMachine`, `setup()` order) | `START_TIME_QUEUE_DEPTH 3` and what the fourth start does; `triggerStart` storing start-domain µs while `triggerFinishWithTimestamp` converts finish-local µs with `_offset` - the same domain on both sides?; continuous mode storing `syncedTimestamp` as reference; `_lastFinishTimestamp` debounce across a `reset()`; `draw()` using `getSyncedMicros()` against the queued start; `formatTime`/`draw` static buffers returned as `String`; `CFs::append` `%d,%lld` with `_lastId` restored by `getLastId()` on boot; `clear()` via `FILE_WRITE`; `readInChunks` yielding inside an async handler; `fs_instance.clear()` on mode toggle (times gone on a 4 s press) |
| 4 Ziel: Web & gemeinsame Module | `finish/customWebserver.{h,cpp}`, `finish/website.h`, `finish/accesspoint.{h,cpp}`, `start/display.*` vs `finish/display.*`, `start/batteryCheck.*` vs `finish/batteryCheck.*`, `start/lora.*` vs `finish/lora.*`, both `powerDownAndSleep` | Every route `GET` and side-effecting (`/reset`, `/discard`, `/enableContinuousMode` from a prefetching browser or a bookmark); handlers calling `fs_instance->clear()` and `sw_instance->reset()` from the AsyncTCP task while `loop()` runs the stopwatch - which shared state, which lock; `data_handler` streaming the whole CSV inside the response stream; `website.h` polling interval and the 5 s abort; the AP's fixed SSID/password/IP; `BatteryCheck` task reading the ADC while `Vext`/display use the bus; `diff start/x finish/x` for every duplicated module - a fix present in one copy only is a finding; GPIO 0 as button and boot-strap pin, ext0 wake with `INPUT_PULLUP` gone after `VextOFF` |

## Key questions (every agent keeps them in mind; the report answers each one explicitly)

1. **Is every recorded time right?** Start µs (start domain) and finish µs (finish domain + offset) are subtracted
   in the same domain; `offset = t1 + halfRtt - t2` has the sign the subtraction needs; all arithmetic is 64-bit
   signed before the division to ms; nothing is truncated into `int` before `snprintf("%d")`; the median of an odd
   sample count is the middle element; a result is never written when the offset is not yet valid.
2. **Does the right start meet the right finish?** FIFO of three in normal mode, oldest consumed per finish; a fall
   (1 s press or `/discard`) removes exactly the oldest; the fourth start is refused *visibly*; continuous mode keeps
   one reference and never pops it; a mode toggle or reset empties the queue and the stopwatch state together.
3. **Does sync complete, and recover?** 15 completed round-trips with a `syncfin` matching its pending `sync`; a
   `sync` that arrives while a pending entry waits (packet lost, 5 s interval vs. the LoRa air time at SF10) is handled
   in a way that still converges; a start unit reboot (`t1` jumps back) resets the finish side and the stopwatch
   offset; a finish unit reboot re-syncs because the start unit keeps sending `sync` every 5 s - or does it stop?
   (`_isTimeSynced` is finish-side only; the start unit has no notion of "synced").
4. **Is a real trigger ever swallowed?** 30 ms minimum press vs. the limit switch's and light barrier's pulse width;
   `burstCooldownActive` for 2 s after a start; `DEBOUNCE_TIME_NORMAL_US` 1 s at the finish; the RX queue of 10 and
   the TX queue (8 / 4) under a burst plus a sync; `buttonTriggered` set in the ISR but consumed only in `BTN_IDLE` -
   a press during `BTN_PRESSED` is lost by design or by accident?
5. **Is shared state safe?** Every variable touched by the ISR, `LoraTask`, the battery task, the AsyncTCP callbacks
   and `loop()`: `volatile` where needed, 64-bit reads in a critical section, FreeRTOS queues as the only hand-over
   between tasks, no `Serial`/heap work in an ISR path that the Heltec driver may call from an interrupt; `_radioBusy`
   / `_rxActive` transitions complete (every `Radio.Send` ends in `OnTxDone` or `OnTxTimeout`).
6. **Does it survive a race day?** Hours of uptime: no `String` or heap growth per loop iteration or per packet (the
   `[HEAP]` log is the hint, not the proof); `millis()` wrap handled by subtraction; `esp_timer_get_time()` monotonic
   across light sleep?; LittleFS append per finish and `clear()` wear; the display redraw every 250 ms; the battery
   percentage formula and when the display can still warn.
7. **Does the button mean what the helper thinks?** Hold thresholds 30 ms / 1 s / 4 s / 7.5 s, interpreted on
   release: can a slow 1.2 s press at the finish discard a start by accident while the helper only meant "finish"?
   Is there display feedback for fall, mode toggle, shutdown-armed, queue full, not synced?
8. **Does the web page tell the truth and change nothing by accident?** `/data` complete and in order while a finish
   is being appended; `/status` matching the display; side effects only from an explicit tap (no `GET` prefetch, no
   double tap doing it twice); the German UI texts matching what the unit does; the page surviving a lost AP
   connection.
9. **Do both sketches agree?** Same `RF_FREQUENCY`, SF, bandwidth, coding rate, `LORA_PREFIX`, `BUFFER_SIZE`; the
   same ISR and state machine pattern; a bug fixed in one `lora.cpp`, `display.cpp` or `batteryCheck.cpp` and not the
   other; the board settings the README prescribes matching what the code assumes (`Vext`, pins 0/35, LittleFS
   partition).

## Failure cases (every agent walks through them for its part)

A `time:` burst where 1 of 3 packets arrives · all 3 lost · the burst arriving *after* the finish trigger (slow racer
on a short course is impossible, but a finish button pressed by mistake first) · `sync` lost · `syncack` lost ·
`syncfin` lost (pending entry stays) · `syncfin` for an old `t1` · a `sync` arriving while a `syncfin` is pending ·
the start unit rebooted mid-race (`t1` back to near 0) · the finish unit rebooted mid-race (queue, offset and
`_lastId` after boot) · both booted, start unit powered on 10 minutes after the finish unit (offset in the order of
600 s, sign) · three racers on course, a fourth starts · a fall: 1 s press, `/discard`, both · the finish barrier
fires twice within 1 s (second racer close behind, or a ski) · a racer crosses during the 2 s burst cooldown of the
start · the finish button held 1.2 s by accident · 4 s by accident (times cleared) · 7.5 s with the display off ·
wake from deep sleep by the gate (is the wake press also a start?) · continuous mode: first trigger before sync is
done · continuous mode: `/disableContinuousMode` while the reference is set · the helper's phone reloading `/` every
few seconds for an hour · two phones · `/data` during an append · `/reset` tapped during a race · the LittleFS mount
failing (every `append` returns false - what does the display show?) · flash full · 6 hours of uptime
(`esp_timer_get_time()` ≈ 2.2 × 10¹⁰ µs, `millis()` fine, `int` seconds in `draw()`) · battery at 3.3 V · `Vext` off
during shutdown with the button still wired through it · the display I²C bus shared with anything else · a LoRa
packet from a foreign device with a payload starting `ST-` · a payload of exactly `BUFFER_SIZE` bytes · a `time:`
payload that is not a number (`strtoull` → 0, stored as a start).

## Step 1 - Spawn the agents

One agent per chosen part, in a single turn, plus the ground-truth compile of review base section 1 when
`arduino-cli` is available (it is not installed by default on this machine - say "nicht kompiliert" otherwise). Give
every agent "What this does", its part, paths and special care, the key questions, the failure cases, sections 2 to 5
of the review base verbatim, and:

> You are reviewing the CURRENT STATE of a real firmware: not a diff. Read `CLAUDE.md`, `README.md`, `SECURITY.md`,
> `CONTRIBUTING.md` and `docs/review/decisions.md` first, then every file of your part in full - and for a duplicated
> module both copies, with `diff start/<file> finish/<file>`. Context: Arduino framework on ESP32-S3 (Heltec WiFi
> LoRa 32 V4), Heltec `LoRaWan_APP` radio driver with `RadioEvents` callbacks and `Radio.IrqProcess()` polled from a
> FreeRTOS task on core 1, `esp_timer_get_time()` as the µs clock, FreeRTOS queues between ISR/tasks/`loop()`,
> LittleFS for `/times.csv`, ESPAsyncWebServer/AsyncTCP serving from its own task, SSD1306 OLED over I²C, deep sleep
> with ext0 wake on GPIO 0. There is no test suite and no simulator; you cannot run anything - reason from the code,
> and where only a measurement on the board would settle it, say so under "Offen/unklar" with what to measure.
> Read-only: do not edit files, do not flash, do not open a serial port.
> Deliver first a **walkthrough** of your part: one row per step of the signal's way (ISR → state machine → queue →
> radio → `OnRxDone` → RX queue → dispatch → timesync/stopwatch → LittleFS → display/web) - step · function
> (path:line) · which task/context runs it · shared state it touches and how it is protected · what happens when the
> step fails or the packet is lost · verdict ✅ / ⚠️ / ❗. Every ⚠️/❗ row is also a finding.
> Severity for this review: **Kritisch** = a recorded time wrong without anyone noticing (wrong domain, wrong sign,
> wrong racer, truncation), recorded times lost by a normal action or a silent failed append, a unit locking up,
> resetting or sleeping during a race; **Hoch** = a real start or finish trigger silently ignored, sync never
> completing or completing wrong, a button or web function broken, a sketch that does not compile, heap/queue/flash
> exhausted on a normal race day, a fix present in one sketch's copy of a module and missing in the other.

## Step 2 - Synthesize

Follow **section 6 of the review base**, then the report as in **section 7**. The skill's own sections:

- **Der Weg eines Signals**: the walkthroughs as one table from the start gate to the CSV row, only ⚠️/❗ rows in
  full, the rest as a count per step.
- **Ausfall-Matrix**: one row per failure case · what the finish unit records (right time / wrong time / nothing) ·
  what the display shows · what the serial log says · what the web page shows · next racer works ✅/❌ · where
  checked (path:line).
- **Zeitdomänen**: a short table of every timestamp variable (`buttonPressMicros`, `t1`..`t4`, `_pending.t2`,
  `_offset`, `startTime` in the queue, `syncedTimestamp`, `_lastTime`, the CSV value): unit (µs/ms), domain (start /
  finish / start-via-offset), signed/unsigned, where converted - and whether every subtraction pairs two values of the
  same domain and unit.
- **Tastenbelegung**: one row per hold duration per unit: threshold · action · display feedback yes/no · reachable
  by accident (how) · intended per CLAUDE.md yes/no.
- **Drift der doppelten Module**: one row per duplicated file: identical / differs on purpose (what) / differs by
  accident (what, which copy is right).
- **Kernfragen**: each key question answered in two or three sentences with the evidence (path:line).
