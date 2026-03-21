Given what’s already working now in the voice path, the next big items look like this:

- Notifications, reminders, alarms, and Pomodoro. This is the biggest obvious MVP gap after wake/transcribe/intent. It’s explicitly in Phase 1 and ties directly to the event/notification requirements. See roadmap.md (line 14) and software-requirements.md (line 78).

- Presence-aware idle mode plus personality behaviors. Right now TheCube can react; the next step is making it feel alive between interactions. That is core Phase 1 product identity work, not just polish. See roadmap.md (line 11) and roadmap.md (line 17).

- Screen UI/navigation and real settings surfaces. The voice flow exists, but users still need discoverable settings, system info, privacy/legal pages, and better on-device control. That gap is called out directly in the Core notes. See roadmap.md (line 18) and core.md (line 66).

- Audio robustness and local speech fallback/polish. The docs still flag audio robustness and local TTS/STT as unfinished, and the requirements explicitly prefer local processing where feasible. After the recent voice work, this is the right hardening pass. See core.md (line 67) and software-requirements.md (line 74).

- App platform completeness: install/update/lifecycle, permissions, and SDK/tooling. If you want to move from first-party demo flows to a real platform, this is the next large systems item. See core.md (line 66), software-requirements.md (line 83), and roadmap.md (line 72).

## Parallel backend track:

Server hardening: auth/device registration, stable API shape, telemetry/admin cleanup, and replacing prototype paths with production-ready implementations. See roadmap.md (line 75) and server.md (line 101).
If you want the practical recommendation, I’d tackle them in this order:

1. notifications/reminders/alarms/Pomodoro
2. presence-aware idle/personality loop
3. UI/settings/system pages
4. audio hardening/local fallback
5. app lifecycle/permissions

One meta-item is also overdue: the roadmap dates and MVP definition need a refresh, because the Phase 1 target in the doc is already stale. See roadmap.md (line 87).