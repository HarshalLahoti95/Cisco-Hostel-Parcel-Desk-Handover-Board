#include "csv.h"

#include <fstream>
#include <sstream>

// The only translation unit permitted to include <fstream>. Keeping file access
// alone in here is what lets the rules engine be tested without a filesystem.

namespace hd {
namespace {

// Minimal RFC 4180 (CL-039): a field may be wrapped in quotes, inside which a
// comma is literal and "" is one literal quote. Required, not optional —
// CL-007 permits shelf labels like "Back room, top rack".
std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size();) {
        char c = line[i];

        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur += '"';   // an escaped quote
                    i += 2;
                    continue;
                }
                inQuotes = false;
                ++i;
                continue;
            }
            cur += c;
            ++i;
            continue;
        }

        if (c == '"' && cur.empty()) {   // a quote only opens at a field's start
            inQuotes = true;
            ++i;
            continue;
        }
        if (c == ',') {
            fields.push_back(cur);
            cur.clear();
            ++i;
            continue;
        }
        cur += c;
        ++i;
    }

    fields.push_back(cur);
    return fields;
}

// CRLF and LF both accepted; a trailing carriage return is stripped so no
// shelf label ever keeps one (CL-039).
std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

// A UTF-8 BOM is stripped if present, or the header would never match.
void stripBom(std::string& s) {
    if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
}

}  // namespace

const char* const kCsvHeader = "event_id,action,parcel_id,student,pickup_code,shelf";

LoadResult parseCsv(const std::string& text) {
    LoadResult r;

    std::string body = text;
    stripBom(body);

    std::vector<std::string> lines = splitLines(body);
    if (lines.empty()) {
        r.error = "empty file: expected header \"" + std::string(kCsvHeader) + "\"";
        return r;
    }

    // The header is required and exact. Missing, misspelled or reordered is an
    // I/O error (exit 2), never a validation failure — a typo'd file layout
    // means nothing ran, not that the event log is bad (CL-040).
    if (lines.front() != kCsvHeader) {
        r.error = "bad header: expected \"" + std::string(kCsvHeader) +
                  "\", found \"" + lines.front() + "\"";
        return r;
    }

    int rowNo = 0;
    for (size_t i = 1; i < lines.size(); ++i) {
        const std::string& line = lines[i];

        // A blank line carries no fields and is skipped, wherever it appears —
        // Excel appends one. A row of empty fields (",,,,,") is NOT blank: it
        // is an event, and fails validation as one.
        if (line.empty()) continue;

        std::vector<std::string> f = splitCsvLine(line);

        Event e;
        e.row = ++rowNo;

        if (f.size() != 6) {
            // Wrong column count: no field can be trusted, so the row is
            // flagged whole and validation reports it on field "Row" (CL-009).
            e.malformedRow = true;
        } else {
            e.id       = f[0];
            e.action   = f[1];
            e.parcelId = f[2];
            e.student  = f[3];
            e.code     = f[4];
            e.shelf    = f[5];
        }

        r.events.push_back(e);
    }

    r.ok = true;
    return r;
}

LoadResult loadCsv(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        LoadResult r;
        r.error = "cannot read input file: " + path;
        return r;
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    return parseCsv(ss.str());
}

// BL-1, for --reset (CL-033, CL-041).
std::string builtInCsv() {
    return std::string(kCsvHeader) + "\n" +
           "E01,ARRIVE,P01,Asha,K7M2,A1\n"
           "E02,ARRIVE,P02,Bilal,R4Q8,B1\n"
           "E03,COLLECT,P01,,ZZZZ,\n"
           "E04,ARRIVE,P03,Chen,T9C4,A2\n"
           "E05,COLLECT,P02,,R4Q8,\n"
           "E06,ARRIVE,P04,Divya,H2N6,B2\n";
}

}  // namespace hd
