# Clarifications & Decisions Log

> Companion to the frozen [problem-and-spec.md](problem-and-spec.md) **v1.0**.
> The baseline spec records only what the brief says. **This file records everything the brief
> does _not_ settle** — every gap, the decision taken, and why.
>
> **Rule:** the baseline never changes. When a decision here contradicts a later reading of the
> brief, update the entry here, not the spec.

## Status legend

| Status | Meaning |
|--------|---------|
| **DECIDED** | Answered by the user. Binding. |
| **SPEC** | The brief actually does answer it — logged so it isn't re-litigated. |
| **ASSUMED** | My default, chosen to be defensible. Reversible on request. |
| **OPEN** | Still needs a decision. |

## Summary

| Status | Count |
|--------|-------|
| DECIDED | 15 |
| SPEC | 10 |
| ASSUMED | 16 |
| OPEN | **0** |
| **Total** | **41** |

## Locked decisions — quick reference

| ID | Decision |
|----|----------|
| **CL-001/002** | Trim **and uppercase** pickup codes, event IDs, parcel IDs. `e01` collides with `E01`. |
| **CL-009** | Report one error per offending row, for every offending row, plus an invalid-row count. |
| **CL-012** | Empty code → `INVALID_EVENT`; present-but-malformed → `INVALID_PICKUP_CODE`. |
| **CL-016** | A repeated parcel ID is **legal input** — one ARRIVE, one COLLECT is the normal lifecycle. |
| **CL-017** | Once accepted, a parcel ID stays seen for the whole run, even after collection. |
| **CL-018** | A **rejected** arrival does not mark the parcel seen; it may succeed later. |
| **CL-025** | Staleness is impossible under the batch model — the report regenerates every run. |
| **CL-030** | **C++**, CSV in → markdown report out. |
| **CL-031** | **CSV**, authored in Excel. No `.xlsx` parser. |
| **CL-039** | Header `event_id,action,parcel_id,student,pickup_code,shelf`; blank lines skipped, RFC-4180 quoting. |
| **CL-040** | Exit 0 valid · 1 validation failure · 2 I/O error. |
| **CL-041** | `handover-report.md`, overwritten silently; `--reset` deletes it. |
| **CL-038** | One `vector<Parcel>` store; `collectedSeq < 0` means pending. |
| **CL-036** | The optional shelf map (`U-9`) **is in scope**, as a derived view only. |

---

## A. Input Handling & Normalization

### CL-001 · Are pickup codes case-normalized? · **DECIDED**
**Traces:** `V-1`, `V-6`
**Q:** `V-6` demands four *uppercase* letters or digits. Is `k7m2` rejected or uppercased?
**Decision:** **Uppercase on trim.** `k7m2` → `K7M2`, accepted.
**Why:** A desk volunteer typing lowercase is a typo, not a data error. The normalization happens once at input, so all downstream comparisons see canonical codes.
**Test:** `k7m2` on E01 must still yield the `BL-3` baseline counts.

### CL-002 · Are event IDs and parcel IDs case-normalized? · **DECIDED**
**Traces:** `V-1`, `V-4`, `P-3`
**Decision:** **Yes — same uppercase-on-trim rule.** Consequence: `e01` and `E01` are the *same* ID → `DUPLICATE_EVENT_ID`. Likewise `p01` and `P01` are the same parcel for `P-3`.
**Why:** Follows CL-001; a split rule would be arbitrary and hard to defend.
**Test:** Set E06's ID to `e05` → must report `DUPLICATE_EVENT_ID`, same as `AC-5`.

### CL-003 · Is the Action field trimmed and normalized? · **ASSUMED**
**Traces:** `V-1`, `V-5`
**Q:** `V-1`'s trim list names five fields — Action is **not** among them. `V-5` demands the action be "exactly ARRIVE or COLLECT". Strictly, `" arrive "` is `INVALID_EVENT`.
**Assumption:** Trim **and** uppercase the action, consistent with CL-001/CL-002.
**Risk:** A strict grader could read `V-5` literally. Low impact — no acceptance criterion tests it.

### CL-004 · Are student names and shelf labels case-normalized? · **ASSUMED**
**Traces:** `V-1`
**Assumption:** **Trimmed only, case preserved.** `Asha` stays `Asha`, `A1` stays `A1`.
**Why:** These are display data, not match keys. Uppercasing a person's name is wrong.

