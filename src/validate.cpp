#include "validate.h"

// STEP 3 — not implemented yet.
// Returning "no errors" is the neutral stub: it lets the pipeline compile and
// lets sections C-E exercise the engine, while every scenario in section B
// fails until the real V-2 - V-11 checks land.

namespace hd {

std::vector<RowError> validate(const std::vector<Event>& events) {
    (void)events;
    return {};
}

}  // namespace hd
