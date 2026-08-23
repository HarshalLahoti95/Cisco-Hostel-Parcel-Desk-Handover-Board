# Test Cases — BDD / Gherkin

> Traces to frozen [problem-and-spec.md](problem-and-spec.md) v1.0 and [clarifications.md](clarifications.md).
> Every scenario names the spec IDs and CL decisions it proves. Coverage matrix at the end.
> **98 scenarios** (112 expanded, counting Scenario Outline examples). No open decisions remain.
> The `@`-tags on each scenario are authoritative; the coverage matrix is derived from them.

## Conventions

The built-in log `BL-1` is referred to as **the built-in log**. Unless a scenario says otherwise,
counts are written **pending / collected / rejected**.

```gherkin
Background:
  Given the handover engine starts with empty parcel, seen, and collected state    # P-1
```

---

## A. Acceptance Criteria

```gherkin
@AC-1 @BL-1 @BL-2 @BL-3
Scenario: TC-001 The built-in log runs in one action
  Given the built-in event log
  When I run the handover
  Then validation reports VALID
  And the outcomes in source order are:
    | E01 | ARRIVED              |
    | E02 | ARRIVED              |
    | E03 | PICKUP_CODE_MISMATCH |
    | E04 | ARRIVED              |
    | E05 | COLLECTED            |
    | E06 | ARRIVED              |
  And the pending board lists P01 on A1, P03 on A2, P04 on B2 in that order
  And the collected list is P02
  And the summary is 3 / 1 / 1

@AC-2 @U-1
Scenario: TC-002 Correcting E03's pickup code collects P01
  Given the built-in event log
  And E03's pickup code is changed to "K7M2"
  When I run the handover
  Then E03's outcome is COLLECTED
  And the pending board lists P03, P04 in that order
  And the summary is 2 / 2 / 0

@AC-3 @P-4 @U-1
Scenario: TC-003 E06 reusing an active code collides
  Given the built-in event log
  And E06's pickup code is changed to "T9C4"
  When I run the handover
  Then E06's outcome is ACTIVE_CODE_COLLISION
  And the pending board does not contain P04
  And the pending board lists P01, P03 in that order
  And the summary is 2 / 1 / 2

@AC-4 @V-11 @CL-015 @CL-029
Scenario: TC-004 An empty event table is valid
  Given an empty event table
  When I run the handover
  Then validation reports VALID
  And there are no event outcomes
  And the pending board is empty
  And the collected list is empty
  And the summary is 0 / 0 / 0

@AC-5 @V-4
Scenario: TC-005 A duplicate event ID suppresses the whole run
  Given the built-in event log
  And E06's event ID is changed to "E05"
  When I run the handover
  Then validation reports DUPLICATE_EVENT_ID for event "E05" on field "Event ID"
  And there are no event outcomes
  And the pending board is empty
  And no summary counts are shown

@AC-6 @P-2 @O-1
Scenario: TC-006 Processing follows row order, not event-ID order
  Given the event table:
    | row | id  | action  | parcel | student | code | shelf |
    | 1   | E09 | ARRIVE  | P01    | Asha    | K7M2 | A1    |
    | 2   | E01 | COLLECT | P01    |         | K7M2 |       |
  When I run the handover
  Then E09's outcome is ARRIVED
  And E01's outcome is COLLECTED
  And the summary is 0 / 1 / 0
```

---

## B. Validation — Structural

