#include "model.h"

#include <algorithm>
#include <ostream>

namespace hd {

bool isRejection(Outcome o) {
    // P-10: exactly these four, and only these four.
    switch (o) {
        case Outcome::PARCEL_ALREADY_SEEN:
        case Outcome::ACTIVE_CODE_COLLISION:
        case Outcome::PARCEL_NOT_PENDING:
        case Outcome::PICKUP_CODE_MISMATCH:
            return true;
        case Outcome::ARRIVED:
        case Outcome::COLLECTED:
            return false;
    }
    return false;
}

const char* toString(Outcome o) {
    switch (o) {
        case Outcome::ARRIVED:               return "ARRIVED";
        case Outcome::COLLECTED:             return "COLLECTED";
        case Outcome::PARCEL_ALREADY_SEEN:   return "PARCEL_ALREADY_SEEN";
        case Outcome::ACTIVE_CODE_COLLISION: return "ACTIVE_CODE_COLLISION";
        case Outcome::PARCEL_NOT_PENDING:    return "PARCEL_NOT_PENDING";
        case Outcome::PICKUP_CODE_MISMATCH:  return "PICKUP_CODE_MISMATCH";
    }
    return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, Outcome o) { return os << toString(o); }

// O-2: the store is already in accepted-arrival order, so filtering preserves it.
std::vector<Parcel> RunResult::pending() const {
    std::vector<Parcel> out;
    for (const auto& p : parcels)
        if (p.isPending()) out.push_back(p);
    return out;
}

// O-3: filter the collected, then order by when they were collected.
std::vector<Parcel> RunResult::collected() const {
    std::vector<Parcel> out;
    for (const auto& p : parcels)
        if (!p.isPending()) out.push_back(p);
    std::sort(out.begin(), out.end(), [](const Parcel& a, const Parcel& b) {
        return a.collectedSeq < b.collectedSeq;
    });
    return out;
}

int RunResult::pendingCount() const {
    int n = 0;
    for (const auto& p : parcels)
        if (p.isPending()) ++n;
    return n;
}

int RunResult::collectedCount() const {
    int n = 0;
    for (const auto& p : parcels)
        if (!p.isPending()) ++n;
    return n;
}

int RunResult::rejectedCount() const {
    int n = 0;
    for (const auto& o : outcomes)
        if (isRejection(o.outcome)) ++n;
    return n;
}

}  // namespace hd
