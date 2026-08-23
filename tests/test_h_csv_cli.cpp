// Section H — CSV Loading, Exit Codes and Output. docs/test-cases.md TC-110 – TC-128.
#include <sys/stat.h>

#include <string>

#include "../src/csv.h"
#include "fixtures.h"

using namespace fx;
static const char* kSec = "H. CSV, Exit Codes and Output";

namespace {
bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}
// The six BL-1 data rows, without a header, so a scenario can supply its own.
const char* kBody =
    "E01,ARRIVE,P01,Asha,K7M2,A1\n"
    "E02,ARRIVE,P02,Bilal,R4Q8,B1\n"
    "E03,COLLECT,P01,,ZZZZ,\n"
    "E04,ARRIVE,P03,Chen,T9C4,A2\n"
    "E05,COLLECT,P02,,R4Q8,\n"
    "E06,ARRIVE,P04,Divya,H2N6,B2\n";
}  // namespace

// @CL-039
SCENARIO(TC_110, kSec, "The exact header is accepted") {
    auto r = runCsvText(
        std::string("event_id,action,parcel_id,student,pickup_code,shelf\n") + kBody);

    CHECK_MSG(r.loaded, "the CSV loads");
    CHECK_EQ(r.events.size(), size_t(6));
    CHECK_SUMMARY(r.handover.result, 3, 1, 1);
}

// @CL-039 @CL-040  (Scenario Outline: three bad headers)
SCENARIO(TC_111, kSec, "A bad header is an I/O error, not a validation failure") {
    const char* headers[] = {
        "event_id,action,parcel_id,student,shelf,pickup_code",
        "eventid,action,parcel_id,student,pickup_code,shelf",
        "event_id,action,parcel_id,student,pickup_code",
    };

    for (const char* header : headers) {
        auto r = runCsvText(std::string(header) + "\n" + kBody);
        CHECK_MSG(!r.loaded, std::string("header rejected: ") + header);

        Workspace ws;
        ws.write("events.csv", std::string(header) + "\n" + kBody);
        auto cli = ws.cli({"events.csv"});
        CHECK_EQ(cli.exitCode, 2);
        CHECK_MSG(!ws.exists("handover-report.md"), "no report is produced");
    }
}

// @CL-039
SCENARIO(TC_112, kSec, "A trailing blank line is skipped") {
    auto r = runCsvText(builtInCsvText() + "\n");

    CHECK_MSG(r.loaded, "the CSV loads");
    CHECK_EQ(r.events.size(), size_t(6));
    CHECK_MSG(r.handover.valid(), "validation reports VALID");
    CHECK_SUMMARY(r.handover.result, 3, 1, 1);
}

// @CL-039
SCENARIO(TC_113, kSec, "A blank interior line is skipped") {
    auto r = runCsvText(
        "event_id,action,parcel_id,student,pickup_code,shelf\n"
        "E01,ARRIVE,P01,Asha,K7M2,A1\n"
        "E02,ARRIVE,P02,Bilal,R4Q8,B1\n"
        "E03,COLLECT,P01,,ZZZZ,\n"
        "\n"
        "E04,ARRIVE,P03,Chen,T9C4,A2\n"
        "E05,COLLECT,P02,,R4Q8,\n"
        "E06,ARRIVE,P04,Divya,H2N6,B2\n");

    CHECK_MSG(r.loaded, "the CSV loads");
    CHECK_EQ(r.events.size(), size_t(6));
    CHECK_SUMMARY(r.handover.result, 3, 1, 1);
}

// @CL-039 @V-2
SCENARIO(TC_114, kSec, "A row of empty fields is an event, not a blank line") {
    auto r = runCsvText(builtInCsvText() + ",,,,,\n");

    CHECK_MSG(r.loaded, "the CSV loads");
    CHECK_EQ(r.events.size(), size_t(7));

    const RowError* e = errorOnRow(r.handover.errors, 7);
    CHECK_MSG(e != nullptr, "row 7 is reported");
    CHECK_EQ(e->code, std::string("INVALID_EVENT"));
    CHECK_EQ(e->field, std::string("Event ID"));

    CHECK_EQ(r.handover.result.outcomes.size(), size_t(0));
    CHECK_EQ(r.handover.result.parcels.size(), size_t(0));
}

