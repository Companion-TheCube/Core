# SFML to GLFW Migration Plan (Concrete Execution Version)

## Objective

Migrate TheCube Core from SFML-dependent window/input/audio usage to:

1. GLFW for windowing + input
2. Existing OpenGL + GLEW render stack unchanged where possible
3. RtAudio-only playback path (no SFML Audio)
4. Zero SFML linkage in final build

This plan is phased to reduce blast radius and keep the app runnable after every phase.

## Current State (Baseline)

SFML is currently used in three areas:

1. Window + GL context creation and event polling in renderer
2. Event types and input handling across GUI/event manager path
3. Audio playback object path (in progress of moving to RtAudio-only)

Build system still fetches and links SFML, so migration requires both code and CMake cleanup.

## Success Criteria

Migration is complete when all are true:

1. No `#include <SFML/...>` or `#include "SFML/..."` in `src/`
2. No `sfml-*` targets linked in CMake
3. App launches with GLFW window and renders normally
4. Input flows (click, drag, keyboard handlers) behave the same as baseline
5. Audio notifications, alarms, wake sound, and TTS playback work through RtAudio path only
6. No regressions in startup/shutdown stability or event loop threading

## Work Breakdown by Phase

## Phase 0 - Stabilize Baseline and Instrumentation

### Goal

Lock behavior so regressions are easy to detect during migration.

### Tasks

1. Capture baseline behavior checklist:
    - App startup and window visibility
    - Menu click and drag behavior
    - Keyboard shortcuts currently wired
    - Notification/alarm/wake audio playback
2. Add migration branch guardrails:
    - Keep feature work minimal during migration window
    - Require smoke-test pass for each migration PR
3. Add targeted runtime logs around:
    - Event ingestion
    - Window lifecycle
    - Audio clip queue/mix path

### Deliverables

1. Baseline smoke-test checklist in repo docs
2. Known-good commit hash tagged for rollback point

### Exit Criteria

1. Team agrees what "equivalent behavior" means for input/render/audio

## Phase 1 - Event Abstraction Layer (SFML Backend Still Active)

### Goal

Remove SFML types from the core event API surface before changing backend.

### Tasks

1. Introduce platform-agnostic event model:
    - `CubeEventType` enum
    - `CubeKey` enum
    - `CubeMouseButton` enum
    - `CubeEvent` struct/union payload (key, mouse move, button, wheel)
2. Update event interfaces to use `CubeEvent` instead of `sf::Event`:
    - `EventHandler`
    - `EventManager`
    - GUI handlers currently casting `void*` to `sf::Event*`
3. Keep SFML event polling in renderer temporarily, but convert `sf::Event -> CubeEvent` at boundary.
4. Remove direct `sf::Mouse` and `sf::Touch` polling from business logic and route through unified input state.

### Deliverables

1. New `CubeEvent` abstraction and translation helpers
2. Event pipeline independent from SFML headers

### Exit Criteria

1. `EventManager` compiles without SFML types in signatures
2. Existing behavior unchanged under SFML backend

### Risks

1. Keycode mapping mismatches when converting enums
2. Pointer/lifetime bugs from current `void*` event payload style

## Phase 2 - Renderer Backend Switch to GLFW

### Goal

Replace SFML window/context/event pump with GLFW while preserving render loop behavior.

### Tasks

1. Add GLFW dependency in CMake and link for core target.
2. Replace renderer window lifecycle:
    - `sf::RenderWindow` -> `GLFWwindow*`
    - Context settings equivalents via GLFW hints
    - Swap buffers and event polling via GLFW
3. Keep existing OpenGL initialization and draw path intact.
4. Implement GLFW callback bridge to queue `CubeEvent` values.
5. Port:
    - Cursor visibility behavior
    - FPS/VSync settings
    - Window close and shutdown behavior

### Deliverables

1. Renderer running on GLFW backend
2. Event queue fed by GLFW callbacks/polling into `CubeEvent`

### Exit Criteria

1. App launches and renders with GLFW
2. No SFML usage remaining in renderer implementation

### Risks

1. Thread-affinity issues for GL context in renderer thread
2. Event ordering differences vs SFML poll loop

