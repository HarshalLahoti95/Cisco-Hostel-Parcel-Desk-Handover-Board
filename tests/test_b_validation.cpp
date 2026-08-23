// Section B — Validation, structural. docs/test-cases.md TC-010 – TC-041.
#include <string>

#include "fixtures.h"

using namespace fx;
static const char* kSec = "B. Validation - Structural";

namespace {
// Asserts the single error reported for `r` carries `code` on `field`.
void expectRowError(const std::vector<RowError>& errs, int r,
                    const std::string& code, const std::string& field,
                    const char* file, int line) {
    const RowError* e = errorOnRow(errs, r);
    if (!e) th::fail(file, line, "no validation error reported on row " + std::to_string(r));
    ::th::checkEq(file, line, "error code", "expected", e->code, code);
    ::th::checkEq(file, line, "error field", "expected", e->field, field);
}
}  // namespace

#define EXPECT_ROW_ERROR(errs, r, code, field) \
    expectRowError((errs), (r), (code), (field), __FILE__, __LINE__)

// @V-2
SCENARIO(TC_010, kSec, "An empty event ID is rejected") {
    auto log = builtInLog();
    row(log, 1).id = "";
    EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Event ID");
}

// @V-3
SCENARIO(TC_011, kSec, "An empty parcel ID is rejected") {
    auto log = builtInLog();
    row(log, 1).parcelId = "";
    EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Parcel ID");
}

// @V-4 @CL-002
SCENARIO(TC_012, kSec, "Duplicate detection is case-insensitive") {
    auto log = builtInLog();
    row(log, 6).id = "e05";
    CHECK_MSG(anyError(run(log).errors, "DUPLICATE_EVENT_ID"),
              "e05 collides with E05");
}

// @V-4 @CL-011
SCENARIO(TC_013, kSec, "The later occurrence is flagged, not the first") {
    auto log = builtInLog();
    row(log, 6).id = "E05";
    auto errs = run(log).errors;

    EXPECT_ROW_ERROR(errs, 6, "DUPLICATE_EVENT_ID", "Event ID");
    CHECK_MSG(errorOnRow(errs, 5) == nullptr, "row 5 is not reported");
}

// @V-4 @CL-011
SCENARIO(TC_014, kSec, "A triplicate ID flags both later rows") {
    auto log = logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E01", "P02", "R4Q8", "B1", "Bilal"),
        arrive("E01", "P03", "T9C4", "A2", "Chen"),
    });
    auto errs = run(log).errors;

    EXPECT_ROW_ERROR(errs, 2, "DUPLICATE_EVENT_ID", "Event ID");
    EXPECT_ROW_ERROR(errs, 3, "DUPLICATE_EVENT_ID", "Event ID");
    CHECK_MSG(errorOnRow(errs, 1) == nullptr, "row 1 is not reported");
}

// @V-5  (Scenario Outline: DELIVER, RETURN, blank, ARRIVED)
SCENARIO(TC_015, kSec, "Only ARRIVE and COLLECT are valid actions") {
    for (const std::string& action : {"DELIVER", "RETURN", "", "ARRIVED"}) {
        auto log = builtInLog();
        row(log, 1).action = action;
        EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Action");
    }
}

// @V-5 @CL-003
SCENARIO(TC_016, kSec, "The action is trimmed and uppercased") {
    auto log = builtInLog();
    row(log, 1).action = "  arrive  ";

    auto h = run(log);
    CHECK_MSG(h.valid(), "validation reports VALID");
    CHECK_SUMMARY(h.result, 3, 1, 1);
}

// @V-6  (Scenario Outline: too short, too long, symbol, inner whitespace, symbol)
SCENARIO(TC_017, kSec, "A pickup code must be exactly four alphanumerics") {
    for (const std::string& code : {"K7M", "K7M2X", "K7-2", "K 7M", "K7M@"}) {
        auto log = builtInLog();
        row(log, 1).code = code;
        EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_PICKUP_CODE", "Pickup code");
    }
}

// @V-6 @CL-001
SCENARIO(TC_018, kSec, "A lowercase pickup code is uppercased, not rejected") {
    auto log = builtInLog();
    row(log, 1).code = "k7m2";

    auto h = run(log);
    CHECK_MSG(h.valid(), "validation reports VALID");
    CHECK_SUMMARY(h.result, 3, 1, 1);
}

// @V-6 @CL-001
SCENARIO(TC_019, kSec, "Normalization applies to both sides of a code comparison") {
    auto log = builtInLog();
    row(log, 3).code = "k7m2";

    auto h = run(log);
    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::COLLECTED);
    CHECK_SUMMARY(h.result, 2, 2, 0);
}

