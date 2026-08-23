// Section C — Processing, ARRIVE. docs/test-cases.md TC-050 – TC-059.
#include "fixtures.h"

using namespace fx;
static const char* kSec = "C. Processing - ARRIVE";

// @P-5
SCENARIO(TC_050, kSec, "A clean arrival is accepted") {
    auto h = run(logOf({arrive("E01", "P01", "K7M2", "A1", "Asha")}));

    CHECK_EQ(outcomeOf(h.result, "E01"), Outcome::ARRIVED);
    CHECK_IDS(pendingIds(h.result), {"P01"});
    CHECK_EQ(h.result.pending().at(0).shelf, std::string("A1"));
    CHECK_SUMMARY(h.result, 1, 0, 0);
}

// @P-3
SCENARIO(TC_051, kSec, "A repeat arrival of a pending parcel is rejected") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P01", "H2N6", "B9", "Bilal"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E02"), Outcome::PARCEL_ALREADY_SEEN);
    CHECK_IDS(pendingIds(h.result), {"P01"});
    const Parcel* p = pendingParcel(h.result, "P01");
    CHECK_MSG(p != nullptr, "P01 is pending");
    CHECK_EQ(p->code, std::string("K7M2"));
    CHECK_EQ(p->shelf, std::string("A1"));
    CHECK_SUMMARY(h.result, 1, 0, 1);
}

// @P-3 @CL-017
SCENARIO(TC_052, kSec, "A parcel that was collected is still seen") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        collect("E02", "P01", "K7M2"),
        arrive("E03", "P01", "H2N6", "B9", "Bilal"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::PARCEL_ALREADY_SEEN);
    CHECK_EQ(pendingIds(h.result).size(), size_t(0));
    CHECK_SUMMARY(h.result, 0, 1, 1);
}

// @P-4
SCENARIO(TC_053, kSec, "An arrival reusing an active code is rejected") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "K7M2", "B1", "Bilal"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E02"), Outcome::ACTIVE_CODE_COLLISION);
    CHECK_MSG(pendingParcel(h.result, "P02") == nullptr,
              "the pending board does not contain P02");
    CHECK_SUMMARY(h.result, 1, 0, 1);
}

// @P-4 @P-8 @CL-019 @CL-038
SCENARIO(TC_054, kSec, "A collected parcel's code is freed for reuse") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        collect("E02", "P01", "K7M2"),
        arrive("E03", "P02", "K7M2", "B1", "Bilal"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::ARRIVED);
    CHECK_IDS(pendingIds(h.result), {"P02"});
    CHECK_SUMMARY(h.result, 1, 1, 0);
}

// @P-3 @P-4 @CL-018
SCENARIO(TC_055, kSec, "A rejected arrival does not mark the parcel as seen") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "K7M2", "B1", "Bilal"),
        collect("E03", "P01", "K7M2"),
        arrive("E04", "P02", "K7M2", "B1", "Bilal"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E02"), Outcome::ACTIVE_CODE_COLLISION);
    CHECK_EQ(outcomeOf(h.result, "E04"), Outcome::ARRIVED);
    CHECK_IDS(pendingIds(h.result), {"P02"});
    CHECK_SUMMARY(h.result, 1, 1, 1);
}

// @P-3 @P-4 @CL-024
SCENARIO(TC_056, kSec, "Seen is checked before collision") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "R4Q8", "B1", "Bilal"),
        arrive("E03", "P01", "R4Q8", "A2", "Chen"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::PARCEL_ALREADY_SEEN);
    CHECK_MSG(outcomeOf(h.result, "E03") != Outcome::ACTIVE_CODE_COLLISION,
              "and not ACTIVE_CODE_COLLISION");
}

// @P-3 @P-4
SCENARIO(TC_057, kSec, "A rejected arrival changes no state at all") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P01", "H2N6", "B9", "Bilal"),
    }));

    const Parcel* p = pendingParcel(h.result, "P01");
    CHECK_MSG(p != nullptr, "P01 is pending");
    CHECK_EQ(p->code, std::string("K7M2"));
    CHECK_EQ(p->shelf, std::string("A1"));
    CHECK_EQ(pendingIds(h.result).size(), size_t(1));
}

// @V-1 @CL-002 @P-3
SCENARIO(TC_058, kSec, "Parcel ID case-folding mirrors event ID case-folding") {
    auto h = run(logOf({
        arrive("E01", "p01", "K7M2", "A1", "Asha"),
        arrive("E02", "P01", "H2N6", "B9", "Bilal"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E02"), Outcome::PARCEL_ALREADY_SEEN);
    CHECK_IDS(pendingIds(h.result), {"P01"});
    const Parcel* p = pendingParcel(h.result, "P01");
    CHECK_MSG(p != nullptr, "P01 is pending");
    CHECK_EQ(p->code, std::string("K7M2"));
    CHECK_SUMMARY(h.result, 1, 0, 1);
}

// @V-1 @CL-002 @P-6
SCENARIO(TC_059, kSec, "A collect matches a differently-cased parcel ID") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        collect("E02", "p01", "K7M2"),
    }));

    CHECK_EQ(outcomeOf(h.result, "E02"), Outcome::COLLECTED);
    CHECK_IDS(collectedIds(h.result), {"P01"});
    CHECK_SUMMARY(h.result, 0, 1, 0);
}
