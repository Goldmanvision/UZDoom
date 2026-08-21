#include "doda/DoDA_Simulation.h"
#include "doda/DoDA_SaveSchema.h"
#include "doda/DoDA_LibColonyBridge.h"
#include "serializer.h"
#include "printf.h"
#include <algorithm>
#include <map>
#include <set>

DoDASimulation& DoDASimulation::GetInstance()
{
    static DoDASimulation instance;
    return instance;
}

bool DoDASimulation::HasPersonnelSnapshotV1State() const
{
    return !mPeople.empty() || !mTasks.empty() || !mAssignments.empty() || !mLocations.empty() || mStrategicMinutes != 0;
}

void DoDASimulation::ClearToDefaultState()
{
    mStrategicMinutes = 0;
    mNextPersonId = 1;
    mNextTaskId = 1;
    mNextAssignmentId = 1;
    mNextLocationId = 1;
    mPeople.clear();
    mLocations.clear();
    mTasks.clear();
    mAssignments.clear();
}

void DoDASimulation::ResetPersonnelSnapshotAfterLoadFailure()
{
    const bool projectionChanged = HasPersonnelSnapshotV1State();
    ClearToDefaultState();

    if (projectionChanged)
    {
        BumpPersonnelSnapshotRevision();
    }
}

void DoDASimulation::InitializeDefaultFixture()
{
    ClearToDefaultState();

    mLocations.push_back({ mNextLocationId++, "Morrow Point Waterworks" });

    mPeople.push_back({ mNextPersonId++, "Ruth M. Green", 5, 0, DoDAPersonStatus::Available });
    mPeople.push_back({ mNextPersonId++, "Michelle C. Thomas", 7, 1, DoDAPersonStatus::Assigned });
    mPeople.push_back({ mNextPersonId++, "Brian C. Gordon", 8, 1, DoDAPersonStatus::Assigned });
    mPeople.push_back({ mNextPersonId++, "Leonard M. Martin", 6, 0, DoDAPersonStatus::Available });
    mPeople.push_back({ mNextPersonId++, "Harold M. Beltz", 5, 0, DoDAPersonStatus::Available });

    mTasks.push_back({ mNextTaskId++, "Survey anomalous emissions", 1, 10, 5, 40, DoDATaskStatus::Assigned, 0 });

    mAssignments.push_back({ mNextAssignmentId++, 2, 1, 0 });
    mAssignments.push_back({ mNextAssignmentId++, 3, 1, 0 });
}

