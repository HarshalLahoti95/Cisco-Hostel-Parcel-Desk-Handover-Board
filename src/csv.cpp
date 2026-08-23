#include "csv.h"

#include <fstream>

// STEP 4 — not implemented yet. The only translation unit permitted to include
// <fstream>; keeping it alone here is what makes the rules engine testable.

namespace hd {

const char* const kCsvHeader = "event_id,action,parcel_id,student,pickup_code,shelf";

LoadResult parseCsv(const std::string& text) {
    (void)text;
    LoadResult r;
    r.ok = false;
    r.error = "csv parser not implemented (step 4)";
    return r;
}

LoadResult loadCsv(const std::string& path) {
    (void)path;
    LoadResult r;
    r.ok = false;
    r.error = "csv loader not implemented (step 4)";
    return r;
}

std::string builtInCsv() { return ""; }

}  // namespace hd
