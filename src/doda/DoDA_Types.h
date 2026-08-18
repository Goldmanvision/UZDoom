#pragma once

#include <cstdint>
#include "zstring.h"

using DoDAPersonId = uint64_t;
using DoDATaskId = uint64_t;
using DoDAAssignmentId = uint64_t;

enum class DoDAPersonStatus : uint8_t
{
    Available,
    Assigned,
    Unavailable
};

enum class DoDATaskStatus : uint8_t
{
    Pending,
    Assigned,
    Completed,
    Failed
};

struct DoDAPersonRecord
{
    DoDAPersonId Id;
    FString Name;
    int Skill;
    int Workload;
    DoDAPersonStatus Status;
};

struct DoDATaskRecord
{
    DoDATaskId Id;
    FString Title;
    int Priority;
    int RequiredSkill;
    int EstimatedWork;
    DoDATaskStatus Status;
};

struct DoDAAssignmentRecord
{
    DoDAAssignmentId Id;
    DoDAPersonId PersonId;
    DoDATaskId TaskId;
    int Cost;
};
