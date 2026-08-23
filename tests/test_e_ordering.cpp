// Section E — Ordering. docs/test-cases.md TC-070 – TC-073.
#include "fixtures.h"

using namespace fx;
static const char* kSec = "E. Ordering";

// @O-1
SCENARIO(TC_070, kSec, "Outcomes are listed in source order") {
    auto h = run(builtInLog());
    CHECK_IDS(outcomeOrder(h.result), {"E01", "E02", "E03", "E04", "E05", "E06"});
}

// @O-2
SCENARIO(TC_071, kSec, "Pending parcels list by arrival order, not ID or shelf order") {
    auto h = run(logOf({
        arrive("E01", "P03", "T9C4", "A2", "Chen"),
        arrive("E02", "P01", "K7M2", "A1", "Asha"),
    }));

    CHECK_IDS(pendingIds(h.result), {"P03", "P01"});
}

// @O-2
SCENARIO(TC_072, kSec, "A collection does not disturb the order of the remaining parcels") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "R4Q8", "B1", "Bilal"),
        arrive("E03", "P03", "T9C4", "A2", "Chen"),
        collect("E04", "P02", "R4Q8"),
    }));

    CHECK_IDS(pendingIds(h.result), {"P01", "P03"});
}

// @O-3
SCENARIO(TC_073, kSec, "Collected parcels list by collection order") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "R4Q8", "B1", "Bilal"),
        collect("E03", "P02", "R4Q8"),
        collect("E04", "P01", "K7M2"),
    }));

    CHECK_IDS(collectedIds(h.result), {"P02", "P01"});
}
