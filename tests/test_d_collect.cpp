// Section D — Processing, COLLECT. docs/test-cases.md TC-060 – TC-068.
#include <set>

#include "fixtures.h"

using namespace fx;
static const char* kSec = "D. Processing - COLLECT";

// @P-8
SCENARIO(TC_060, kSec, "A correct code collects the parcel") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        collect("E02", "P01", "K7M2"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E02"), Outcome::COLLECTED);
    CHECK_EQ(pendingIds(h.result).size(), size_t(0));
    CHECK_IDS(collectedIds(h.result), {"P01"});
    CHECK_SUMMARY(h.result, 0, 1, 0);
}

// @P-7
SCENARIO(TC_061, kSec, "A wrong code leaves the parcel pending") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        collect("E02", "P01", "ZZZZ"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E02"), Outcome::PICKUP_CODE_MISMATCH);
    CHECK_IDS(pendingIds(h.result), {"P01"});
    CHECK_SUMMARY(h.result, 1, 0, 1);
}

// @P-6
SCENARIO(TC_062, kSec, "Collecting a parcel that never arrived is rejected") {
    auto h = run(logOf({collect("E01", "P09", "K7M2")}));

    CHECK_EQ(outcomeOf(h.result, "E01"), Outcome::PARCEL_NOT_PENDING);
    CHECK_SUMMARY(h.result, 0, 0, 1);
}

// @P-6 @CL-022
SCENARIO(TC_063, kSec, "Collecting the same parcel twice is rejected the second time") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        collect("E02", "P01", "K7M2"),
        collect("E03", "P01", "K7M2"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::PARCEL_NOT_PENDING);
    CHECK_IDS(collectedIds(h.result), {"P01"});
    CHECK_SUMMARY(h.result, 0, 1, 1);
}

// @P-6 @P-7 @CL-020
SCENARIO(TC_064, kSec, "A code belonging to another pending parcel does not collect it") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "R4Q8", "B1", "Bilal"),
        collect("E03", "P01", "R4Q8"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::PICKUP_CODE_MISMATCH);
    CHECK_IDS(pendingIds(h.result), {"P01", "P02"});
    CHECK_MSG(pendingParcel(h.result, "P02") != nullptr, "P02 is not collected");
    CHECK_SUMMARY(h.result, 2, 0, 1);
}

// @P-4 @CL-021
SCENARIO(TC_065, kSec, "Two pending parcels can never share a code") {
    std::vector<std::vector<Event>> logs = {
        builtInLog(),
        logOf({
            arrive("E01", "P01", "K7M2", "A1", "Asha"),
            arrive("E02", "P02", "K7M2", "B1", "Bilal"),
            arrive("E03", "P03", "K7M2", "A2", "Chen"),
        }),
        logOf({
            arrive("E01", "P01", "K7M2", "A1", "Asha"),
            collect("E02", "P01", "K7M2"),
            arrive("E03", "P02", "K7M2", "B1", "Bilal"),
            arrive("E04", "P03", "K7M2", "A2", "Chen"),
        }),
    };

    for (const auto& log : logs) {
        auto h = run(log);
        std::set<std::string> codes;
        for (const auto& p : h.result.pending()) {
            CHECK_MSG(codes.insert(p.code).second,
                      "two pending parcels share code " + p.code);
        }
    }
}

// @P-9
SCENARIO(TC_066, kSec, "A state rejection does not stop later events") {
    auto h = run(builtInLog());

    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::PICKUP_CODE_MISMATCH);
    CHECK_MSG(hasOutcome(h.result, "E04"), "E04 still produces an outcome");
    CHECK_MSG(hasOutcome(h.result, "E05"), "E05 still produces an outcome");
    CHECK_MSG(hasOutcome(h.result, "E06"), "E06 still produces an outcome");
}

// @P-10
SCENARIO(TC_067, kSec, "The rejected count sums all four rejection outcomes") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),   // ARRIVED
        arrive("E02", "P02", "K7M2", "B1", "Bilal"),  // ACTIVE_CODE_COLLISION
        collect("E03", "P09", "R4Q8"),                // PARCEL_NOT_PENDING
        collect("E04", "P01", "ZZZZ"),                // PICKUP_CODE_MISMATCH
        arrive("E05", "P01", "H2N6", "B9", "Chen"),   // PARCEL_ALREADY_SEEN
    }));

    CHECK_EQ(outcomeOf(h.result, "E02"), Outcome::ACTIVE_CODE_COLLISION);
    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::PARCEL_NOT_PENDING);
    CHECK_EQ(outcomeOf(h.result, "E04"), Outcome::PICKUP_CODE_MISMATCH);
    CHECK_EQ(outcomeOf(h.result, "E05"), Outcome::PARCEL_ALREADY_SEEN);
    CHECK_EQ(h.result.rejectedCount(), 4);
}

// @P-10 @U-3 @CL-023
SCENARIO(TC_068, kSec, "Each event yields exactly one outcome") {
    auto log = builtInLog();
    auto h = run(log);

    CHECK_EQ(h.result.outcomes.size(), log.size());

    int accepted = 0, rejected = 0;
    for (const auto& o : h.result.outcomes) {
        if (hd::isRejection(o.outcome)) ++rejected;
        else ++accepted;
    }
    CHECK_EQ(accepted + rejected, int(log.size()));
    CHECK_EQ(rejected, h.result.rejectedCount());
}
