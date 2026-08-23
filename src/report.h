// report.h — U-3 – U-6 and the U-9 shelf map. Renders, stores nothing.
#pragma once

#include <string>
#include <vector>

#include "handover.h"
#include "model.h"

namespace hd {

// U-9 / CL-036: derived at render time over the pending subset of the one
// parcel store. Never a second store.
struct ShelfGroup {
    std::string              shelf;
    std::vector<std::string> parcelIds;   // arrival order
};

std::vector<ShelfGroup> shelfMap(const RunResult& r);

// The full markdown report: validation message, outcomes, pending board,
// collected section, counts, shelf map.
std::string renderReport(const Handover& h);

// The short console line (CL-035).
std::string renderConsoleSummary(const Handover& h);

}  // namespace hd
