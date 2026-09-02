#pragma once

#include <cstdint>
#include "zstring.h"

using DoDAPersonId = uint64_t;
using DoDATaskId = uint64_t;
using DoDAAssignmentId = uint64_t;
using DoDALocationId = uint64_t;

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
    int Fatigue = 0;
};

struct DoDALocationRecord
{
    DoDALocationId Id;
    FString Name;
};

struct DoDATaskRecord
{
    DoDATaskId Id;
    FString Title;
    DoDALocationId LocationId;
    int Priority;
    int RequiredSkill;
    int EstimatedWork;
    DoDATaskStatus Status;
    int Progress;
};

struct DoDAAssignmentRecord
{
    DoDAAssignmentId Id;
    DoDAPersonId PersonId;
    DoDATaskId TaskId;
    int Cost;
};
