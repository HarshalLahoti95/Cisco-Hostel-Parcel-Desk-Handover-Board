// validate.h — V-2 – V-11 over the complete table, before any processing.
#pragma once

#include <vector>

#include "model.h"

namespace hd {

// Expects already-normalized events (CL-038).
// Returns one error per offending row (CL-009); empty means valid (V-11 included).
// Precedence within a row, then cross-row uniqueness last: CL-010.
std::vector<RowError> validate(const std::vector<Event>& events);

}  // namespace hd
