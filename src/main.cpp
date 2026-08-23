// main.cpp — the CLI. One invocation is one Run Handover (CL-034):
// load, validate, process and report all happen here, in that order.
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "csv.h"
#include "handover.h"
#include "report.h"

namespace {

constexpr int kOk         = 0;   // ran and validated, state rejections included
constexpr int kInvalid    = 1;   // structural validation failure (V-10)
constexpr int kIoError    = 2;   // nothing ran at all (CL-040)

const char* const kDefaultInput  = "events.csv";
const char* const kDefaultOutput = "handover-report.md";

struct Options {
    std::string input  = kDefaultInput;
    std::string output = kDefaultOutput;
    bool        reset  = false;
};

bool parseArgs(int argc, char** argv, Options& opt, std::string& error) {
    bool inputGiven = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];

        if (a == "--reset") {
            opt.reset = true;
        } else if (a == "--out") {
            if (i + 1 >= argc) {
                error = "--out requires a path";
                return false;
            }
            opt.output = argv[++i];
        } else if (a.rfind("--", 0) == 0) {
            error = "unknown option: " + a;
            return false;
        } else if (!inputGiven) {
            opt.input = a;
            inputGiven = true;
        } else {
            error = "unexpected argument: " + a;
            return false;
        }
    }
    return true;
}

bool writeFile(const std::string& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << text;
    return out.good();
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    std::string error;
    if (!parseArgs(argc, argv, opt, error)) {
        std::fprintf(stderr, "handover: %s\n", error.c_str());
        return kIoError;
    }

    // U-7 / CL-033 / CL-041: reset rewrites the input with the six built-in
    // events, creating it if absent, and deletes any existing report so no
    // stale outcomes, rows or counts survive. It does not run the handover.
    if (opt.reset) {
        if (!writeFile(opt.input, hd::builtInCsv())) {
            std::fprintf(stderr, "handover: cannot write input file: %s\n",
                         opt.input.c_str());
            return kIoError;
        }
        std::remove(opt.output.c_str());
        std::printf("Reset %s to the six built-in events. Cleared %s.\n",
                    opt.input.c_str(), opt.output.c_str());
        return kOk;
    }

    // A missing, unreadable or badly-headed file means nothing ran — which is
    // a different thing from an event log that failed validation (CL-040).
    hd::LoadResult loaded = hd::loadCsv(opt.input);
    if (!loaded.ok) {
        std::fprintf(stderr, "handover: %s\n", loaded.error.c_str());
        return kIoError;
    }

    hd::Handover h = hd::runHandover(loaded.events);

    // CL-025: the report is regenerated from the input on every run, so it can
    // never disagree with it. Overwritten silently — it is derived output,
    // never a record to preserve (CL-041).
    if (!writeFile(opt.output, hd::renderReport(h))) {
        std::fprintf(stderr, "handover: cannot write report: %s\n",
                     opt.output.c_str());
        return kIoError;
    }

    // CL-035: a short console summary alongside the markdown report.
    std::printf("%s\n", hd::renderConsoleSummary(h).c_str());
    std::printf("Report written to %s\n", opt.output.c_str());

    return h.valid() ? kOk : kInvalid;
}
