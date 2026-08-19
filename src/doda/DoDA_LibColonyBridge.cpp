#include "doda/thirdparty/libcolony/colony_msvc_compat.h"

#include <vector>

namespace
{
    // Minimal private compile smoke construct to validate LibColony templates/types under MSVC C++20.
    // This is never invoked at runtime and exposes no types or symbols outside this translation unit.
    [[maybe_unused]] void DoDA_LibColony_CompileSmoke()
    {
        std::vector<colony::Assignment> sampleAssignments;
        sampleAssignments.push_back({0, 0, colony::ComputeCost(1.0, 2.0, 0.0, 1.0)});
        colony::LimitAssignments(sampleAssignments, 1, 1);
        colony::Optimize(sampleAssignments);
    }
}
