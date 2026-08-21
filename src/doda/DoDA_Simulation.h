#pragma once

#include <vector>
#include <cstdint>
#include "doda/DoDA_Types.h"

class FSerializer;

class DoDASimulation
{
public:
    static DoDASimulation& GetInstance();

    void Serialize(FSerializer& arc);
    void ClearForNewCampaign();

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

    // Personnel Snapshot V1 Query API
    int GetPersonnelSnapshotApiVersion() const { return 1; }
    FString GetPersonnelSnapshotRevisionText() const;
    FString GetPersonnelSnapshotStrategicMinutesText() const;
    int GetPersonnelSnapshotCount() const { return static_cast<int>(mPeople.size()); }

    FString GetPersonnelPersonIdText(int index) const;
    FString GetPersonnelDisplayName(int index) const;
    int GetPersonnelStatus(int index) const;
    int GetPersonnelSkill(int index) const;
    int GetPersonnelWorkload(int index) const;

    FString GetPersonnelCurrentAssignmentIdText(int index) const;
    FString GetPersonnelCurrentTaskIdText(int index) const;
    FString GetPersonnelCurrentTaskTitle(int index) const;

    // Location & Task Queries
    int GetLocationCount() const { return static_cast<int>(mLocations.size()); }
    FString GetLocationIdText(int index) const;
    FString GetLocationDisplayName(int index) const;

    FString GetTaskIdText(int index) const;
    FString GetTaskTitle(int index) const;
    int GetTaskStatus(int index) const;
    int GetTaskProgress(int index) const;
    FString GetTaskLocationIdText(int index) const;

private:
    DoDASimulation() = default;

    static FString FormatUInt64Decimal(uint64_t value);
    void BumpPersonnelSnapshotRevision();
    bool HasPersonnelSnapshotV1State() const;
    void ResetPersonnelSnapshotAfterLoadFailure();
    void ClearToDefaultState();
    void InitializeDefaultFixture();

    uint64_t mStrategicMinutes = 0;
    uint64_t mPersonnelSnapshotRevision = 1;
    DoDAPersonId mNextPersonId = 1;
    DoDATaskId mNextTaskId = 1;
    DoDAAssignmentId mNextAssignmentId = 1;
    DoDALocationId mNextLocationId = 1;

    std::vector<DoDAPersonRecord> mPeople;
    std::vector<DoDALocationRecord> mLocations;
    std::vector<DoDATaskRecord> mTasks;
    std::vector<DoDAAssignmentRecord> mAssignments;
};