```gherkin
@V-2
Scenario: TC-010 An empty event ID is rejected
  Given the built-in log with E01's event ID blanked
  Then validation reports INVALID_EVENT on field "Event ID"

@V-3
Scenario: TC-011 An empty parcel ID is rejected
  Given the built-in log with E01's parcel ID blanked
  Then validation reports INVALID_EVENT on field "Parcel ID"

@V-4 @CL-002
Scenario: TC-012 Duplicate detection is case-insensitive
  Given the built-in log with E06's event ID changed to "e05"
  Then validation reports DUPLICATE_EVENT_ID

@V-4 @CL-011
Scenario: TC-013 The later occurrence is flagged, not the first
  Given the built-in log with E06's event ID changed to "E05"
  Then the reported duplicate is the event on row 6
  And row 5 is not reported

@V-4 @CL-011
Scenario: TC-014 A triplicate ID flags both later rows
  Given an event table where rows 1, 2 and 3 all use event ID "E01"
  Then rows 2 and 3 are reported as DUPLICATE_EVENT_ID
  And row 1 is not reported

@V-5
Scenario Outline: TC-015 Only ARRIVE and COLLECT are valid actions
  Given the built-in log with E01's action changed to "<action>"
  Then validation reports INVALID_EVENT on field "Action"
  Examples:
    | action  |
    | DELIVER |
    | RETURN  |
    |         |
    | ARRIVED |

@V-5 @CL-003
Scenario: TC-016 The action is trimmed and uppercased
  Given the built-in log with E01's action changed to "  arrive  "
  When I run the handover
  Then validation reports VALID
  And the summary is 3 / 1 / 1

@V-6
Scenario Outline: TC-017 A pickup code must be exactly four alphanumerics
  Given the built-in log with E01's pickup code changed to "<code>"
  Then validation reports INVALID_PICKUP_CODE on field "Pickup code"
  Examples:
    | code   | note              |
    | K7M    | too short         |
    | K7M2X  | too long          |
    | K7-2   | symbol            |
    | K 7M   | inner whitespace  |
    | K7M@   | symbol            |

@V-6 @CL-001
Scenario: TC-018 A lowercase pickup code is uppercased, not rejected
  Given the built-in log with E01's pickup code changed to "k7m2"
  When I run the handover
  Then validation reports VALID
  And the summary is 3 / 1 / 1

@V-6 @CL-001
Scenario: TC-019 Normalization applies to both sides of a code comparison
  Given the built-in log with E03's pickup code changed to "k7m2"
  When I run the handover
  Then E03's outcome is COLLECTED
  And the summary is 2 / 2 / 0

@V-7 @CL-012
Scenario: TC-020 An empty code on ARRIVE is INVALID_EVENT, not INVALID_PICKUP_CODE
  Given the built-in log with E01's pickup code blanked
  Then validation reports INVALID_EVENT on field "Pickup code"

@V-8 @CL-012
Scenario: TC-021 An empty code on COLLECT is INVALID_EVENT
  Given the built-in log with E03's pickup code blanked
  Then validation reports INVALID_EVENT on field "Pickup code"

@V-7
Scenario Outline: TC-022 ARRIVE requires student and shelf
  Given the built-in log with E01's <field> blanked
  Then validation reports INVALID_EVENT on field "<field>"
  Examples:
    | field   |
    | Student |
    | Shelf   |

@V-8
Scenario: TC-023 COLLECT does not require student or shelf
  Given the built-in log
  Then E03 and E05, whose student and shelf are blank, pass validation

@V-8 @CL-014
Scenario: TC-024 Student and shelf on a COLLECT row are ignored entirely
  Given the built-in log with E05's student set to "!!" and shelf set to "   "
  When I run the handover
  Then validation reports VALID
  And E05's outcome is COLLECTED
  And the summary is 3 / 1 / 1

@V-6 @CL-013
Scenario: TC-025 A well-formed but wrong COLLECT code passes validation
  Given the built-in log
  Then E03's code "ZZZZ" passes validation
  And E03 is rejected later at processing time as PICKUP_CODE_MISMATCH

@V-1 @V-2 @V-3 @V-7 @CL-005
Scenario Outline: TC-026 A whitespace-only required field is empty after trimming
  Given the built-in log with E01's <field> set to "   "
  Then validation reports INVALID_EVENT on field "<field>"
  Examples:
    | field    |
    | Event ID |
    | Parcel ID|
    | Student  |
    | Shelf    |

@V-1
Scenario: TC-027 Surrounding whitespace is trimmed from every field
  Given the built-in log where every field is padded with leading and trailing spaces
  When I run the handover
  Then validation reports VALID
  And the summary is 3 / 1 / 1

@V-2 @CL-010
Scenario: TC-028 Precedence — a blank event ID outranks a malformed code
  Given an event row with a blank event ID and pickup code "XX"
  Then validation reports INVALID_EVENT on field "Event ID"

@V-5 @CL-010
Scenario: TC-029 Precedence — an invalid action outranks a blank parcel ID
  Given an event row with action "SEND" and a blank parcel ID
  Then validation reports INVALID_EVENT on field "Action"

@V-4 @V-6 @CL-010
Scenario: TC-030 Precedence — a malformed code outranks a duplicate event ID
  Given the built-in log with E06's event ID changed to "E05" and its code changed to "XX"
  Then validation reports INVALID_PICKUP_CODE on row 6
  And DUPLICATE_EVENT_ID is not reported for that row

@V-3 @CL-010
Scenario: TC-031 Precedence — a blank parcel ID outranks a missing shelf
  Given an ARRIVE row with a blank parcel ID and a blank shelf
  Then validation reports INVALID_EVENT on field "Parcel ID"

@CL-010
Scenario: TC-040 Precedence — a blank event ID outranks an invalid action
  Given an event row with a blank event ID and action "SEND"
  Then validation reports INVALID_EVENT on field "Event ID"

@CL-010
Scenario: TC-041 Precedence — a missing required field outranks a malformed code
  Given an ARRIVE row with a blank shelf and pickup code "XX"
  Then validation reports INVALID_EVENT on field "Shelf"

@V-9 @CL-009
Scenario: TC-032 Every offending row is reported, one error each
  Given the built-in log with E02's code blanked and E04's parcel ID blanked
  Then validation reports 2 invalid rows
  And row 2 is reported INVALID_EVENT on field "Pickup code"
  And row 4 is reported INVALID_EVENT on field "Parcel ID"

@V-10
Scenario: TC-033 A structural error produces no partial results
  Given the built-in log with E04's action changed to "DELIVER"
  When I run the handover
  Then there are no event outcomes
  And the pending board is empty
  And no summary counts are shown
  And E01 through E03, which are individually valid, produce no outcomes

@V-10
Scenario: TC-034 A failed run clears output from an earlier successful run
  Given I have run the built-in log successfully
  When I introduce a duplicate event ID and run the handover again
  Then the previous outcomes, pending board and counts are gone
  And only the validation error is shown

@V-4 @CL-016
Scenario: TC-035 A repeated parcel ID is legal input
  Given the built-in log
  Then P01 appearing on both E01 and E03 is not a validation error
  And P02 appearing on both E02 and E05 is not a validation error

@V-7 @CL-006
Scenario: TC-036 Student names and shelf labels have no length limit
  Given the built-in log with E01's student set to a 500-character name
  Then validation reports VALID

@V-7 @CL-007
Scenario: TC-037 A shelf label has no required format
  Given the built-in log with E01's shelf set to "Back room, top rack"
  Then validation reports VALID

@V-1 @CL-008
Scenario: TC-038 Non-ASCII names are accepted and preserved
  Given the built-in log with E01's student set to "Zoë Ahmad"
  When I run the handover
  Then validation reports VALID
  And the pending board shows the student as "Zoë Ahmad"

@V-1 @CL-004
Scenario: TC-039 Student and shelf casing is preserved, not uppercased
  Given the built-in log
  When I run the handover
  Then the pending board shows the student as "Asha", not "ASHA"
  And the shelf as "A1"
```

