// handover.h — the run pipeline: normalize -> validate -> gate -> replay.
#pragma once

#include <vector>

#include "model.h"

namespace hd {

struct Handover {
    std::vector<Event>    events;   // normalized
    std::vector<RowError> errors;   // empty == valid
    RunResult             result;   // V-10: stays empty when `errors` is non-empty

    bool valid() const { return errors.empty(); }
};

// Takes raw, un-normalized events straight from the table.
Handover runHandover(const std::vector<Event>& rawEvents);

}  // namespace hd