void DoDASimulation::ClearForNewCampaign()
{
    const bool projectionChanged = HasPersonnelSnapshotV1State();
    InitializeDefaultFixture();

    if (projectionChanged)
    {
        BumpPersonnelSnapshotRevision();
    }
    Printf("[DoDA] New campaign: strategic state initialized with %u personnel, %u location(s), %u task(s), %u assignment(s).\n",
        static_cast<unsigned>(mPeople.size()),
        static_cast<unsigned>(mLocations.size()),
        static_cast<unsigned>(mTasks.size()),
        static_cast<unsigned>(mAssignments.size()));
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
                        // NOTE: task.LocationId is deliberately NOT serialized in this POC.
                        // Writing location_id without also serializing mLocations/mNextLocationId
                        // would persist ids that reference a location list rebuilt only by the
                        // hardcoded fallback below. Deferred to the strategic-time/persistence batch.
                        DoDATaskId id = task.Id;
                        FString title = task.Title;
                        int priority = task.Priority;
                        int requiredSkill = task.RequiredSkill;
                        int estimatedWork = task.EstimatedWork;
                        int rawStatus = static_cast<int>(task.Status);
                        int progress = task.Progress;

                        arc("id", id)
                           ("title", title)
                           ("priority", priority)
                           ("required_skill", requiredSkill)
                           ("estimated_work", estimatedWork)
                           ("status", rawStatus)
                           ("progress", progress);
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
            ResetPersonnelSnapshotAfterLoadFailure();
            return;
        }

        if (!arc.HasKey("schema_version"))
        {
            ResetPersonnelSnapshotAfterLoadFailure();
            Printf("[DoDA] Error: Missing schema_version in savegame; reset to default state.\n");
            arc.EndObject();
            return;
        }

        int schemaVersion = 0;
        arc("schema_version", schemaVersion);

        if (schemaVersion < 1 || schemaVersion > DODA_SAVE_SCHEMA_VERSION)
        {
            ResetPersonnelSnapshotAfterLoadFailure();
            Printf("[DoDA] Error: Unsupported save schema version %d (max supported %d); reset to default state.\n",
                schemaVersion, DODA_SAVE_SCHEMA_VERSION);
            arc.EndObject();
            return;
        }

        if (schemaVersion == 1)
        {
            if (!arc.HasKey("strategic_minutes"))
            {
                ResetPersonnelSnapshotAfterLoadFailure();
                Printf("[DoDA] Error: Schema 1 missing strategic_minutes; reset to default state.\n");
                arc.EndObject();
                return;
            }

            uint64_t tempMinutes = 0;
            arc("strategic_minutes", tempMinutes);
            arc.EndObject();

            const bool projectionChanged = HasPersonnelSnapshotV1State();
            ClearToDefaultState();
            mStrategicMinutes = tempMinutes;
            if (projectionChanged || tempMinutes != 0)
            {
                BumpPersonnelSnapshotRevision();
            }
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
                ResetPersonnelSnapshotAfterLoadFailure();
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
                        DoDALocationId locId = 1;
                        int priority = 0;
                        int requiredSkill = 0;
                        int estimatedWork = 0;
                        int rawStatus = 0;
                        int progress = 0;

                        arc("id", id)
                           ("title", title)
                           ("priority", priority)
                           ("required_skill", requiredSkill)
                           ("estimated_work", estimatedWork)
                           ("status", rawStatus);
                        if (arc.HasKey("location_id"))
                            arc("location_id", locId);
                        if (arc.HasKey("progress"))
                            arc("progress", progress);
                        arc.EndObject();

                        if (rawStatus < 0 || rawStatus > static_cast<int>(DoDATaskStatus::Failed))
                        {
                            parseOk = false;
                            break;
                        }

                        DoDATaskRecord task{};
                        task.Id = id;
                        task.Title = title;
                        task.LocationId = locId;
                        task.Priority = priority;
                        task.RequiredSkill = requiredSkill;
                        task.EstimatedWork = estimatedWork;
                        task.Status = static_cast<DoDATaskStatus>(rawStatus);
                        task.Progress = progress;
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
                ResetPersonnelSnapshotAfterLoadFailure();
                Printf("[DoDA] Error: Schema 2 array parsing failed or exceeded limits; reset to default state.\n");
                return;
            }

            if (tempNextPersonId < 1 || tempNextTaskId < 1 || tempNextAssignmentId < 1)
            {
                ResetPersonnelSnapshotAfterLoadFailure();
                Printf("[DoDA] Error: Schema 2 counters must be >= 1; reset to default state.\n");
                return;
            }

            std::set<DoDAPersonId> personIds;
            for (const auto& p : tempPeople)
            {
                if (p.Id == 0 || p.Id >= tempNextPersonId)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 person ID %llu invalid or exceeds counter %llu; reset to default state.\n",
                        static_cast<unsigned long long>(p.Id), static_cast<unsigned long long>(tempNextPersonId));
                    return;
                }
                if (!personIds.insert(p.Id).second)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 duplicate person ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(p.Id));
                    return;
                }
                if (p.Name.IsEmpty() || static_cast<size_t>(p.Name.Len()) > DODA_MAX_PERSON_NAME_LENGTH)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 person name empty or exceeds max length; reset to default state.\n");
                    return;
                }
                if (p.Skill < 0 || p.Workload < 0)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 person skill/workload negative; reset to default state.\n");
                    return;
                }
                if (static_cast<uint8_t>(p.Status) > static_cast<uint8_t>(DoDAPersonStatus::Unavailable))
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 person status out of enum range; reset to default state.\n");
                    return;
                }
            }

            std::set<DoDATaskId> taskIds;
            for (const auto& t : tempTasks)
            {
                if (t.Id == 0 || t.Id >= tempNextTaskId)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 task ID %llu invalid or exceeds counter %llu; reset to default state.\n",
                        static_cast<unsigned long long>(t.Id), static_cast<unsigned long long>(tempNextTaskId));
                    return;
                }
                if (!taskIds.insert(t.Id).second)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 duplicate task ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(t.Id));
                    return;
                }
                if (t.Title.IsEmpty() || static_cast<size_t>(t.Title.Len()) > DODA_MAX_TASK_TITLE_LENGTH)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 task title empty or exceeds max length; reset to default state.\n");
                    return;
                }
                if (t.Priority < 0 || t.RequiredSkill < 0 || t.EstimatedWork < 0)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 task priority/skill/work negative; reset to default state.\n");
                    return;
                }
                if (static_cast<uint8_t>(t.Status) > static_cast<uint8_t>(DoDATaskStatus::Failed))
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 task status out of enum range; reset to default state.\n");
                    return;
                }
                if (t.Progress < 0 || t.Progress > 100)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 task progress %d out of range 0-100; reset to default state.\n",
                        t.Progress);
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
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 assignment ID %llu invalid or exceeds counter %llu; reset to default state.\n",
                        static_cast<unsigned long long>(a.Id), static_cast<unsigned long long>(tempNextAssignmentId));
                    return;
                }
                if (!assignmentIds.insert(a.Id).second)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 duplicate assignment ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(a.Id));
                    return;
                }
                if (a.Cost < 0)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 assignment cost negative; reset to default state.\n");
                    return;
                }
                if (personIds.find(a.PersonId) == personIds.end())
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 assignment references non-existent person ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(a.PersonId));
                    return;
                }
                if (taskIds.find(a.TaskId) == taskIds.end())
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 assignment references non-existent task ID %llu; reset to default state.\n",
                        static_cast<unsigned long long>(a.TaskId));
                    return;
                }
                if (!assignedPersonIds.insert(a.PersonId).second)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 person ID %llu appears in multiple assignments; reset to default state.\n",
                        static_cast<unsigned long long>(a.PersonId));
                    return;
                }
                // NOTE: a task may legitimately carry several assignments (multiple personnel
                // working one task); only the per-person uniqueness above is an invariant.
                // assignedTaskIds is still collected for the task status cross-link check below.
                assignedTaskIds.insert(a.TaskId);
            }

            for (const auto& p : tempPeople)
            {
                bool isAssignedInRecords = (assignedPersonIds.count(p.Id) > 0);
                if (isAssignedInRecords && p.Status != DoDAPersonStatus::Assigned)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 person ID %llu assigned in records but status is not Assigned; reset to default state.\n",
                        static_cast<unsigned long long>(p.Id));
                    return;
                }
                if (!isAssignedInRecords && p.Status == DoDAPersonStatus::Assigned)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
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
                    ResetPersonnelSnapshotAfterLoadFailure();
                    Printf("[DoDA] Error: Schema 2 task ID %llu assigned in records but status is not Assigned; reset to default state.\n",
                        static_cast<unsigned long long>(t.Id));
                    return;
                }
                if (!isAssignedInRecords && t.Status == DoDATaskStatus::Assigned)
                {
                    ResetPersonnelSnapshotAfterLoadFailure();
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

            if (mLocations.empty())
            {
                mLocations.push_back({ 1, "Morrow Point Waterworks" });
                mNextLocationId = 2;
            }

            BumpPersonnelSnapshotRevision();

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
    InitializeDefaultFixture();
    BumpPersonnelSnapshotRevision();

    Printf("[DoDA] ResetDebugFixture: Initialized fixture with %u personnel, %u location(s), %u task(s), %u assignment(s).\n",
        static_cast<unsigned>(mPeople.size()),
        static_cast<unsigned>(mLocations.size()),
        static_cast<unsigned>(mTasks.size()),
        static_cast<unsigned>(mAssignments.size()));

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

    for (auto& task : mTasks)
    {
        if (task.Status == DoDATaskStatus::Assigned)
        {
            if (task.Progress < 100)
            {
                int addedProgress = (deltaMinutes * 25) / 60;
                task.Progress += addedProgress;
                if (task.Progress >= 100)
                {
                    task.Progress = 100;
                    task.Status = DoDATaskStatus::Completed;

                    std::set<DoDAPersonId> completedPersonIds;
                    for (const auto& a : mAssignments)
                    {
                        if (a.TaskId == task.Id)
                        {
                            completedPersonIds.insert(a.PersonId);
                        }
                    }

                    for (auto& p : mPeople)
                    {
                        if (completedPersonIds.find(p.Id) != completedPersonIds.end())
                        {
                            p.Status = DoDAPersonStatus::Available;
                            p.Workload = 0;
                        }
                    }

                    mAssignments.erase(
                        std::remove_if(mAssignments.begin(), mAssignments.end(),
                            [&task](const DoDAAssignmentRecord& a) {
                                return a.TaskId == task.Id;
                            }),
                        mAssignments.end()
                    );

                    Printf("[DoDA] Task %llu ('%s') reached 100%% progress -> Completed. %u personnel returned to Available.\n",
                        static_cast<unsigned long long>(task.Id),
                        task.Title.GetChars(),
                        static_cast<unsigned>(completedPersonIds.size()));
                }
            }
        }
    }

    BumpPersonnelSnapshotRevision();

    Printf("[DoDA] AdvanceStrategicMinutes: Advanced by %d minute(s), total %llu minute(s).\n",
        deltaMinutes,
        static_cast<unsigned long long>(mStrategicMinutes));

    return true;
}

