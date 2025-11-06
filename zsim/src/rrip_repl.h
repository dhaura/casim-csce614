#ifndef RRIP_REPL_H_
#define RRIP_REPL_H_

#include <algorithm>
#include "repl_policies.h"

// Static RRIP
class SRRIPReplPolicy : public ReplPolicy {
    protected:
        uint32_t rpvMax;
        uint32_t numLines;
        uint32_t* rpvArray;  // Array to hold RRPV values for each cache line.
        bool* evicted;  // Array to track if a line has been evicted.

    public:
        // add member methods here, refer to repl_policies.h
        explicit SRRIPReplPolicy(uint32_t numLines, uint32_t rpvMax) {
            this->rpvMax = rpvMax;
            this->numLines = numLines;
            rpvArray = gm_calloc<uint32_t>(numLines);
            evicted = gm_calloc<bool>(numLines);
            std::fill_n(rpvArray, numLines, rpvMax); // Initialize all RRPVs to max value.
            std::fill_n(evicted, numLines, true); // Initialize all lines as evicted.
        }

        ~SRRIPReplPolicy() {
            gm_free(rpvArray);
            gm_free(evicted);
        }

        void update(uint32_t id, const MemReq* req) {
            if (evicted[id]) {
                // When a cache line is inserted, set its RRPV to max - 1.
                rpvArray[id] = rpvMax - 1;
                // After setting the rrpv value for the newly inserted line, mark it as not evicted.
                evicted[id] = false;
            } else {
                // Hit Priority (HP) Policy: set RRPV to 0 on re-reference.
                rpvArray[id] = 0;
            }
        }

        void replaced(uint32_t id) {
            // Once the cache line is replaced, set its RRPV to max value and mark it as evicted.
            rpvArray[id] = rpvMax;
            evicted[id] = true;
        }

        template <typename C> inline uint32_t rank(const MemReq* req, C cands) {
            uint32_t bestCand = -1;
            bool found = false;
            while (!found) {
                for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
                    uint32_t rrpv = score(*ci);
                    // Find the first candidate with RRPV equal to max value.
                    if (rrpv == rpvMax) {
                        bestCand = *ci;
                        found = true;
                        break;
                    }
                }
                // If no candidate found with RRPV equal to max value, increment all RRPVs by 1.
                if (!found) {
                    // Increment all RRPVs by 1.
                    for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
                        rpvArray[*ci]++;
                    }
                }
            }
            return bestCand;
        }

        DECL_RANK_BINDINGS;

        private:
            inline uint64_t score(uint32_t id) {
                return cc->isValid(id) ? rpvArray[id] : rpvMax;
            }
};
#endif // RRIP_REPL_H_
