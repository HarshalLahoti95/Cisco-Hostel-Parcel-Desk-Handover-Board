#include "validate.h"

#include <set>
#include <string>

namespace hd {
namespace {

const char* const kInvalidEvent  = "INVALID_EVENT";
const char* const kInvalidCode   = "INVALID_PICKUP_CODE";
const char* const kDuplicateId   = "DUPLICATE_EVENT_ID";

// V-6: exactly four characters, each an uppercase letter or a digit.
// Normalization has already folded case (CL-001), so this is a pure shape test.
bool isWellFormedCode(const std::string& code) {
    if (code.size() != 4) return false;
    for (char c : code) {
        bool alnum = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (!alnum) return false;
    }
    return true;
}

// Steps 0-5 of CL-010: everything decidable from the row alone.
// Returns false when the row is clean so far.
bool rowLocalError(const Event& e, RowError& out) {
    auto report = [&](const char* code, const char* field) {
        out = RowError{e.row, e.id, field, code};
        return true;
    };

    // Step 0 (CL-039): the column count was wrong, so no field can be trusted.
    if (e.malformedRow) return report(kInvalidEvent, "Row");

    // Step 1 — V-2
    if (e.id.empty()) return report(kInvalidEvent, "Event ID");

    // Step 2 — V-5. Resolved early because steps 3-4 depend on the action.
    const bool isArrive  = e.action == "ARRIVE";
    const bool isCollect = e.action == "COLLECT";
    if (!isArrive && !isCollect) return report(kInvalidEvent, "Action");

    // Step 3 — V-3
    if (e.parcelId.empty()) return report(kInvalidEvent, "Parcel ID");

    // Step 4 — V-7 / V-8, required fields for that action.
    // An empty code is a missing field, not a malformed one (CL-012).
    if (isArrive) {
        if (e.student.empty()) return report(kInvalidEvent, "Student");
        if (e.code.empty())    return report(kInvalidEvent, "Pickup code");
        if (e.shelf.empty())   return report(kInvalidEvent, "Shelf");
    } else {
        // V-8: student and shelf are unused on COLLECT and may be blank,
        // valid or not (CL-014). Only the code is required.
        if (e.code.empty()) return report(kInvalidEvent, "Pickup code");
    }

    // Step 5 — V-6. The code is present, so now its shape is judged.
    if (!isWellFormedCode(e.code)) return report(kInvalidCode, "Pickup code");

    return false;
}

}  // namespace

std::vector<RowError> validate(const std::vector<Event>& events) {
    std::vector<RowError> errors;
    std::set<std::string> seenIds;

    // CL-009: one error per offending row, and every offending row is reported,
    // so the volunteer fixes the whole table in a single pass.
    for (const auto& e : events) {
        RowError err;
        if (rowLocalError(e, err)) {
            errors.push_back(err);
            continue;   // CL-023-style short-circuit: one error per row
        }

        // Step 6 — V-4, last because it is the only check needing whole-table
        // context. CL-011: the first row keeps the ID, each later repeat is
        // reported. Note V-4 governs event IDs only — a repeated *parcel* ID is
        // the normal lifecycle, not an error (CL-016).
        if (!seenIds.insert(e.id).second)
            errors.push_back(RowError{e.row, e.id, "Event ID", kDuplicateId});
    }

    return errors;
}

}  // namespace hd
