// Section F — Run, Report and CLI. docs/test-cases.md TC-080 – TC-093.
#include "../src/report.h"

#include <string>

#include "fixtures.h"

using namespace fx;
using hd::ShelfGroup;
static const char* kSec = "F. Run, Report and CLI";

namespace {
bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}
const ShelfGroup* shelf(const std::vector<ShelfGroup>& m, const std::string& label) {
    for (const auto& g : m)
        if (g.shelf == label) return &g;
    return nullptr;
}
}  // namespace

// @P-1
SCENARIO(TC_080, kSec, "Consecutive runs do not leak state") {
    auto a = run(builtInLog());
    auto b = run(builtInLog());

    CHECK_IDS(outcomeOrder(b.result), outcomeOrder(a.result));
    CHECK_IDS(pendingIds(b.result), pendingIds(a.result));
    CHECK_IDS(collectedIds(b.result), collectedIds(a.result));
    CHECK_EQ(b.result.pendingCount(), a.result.pendingCount());
    CHECK_EQ(b.result.collectedCount(), a.result.collectedCount());
    CHECK_EQ(b.result.rejectedCount(), a.result.rejectedCount());
}

// @AC-1 @CL-034 @U-2
SCENARIO(TC_081, kSec, "One command performs the whole handover") {
    Workspace ws;
    ws.write("events.csv", builtInCsvText());

    auto r = ws.cli({"events.csv"});
    CHECK_EQ(r.exitCode, 0);
    CHECK_MSG(ws.exists("handover-report.md"),
              "the single invocation wrote the report");

    std::string report = ws.read("handover-report.md");
    CHECK_MSG(contains(report, "VALID"), "it validated");
    CHECK_MSG(contains(report, "P01"), "it processed");
}

// @U-7 @CL-033
SCENARIO(TC_082, kSec, "Reset restores the built-in log and clears results") {
    Workspace ws;
    ws.write("events.csv",
             "event_id,action,parcel_id,student,pickup_code,shelf\n"
             "E99,ARRIVE,P99,Edited,QQQQ,Z9\n");
    ws.cli({"events.csv"});

    auto r = ws.cli({"events.csv", "--reset"});
    CHECK_EQ(r.exitCode, 0);
    CHECK_EQ(ws.read("events.csv"), builtInCsvText());
    CHECK_MSG(!ws.exists("handover-report.md"),
              "no outcomes, handover rows or counts are shown until the next run");
}

// @U-6 @CL-027
SCENARIO(TC_083, kSec, "A successful run states that it is valid") {
    auto h = run(builtInLog());
    std::string report = hd::renderReport(h);

    CHECK_MSG(contains(report, "VALID"), "the validation message reads VALID");
    CHECK_MSG(contains(report, "6"), "with the number of events accepted");
    CHECK_MSG(!hd::renderConsoleSummary(h).empty(), "it is never blank");
}

// @U-4 @CL-026
SCENARIO(TC_084, kSec, "The pending board carries everything needed for handover") {
    auto h = run(builtInLog());
    std::string report = hd::renderReport(h);

    for (const std::string& cell : {"P01", "Asha", "A1", "K7M2"})
        CHECK_MSG(contains(report, cell), "pending row shows " + cell);
}

// @U-5 @CL-028
SCENARIO(TC_085, kSec, "Collected parcels are shown in their own section") {
    auto h = run(builtInLog());
    std::string report = hd::renderReport(h);

    CHECK_MSG(contains(report, "Collected"), "the report has a collected section");
    CHECK_MSG(contains(report, "P02"), "listing P02");
}

// @CL-035
SCENARIO(TC_086, kSec, "The report is written to markdown and summarised on the console") {
    Workspace ws;
    ws.write("events.csv", builtInCsvText());

    auto r = ws.cli({"events.csv"});
    CHECK_MSG(ws.exists("handover-report.md"), "a markdown report file is written");
    CHECK_MSG(!r.out.empty(), "a short summary is printed to the console");
}

// @U-1 @U-8 @CL-025
SCENARIO(TC_087, kSec, "The report always matches the current input") {
    Workspace ws;
    ws.write("events.csv", builtInCsvText());
    ws.cli({"events.csv"});

    ws.write("events.csv",
             "event_id,action,parcel_id,student,pickup_code,shelf\n"
             "E01,ARRIVE,P77,Nadia,W3X8,C3\n");
    ws.cli({"events.csv"});

    std::string report = ws.read("handover-report.md");
    CHECK_MSG(contains(report, "P77"), "the new report reflects the edited input");
    for (const std::string& gone : {"P01", "P02", "P03", "P04"})
        CHECK_MSG(!contains(report, gone), "no rows from the previous run remain: " + gone);
}

// @U-1 @CL-031 @CL-032
SCENARIO(TC_088, kSec, "Input is read from CSV") {
    auto r = runCsvText(builtInCsvText());
    CHECK_MSG(r.loaded, "the CSV loads");
    CHECK_EQ(r.events.size(), size_t(6));

    std::vector<std::string> ids;
    for (const auto& e : r.events) ids.push_back(e.id);
    CHECK_IDS(ids, {"E01", "E02", "E03", "E04", "E05", "E06"});
}

// @V-11 @CL-029
SCENARIO(TC_089, kSec, "An empty log produces a well-formed empty report") {
    auto h = run({});
    std::string report = hd::renderReport(h);

    CHECK_MSG(contains(report, "VALID"), "the report shows VALID");
    CHECK_EQ(h.result.outcomes.size(), size_t(0));
    CHECK_EQ(pendingIds(h.result).size(), size_t(0));
    CHECK_SUMMARY(h.result, 0, 0, 0);
}

// @U-9 @CL-036
SCENARIO(TC_090, kSec, "The shelf map is grouped from the final pending state") {
    auto h = run(builtInLog());
    auto map = hd::shelfMap(h.result);

    const ShelfGroup* a1 = shelf(map, "A1");
    const ShelfGroup* a2 = shelf(map, "A2");
    const ShelfGroup* b2 = shelf(map, "B2");
    CHECK_MSG(a1 && a2 && b2, "A1, A2 and B2 all appear");
    CHECK_IDS(a1->parcelIds, {"P01"});
    CHECK_IDS(a2->parcelIds, {"P03"});
    CHECK_IDS(b2->parcelIds, {"P04"});
}

// @U-9 @CL-036
SCENARIO(TC_091, kSec, "The shelf map excludes collected parcels") {
    auto h = run(builtInLog());
    auto map = hd::shelfMap(h.result);

    const ShelfGroup* b1 = shelf(map, "B1");
    CHECK_MSG(b1 == nullptr || b1->parcelIds.empty(),
              "shelf B1 is absent or empty, because P02 was collected");
}

// @U-9 @CL-036
SCENARIO(TC_092, kSec, "The shelf map is empty when nothing is pending") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        collect("E02", "P01", "K7M2"),
    }));

    CHECK_EQ(hd::shelfMap(h.result).size(), size_t(0));
}

// @U-9 @CL-036
SCENARIO(TC_093, kSec, "Several parcels on one shelf group together") {
    auto h = run(logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "R4Q8", "A1", "Bilal"),
    }));

    auto map = hd::shelfMap(h.result);
    const ShelfGroup* a1 = shelf(map, "A1");
    CHECK_MSG(a1 != nullptr, "A1 appears");
    CHECK_IDS(a1->parcelIds, {"P01", "P02"});
}