### CL-005 · Whitespace-only fields · **SPEC**
**Traces:** `V-1` → `V-2`, `V-3`, `V-7`, `V-8`
`"   "` trims to empty and therefore fails the relevant non-empty rule → `INVALID_EVENT`.

### CL-006 · Length limits on student names or shelf labels · **ASSUMED**
**Traces:** `V-7`
**Assumption:** **None.** Only non-emptiness is required.

### CL-007 · Does the shelf label have a required format? · **ASSUMED**
**Traces:** `V-7`
**Assumption:** **No.** The `A1`/`B2` pattern in `BL-1` is sample data, not a contract. Any non-empty string is a valid shelf.

### CL-008 · Non-ASCII characters in names · **ASSUMED**
**Traces:** `V-1`, `V-7`
**Assumption:** **Allowed and passed through** for student and shelf. Pickup codes remain restricted to `A–Z0–9` by `V-6`.

---

## B. Validation Semantics

### CL-009 · How are multiple structural errors reported? · **DECIDED**
**Traces:** `V-9`, `V-10`
**Decision:** **One error per offending row, every offending row reported,** plus a summary count of invalid rows.
**Why:** The volunteer fixes the whole table in one pass instead of re-running once per typo.
**Note:** `V-10` is unaffected — *any* structural error still suppresses all outcomes, rows, and counts.

### CL-010 · Which error wins when one row breaks several rules? · **ASSUMED**
**Traces:** `V-2` … `V-8`
**Assumption:** Fixed precedence — check the row's own fields top to bottom, then cross-row uniqueness:

| Order | Check | Error |
|-------|-------|-------|
| 1 | `V-2` event ID non-empty | `INVALID_EVENT` |
| 2 | `V-5` action is ARRIVE or COLLECT | `INVALID_EVENT` |
| 3 | `V-3` parcel ID non-empty | `INVALID_EVENT` |
| 4 | `V-7`/`V-8` required fields present for that action | `INVALID_EVENT` |
| 5 | `V-6` pickup code format | `INVALID_PICKUP_CODE` |
| 6 | `V-4` event ID unique across table | `DUPLICATE_EVENT_ID` |

**Why:** Action is resolved early because steps 3–4 depend on it. Uniqueness is last because it is the only check needing whole-table context.

### CL-011 · On a duplicate ID, which row is flagged? · **ASSUMED**
**Traces:** `V-4`, `AC-5`
**Assumption:** **The later occurrence(s).** The first row keeps the ID; each subsequent repeat is reported.
**Why:** `AC-5` renames E06 to `E05`; naming row 6 as the offender matches how a person reads the table.

### CL-012 · Blank pickup code — `INVALID_EVENT` or `INVALID_PICKUP_CODE`? · **DECIDED**
**Traces:** `V-6`, `V-7`, `V-8`
**Q:** An empty code fails both the presence rule and the format rule.
**Decision:** **Empty → `INVALID_EVENT`** (a missing required field). **Present but malformed → `INVALID_PICKUP_CODE`.** Applies to both actions.
**Why:** Keeps `INVALID_PICKUP_CODE` meaning exactly one thing — "you typed a code, and its shape is wrong."

### CL-013 · Is a COLLECT's pickup code format-checked? · **SPEC**
**Traces:** `V-6`, `V-8`
**Yes.** `V-6` governs every pickup code, and `V-8` requires COLLECT to carry one. Note `ZZZZ` on E03 is format-**valid** — it fails later at `P-7`, as a state rejection, which is exactly why `BL-3` counts 1 rejection rather than a validation failure.

### CL-014 · Are non-blank student/shelf values on a COLLECT row validated? · **SPEC**
**Traces:** `V-8`
**No.** `V-8` says they are "not used and may be blank" → ignored entirely, valid or not.

### CL-015 · Is an empty event table valid? · **SPEC**
**Traces:** `V-11`, `AC-4`
**Yes** — valid, zero counts, no handover rows.

### CL-016 · Is a repeated parcel ID a structural error? · **DECIDED**
**Traces:** `V-4`, `P-3`, `P-6`
**Decision:** **No.** `V-4` requires uniqueness for **event IDs only**.
**Why:** A parcel ID is *expected* to appear twice — once for its ARRIVE and once for its COLLECT. That is the normal lifecycle, not an error. In `BL-1`, `P01` and `P02` each appear exactly this way.
**Consequence:** Repeats are judged at *processing* time by `P-3` / `P-6`, never at validation time. Validating parcel uniqueness would reject the built-in log outright.

