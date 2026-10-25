# Review base - shared by the review skills

The review skills (`ski-timing-code-review`, and any scoped one added later) each define **what** they review: their
parts, paths and key questions. This file defines **how**: the rules, the decision register, what counts as a finding,
verification, the budget and the report. A skill that includes this file follows every rule here unless it says
otherwise.

The goal of a review is a **small, reliable set of changes**, not a complete list of everything that could be better.
Fewer findings that all hold beat many findings that need sorting. This firmware runs at real races: a wrong finding
costs the maintainer an evening, a missed timing bug costs a club its results.

## 1. Rules

- **Current working tree, never a diff.** Ignore `<PR#>`, branch, `--comment`, `--fix` or `--post` arguments and say
  that the skill always reviews the current working tree (uncommitted changes included).
- **Read-only.** No code changes, no commits, no flashing, no serial port, no `arduino-cli upload`. A board that may
  be plugged in is the developer's; never touch it.
- **The yardstick** is [CLAUDE.md](../CLAUDE.md) (how the system works, the message protocol, the code style),
  [README.md](../README.md) (hardware, board settings, setup) and [SECURITY.md](../SECURITY.md) (what the system
  promises and deliberately does not do: open LoRa link, access point without login, never on the internet). A
  deviation from them is a finding; a rule the code has outgrown is a suggestion to update the file that states it.
- **Agents** are spawned in a single assistant turn with `subagent_type: general-purpose` and
  `run_in_background: false`. Each gets a self-contained prompt (it has no conversation context) that contains the
  skill's "What this does", its own part with paths and special care, the skill's key questions and failure cases, and
  sections 2 to 5 of this file verbatim.
- **Ground truth**: there is no test suite and no simulator (CONTRIBUTING.md). If `arduino-cli` is installed
  (`which arduino-cli`) and the `Heltec-esp32` core is present (`arduino-cli core list`), compile both sketches in the
  background in the same turn as the agents - `arduino-cli compile --fqbn <the Heltec WiFi LoRa 32 V4 FQBN from
  arduino-cli board listall> start` and the same for `finish` - into a build folder under the scratchpad, never
  `build/` in the repository. A sketch that does not compile is a finding (Hoch). Without `arduino-cli` say so in the
  report ("nicht kompiliert") and treat compile correctness as reviewed by reading only.

## 2. The decision register

[`docs/review/decisions.md`](../docs/review/decisions.md) lists what the user has decided to accept (E-numbers).
Every agent reads it before reviewing.

- A finding that an entry covers is **not reported**. The agent counts it under "vom Register abgedeckt: E01".
- Only if the entry's premise no longer holds (the code changed so the reason is gone, or the consequence is worse
  than the entry says) the finding is reported, marked **"Entscheidung prüfen (E01)"**, with what changed.
- Decisions in CLAUDE.md, README.md and SECURITY.md ("by design", "deliberately", "decided", "not done on purpose")
  count the same as register entries. In particular: no authentication and no encryption on the LoRa link, the access
  point and web page without login, `ST-` as the only packet filter, and the duplicated modules in `start/` and
  `finish/` (two independent Arduino sketches, no shared library) are documented decisions, not findings.

## 3. No hallucination

A review that reports something the code does not do costs the user more than a finding it misses. Therefore:

- **Only what you read.** Every claim about the code rests on lines you read in this review, cited as `path:line`.
  Never a function, constant, pin, queue depth or threshold from memory, from its name or from "how such firmware
  usually looks". Before citing, quote the lines to yourself; a `path:line` that does not show what you claim is a
  wrong finding.
- **Follow the call, don't guess it.** A finding that depends on a caller, a guard, a queue, a cooldown or a later
  step names where you checked it (`grep` for the callers, read them). "Probably not checked elsewhere" is not a
  finding.
- **The two sketches are separate.** `start/lora.cpp` and `finish/lora.cpp` differ on purpose; a claim about one is not
  a claim about the other. Cite the copy you read.
- **Libraries are not guessed either.** Behaviour of the Heltec `LoRaWan_APP` radio driver, `esp_timer`, FreeRTOS
  queues and tasks on ESP32-S3, LittleFS, ESPAsyncWebServer/AsyncTCP, deep sleep and ext0 wake-up that a finding
  depends on is either visible in the project (code, comment, README) or marked as **Annahme** with what would confirm
  it (a datasheet, a library source line, a measurement on the board). An Annahme is at most Mittel until confirmed.
- **Hardware behaviour is not guessed.** Switch bounce, light-barrier output timing, LoRa air time at SF10, clock
  drift of the ESP32 crystal: give the figure you rely on and its source, or mark it Annahme.
- **Unsure is a valid answer.** What you cannot settle from the code goes into a short list "Offen/unklar" with the
  question and where to look (or what to measure on the board) - never into the findings.
- **"Nothing to find" is a valid result** for a part or a key question. Do not produce findings to fill a list. Say
  briefly what is solid, so well-built parts are not touched.

## 4. No over-optimisation

The goal is lean, readable firmware (CLAUDE.md "Code style"). A review that recommends abstractions, a shared library,
configuration options, a protocol redesign or a "proper" RTOS architecture for a case nobody has makes the code worse.

- **Damage today, not in theory.** A finding needs a concrete scenario with today's code and use (or a use certain to
  come): a race with up to 3 racers on course, sessions of several hours on battery, two units a few hundred metres
  apart, helpers pressing buttons. "Could overflow after 584,000 years of µs" is not a finding; "the 3-deep start queue
  silently drops the fourth racer's start" is (if the code does that).
