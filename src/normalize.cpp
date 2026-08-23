#include "normalize.h"

#include <cctype>

namespace hd {

// V-1. Whitespace-only therefore trims to empty, which is what CL-005 relies on.
std::string trim(const std::string& s) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };

    size_t b = 0;
    while (b < s.size() && isSpace(static_cast<unsigned char>(s[b]))) ++b;

    size_t e = s.size();
    while (e > b && isSpace(static_cast<unsigned char>(s[e - 1]))) --e;

    return s.substr(b, e - b);
}

// CL-008: only ASCII letters are folded, so multi-byte UTF-8 passes through
// untouched. Codes are A-Z0-9 by V-6 anyway.
std::string upper(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x80) c = static_cast<char>(std::toupper(u));
    }
    return out;
}

Event normalizeEvent(const Event& e) {
    Event n = e;
    n.id       = upper(trim(e.id));        // CL-002
    n.action   = upper(trim(e.action));    // CL-003
    n.parcelId = upper(trim(e.parcelId));  // CL-002
    n.code     = upper(trim(e.code));      // CL-001
    n.student  = trim(e.student);          // CL-004: case preserved
    n.shelf    = trim(e.shelf);            // CL-004: case preserved
    return n;
}

std::vector<Event> normalizeAll(const std::vector<Event>& events) {
    std::vector<Event> out;
    out.reserve(events.size());
    for (const auto& e : events) out.push_back(normalizeEvent(e));
    return out;
}

}  // namespace hd