// @CL-039  (Scenario Outline: 4 columns, 7 columns)
SCENARIO(TC_115, kSec, "A wrong column count is reported per row") {
    const char* rows[] = {
        "E03,COLLECT,P01,\n",                 // 4 columns
        "E03,COLLECT,P01,,ZZZZ,,extra\n",     // 7 columns
    };

    for (const char* bad : rows) {
        std::string text =
            "event_id,action,parcel_id,student,pickup_code,shelf\n"
            "E01,ARRIVE,P01,Asha,K7M2,A1\n"
            "E02,ARRIVE,P02,Bilal,R4Q8,B1\n";
        text += bad;
        text +=
            "E04,ARRIVE,P03,Chen,T9C4,A2\n"
            "E05,COLLECT,P02,,R4Q8,\n"
            "E06,ARRIVE,P04,Divya,H2N6,B2\n";

        auto r = runCsvText(text);
        CHECK_MSG(r.loaded, "a bad column count is a row error, not an I/O error");

        const RowError* e = errorOnRow(r.handover.errors, 3);
        CHECK_MSG(e != nullptr, "row 3 is reported");
        CHECK_EQ(e->code, std::string("INVALID_EVENT"));
        CHECK_EQ(e->field, std::string("Row"));
    }
}

// @CL-039
SCENARIO(TC_116, kSec, "CRLF line endings are accepted") {
    std::string crlf;
    for (char c : builtInCsvText()) {
        if (c == '\n') crlf += '\r';
        crlf += c;
    }

    auto r = runCsvText(crlf);
    CHECK_MSG(r.loaded, "the CSV loads");
    CHECK_SUMMARY(r.handover.result, 3, 1, 1);

    for (const auto& p : r.handover.result.parcels)
        CHECK_MSG(p.shelf.find('\r') == std::string::npos,
                  "no shelf label retains a trailing carriage return");
}

// @CL-039
SCENARIO(TC_117, kSec, "A UTF-8 BOM is stripped") {
    auto r = runCsvText(std::string("\xEF\xBB\xBF") + builtInCsvText());

    CHECK_MSG(r.loaded, "the header is recognised");
    CHECK_SUMMARY(r.handover.result, 3, 1, 1);
}

// @CL-039 @CL-007
SCENARIO(TC_118, kSec, "A quoted field may contain a comma") {
    auto r = runCsvText(
        "event_id,action,parcel_id,student,pickup_code,shelf\n"
        "E01,ARRIVE,P01,Asha,K7M2,\"Back room, top rack\"\n");

    CHECK_MSG(r.loaded, "the CSV loads");
    CHECK_MSG(r.handover.valid(), "validation reports VALID");
    const Parcel* p = pendingParcel(r.handover.result, "P01");
    CHECK_MSG(p != nullptr, "P01 is pending");
    CHECK_EQ(p->shelf, std::string("Back room, top rack"));
}

// @CL-039
SCENARIO(TC_119, kSec, "A doubled quote inside a quoted field is one literal quote") {
    auto r = runCsvText(
        "event_id,action,parcel_id,student,pickup_code,shelf\n"
        "E01,ARRIVE,P01,\"Asha \"\"Ash\"\" Rao\",K7M2,A1\n");

    CHECK_MSG(r.loaded, "the CSV loads");
    const Parcel* p = pendingParcel(r.handover.result, "P01");
    CHECK_MSG(p != nullptr, "P01 is pending");
    CHECK_EQ(p->student, std::string("Asha \"Ash\" Rao"));
}