---

## C. Processing — ARRIVE

```gherkin
@P-5
Scenario: TC-050 A clean arrival is accepted
  Given an empty event table with one ARRIVE of P01, code K7M2, shelf A1
  When I run the handover
  Then the outcome is ARRIVED
  And the pending board lists P01 on A1
  And the summary is 1 / 0 / 0

@P-3
Scenario: TC-051 A repeat arrival of a pending parcel is rejected
  Given events: ARRIVE P01 code K7M2, then ARRIVE P01 code H2N6
  When I run the handover
  Then the second outcome is PARCEL_ALREADY_SEEN
  And the pending board lists P01 once, with code K7M2 and its original shelf
  And the summary is 1 / 0 / 1

@P-3 @CL-017
Scenario: TC-052 A parcel that was collected is still "seen"
  Given events: ARRIVE P01 code K7M2, COLLECT P01 code K7M2, ARRIVE P01 code H2N6
  When I run the handover
  Then the third outcome is PARCEL_ALREADY_SEEN
  And the pending board is empty
  And the summary is 0 / 1 / 1

@P-4
Scenario: TC-053 An arrival reusing an active code is rejected
  Given events: ARRIVE P01 code K7M2, then ARRIVE P02 code K7M2
  When I run the handover
  Then the second outcome is ACTIVE_CODE_COLLISION
  And the pending board does not contain P02
  And the summary is 1 / 0 / 1

@P-4 @P-8 @CL-019 @CL-038
Scenario: TC-054 A collected parcel's code is freed for reuse
  Given events: ARRIVE P01 code K7M2, COLLECT P01 code K7M2, ARRIVE P02 code K7M2
  When I run the handover
  Then the third outcome is ARRIVED
  And the pending board lists P02
  And the summary is 1 / 1 / 0

@P-3 @P-4 @CL-018
Scenario: TC-055 A rejected arrival does not mark the parcel as seen
  Given events:
    | 1 | ARRIVE  | P01 | code K7M2 |
    | 2 | ARRIVE  | P02 | code K7M2 |
    | 3 | COLLECT | P01 | code K7M2 |
    | 4 | ARRIVE  | P02 | code K7M2 |
  When I run the handover
  Then event 2's outcome is ACTIVE_CODE_COLLISION
  And event 4's outcome is ARRIVED
  And the pending board lists P02
  And the summary is 1 / 1 / 1

@P-3 @P-4 @CL-024
Scenario: TC-056 Seen is checked before collision
  Given events: ARRIVE P01 code K7M2, ARRIVE P02 code R4Q8, ARRIVE P01 code R4Q8
  When I run the handover
  Then the third outcome is PARCEL_ALREADY_SEEN
  And not ACTIVE_CODE_COLLISION

@P-3 @P-4
Scenario: TC-057 A rejected arrival changes no state at all
  Given events: ARRIVE P01 code K7M2, ARRIVE P01 code H2N6 shelf B9
  When I run the handover
  Then the pending board shows P01 with its original code K7M2 and original shelf
  And the pending board has exactly 1 row
```

