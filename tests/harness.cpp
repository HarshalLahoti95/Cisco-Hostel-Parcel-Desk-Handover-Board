#include "harness.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>

namespace th {
namespace {

std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

const char* kGreen = "\033[32m";
const char* kRed   = "\033[31m";
const char* kDim   = "\033[2m";
const char* kReset = "\033[0m";

}  // namespace

void fail(const char* file, int line, const std::string& what) {
    const char* base = std::strrchr(file, '/');
    std::ostringstream os;
    os << (base ? base + 1 : file) << ":" << line << "  " << what;
    throw Failure{os.str()};
}

namespace {
// The SCENARIO macro must spell ids with underscores to form a C++ identifier,
// but docs/test-cases.md spells them TC-001. Display and filter on the
// document's form so the two never have to be translated by hand.
std::string docId(std::string id) {
    for (char& c : id)
        if (c == '_') c = '-';
    return id;
}
}  // namespace

void registerTest(const TestCase& tc) {
    TestCase copy = tc;
    copy.id = docId(tc.id);
    registry().push_back(copy);
}

int runAll(int argc, char** argv) {
    std::string filter;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (!a.empty() && a[0] != '-') filter = a;
    }

    auto tests = registry();
    std::sort(tests.begin(), tests.end(),
              [](const TestCase& a, const TestCase& b) { return a.id < b.id; });

    std::string                  currentSection;
    int                          passed = 0, failed = 0, skipped = 0;
    std::vector<std::string>     failures;
    std::map<std::string, std::pair<int, int>> bySection;  // section -> {pass, total}

    for (const auto& tc : tests) {
        const std::string needle = docId(filter);
        if (!filter.empty() &&
            tc.id.find(needle) == std::string::npos &&
            tc.section.find(filter) == std::string::npos) {
            ++skipped;
            continue;
        }
        if (tc.section != currentSection) {
            currentSection = tc.section;
            std::printf("\n%s%s%s\n", kDim, currentSection.c_str(), kReset);
        }

        auto& tally = bySection[tc.section];
        ++tally.second;

        try {
            tc.fn();
            ++passed;
            ++tally.first;
            std::printf("  %sPASS%s %s  %s\n", kGreen, kReset, tc.id.c_str(),
                        tc.name.c_str());
        } catch (const Failure& f) {
            ++failed;
            std::printf("  %sFAIL%s %s  %s\n       %s\n", kRed, kReset,
                        tc.id.c_str(), tc.name.c_str(), f.message.c_str());
            failures.push_back(tc.id + "  " + tc.name);
        } catch (const std::exception& e) {
            ++failed;
            std::printf("  %sFAIL%s %s  %s\n       threw: %s\n", kRed, kReset,
                        tc.id.c_str(), tc.name.c_str(), e.what());
            failures.push_back(tc.id + "  (exception)");
        }
    }

    std::printf("\n%s%s%s\n", kDim, std::string(60, '-').c_str(), kReset);
    for (const auto& kv : bySection) {
        std::printf("  %-34s %2d / %-2d\n", kv.first.c_str(), kv.second.first,
                    kv.second.second);
    }
    std::printf("%s%s%s\n", kDim, std::string(60, '-').c_str(), kReset);
    std::printf("  %s%d passed%s, %s%d failed%s, %d total",
                passed ? kGreen : kDim, passed, kReset,
                failed ? kRed : kDim, failed, kReset, passed + failed);
    if (skipped) std::printf(" (%d filtered out)", skipped);
    std::printf("\n\n");

    return failed == 0 ? 0 : 1;
}

}  // namespace th
