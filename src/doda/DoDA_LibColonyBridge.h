#pragma once

#include "doda/DoDA_Types.h"
#include <vector>
#include <cstdint>

constexpr int64_t DODA_COST_SCALE = 1048576;
constexpr int DODA_MAX_BASE_COST = 10000;

struct DoDACandidateInput
{
    DoDAPersonId PersonId = 0;
    DoDATaskId TaskId = 0;
    int BaseCost = 0;
    int64_t TotalCost = 0;
};

struct DoDAAssignmentOutput
{
    DoDAPersonId PersonId = 0;
    DoDATaskId TaskId = 0;
    int BaseCost = 0;
};

bool DoDA_OptimizeAssignments(const std::vector<DoDACandidateInput>& candidates,
                              std::vector<DoDAAssignmentOutput>& outAssignments);