@V-1 @CL-002 @P-3
Scenario: TC-058 Parcel ID case-folding mirrors event ID case-folding
  Given events: ARRIVE p01 code K7M2, then ARRIVE P01 code H2N6
  When I run the handover
  Then the second outcome is PARCEL_ALREADY_SEEN
  And the pending board lists P01 once, with code K7M2
  And the summary is 1 / 0 / 1

@V-1 @CL-002 @P-6
Scenario: TC-059 A collect matches a differently-cased parcel ID
  Given events: ARRIVE P01 code K7M2, then COLLECT p01 code K7M2
  When I run the handover
  Then the second outcome is COLLECTED
  And the collected list is P01
  And the summary is 0 / 1 / 0

---

## D. Processing — COLLECT

```gherkin
@P-8
Scenario: TC-060 A correct code collects the parcel
  Given events: ARRIVE P01 code K7M2, COLLECT P01 code K7M2
  When I run the handover
  Then the second outcome is COLLECTED
  And the pending board is empty
  And the collected list is P01
  And the summary is 0 / 1 / 0

@P-7
Scenario: TC-061 A wrong code leaves the parcel pending
  Given events: ARRIVE P01 code K7M2, COLLECT P01 code ZZZZ
  When I run the handover
  Then the second outcome is PICKUP_CODE_MISMATCH
  And the pending board still lists P01
  And the summary is 1 / 0 / 1

@P-6
Scenario: TC-062 Collecting a parcel that never arrived is rejected
  Given events: COLLECT P09 code K7M2
  When I run the handover
  Then the outcome is PARCEL_NOT_PENDING
  And the summary is 0 / 0 / 1

@P-6 @CL-022
Scenario: TC-063 Collecting the same parcel twice is rejected the second time
  Given events: ARRIVE P01 code K7M2, COLLECT P01 code K7M2, COLLECT P01 code K7M2
  When I run the handover
  Then the third outcome is PARCEL_NOT_PENDING
  And the collected list is P01, listed once
  And the summary is 0 / 1 / 1

@P-6 @P-7 @CL-020
Scenario: TC-064 A code belonging to another pending parcel does not collect it
  Given events: ARRIVE P01 code K7M2, ARRIVE P02 code R4Q8, COLLECT P01 code R4Q8
  When I run the handover
  Then the third outcome is PICKUP_CODE_MISMATCH
  And the pending board lists P01, P02 in that order
  And P02 is not collected
  And the summary is 2 / 0 / 1

@P-4 @CL-021
Scenario: TC-065 Two pending parcels can never share a code
  Given any event log
  When I run the handover
  Then no two parcels on the pending board have the same pickup code

@P-9
Scenario: TC-066 A state rejection does not stop later events
  Given the built-in log
  When I run the handover
  Then E03 is rejected
  And E04, E05 and E06 still produce outcomes

@P-10
Scenario: TC-067 The rejected count sums all four rejection outcomes
  Given events producing one each of PARCEL_ALREADY_SEEN, ACTIVE_CODE_COLLISION,
        PARCEL_NOT_PENDING and PICKUP_CODE_MISMATCH
  When I run the handover
  Then the rejected count is 4

@P-10 @U-3 @CL-023
Scenario: TC-068 Each event yields exactly one outcome
  Given any valid event log of N events
  When I run the handover
  Then exactly N outcomes are reported
  And accepted plus rejected outcomes equal N
```

