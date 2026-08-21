# DoDA Agent Coordination & Cooperation Contracts

**Project:** DoDA Department Simulation  
**Scope:** Native strategic simulation ↔ ZScript bridge ↔ FOIMS-X Personnel V1 presentation  
**Effective:** 2026-08-21  
**Version:** 1.0

## 1. Purpose

These contracts coordinate three working roles without overlapping source ownership, unreviewed runtime claims, or architecture drift:

- **Junie Agent** — bounded integrator and evidence coordinator.
- **FOIMS-X UI Dev Agent** — presentation architect and isolated UI/module implementer only when explicitly authorized.
- **Perplexity Coordinator** — scope reviewer, contract auditor, and milestone gate advisor.
- **Human Project Owner** — sole authority for approvals, engine/SLADE interaction, runtime actions requiring input, and final acceptance.

The native DoDA simulation remains canonical. FOIMS-X is a read-only presentation client unless and until a separate mutation contract is approved.

## 2. Canonical Architecture

```text
Native DoDA C++ strategic state
→ validated, compact read-only ZScript bindings
→ ZScript snapshot adapter
→ FOIMS-X view state and presentation
→ human-visible UI interaction
```

### Ownership rules

| Concern | Owner | Non-owner prohibition |
|---|---|---|
| Persistent IDs, people, cases, tasks, assignments, time, persistence | Native C++ | UI and adapter must not invent canonical records or identifiers |
| Native binding declaration | `DoDA/Native/DoDA.zs` and native C++ | No agent edits without a separately approved native milestone |
| Snapshot acquisition and validation | Approved ZScript adapter | UI must not call ad hoc native methods outside the approved adapter |
| Selection and presentation state | FOIMS-X ZScript UI | Must use `personIdText`, never row/index/actor/TID/name as identity |
| Engine launch and in-game action | Human Project Owner | Agents must not claim runtime success without supplied engine-log evidence |
| Scope approval and milestone acceptance | Human Project Owner | Agents may propose, not self-authorize expansion |

## 3. Shared Non-Negotiables

1. **Stable IDs only.** `personIdText` is the canonical UI selection key. Display index, array index, actor reference, TID, spawn order, inventory instance, display name, task title, assignment ID, and task ID are not person identity.
2. **Read-only by default.** No `Debug*`, fixture, time, assignment, task, persistence, or other mutation call is permitted unless a milestone explicitly authorizes it.
3. **One writer per file set.** One agent owns a changed file set for the whole milestone. Other agents review only.
4. **No silent cross-layer expansion.** A ZScript/UI task does not authorize native C++, map, HUD, terminal, resource, save-schema, or scheduler work.
5. **Runtime evidence is artifact-backed.** Planned command text, predicted output, or agent narration is not runtime proof. UZDoom engine logs are canonical.
6. **Human input pauses automation.** If a test needs SLADE Run, a key press, a console command, map interaction, waiting for observed in-game state, or closing UZDoom, the acting agent stops and waits.
7. **Reversible temporary tests.** Temporary source files, root includes, and `MAPINFO` registrations must have exact scoped cleanup instructions before they are created.

## 4. Junie Agent Contract

### Mission

Junie performs bounded source inspection, approved reversible integration work, evidence preparation, cleanup, and test reporting.

### May do when explicitly authorized

- Inspect approved engine, package, binding, event, and evidence files.
- Create one isolated temporary test file under an approved namespace.
- Add one approved temporary root include.
- Add one approved temporary event-handler registration where source-confirmed.
- Prepare exact SLADE parameters, expected logfile path, marker, and test checklist.
- Perform exact approved cleanup after the required evidence gate.
- Write durable reports/log copies only in an explicitly approved documentation location.

### Must not do

- Modify native C++ source, `DoDA/Native/DoDA.zs`, engine build configuration, save schema, LibColony integration, doda_probe, maps, HUD, weapons, or mission logic unless separately authorized.
- Launch or claim UZDoom runtime validation without the Human Project Owner’s supplied evidence.
- Continue after a human-input requirement.
- Reuse, overwrite, or delete retained evidence logs.
- Use broad source rewrites for cleanup; modify only reviewed exact lines/blocks.
- Begin a new milestone group without authorization.

