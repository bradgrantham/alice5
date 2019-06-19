
#include "pcopy.h"

// Find pair whose source matches the specified location.
static std::vector<PCopyPair>::const_iterator findSource(
        const std::vector<PCopyPair> &pairs,
        const PCopyLocation &location) {

    for (auto itr = pairs.begin(); itr != pairs.end(); itr++) {
        if (itr->mSource == location) {
            return itr;
        }
    }

    return pairs.end();
}

// Return whether the location appears as a source in the list of pairs.
static bool isSource(const std::vector<PCopyPair> &pairs, const PCopyLocation &location) {
    return findSource(pairs, location) != pairs.end();
}

void parallel_copy(const std::vector<PCopyPair> &pairsOrig,
        std::vector<PCopyInstruction> &instructions) {

    instructions.clear();

    // Make a copy of the pairs so we can delete items as we deal with them.
    std::vector<PCopyPair> pairs = pairsOrig;

    // First do the simple copies.
    bool found;
    do {
        found = false;

        // Look for a pair whose destination is no longer needed.
        for (auto itr = pairs.begin(); itr != pairs.end(); itr++) {
            if (!isSource(pairs, itr->mDestination)) {
                // We can copy to this destination.
                instructions.push_back({PCOPY_OP_MOVE, *itr});
                pairs.erase(itr);
                found = true;
                break;
            }
        }

    } while (found);

    // From now on everything is a cycle.
    while (!pairs.empty()) {
        // Pull any pair.
        PCopyPair pair = pairs.back();
        pairs.pop_back();

        // If it's a self-copy, drop it.
        if (pair.mSource == pair.mDestination) {
            continue;
        }

        // Make a list of the cycle in order of movement.
        std::vector<PCopyLocation> cycle;
        cycle.push_back(pair.mDestination);

        // Find other pairs, walking through cycle.
        while (true) {
            auto itr = findSource(pairs, pair.mDestination);
            if (itr == pairs.end()) {
                break;
            }

            pair = *itr;
            cycle.push_back(pair.mDestination);
            pairs.erase(itr);
        }

        // Work backward through cycle.
        for (int i = cycle.size() - 2; i >= 0; i--) {
            instructions.push_back({PCOPY_OP_EXCHANGE, {cycle[i], cycle[i + 1]}});
        }
    }
}