---

## E. Ordering

```gherkin
@O-1
Scenario: TC-070 Outcomes are listed in source order
  Given the built-in log
  When I run the handover
  Then the outcomes appear in the order E01, E02, E03, E04, E05, E06

@O-2
Scenario: TC-071 Pending parcels list by arrival order, not ID or shelf order
  Given events: ARRIVE P03 shelf A2 code T9C4, ARRIVE P01 shelf A1 code K7M2
  When I run the handover
  Then the pending board lists P03, then P01

@O-2
Scenario: TC-072 A collection does not disturb the order of the remaining parcels
  Given events: ARRIVE P01, ARRIVE P02, ARRIVE P03, COLLECT P02 with its correct code
  When I run the handover
  Then the pending board lists P01, then P03

@O-3
Scenario: TC-073 Collected parcels list by collection order
  Given events: ARRIVE P01, ARRIVE P02, COLLECT P02 correctly, COLLECT P01 correctly
  When I run the handover
  Then the collected list is P02, then P01
```

---

## F. Run, Report and CLI

```gherkin
@P-1
Scenario: TC-080 Consecutive runs do not leak state
  Given I have run the built-in log
  When I run the built-in log again without changing it
  Then the outcomes, pending board and summary are identical to the first run

@AC-1 @CL-034 @U-2
Scenario: TC-081 One command performs the whole handover
  When I invoke the program once on the built-in CSV
  Then it loads, validates, processes and writes the report in that single invocation

@U-7 @CL-033
Scenario: TC-082 Reset restores the built-in log and clears results
  Given an event table that has been edited and run
  When I invoke reset
  Then the event table contains exactly the six built-in events
  And no outcomes, handover rows or counts are shown until the next run

@U-6 @CL-027
Scenario: TC-083 A successful run states that it is valid
  Given the built-in log
  When I run the handover
  Then the validation message reads VALID with the number of events accepted
  And it is never blank

@U-4 @CL-026
Scenario: TC-084 The pending board carries everything needed for handover
  Given the built-in log
  When I run the handover
  Then each pending row shows parcel ID, student, shelf and pickup code

@U-5 @CL-028
Scenario: TC-085 Collected parcels are shown in their own section
  Given the built-in log
  When I run the handover
  Then the report contains a collected section listing P02

@CL-035
Scenario: TC-086 The report is written to markdown and summarised on the console
  When I run the handover
  Then a markdown report file is written
  And a short summary is printed to the console

@U-1 @U-8 @CL-025
Scenario: TC-087 The report always matches the current input
  Given I have run the handover
  When I edit the CSV and run again
  Then the new report reflects only the edited input
  And no rows from the previous run remain

@U-1 @CL-031 @CL-032
Scenario: TC-088 Input is read from CSV
  Given a CSV file containing the six built-in events
  When I run the handover
  Then all six events are loaded in file order

@V-11 @CL-029
Scenario: TC-089 An empty log produces a well-formed empty report
  Given an event table with no rows
  When I run the handover
  Then the report shows VALID, no outcome rows, no handover rows and 0 / 0 / 0

@U-9 @CL-036
Scenario: TC-090 The shelf map is grouped from the final pending state
  Given the built-in log
  When I run the handover
  Then the shelf map shows A1 holding P01, A2 holding P03 and B2 holding P04

@U-9 @CL-036
Scenario: TC-091 The shelf map excludes collected parcels
  Given the built-in log
  When I run the handover
  Then shelf B1 is absent or empty, because P02 was collected

@U-9 @CL-036
Scenario: TC-092 The shelf map is empty when nothing is pending
  Given events: ARRIVE P01 code K7M2, COLLECT P01 code K7M2
  When I run the handover
  Then the shelf map is empty

@U-9 @CL-036
Scenario: TC-093 Several parcels on one shelf group together
  Given events: ARRIVE P01 shelf A1 code K7M2, ARRIVE P02 shelf A1 code R4Q8
  When I run the handover
  Then the shelf map shows A1 holding P01 and P02 in arrival order
```

