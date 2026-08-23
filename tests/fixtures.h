// fixtures.h — shared scaffolding for the scenarios in docs/test-cases.md.
// Contains no expectations of its own; every assertion lives in a scenario.
#pragma once

#include <initializer_list>
#include <string>
#include <vector>

#include "../src/handover.h"
#include "../src/model.h"
#include "harness.h"

namespace fx {

using hd::Event;
using hd::Handover;
using hd::Outcome;
using hd::Parcel;
using hd::RowError;
using hd::RunResult;

// ---- Building event tables -------------------------------------------------

Event arrive(const std::string& id, const std::string& parcel,
             const std::string& code, const std::string& shelf = "A1",
             const std::string& student = "Asha");

Event collect(const std::string& id, const std::string& parcel,
              const std::string& code);

// Numbers the rows 1..n, as the CSV loader would.
std::vector<Event> logOf(std::initializer_list<Event> events);

// BL-1, exactly as printed in problem-and-spec.md section 2.
std::vector<Event> builtInLog();

// 1-based row access, so a scenario can blank the field an event is keyed by.
Event& row(std::vector<Event>& log, int n);

// ---- Running ---------------------------------------------------------------

Handover run(const std::vector<Event>& raw);

// ---- Querying a result -----------------------------------------------------

bool        hasOutcome(const RunResult& r, const std::string& eventId);
Outcome     outcomeOf(const RunResult& r, const std::string& eventId);
std::vector<std::string> outcomeOrder(const RunResult& r);

std::vector<std::string> pendingIds(const RunResult& r);
std::vector<std::string> collectedIds(const RunResult& r);
const Parcel*            pendingParcel(const RunResult& r, const std::string& parcelId);
const Parcel*            storedParcel(const RunResult& r, const std::string& parcelId);

// ---- Querying validation ---------------------------------------------------

const RowError* errorOnRow(const std::vector<RowError>& errs, int row);
bool            anyError(const std::vector<RowError>& errs, const std::string& code);

// ---- Driving the built binary (sections F and H) ---------------------------

struct CliResult {
    int         exitCode = -1;
    std::string out;
    std::string err;
};

// A scratch directory that cleans itself up.
class Workspace {
  public:
    Workspace();
    ~Workspace();
    std::string       path(const std::string& name) const;
    void              write(const std::string& name, const std::string& text) const;
    std::string       read(const std::string& name) const;
    bool              exists(const std::string& name) const;
    CliResult         cli(const std::vector<std::string>& args) const;

  private:
    std::string dir_;
};

// BL-1 rendered as CSV text, written out longhand here so the loader
// scenarios assert against the document, not against src/csv.cpp.
std::string builtInCsvText();

struct CsvRun {
    bool               loaded = false;
    std::string        loadError;
    std::vector<Event> events;    // exactly as parsed, before normalization
    Handover           handover;  // empty when !loaded
};

CsvRun runCsvText(const std::string& text);

}  // namespace fx

// ---- Scenario-level assertion shorthands -----------------------------------

#define CHECK_SUMMARY(result, p, c, x)          \
    do {                                        \
        CHECK_EQ((result).pendingCount(), p);   \
        CHECK_EQ((result).collectedCount(), c); \
        CHECK_EQ((result).rejectedCount(), x);  \
    } while (0)

#define CHECK_IDS(actual, ...)                                            \
    do {                                                                  \
        std::vector<std::string> expected__ = __VA_ARGS__;                \
        std::vector<std::string> actual__   = (actual);                   \
        ::th::checkEq(__FILE__, __LINE__, #actual, "expected order",      \
                      ::fx::join(actual__), ::fx::join(expected__));      \
    } while (0)

namespace fx {
std::string join(const std::vector<std::string>& v);
}
