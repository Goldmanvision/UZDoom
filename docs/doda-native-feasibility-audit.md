# DoDA Strategic Simulation: Native Feasibility Audit

## 1. Repository & Fork Metadata
- **Origin Remote**: `https://github.com/Goldmanvision/UZDoom.git`
- **Upstream Remote**: `https://github.com/UZDoom/UZDoom.git`
- **Current Branch**: `doda/strategic-core-v0`
- **Current Commit (HEAD)**: `d0fc321233f44afa2a2a6827ad1488a288a9b1dd`
- **Target Engine Base**: UZDoom (C++20 Doom engine fork derived from GZDoom)

## 2. Supported Build Method
- **Build System**: CMake (minimum version 3.16 required, CMP0067 and CMP0091 policies set in `CMakeLists.txt` lines 1–4).
- **Language Standard**: C++20 standard enforced via `set(CMAKE_CXX_STANDARD 20)`, `set(CMAKE_CXX_STANDARD_REQUIRED ON)`, and `set(CMAKE_CXX_EXTENSIONS OFF)` in `CMakeLists.txt` (lines 9–11).
- **Compilers & Generators**:
  - Windows: MSVC `cl.exe` (e.g., MSVC v14.51 / Visual Studio 2022+ BuildTools x64 host/target) using `Ninja` or Visual Studio solution generators.
  - Runtime Library: `MultiThreaded$<$<CONFIG:Debug>:Debug>` (`CMakeLists.txt` line 5).
- **Target Integration**:
  - Main executable target: `uzdoom` defined in `src/CMakeLists.txt`.
  - Header search path / IDE grouping includes `doda/*.h` (`src/CMakeLists.txt` line 587).
  - Source file registration explicitly includes native DoDA translation units (`src/CMakeLists.txt` lines 815–817):
    - `doda/DoDA_LibColonyBridge.cpp`
    - `doda/DoDA_Simulation.cpp`
    - `doda/DoDA_ZBind.cpp`

## 3. Existing Native-to-ZScript Binding Patterns
- **VM Binding Architecture**:
  - Core header: `src/common/scripting/vm/vm.h`.
  - Native function binding macros:
    - `DEFINE_ACTION_FUNCTION(cls, name)` (`src/common/scripting/vm/vm.h` line 960).
    - `DEFINE_ACTION_FUNCTION_NATIVE(cls, name, native)` (`src/common/scripting/vm/vm.h` line 948).
  - Auto-registration mechanism:
    - Struct `AFuncDesc` (`src/common/scripting/vm/vm.h` lines 930–943) derives from `FAutoSegEntry<AFuncDesc>` and registers into linker section / auto-segment `AutoSegs::ActionFunctons`.
    - Auto-segment global table: `AutoSegs::ActionFunctons` (`src/common/objects/autosegs.cpp` line 36, `src/common/objects/autosegs.h` line 126).
  - Class Name Lookup Convention:
    - In `src/common/scripting/core/imports.cpp` (`InitImports()`, lines 203–211), the runtime builds the VM qualified function name as `FStringf("%s.%s", afunc->ClassName + 1, afunc->FuncName)`.
    - Because the first character of `ClassName` is skipped (`afunc->ClassName + 1`), binding to a ZScript class `DoDA` requires declaring the native class name with a leading underscore prefix `_DoDA`.
  - VM parameter extraction and return macros:
    - `PARAM_PROLOGUE`, `PARAM_INT(delta)` (`src/common/scripting/vm/vm.h` lines 1042–1047).
    - `ACTION_RETURN_INT(v)`, `ACTION_RETURN_BOOL(v)`, `ACTION_RETURN_STRING(v)`, `ACTION_RETURN_FLOAT(v)` (`src/common/scripting/vm/vm.h` lines 1015–1017).
