#include "handover.h"

#include "engine.h"
#include "normalize.h"
#include "validate.h"

namespace hd {

Handover runHandover(const std::vector<Event>& rawEvents) {
    Handover h;
    h.events = normalizeAll(rawEvents);   // V-1, CL-001 - CL-004: once, up front
    h.errors = validate(h.events);        // V-2 - V-11 over the complete table

    // V-10: a structural error yields no outcomes, no handover rows, no counts.
    if (h.errors.empty()) h.result = run(h.events);

    return h;
}

}  // namespace hd