---

## C. Processing Semantics

### CL-017 · Is a parcel "unseen" again after collection? · **DECIDED**
**Traces:** `P-3`, `P-8`
**Decision:** **No — once seen, always seen for the rest of the run.** A parcel ID arriving again after being collected reports `PARCEL_ALREADY_SEEN`.
**Why:** A parcel ID is meant to identify one physical parcel uniquely. A second ARRIVE for an ID already handled means someone reused an ID for a *different* parcel — precisely the mistake `PARCEL_ALREADY_SEEN` exists to surface. Deliberate choice, and consistent with `P-8` freeing only the **code**.
**Note:** The seen-set clears only between runs, per `P-1`.

### CL-018 · Does a *rejected* ARRIVE mark the parcel as seen? · **DECIDED**
**Traces:** `P-3`, `P-4`
**Decision:** **No.** `P-3` says "earlier **accepted** arrival", and both rejection paths state "no state change".
**Consequence:** A parcel rejected by `P-4` for a code collision can arrive successfully later, once that code is freed by a collection. Pairs with CL-017: **acceptance** is what marks a parcel seen, and that mark then never clears.

### CL-019 · Can a collected parcel's code be reused by a new arrival? · **SPEC**
**Traces:** `P-4`, `P-8`
**Yes.** `P-8` makes the code inactive and `P-4` tests only **pending** parcels.

### CL-020 · COLLECT quoting a code that belongs to a different pending parcel · **SPEC**
**Traces:** `P-6`, `P-7`
Lookup is by **parcel ID first**. If that parcel is pending and the code differs → `PICKUP_CODE_MISMATCH`, regardless of which other parcel owns the code. The other parcel is never collected by accident.

### CL-021 · Can two pending parcels share a code? · **SPEC**
**Traces:** `P-4`
**No** — `P-4` prevents it, so among pending parcels the code → parcel mapping is unique.

### CL-022 · COLLECT on an already-collected parcel · **SPEC**
**Traces:** `P-6`
`PARCEL_NOT_PENDING` — it left pending state at `P-8`.

### CL-023 · Can one event produce more than one rejection? · **ASSUMED**
**Traces:** `P-10`
**Assumption:** **No.** Exactly one outcome per event; the first failing check short-circuits. So the rejected count can never exceed the event count.

### CL-024 · ARRIVE check order · **SPEC**
**Traces:** `P-3`, `P-4`
Seen-check first, then collision — `P-4` opens with "Otherwise". A re-arrival that *also* collides reports `PARCEL_ALREADY_SEEN`.

---

## D. Output & Report

### CL-025 · Stale results after the event table is edited · **DECIDED**
**Traces:** `U-8`
**Decision:** Keep the previous results visible, marked stale, until the next run.
**Note — satisfied by construction:** under the batch model (CL-030) the report is regenerated from the input file on every run, so output can never disagree with input. The stale state cannot occur. Logged because it becomes live again if the project ever moves to an interactive UI.

### CL-026 · Does the pending board show pickup codes? · **ASSUMED**
**Traces:** `U-4`
**Assumption:** **Yes** — parcel ID, student, shelf, and code. The incoming volunteer needs the code to hand the parcel over.

### CL-027 · What does the validation message say on success? · **ASSUMED**
**Traces:** `U-6`
**Assumption:** An explicit line — `VALID — N events accepted` — never blank. Silence is indistinguishable from a crash.

### CL-028 · Are collected parcels listed in the report? · **ASSUMED**
**Traces:** `O-3`, `U-5`
**Assumption:** **Yes, as their own section.** `O-3` mandates an order for them, which is meaningless unless they are displayed.

### CL-029 · Output shape for an empty log · **SPEC**
**Traces:** `V-11`, `AC-4`
Valid message, no outcome rows, no handover rows, counts `0 / 0 / 0`.

---

## E. Delivery Model — CLI Adaptation

### CL-030 · Technology stack · **DECIDED**
**Traces:** `SC-2`, `SC-4`
**Decision:** **C++.** CSV of events in → rules engine → markdown report out.
**Standing:** Explicitly permitted — `SC-4` allows "a CLI that produces a clear visual or tabular report."
**Why:** Confident authorship in the chosen language is a stated evaluation criterion, and the batch model satisfies `U-8` by construction (see CL-025).