// @V-7 @CL-012
SCENARIO(TC_020, kSec, "An empty code on ARRIVE is INVALID_EVENT") {
    auto log = builtInLog();
    row(log, 1).code = "";
    EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Pickup code");
}

// @V-8 @CL-012
SCENARIO(TC_021, kSec, "An empty code on COLLECT is INVALID_EVENT") {
    auto log = builtInLog();
    row(log, 3).code = "";
    EXPECT_ROW_ERROR(run(log).errors, 3, "INVALID_EVENT", "Pickup code");
}

// @V-7  (Scenario Outline: Student, Shelf)
SCENARIO(TC_022, kSec, "ARRIVE requires student and shelf") {
    {
        auto log = builtInLog();
        row(log, 1).student = "";
        EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Student");
    }
    {
        auto log = builtInLog();
        row(log, 1).shelf = "";
        EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Shelf");
    }
}

// @V-8
SCENARIO(TC_023, kSec, "COLLECT does not require student or shelf") {
    auto h = run(builtInLog());
    CHECK_MSG(h.valid(), "the built-in log validates");
    CHECK_MSG(errorOnRow(h.errors, 3) == nullptr, "E03 passes validation");
    CHECK_MSG(errorOnRow(h.errors, 5) == nullptr, "E05 passes validation");
}

// @V-8 @CL-014
SCENARIO(TC_024, kSec, "Student and shelf on a COLLECT row are ignored entirely") {
    auto log = builtInLog();
    row(log, 5).student = "!!";
    row(log, 5).shelf = "   ";

    auto h = run(log);
    CHECK_MSG(h.valid(), "validation reports VALID");
    CHECK_EQ(outcomeOf(h.result, "E05"), Outcome::COLLECTED);
    CHECK_SUMMARY(h.result, 3, 1, 1);
}

// @V-6 @CL-013
SCENARIO(TC_025, kSec, "A well-formed but wrong COLLECT code passes validation") {
    auto h = run(builtInLog());
    CHECK_MSG(errorOnRow(h.errors, 3) == nullptr, "ZZZZ passes validation");
    CHECK_EQ(outcomeOf(h.result, "E03"), Outcome::PICKUP_CODE_MISMATCH);
}

// @V-1 @V-2 @V-3 @V-7 @CL-005  (Outline: Event ID, Parcel ID, Student, Shelf)
SCENARIO(TC_026, kSec, "A whitespace-only required field is empty after trimming") {
    {
        auto log = builtInLog(); row(log, 1).id = "   ";
        EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Event ID");
    }
    {
        auto log = builtInLog(); row(log, 1).parcelId = "   ";
        EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Parcel ID");
    }
    {
        auto log = builtInLog(); row(log, 1).student = "   ";
        EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Student");
    }
    {
        auto log = builtInLog(); row(log, 1).shelf = "   ";
        EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Shelf");
    }
}

// @V-1
SCENARIO(TC_027, kSec, "Surrounding whitespace is trimmed from every field") {
    auto log = builtInLog();
    for (auto& e : log) {
        e.id = "  " + e.id + "  ";
        e.action = "  " + e.action + "  ";
        e.parcelId = "  " + e.parcelId + "  ";
        e.code = "  " + e.code + "  ";
        if (!e.student.empty()) e.student = "  " + e.student + "  ";
        if (!e.shelf.empty()) e.shelf = "  " + e.shelf + "  ";
    }

    auto h = run(log);
    CHECK_MSG(h.valid(), "validation reports VALID");
    CHECK_SUMMARY(h.result, 3, 1, 1);
}

// @V-2 @CL-010
SCENARIO(TC_028, kSec, "Precedence - blank event ID outranks a malformed code") {
    auto log = logOf({arrive("", "P01", "XX", "A1", "Asha")});
    EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Event ID");
}

// @V-5 @CL-010
SCENARIO(TC_029, kSec, "Precedence - an invalid action outranks a blank parcel ID") {
    auto log = logOf({arrive("E01", "", "K7M2", "A1", "Asha")});
    row(log, 1).action = "SEND";
    EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Action");
}

// @V-4 @V-6 @CL-010
SCENARIO(TC_030, kSec, "Precedence - a malformed code outranks a duplicate event ID") {
    auto log = builtInLog();
    row(log, 6).id = "E05";
    row(log, 6).code = "XX";

    auto errs = run(log).errors;
    EXPECT_ROW_ERROR(errs, 6, "INVALID_PICKUP_CODE", "Pickup code");

    const RowError* e = errorOnRow(errs, 6);
    CHECK_MSG(e && e->code != "DUPLICATE_EVENT_ID",
              "DUPLICATE_EVENT_ID is not reported for that row");
}

