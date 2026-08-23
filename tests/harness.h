// harness.h — minimal assertion helper. No framework dependency (CL-037).
#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace th {

struct Failure {
    std::string message;
};

[[noreturn]] void fail(const char* file, int line, const std::string& what);

template <typename T>
std::string show(const T& v) {
    std::ostringstream os;
    os << v;
    return os.str();
}
inline std::string show(const std::string& v) { return "\"" + v + "\""; }
inline std::string show(bool v) { return v ? "true" : "false"; }

template <typename A, typename B>
void checkEq(const char* file, int line, const char* ea, const char* eb,
             const A& a, const B& b) {
    if (!(a == b)) {
        fail(file, line,
             std::string(ea) + " == " + eb + "\n      actual: " + show(a) +
                 "\n    expected: " + show(b));
    }
}

using TestFn = void (*)();

struct TestCase {
    std::string id;       // e.g. "TC-001"
    std::string section;  // e.g. "A. Acceptance Criteria"
    std::string name;     // the scenario title, copied from test-cases.md
    TestFn      fn;
};

void registerTest(const TestCase& tc);

struct Registrar {
    Registrar(const char* id, const char* section, const char* name, TestFn fn) {
        registerTest({id, section, name, fn});
    }
};

int runAll(int argc, char** argv);

}  // namespace th

// SCENARIO(id, section, title) — one Gherkin scenario from docs/test-cases.md.
#define SCENARIO(id, section, title)                                      \
    static void id##_body();                                              \
    static ::th::Registrar id##_reg(#id, section, title, &id##_body);     \
    static void id##_body()

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) ::th::fail(__FILE__, __LINE__, #cond);               \
    } while (0)

#define CHECK_MSG(cond, msg)                                              \
    do {                                                                  \
        if (!(cond))                                                      \
            ::th::fail(__FILE__, __LINE__,                                \
                       std::string(#cond) + " — " + (msg));               \
    } while (0)

#define CHECK_EQ(a, b) ::th::checkEq(__FILE__, __LINE__, #a, #b, (a), (b))