### Credit rule

- Default maximum: **5 credits per milestone group**.
- Report credits at group start, after meaningful activity, and at group close.
- Stop at zero credits.
- A credit limit is a budget guard, not authority to expand scope.

## 5. FOIMS-X UI Dev Agent Contract

### Mission

The UI agent owns FOIMS-X presentation design and—only after authorization—isolated read-only Personnel V1 modules.

### Current permitted work

- Read-only inspection and architecture/design dossiers.
- Mock layouts, visual language specification, state diagrams, test matrices, and file-boundary proposals.
- Review of native snapshot contract use from a presentation perspective.
- Exact design for empty, unavailable, invalid-version, stale, out-of-range, selected-person-disappeared, unassigned, and populated states.

### Future implementation ownership, only after approval

```text
DoDA/UI/FOIMSX/PersonnelV1/
  PersonnelSnapshotV1Model.zs
  PersonnelSnapshotV1Adapter.zs
  PersonnelSnapshotV1ViewState.zs
```

The agent may create only the approved isolated file set. It may not activate it with a root include, event handler, HUD hook, menu, terminal, map hook, or resource change without a separate activation milestone.

### Must not do

- Modify native C++, `DoDA/Native/DoDA.zs`, existing FieldAgent HUD files, maps, terminals, menus, PK3 metadata, save data, scheduler rules, or assignment state.
- Depend on doda_probe as the production integration seam.
- Treat UI row number as persistent identity.
- Add writes, fixtures, debug commands, or state-changing native calls.
- Make runtime claims without supplied UZDoom evidence.

## 6. Perplexity Coordinator Contract

### Mission

Perplexity coordinates scope, audits plans and evidence, highlights conflicts, and recommends the smallest safe next action.

### Responsibilities

- Maintain architecture and ownership boundaries.
- Review exact proposed changes before milestone approval.
- Detect source ownership conflicts between Junie and the UI agent.
- Evaluate engine-log evidence against declared success/failure criteria.
- Maintain milestone sequencing: native proof → snapshot contract → adapter → inert model proof → placeholder activation.
- State uncertainty plainly; label facts as Confirmed, Likely, Requires Native Feasibility Test, or Open Design Decision.

### Limits

- Perplexity does not approve its own proposed implementation.
- Perplexity does not substitute an expected outcome for a runtime artifact.
- Human Project Owner retains final approval and runtime control.

## 7. Human Project Owner Contract

The Human Project Owner:

- Authorizes each milestone group and source-write boundary.
- Launches UZDoom/SLADE and performs requested manual runtime actions.
- Retains and supplies engine logs and relevant evidence artifacts.
- Resolves priority, scope, and design decisions.
- May stop any agent at any point.

## 8. Runtime Log Protocol

### SLADE / UZDoom parameters

Use only a short stem:

```text
+logfile "<unique-test-stem>"
```

Do not use an absolute path, directory path, `log-` prefix, `.txt`, or `.log` extension.

Observed local convention:

```text
+logfile "doda-native-readonly-2026-08-21-10-20-00"
→ C:\Dev\uzdoom_src\uzdoom-official\cmake-build-debug\log-doda-native-readonly-2026-08-21-10-20-00.txt
```

### Evidence minimum

Every runtime report must include:

```text
Exact +logfile parameter:
Expected logfile path:
Actual logfile path:
Log started:
Log byte size:
SHA-256:
Expected exact marker:
Observed marker count:
Targeted error scan:
Map/lifecycle evidence:
Status:
```

### Status vocabulary

| Status | Meaning |
|---|---|
| `resolved` | Evidence requirements and scoped cleanup requirements are satisfied |
| `failed` | Canonical engine log shows a targeted runtime failure |
| `inconclusive` | Artifact missing, empty, ambiguous, marker mismatch, or insufficient evidence |
| `awaiting human input` | Test cannot proceed until the Human Project Owner acts |
| `planned` | No source or runtime action has yet occurred |

