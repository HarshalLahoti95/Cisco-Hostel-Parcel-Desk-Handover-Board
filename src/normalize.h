// normalize.h — V-1 plus CL-001 – CL-004. Runs once, at load.
#pragma once

#include <string>
#include <vector>

#include "model.h"

namespace hd {

std::string trim(const std::string& s);
std::string upper(const std::string& s);

// Trim + uppercase: event ID, action, parcel ID, pickup code (CL-001/002/003).
// Trim only, case preserved: student, shelf (CL-004).
Event normalizeEvent(const Event& e);

std::vector<Event> normalizeAll(const std::vector<Event>& events);

}  // namespace hd
