#include "doda/DoDA_Simulation.h"
#include "doda/DoDA_SaveSchema.h"
#include "serializer.h"
#include "printf.h"
#include <algorithm>
#include <set>

DoDASimulation& DoDASimulation::GetInstance()
{
    static DoDASimulation instance;
    return instance;
}

void DoDASimulation::ClearToDefaultState()
{
    mStrategicMinutes = 0;
    mNextPersonId = 1;
    mNextTaskId = 1;
    mNextAssignmentId = 1;
    mPeople.clear();
    mTasks.clear();
    mAssignments.clear();
}

void DoDASimulation::ClearForNewCampaign()
{
    ClearToDefaultState();
    Printf("[DoDA] New campaign: strategic state cleared.\n");
}

void DoDASimulation::Serialize(FSerializer& arc)
{
    if (arc.isWriting())
    {
        if (arc.BeginObject("doda"))
        {
            int schemaVersion = DODA_SAVE_SCHEMA_VERSION;
            arc("schema_version", schemaVersion);
            arc("strategic_minutes", mStrategicMinutes);
            arc("next_person_id", mNextPersonId);
            arc("next_task_id", mNextTaskId);
            arc("next_assignment_id", mNextAssignmentId);

            if (arc.BeginArray("people"))
            {
                for (const auto& person : mPeople)
                {
                    if (arc.BeginObject(nullptr))
                    {
                        DoDAPersonId id = person.Id;
                        FString name = person.Name;
                        int skill = person.Skill;
                        int workload = person.Workload;
                        int rawStatus = static_cast<int>(person.Status);

                        arc("id", id)
                           ("name", name)
                           ("skill", skill)
                           ("workload", workload)
                           ("status", rawStatus);
                        arc.EndObject();
                    }
                }
                arc.EndArray();
            }

            if (arc.BeginArray("tasks"))
            {
                for (const auto& task : mTasks)
                {
                    if (arc.BeginObject(nullptr))
                    {
                        DoDATaskId id = task.Id;
                        FString title = task.Title;
                        int priority = task.Priority;
                        int requiredSkill = task.RequiredSkill;
                        int estimatedWork = task.EstimatedWork;
                        int rawStatus = static_cast<int>(task.Status);

                        arc("id", id)
                           ("title", title)
                           ("priority", priority)
                           ("required_skill", requiredSkill)
                           ("estimated_work", estimatedWork)
                           ("status", rawStatus);
                        arc.EndObject();
                    }
                }
                arc.EndArray();
            }

            if (arc.BeginArray("assignments"))
            {
                for (const auto& assignment : mAssignments)
                {
                    if (arc.BeginObject(nullptr))
                    {
                        DoDAAssignmentId id = assignment.Id;
                        DoDAPersonId personId = assignment.PersonId;
                        DoDATaskId taskId = assignment.TaskId;
                        int cost = assignment.Cost;

                        arc("id", id)
                           ("person_id", personId)
                           ("task_id", taskId)
                           ("cost", cost);
                        arc.EndObject();
                    }
                }
                arc.EndArray();
            }

            arc.EndObject();
        }
    }
    else if (arc.isReading())
    {
        if (!arc.BeginObject("doda"))
        {
            ClearToDefaultState();
            return;
        }

        if (!arc.HasKey("schema_version"))
        {
            ClearToDefaultState();
            Printf("[DoDA] Error: Missing schema_version in savegame; reset to default state.\n");
            arc.EndObject();
            return;
        }

        int schemaVersion = 0;
        arc("schema_version", schemaVersion);

        if (schemaVersion < 1 || schemaVersion > DODA_SAVE_SCHEMA_VERSION)
        {
            ClearToDefaultState();
            Printf("[DoDA] Error: Unsupported save schema version %d (max supported %d); reset to default state.\n",
                schemaVersion, DODA_SAVE_SCHEMA_VERSION);
            arc.EndObject();
            return;
        }

        if (schemaVersion == 1)
        {
            if (!arc.HasKey("strategic_minutes"))
            {
                ClearToDefaultState();
                Printf("[DoDA] Error: Schema 1 missing strategic_minutes; reset to default state.\n");
                arc.EndObject();
                return;
            }

            uint64_t tempMinutes = 0;
            arc("strategic_minutes", tempMinutes);
            arc.EndObject();

            ClearToDefaultState();
            mStrategicMinutes = tempMinutes;
            return;
        }

        if (schemaVersion == 2)
        {
            if (!arc.HasKey("strategic_minutes") ||
                !arc.HasKey("next_person_id") ||
                !arc.HasKey("next_task_id") ||
                !arc.HasKey("next_assignment_id") ||
                !arc.HasKey("people") ||
                !arc.HasKey("tasks") ||
                !arc.HasKey("assignments"))
            {
                ClearToDefaultState();
                Printf("[DoDA] Error: Schema 2 missing required root fields; reset to default state.\n");
                arc.EndObject();
                return;
            }

            uint64_t tempMinutes = 0;
            DoDAPersonId tempNextPersonId = 0;
            DoDATaskId tempNextTaskId = 0;
            DoDAAssignmentId tempNextAssignmentId = 0;
            std::vector<DoDAPersonRecord> tempPeople;
            std::vector<DoDATaskRecord> tempTasks;
            std::vector<DoDAAssignmentRecord> tempAssignments;

            arc("strategic_minutes", tempMinutes);
            arc("next_person_id", tempNextPersonId);
            arc("next_task_id", tempNextTaskId);
            arc("next_assignment_id", tempNextAssignmentId);

            bool parseOk = true;

            if (arc.BeginArray("people"))
            {
                unsigned peopleCount = arc.ArraySize();
                if (peopleCount > DODA_MAX_PEOPLE)
                {
                    parseOk = false;
                }
                else
                {
                    tempPeople.reserve(peopleCount);
                    for (unsigned i = 0; i < peopleCount; ++i)
                    {
                        if (!arc.BeginObject(nullptr))
                        {
                            parseOk = false;
                            break;
                        }

                        if (!arc.HasKey("id") || !arc.HasKey("name") || !arc.HasKey("skill") ||
                            !arc.HasKey("workload") || !arc.HasKey("status"))
                        {
                            parseOk = false;
                            arc.EndObject();
                            break;
                        }

                        DoDAPersonId id = 0;
                        FString name;
                        int skill = 0;
                        int workload = 0;
                        int rawStatus = 0;

                        arc("id", id)
                           ("name", name)
                           ("skill", skill)
                           ("workload", workload)
                           ("status", rawStatus);
                        arc.EndObject();

                        if (rawStatus < 0 || rawStatus > static_cast<int>(DoDAPersonStatus::Unavailable))
                        {
                            parseOk = false;
                            break;
                        }

                        DoDAPersonRecord person{};
                        person.Id = id;
                        person.Name = name;
                        person.Skill = skill;
                        person.Workload = workload;
                        person.Status = static_cast<DoDAPersonStatus>(rawStatus);
                        tempPeople.push_back(person);
                    }
                }
                arc.EndArray();
            }
            else
            {
                parseOk = false;
            }

            if (parseOk && arc.BeginArray("tasks"))
            {
                unsigned tasksCount = arc.ArraySize();
                if (tasksCount > DODA_MAX_TASKS)
                {
                    parseOk = false;
                }
                else
                {
                    tempTasks.reserve(tasksCount);
                    for (unsigned i = 0; i < tasksCount; ++i)
                    {
                        if (!arc.BeginObject(nullptr))
                        {
                            parseOk = false;
                            break;
                        }

                        if (!arc.HasKey("id") || !arc.HasKey("title") || !arc.HasKey("priority") ||
                            !arc.HasKey("required_skill") || !arc.HasKey("estimated_work") || !arc.HasKey("status"))
                        {
                            parseOk = false;
                            arc.EndObject();
                            break;
                        }

                        DoDATaskId id = 0;
                        FString title;
                        int priority = 0;
                        int requiredSkill = 0;
                        int estimatedWork = 0;
                        int rawStatus = 0;

                        arc("id", id)
                           ("title", title)
                           ("priority", priority)
                           ("required_skill", requiredSkill)
                           ("estimated_work", estimatedWork)
                           ("status", rawStatus);
                        arc.EndObject();

                        if (rawStatus < 0 || rawStatus > static_cast<int>(DoDATaskStatus::Failed))
                        {
                            parseOk = false;
                            break;
                        }

                        DoDATaskRecord task{};
                        task.Id = id;
                        task.Title = title;
                        task.Priority = priority;
                        task.RequiredSkill = requiredSkill;
                        task.EstimatedWork = estimatedWork;
                        task.Status = static_cast<DoDATaskStatus>(rawStatus);
                        tempTasks.push_back(task);
                    }
                }
                arc.EndArray();
            }
            else if (parseOk)
            {
                parseOk = false;
            }

            if (parseOk && arc.BeginArray("assignments"))
            {
                unsigned assignmentsCount = arc.ArraySize();
                if (assignmentsCount > DODA_MAX_ASSIGNMENTS)
                {
                    parseOk = false;
                }
                else
                {
                    tempAssignments.reserve(assignmentsCount);
                    for (unsigned i = 0; i < assignmentsCount; ++i)
                    {
                        if (!arc.BeginObject(nullptr))
                        {
                            parseOk = false;
                            break;
                        }

                        if (!arc.HasKey("id") || !arc.HasKey("person_id") || !arc.HasKey("task_id") || !arc.HasKey("cost"))
                        {
                            parseOk = false;
                            arc.EndObject();
                            break;
                        }

                        DoDAAssignmentId id = 0;
                        DoDAPersonId personId = 0;
                        DoDATaskId taskId = 0;
                        int cost = 0;

                        arc("id", id)
                           ("person_id", personId)
                           ("task_id", taskId)
                           ("cost", cost);
                        arc.EndObject();

                        DoDAAssignmentRecord assignment{};
                        assignment.Id = id;
                        assignment.PersonId = personId;
                        assignment.TaskId = taskId;
                        assignment.Cost = cost;
                        tempAssignments.push_back(assignment);
                    }
                }
                arc.EndArray();
            }
            else if (parseOk)
            {
                parseOk = false;
            }

            arc.EndObject();

            if (!parseOk)
            {
                ClearToDefaultState();
                Printf("[DoDA] Error: Schema 2 array parsing failed or exceeded limits; reset to default state.\n");
                return;
            }

            if (tempNextPersonId < 1 || tempNextTaskId < 1 || tempNextAssignmentId < 1)
            {
                ClearToDefaultState();
                Printf("[DoDA] Error: Schema 2 counters must be >= 1; reset to default state.\n");
                return;
            }

            std::set<DoDAPersonId> personIds;
            for (const auto& p : tempPeople)
            {
                if (p.Id == 0 || p.Id >= tempNextPersonId)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 person ID %llu invalid or exceeds counter %llu; reset to default state.\n",
                        static_cast<unsigned long long>(p.Id), static_cast<unsigned long long>(tempNextPersonId));
                    return;
                }
                if (!personIds.insert(p.Id).second)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 duplicate person ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(p.Id));
                    return;
                }
                if (p.Name.IsEmpty() || static_cast<size_t>(p.Name.Len()) > DODA_MAX_PERSON_NAME_LENGTH)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 person name empty or exceeds max length; reset to default state.\n");
                    return;
                }
                if (p.Skill < 0 || p.Workload < 0)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 person skill/workload negative; reset to default state.\n");
                    return;
                }
                if (static_cast<uint8_t>(p.Status) > static_cast<uint8_t>(DoDAPersonStatus::Unavailable))
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 person status out of enum range; reset to default state.\n");
                    return;
                }
            }

            std::set<DoDATaskId> taskIds;
            for (const auto& t : tempTasks)
            {
                if (t.Id == 0 || t.Id >= tempNextTaskId)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 task ID %llu invalid or exceeds counter %llu; reset to default state.\n",
                        static_cast<unsigned long long>(t.Id), static_cast<unsigned long long>(tempNextTaskId));
                    return;
                }
                if (!taskIds.insert(t.Id).second)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 duplicate task ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(t.Id));
                    return;
                }
                if (t.Title.IsEmpty() || static_cast<size_t>(t.Title.Len()) > DODA_MAX_TASK_TITLE_LENGTH)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 task title empty or exceeds max length; reset to default state.\n");
                    return;
                }
                if (t.Priority < 0 || t.RequiredSkill < 0 || t.EstimatedWork < 0)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 task priority/skill/work negative; reset to default state.\n");
                    return;
                }
                if (static_cast<uint8_t>(t.Status) > static_cast<uint8_t>(DoDATaskStatus::Failed))
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 task status out of enum range; reset to default state.\n");
                    return;
                }
            }

            std::set<DoDAAssignmentId> assignmentIds;
            std::set<DoDAPersonId> assignedPersonIds;
            std::set<DoDATaskId> assignedTaskIds;

            for (const auto& a : tempAssignments)
            {
                if (a.Id == 0 || a.Id >= tempNextAssignmentId)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 assignment ID %llu invalid or exceeds counter %llu; reset to default state.\n",
                        static_cast<unsigned long long>(a.Id), static_cast<unsigned long long>(tempNextAssignmentId));
                    return;
                }
                if (!assignmentIds.insert(a.Id).second)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 duplicate assignment ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(a.Id));
                    return;
                }
                if (a.Cost < 0)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 assignment cost negative; reset to default state.\n");
                    return;
                }
                if (personIds.find(a.PersonId) == personIds.end())
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 assignment references non-existent person ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(a.PersonId));
                    return;
                }
                if (taskIds.find(a.TaskId) == taskIds.end())
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 assignment references non-existent task ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(a.TaskId));
                    return;
                }
                if (!assignedPersonIds.insert(a.PersonId).second)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 person ID %llu appears in multiple assignments; reset to default state.\n",
                        static_cast<unsigned long long>(a.PersonId));
                    return;
                }
                if (!assignedTaskIds.insert(a.TaskId).second)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 task ID %llu appears in multiple assignments; reset to default state.\n",
                        static_cast<unsigned long long>(a.TaskId));
                    return;
                }
            }

            for (const auto& p : tempPeople)
            {
                bool isAssignedInRecords = (assignedPersonIds.count(p.Id) > 0);
                if (isAssignedInRecords && p.Status != DoDAPersonStatus::Assigned)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 person ID %llu assigned in records but status is not Assigned; reset to default state.\n",
                        static_cast<unsigned long long>(p.Id));
                    return;
                }
                if (!isAssignedInRecords && p.Status == DoDAPersonStatus::Assigned)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 person ID %llu status is Assigned but no matching assignment record; reset to default state.\n",
                        static_cast<unsigned long long>(p.Id));
                    return;
                }
            }

            for (const auto& t : tempTasks)
            {
                bool isAssignedInRecords = (assignedTaskIds.count(t.Id) > 0);
                if (isAssignedInRecords && t.Status != DoDATaskStatus::Assigned)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 task ID %llu assigned in records but status is not Assigned; reset to default state.\n",
                        static_cast<unsigned long long>(t.Id));
                    return;
                }
                if (!isAssignedInRecords && t.Status == DoDATaskStatus::Assigned)
                {
                    ClearToDefaultState();
                    Printf("[DoDA] Error: Schema 2 task ID %llu status is Assigned but no matching assignment record; reset to default state.\n",
                        static_cast<unsigned long long>(t.Id));
                    return;
                }
            }

            mStrategicMinutes = tempMinutes;
            mNextPersonId = tempNextPersonId;
            mNextTaskId = tempNextTaskId;
            mNextAssignmentId = tempNextAssignmentId;
            mPeople = std::move(tempPeople);
            mTasks = std::move(tempTasks);
            mAssignments = std::move(tempAssignments);

            Printf("[DoDA] Restore: Successfully loaded Schema 2 state (%u people, %u tasks, %u assignments, %llu minutes).\n",
                static_cast<unsigned>(mPeople.size()),
                static_cast<unsigned>(mTasks.size()),
                static_cast<unsigned>(mAssignments.size()),
                static_cast<unsigned long long>(mStrategicMinutes));
        }
    }
}

bool DoDASimulation::ResetDebugFixture()
{
    ClearToDefaultState();

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
