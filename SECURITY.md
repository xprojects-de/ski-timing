# Security

## Reporting a vulnerability

Please **don't open a public issue**. Use the **"Report a vulnerability"** button on the
*Security* tab instead — that opens a private channel visible only to the maintainer.
Alternatively, email `infos@x-projects.de`.

Helpful in the report: which board/firmware version, what you did, what happened, and what you
expected. A reproduction path is gold.

This is a hobby project by a single developer, built in evenings between race weekends. I'll get
back to you as soon as I can, but can't promise a fixed response time. Feel free to follow up if
nothing came back after two weeks.

## What this is built for — and what it isn't

This is not a limitation we regret, it's a deliberate design decision: the start and finish units
run as a **closed pair on an isolated LoRa link, at a race course you control**. A few properties
follow from that which are fine in this context and would be dangerous in another:

- **No authentication, no encryption on the LoRa link.** Anyone with a compatible LoRa radio in
  range can listen to or spoof start/finish/sync packets.
- **The finish unit's WiFi access point and web page have no login.** They expose the recorded
  times and basic controls (reset, discard, mode switch) to anyone who joins that access point.
- **No hardening against attacks from the internet**, because it was never meant to be reachable
  from there.

That gives the one rule that actually matters:

> **Never expose the finish unit's access point or web page to the internet.** No port
> forwarding, no reverse proxy, no tunnel service. It's fine on an isolated access point at the
> finish line; made publicly reachable, it is neither secure nor appropriate for the timing data
> it holds.

A finding that only holds because someone deliberately bridged the device to the internet is an
operational mistake, not a vulnerability in the firmware — report it anyway if you think the
documentation is at fault, since that would be a real gap.

## Supported versions

Security fixes are made for the current version on the default branch. Older tagged releases are
not maintained.
