// csv.h — CL-031, CL-039. The only place the filesystem is touched.
#pragma once

#include <string>
#include <vector>

#include "model.h"

namespace hd {

extern const char* const kCsvHeader;   // CL-039, exact and required

struct LoadResult {
    std::vector<Event> events;
    bool               ok = false;
    std::string        error;   // populated when !ok -> exit 2 (CL-040)
};

// Parses CSV text. Header must match kCsvHeader exactly, else ok == false.
// Blank lines skipped; wrong column counts flagged via Event::malformedRow.
LoadResult parseCsv(const std::string& text);

LoadResult loadCsv(const std::string& path);

// The six BL-1 events as a CSV document, used by --reset (CL-033, CL-041).
std::string builtInCsv();

}  // namespace hd
