// Section A — Acceptance Criteria. docs/test-cases.md TC-001 – TC-006.
#include "fixtures.h"

using namespace fx;
static const char* kSec = "A. Acceptance Criteria";

// @AC-1 @BL-1 @BL-2 @BL-3
SCENARIO(TC_001, kSec, "The built-in log runs in one action") {
    auto h = run(builtInLog());

    CHECK_MSG(h.valid(), "validation reports VALID");

    CHECK_IDS(outcomeOrder(h.result), {"E01", "E02", "E03", "E04", "E05", "E06"});
    CHECK_EQ(outcomeOf(h.result, "E01"), Outcome::ARRIVED);
    CHECK_EQ(outcomeOf(h.result, "E02"), Outcome::ARRIVED);
    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::PICKUP_CODE_MISMATCH);
    CHECK_EQ(outcomeOf(h.result, "E04"), Outcome::ARRIVED);
    CHECK_EQ(outcomeOf(h.result, "E05"), Outcome::COLLECTED);
    CHECK_EQ(outcomeOf(h.result, "E06"), Outcome::ARRIVED);

    // pending board lists P01 on A1, P03 on A2, P04 on B2 in that order
    CHECK_IDS(pendingIds(h.result), {"P01", "P03", "P04"});
    auto pend = h.result.pending();
    CHECK_EQ(pend.at(0).shelf, std::string("A1"));
    CHECK_EQ(pend.at(1).shelf, std::string("A2"));
    CHECK_EQ(pend.at(2).shelf, std::string("B2"));

    CHECK_IDS(collectedIds(h.result), {"P02"});
    CHECK_SUMMARY(h.result, 3, 1, 1);
}

// @AC-2 @U-1
SCENARIO(TC_002, kSec, "Correcting E03's pickup code collects P01") {
    auto log = builtInLog();
    row(log, 3).code = "K7M2";

    auto h = run(log);
    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::COLLECTED);
    CHECK_IDS(pendingIds(h.result), {"P03", "P04"});
    CHECK_SUMMARY(h.result, 2, 2, 0);
}

// @AC-3 @P-4 @U-1
SCENARIO(TC_003, kSec, "E06 reusing an active code collides") {
    auto log = builtInLog();
    row(log, 6).code = "T9C4";

    auto h = run(log);
    CHECK_EQ(outcomeOf(h.result, "E06"), Outcome::ACTIVE_CODE_COLLISION);
    CHECK_MSG(pendingParcel(h.result, "P04") == nullptr,
              "the pending board does not contain P04");
    CHECK_IDS(pendingIds(h.result), {"P01", "P03"});
    CHECK_SUMMARY(h.result, 2, 1, 2);
}

// @AC-4 @V-11 @CL-015 @CL-029
SCENARIO(TC_004, kSec, "An empty event table is valid") {
    auto h = run({});

    CHECK_MSG(h.valid(), "validation reports VALID");
    CHECK_EQ(h.result.outcomes.size(), size_t(0));
    CHECK_EQ(pendingIds(h.result).size(), size_t(0));
    CHECK_EQ(collectedIds(h.result).size(), size_t(0));
    CHECK_SUMMARY(h.result, 0, 0, 0);
}

// @AC-5 @V-4
SCENARIO(TC_005, kSec, "A duplicate event ID suppresses the whole run") {
    auto log = builtInLog();
    row(log, 6).id = "E05";

    auto h = run(log);
    CHECK_MSG(!h.valid(), "validation fails");

    const RowError* e = errorOnRow(h.errors, 6);
    CHECK_MSG(e != nullptr, "row 6 is reported");
    CHECK_EQ(e->code, std::string("DUPLICATE_EVENT_ID"));
    CHECK_EQ(e->eventId, std::string("E05"));
    CHECK_EQ(e->field, std::string("Event ID"));

    // V-10: no outcomes, no handover rows, no counts
    CHECK_EQ(h.result.outcomes.size(), size_t(0));
    CHECK_EQ(pendingIds(h.result).size(), size_t(0));
    CHECK_EQ(h.result.parcels.size(), size_t(0));
}

// @AC-6 @P-2 @O-1
SCENARIO(TC_006, kSec, "Processing follows row order, not event-ID order") {
    auto log = logOf({
        arrive("E09", "P01", "K7M2", "A1", "Asha"),
        collect("E01", "P01", "K7M2"),
    });

    auto h = run(log);
    CHECK_EQ(outcomeOf(h.result, "E09"), Outcome::ARRIVED);
    CHECK_EQ(outcomeOf(h.result, "E01"), Outcome::COLLECTED);
    CHECK_SUMMARY(h.result, 0, 1, 0);
}
