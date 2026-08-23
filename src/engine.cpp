#include "engine.h"

namespace hd {
namespace {

// P-3 / CL-017: membership in the store *is* the seen-set. Collected parcels are
// never removed, so a parcel stays seen for the rest of the run.
const Parcel* findSeen(const std::vector<Parcel>& parcels, const std::string& id) {
    for (const auto& p : parcels)
        if (p.id == id) return &p;
    return nullptr;
}

// P-6 / CL-020: a COLLECT is resolved by parcel ID first, never by code.
Parcel* findPending(std::vector<Parcel>& parcels, const std::string& id) {
    for (auto& p : parcels)
        if (p.id == id && p.isPending()) return &p;
    return nullptr;
}

// P-4: the scan MUST filter isPending(), or a collected parcel's freed code
// would wrongly collide (CL-019, AC-3).
bool codeHeldByPending(const std::vector<Parcel>& parcels, const std::string& code) {
    for (const auto& p : parcels)
        if (p.isPending() && p.code == code) return true;
    return false;
}

}  // namespace

RunResult run(const std::vector<Event>& events) {
    RunResult r;                 // P-1: every run starts from empty state
    int nextCollectSeq = 0;      // P-1: and a zeroed collection counter

    // P-2: source order. Event IDs are labels and never reorder anything.
    for (const auto& e : events) {
        if (e.action == "ARRIVE") {
            if (findSeen(r.parcels, e.parcelId)) {
                // P-3, checked before P-4 (CL-024). No state change.
                r.outcomes.push_back({e.id, Outcome::PARCEL_ALREADY_SEEN});
            } else if (codeHeldByPending(r.parcels, e.code)) {
                // P-4. No state change, so the parcel is not marked seen (CL-018).
                r.outcomes.push_back({e.id, Outcome::ACTIVE_CODE_COLLISION});
            } else {
                // P-5: append to the one store. The index is the arrival
                // sequence, which is what O-2 orders by.
                r.parcels.push_back({e.parcelId, e.student, e.code, e.shelf, -1});
                r.outcomes.push_back({e.id, Outcome::ARRIVED});
            }
        } else if (e.action == "COLLECT") {
            Parcel* p = findPending(r.parcels, e.parcelId);
            if (!p) {
                // P-6: never arrived, or already collected (CL-022).
                r.outcomes.push_back({e.id, Outcome::PARCEL_NOT_PENDING});
            } else if (p->code != e.code) {
                // P-7. The parcel stays pending; whichever parcel owns the
                // quoted code is never collected by accident (CL-020).
                r.outcomes.push_back({e.id, Outcome::PICKUP_CODE_MISMATCH});
            } else {
                // P-8: stays in place, marked collected. Its code goes inactive
                // by virtue of the P-4 scan skipping non-pending parcels.
                p->collectedSeq = nextCollectSeq++;
                r.outcomes.push_back({e.id, Outcome::COLLECTED});
            }
        }
        // No other action reaches here: V-5 is enforced before the replay runs.
    }

    return r;
}

}  // namespace hd