## Phase 3 - GUI Input Behavior Parity Pass

### Goal

Ensure all user interaction behavior matches baseline after backend swap.

### Tasks

1. Validate and tune mappings for:
    - Key pressed/released events
    - Mouse down/up/move/wheel
    - Drag edge detection and thresholds
2. Update click area handling and popup widgets to consume `CubeEvent` payloads.
3. Resolve any touch-specific assumptions:
    - If touch is required on target hardware, add platform path
    - If not, provide explicit fallback behavior
4. Remove remaining SFML event references from GUI and message box code.

### Deliverables

1. Input parity report against baseline checklist
2. All GUI/event modules free of SFML types

### Exit Criteria

1. Menu navigation, drag, click actions, and keyboard handlers all pass smoke tests

### Risks

1. Subtle regressions in drag/click race timing
2. Key repeat behavior differences across backends

## Phase 4 - Audio Finalization (RtAudio Sole Playback Path)

### Goal

Complete and validate RtAudio-only playback path; remove any residual SFML audio dependency.

### Tasks

1. Finalize current RtAudio mixer path for:
    - File playback
    - PCM playback
    - Concurrent clip mixing
2. Validate supported formats policy:
    - Confirm `.wav` is sufficient, or expand decoder support if needed
3. Add guardrails for clipping and queue growth under load.
4. Verify notification, alarm, wake sound, and TTS end-to-end.

### Deliverables

1. Stable RtAudio-only playback in production codepath
2. No SFML audio headers or symbols used

### Exit Criteria

1. All known audio playback features work without SFML linked

### Risks

1. Mixer contention under heavy concurrent sounds
2. Format limitations if non-WAV assets appear later

## Phase 5 - Build Cleanup and SFML Removal

### Goal

Remove SFML from build graph and dependency fetch entirely.

### Tasks

1. Remove SFML `FetchContent` block.
2. Remove all `sfml-*` link targets and include path usage.
3. Run full-tree search to verify no SFML includes/references remain.
4. Clean up stale migration comments and dead code.

### Deliverables

1. Clean CMake config with GLFW + RtAudio stack only
2. Verified no SFML compile-time or link-time usage

### Exit Criteria

1. Build config no longer downloads or links SFML

## Phase 6 - Hardening, Regression, and Release

### Goal

De-risk release by validating runtime stability and parity.

### Tasks

1. Run smoke matrix:
    - Startup/shutdown cycles
    - Long-run idle test
    - Interactive menu stress (rapid navigation, drag)
    - Audio overlap stress (multiple queued clips)
2. Validate logs for:
    - RtAudio underflow frequency
    - Event queue anomalies
    - Thread shutdown cleanliness
3. Document architecture changes and maintenance notes.

### Deliverables

1. Migration validation report
2. Updated dev docs on event model and backend responsibilities

### Exit Criteria

1. No critical regressions open
2. Team sign-off for production merge

## Suggested Timeline (Single-Threaded Execution)

1. Phase 0: 0.5 day
2. Phase 1: 2-3 days
3. Phase 2: 2-3 days
4. Phase 3: 1-2 days
5. Phase 4: 1-2 days
6. Phase 5: 0.5-1 day
7. Phase 6: 1-2 days

Expected total: 8-14 working days (matches earlier estimate).

## PR Plan (Recommended)

1. PR 1: Phase 1 abstraction only
2. PR 2: Phase 2 GLFW backend switch
3. PR 3: Phase 3 parity fixes
4. PR 4: Phase 4 audio finalization
5. PR 5: Phase 5 dependency removal
6. PR 6: Phase 6 hardening/docs

Each PR should be independently runnable and include a short smoke-test result section.

## Rollback Strategy

1. Keep each phase in its own PR with clear boundaries.
2. If Phase N regresses behavior, revert only that PR and retain prior validated phases.
3. Maintain a temporary backend toggle branch only if rollback speed becomes critical.

## Immediate Next Actions

1. Mark current RtAudio playback migration status as "in progress" and define acceptance checks.
2. Start Phase 1 by introducing `CubeEvent` and replacing SFML types in event manager interfaces.
3. Open a tracking issue with per-phase checklist and target commits.