- **Current DoDA Binding File (`src/doda/DoDA_ZBind.cpp`)**:
  - Binds native `DoDASimulation` methods to ZScript class `DoDA` (`_DoDA`).
  - Exported symbols:
    - `_DoDA.GetNativeApiVersion` (line 5) -> returns `DODA_NATIVE_API_VERSION` (`1` from `src/doda/DoDA_Version.h`).
    - `_DoDA.DebugResetFixture` (line 11) -> calls `DoDASimulation::ResetDebugFixture()`.
    - `_DoDA.DebugAdvanceMinutes` (line 18) -> calls `DoDASimulation::AdvanceStrategicMinutes(delta)`.
    - `_DoDA.DebugRunAssignment` (line 26) -> calls `DoDASimulation::RunDeterministicAssignment()`.
    - `_DoDA.GetStrategicMinutes` (line 33) -> calls `DoDASimulation::GetStrategicMinutes()`.
    - `_DoDA.GetPersonCount` (line 40), `_DoDA.GetTaskCount` (line 47), `_DoDA.GetAssignmentCount` (line 54).
    - `_DoDA.GetAssignmentDebugText` (line 61).
    - `_DoDA.GetPersonnelSnapshotApiVersion` (line 69), `_DoDA.GetPersonnelSnapshotRevisionText` (line 75), `_DoDA.GetPersonnelSnapshotStrategicMinutesText` (line 82), `_DoDA.GetPersonnelSnapshotCount` (line 89).
    - Personnel getters: `_DoDA.GetPersonnelPersonIdText` (line 96), `_DoDA.GetPersonnelDisplayName` (line 104), `_DoDA.GetPersonnelStatus` (line 112), `_DoDA.GetPersonnelSkill` (line 120), `_DoDA.GetPersonnelWorkload` (line 128), `_DoDA.GetPersonnelCurrentAssignmentIdText` (line 136), `_DoDA.GetPersonnelCurrentTaskIdText` (line 144), `_DoDA.GetPersonnelCurrentTaskTitle` (line 152).
    - Location & Task getters: `_DoDA.GetLocationCount` (line 160), `_DoDA.GetLocationIdText` (line 167), `_DoDA.GetLocationDisplayName` (line 175), `_DoDA.GetTaskIdText` (line 183), `_DoDA.GetTaskTitle` (line 191), `_DoDA.GetTaskStatus` (line 199), `_DoDA.GetTaskProgress` (line 207), `_DoDA.GetTaskLocationIdText` (line 215).

## 4. Save/Load and Serialization Extension Points
- **Engine Serialization Architecture**:
  - Uses `FSerializer` (`src/common/scripting/core/serializer.h`, `serializer_doom.h`, `serializer_doom.cpp`) storing structured JSON payload (`globals.json`) inside the zip save container.
- **Hook Locations**:
  - **Save Game Hook**: `src/g_game.cpp`, `G_SaveGame()` (line 2510):
    - `DoDASimulation::GetInstance().Serialize(savegameglobals);`
  - **Load Game Hook**: `src/g_game.cpp`, `G_ReadSavegame()` / `G_DoLoadGame()` (line 2215):
    - `DoDASimulation::GetInstance().Serialize(arc);`
  - **New Game / Campaign Reset Hooks**:
    - `src/d_main.cpp`, `D_DoomMain_Internal()` (line 3910):
      - `DoDASimulation::GetInstance().ClearForNewCampaign();`
    - `src/g_level.cpp`, `G_DoNewGame()` (line 506):
      - `DoDASimulation::GetInstance().ClearForNewCampaign();`
- **DoDA Native Serializer Implementation**:
  - Implemented in `src/doda/DoDA_Simulation.cpp` (`DoDASimulation::Serialize(FSerializer& arc)`, lines 79–238).
  - Schema constants defined in `src/doda/DoDA_SaveSchema.h`:
    - `DODA_SAVE_SCHEMA_VERSION = 3` (line 5)
    - Structural bounds: `DODA_MAX_PEOPLE = 1024`, `DODA_MAX_TASKS = 1024`, `DODA_MAX_ASSIGNMENTS = 1024`, `DODA_MAX_PERSON_NAME_LENGTH = 256`, `DODA_MAX_TASK_TITLE_LENGTH = 256` (lines 7–11).
  - Serializes strategic minutes, snapshot revisions, entity ID sequences, and record lists (`mPeople`, `mLocations`, `mTasks`, `mAssignments`) with schema validation and error recovery via `ResetPersonnelSnapshotAfterLoadFailure()`.

## 5. Architectural Assessment

### 5.1 Isolated Native DoDA Code Under `src/doda/`
- **Feasibility**: Fully verified and actively operational in the repository.
- **Evidence**:
  - `src/doda/DoDA_Types.h`: Defines domain primitives and structs (`DoDAPersonRecord`, `DoDALocationRecord`, `DoDATaskRecord`, `DoDAAssignmentRecord`).
  - `src/doda/DoDA_SaveSchema.h`: Defines schema version constants and sanity limits.
  - `src/doda/DoDA_Version.h`: Defines `DODA_NATIVE_API_VERSION`.
  - `src/doda/DoDA_Simulation.h` / `src/doda/DoDA_Simulation.cpp`: Encapsulates canonical simulation state behind `DoDASimulation::GetInstance()`.
  - `src/doda/DoDA_LibColonyBridge.h` / `src/doda/DoDA_LibColonyBridge.cpp`: Provides clean solver boundary.
  - `src/CMakeLists.txt`: Sources are compiled directly as part of the engine binary.

### 5.2 Viability of Narrow Binding File (`DoDA_ZBind.cpp`)
- **Feasibility**: Fully verified and actively operational.
- **Evidence**:
  - `src/doda/DoDA_ZBind.cpp` acts as the single, thin translation layer between ZScript VM and `DoDASimulation`.
  - Shields simulation domain logic from VM op macros and parameter unpacking details.
  - Exposes read-only scalar/string query APIs that prevent garbage collection pressure in ZScript UI renderers (FOIMSX/HUD).

