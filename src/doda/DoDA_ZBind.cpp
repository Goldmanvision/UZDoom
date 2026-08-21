#include "vm.h"
#include "doda/DoDA_Version.h"
#include "doda/DoDA_Simulation.h"

DEFINE_ACTION_FUNCTION(_DoDA, GetNativeApiVersion)
{
    PARAM_PROLOGUE;
    ACTION_RETURN_INT(DODA_NATIVE_API_VERSION);
}

DEFINE_ACTION_FUNCTION(_DoDA, DebugResetFixture)
{
    PARAM_PROLOGUE;
    bool ok = DoDASimulation::GetInstance().ResetDebugFixture();
    ACTION_RETURN_BOOL(ok);
}

DEFINE_ACTION_FUNCTION(_DoDA, DebugAdvanceMinutes)
{
    PARAM_PROLOGUE;
    PARAM_INT(delta);
    bool ok = DoDASimulation::GetInstance().AdvanceStrategicMinutes(delta);
    ACTION_RETURN_BOOL(ok);
}

DEFINE_ACTION_FUNCTION(_DoDA, DebugRunAssignment)
{
    PARAM_PROLOGUE;
    bool ok = DoDASimulation::GetInstance().RunDeterministicAssignment();
    ACTION_RETURN_BOOL(ok);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetStrategicMinutes)
{
    PARAM_PROLOGUE;
    int minutes = static_cast<int>(DoDASimulation::GetInstance().GetStrategicMinutes());
    ACTION_RETURN_INT(minutes);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonCount)
{
    PARAM_PROLOGUE;
    int count = static_cast<int>(DoDASimulation::GetInstance().GetPersonCount());
    ACTION_RETURN_INT(count);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetTaskCount)
{
    PARAM_PROLOGUE;
    int count = static_cast<int>(DoDASimulation::GetInstance().GetTaskCount());
    ACTION_RETURN_INT(count);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetAssignmentCount)
{
    PARAM_PROLOGUE;
    int count = static_cast<int>(DoDASimulation::GetInstance().GetAssignmentCount());
    ACTION_RETURN_INT(count);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetAssignmentDebugText)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetAssignmentDebugText(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelSnapshotApiVersion)
{
    PARAM_PROLOGUE;
    ACTION_RETURN_INT(DoDASimulation::GetInstance().GetPersonnelSnapshotApiVersion());
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelSnapshotRevisionText)
{
    PARAM_PROLOGUE;
    FString text = DoDASimulation::GetInstance().GetPersonnelSnapshotRevisionText();
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelSnapshotStrategicMinutesText)
{
    PARAM_PROLOGUE;
    FString text = DoDASimulation::GetInstance().GetPersonnelSnapshotStrategicMinutesText();
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelSnapshotCount)
{
    PARAM_PROLOGUE;
    int count = DoDASimulation::GetInstance().GetPersonnelSnapshotCount();
    ACTION_RETURN_INT(count);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelPersonIdText)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetPersonnelPersonIdText(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelDisplayName)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetPersonnelDisplayName(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelStatus)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    int status = DoDASimulation::GetInstance().GetPersonnelStatus(index);
    ACTION_RETURN_INT(status);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelSkill)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    int skill = DoDASimulation::GetInstance().GetPersonnelSkill(index);
    ACTION_RETURN_INT(skill);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelWorkload)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    int workload = DoDASimulation::GetInstance().GetPersonnelWorkload(index);
    ACTION_RETURN_INT(workload);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelCurrentAssignmentIdText)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetPersonnelCurrentAssignmentIdText(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelCurrentTaskIdText)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetPersonnelCurrentTaskIdText(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetPersonnelCurrentTaskTitle)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetPersonnelCurrentTaskTitle(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetLocationCount)
{
    PARAM_PROLOGUE;
    int count = DoDASimulation::GetInstance().GetLocationCount();
    ACTION_RETURN_INT(count);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetLocationIdText)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetLocationIdText(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetLocationDisplayName)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetLocationDisplayName(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetTaskIdText)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetTaskIdText(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetTaskTitle)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetTaskTitle(index);
    ACTION_RETURN_STRING(text);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetTaskStatus)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    int status = DoDASimulation::GetInstance().GetTaskStatus(index);
    ACTION_RETURN_INT(status);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetTaskProgress)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    int progress = DoDASimulation::GetInstance().GetTaskProgress(index);
    ACTION_RETURN_INT(progress);
}

DEFINE_ACTION_FUNCTION(_DoDA, GetTaskLocationIdText)
{
    PARAM_PROLOGUE;
    PARAM_INT(index);
    FString text = DoDASimulation::GetInstance().GetTaskLocationIdText(index);
    ACTION_RETURN_STRING(text);
}
