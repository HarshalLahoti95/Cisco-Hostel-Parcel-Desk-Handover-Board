// Section G — Data-Structure Invariants. docs/test-cases.md TC-100 – TC-103.
#include "fixtures.h"

using namespace fx;
static const char* kSec = "G. Data-Structure Invariants";

// @CL-038 @CL-018
SCENARIO(TC_100, kSec, "A rejected arrival is never appended to the parcel store") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "K7M2", "B1", "Bilal"),
    }));

    CHECK_EQ(h.result.parcels.size(), size_t(1));
}

// @CL-038 @CL-017
SCENARIO(TC_101, kSec, "A collected parcel stays in the store") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        collect("E02", "P01", "K7M2"),
    }));

    const Parcel* p = storedParcel(h.result, "P01");
    CHECK_MSG(p != nullptr, "the store still holds P01");
    CHECK_MSG(!p->isPending(), "P01 is marked collected");
    CHECK_EQ(pendingIds(h.result).size(), size_t(0));
}

// @CL-038 @O-2
SCENARIO(TC_102, kSec, "Collection preserves a parcel's position in the store") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "R4Q8", "B1", "Bilal"),
        arrive("E03", "P03", "T9C4", "A2", "Chen"),
        collect("E04", "P01", "K7M2"),
    }));

    std::vector<std::string> storeOrder;
    for (const auto& p : h.result.parcels) storeOrder.push_back(p.id);
    CHECK_IDS(storeOrder, {"P01", "P02", "P03"});
    CHECK_IDS(pendingIds(h.result), {"P02", "P03"});
}

// @CL-038 @P-1
SCENARIO(TC_103, kSec, "A run clears the store and the collection counter") {
    // A completed run with collected parcels.
    auto first = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "R4Q8", "B1", "Bilal"),
        collect("E03", "P01", "K7M2"),
        collect("E04", "P02", "R4Q8"),
    }));
    CHECK_EQ(first.result.collectedCount(), 2);

    // A new run begins.
    auto second = run(logOf({
        arrive("E01", "P09", "T9C4", "C1", "Divya"),
        collect("E02", "P09", "T9C4"),
    }));

    CHECK_EQ(second.result.parcels.size(), size_t(1));
    const Parcel* p = storedParcel(second.result, "P09");
    CHECK_MSG(p != nullptr, "the new run's store holds only its own parcel");
    CHECK_MSG(p->collectedSeq == 0,
              "the collection sequence number restarts at zero");
}