---

## G. Data-Structure Invariants

```gherkin
@CL-038 @CL-018
Scenario: TC-100 A rejected arrival is never appended to the parcel store
  Given events: ARRIVE P01 code K7M2, ARRIVE P02 code K7M2
  When I run the handover
  Then the parcel store holds exactly 1 parcel

@CL-038 @CL-017
Scenario: TC-101 A collected parcel stays in the store
  Given events: ARRIVE P01 code K7M2, COLLECT P01 code K7M2
  When I run the handover
  Then the parcel store still holds P01
  And P01 is marked collected
  And the pending board is empty

@CL-038 @O-2
Scenario: TC-102 Collection preserves a parcel's position in the store
  Given events: ARRIVE P01, ARRIVE P02, ARRIVE P03, COLLECT P01 correctly
  When I run the handover
  Then the store order is still P01, P02, P03
  And the pending board lists P02, then P03

@CL-038 @P-1
Scenario: TC-103 A run clears the store and the collection counter
  Given a completed run with collected parcels
  When a new run begins
  Then the parcel store is empty
  And the next collection sequence number restarts at zero
```

---

## H. CSV Loading, Exit Codes and Output

```gherkin
@CL-039
Scenario: TC-110 The exact header is accepted
  Given a CSV whose first line is "event_id,action,parcel_id,student,pickup_code,shelf"
  And the six built-in events beneath it
  When I run the handover
  Then all six events load and the summary is 3 / 1 / 1

@CL-039 @CL-040
Scenario Outline: TC-111 A bad header is an I/O error, not a validation failure
  Given a CSV whose header is "<header>"
  When I run the handover
  Then the program exits with code 2
  And no validation message or report is produced
  Examples:
    | header                                                        |
    | event_id,action,parcel_id,student,shelf,pickup_code           |
    | eventid,action,parcel_id,student,pickup_code,shelf            |
    | event_id,action,parcel_id,student,pickup_code                 |

@CL-039
Scenario: TC-112 A trailing blank line is skipped
  Given a CSV of the six built-in events followed by an empty line
  When I run the handover
  Then six events are loaded
  And validation reports VALID
  And the summary is 3 / 1 / 1

@CL-039
Scenario: TC-113 A blank interior line is skipped
  Given a CSV with an empty line between events E03 and E04
  When I run the handover
  Then six events are loaded and the summary is 3 / 1 / 1

@CL-039 @V-2
Scenario: TC-114 A row of empty fields is an event, not a blank line
  Given a CSV of the six built-in events followed by the line ",,,,,"
  When I run the handover
  Then row 7 is reported INVALID_EVENT on field "Event ID"
  And no outcomes or counts are produced

@CL-039
Scenario Outline: TC-115 A wrong column count is reported per row
  Given a CSV where row 3 has <n> columns
  When I run the handover
  Then row 3 is reported INVALID_EVENT on field "Row"
  Examples:
    | n |
    | 4 |
    | 7 |

@CL-039
Scenario: TC-116 CRLF line endings are accepted
  Given a CSV of the six built-in events saved with CRLF line endings
  When I run the handover
  Then the summary is 3 / 1 / 1
  And no shelf label retains a trailing carriage return

@CL-039
Scenario: TC-117 A UTF-8 BOM is stripped
  Given a CSV of the six built-in events beginning with a UTF-8 BOM
  When I run the handover
  Then the header is recognised and the summary is 3 / 1 / 1

@CL-039 @CL-007
Scenario: TC-118 A quoted field may contain a comma
  Given the built-in log with E01's shelf set to "Back room, top rack"
  When I run the handover
  Then validation reports VALID
  And the pending board shows P01 on shelf "Back room, top rack"

@CL-039
Scenario: TC-119 A doubled quote inside a quoted field is one literal quote
  Given the built-in log with E01's student set to "Asha \"Ash\" Rao"
  When I run the handover
  Then the pending board shows the student as: Asha "Ash" Rao

@CL-040
Scenario: TC-120 A missing input file exits 2
  Given no CSV file exists at the given path
  When I run the handover
  Then an error naming the path is written to stderr
  And the program exits with code 2
  And no report file is written

@CL-040
Scenario: TC-121 An unreadable input file exits 2
  Given a CSV file that cannot be opened for reading
  When I run the handover
  Then the program exits with code 2

@CL-040 @V-10
Scenario: TC-122 A structural validation failure exits 1
  Given the built-in log with a duplicate event ID
  When I run the handover
  Then the program exits with code 1
  And the validation error is reported

@CL-040
Scenario: TC-123 A run with state rejections still exits 0
  Given the built-in log, which contains one PICKUP_CODE_MISMATCH
  When I run the handover
  Then the program exits with code 0

@CL-041
Scenario: TC-124 The report is written to the default path
  Given the built-in log
  When I run the handover
  Then a report is written to "handover-report.md"

@CL-041
Scenario: TC-125 An existing report is overwritten silently
  Given a previous "handover-report.md" exists
  When I run the handover
  Then it is replaced without prompting
  And it contains no content from the previous run

@CL-041
Scenario: TC-126 The output path can be overridden
  When I run the handover with --out "shift-report.md"
  Then the report is written to "shift-report.md"

@CL-041 @U-7
Scenario: TC-127 Reset creates the CSV if it is absent
  Given no input CSV exists
  When I invoke reset
  Then a CSV containing the six built-in events is created with the exact header

@CL-041 @U-7
Scenario: TC-128 Reset deletes a stale report
  Given a previous run wrote "handover-report.md"
  When I invoke reset
  Then the report file no longer exists
  And no outcomes, handover rows or counts remain anywhere
```