- **This limits what gets recommended, never what gets looked at.** ISR safety, FreeRTOS concurrency (shared state
  between `loop()`, `LoraTask` and the battery task), timing precision, integer overflow and signedness in time
  arithmetic, heap fragmentation over long uptimes, flash wear, deep-sleep and wake-up paths, and the web server's
  behaviour under a helper's phone reloading the page are examined in full depth.
- **The smallest fix that removes the damage.** Prefer deleting or simplifying code over adding layers; one guard, one
  critical section or one `static_assert` over a new mechanism. No abstraction in advance, no shared library between
  the sketches, no configuration option nobody asked for.
- **Weigh value against cost.** When a fix costs more complexity than the damage is worth, say so: "Fix lohnt sich
  derzeit nicht" - the finding then goes to "Bewusst nicht empfohlen", not into the main list.
- **Severities are not inflated.** A papercut is Niedrig, even when there are many of them. No pure style findings:
  naming, formatting, a German log line or "could be more elegant" only count when a rule of CLAUDE.md is broken, and
  then as Niedrig.

## 5. What counts as a finding

- **Concrete failure scenario**: which trigger, timing, packet loss, button sequence, reboot, battery state or web
  request leads to which wrong behaviour - said as it would happen at the race. "Could be a problem" is not a finding.
- **Verified in the code** (section 3), with `path:line`.
- **No findings outside the skill's scope**, except a **Kritisch** one, which goes into a separate short list.
- **Severities** (a skill makes them more specific for its scope):
  - **Kritisch**: a recorded time that is wrong without anyone noticing (wrong offset applied, start matched to the
    wrong racer, elapsed time computed in the wrong time domain, a stored value truncated or overflowed); recorded
    times lost (CSV cleared or overwritten by a normal action, LittleFS append silently failing); a unit that locks up,
    resets or sleeps during a race (watchdog, ISR fault, deadlock between tasks, unintended deep sleep); the finish
    unit writing outside `/times.csv` or the web server crashing the unit on a normal request.
  - **Hoch**: a start or finish trigger silently ignored under normal use (debounce too long, queue full without
    display feedback, burst cooldown swallowing a real event); sync never completing or completing with a wrong
    median; a feature of the button or web UI broken; a sketch that does not compile; a resource exhausted by a
    normal race day (heap, TX/RX queue, flash).
  - **Mittel**: wrong or inconsistent behaviour with limited or self-repairing consequence (display shows a stale
    value for a moment, a log line lies, a 404 where a 400 was meant).
  - **Niedrig**: a papercut.
- For every finding: `path:line`, part, severity, failure scenario (one sentence), suggested fix (one sentence),
  `renntagsrelevant: true/false` (does it hurt at a race), and the size of the fix: **S** (a few lines in one file),
  **M** (one module, both sketches if duplicated), **L** (protocol change or several modules).

## 6. Verification and budget (synthesis)

1. **Pool and dedupe** the agents' findings (keep the more precise `path:line`; a finding in a duplicated module is
   one finding naming both copies), drop what the register covers.
2. **Refute before reporting.** Spawn one more agent, the *verifier*, with every Kritisch, Hoch and Mittel candidate
   (path:line, scenario, claimed severity) and sections 3 and 4 of this file. Its only task is to **try to refute**
   each one by reading the code: the cited lines do not say that; a guard, cooldown, critical section, queue check or
   later step prevents the scenario; a premise does not hold (wrong sketch cited, wrong unit of time); the fix is out of
   proportion. It answers per finding `bestätigt` / `widerlegt` (with the preventing line) / `überzogen` (suggested
   severity) / `unverhältnismäßig` (to "Bewusst nicht empfohlen"). Drop the refuted, adjust the rest. **Read every
   remaining Kritisch and Hoch yourself** at its cited lines before it goes into the report.
3. **Budget**:
   - **Main list**: at most **10** findings, only Kritisch and Hoch, most severe first.
   - **Anhang**: Mittel and Niedrig as one-liners, at most 10 each. More are cut; say how many.
4. **Fix packages**: group the main list (and Mittel findings that belong to the same code) into **at most three
   packages**. Each package is self-contained (one change, one PR): title, findings it fixes, files (both copies when a
   duplicated module is touched), how to verify it on the board (the concrete button sequence or race scenario to try,
   what the serial log and display must show), size S/M/L. Order by value: what prevents the worst wrong time per
   effort first.

## 7. Report (German)

In this order, headings in German, identifiers as they are, clickable `path:line` relative to the repository root:

1. **Überblick**: parts reviewed (and what the user chose), compile result (or "nicht kompiliert"), findings per
   severity before and after verification (how many refuted, downgraded, moved to "Bewusst nicht empfohlen"), how many
   the register covered (`E01 ×2`), two or three sentences on the state including what is solid.
2. The skill's own sections (answers to its key questions, tables, walkthroughs).
3. **Befunde** (main list, max. 10).
4. **Fix-Pakete** (max. 3), each with its board test.
5. **Anhang**: Mittel, Niedrig, "Entscheidung prüfen" entries, out-of-scope Kritisch findings, "Offen/unklar" (with
   what to measure on the board where code reading cannot settle it).
6. **Bewusst nicht empfohlen**: one line each with the reason, so the user sees what was weighed and why it stays.
7. **Kandidaten fürs Register**: findings the user will likely accept as intended (a deliberate trade-off, a documented
   limit), each as a ready-to-paste entry in the format of `docs/review/decisions.md` (English). What produces a wrong
   recorded time, loses recorded times or stops a unit during a race is never a candidate - it is fixed.

Do not fix anything unasked (CLAUDE.md "Git": never commit either). End by asking which fix package to work on. When the
user answers that a finding is intended, offer to add it to the register.
