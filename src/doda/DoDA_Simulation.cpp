#include "doda/DoDA_Simulation.h"
#include "printf.h"
#include <algorithm>

DoDASimulation& DoDASimulation::GetInstance()
{
    static DoDASimulation instance;
    return instance;
}

bool DoDASimulation::ResetDebugFixture()
{
    mStrategicMinutes = 0;
    mNextPersonId = 1;
    mNextTaskId = 1;
    mNextAssignmentId = 1;
    mPeople.clear();
    mTasks.clear();
    mAssignments.clear();

    mPeople.push_back({ mNextPersonId++, "Alice", 5, 1, DoDAPersonStatus::Available });
    mPeople.push_back({ mNextPersonId++, "Bob", 3, 0, DoDAPersonStatus::Available });
    mPeople.push_back({ mNextPersonId++, "Charlie", 8, 2, DoDAPersonStatus::Available });

    mTasks.push_back({ mNextTaskId++, "Scout Perimeter", 10, 2, 20, DoDATaskStatus::Pending });
    mTasks.push_back({ mNextTaskId++, "Repair Generator", 20, 4, 30, DoDATaskStatus::Pending });
    mTasks.push_back({ mNextTaskId++, "Decrypt Archive", 15, 10, 40, DoDATaskStatus::Pending });

    Printf("[DoDA] ResetDebugFixture: Initialized fixture with %u people and %u tasks.\n",
        static_cast<unsigned>(mPeople.size()),
        static_cast<unsigned>(mTasks.size()));

    return true;
}

bool DoDASimulation::AdvanceStrategicMinutes(int deltaMinutes)
{
    if (deltaMinutes <= 0)
    {
        return false;
    }

    if (deltaMinutes > 1440)
    {
        deltaMinutes = 1440;
    }

    mStrategicMinutes += static_cast<uint64_t>(deltaMinutes);

    Printf("[DoDA] AdvanceStrategicMinutes: Advanced by %d minute(s), total %llu minute(s).\n",
        deltaMinutes,
        static_cast<unsigned long long>(mStrategicMinutes));

    return true;
}

bool DoDASimulation::RunDeterministicAssignment()
{
    struct Candidate
    {
        int TaskPriority;
        DoDATaskId TaskId;
        int Cost;
        int PersonWorkload;
        DoDAPersonId PersonId;
        size_t TaskIndex;
        size_t PersonIndex;
    };

    std::vector<Candidate> candidates;

    for (size_t t = 0; t < mTasks.size(); ++t)
    {
        const auto& task = mTasks[t];
        if (task.Status != DoDATaskStatus::Pending)
            continue;

        for (size_t p = 0; p < mPeople.size(); ++p)
        {
            const auto& person = mPeople[p];
            if (person.Status != DoDAPersonStatus::Available)
                continue;

            if (person.Skill < task.RequiredSkill)
                continue;

            int skillGap = task.RequiredSkill - person.Skill;
            int skillGapPenalty = std::max(0, skillGap);
            int rawCost = skillGapPenalty + person.Workload * 10 + task.EstimatedWork - person.Skill * 5;
            int cost = std::max(0, rawCost);

            Candidate cand;
            cand.TaskPriority = task.Priority;
            cand.TaskId = task.Id;
            cand.Cost = cost;
            cand.PersonWorkload = person.Workload;
            cand.PersonId = person.Id;
            cand.TaskIndex = t;
            cand.PersonIndex = p;

            candidates.push_back(cand);
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        if (a.TaskPriority != b.TaskPriority)
            return a.TaskPriority > b.TaskPriority;
        if (a.TaskId != b.TaskId)
            return a.TaskId < b.TaskId;
        if (a.Cost != b.Cost)
            return a.Cost < b.Cost;
        if (a.PersonWorkload != b.PersonWorkload)
            return a.PersonWorkload < b.PersonWorkload;
        return a.PersonId < b.PersonId;
    });

    std::vector<bool> taskAssigned(mTasks.size(), false);
    std::vector<bool> personAssigned(mPeople.size(), false);

    struct PendingMatch
    {
        size_t TaskIndex;
        size_t PersonIndex;
        DoDATaskId TaskId;
        DoDAPersonId PersonId;
        int Cost;
    };
    std::vector<PendingMatch> matches;

    for (const auto& cand : candidates)
    {
        if (taskAssigned[cand.TaskIndex] || personAssigned[cand.PersonIndex])
            continue;

        taskAssigned[cand.TaskIndex] = true;
        personAssigned[cand.PersonIndex] = true;

        PendingMatch match;
        match.TaskIndex = cand.TaskIndex;
        match.PersonIndex = cand.PersonIndex;
        match.TaskId = cand.TaskId;
        match.PersonId = cand.PersonId;
        match.Cost = cand.Cost;
        matches.push_back(match);
    }

    // Validate candidates before commit
    for (const auto& match : matches)
    {
        if (match.TaskIndex >= mTasks.size() || match.PersonIndex >= mPeople.size())
            return false;
        if (mTasks[match.TaskIndex].Id != match.TaskId || mPeople[match.PersonIndex].Id != match.PersonId)
            return false;
        if (mTasks[match.TaskIndex].Status != DoDATaskStatus::Pending)
            return false;
        if (mPeople[match.PersonIndex].Status != DoDAPersonStatus::Available)
            return false;
        if (mPeople[match.PersonIndex].Skill < mTasks[match.TaskIndex].RequiredSkill)
            return false;
    }

    // Commit assignments
    for (const auto& match : matches)
    {
        mTasks[match.TaskIndex].Status = DoDATaskStatus::Assigned;
        mPeople[match.PersonIndex].Status = DoDAPersonStatus::Assigned;

        DoDAAssignmentRecord rec;
        rec.Id = mNextAssignmentId++;
        rec.PersonId = match.PersonId;
        rec.TaskId = match.TaskId;
        rec.Cost = match.Cost;
        mAssignments.push_back(rec);
    }

    Printf("[DoDA] RunDeterministicAssignment: Generated and committed %u assignment(s).\n",
        static_cast<unsigned>(matches.size()));

    return true;
}

FString DoDASimulation::GetStrategicSummary() const
{
    FString summary;
    summary.Format("StrategicMinutes=%llu People=%u Tasks=%u Assignments=%u",
        static_cast<unsigned long long>(mStrategicMinutes),
        static_cast<unsigned>(mPeople.size()),
        static_cast<unsigned>(mTasks.size()),
        static_cast<unsigned>(mAssignments.size()));
    return summary;
}

FString DoDASimulation::GetAssignmentSummary() const
{
    FString summary;
    summary.Format("AssignmentsCount=%u",
        static_cast<unsigned>(mAssignments.size()));
    return summary;
}

FString DoDASimulation::GetAssignmentDebugText(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mAssignments.size())
    {
        return FString("");
    }

    const auto& rec = mAssignments[static_cast<size_t>(index)];
    FString text;
    text.Format("PersonId=%llu TaskId=%llu Cost=%d",
        static_cast<unsigned long long>(rec.PersonId),
        static_cast<unsigned long long>(rec.TaskId),
        rec.Cost);
    return text;
}