### CL-031 · Input file format · **DECIDED**
**Traces:** `SC-4`, `U-1`
**Decision:** **CSV** — authored in Excel, saved as `.csv`.
**Why:** An `.xlsx` is a ZIP of XML and needs a third-party parser (xlnt / OpenXLSX) plus build configuration — code unrelated to the problem. CSV parsing is a few lines of `std::getline` with zero dependencies. The Excel editing workflow is unchanged.

### CL-032 · How is the "editable event table" satisfied? · **ASSUMED**
**Traces:** `U-1`
**Assumption:** The input file **is** the editable table — edited in Excel or any editor. `AC-2`/`AC-3` become "change one cell, re-run".

### CL-033 · How is Reset satisfied? · **ASSUMED**
**Traces:** `U-7`
**Assumption:** A `--reset` flag rewrites the input file with the six built-in `BL-1` events and clears the previous report, leaving no outcomes or counts until a normal run.

### CL-034 · How is "one action" satisfied? · **ASSUMED**
**Traces:** `AC-1`, `U-2`
**Assumption:** A single command run = one Run Handover. Load, validate, process, and report happen in that one invocation.

### CL-035 · Where does output go? · **ASSUMED**
**Traces:** `SC-2`, `U-3`–`U-6`
**Assumption:** A markdown report file (outcomes, pending board, collected list, counts, validation message) **plus** a short console summary. Rendered markdown preview serves as the "attractive report".

### CL-036 · Optional shelf map · **DECIDED — in scope**
**Traces:** `U-9`
**Decision:** **Build it.** A compact shelf map, grouped by shelf label, rendered in the report.
**Constraint:** `U-9` forbids a second parcel store — the map must be a derived view over the same final pending list, computed at render time.

### CL-037 · Test approach · **ASSUMED**
**Traces:** `AC-6`, `PR-3`
**Assumption:** A separate test binary with a minimal assertion helper — no framework dependency — covering the six `AC-6` checks. Requires the rules engine to be separable from file I/O.

---

### CL-038 · Core data structures · **DECIDED**
**Traces:** `P-1`, `P-3`, `P-4`, `P-6`, `P-8`, `O-2`, `O-3`, `U-9`
**Decision:** A **single parcel store**, plus a vector for the event log.

```cpp
struct Event  { std::string id, action, parcelId, student, code, shelf; int row; };

struct Parcel {
    std::string id, student, code, shelf;
    int collectedSeq = -1;                                  // <0 = pending
    bool isPending() const { return collectedSeq < 0; }
};

std::vector<Event>  events;        // source order = P-2, O-1
std::vector<Parcel> parcels;       // arrival order; index = arrival seq (O-2)
int nextCollectSeq = 0;
```

**Why one store:**
- Membership in `parcels` **is** the seen-set — collected parcels are never removed — so CL-017 is structural, not remembered.
- `U-9` forbids a second parcel store; with one store the shelf map cannot become one.
- No `erase`, so no index or iterator invalidation.

**Rejected alternatives:**
| Alternative | Why not |
|---|---|
| Separate `pending` + `collected` + `seen` containers | Three stores to keep consistent; CL-017 becomes a remembered rule |
| `vector` + `unordered_map` index on id/code | Every removal invalidates the index; pointless at N≈6 |
| `map` / `set` for pending | Sorts by key, destroying `O-2` |
| `status` enum **and** `collectedSeq` | Two fields, one fact — they can contradict. `collectedSeq < 0` is the single source of truth |
| Explicit `arrivalSeq` field | Redundant: each parcel is appended exactly once, so the vector index already is it |

**Implementation constraints — each maps to a rule:**
| Constraint | Rule |
|---|---|
| Append **only** on an accepted ARRIVE; rejected arrivals must not enter the vector | `P-3`, `P-4`, CL-018 |
| `find(id)` answers "seen?"; `findPending(id)` answers "pending?" — keep them distinct | `P-3` vs `P-6` |
| The code-collision scan **must** filter `isPending()`, or a collected parcel's freed code wrongly collides | `P-4`, CL-019, `AC-3` |
| Collect sets `collectedSeq = nextCollectSeq++`; the parcel stays in place | `P-8` |
| `O-3` output = filter collected, sort by `collectedSeq` (vector is in arrival order) | `O-3` |
| `P-1` reset = clear `parcels` and `nextCollectSeq` at the start of every run | `P-1` |
| Normalization (trim + uppercase) happens once at CSV load, so all comparisons are plain `==` | CL-001, CL-002 |

