# Implementation Plan — `PR-2`

> Satisfies **`PR-2`**: a short plan of ordered steps with useful checkpoints, written **before**
> implementation. Traces to frozen [problem-and-spec.md](problem-and-spec.md) v1.0,
> [clarifications.md](clarifications.md) (41/41 resolved), and [test-cases.md](test-cases.md) (98 scenarios).
>
> **Status:** **Complete.** All five steps done; **98 / 98 scenarios green, none vacuous.**
> `AC-1` – `AC-6` verified end to end against the built binary, with exit codes 0, 1 and 2 all
> demonstrated.
> Tests were authored **before** any implementation, so none of them can have been fitted to code.

---

## Target shape

C++17, no third-party dependencies. The one architectural rule, forced by **CL-037**: the rules
engine must be compilable and testable **without any file I/O**, so `<fstream>` appears in exactly
one translation unit.

```
src/
  model.h        Event, Parcel, Outcome, RunResult   CL-038
  normalize.*    trim + uppercase                    V-1, CL-001–CL-004
  validate.*     whole-table structural checks       V-2 – V-11, CL-009 – CL-016
  engine.*       replay + ordering                   P-1 – P-10, O-1 – O-3
  report.*       markdown renderer + shelf map       U-3 – U-6, U-9, CL-026 – CL-029
  csv.*          loader — the ONLY TU with <fstream> CL-031, CL-039
  main.cpp       CLI, flags, exit codes              CL-034, CL-040, CL-041
tests/
  test_main.cpp  minimal assert helper, no framework CL-037
```

The seam that makes this work:

```cpp
std::vector<RowError> validate(const std::vector<Event>&);   // empty == valid
RunResult             run(const std::vector<Event>&);        // pure; P-1 clears state
```

`validate` and `run` never touch the filesystem. Every scenario in sections A–E and G of
[test-cases.md](test-cases.md) is reachable through those two calls alone.

---

## Step 1 — Foundation

Build system, `model.h` exactly as fixed in **CL-038**, `normalize`, and the test harness.