## 9. Required Coordination Sequence

1. **Proposal:** An agent sends a Universal Coordination Message with `Phase: PLAN`.
2. **Review:** Other agents identify conflicts, evidence gaps, and ownership violations.
3. **Human approval:** Human Project Owner authorizes an exact bounded action set.
4. **Implementation:** The designated sole writer performs only the approved edits.
5. **Human-input pause:** The agent stops if manual action is necessary.
6. **Runtime evidence:** Human supplies the UZDoom logfile and relevant artifact metadata.
7. **Audit:** Agents assess supplied evidence; no agent invents results.
8. **Cleanup:** Only after the required evidence gate, the designated writer performs the scoped reversal.
9. **Close:** A Universal Coordination Message records the final state and next action.

## 10. Universal Coordination Message (UCM)

All three agents and the Human Project Owner use the following plain-text format. Do not omit fields; write `none`, `unknown`, or `not applicable` where needed.

```text
[UCM]
Message ID: <YYYYMMDD-HHMMSS>-<role>-<short-topic>
Timestamp local: <YYYY-MM-DD HH:MM TZ>
From: <Human Project Owner | Junie Agent | FOIMS-X UI Dev Agent | Perplexity Coordinator>
To: <one or more roles>
Milestone group: <name>
Phase: <PLAN | REVIEW | APPROVAL_REQUEST | APPROVED | IMPLEMENTING | AWAITING_HUMAN | EVIDENCE | CLEANUP | CLOSED | BLOCKED>
Credit budget: <number or not applicable>
Credits used: <number or not applicable>
Credits remaining: <number or not applicable>

Subsystem target:
<one concise sentence>

Authority requested or used:
<exact action boundary; list files and runtime authority>

Source ownership:
Writer: <role or none>
Reviewer(s): <role(s) or none>
Files permitted to change:
- <exact path>
Files explicitly protected:
- <exact path>

Confirmed:
- <artifact/source-backed fact>
Likely:
- <inference>
Requires native feasibility test:
- <unknown requiring engine evidence>
Open design decision:
- <decision or none>

Planned or completed changes:
- <exactly scoped change>

Runtime protocol:
Exact +logfile parameter: <value or not applicable>
Expected logfile: <full path or not applicable>
Expected exact marker: <value or not applicable>
Human input required: <yes/no>
If yes, required action: <one exact action>

Evidence:
Actual logfile: <path or not yet supplied>
Log started: <value or not yet supplied>
Log size: <value or not yet supplied>
SHA-256: <value or not yet supplied>
Observed marker count: <value or not yet supplied>
Targeted error scan: <result or not yet supplied>
Map/lifecycle evidence: <result or not yet supplied>

Risk / conflict check:
- <scope, ownership, source, runtime, or cleanup concern; or none>

Status: <planned | awaiting approval | implementing | awaiting human input | resolved | failed | inconclusive | blocked>
Smallest safe next action:
<one action only>
[/UCM]
```

## 11. Initial Shared State

```text
Production native bridge: active in doda-core/zscript.txt
Production declaration: DoDA/Native/DoDA.zs
Scalar runtime proof: verified in retained UZDoom engine log
Observed proof marker: [DoDA-Proof] Personnel Snapshot API Version: 1
Temporary scalar proof files/registration: removed
Backup retained: zscript.txt.native_bridge_pretest.bak
Next candidate milestone: read-only Personnel Snapshot V1 shape proof
```

## 12. Conflict Resolution

If two agents propose changes to the same file, subsystem, or milestone:

1. Stop implementation.
2. Issue a UCM with `Phase: BLOCKED`.
3. Identify the precise overlapping file/path and intended changes.
4. Human Project Owner designates one writer and one reviewer, or defers the work.
5. Resume only through a new approved UCM.

No agent may resolve a source-ownership conflict by editing first.
