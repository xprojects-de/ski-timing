# Hardware overview

Reference for wiring the external triggers (start switch, finish light barrier) and for choosing GPIOs. Every
statement names its source; what is not backed by a datasheet is marked as **assumption** or **to verify on the
board**.

Datasheets in [datasheets/](datasheets/):

| Part | File | Source |
|---|---|---|
| Heltec WiFi LoRa 32 V4 (board) | `Heltec WiFi LoRa 32 V4.3.1.pdf` | heltec.cn, datasheet V4.3.1 |
| autosen AL012 laser retro-reflective sensor | `Laser retro-reflective sensor-AL012.pdf` | autosen.com |
| QWORK roller-lever limit switch | — (no datasheet, only the Amazon listing) | amazon.de/dp/B09LCL36P2 |

## Wiring at a glance

Trigger input on both units: **GPIO 6** = header J3 pin 17, labelled `6` on the back of the PCB, second pin from
the `7` end of the header (sections 2–4 explain why and show the circuits).

| Unit | Board pin (label, header) | Connect to |
|---|---|---|
| Start | `6` (J3 pin 17) | 1 kΩ → limit switch terminal 4 |
| Start | `3V3` (J3 pin 2) | 4.7 kΩ pull-up → limit switch terminal 4 |
| Start | `GND` (J3 pin 1) | limit switch terminal 3 |
| Finish | `6` (J3 pin 17) | 1 kΩ → optocoupler collector |
| Finish | `3V3` (J3 pin 2) | 4.7 kΩ pull-up → optocoupler collector |
| Finish | `GND` (J3 pin 1) | optocoupler emitter |
| Finish | `5V` (J2 pin 2) | isolated DC-DC +Vin |
| Finish | `GND` (J2 pin 1) | isolated DC-DC −Vin |