- `Event`, `Parcel` with `collectedSeq = -1` and `isPending()`. One store, no `status` enum.
- `trim` + `upper` for event ID, parcel ID, pickup code, action; **trim only** for student and
  shelf (CL-004 — never uppercase a person's name).
- Assertion helper: a `CHECK(cond, msg)` macro with a failure counter and a non-zero exit.

**Checkpoint** — `cmake --build build && ./build/run_tests` builds clean and registers all **98**
scenarios. Normalization and the `CL-038` derivations are provable in isolation: `k7m2` → `K7M2`,
`e01` → `E01`, `Asha` unchanged, `Zoë` preserved; `pending()` in arrival order, `collected()` in
`collectedSeq` order, `rejectedCount()` over exactly the four `P-10` outcomes.

---

## Step 2 — Rules engine

The core, and the highest-risk step — build it second so the subtle decisions get the most runway.

`run()` replays a pre-validated, normalized event vector in source order (`P-2`), appending to
`parcels` **only** on an accepted ARRIVE, and setting `collectedSeq = nextCollectSeq++` on collect.

Three places this goes wrong if written carelessly:

| Trap | Rule | Caught by |
|---|---|---|
| Code-collision scan not filtered by `isPending()` — a collected parcel's freed code wrongly blocks a new arrival | `P-4`, CL-019 | `AC-3` / `TC-003` |
| Rejected arrival appended to `parcels`, marking the parcel seen | `P-3`, CL-018 | `TC-055` |
| Seen-check placed after the collision check | CL-024 | `TC-054` |

`P-3` asks "in `parcels` at all?"; `P-6` asks "in `parcels` **and** pending?" — two distinct
lookups, deliberately not merged.

**Checkpoint** — sections C, D, E and G green (`TC-050` – `TC-073`, `TC-100` – `TC-103`, 27
scenarios). `AC-1` provable at unit level from a hardcoded `BL-1`: six exact outcomes, pending
`P01, P03, P04`, counts **3 / 1 / 1**.

---

## Step 3 — Validator and run gate

Whole-table structural validation, in the fixed precedence of **CL-010** — row-local checks top to
bottom, then cross-row uniqueness last, since it is the only check needing whole-table context.
One error per offending row, every offending row reported (**CL-009**), later duplicate flagged
rather than the first (**CL-011**).

Then wire the gate: any `RowError` ⇒ **no** outcomes, **no** handover rows, **no** counts (`V-10`).

The two easy mistakes here are both already decided: an empty code is `INVALID_EVENT`, not
`INVALID_PICKUP_CODE` (**CL-012**), and a repeated *parcel* ID is legal input — validating it
would reject the built-in log outright (**CL-016**).

**Checkpoint** — section B green (`TC-010` – `TC-041`, 32 scenarios). `AC-4` empty table is valid
with `0 / 0 / 0`; `AC-5` duplicate event ID yields `DUPLICATE_EVENT_ID` and suppresses everything
downstream. Engine and validator are now feature-complete with zero file I/O written so far.

---

## Step 4 — CSV loader

The only step that opens a file. Exact header required (**CL-039**); missing, misspelled or
reordered ⇒ I/O error and exit 2, *not* a validation failure. Blank lines skipped; a row of empty
fields is an event row that fails validation. Minimal RFC 4180 quoting, because CL-007 permits a
shelf label like `"Back room, top rack"`. Strip UTF-8 BOM and trailing `\r`.

Normalization runs once, here, so every downstream comparison is a plain `==`.

**Checkpoint** — `TC-110` – `TC-119` green. A CSV round-trips into the same event vector Step 2
tested by hand, and `BL-1` on disk reproduces the **3 / 1 / 1** result end to end.

---

## Step 5 — Report, CLI, shelf map

Markdown renderer: outcomes table (`U-3`), pending board with codes (`U-4`, CL-026), collected list
in collection order (`U-5`, CL-028), counts, and an explicit `VALID — N events accepted` line that
is never blank (**CL-027**).

Shelf map (`U-9`, CL-036) grouped at render time over the pending subset — **no second store**,
which is the entire reason `U-9` was allowed in scope.

CLI: `handover [input.csv] [--out <path>] [--reset]`. One invocation = one Run Handover (`CL-034`).
Exit **0** valid · **1** validation failure · **2** I/O error (**CL-040**). `--reset` rewrites the
input with the six `BL-1` events and deletes any existing report, so no stale rows survive (`U-7`,
CL-041).

**Checkpoint** — sections F and H green (`TC-080` – `TC-093`, `TC-120` – `TC-128`). All **98**
scenarios pass. `AC-1` – `AC-6` demonstrated end to end: run the built-in log, edit one cell for
`AC-2` and `AC-3`, empty the table for `AC-4`, duplicate an ID for `AC-5`.

---

## Test discipline

All 98 scenarios were written from [test-cases.md](test-cases.md) **before** any step was
implemented, against an API seam declared in headers only. That ordering is the guarantee the
tests describe the document rather than the code.

```
cmake -S . -B build && cmake --build build && ./build/run_tests
./build/run_tests TC-055        # one scenario
./build/run_tests "C. Proc"     # one section
```

Scenario Outlines are single scenarios that loop over their Examples table, keeping the count at
the document's 98 rather than the 112 expanded rows.

**Caveat on vacuous passes.** At the Step 1 baseline, 12 scenarios passed against empty stubs
because they assert an *absence* — no errors, no pending rows, an empty shelf map — which a stub
satisfies trivially. Step 2 unmasked four of them: `TC-033` flipped pass → **fail** the moment the
engine started producing the outcomes that the still-missing `V-10` gate is supposed to suppress,
and `TC-004`, `TC-065`, `TC-080` became genuine. Eight remain vacuous — `TC-023`, `TC-035`,
`TC-036`, `TC-037` (they read "valid", and `validate()` still returns nothing), `TC-091`, `TC-092`
(the shelf map is still an empty stub), and `TC-111`, `TC-121` (they expect exit 2, and the
unimplemented CLI exits 2 for every input). Track this column, not just the total.
>
> Step 3 resolved four more: `TC-023`, `TC-035`, `TC-036` and `TC-037` now read "valid" from a real
> validator. Step 4 resolved the parser half of `TC-111`. Step 5 resolved the last four — `TC-091`
> and `TC-092` now read a real shelf map, and `TC-111` and `TC-121` a real CLI whose exit 2 is
> earned rather than blanket. **At 98 green, none are vacuous.**

## Order rationale

Risk first, I/O last. Steps 1–3 deliver the complete rules engine with no filesystem dependency,
so 59 of the 98 scenarios are green before a single file is opened. That inversion — engine before
loader — is what keeps `PR-3`'s "show test evidence" cheap: the interesting behaviour is unit-testable,
and Steps 4–5 only prove the plumbing around it.

## Deviation log

Changes to this plan get recorded here with the reason, per `PR-3` ("explain changes to it").

| Date | Step | Change | Why |
|---|---|---|---|
| 2026-08-23 | 1 | `handover.cpp` — the normalize → validate → gate → replay wiring — built in Step 1 rather than Step 3 | It is the composition root of the pure core and the seam every scenario calls. Without it the suite could not be written before implementation, which was the point. The `V-10` gate it contains stays unproven until Step 3 supplies a real `validate()`. |
| 2026-08-23 | 1 | Step 1 checkpoint corrected: `TC-100` – `TC-103` moved to Step 2 | Those scenarios call `run()`, so they cannot go green until the engine exists. The original checkpoint was wrong. |
| 2026-08-23 | 3 | CL-010's precedence ladder gained a step 0: `malformedRow` is checked before every field rule | A wrong column count means no field can be trusted. `TC-115` requires field `"Row"` on a row whose pickup code would otherwise read as blank and report `"Pickup code"`. |
| 2026-08-23 | 5 | Shelf-map groups are ordered by the arrival of their first parcel, not alphabetically | `CL-036` states the grouping but no order. Deriving it from `O-2` avoids inventing a second ordering rule the spec never gave. |
| 2026-08-23 | 5 | A validation failure still writes a report — one containing only the error table | `V-10` requires clearing output from an earlier run, and `CL-025` regenerates the report every run. An error-only report satisfies both; leaving a stale success report on disk would not. |
