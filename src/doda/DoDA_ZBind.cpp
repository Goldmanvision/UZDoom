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
