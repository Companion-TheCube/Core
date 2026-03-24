Given what is already working in the wake/transcribe/intent path, these are the next highest-value items.

## Priority stack

- Notifications, reminders, alarms, and Pomodoro.
    - Biggest remaining Phase 1/MVP gap after voice flow.
    - Directly tied to event and notification requirements.
    - References: Planning-Docs/project/roadmap.md (lines 15-16), Planning-Docs/specs/software-requirements.md (lines 78-81).

- Presence-aware idle mode plus personality loop.
    - Core product identity work so TheCube feels alive between interactions.
    - References: Planning-Docs/project/roadmap.md (line 17), Planning-Docs/project/roadmap.md (line 56).

- Screen UI/navigation and real settings/system/privacy surfaces.
    - Need discoverable controls for settings, system info, legal/privacy, and day-to-day device operations.
    - References: Planning-Docs/components/core.md (line 68), Planning-Docs/components/core.md (line 12).

- Audio robustness and remote voice pipeline hardening.
    - Focus on streaming reliability, retries, latency, and response playback quality.
    - Architecture note: speech recognition, intent detection, and TTS are remote-server responsibilities; do not plan local ASR/TTS fallback as a core direction.
    - References: Planning-Docs/components/core.md (line 67), Planning-Docs/specs/software-requirements.md (lines 74-75), Planning-Docs/components/server.md (line 63).

- App platform completeness: install/update/lifecycle, permissions, SDK/tooling.
    - Needed to move from first-party demos to a real platform.
    - References: Planning-Docs/components/core.md (line 77), Planning-Docs/specs/software-requirements.md (line 83), Planning-Docs/apps/platform-overview.md (lines 52, 65).

## Recommended execution order

1. Notifications, reminders, alarms, Pomodoro.
    - Fix notification modal bugs and polish user flows.
    - Remove one-shot alarms from DB after firing.
    - Add thorough tests for reminders/alarms, including DST changes, timezone changes, and device restarts.
2. Presence-aware idle/personality loop.
3. UI/settings/system/privacy pages.
4. Audio hardening for remote pipeline behavior (not local ASR/TTS fallback).
5. App lifecycle/permissions/tooling.

## Parallel backend track

Server hardening in parallel: auth/device registration, stable API shape, telemetry/admin cleanup, and replacing prototype paths with production-ready implementations.

- References: Planning-Docs/project/roadmap.md (lines 77-84), Planning-Docs/components/server.md (lines 101-121), Planning-Docs/specs/api-spec.md (line 13).

## Meta item (overdue)

Refresh roadmap dates and tighten MVP definition, since the current Phase 1 target dates are stale.

- Reference: Planning-Docs/project/roadmap.md (line 87).
