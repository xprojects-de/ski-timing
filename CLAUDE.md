# CLAUDE.md

Guidance for Claude Code when working in this repository.

## What this project is

Arduino/ESP32 firmware for a **LoRa-based start/finish timing system** for ski races and similar
club competitions. Two Heltec WiFi LoRa 32 (V4, ESP32-S3) boards form a closed pair:

- **Start unit** (`start/`) — sends a microsecond timestamp over LoRa the moment the start gate /
  button is triggered, and drives the clock synchronization with the finish unit.
- **Finish unit** (`finish/`) — receives the start timestamps, timestamps the finish trigger
  (e.g. a laser light barrier), computes elapsed times, stores them in a CSV on flash, and serves
  them on a small web page over its own WiFi access point.

Both run standalone on battery with an OLED status display, without any network or internet. The
CSV export (`id,time` in ms) is meant to be imported into the sister project
[Alpdesk Time Control](https://github.com/xprojects-de/alpdesk-time-control).

## Repository layout

Two independent Arduino sketches; each folder is opened and flashed on its own in the Arduino IDE.
There is no shared library folder — common modules are **duplicated** in both sketches.

```
start/   start.ino        main loop: button state machine, sync initiator, display
         lora.*           LoRa radio wrapper (FreeRTOS task + TX/RX queues), burst sending
         display.*        thin wrapper around the Heltec SSD1306 OLED
         batteryCheck.*   battery voltage/percentage in a background FreeRTOS task
finish/  finish.ino       main loop: button state machine, LoRa message dispatch, display, sync LED
         lora.*           same as start, plus duplicate filtering of burst `time:` packets
         timesync.*       RTT-based clock offset estimation (median of 15 samples)
         stopwatch.*      start-time queue, elapsed-time calculation, normal/continuous mode
         customFilesystem.*  LittleFS `/times.csv` append/clear/chunked read
         customWebserver.*   ESPAsyncWebServer routes
         website.h        embedded HTML/JS UI (PROGMEM string, German UI text)
         accesspoint.*    WiFi soft-AP "time-control" at 192.168.4.1
         gpsTime.*        GPS (TinyGPS++) helper — NOT used; time sync is LoRa-only, not GPS
settings/settings.png     Arduino IDE board settings screenshot
docs/hardware.md          GPIO map, trigger wiring (start switch, finish light barrier), datasheet facts
docs/datasheets/          board and sensor datasheets (PDF)
```

`display.*` and `batteryCheck.*` are identical in both folders; `lora.*` differs (start: burst
send, TX queue depth 8; finish: no burst, TX depth 4, duplicate cache). When changing a shared
module, check whether the other copy needs the same change.

## How it works

### LoRa link
868.1 MHz, SF10, 125 kHz, CR 4/8, 14 dBm. Every payload is prefixed with `ST-`; packets without
the prefix are dropped. Radio access runs in a dedicated FreeRTOS task (`LoraTask`, core 1); the
main loop only talks to it through thread-safe queues (`sendData` / `tryGetPacket`). The RX
timestamp is captured in `OnRxDone` with `esp_timer_get_time()`.

### Message protocol (text, after the `ST-` prefix)
| Message | Direction | Meaning |
|---|---|---|
| `sync:<t1>` | start → finish | sync request, `t1` = start-local µs at send (every 5 s) |
| `syncack:<t1>` | finish → start | echo of `t1`; finish stores its receive time `t2` |
| `syncfin:<t1>:<halfRtt>` | start → finish | start measured `t4`, sends `halfRtt = (t4 - t1) / 2` |
| `time:<µs>` | start → finish | start trigger timestamp, sent as a burst of 3 packets |

### Time synchronization (`finish/timesync.cpp`)
Per round-trip: `offset = t1 + halfRtt - t2`. After 15 samples (`TIMESYNC_SAMPLES`, must be odd)
the median becomes the stopwatch offset, the sync LED (pin 35) turns on and the display shows
`sync`. If `t1` jumps backwards (start unit rebooted) the sync is reset and redone. Start
timestamps (`time:`) are ignored until sync is complete.

### Timing (`finish/stopwatch.cpp`)
- Finish timestamps are converted into the start unit's time domain (`timestamp + offset`), then
  `elapsed = synced finish - start`, stored in **milliseconds as integer**.
- **Normal mode**: FIFO queue of up to 3 start times (3 racers on course at once). Each finish
  consumes the oldest start. Finish debounce 1 s.
- **Continuous mode**: the first trigger sets a single reference start; every following finish
  records the time since that reference (lap/split style). Debounce 50 ms. Switching modes
  resets the stopwatch and clears the CSV.

### Button / trigger handling
`BUTTON_PIN` = GPIO 0, falling-edge ISR captures the µs timestamp; a non-blocking state machine
in `loop()` interprets the hold duration on release:

| Hold | Start unit | Finish unit |
|---|---|---|
| ≥ 30 ms | start trigger (then 2 s burst cooldown) | finish trigger |
| ≥ 1 s | — | "fall": discard oldest start time (normal mode only) |
| ≥ 4 s | — | toggle normal/continuous mode (clears times) |
| ≥ 7.5 s, then release | deep sleep (wake on button) | deep sleep (wake on button) |

### Finish web interface
Join WiFi `time-control` (password `time-control`) and open `http://192.168.4.1/`.
Routes (all `GET`): `/` UI, `/data` raw CSV, `/status` (`normal`/`continuous`), `/discard`,
`/reset`, `/enableContinuousMode`, `/disableContinuousMode`, `/ping`. No authentication — by
design, see [SECURITY.md](SECURITY.md).

## Reviews

`/ski-timing-code-review` ([.claude/skills/ski-timing-code-review/SKILL.md](.claude/skills/ski-timing-code-review/SKILL.md))
is a read-only, multi-agent review of the whole current firmware - both sketches, never a diff. It follows
[.claude/review-base.md](.claude/review-base.md) (no hallucination, no over-optimisation, at most 10 verified
findings, fix packages with a board test each, report in German) and skips what
[docs/review/decisions.md](docs/review/decisions.md) lists as accepted. A finding the user accepts as intended is
added there as an E-entry, never silently dropped. For a diff review use the built-in `/code-review`.

## Building and testing

- Build/flash only via the **Arduino IDE** (or Arduino CLI) with the ESP32 + Heltec board
  packages and the libraries listed in [README.md](README.md#arduino-ide) (Heltec ESP32 Dev-Boards,
  ESPAsyncWebServer, AsyncTCP). Board: `WiFi LoRa 32(V4)`, region `REGION_EU868`, partition
  scheme with SPIFFS — full settings table in the README.
- There is **no automated test suite and no simulator**. Changes can only really be verified on
  hardware; don't claim a change "works" from reading code alone — say it needs flashing.
- Serial monitor at 115200 baud; logs use tags like `[LORA]`, `[SYNC]`, `[TIMESYNC]`,
  `[STOPWATCH]`, `[BTN]`, `[HEAP]`, `[POWER]`.

## Git (always)

**Never commit, amend or push on your own** — not after a finished change, not when a plan was
agreed, not when "let's continue" follows. The user reviews every change in the working tree first
and commits it themselves. Leave the changes uncommitted, report what changed, and commit only when
the user explicitly asks for that commit.

- Commit messages and PR descriptions are **English**.
- `main` is the default/release branch, `dev` the integration branch of those with write access.
  External contributors branch off `main` in their fork and open the PR against `main`.
- One thing per PR — two small ones are better than one that reformats on the side.
- A PR describes **why** the change is needed (for timing accuracy or the LoRa protocol: a concrete
  race-day scenario) and which board(s) it was flashed and tested on.
- Docs are updated in the same change: behaviour, hardware or setup → [README.md](README.md); what the
  system assumes or deliberately does not do (open LoRa link, AP without login) →
  [SECURITY.md](SECURITY.md); this file when a rule or the architecture changes.

## Language convention (always)

- **The finish unit's web UI (`finish/website.h`) is German** — buttons, labels, messages. Its users
  are the German-speaking helpers at the finish line.
- **Everything else is English** — identifiers, comments, serial log messages, HTTP routes and plain-text
  responses, docs, commit messages. Some older log lines are still German; new ones are English.
- Don't translate one into the other's territory.

## No real data in the repository (always)

No real race results, start lists, names of athletes or club members, real WiFi passwords, local paths
or personal data — not in code, sample CSVs, screenshots or comments. It stays in the Git history even
when the file is deleted later. Examples use invented data.

## Code style (always)

Code is written to be read: someone new to the project must follow a file from top to bottom without
jumping around. Lean and readable beats clever and general.

- **Names say what, comments say why.** A comment explains a reason, a threshold, a hardware or
  library quirk the code cannot show — never repeats the code.
- **Small, flat functions**: early returns instead of nested ifs; short, flat `switch`/state-machine
  functions for the button handling; a well-named helper instead of a block with a comment above it.
- **No abstraction in advance**: no base class with one implementation, no generic framework for one
  use, no configuration option nobody asked for. Add it when the second case is there.
- **No optimisation without a real case** — only where a load exists, a timing error is measured, or it
  removes a wrong result. In doubt leave it, say so ("not done on purpose: ...") and let the user
  decide. This limits what gets *built*, never what gets *looked at*: ISR safety, FreeRTOS
  concurrency, heap fragmentation and timing precision are always checked in full depth.
- **A recommendation is held to the same bar as a change**: before recommending something, check there
  is a real case today and what the damage without it is.
- **Follow the neighbours**: a new module looks like the existing ones (class with `init()` /
  `startTask()`, header guard, `#define` constants at the top of the `.cpp`).

Firmware-specific (from [CONTRIBUTING.md](CONTRIBUTING.md)):

- No blocking `delay()` in the main loop paths — use `millis()` intervals like the existing code.
- `snprintf` into fixed-size buffers instead of `String` concatenation; avoid heap churn (long
  race-day uptimes, heap is logged every 60 s for fragmentation).
- Timestamps as **integers** (µs from `esp_timer_get_time()`, ms in the CSV) — no floating-point
  time arithmetic.
- 64-bit values shared with an ISR are read inside `noInterrupts()` / `interrupts()`.