// @CL-040
SCENARIO(TC_120, kSec, "A missing input file exits 2") {
    Workspace ws;
    auto r = ws.cli({"nowhere.csv"});

    CHECK_EQ(r.exitCode, 2);
    CHECK_MSG(contains(r.err, "nowhere.csv"),
              "an error naming the path is written to stderr");
    CHECK_MSG(!ws.exists("handover-report.md"), "no report file is written");
}

// @CL-040
SCENARIO(TC_121, kSec, "An unreadable input file exits 2") {
    Workspace ws;
    ws.write("events.csv", builtInCsvText());
    ::chmod(ws.path("events.csv").c_str(), 0000);

    auto r = ws.cli({"events.csv"});
    ::chmod(ws.path("events.csv").c_str(), 0644);   // so the workspace can clean up

    CHECK_EQ(r.exitCode, 2);
}

// @CL-040 @V-10
SCENARIO(TC_122, kSec, "A structural validation failure exits 1") {
    Workspace ws;
    std::string text = builtInCsvText();
    // E06 renamed to E05 -> DUPLICATE_EVENT_ID
    text.replace(text.rfind("E06"), 3, "E05");
    ws.write("events.csv", text);

    auto r = ws.cli({"events.csv"});
    CHECK_EQ(r.exitCode, 1);
    CHECK_MSG(contains(r.out + r.err, "DUPLICATE_EVENT_ID"),
              "the validation error is reported");
}

// @CL-040
SCENARIO(TC_123, kSec, "A run with state rejections still exits 0") {
    Workspace ws;
    ws.write("events.csv", builtInCsvText());

    auto r = ws.cli({"events.csv"});
    CHECK_EQ(r.exitCode, 0);
}

// @CL-041
SCENARIO(TC_124, kSec, "The report is written to the default path") {
    Workspace ws;
    ws.write("events.csv", builtInCsvText());
    ws.cli({"events.csv"});

    CHECK_MSG(ws.exists("handover-report.md"),
              "a report is written to handover-report.md");
}

// @CL-041
SCENARIO(TC_125, kSec, "An existing report is overwritten silently") {
    Workspace ws;
    ws.write("events.csv", builtInCsvText());
    ws.write("handover-report.md", "# STALE\nP99 from a previous run\n");

    auto r = ws.cli({"events.csv"});
    CHECK_EQ(r.exitCode, 0);

    std::string report = ws.read("handover-report.md");
    CHECK_MSG(!contains(report, "STALE"), "it contains no content from the previous run");
    CHECK_MSG(!contains(report, "P99"), "it contains no content from the previous run");
}

// @CL-041
SCENARIO(TC_126, kSec, "The output path can be overridden") {
    Workspace ws;
    ws.write("events.csv", builtInCsvText());

    auto r = ws.cli({"events.csv", "--out", "shift-report.md"});
    CHECK_EQ(r.exitCode, 0);
    CHECK_MSG(ws.exists("shift-report.md"), "the report is written to shift-report.md");
}

// @CL-041 @U-7
SCENARIO(TC_127, kSec, "Reset creates the CSV if it is absent") {
    Workspace ws;
    CHECK_MSG(!ws.exists("events.csv"), "no input CSV exists");

    auto r = ws.cli({"events.csv", "--reset"});
    CHECK_EQ(r.exitCode, 0);
    CHECK_MSG(ws.exists("events.csv"), "a CSV is created");
    CHECK_EQ(ws.read("events.csv"), builtInCsvText());
}

// @CL-041 @U-7
SCENARIO(TC_128, kSec, "Reset deletes a stale report") {
    Workspace ws;
    ws.write("events.csv", builtInCsvText());
    ws.cli({"events.csv"});
    CHECK_MSG(ws.exists("handover-report.md"), "a previous run wrote the report");

    auto r = ws.cli({"events.csv", "--reset"});
    CHECK_EQ(r.exitCode, 0);
    CHECK_MSG(!ws.exists("handover-report.md"), "the report file no longer exists");
}