---

## Coverage Matrix

| Spec ID | Scenarios |
|---------|-----------|
| `BL-1` `BL-2` `BL-3` | TC-001 |
| `V-1` | TC-026, TC-027, TC-038, TC-039, TC-058, TC-059 |
| `V-2` | TC-010, TC-026, TC-028, TC-114 |
| `V-3` | TC-011, TC-026, TC-031 |
| `V-4` | TC-005, TC-012, TC-013, TC-014, TC-030, TC-035 |
| `V-5` | TC-015, TC-016, TC-029 |
| `V-6` | TC-017, TC-018, TC-019, TC-025, TC-030 |
| `V-7` | TC-020, TC-022, TC-026, TC-036, TC-037 |
| `V-8` | TC-021, TC-023, TC-024 |
| `V-9` | TC-032 |
| `V-10` | TC-033, TC-034, TC-122 |
| `V-11` | TC-004, TC-089 |
| `P-1` | TC-080, TC-103 |
| `P-2` | TC-006 |
| `P-3` | TC-051, TC-052, TC-055, TC-056, TC-057, TC-058 |
| `P-4` | TC-003, TC-053, TC-054, TC-055, TC-056, TC-057, TC-065 |
| `P-5` | TC-050 |
| `P-6` | TC-059, TC-062, TC-063, TC-064 |
| `P-7` | TC-061, TC-064 |
| `P-8` | TC-054, TC-060 |
| `P-9` | TC-066 |
| `P-10` | TC-067, TC-068 |
| `O-1` | TC-006, TC-070 |
| `O-2` | TC-071, TC-072, TC-102 |
| `O-3` | TC-073 |
| `U-1` | TC-002, TC-003, TC-087, TC-088 |
| `U-2` | TC-081 |
| `U-4` | TC-084 |
| `U-6` | TC-083 |
| `U-3` | TC-068 |
| `U-5` | TC-085 |
| `U-7` | TC-082, TC-127, TC-128 |
| `U-8` | TC-087 |
| `U-9` | TC-090, TC-091, TC-092, TC-093 |
| `AC-1` | TC-001, TC-081 |
| `AC-2` | TC-002 |
| `AC-3` | TC-003 |
| `AC-4` | TC-004 |
| `AC-5` | TC-005 |
| `AC-6` | TC-006 |

