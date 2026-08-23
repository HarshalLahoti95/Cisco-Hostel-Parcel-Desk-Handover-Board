// engine.h — P-1 – P-10, O-1 – O-3. Pure: no file I/O, no console.
#pragma once

#include <vector>

#include "model.h"

namespace hd {

// Replays a normalized, already-validated event log in source order (P-2).
// Every call starts from empty state (P-1).
RunResult run(const std::vector<Event>& events);

}  // namespace hd