bool DoDASimulation::RunDeterministicAssignment()
{
    if (!mAssignments.empty())
    {
        Printf("[DoDA] Error: RunDeterministicAssignment called with pre-existing assignments; single-run policy enforced.\n");
        return false;
    }

    std::vector<DoDACandidateInput> candidates;

    for (const auto& task : mTasks)
    {
        if (task.Status != DoDATaskStatus::Pending)
            continue;
        if (task.Id == 0 || task.Id >= mNextTaskId)
            continue;
        if (task.Priority < 0 || task.RequiredSkill < 0 || task.EstimatedWork < 0)
            continue;

        for (const auto& person : mPeople)
        {
            if (person.Status != DoDAPersonStatus::Available)
                continue;
            if (person.Id == 0 || person.Id >= mNextPersonId)
                continue;
            if (person.Skill < 0 || person.Workload < 0)
                continue;

            if (person.Skill < task.RequiredSkill)
                continue;

            const int64_t rawCost =
                static_cast<int64_t>(person.Workload) * 10 +
                static_cast<int64_t>(task.EstimatedWork) -
                static_cast<int64_t>(person.Skill) * 5;

            const int64_t baseCost64 = std::max<int64_t>(0, rawCost);
            if (baseCost64 > DODA_MAX_BASE_COST)
                continue;

            const int baseCost = static_cast<int>(baseCost64);

            int64_t tieBreak = (static_cast<int64_t>(task.Id) - 1) * static_cast<int64_t>(DODA_MAX_PEOPLE) + (static_cast<int64_t>(person.Id) - 1);
            if (tieBreak < 0)
                continue;

            int64_t totalCost = static_cast<int64_t>(baseCost) * DODA_COST_SCALE + tieBreak;
            if (totalCost < 0 || totalCost >= (1LL << 53))
                continue;

            DoDACandidateInput cand{};
            cand.PersonId = person.Id;
            cand.TaskId = task.Id;
            cand.BaseCost = baseCost;
            cand.TotalCost = totalCost;
            candidates.push_back(cand);
        }
    }

    std::vector<DoDAAssignmentOutput> optimizedOutputs;
    if (!DoDA_OptimizeAssignments(candidates, optimizedOutputs))
    {
        Printf("[DoDA] Error: LibColony optimization bridge failed; strategic state unchanged.\n");
        return false;
    }

    // Sort assignments by TaskId descending (and PersonId ascending) for deterministic storage order
    std::sort(optimizedOutputs.begin(), optimizedOutputs.end(), [](const DoDAAssignmentOutput& a, const DoDAAssignmentOutput& b) {
        if (a.TaskId != b.TaskId)
            return a.TaskId > b.TaskId;
        return a.PersonId < b.PersonId;
    });

    if (optimizedOutputs.size() > DODA_MAX_ASSIGNMENTS)
    {
        Printf("[DoDA] Error: Assignment count exceeds maximum capacity; strategic state unchanged.\n");
        return false;
    }

    std::map<std::pair<DoDAPersonId, DoDATaskId>, int> candidateMap;
    for (const auto& cand : candidates)
    {
        candidateMap[std::make_pair(cand.PersonId, cand.TaskId)] = cand.BaseCost;
    }

    std::set<DoDAPersonId> matchedPeople;
    std::set<DoDATaskId> matchedTasks;
    std::vector<size_t> matchedPersonIndices;
    std::vector<size_t> matchedTaskIndices;
    matchedPersonIndices.reserve(optimizedOutputs.size());
    matchedTaskIndices.reserve(optimizedOutputs.size());

    for (const auto& out : optimizedOutputs)
    {
        if (out.PersonId == 0 || out.TaskId == 0)
        {
            Printf("[DoDA] Error: Assignment output contains invalid zero ID; strategic state unchanged.\n");
            return false;
        }

        if (!matchedPeople.insert(out.PersonId).second)
        {
            Printf("[DoDA] Error: Duplicate person ID %llu in optimization output; strategic state unchanged.\n",
                static_cast<unsigned long long>(out.PersonId));
            return false;
        }

        if (!matchedTasks.insert(out.TaskId).second)
        {
            Printf("[DoDA] Error: Duplicate task ID %llu in optimization output; strategic state unchanged.\n",
                static_cast<unsigned long long>(out.TaskId));
            return false;
        }

        auto candIt = candidateMap.find(std::make_pair(out.PersonId, out.TaskId));
        if (candIt == candidateMap.end())
        {
            Printf("[DoDA] Error: Optimization output (%llu, %llu) not in input candidates; strategic state unchanged.\n",
                static_cast<unsigned long long>(out.PersonId),
                static_cast<unsigned long long>(out.TaskId));
            return false;
        }

        if (out.BaseCost != candIt->second)
        {
            Printf("[DoDA] Error: Optimization output BaseCost mismatch; strategic state unchanged.\n");
            return false;
        }

        size_t personIndex = mPeople.size();
        for (size_t i = 0; i < mPeople.size(); ++i)
        {
            if (mPeople[i].Id == out.PersonId)
            {
                personIndex = i;
                break;
            }
        }
        if (personIndex >= mPeople.size())
        {
            Printf("[DoDA] Error: Assigned person ID %llu not found in canonical records; strategic state unchanged.\n",
                static_cast<unsigned long long>(out.PersonId));
            return false;
        }

        size_t taskIndex = mTasks.size();
        for (size_t i = 0; i < mTasks.size(); ++i)
        {
            if (mTasks[i].Id == out.TaskId)
            {
                taskIndex = i;
                break;
            }
        }
        if (taskIndex >= mTasks.size())
        {
            Printf("[DoDA] Error: Assigned task ID %llu not found in canonical records; strategic state unchanged.\n",
                static_cast<unsigned long long>(out.TaskId));
            return false;
        }

        const auto& person = mPeople[personIndex];
        const auto& task = mTasks[taskIndex];

        if (person.Status != DoDAPersonStatus::Available)
        {
            Printf("[DoDA] Error: Assigned person ID %llu is not Available; strategic state unchanged.\n",
                static_cast<unsigned long long>(out.PersonId));
            return false;
        }

        if (task.Status != DoDATaskStatus::Pending)
        {
            Printf("[DoDA] Error: Assigned task ID %llu is not Pending; strategic state unchanged.\n",
                static_cast<unsigned long long>(out.TaskId));
            return false;
        }

        if (person.Skill < task.RequiredSkill)
        {
            Printf("[DoDA] Error: Assigned person ID %llu skill %d below task required skill %d; strategic state unchanged.\n",
                static_cast<unsigned long long>(out.PersonId), person.Skill, task.RequiredSkill);
            return false;
        }

        if (person.Skill < 0 || person.Workload < 0 || task.Priority < 0 || task.RequiredSkill < 0 || task.EstimatedWork < 0)
        {
            Printf("[DoDA] Error: Negative person/task attributes in assignment validation; strategic state unchanged.\n");
            return false;
        }

        const int64_t expectedRawCost =
            static_cast<int64_t>(person.Workload) * 10 +
            static_cast<int64_t>(task.EstimatedWork) -
            static_cast<int64_t>(person.Skill) * 5;

        const int64_t expectedBaseCost64 = std::max<int64_t>(0, expectedRawCost);
        if (expectedBaseCost64 > DODA_MAX_BASE_COST)
        {
            Printf("[DoDA] Error: Expected BaseCost exceeds maximum allowed base cost; strategic state unchanged.\n");
            return false;
        }

        const int expectedBaseCost = static_cast<int>(expectedBaseCost64);
        if (out.BaseCost != expectedBaseCost)
        {
            Printf("[DoDA] Error: Output BaseCost %d differs from recalculation %d; strategic state unchanged.\n",
                out.BaseCost, expectedBaseCost);
            return false;
        }

        matchedPersonIndices.push_back(personIndex);
        matchedTaskIndices.push_back(taskIndex);
    }

    std::vector<DoDAAssignmentRecord> newAssignments;
    newAssignments.reserve(optimizedOutputs.size());
    DoDAAssignmentId nextAssignmentId = mNextAssignmentId;

    for (const auto& out : optimizedOutputs)
    {
        DoDAAssignmentRecord rec{};
        rec.Id = nextAssignmentId++;
        rec.PersonId = out.PersonId;
        rec.TaskId = out.TaskId;
        rec.Cost = out.BaseCost;
        newAssignments.push_back(rec);
    }

    for (size_t personIndex : matchedPersonIndices)
    {
        mPeople[personIndex].Status = DoDAPersonStatus::Assigned;
    }

    for (size_t taskIndex : matchedTaskIndices)
    {
        mTasks[taskIndex].Status = DoDATaskStatus::Assigned;
    }

    mNextAssignmentId = nextAssignmentId;
    mAssignments = std::move(newAssignments);

    BumpPersonnelSnapshotRevision();

    Printf("[DoDA] RunDeterministicAssignment: Generated and committed %u assignment(s).\n",
        static_cast<unsigned>(mAssignments.size()));

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

FString DoDASimulation::FormatUInt64Decimal(uint64_t value)
{
    FString text;
    text.Format("%llu", static_cast<unsigned long long>(value));
    return text;
}

void DoDASimulation::BumpPersonnelSnapshotRevision()
{
    ++mPersonnelSnapshotRevision;
    if (mPersonnelSnapshotRevision == 0)
    {
        mPersonnelSnapshotRevision = 1;
    }
}

FString DoDASimulation::GetPersonnelSnapshotRevisionText() const
{
    return FormatUInt64Decimal(mPersonnelSnapshotRevision);
}

FString DoDASimulation::GetPersonnelSnapshotStrategicMinutesText() const
{
    return FormatUInt64Decimal(mStrategicMinutes);
}

FString DoDASimulation::GetPersonnelPersonIdText(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mPeople.size())
    {
        return "";
    }
    return FormatUInt64Decimal(mPeople[index].Id);
}

