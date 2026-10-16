# Contributing

Thanks for stopping by. This firmware grew out of a real need at a ski club, and it's used at
real races — contributions from that same kind of practice are especially welcome: support for
another board, a bug you tripped over at the start gate, or an improvement to the LoRa sync.

## What you need

| | |
|---|---|
| **Arduino IDE** | With the ESP32 and Heltec board packages, and the libraries listed in [README.md](README.md#arduino-ide) installed. |
| **A Heltec WiFi LoRa 32 board** (or two) | There is no simulator for this project — the start/finish sync, the LoRa link, the display and the button/deep-sleep logic can only really be verified on real hardware. |
| **esptool** (optional) | For resetting or reflashing a board that got stuck, see [README.md](README.md#esptool). |

## Building and flashing

Follow the [Setup section in README.md](README.md#setup) to install the board packages and
libraries, then open `start/start.ino` or `finish/finish.ino` in the Arduino IDE, select the
correct board and port under *Tools* (see [settings/settings.png](settings/settings.png)), and
upload.

There is currently no automated test suite — this is firmware for two small embedded units with
a physical button, an LoRa radio and a display, and the realistic way to verify a change is
flashing it onto a board and trying it. When you submit a change, say in the PR which board(s)
you actually flashed and tested it on.

## Code conventions

- Code, comments, log and error messages are in English throughout.
- Keep the existing style: short, flat `switch`/state-machine functions for the button handling,
  `snprintf` into fixed-size buffers rather than `String` concatenation, and comments that explain
  *why* a threshold or workaround exists rather than what the next line does.
- Timestamps are kept as integers (microseconds or milliseconds, depending on the module) — avoid
  introducing floating-point time arithmetic, since that's exactly the kind of precision loss the
  existing code goes out of its way to avoid for long-running sessions.

## Pull requests

The usual path: fork the repository, branch off `main` in your fork, work there, and open the
pull request against `main`.

`dev` is the integration branch for people with write access to the repository. As an external
contributor you don't need it — branch from `main`, and your PR still applies cleanly even while
something else is in flight on `dev`.

- One thing per PR. Two small PRs beat one that also reformats unrelated files.
- Describe **why** the change is needed — for anything touching timing accuracy or the LoRa
  protocol, a concrete race-day scenario helps a lot.
- Say which board(s) you flashed and tested the change on.

## Questions

Feel free to open an issue, including for "is this intended behavior?". Please don't use an issue
for security topics — see [SECURITY.md](SECURITY.md) for that.
