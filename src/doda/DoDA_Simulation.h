#pragma once

#include <vector>
#include <cstdint>
#include "doda/DoDA_Types.h"

class DoDASimulation
{
public:
    static DoDASimulation& GetInstance();

    bool ResetDebugFixture();
    bool AdvanceStrategicMinutes(int deltaMinutes);
    bool RunDeterministicAssignment();

    FString GetStrategicSummary() const;
    FString GetAssignmentSummary() const;

    uint64_t GetStrategicMinutes() const { return mStrategicMinutes; }
    size_t GetPersonCount() const { return mPeople.size(); }
    size_t GetTaskCount() const { return mTasks.size(); }
    size_t GetAssignmentCount() const { return mAssignments.size(); }
    FString GetAssignmentDebugText(int index) const;

private:
    DoDASimulation() = default;

    uint64_t mStrategicMinutes = 0;
    DoDAPersonId mNextPersonId = 1;
    DoDATaskId mNextTaskId = 1;
    DoDAAssignmentId mNextAssignmentId = 1;

    std::vector<DoDAPersonRecord> mPeople;
    std::vector<DoDATaskRecord> mTasks;
    std::vector<DoDAAssignmentRecord> mAssignments;
};