FString DoDASimulation::GetPersonnelDisplayName(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mPeople.size())
    {
        return "";
    }
    return mPeople[index].Name;
}

int DoDASimulation::GetPersonnelStatus(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mPeople.size())
    {
        return 2;
    }
    return static_cast<int>(mPeople[index].Status);
}

int DoDASimulation::GetPersonnelSkill(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mPeople.size())
    {
        return 0;
    }
    return mPeople[index].Skill;
}

int DoDASimulation::GetPersonnelWorkload(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mPeople.size())
    {
        return 0;
    }
    return mPeople[index].Workload;
}

FString DoDASimulation::GetPersonnelCurrentAssignmentIdText(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mPeople.size())
    {
        return "";
    }

    const DoDAPersonId personId = mPeople[index].Id;
    for (const auto& assignment : mAssignments)
    {
        if (assignment.PersonId == personId)
        {
            return FormatUInt64Decimal(assignment.Id);
        }
    }
    return "";
}

FString DoDASimulation::GetPersonnelCurrentTaskIdText(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mPeople.size())
    {
        return "";
    }

    const DoDAPersonId personId = mPeople[index].Id;
    for (const auto& assignment : mAssignments)
    {
        if (assignment.PersonId == personId)
        {
            return FormatUInt64Decimal(assignment.TaskId);
        }
    }
    return "";
}

