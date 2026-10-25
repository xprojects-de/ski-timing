# Review decisions

What is listed here is deliberately so and must not come up as a finding in any code review again. The review skills
(`ski-timing-code-review` and any scoped one following [.claude/review-base.md](../../.claude/review-base.md)) read
this file: a finding an entry covers is not reported. Only when the **reason** of an entry no longer holds does a
review report it as "Entscheidung prüfen (E…)".

Decisions written in `CLAUDE.md`, `README.md` or `SECURITY.md` ("by design", "deliberately", "decided", "not done on
purpose") count the same and need not be repeated here - among them: no authentication or encryption on the LoRa link,
the finish unit's access point and web page without login, and the duplicated modules in `start/` and `finish/`.

**Upkeep:** an entry is added when the user deliberately accepts a finding (a review ends with "Kandidaten fürs
Register"). Only scope, presentation or a deliberately accepted limit belongs here. Whatever produces a wrong recorded
time, loses recorded times or stops a unit during a race is fixed - never entered. Entries are never renumbered. An
outdated entry gets the status "revoked" and stays.

An entry consists of a number, the area, the decision, the reason and what would revoke it:

```markdown
### E01 · <short title>
- **Area:** <area, affected reviews>
- **Decision:** <what is deliberately so>
- **Reason:** <why>
- **Revoked when:** <what would make the reason fail>
- **Source:** <file/section, if any>
```

## Entries

(none yet)
