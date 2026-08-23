#include "fixtures.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/wait.h>

#include "../src/csv.h"

namespace fs = std::filesystem;

namespace fx {

Event arrive(const std::string& id, const std::string& parcel,
             const std::string& code, const std::string& shelf,
             const std::string& student) {
    Event e;
    e.id = id;
    e.action = "ARRIVE";
    e.parcelId = parcel;
    e.student = student;
    e.code = code;
    e.shelf = shelf;
    return e;
}

Event collect(const std::string& id, const std::string& parcel,
              const std::string& code) {
    Event e;
    e.id = id;
    e.action = "COLLECT";
    e.parcelId = parcel;
    e.code = code;
    return e;  // student and shelf deliberately blank (V-8)
}

std::vector<Event> logOf(std::initializer_list<Event> events) {
    std::vector<Event> out(events);
    for (size_t i = 0; i < out.size(); ++i) out[i].row = static_cast<int>(i + 1);
    return out;
}

std::vector<Event> builtInLog() {
    return logOf({
        arrive("E01", "P01", "K7M2", "A1", "Asha"),
        arrive("E02", "P02", "R4Q8", "B1", "Bilal"),
        collect("E03", "P01", "ZZZZ"),
        arrive("E04", "P03", "T9C4", "A2", "Chen"),
        collect("E05", "P02", "R4Q8"),
        arrive("E06", "P04", "H2N6", "B2", "Divya"),
    });
}

Event& row(std::vector<Event>& log, int n) {
    return log.at(static_cast<size_t>(n - 1));
}

Handover run(const std::vector<Event>& raw) { return hd::runHandover(raw); }

bool hasOutcome(const RunResult& r, const std::string& eventId) {
    for (const auto& o : r.outcomes)
        if (o.id == eventId) return true;
    return false;
}

Outcome outcomeOf(const RunResult& r, const std::string& eventId) {
    for (const auto& o : r.outcomes)
        if (o.id == eventId) return o.outcome;
    throw th::Failure{"no outcome recorded for event " + eventId};
}

std::vector<std::string> outcomeOrder(const RunResult& r) {
    std::vector<std::string> ids;
    for (const auto& o : r.outcomes) ids.push_back(o.id);
    return ids;
}

std::vector<std::string> pendingIds(const RunResult& r) {
    std::vector<std::string> ids;
    for (const auto& p : r.pending()) ids.push_back(p.id);
    return ids;
}

std::vector<std::string> collectedIds(const RunResult& r) {
    std::vector<std::string> ids;
    for (const auto& p : r.collected()) ids.push_back(p.id);
    return ids;
}

const Parcel* pendingParcel(const RunResult& r, const std::string& parcelId) {
    for (const auto& p : r.parcels)
        if (p.id == parcelId && p.isPending()) return &p;
    return nullptr;
}

const Parcel* storedParcel(const RunResult& r, const std::string& parcelId) {
    for (const auto& p : r.parcels)
        if (p.id == parcelId) return &p;
    return nullptr;
}

const RowError* errorOnRow(const std::vector<RowError>& errs, int r) {
    for (const auto& e : errs)
        if (e.row == r) return &e;
    return nullptr;
}

bool anyError(const std::vector<RowError>& errs, const std::string& code) {
    for (const auto& e : errs)
        if (e.code == code) return true;
    return false;
}

std::string join(const std::vector<std::string>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) s += ", ";
        s += v[i];
    }
    return "[" + s + "]";
}

std::string builtInCsvText() {
    return
        "event_id,action,parcel_id,student,pickup_code,shelf\n"
        "E01,ARRIVE,P01,Asha,K7M2,A1\n"
        "E02,ARRIVE,P02,Bilal,R4Q8,B1\n"
        "E03,COLLECT,P01,,ZZZZ,\n"
        "E04,ARRIVE,P03,Chen,T9C4,A2\n"
        "E05,COLLECT,P02,,R4Q8,\n"
        "E06,ARRIVE,P04,Divya,H2N6,B2\n";
}

CsvRun runCsvText(const std::string& text) {
    CsvRun r;
    hd::LoadResult lr = hd::parseCsv(text);
    r.loaded = lr.ok;
    r.loadError = lr.error;
    r.events = lr.events;
    if (lr.ok) r.handover = hd::runHandover(lr.events);
    return r;
}

// ---- Workspace -------------------------------------------------------------

Workspace::Workspace() {
    auto base = fs::temp_directory_path() / fs::path("hd-test-" + std::to_string(::rand()));
    fs::create_directories(base);
    dir_ = base.string();
}

Workspace::~Workspace() {
    std::error_code ec;
    fs::remove_all(dir_, ec);
}

std::string Workspace::path(const std::string& name) const {
    return (fs::path(dir_) / name).string();
}

void Workspace::write(const std::string& name, const std::string& text) const {
    std::ofstream out(path(name), std::ios::binary);
    out << text;
}

std::string Workspace::read(const std::string& name) const {
    std::ifstream in(path(name), std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool Workspace::exists(const std::string& name) const {
    return fs::exists(path(name));
}

namespace {
std::string shellQuote(const std::string& s) {
    std::string q = "'";
    for (char c : s) {
        if (c == '\'') q += "'\\''";
        else q += c;
    }
    return q + "'";
}
}  // namespace

CliResult Workspace::cli(const std::vector<std::string>& args) const {
    CliResult res;
    std::string outFile = path("__stdout"), errFile = path("__stderr");

    std::string cmd = "cd " + shellQuote(dir_) + " && " + shellQuote(HANDOVER_BIN);
    for (const auto& a : args) cmd += " " + shellQuote(a);
    cmd += " >" + shellQuote(outFile) + " 2>" + shellQuote(errFile);

    int rc = std::system(cmd.c_str());
    res.exitCode = WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
    res.out = read("__stdout");
    res.err = read("__stderr");
    return res;
}

}  // namespace fx
