#include "doda/DoDA_LibColonyBridge.h"
#include "doda/DoDA_SaveSchema.h"
#include "doda/thirdparty/libcolony/colony_msvc_compat.h"

#include <vector>
#include <map>
#include <set>
#include <cstdint>
#include <limits>

bool DoDA_OptimizeAssignments(const std::vector<DoDACandidateInput>& candidates,
                              std::vector<DoDAAssignmentOutput>& outAssignments)
{
    outAssignments.clear();

    if (candidates.empty())
    {
        return true;
    }

    std::map<std::pair<DoDAPersonId, DoDATaskId>, int> candidateCostMap;
    std::vector<colony::Assignment> colonyAssignments;
    colonyAssignments.reserve(candidates.size());

    for (const auto& cand : candidates)
    {
        if (cand.PersonId == 0 || cand.TaskId == 0)
        {
            return false;
        }

        if (cand.PersonId > static_cast<uint64_t>(std::numeric_limits<int>::max()) ||
            cand.TaskId > static_cast<uint64_t>(std::numeric_limits<int>::max()))
        {
            return false;
        }

        if (cand.BaseCost < 0 || cand.BaseCost > DODA_MAX_BASE_COST)
        {
            return false;
        }

        if (cand.TotalCost < 0 || cand.TotalCost >= (1LL << 53))
        {
            return false;
        }

        auto key = std::make_pair(cand.PersonId, cand.TaskId);
        if (!candidateCostMap.insert(std::make_pair(key, cand.BaseCost)).second)
        {
            return false;
        }

        colony::Assignment a{};
        a.character = static_cast<colony::CharacterId>(cand.PersonId);
        a.task = static_cast<colony::TaskId>(cand.TaskId);
        a.cost = static_cast<double>(cand.TotalCost);
        colonyAssignments.push_back(a);
    }

    colony::Optimize(colonyAssignments);

    if (colonyAssignments.size() > DODA_MAX_ASSIGNMENTS)
    {
        return false;
    }

    std::set<DoDAPersonId> assignedPeople;
    std::set<DoDATaskId> assignedTasks;

    for (const auto& a : colonyAssignments)
    {
        if (a.character <= 0 || a.task <= 0)
        {
            outAssignments.clear();
            return false;
        }

        DoDAPersonId personId = static_cast<DoDAPersonId>(a.character);
        DoDATaskId taskId = static_cast<DoDATaskId>(a.task);

        if (!assignedPeople.insert(personId).second)
        {
            outAssignments.clear();
            return false;
        }

        if (!assignedTasks.insert(taskId).second)
        {
            outAssignments.clear();
            return false;
        }

        auto it = candidateCostMap.find(std::make_pair(personId, taskId));
        if (it == candidateCostMap.end())
        {
            outAssignments.clear();
            return false;
        }

        DoDAAssignmentOutput output{};
        output.PersonId = personId;
        output.TaskId = taskId;
        output.BaseCost = it->second;
        outAssignments.push_back(output);
    }

    return true;
}