Finish, sensor side (galvanically isolated, **no connection to the board's GND**): DC-DC +Vout (12 V) → sensor
pin 1 brown (L+); DC-DC 0 V → sensor pin 3 blue (L−) and optocoupler LED cathode; sensor pin 4 black (OUT) →
1.8 kΩ → optocoupler LED anode. Sensor mode: dark-on.

## 1. Board: Heltec WiFi LoRa 32 V4

| Item | Value | Source |
|---|---|---|
| MCU | ESP32-S3R2 (2 MB quad PSRAM in package), 16 MB flash | datasheet 1.2, 3.1 |
| LoRa | SX1262 + RF front end (FEM), max 28 ±1 dBm | datasheet 1.2, 3.5 |
| GPIO level | 3.3 V, **not 5 V tolerant** | ESP32-S3 (absolute max ≈ 3.6 V) |
| 3V3 header pin | up to 500 mA, always on while the board runs | datasheet 3.3 |
| Ve header pin | 3.3 V, switched by `Vext` (GPIO 36) — the datasheet says "pulled high", `VextON()` drives LOW (**to verify**); pull-ups therefore go to 3V3, not Ve | datasheet 3.3, `VextON()` |
| 5V header pin | input **or** USB, not both; no 5 V output on battery | datasheet 3.2 |
| Supply on battery | 3.3–4.2 V LiPo | datasheet 3.2 |

Board revisions differ in pin use: the V4**R8** moves `Vext_Ctrl` to GPIO 40 and uses GPIO 46 as LED (search result
on the V4R8 datasheet, not verified here). This document covers the plain V4 the README targets.

### FEM type changes which GPIOs the radio uses

The Heltec board package 3.3.8 installed here has a menu **Tools → LoRa FEM Type** (`USE_NONE_PA` default,
`USE_KCT8103L_PA`, `USE_GC1109_PA`). The Heltec library then drives these pins
(`Heltec_ESP32_Dev-Boards/src/driver/board-config.h`):

| FEM type | GPIO 2 | GPIO 5 | GPIO 7 | GPIO 46 |
|---|---|---|---|---|
| `USE_GC1109_PA` | PA_EN | — | PA_POWER | PA_TX_EN |
| `USE_KCT8103L_PA` | PA_CSD | PA_CTX | PA_POWER | — |
| `USE_NONE_PA` | — | — | — | — |

The board datasheet itself names GPIO 2 `FEM_EN` and GPIO 7 `VFEM_Control`. So **GPIO 2, 5, 7 and 46 are off limits**
for triggers, whatever FEM type is selected.

**Our boards (checked 2026-10-25):** PCB revision **V4.2**; the IDE has *LoRa FEM Type* = `USE_NONE_PA (default)`.
The library therefore drives none of the FEM pins, although the V4.2 is meant for `USE_GC1109_PA` (the README
settings table and `settings/settings.png` predate this menu). What that does to the LoRa range is **open** — not
part of the trigger work, but worth a separate range test with `USE_GC1109_PA`.

## 2. GPIO map (plain V4)

| GPIO | Used by | Usable for a trigger? |
|---|---|---|
| 0 | PRG button, `BUTTON_PIN`, deep-sleep wake | no — **strapping pin**: LOW at reset boots into the ROM download mode |
| 1 | battery voltage ADC (`BatteryCheck`) | no |
| 2 | FEM enable (datasheet + library) | no |
| 3 | strapping pin (JTAG source); `SDA` default in `pins_arduino.h` | avoid |
| 4 | free; `SCL` default in `pins_arduino.h` (any `Wire.begin()` without pins grabs it) | possible, second choice |
| 5 | KCT8103L `PA_CTX` | no |
| **6** | free (J3 pin 17), ADC1_CH5/TOUCH6 not used | **yes — recommended** |
| 7 | FEM power (`VFEM_Control`) | no |
| 8–14 | SX1262 SPI, reset, busy, DIO1 | no |
| 15, 16 | 32 kHz crystal | no |
| 17, 18, 21 | OLED SDA, SCL, RST | no |
| 19, 20 | USB D-/D+ (upload and serial monitor) | no |
| 26 | `SPICS1` — chip select of the in-package PSRAM on the S3R2 | no |
| 33 | free (J2 pin 12) | possible |
| 34 | GNSS power in `gpsTime.h` (default `pwr_pin = 34`) | avoid (board with GNSS module) |
| 35 | sync LED (`SYNC_LED_PIN`, finish) / LED | no |
| 36 | `Vext` control | no |
| 37 | battery ADC control | no |
| 38–42 | GNSS connector (RX, TX, wake, PPS, reset); 39–42 also JTAG | no (the board kit with GPS uses them) |
| 43, 44 | UART0 TX/RX | no |
| 45, 46 | strapping pins (45 sets the flash voltage at boot); 46 GC1109 TX_EN | no |
| 47, 48 | free (J2 pins 13, 14) | possible |

On our V4.2 the back of the PCB labels header J3 as `7 6 5 4 3 2 1 38 39 40 41 42 45 46 37 3V3 3V3 GND` — GPIO 6
is the second pin from the end that starts with GPIO 7; 3V3 and GND sit at the other end of the same header.

**Recommendation: GPIO 6 on both units** — same pin and same code in both sketches, not a strapping pin, not used by
the radio for any FEM type, not a default bus pin. Fallback: GPIO 47 or 48.

Because the trigger moves off GPIO 0, a pressed switch or a blocked beam at power-up can no longer put the board
into download mode — something that the current wiring "on `BUTTON_PIN`" (README) would do.

## 3. Start: roller-lever limit switch (QWORK)

No manufacturer datasheet. Source: the type label on the part (photo, 2026-10-25); the Amazon listing calls it
"DPDT" and also names ME-8104 / WD5802, the label says otherwise:

| Item | Value | Source |
|---|---|---|
| Type | ME-8108, roller lever, cable gland | label |
| Contacts | **4 terminals, two separate contacts**: NO = 3–4, NC = 1–2 (no common terminal) | label |
| Rating | 5 A / 250 V AC, 0.4 A / 115 V DC | label |
| Bounce time | not given — **assumption**: a few ms, typical for snap-action switches | — |

Wiring of the NO contact (same logic as the PRG button: active LOW, falling edge = trigger). The contacts are
potential-free, so which of 3 / 4 goes to GND does not matter; NC (1–2) stays unconnected.

```
3V3 ──[ 4.7 kΩ ]──┬──[ 1 kΩ ]── GPIO 6
                  │
     switch term. 4
     switch term. 3 ──────────── GND
```

- **External pull-up** 1–4.7 kΩ instead of only the internal one (~45 kΩ, ≈ 70 µA): a switch rated for 5 A gets
  more reliable contact with a few mA of wetting current, and a stiff pull-up is less sensitive to noise on a long
  cable. The internal pull-up stays enabled in code as a fallback.
- **1 kΩ series resistor** at the board limits current from ESD/induced spikes on the cable into the GPIO.
- NO contact = idle HIGH, pressed LOW. With NC instead, a broken cable would look like a trigger — NO keeps a cut
  cable silent (no false start).
- **Mechanics**: the start gate stays open after the racer passed — the NO contact stays closed, the input LOW,
  until the gate is closed for the next racer. Section 5 relies on that.

## 4. Finish: autosen AL012 laser retro-reflective sensor

From the datasheet:

| Item | Value |
|---|---|
| Supply | **10–30 V DC**, max 35 mA |
| Output | **PNP**, max 100 mA, voltage drop max 2 V; short-circuit, overload and reverse-polarity protected |
| Mode | light-on / dark-on, selectable on the potentiometer (also sensitivity) |
| Switching frequency | 2000 Hz (response time not given; ≈ 0.25 ms derived from it — **assumption**) |
| Range | 0.1–12 m, only with the AA966 reflector (polarisation filter) |
| Connector | M8, 4-pin: 1 BN = L+, 3 BU = L−, 4 BK = OUT, 2 unused |
| Environment | −25 … 55 °C, IP67 |
| Indicators | green = power, yellow = output switched |

Consequences:

1. **The output must never go straight to a GPIO.** Active, it is at the supply voltage minus ≤ 2 V (8–28 V);
   inactive, a PNP output is open, not LOW.
2. **The sensor needs its own 10–30 V supply.** The board has nothing above 5 V (and 5 V only on USB) — see
   [Power supply of the finish unit](#power-supply-of-the-finish-unit) below.
3. **Mode: dark-on.** The output switches when the beam is interrupted, i.e. when the racer passes. If the sensor
   loses power or its cable breaks, the output stays off: no false finish, but a silently missed one — the green
   LED shows the sensor has power, the yellow one that it switches.

Recommended interface: an **optocoupler** (e.g. PC817), which converts the PNP output into the same active-LOW
signal as the button and separates the sensor's supply from the board:

```
sensor BK (OUT) ──[ R ]──▶|── sensor BU (L−)        R ≈ 1.8 kΩ at 12 V, ≈ 4.7 kΩ at 24 V (≈ 5 mA LED current)
                         opto LED

3V3 ──[ 4.7 kΩ ]──┬──[ 1 kΩ ]── GPIO 6
                  │
     opto collector
     opto emitter ─────────────── GND (board)
```

Beam interrupted → opto conducts → GPIO 6 LOW → falling edge, exactly like the button. The opto adds a few µs of
switch-on delay (constant, irrelevant at the ms resolution of the CSV).

Alternative without isolation: a voltage divider (sized for the highest supply voltage) plus a common ground —
cheaper, but its level depends on the supply and inverts the logic (active HIGH). Not recommended.

### Power supply of the finish unit

**Recommended (decided 2026-10-25): one USB powerbank for board and light barrier, the sensor side galvanically
isolated** by an isolated DC-DC converter (power) and the optocoupler above (signal). Sensor, its cable and the
12 V side then share no conductor with the board; a ground loop or a spike picked up on the sensor cable cannot
reach the ESP32.

```
                         board (Heltec V4.2)
powerbank ══USB-C══════▶ USB-C
                         J2 pin 2 (5V) ──────┬── +Vin   isolated DC-DC   +Vout (12 V) ── sensor BN (L+)
                         J2 pin 1 (GND) ─────┴── −Vin   5 V → 12 V, 1 W  0 V ────────── sensor BU (L−)
                                                        ═══ isolation barrier ═══        │
                         GPIO 6 ◀── opto transistor ◀═══ opto LED ◀── R ◀── sensor BK (OUT)
                                                                       opto LED cathode ─┘ (to 0 V)
```

| Part | Recommendation | Why |
|---|---|---|
| Isolated DC-DC | 5 V → 12 V, 1 W, ≥ 1 kV isolation, SIP-7 or SIP-4, e.g. Mornsun B0512S-1WR3, Traco TMA 0512S, Recom R1SE-0512 (**check the chosen part's datasheet** for input/output capacitors) | 12 V × (35 mA sensor + 5 mA opto LED) ≈ 0.5 W → ~50 % load of a 1 W module. Unregulated is fine: the sensor accepts 10–30 V |
| Feed of the DC-DC | the board's **5V pin** (J2 pin 2, GND J2 pin 1) | Datasheet 3.3: the 5V pin outputs USB 5 V with up to 500 mA. One cable from the powerbank, nothing to split |
| Optocoupler | PC817 (or similar), LED resistor ≈ 1.8 kΩ at 12 V | see interface above |
| Powerbank | USB-C, ≥ 10 000 mAh, **no auto-off at low current** ("always on" / "low current" mode) — check before race day | see below |

Rules:

- **Never connect the 12 V side's 0 V to the board's GND** — that removes the isolation.
- The DC-DC's `+Vin/−Vin` belong to the board side; `+Vout/0 V`, the sensor and the opto LED to the sensor side.

Consumption and runtime (**estimates**): board with WiFi AP ≈ 170 mA (datasheet 3.4) + DC-DC ≈ 0.5 W / 75 %
efficiency / 5 V ≈ 130 mA → ≈ 0.3 A at 5 V, ≈ 1.5 W. A 10 000 mAh powerbank (≈ 37 Wh, ≈ 25 Wh usable at 5 V)
lasts ≈ 15 h — a race day with margin. If the board's LiPo is also connected, USB charges it with up to 500 mA
(datasheet 3.4) on top, until it is full.

Things to know on race day:

- **Powerbank auto-off.** Many powerbanks switch off below a load of roughly 50–100 mA (varies by model, not in
  any datasheet here). 0.3 A is above that while the board runs; in deep sleep the board draws almost nothing, so
  the powerbank may switch off — waking then needs the powerbank's button and the PRG button.
- **LiPo as buffer hides a dead powerbank.** With the board's LiPo connected, the board keeps running on battery
  when the powerbank goes off, but the 5V pin — and with it the light barrier — goes dead. Dark-on means no false
  finish, but every finish is missed silently. Either run without the LiPo (the display goes dark with the
  powerbank — obvious), or watch the sensor's green LED.
- **Cold.** Li-ion cells lose capacity well below 0 °C; keep the powerbank in an insulated box or inside a jacket
  and connect it by cable.

Considered and dropped: a boost converter from the board's LiPo (no isolation, doubles the drain on a 3000 mAh
cell); a separate 12 V battery (isolated as well, but one more battery to charge and a second supply to forget).

## 5. Trigger inputs in the firmware

`TRIGGER_PIN` = **GPIO 6** in `start/start.ino` and `finish/finish.ino`, `INPUT_PULLUP` (backup for the external
pull-up). The PRG button on GPIO 0 is unchanged — manual timing works exactly as before, including all hold-time
functions.

Start and finish use different code, because the devices behave differently: the start gate **stays open** after
the racer passed (input stays LOW until the gate is closed for the next racer), the light barrier gives a **short
pulse** (a few ms per leg).

**Start — trigger on the edge, confirmed while the gate is open** (`handleTriggerInput()` in `start.ino`):

1. Falling-edge ISR stores the time of the **first** edge and then ignores further edges (bounce).
2. `TRIGGER_CONFIRM_US` (10 ms) later `loop()` checks the input: still LOW → the start is sent with the edge time;
   HIGH again → it was interference, nothing is sent.
3. The ISR is re-armed only when the input is HIGH again — the gate was closed for the next racer.
4. The 2 s burst cooldown still applies, shared with the button.

**Finish — trigger on release** (`handleTriggerInputChange()` in `finish.ino`):

1. Falling edge → the ISR stores its time.
2. Rising edge → if the input was LOW for at least `TRIGGER_MIN_PULSE_US` (1 ms), a finish is reported with the
   stored falling-edge time. Shorter pulses are interference (µs). 1 ms stays below the ~3 ms a leg blocks the beam
   at 100 km/h (**estimate**). The time comes from the ISR, so a late `loop()` (OLED refresh ≈ 18 ms) does not
   matter.
3. Repeats (second leg, poles) are dropped by the stopwatch debounce: 1 s in normal mode. Continuous mode keeps
   50 ms — it is timed by hand on the button, never with the light barrier (decided 2026-10-25).

Common to both:

- **Only the trigger function, no hold-time functions.** An open gate or a blocked beam must not discard a start
  time (1 s), switch the mode and clear all results (4 s) or put the unit to sleep (7.5 s).
- **Deep sleep** detaches the trigger interrupt as well; wake stays on the PRG button only.
- **Idle level at boot**: the pull-up keeps an unconnected input HIGH. Since the trigger is not on GPIO 0, it cannot
  force the download mode.
- Dropped repeats show in the serial log (`[TRIGGER] Start ignored …` / `[STOPWATCH] Finish ignored …`).

Accepted limits, in exchange for little code:

- Start: a gate that closes again within 10 ms of opening is taken as interference (it stays open by design).
- Finish: a beam blocked for seconds gives its finish only when it is free again (time is still that of the break);
  closing bounce does not exist behind the optocoupler.
- Interference longer than the filter (start 10 ms LOW, finish 1 ms) counts as a trigger. The hardware keeps it
  rare (stiff 4.7 kΩ pull-up, 1 kΩ series resistor, optocoupler at the finish).

The PRG button needs no extra filter: it sits on the board (no cable to pick up interference) and already ignores
presses shorter than 30 ms (`BTN_MIN_VALID_PRESS_MS`).

## 6. Open points to check on the board

- ~~Board revision~~ V4.2; ~~FEM type in the IDE~~ `USE_NONE_PA` — range test with `USE_GC1109_PA` still open.
- ~~Number of terminals on the limit switch~~ 4: NO 3–4, NC 1–2.
- README settings table is behind the IDE (photo 2026-10-25): it lacks *LoRa FEM Type*, *LoRa External FEM Receive
  Gain* (`Disabled`) and *PSRAM* (`Disabled`), and shows *Core Debug Level* `Info` / *Upload Speed* `115200` where
  the IDE has `None` / `921600`.
- ~~Supply for the AL012~~ powerbank + isolated 5 V → 12 V DC-DC from the board's 5V pin (section 4). On the board,
  measure that the 5V pin really carries 5 V with USB connected; check the powerbank for auto-off at low load.
- ~~Lockout in continuous mode~~ stays 50 ms: continuous mode is timed by hand, never with the light barrier.
- On the board: one start per switch contact, one finish per racer (also with poles), a beam held blocked for > 8 s
  gives one finish on release and no mode switch or sleep, deep sleep / wake still on the PRG button only (serial
  log `[TRIGGER]`, `[STOPWATCH]`).
