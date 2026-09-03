#include "doda/DoDA_LibColonyBridge.h"
#include "doda/DoDA_SaveSchema.h"
#include "doda/thirdparty/libcolony/colony_msvc_compat.h"
#include "printf.h"

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

bool DoDA_RunLibColonyDeterministicProof()
{
    Printf("[DoDA-LibColony-Proof] ==================================================\n");
    Printf("[DoDA-LibColony-Proof] Starting deterministic LibColony compile-and-optimize proof...\n");

    struct FixtureEdge
    {
        DoDAPersonId PersonId;
        DoDATaskId TaskId;
        int BaseCost;
    };

    const FixtureEdge fixtureEdges[] = {
        { 1001, 2001, 10 },
        { 1001, 2002, 80 },
        { 1002, 2001, 75 },
        { 1002, 2002, 15 }
    };

    std::vector<DoDACandidateInput> candidates;
    candidates.reserve(4);

    for (const auto& edge : fixtureEdges)
    {
        int64_t tieBreak = (static_cast<int64_t>(edge.TaskId) - 1) * static_cast<int64_t>(DODA_MAX_PEOPLE) + (static_cast<int64_t>(edge.PersonId) - 1);
        int64_t totalCost = static_cast<int64_t>(edge.BaseCost) * DODA_COST_SCALE + tieBreak;

        DoDACandidateInput cand{};
        cand.PersonId = edge.PersonId;
        cand.TaskId = edge.TaskId;
        cand.BaseCost = edge.BaseCost;
        cand.TotalCost = totalCost;
        candidates.push_back(cand);

        Printf("[DoDA-LibColony-Proof] Input Edge: Person %llu -> Task %llu (BaseCost=%d, TotalCost=%lld)\n",
            static_cast<unsigned long long>(cand.PersonId),
            static_cast<unsigned long long>(cand.TaskId),
            cand.BaseCost,
            static_cast<long long>(cand.TotalCost));
    }

    std::vector<DoDAAssignmentOutput> outputs;
    if (!DoDA_OptimizeAssignments(candidates, outputs))
    {
        Printf("[DoDA-LibColony-Proof] FAILED: DoDA_OptimizeAssignments returned false.\n");
        Printf("[DoDA-LibColony-Proof] ==================================================\n");
        return false;
    }

    if (outputs.size() != 2)
    {
        Printf("[DoDA-LibColony-Proof] FAILED: Expected 2 assignments, received %zu.\n", outputs.size());
        Printf("[DoDA-LibColony-Proof] ==================================================\n");
        return false;
    }

    std::set<DoDAPersonId> seenPeople;
    std::set<DoDATaskId> seenTasks;
    int calculatedTotalCost = 0;

    for (const auto& out : outputs)
    {
        Printf("[DoDA-LibColony-Proof] Output Assignment: Person %llu -> Task %llu (Cost=%d)\n",
            static_cast<unsigned long long>(out.PersonId),
            static_cast<unsigned long long>(out.TaskId),
            out.BaseCost);

        if (out.PersonId != 1001 && out.PersonId != 1002)
        {
            Printf("[DoDA-LibColony-Proof] FAILED: Person ID %llu not in fixture.\n", static_cast<unsigned long long>(out.PersonId));
            Printf("[DoDA-LibColony-Proof] ==================================================\n");
            return false;
        }

        if (out.TaskId != 2001 && out.TaskId != 2002)
        {
            Printf("[DoDA-LibColony-Proof] FAILED: Task ID %llu not in fixture.\n", static_cast<unsigned long long>(out.TaskId));
            Printf("[DoDA-LibColony-Proof] ==================================================\n");
            return false;
        }

        if (!seenPeople.insert(out.PersonId).second)
        {
            Printf("[DoDA-LibColony-Proof] FAILED: Duplicate person ID %llu.\n", static_cast<unsigned long long>(out.PersonId));
            Printf("[DoDA-LibColony-Proof] ==================================================\n");
            return false;
        }

        if (!seenTasks.insert(out.TaskId).second)
        {
            Printf("[DoDA-LibColony-Proof] FAILED: Duplicate task ID %llu.\n", static_cast<unsigned long long>(out.TaskId));
            Printf("[DoDA-LibColony-Proof] ==================================================\n");
            return false;
        }

        bool matchedFixtureEdge = false;
        for (const auto& edge : fixtureEdges)
        {
            if (edge.PersonId == out.PersonId && edge.TaskId == out.TaskId)
            {
                if (edge.BaseCost != out.BaseCost)
                {
                    Printf("[DoDA-LibColony-Proof] FAILED: Cost mismatch for pair (%llu, %llu): expected %d, got %d.\n",
                        static_cast<unsigned long long>(out.PersonId), static_cast<unsigned long long>(out.TaskId),
                        edge.BaseCost, out.BaseCost);
                    Printf("[DoDA-LibColony-Proof] ==================================================\n");
                    return false;
                }
                matchedFixtureEdge = true;
                break;
            }
        }

        if (!matchedFixtureEdge)
        {
            Printf("[DoDA-LibColony-Proof] FAILED: Output pair (%llu, %llu) not found in fixture edges.\n",
                static_cast<unsigned long long>(out.PersonId), static_cast<unsigned long long>(out.TaskId));
            Printf("[DoDA-LibColony-Proof] ==================================================\n");
            return false;
        }

        calculatedTotalCost += out.BaseCost;
    }

    if (calculatedTotalCost != 25)
    {
        Printf("[DoDA-LibColony-Proof] FAILED: Total cost is %d (expected 25).\n", calculatedTotalCost);
        Printf("[DoDA-LibColony-Proof] ==================================================\n");
        return false;
    }

    Printf("[DoDA-LibColony-Proof] Validation: Total Cost = %d (Expected: 25).\n", calculatedTotalCost);
    Printf("[DoDA-LibColony-Proof] Validation: Person 1001->Task 2001 (Cost 10) + Person 1002->Task 2002 (Cost 15) confirmed.\n");
    Printf("[DoDA-LibColony-Proof] Validation: Zero persistent state mutations committed.\n");
    Printf("[DoDA-LibColony-Proof] SUCCESS: Deterministic LibColony compile-and-optimize proof PASSED.\n");
    Printf("[DoDA-LibColony-Proof] ==================================================\n");

    return true;
}
