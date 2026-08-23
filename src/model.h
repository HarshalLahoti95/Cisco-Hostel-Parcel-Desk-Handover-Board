// model.h — core data structures, fixed by CL-038.
// Deliberately dependency-free: no <fstream>, no <iostream>.
#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace hd {

// One row of the event table (U-1). `row` is the 1-based data row number,
// excluding the header, and is what V-9 / CL-009 report against.
struct Event {
    std::string id;
    std::string action;
    std::string parcelId;
    std::string student;
    std::string code;
    std::string shelf;
    int         row = 0;

    // Set by the CSV loader when the row had the wrong column count (CL-039).
    // Validation turns this into INVALID_EVENT on field "Row".
    bool malformedRow = false;
};

// CL-038: a single parcel store. `collectedSeq < 0` means pending — one field,
// one fact. Membership in the store *is* the seen-set (CL-017).
struct Parcel {
    std::string id;
    std::string student;
    std::string code;
    std::string shelf;
    int         collectedSeq = -1;

    bool isPending() const { return collectedSeq < 0; }
};

// P-3 – P-8. Exactly one per event (CL-023).
enum class Outcome {
    ARRIVED,
    COLLECTED,
    PARCEL_ALREADY_SEEN,
    ACTIVE_CODE_COLLISION,
    PARCEL_NOT_PENDING,
    PICKUP_CODE_MISMATCH,
};

// P-10: the rejected count is these four, and only these four.
bool        isRejection(Outcome o);
const char* toString(Outcome o);
std::ostream& operator<<(std::ostream& os, Outcome o);

struct EventOutcome {
    std::string id;       // the event's ID, as normalized
    Outcome     outcome;
};

// The result of one replay. O-1 order in `outcomes`, O-2 order in `parcels`.
struct RunResult {
    std::vector<EventOutcome> outcomes;
    std::vector<Parcel>       parcels;   // the store; never erased from

    std::vector<Parcel> pending() const;    // O-2: arrival order
    std::vector<Parcel> collected() const;  // O-3: collection order

    int pendingCount() const;
    int collectedCount() const;
    int rejectedCount() const;   // P-10
};

// V-9 / CL-009: one error per offending row, for every offending row.
struct RowError {
    int         row = 0;
    std::string eventId;   // as given; may be empty when that is the fault
    std::string field;     // "Event ID" | "Action" | "Parcel ID" | "Student" | "Shelf" | "Pickup code" | "Row"
    std::string code;      // "INVALID_EVENT" | "INVALID_PICKUP_CODE" | "DUPLICATE_EVENT_ID"
};

}  // namespace hd