| Decision | Scenarios |
|----------|-----------|
| CL-001 | TC-018, TC-019 |
| CL-002 | TC-012, TC-058, TC-059 |
| CL-003 | TC-016 |
| CL-004 | TC-039 |
| CL-005 | TC-026 |
| CL-006 | TC-036 |
| CL-007 | TC-037, TC-118 |
| CL-008 | TC-038 |
| CL-009 | TC-032 |
| CL-010 | TC-028, TC-029, TC-030, TC-031, TC-040, TC-041 |
| CL-011 | TC-013, TC-014 |
| CL-012 | TC-020, TC-021 |
| CL-013 | TC-025 |
| CL-014 | TC-024 |
| CL-015 | TC-004 |
| CL-016 | TC-035 |
| CL-017 | TC-052, TC-101 |
| CL-018 | TC-055, TC-100 |
| CL-019 | TC-054 |
| CL-020 | TC-064 |
| CL-021 | TC-065 |
| CL-022 | TC-063 |
| CL-023 | TC-068 |
| CL-024 | TC-056 |
| CL-025 | TC-087 |
| CL-026 | TC-084 |
| CL-027 | TC-083 |
| CL-028 | TC-085 |
| CL-029 | TC-004, TC-089 |
| CL-031 | TC-088 |
| CL-032 | TC-088 |
| CL-033 | TC-082 |
| CL-034 | TC-081 |
| CL-035 | TC-086 |
| CL-036 | TC-090 – TC-093 |
| CL-038 | TC-054, TC-100 – TC-103 |
| CL-039 | TC-110 – TC-119 |
| CL-040 | TC-111, TC-120 – TC-123 |
| CL-041 | TC-124 – TC-128 |

### Not directly testable

These carry no scenario by design, and their absence above is deliberate rather than an omission.

| ID | Why |
|----|-----|
| `SC-1` – `SC-4` | Scope and medium statements, not runtime behaviour |
| `PR-1` – `PR-3` | Process requirements on how the work is carried out |
| CL-030 | A language choice. No scenario can prove "this is C++" |
| CL-033 – CL-035 | Delivery-shape mappings; their observable effects are covered by TC-082, TC-086, TC-124 – TC-128 |
| CL-037 | Describes the suite itself |