// @V-3 @CL-010
SCENARIO(TC_031, kSec, "Precedence - a blank parcel ID outranks a missing shelf") {
    auto log = logOf({arrive("E01", "", "K7M2", "", "Asha")});
    EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Parcel ID");
}

// @V-9 @CL-009
SCENARIO(TC_032, kSec, "Every offending row is reported, one error each") {
    auto log = builtInLog();
    row(log, 2).code = "";
    row(log, 4).parcelId = "";

    auto errs = run(log).errors;
    CHECK_EQ(errs.size(), size_t(2));
    EXPECT_ROW_ERROR(errs, 2, "INVALID_EVENT", "Pickup code");
    EXPECT_ROW_ERROR(errs, 4, "INVALID_EVENT", "Parcel ID");
}

// @V-10
SCENARIO(TC_033, kSec, "A structural error produces no partial results") {
    auto log = builtInLog();
    row(log, 4).action = "DELIVER";

    auto h = run(log);
    CHECK_EQ(h.result.outcomes.size(), size_t(0));
    CHECK_EQ(pendingIds(h.result).size(), size_t(0));
    CHECK_EQ(h.result.parcels.size(), size_t(0));

    // E01 - E03 are individually valid and still produce no outcomes
    CHECK_MSG(!hasOutcome(h.result, "E01"), "E01 produces no outcome");
    CHECK_MSG(!hasOutcome(h.result, "E02"), "E02 produces no outcome");
    CHECK_MSG(!hasOutcome(h.result, "E03"), "E03 produces no outcome");
}

// @V-10
SCENARIO(TC_034, kSec, "A failed run clears output from an earlier successful run") {
    auto first = run(builtInLog());
    CHECK_MSG(first.valid(), "the first run succeeds");
    CHECK_EQ(first.result.outcomes.size(), size_t(6));

    auto log = builtInLog();
    row(log, 6).id = "E05";
    auto second = run(log);

    CHECK_EQ(second.result.outcomes.size(), size_t(0));
    CHECK_EQ(pendingIds(second.result).size(), size_t(0));
    CHECK_EQ(second.result.parcels.size(), size_t(0));
    CHECK_MSG(!second.errors.empty(), "only the validation error is shown");
}

// @V-4 @CL-016
SCENARIO(TC_035, kSec, "A repeated parcel ID is legal input") {
    auto h = run(builtInLog());
    CHECK_MSG(h.valid(), "P01 on E01+E03 and P02 on E02+E05 are not errors");
    CHECK_EQ(h.errors.size(), size_t(0));
}

// @V-7 @CL-006
SCENARIO(TC_036, kSec, "Student names and shelf labels have no length limit") {
    auto log = builtInLog();
    row(log, 1).student = std::string(500, 'a');
    CHECK_MSG(run(log).valid(), "validation reports VALID");
}

// @V-7 @CL-007
SCENARIO(TC_037, kSec, "A shelf label has no required format") {
    auto log = builtInLog();
    row(log, 1).shelf = "Back room, top rack";
    CHECK_MSG(run(log).valid(), "validation reports VALID");
}

// @V-1 @CL-008
SCENARIO(TC_038, kSec, "Non-ASCII names are accepted and preserved") {
    auto log = builtInLog();
    row(log, 1).student = "Zoë Ahmad";

    auto h = run(log);
    CHECK_MSG(h.valid(), "validation reports VALID");
    const Parcel* p = pendingParcel(h.result, "P01");
    CHECK_MSG(p != nullptr, "P01 is pending");
    CHECK_EQ(p->student, std::string("Zoë Ahmad"));
}

// @V-1 @CL-004
SCENARIO(TC_039, kSec, "Student and shelf casing is preserved, not uppercased") {
    auto h = run(builtInLog());
    const Parcel* p = pendingParcel(h.result, "P01");
    CHECK_MSG(p != nullptr, "P01 is pending");
    CHECK_EQ(p->student, std::string("Asha"));
    CHECK_EQ(p->shelf, std::string("A1"));
}

// @CL-010
SCENARIO(TC_040, kSec, "Precedence - a blank event ID outranks an invalid action") {
    auto log = logOf({arrive("", "P01", "K7M2", "A1", "Asha")});
    row(log, 1).action = "SEND";
    EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Event ID");
}

// @CL-010
SCENARIO(TC_041, kSec, "Precedence - a missing required field outranks a malformed code") {
    auto log = logOf({arrive("E01", "P01", "XX", "", "Asha")});
    EXPECT_ROW_ERROR(run(log).errors, 1, "INVALID_EVENT", "Shelf");
}