**Shelf map (`U-9`):** grouped at render time over the pending subset of `parcels`. No stored grouping.

---

### CL-039 · CSV format contract · **DECIDED**
**Traces:** `U-1`, CL-031
**Decision — header is required and exact:**
```
event_id,action,parcel_id,student,pickup_code,shelf
```
| Case | Behaviour |
|---|---|
| Header missing, misspelled or reordered | I/O error, exit 2 (CL-040) — not a validation failure |
| Blank line, trailing or interior | **Skipped.** Excel appends one; it carries no fields |
| Row of empty fields (`,,,,,`) | **An event row** → `INVALID_EVENT`. Distinct from a blank line |
| Wrong column count (too few or too many) | `INVALID_EVENT` on that row, field "Row" — keeps per-row reporting (CL-009) |
| Quoted field containing a comma | Supported — minimal RFC 4180: `"..."`, with `""` as an escaped quote |
| Line endings | LF and CRLF both accepted; a trailing `` is stripped |
| UTF-8 BOM | Stripped if present |

**Why quoting is required, not optional:** CL-007 allows any shelf label, and TC-037 uses
`"Back room, top rack"`. Without quote handling that row would split into seven columns.

### CL-040 · Missing or unreadable input file · **DECIDED**
**Traces:** CL-031, CL-039, `V-10`
**Decision:** Message to **stderr**, no report written, distinct exit code.

| Exit | Meaning |
|---|---|
| **0** | Ran and validated — including runs with state rejections |
| **1** | Structural validation failure (`V-10`) |
| **2** | I/O error — file missing, unreadable, or bad header |

**Why separate:** a validation failure is a *result* the volunteer must act on; an I/O error means
nothing ran at all. Merging them would make a typo'd filename look like a bad event log.

### CL-041 · Report output path and overwrite · **DECIDED**
**Traces:** CL-035, CL-033, `U-7`
**Decision:**
- Report defaults to `handover-report.md`; `--out <path>` overrides.
- Overwrites silently — the report is derived output, never a record to preserve.
- `--reset` rewrites the input CSV with the six `BL-1` events, creating it if absent, and **deletes
  any existing report** so no stale outcomes, rows or counts survive, per `U-7`.

---

## Traceability Matrix

| Spec ID | Clarifications |
|---------|----------------|
| `SC-2` | CL-030, CL-035 |
| `SC-4` | CL-030, CL-031 |
| `V-1` | CL-001, CL-002, CL-003, CL-004, CL-005 |
| `V-2` | CL-005, CL-010 |
| `V-3` | CL-005, CL-010 |
| `V-4` | CL-002, CL-010, CL-011, CL-016 |
| `V-5` | CL-003, CL-010 |
| `V-6` | CL-001, CL-010, CL-012, CL-013 |
| `V-7` | CL-005, CL-006, CL-007, CL-008, CL-012 |
| `V-8` | CL-005, CL-012, CL-013, CL-014 |
| `V-9` | CL-009 |
| `V-10` | CL-009 |
| `V-11` | CL-015, CL-029 |
| `P-3` | CL-002, CL-016, CL-017, CL-018, CL-024, CL-038 |
| `P-4` | CL-018, CL-019, CL-021, CL-024, CL-038 |
| `P-6` | CL-016, CL-020, CL-022, CL-038 |
| `P-7` | CL-020 |
| `P-8` | CL-017, CL-019, CL-038 |
| `P-10` | CL-023 |
| `O-2` | CL-038 |
| `O-3` | CL-028, CL-038 |
| `U-1` | CL-031, CL-032 |
| `U-2` | CL-034 |
| `U-4` | CL-026 |
| `U-6` | CL-027 |
| `U-7` | CL-033 |
| `U-8` | CL-025 |
| `U-9` | CL-036, CL-038 |
| `AC-1` | CL-034 |
| `AC-4` | CL-015, CL-029 |
| `AC-5` | CL-002, CL-011 |
| `AC-6` | CL-037 |

---

## Open Items

**None.** All 41 items are resolved — 15 decided by the user, 10 settled by the brief, 16 held as
stated assumptions. Assumptions are reversible; raise the CL id to change one.
