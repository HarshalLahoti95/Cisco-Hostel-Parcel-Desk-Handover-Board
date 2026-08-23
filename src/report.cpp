#include "report.h"

#include <sstream>
#include <string>

namespace hd {
namespace {

// A shelf label may be any non-empty string (CL-007), so a stray pipe would
// otherwise break the markdown table.
std::string cell(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '|') out += "\\|";
        else out += c;
    }
    return out;
}

void writeParcelTable(std::ostringstream& os, const std::vector<Parcel>& rows,
                      const char* emptyNote) {
    if (rows.empty()) {
        os << "_" << emptyNote << "_\n";
        return;
    }
    // CL-026: the incoming volunteer needs the code to hand the parcel over.
    os << "| Parcel | Student | Shelf | Pickup code |\n";
    os << "|---|---|---|---|\n";
    for (const auto& p : rows) {
        os << "| " << cell(p.id) << " | " << cell(p.student) << " | "
           << cell(p.shelf) << " | " << cell(p.code) << " |\n";
    }
}

}  // namespace

// U-9 / CL-036: derived at render time over the pending subset of the one
// parcel store. Groups appear in the order their first parcel arrived, so this
// introduces no ordering rule of its own beyond O-2.
std::vector<ShelfGroup> shelfMap(const RunResult& r) {
    std::vector<ShelfGroup> groups;

    for (const auto& p : r.pending()) {
        ShelfGroup* g = nullptr;
        for (auto& candidate : groups)
            if (candidate.shelf == p.shelf) { g = &candidate; break; }

        if (!g) {
            groups.push_back(ShelfGroup{p.shelf, {}});
            g = &groups.back();
        }
        g->parcelIds.push_back(p.id);
    }

    return groups;
}

std::string renderReport(const Handover& h) {
    std::ostringstream os;
    os << "# Hostel Parcel-Desk Handover\n\n";

    // U-6: the validation message. Never blank — silence is indistinguishable
    // from a crash (CL-027).
    if (!h.valid()) {
        os << "**INVALID — " << h.errors.size() << " row"
           << (h.errors.size() == 1 ? "" : "s") << " rejected**\n\n";

        // CL-009: one error per offending row, every offending row listed, so
        // the whole table can be fixed in a single pass.
        os << "## Validation Errors\n\n";
        os << "| Row | Event | Field | Error |\n";
        os << "|---|---|---|---|\n";
        for (const auto& e : h.errors) {
            os << "| " << e.row << " | " << cell(e.eventId) << " | "
               << cell(e.field) << " | " << e.code << " |\n";
        }

        // V-10: no outcomes, no handover rows, no summary counts.
        os << "\nNo event outcomes, handover rows or summary counts are "
              "produced for a table with a structural error.\n";
        return os.str();
    }

    os << "**VALID — " << h.events.size() << " events accepted**\n\n";

    // U-3 / O-1: one outcome per event, in source order.
    os << "## Event Outcomes\n\n";
    if (h.result.outcomes.empty()) {
        os << "_no events_\n";
    } else {
        os << "| Row | Event | Action | Parcel | Outcome |\n";
        os << "|---|---|---|---|---|\n";
        for (size_t i = 0; i < h.result.outcomes.size(); ++i) {
            const auto& o = h.result.outcomes[i];
            const auto& e = h.events[i];
            os << "| " << e.row << " | " << cell(o.id) << " | " << cell(e.action)
               << " | " << cell(e.parcelId) << " | " << toString(o.outcome)
               << " |\n";
        }
    }

    // U-4 / O-2: the handover state, in accepted-arrival order.
    os << "\n## Pending Board\n\n";
    writeParcelTable(os, h.result.pending(), "nothing pending");

    // U-5 / O-3 / CL-028: their own section, in collection order.
    os << "\n## Collected\n\n";
    writeParcelTable(os, h.result.collected(), "nothing collected");

    os << "\n## Shelf Map\n\n";
    auto map = shelfMap(h.result);
    if (map.empty()) {
        os << "_no parcels on the shelves_\n";
    } else {
        for (const auto& g : map) {
            os << "- **" << cell(g.shelf) << "** — ";
            for (size_t i = 0; i < g.parcelIds.size(); ++i) {
                if (i) os << ", ";
                os << cell(g.parcelIds[i]);
            }
            os << "\n";
        }
    }

    os << "\n## Summary\n\n";
    os << "| Pending | Collected | Rejected |\n";
    os << "|---|---|---|\n";
    os << "| " << h.result.pendingCount() << " | " << h.result.collectedCount()
       << " | " << h.result.rejectedCount() << " |\n";

    return os.str();
}

std::string renderConsoleSummary(const Handover& h) {
    std::ostringstream os;

    if (!h.valid()) {
        os << "INVALID - " << h.errors.size() << " row"
           << (h.errors.size() == 1 ? "" : "s") << " rejected";
        for (const auto& e : h.errors)
            os << "\n  row " << e.row << ": " << e.code << " on " << e.field
               << (e.eventId.empty() ? "" : " (event " + e.eventId + ")");
        return os.str();
    }

    os << "VALID - " << h.events.size() << " events accepted  |  "
       << h.result.pendingCount() << " pending / " << h.result.collectedCount()
       << " collected / " << h.result.rejectedCount() << " rejected";
    return os.str();
}

}  // namespace hd