### 5.3 Standalone DLL or Plugin Approach
- **Feasibility**: Not viable; incompatible with engine architecture.
- **Evidence**:
  - UZDoom/GZDoom is built on a monolithic C++ architecture where all gameplay systems, thinkers, serializers, and ZScript VM bindings are statically linked into `uzdoom.exe`.
  - Dynamic loading via `FModule` (`src/common/utility/i_module.h`, `src/common/utility/i_module.cpp`) is reserved exclusively for external third-party/OS libraries (e.g., OpenAL, libcurl, OpenGL, Vulkan, XInput).
  - VM native function registration relies on linker section tables (`AutoSegs::ActionFunctons`), and serialization relies on `FSerializer` internal symbols, which cannot cross dynamic library boundaries without significant architectural divergence and ABI risk.

### 5.4 Pinned LibColony Single Native Translation Unit Compilation
- **Feasibility**: Fully verified and actively operational.
- **Evidence**:
  - Upstream pinned commit: `7406bd9fbb00e53273ea9da84eeba5aa26af382f` (preserved in `src/doda/thirdparty/libcolony/colony_upstream_7406bd9f.h`).
  - MSVC-compatible implementation: `src/doda/thirdparty/libcolony/colony_msvc_compat.h` adapts temporary VLA structures to standard C++ containers (`std::vector`) to maintain MSVC C++20 compliance.
  - Compiles as a single translation unit inside `src/doda/DoDA_LibColonyBridge.cpp`, invoked via `DoDA_OptimizeAssignments()` without external dependencies or multi-target overhead.

## 6. Categorized Findings

### Confirmed
1. **Repository Identity & Commit**: Git remote `origin` (`https://github.com/Goldmanvision/UZDoom.git`), upstream (`https://github.com/UZDoom/UZDoom.git`), branch `doda/strategic-core-v0`, HEAD commit `d0fc321233f44afa2a2a6827ad1488a288a9b1dd`.
2. **Monolithic C++20 Build System**: CMake with MSVC `cl.exe` (x64) and Ninja/VS compiles all DoDA native units directly into `uzdoom.exe`.
3. **Isolated Domain Module**: Native DoDA code cleanly resides under `src/doda/` without architectural leaks into engine core headers.
4. **ZScript VM Binding via `DoDA_ZBind.cpp`**: Function exports use `DEFINE_ACTION_FUNCTION(_DoDA, ...)` and register into `AutoSegs::ActionFunctons` under the `DoDA` ZScript class.
5. **Save/Load & Reset Integration**: Persistence hooks are established in `src/g_game.cpp` (lines 2215, 2510) via `FSerializer` into `globals.json`, and campaign resets in `src/d_main.cpp` (line 3910) and `src/g_level.cpp` (line 506).
6. **LibColony Single TU Compilation**: Pinned LibColony code is fully integrated and compiled in `src/doda/DoDA_LibColonyBridge.cpp`.
7. **Absence of DLL Plugin Architecture**: Engine does not have a plugin framework for gameplay code; monolithic compilation is the only supported path.

### Likely
1. **Low VM GC Pressure**: Snapshot query patterns returning native scalar/FString copies enable UI polling without allocation churn in the ZScript garbage collector.
2. **Domain Extensibility**: Additional simulation domains (fatigue modeling, base logistics, department rosters) can be added cleanly within `src/doda/` without requiring engine modifications beyond updating `DoDASimulation::Serialize()`.

### Requires Native Feasibility Test
1. **Upper-Bound Solver Latency**: Performance of `DoDA_OptimizeAssignments` when candidate matrices approach maximum limits (`DODA_MAX_PEOPLE = 1024`, `DODA_MAX_TASKS = 1024`) during an active game tic (< 1 ms budget).
2. **Background Threading Feasibility**: If simulation computation expands, whether background worker thread execution for strategic ticks can be safely orchestrated without introducing race conditions with main thread save/load or ZScript reads.

### Open Design Decision
1. **ZScript Mutation Granularity**: Whether ZScript UI will send fine-grained mutation commands (e.g. manual person assignment, task priority overrides) via `DoDA_ZBind.cpp` or if native simulation steps remain strictly autonomous.
2. **Snapshot Invalidation Mechanism**: Whether UI renderers should continue polling `GetPersonnelSnapshotRevisionText()` per frame or adopt an engine-level event dispatch hook to notify ZScript of state changes.

## 7. Smallest Safe Next Action & Focused Test
- **Smallest Safe Next Action**: Maintain the isolated native pattern in `src/doda/` and ensure all future simulation state additions strictly register in `DoDA_SaveSchema.h` and `DoDASimulation::Serialize()`.
- **One Focused Test**: Run a focused automated check or headless engine execution verifying that `DoDA.GetNativeApiVersion()` returns `1`, `DoDA.GetPersonnelSnapshotApiVersion()` returns `1`, and a save/load cycle preserves all `DoDAPersonRecord` fields (including fatigue and assignment state) identically.