FString DoDASimulation::GetPersonnelCurrentTaskTitle(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mPeople.size())
    {
        return "";
    }

    const DoDAPersonId personId = mPeople[index].Id;
    for (const auto& assignment : mAssignments)
    {
        if (assignment.PersonId == personId)
        {
            for (const auto& task : mTasks)
            {
                if (task.Id == assignment.TaskId)
                {
                    return task.Title;
                }
            }
            return "";
        }
    }
    return "";
}

FString DoDASimulation::GetLocationIdText(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mLocations.size())
    {
        return "";
    }
    return FormatUInt64Decimal(mLocations[index].Id);
}

FString DoDASimulation::GetLocationDisplayName(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mLocations.size())
    {
        return "";
    }
    return mLocations[index].Name;
}

FString DoDASimulation::GetTaskIdText(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mTasks.size())
    {
        return "";
    }
    return FormatUInt64Decimal(mTasks[index].Id);
}

FString DoDASimulation::GetTaskTitle(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mTasks.size())
    {
        return "";
    }
    return mTasks[index].Title;
}

int DoDASimulation::GetTaskStatus(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mTasks.size())
    {
        return 0;
    }
    return static_cast<int>(mTasks[index].Status);
}

int DoDASimulation::GetTaskProgress(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mTasks.size())
    {
        return 0;
    }
    return mTasks[index].Progress;
}

FString DoDASimulation::GetTaskLocationIdText(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= mTasks.size())
    {
        return "";
    }
    return FormatUInt64Decimal(mTasks[index].LocationId);
}
