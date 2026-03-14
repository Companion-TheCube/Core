# TheCube App Launch and Sandboxing Plan

## 1. Purpose

This document defines a practical architecture for application lifecycle management on TheCube using three primary elements:

1. **TheCube Core** as the orchestration and policy authority.
2. **systemd** as the service supervisor and lifecycle engine.
3. **A custom app launcher** as a narrow, security-focused executable that applies app-specific restrictions, especially **Landlock**, before handing control to the target app.

The goal is to create an app model that is:

* modular
* restartable
* observable
* secure by default
* suitable for local apps running on embedded Linux
* compatible with future developer-facing app packaging and SDK workflows

This plan assumes that TheCube Core does **not** directly `exec()` arbitrary third-party apps. Instead, TheCube Core requests lifecycle actions, systemd executes the launcher under a controlled service definition, and the launcher performs the final app-specific sandboxing and process handoff.

---

## 2. Background and Intent

TheCube is intended to support a growing ecosystem of applications on top of a core platform. Existing project materials already establish several important design expectations:

* TheCube-Core acts as the central controller and API provider for the device, including hardware management, AI processing, and application-facing infrastructure. fileciteturn0file2
* The software requirements emphasize application management, permissions, structured APIs, maintainability, and security/privacy controls. fileciteturn0file7
* The overall project direction emphasizes modularity, openness to third-party development, and a future app ecosystem. fileciteturn0file6
* Public-facing positioning also describes a modular, app-driven, privacy-focused platform intended for makers, tinkerers, and developers. fileciteturn0file8

Given those goals, app execution cannot be treated as a simple "run a binary" feature. The platform needs a deliberate lifecycle and security model.

This document lays out that model.

---

## 3. Design Goals

## 3.1 Primary Goals

### A. Strong separation of responsibilities

* **TheCube Core** decides *what* should run and *under what policy*.
* **systemd** decides *how the process is supervised*.
* **The launcher** decides *how the app is finally sandboxed and executed*.

### B. Secure-by-default app execution

Each app should receive the minimum filesystem and kernel-surface access required for its purpose. The default position is deny first, then selectively allow only what the app needs.

### C. Reliable lifecycle control

The platform should support:

* start
* stop
* restart
* enable/disable
* crash recovery
* status inspection
* logging
* boot-time activation where appropriate

### D. Per-app policy variability

Different apps will need different permissions. A game, a calendar notifier, a local music controller, and a voice assistant plugin should not all run with the same access profile.

### E. Clear future developer workflow

Eventually, app packaging should make it straightforward for a developer to declare:

* runtime class and launch target
* runtime arguments
* working directory
* requested capabilities
* filesystem access needs
* network requirements
* event subscriptions
* desired restart behavior

### F. Defense in depth

Landlock is useful, but it should be one layer, not the only layer. systemd sandboxing, Linux users/groups, service isolation, API mediation, and hardware brokering should all work together.

---

## 4. High-Level Architecture

## 4.1 Main Components

### TheCube Core

TheCube Core is the authoritative service manager at the application platform level. It should:

* maintain the installed app registry
* track app metadata and declared permissions
* decide whether an app is allowed to launch
* request start/stop/restart actions
* expose app status to UI and companion tools
* collect health and telemetry signals
* broker privileged resources through controlled APIs

### systemd

systemd is the runtime supervisor and should:

* launch app services
* monitor process lifetime
* restart on failure when configured
* capture stdout/stderr and journal logs
* enforce service-level resource controls
* provide a stable control surface for lifecycle operations

### The App Launcher

The launcher is a small, auditable executable called by systemd. It should:

* read the intended app identity and execution spec
* validate runtime inputs
* set up the execution environment
* optionally drop privileges further
* apply Landlock restrictions
* optionally apply additional pre-exec hygiene
* `execve()` the target app so the app becomes the main service process

### The App

The app itself should be treated as untrusted or partially trusted code relative to the core platform, even if it is first-party. It should access device capabilities through platform APIs whenever possible rather than raw system access.

---

## 4.2 Control Flow Summary

1. A user action, automation, or system event causes TheCube Core to decide that an app should start.
2. TheCube Core resolves the app manifest and policy.
3. TheCube Core asks systemd to start the app service instance.
4. systemd launches the custom app launcher with the app identity or manifest path.
5. The launcher loads the resolved app policy.
6. The launcher prepares the execution context.
7. The launcher applies Landlock rules and any additional restrictions.
8. The launcher replaces itself with the target app.
9. systemd supervises the running process.
10. TheCube Core observes status through systemd and platform-side health signals.

This gives a clean chain of authority:

**Core chooses -> systemd supervises -> launcher constrains -> app runs**

---

## 5. Why systemd Should Be in the Middle

Using systemd as the lifecycle layer is a strong fit for TheCube.

## 5.1 Benefits

### Reliable supervision

Apps can be restarted if they crash. Policies like `Restart=on-failure` can be tuned per app class.

### Structured logging

Using journald gives a central place to inspect logs for debugging, support, and developer workflows.

### Resource control

CPU, memory, task counts, IO constraints, and startup ordering can all be managed through systemd service properties.

### Clear operational semantics

systemd already has a mature model for:

* one-shot services
* long-running services
* restartable daemons
* timers
* dependencies
* target-based startup

### Better than Core directly spawning apps

If TheCube Core directly spawns and owns app processes, it inherits complexity around:

* child process cleanup
* restart policy
* output capture
* state tracking
* zombie handling
* boot/start ordering
* failure recovery

That is exactly the category of problem systemd already solves well.

---

## 6. Why a Custom Launcher Is Still Needed

If systemd is already launching the app, it is reasonable to ask why not simply point `ExecStart=` to the app directly.

The main answer is that TheCube needs **per-app dynamic policy application**, especially for Landlock and future policy logic.

## 6.1 Reasons for a launcher layer

### A. Landlock rules may vary by app

Landlock policies need to be derived from app-specific requirements. A single static service file is not expressive enough for a long-term app ecosystem.

### B. Manifest-driven execution

The launcher can read a manifest or precompiled policy blob and build the exact runtime view for that app.

### C. Consistent pre-exec validation

The launcher can reject misconfigured apps before execution.

### D. Better auditability

You can centralize the sensitive logic that decides:

* what executable is allowed
* what directories are granted
* what environment variables are passed
* whether the app may start at all

### E. Future compatibility

The launcher can later grow to support:

* versioned sandbox policies
* feature flags
* developer mode relaxations
* app package signatures
* compatibility shims
* performance tracing hooks

In other words, systemd gives you supervision, but the launcher gives you a policy enforcement choke point.

---

## 7. Recommended App Model

## 7.1 Runtime Ownership Model

The lifecycle chain should remain:

**TheCube Core -> systemd -> launcher -> app**

What needs to broaden is the definition of "app." The launcher should be able to execute a resolved target that may be:

* a native executable
* a script plus a platform-managed runtime
* a self-contained packaged app built from a runtime language
* a web bundle with an optional backend service
* a developer-mode raw source app
* a container later if TheCube eventually chooses to support that class

The main platform rule should be:

**Apps declare a runtime class, but TheCube owns the runtime strategy.**

That means an app can declare that it is native, Python, Node, or a web bundle, but the platform still controls:

* which runtime families are supported
* which versions are allowed on-device
* where runtimes live on disk
* how dependencies are prepared at install time
* what final `argv` and launch policy are generated

This is the difference between a real platform and "anything that happens to run on Linux."

## 7.2 Supported App Classes

The most realistic app classes for TheCube on Raspberry Pi 5 are:

### Class 1: Native compiled app

Examples:

* C
* C++
* Rust
* Go

These are the cleanest operational fit. They usually have the best startup time, the lowest idle memory, and the simplest sandbox story.

### Class 2: Runtime app with platform-managed runtime

Examples:

* Python
* Node.js
* potentially Java, .NET, Bun, or Deno later

These are reasonable to support, but only if the platform controls runtime versions and installation behavior.

### Class 3: Self-contained packaged app

Examples:

* PyInstaller or Nuitka output
* packaged Node application with an embedded runtime strategy
* any frozen binary produced from a higher-level runtime language

These are often a better production fit than raw source apps because launch-time behavior becomes simpler and more deterministic.

### Class 4: Web bundle app

Examples:

* static HTML/CSS/JS bundle rendered in a trusted shell or WebView
* a frontend bundle paired with a local backend service

This is likely to become an important category because many apps will be mostly UI plus API access rather than direct system integration.

### Class 5: Developer-mode raw source app

Examples:

* loose Python source tree
* loose Node source tree

This should be allowed for experimentation, but it should not become the default production contract.

### Containers

Containers may be useful later, but they should not be the primary early app model. They add significant operational complexity and duplicated storage cost on an embedded platform.

A practical initial runtime support matrix would look like:

* Tier 1: `native`, `python` on a supported major/minor line, `node` on a supported major line, and `web-bundle`
* Tier 2: self-contained packaged variants and additional runtime families
* Tier 3: developer-mode or sideload-only custom runtimes

## 7.3 App Package Structure

A practical long-term model is for each app to have an install directory such as:

`/opt/thecube/apps/<app-id>/`

Example structure:

* `manifest.json` or `manifest.toml`
* `assets/`
* `data/` optional immutable packaged content
* `bin/` for native or packaged executable artifacts
* `lib/` optional private libraries
* `.venv/` for a Python app-local virtual environment when that distribution class is used
* `dist/` for built Node or bundled JavaScript output
* `node_modules/` for app-local Node dependencies when needed
* `web/` for frontend bundle assets when the app ships a UI bundle
* `app.service.template` optional generated metadata source, not directly used at runtime

Representative layouts:

Python app:

* `main.py`
* `.venv/`

Node app:

* `dist/index.js`
* `node_modules/`

Native app:

* `bin/app`

Web bundle app:

* `web/index.html`
* optional backend service content alongside it

Per-app writable data should live elsewhere, for example:

* `/var/lib/thecube/apps/<app-id>/` for persistent data
* `/var/cache/thecube/apps/<app-id>/` for cache
* `/run/thecube/apps/<app-id>/` for runtime state and sockets

This separation is important because Landlock and service policy can then cleanly distinguish:

* immutable app code
* persistent app data
* ephemeral runtime files

Platform-managed shared runtimes, when used, should live in fixed locations such as:

* `/opt/thecube/runtimes/python/3.11/bin/python3`
* `/opt/thecube/runtimes/node/20/bin/node`

Those locations should be owned by the platform, not by individual apps.

---

## 7.4 App Manifest Concepts

Each app should declare at least:

* `app_id`
* `display_name`
* `version`
* `runtime.type`
* `runtime.distribution`
* runtime compatibility target such as Python `3.11` or Node `20`
* one runtime-specific launch target such as `runtime.native.entrypoint`, `runtime.python.entry_script`, `runtime.node.entry_script`, or `runtime.web.entry`
* optional `args`
* `working_directory`
* `run_as_user` or app sandbox identity class
* `autostart` behavior
* `restart_policy`
* `environment` allowlist
* filesystem access declarations
* network needs
* platform API permissions
* device feature permissions
* logging verbosity hints
* health check mode

Runtime-managed apps should also declare enough intent for Core to reject unsafe packaging patterns. For example:

* Python apps may request `distribution = "venv"` or `distribution = "platform-runtime"`
* Node apps may request `distribution = "app-local-deps"` or `distribution = "platform-runtime"`
* production manifests should not imply `pip install`, `npm install`, or arbitrary build steps during launch

A filesystem declaration might look conceptually like:

* read-only access to its own install directory
* read-write access to its own data directory
* read-write access to its runtime directory
* optional read-only access to shared asset directories
* no general access to user home or system paths

The launcher should not trust the manifest blindly. TheCube Core should validate and compile it into an approved runtime policy.

---

## 8. Service Topology Options

There are two main ways to model app services in systemd.

## 8.1 Option A: One generated service per app

Example:

* `thecube-app-calendar.service`
* `thecube-app-music.service`
* `thecube-app-pomodoro.service`

### Advantages

* simple inspection with normal systemd tools
* clear per-app unit status
* easy per-app overrides
* easy tailored dependencies

### Disadvantages

* unit file generation and cleanup complexity
* more management overhead as app count grows

## 8.2 Option B: A templated unit

Example:

* `thecube-app@calendar.service`
* `thecube-app@music.service`

`%i` or `%I` can pass the app ID to the launcher.

### Advantages

* elegant and scalable
* fewer static unit files
* easy standardization

### Disadvantages

* some per-app deviations may need drop-ins or generated environment files

## 8.3 Recommendation

Use a **templated systemd unit** as the default architecture.

That gives a strong baseline:

* one unit definition
* app-specific runtime data via instance name and generated config
* manageable scaling as the app ecosystem grows

If a small number of special apps later need custom unit behavior, they can have dedicated units.

---

## 9. Proposed Runtime Data Flow

## 9.1 Source of truth

TheCube Core should maintain an internal app registry database that stores:

* installed app metadata
* approved permissions
* approved managed resource assignments such as shared PostgreSQL database and role names
* current enabled/disabled state
* current version
* policy compilation status
* managed resource provisioning status
* lifecycle preferences

## 9.2 Generated runtime artifacts

Before or during launch, TheCube Core can generate a resolved runtime bundle for the launcher, such as:

* `/run/thecube/launch/<app-id>/policy.json`
* `/run/thecube/launch/<app-id>/env.list`
* `/run/thecube/launch/<app-id>/argv.json`

These should be generated by trusted core logic and owned with restrictive permissions.

The launcher then consumes the resolved policy rather than a developer-supplied manifest directly.

This is important because it means:

* developer intent is not the final authority
* core policy validation happens before launch
* the launcher can remain smaller and simpler

---

## 10. Detailed Responsibilities

## 10.1 TheCube Core Responsibilities

TheCube Core should:

### App installation and registration

* install app packages into approved locations
* validate manifest format and version
* validate requested permissions against platform policy
* reject apps with invalid or prohibited requests
* provision approved managed resources such as shared PostgreSQL access
* compile approved runtime policy

### Lifecycle orchestration

* request app start via systemd
* request app stop via systemd
* request restart via systemd
* check app state via systemd or cached state
* distinguish enabled, disabled, installed, failed, and running states

### Permission brokering

* map high-level permissions such as `display.basic`, `notifications.post`, `audio.playback`, `companion.phone.mirror`, or `storage.app.private` into concrete runtime constraints
* expose APIs for resources that apps should not access directly

### Observability

* capture lifecycle events
* surface logs to developer UI
* record crash counts and failure reasons
* optionally quarantine repeatedly failing apps

### Policy ownership

* decide which paths are granted to Landlock
* decide which systemd sandboxing knobs are applied globally or per app class
* decide whether an app gets network access

---

## 10.2 systemd Responsibilities

systemd should:

* run the launcher for a given app instance
* enforce service restart policy
* maintain service state
* record logs in journald
* apply service-level restrictions such as namespace and file system protection where appropriate
* stop the service cleanly with signals and timeouts
* kill leftover processes in the cgroup if needed

systemd should be treated as the authoritative answer to "is the service running?"

---

## 10.3 Launcher Responsibilities

The launcher should be intentionally small. It is not a second core. It is not an app manager. It is a narrow execution wrapper.

It should:

### Read runtime policy

* load the resolved launch policy from a trusted path
* verify ownership, permissions, and format
* reject missing or malformed policy data

### Construct execution context

* determine target executable and argv
* set working directory
* set curated environment variables
* clear dangerous inherited environment values unless explicitly allowed

### Apply security restrictions

* optionally set `PR_SET_NO_NEW_PRIVS`
* apply Landlock rules
* optionally adjust rlimits
* optionally drop supplemental groups or verify existing service identity

### Final handoff

* perform `execve()` to replace the launcher process with the app

The launcher should avoid long-running supervision logic. That is systemd’s job.

---

## 11. Landlock Strategy

## 11.1 Role of Landlock in this design

Landlock should be used primarily for **fine-grained filesystem restrictions** on the launched app.

It is a very good fit for the last step before `exec()` because it lets the launcher confine the eventual app process and its descendants.

## 11.2 What Landlock is good for here

* restricting read access to only approved paths
* restricting write access to only approved app-private paths
* preventing opportunistic reads of unrelated system or app data
* enforcing least privilege even if the process is already running under a dedicated user

## 11.3 What Landlock should not be expected to solve alone

Landlock is not the whole sandbox story. It should not be the only defense.

It does not replace:

* dedicated Unix users/groups
* systemd service restrictions
* platform API permission checks
* brokered access to hardware buses and privileged services
* careful network exposure control

## 11.4 Recommended Landlock defaults

A typical app should get:

### Read-only

* its own install directory
* selected shared read-only assets
* maybe limited read-only access to system resources truly needed for runtime, if unavoidable

### Read-write

* its own persistent state directory
* its own cache directory
* its own runtime directory

### Denied by default

* other apps' install directories
* other apps' data directories
* general system configuration directories
* arbitrary user data paths
* device nodes unless explicitly unavoidable and carefully justified

## 11.5 Important architectural consequence

Because apps will not be granted direct access to many hardware or OS resources, **TheCube Core must provide brokered APIs** for those capabilities.

That is a good thing. It keeps app permissions understandable.

---

## 12. systemd Hardening Recommendations

Landlock should be paired with systemd hardening where practical.

Exact settings will need testing against the final runtime, but the design intent should include as much of the following as possible:

* `NoNewPrivileges=yes`
* `PrivateTmp=yes`
* `ProtectSystem=strict` or an appropriate reduced setting
* `ProtectHome=yes` or `tmpfs`-style isolation if relevant
* `ProtectControlGroups=yes`
* `ProtectKernelLogs=yes`
* `ProtectKernelModules=yes`
* `ProtectKernelTunables=yes`
* `RestrictSUIDSGID=yes`
* `LockPersonality=yes`
* `MemoryDenyWriteExecute=yes` where runtime permits it
* `RestrictRealtime=yes`
* `SystemCallArchitectures=native`
* `RemoveIPC=yes`
* carefully considered `SystemCallFilter=` profiles if feasible
* `DevicePolicy=closed` with explicit device allowances only if genuinely required
* `ReadWritePaths=` only for tightly scoped writable locations, especially if using a broader `ProtectSystem` mode

Not every app will tolerate every setting. The point is to make the baseline hardened and selectively relax only when justified.

---

## 13. Users, Identities, and Ownership

## 13.1 Service identity model

Avoid running apps as the same user as TheCube Core.

Recommended options:

### Option A: Shared low-privilege app user

All apps run as a common user such as `thecube-app`.

#### Pros

* simpler to implement initially
* easier file ownership patterns

#### Cons

* weaker isolation between apps unless heavily compensated by Landlock and directory permissions

### Option B: Per-app users

Each app runs as its own Unix user, such as `thecube-app-calendar`, `thecube-app-pomodoro`, etc.

#### Pros

* stronger isolation
* clearer ownership of writable data
* better blast-radius reduction

#### Cons

* more install/uninstall complexity
* more account management overhead

## 13.2 Recommendation

For long-term architecture, **per-app users are better**. For early development, a shared `thecube-app` user may be acceptable if paired with strong per-app directory controls and Landlock.

A staged approach is reasonable:

* early prototype: shared app user
* pre-release or production hardening: migrate toward per-app users or at least per-trust-tier users

---

## 14. API and Resource Access Philosophy

## 14.1 Do not give apps raw hardware access unless there is no reasonable alternative

This is especially important for TheCube because many of the most valuable features involve shared hardware and sensitive resources.

Examples include:

* display surfaces
* audio output and microphone capture
* sensors
* notifications
* phone mirroring integrations
* presence data
* NFC interactions
* companion personality state
* AI services

If apps directly access the underlying buses and devices, sandboxing becomes much harder and the platform becomes unstable.

## 14.2 Preferred model: brokered access through Core

Apps should talk to TheCube Core over a controlled IPC/API boundary. The core then performs privileged or shared operations on behalf of apps.

Examples:

* App requests "show this notification" instead of writing directly to display devices.
* App requests "play this sound asset" instead of opening audio hardware directly.
* App subscribes to presence events instead of reading raw sensor streams.
* App requests AI summarization instead of managing LLM backends on its own.

This fits the role already defined for TheCube-Core as the centralized API provider and system hub. fileciteturn0file2turn0file7

---

## 15. IPC Between Core and Apps

## 15.1 Likely IPC directions

There are really two different IPC relationships here:

### A. Core <-> systemd control path

TheCube Core needs a way to ask systemd to start, stop, and inspect units.

This should generally be done through the systemd control interface rather than by shelling out wherever possible.

### B. App <-> Core runtime path

Running apps need a way to use platform services.

A good long-term model is a local IPC API such as:

* Unix domain sockets
* D-Bus if you want richer service discovery and policy integration
* gRPC over local sockets if you want strongly typed service contracts
* a custom message bus only if there is a compelling reason

## 15.2 Recommendation

Use:

* **systemd's control interfaces** for lifecycle management
* **Unix domain socket based IPC** between apps and TheCube Core for platform services

This keeps the system local, efficient, and easier to sandbox than network-based localhost APIs.

---

## 16. Unit Design Recommendation

A templated unit could conceptually do the following:

* run under the app service identity
* receive app ID as the instance name
* load an app-specific environment file from `/run/thecube/launch/<app-id>/`
* invoke the launcher with the app ID or policy file path
* apply baseline systemd sandboxing
* set restart and timeout behavior
* join a dedicated slice if resource grouping is desired

Possible conceptual service classes:

* `thecube-app@.service` for standard apps
* `thecube-agent@.service` for background agents
* `thecube-ui@.service` for UI-heavy or foreground-integrated apps

This lets you evolve different policy baselines by class rather than per app only.

---

## 17. Resource Control and QoS

TheCube runs on embedded hardware, so resource governance matters.

systemd should be used to cap or bias:

* CPU shares or weights
* memory limits
* task count
* IO priority where appropriate

On Raspberry Pi 5, the design question is not just "can this runtime execute?" It is also:

* what startup latency does it introduce
* how much idle memory does it consume
* how much storage duplication does it create
* how many of these apps can reasonably stay resident at once

This argues for runtime budget classes:

### Lightweight apps

Examples:

* clock
* notification helper
* small sensor dashboard

Best fit:

* native
* possibly a small web bundle with brokered APIs

### Moderate interactive apps

Examples:

* notes
* calendar
* media controller

Best fit:

* native
* controlled Python or Node when the developer ergonomics justify it

### Heavy compute or service apps

Examples:

* AI service helper
* media processing
* large local web service

Best fit:

* dedicated service class
* tightly controlled first-party service
* native or otherwise carefully budgeted runtime usage

Potential policy classes:

### Foreground interactive apps

* higher CPU weight
* modest memory ceiling
* fast restart tolerance

### Background notification agents

* lower CPU weight
* strict memory ceiling
* conservative restart policy

### Experimental or developer apps

* lower trust tier
* tighter limits
* more verbose logging

TheCube Core can assign app classes at install time or via signed metadata.

---

## 18. Failure Handling Model

## 18.1 Types of failure

The platform should distinguish between:

* launch failure before `exec`
* app crash after launch
* app policy violation detected before launch
* repeated crash loop
* IPC health failure while process is still alive

## 18.2 Recommended behavior

### Launch policy failure

The launcher exits with a clear code and logs the reason.

### App crash

systemd applies `Restart=on-failure` for approved app classes.

### Crash loop

Core detects repeated failures and marks the app unhealthy. It may:

* stop retrying
* notify the user or developer UI
* offer safe mode launch in developer mode
* collect diagnostic bundle references

### Hung app

If applicable, use watchdog or app-level health pings via IPC for detection.

---

## 19. Logging and Diagnostics

## 19.1 Logging path

* app stdout/stderr -> journald via systemd
* launcher diagnostics -> journald
* core app lifecycle audit events -> core logs and optional journal entries

## 19.2 Recommended metadata tagging

Every log source should include or imply:

* app ID
* version
* service instance
* lifecycle event type
* policy version

## 19.3 Developer-facing diagnostic tools

Eventually, TheCube should provide a developer interface that can show:

* installed apps
* running status
* last exit code
* recent logs
* effective permissions
* restart counts
* launch time

This would be very helpful for the open app ecosystem direction already envisioned for the platform. fileciteturn0file6turn0file8

---

## 20. Trust Tiers

A useful refinement is to define app trust tiers.

## 20.1 Example tiers

### Tier 1: First-party core-adjacent apps

* highest trust among apps
* still sandboxed
* may receive broader but still explicit permissions

### Tier 2: Reviewed third-party apps

* standard sandbox baseline
* explicit permission grants only

### Tier 3: Experimental or sideloaded apps

* strictest baseline
* no raw device access
* tighter resource limits
* stronger warnings and developer-only install paths

This tiering can influence:

* allowable permissions
* default systemd hardening level
* restart policies
* UI warnings
* install workflow

---

## 21. Security Model Summary

The security model should be layered as follows:

### Layer 1: Controlled installation and manifest validation

TheCube Core validates what the app claims it needs.

### Layer 2: Dedicated service identity

The app runs as a low-privilege user, ideally isolated from Core and from other apps.

### Layer 3: systemd sandboxing

The unit definition restricts broad OS surface area.

### Layer 4: Landlock

The launcher applies fine-grained filesystem restrictions just before `exec()`.

### Layer 5: Brokered platform APIs

Sensitive and shared resources stay behind TheCube Core service boundaries.

### Layer 6: Observability and enforcement

Crashes, loops, and suspicious behavior become visible and actionable.

This is much stronger than relying on any one mechanism alone.

---

## 22. Recommended End-to-End Launch Sequence

Below is a concrete recommended launch sequence.

### Step 1: App install time

TheCube Core:

* validates package and manifest
* stores app metadata in registry
* creates data/cache/runtime directories
* materializes controlled runtime artifacts such as app-local Python `venv`s when required
* provisions approved managed resources such as shared PostgreSQL database access when requested
* assigns app user or class
* compiles approved runtime policy

### Step 2: Launch request

A user, automation, or system event asks to start the app.

### Step 3: Core policy resolution

TheCube Core:

* confirms app is installed and enabled
* checks that prerequisites are satisfied
* writes resolved launch artifacts into `/run/thecube/launch/<app-id>/`

### Step 4: systemd start

TheCube Core requests `start thecube-app@<app-id>.service`

### Step 5: systemd executes launcher

systemd:

* loads baseline sandboxing
* sets service identity
* starts launcher with app ID

### Step 6: launcher validation and confinement

The launcher:

* reads trusted policy
* validates the resolved executable and launch target
* prepares environment and cwd
* applies `no_new_privs`
* applies Landlock rules
* optionally sets other rlimits or hygiene steps

### Step 7: exec handoff

The launcher `execve()`s the real app

### Step 8: runtime

The app interacts with TheCube Core over local IPC and uses approved app-private storage

### Step 9: supervision

systemd tracks lifecycle and restarts if configured

### Step 10: observation

TheCube Core reflects status to UI, logs, and developer tools

---

## 23. Start, Stop, Restart, and Status Semantics

## 23.1 Start

A start request should fail cleanly if:

* app is disabled
* required policy is missing
* dependencies are unavailable
* current state forbids duplicate instance launch

## 23.2 Stop

Stopping should primarily be delegated to systemd so the full service cgroup is terminated cleanly.

## 23.3 Restart

Restart should usually mean systemd stop followed by systemd start, preserving the same resolved policy unless Core intentionally regenerates it.

## 23.4 Status

TheCube Core should expose status concepts more user-friendly than raw systemd states, such as:

* installed
* disabled
* ready
* starting
* running
* degraded
* failed
* crash-looping
* stopping

Internally, these can map from systemd state plus platform health knowledge.

---

## 24. Draft App Manifest Schema

This section proposes a first-pass app manifest structure for TheCube app ecosystem.

The main design goals of the manifest are:

* make app packaging predictable
* keep developer intent separate from final runtime authority
* support validation by TheCube Core
* support future expansion without breaking old apps
* describe permissions at a level developers can understand

The manifest should be treated as a **developer declaration**, not as the final launch authority. TheCube Core should validate it, normalize it, and compile an approved runtime policy for the launcher.

## 24.1 Manifest design principles

### Human-readable and machine-validated

The manifest should be easy for developers to read and write, but strict enough to validate automatically.

### Stable versioning

The manifest should include a schema version so TheCube Core can support migration and compatibility handling over time.

### Declarative, not procedural

The manifest should say what the app is and what it needs, not contain launch scripts or arbitrary setup logic.

### Minimize ambiguity

Paths, permissions, runtime family, and service mode should be explicit.

### Support policy review

The manifest should be structured so TheCube Core can clearly answer:

* what this app is trying to execute
* what files it wants to read or write
* whether it needs network access
* what platform APIs it wants
* whether it is long-running or on-demand

## 24.2 Recommended format

A good initial choice is **JSON** for the manifest because:

* it is easy to parse from C++ and many other languages
* it supports JSON Schema validation well
* it is easy to embed into package tooling
* it is common for app/package metadata

TOML is also reasonable, but JSON is probably the easiest first implementation.

## 24.3 Proposed top-level manifest shape

A first draft could look like this conceptually:

```json
{
  "schema_version": "1.0",
  "app": {
    "id": "com.amcd.notes",
    "name": "Notes",
    "version": "0.9.0",
    "vendor": "A-McD Technology LLC",
    "description": "Local notes app for TheCube.",
    "category": "productivity",
    "args": ["--mode", "cube"],
    "working_directory": ".",
    "type": "interactive",
    "autostart": false
  },
  "runtime": {
    "type": "python",
    "distribution": "venv",
    "compatibility": "3.11",
    "python": {
      "entry_script": "main.py"
    },
    "restart_policy": "on-failure",
    "max_restart_burst": 5,
    "restart_window_seconds": 60,
    "stop_timeout_seconds": 10,
    "start_timeout_seconds": 15,
    "healthcheck": {
      "mode": "ipc-ping",
      "interval_seconds": 30,
      "timeout_seconds": 5
    }
  },
  "permissions": {
    "platform": [
      "display.basic",
      "notifications.post",
      "storage.app.private"
    ],
    "filesystem": {
      "read_only": [
        "app://install",
        "app://assets"
      ],
      "read_write": [
        "app://data",
        "app://cache",
        "app://runtime"
      ]
    },
    "network": {
      "allow": true,
      "inbound": false,
      "domains": ["sync.notes.example"],
      "local_network": false
    }
  },
  "environment": {
    "allow_inherit": [],
    "set": {
      "TZ": "UTC"
    }
  },
  "ui": {
    "display_name": "Notes",
    "icon": "assets/icon.png",
    "foreground": true
  }
}
```

That exact structure will likely evolve, but it provides a strong baseline.

## 24.4 Field-by-field manifest specification

### `schema_version`

String. Required.

Identifies the manifest format version.

Example:

```json
"schema_version": "1.0"
```

### `app`

Object. Required.

Contains the core identity and execution metadata.

#### `app.id`

String. Required.

Globally unique app identifier. Reverse-DNS style is recommended.

Examples:

* `com.amcd.companion.notes`
* `com.amcd.thecube.clock`
* `io.thirdparty.spotify-controller`

Rules:

* lowercase letters, digits, dots, and hyphens only
* immutable after first release
* used in install path, runtime path, service instance naming, and registry identity

#### `app.name`

String. Required.

Human-readable product name.

#### `app.version`

String. Required.

App version string. SemVer is recommended.

#### `app.vendor`

String. Optional but recommended.

Name of the publisher.

#### `app.description`

String. Optional.

Short app summary.

#### `app.category`

String. Optional.

Examples:

* `utility`
* `music`
* `productivity`
* `system`
* `developer`

#### `app.args`

Array of strings. Optional.

Static default arguments for the app.

#### `app.working_directory`

String. Optional.

Relative working directory inside the install root. Default could be `.`.

#### `app.type`

String. Required.

Initial recommended values:

* `interactive`
* `background`
* `agent`
* `oneshot`

This helps Core choose service defaults and UI behavior.

#### `app.autostart`

Boolean. Optional.

Requests boot/startup launch, subject to Core policy approval.

## 24.5 `runtime` section

Object. Required.

This section describes desired runtime semantics. TheCube Core may clamp or override these values.

#### `runtime.type`

String. Required.

Recommended initial values:

* `native`
* `python`
* `node`
* `web-bundle`

This is the field that tells TheCube which runtime family the app expects.

#### `runtime.distribution`

String. Optional but strongly recommended.

Recommended initial values:

* `self-contained`
* `platform-runtime`
* `venv`
* `app-local-deps`
* `frozen-binary`
* `developer-source`

This describes how the app expects its runtime and dependencies to be packaged.

#### `runtime.compatibility`

String. Optional for native apps. Recommended for runtime-managed apps.

Examples:

* `3.11`
* `20`

This lets the manifest declare a supported Python or Node compatibility line without hard-coding host paths.

#### `runtime.native`

Object. Required when `runtime.type = "native"`.

Proposed fields:

* `entrypoint`

The `entrypoint` must be a relative path inside the app install directory.

#### `runtime.python`

Object. Required when `runtime.type = "python"`.

Proposed fields:

* `entry_script`

The `entry_script` must be a relative path inside the app install directory.

If `distribution = "venv"`, the manifest is saying the app should be installed with an app-local virtual environment. The app should not create or mutate that virtual environment at launch time.

#### `runtime.node`

Object. Required when `runtime.type = "node"`.

Proposed fields:

* `entry_script`

The `entry_script` must be a relative path inside the app install directory, typically something like `dist/index.js`.

Node apps should assume a platform-managed Node binary in a fixed location. `nvm` is reasonable for development on a workstation, but it should not be part of the device runtime contract.

#### `runtime.web`

Object. Required when `runtime.type = "web-bundle"`.

Proposed fields:

* `entry`
* optional `backend_service`

This allows a frontend bundle to be represented cleanly, with an optional backend service resolved by Core.

#### `runtime.restart_policy`

String. Optional.

Recommended allowed values:

* `never`
* `on-failure`
* `always`

#### `runtime.max_restart_burst`

Integer. Optional.

Maximum restart attempts within the configured window.

#### `runtime.restart_window_seconds`

Integer. Optional.

Window used with restart burst counting.

#### `runtime.stop_timeout_seconds`

Integer. Optional.

Graceful stop timeout.

#### `runtime.start_timeout_seconds`

Integer. Optional.

Maximum launch time before systemd or Core treats startup as failed.

#### `runtime.healthcheck`

Object. Optional.

Proposed fields:

* `mode`: `none`, `ipc-ping`, `ready-notify`, `heartbeat`
* `interval_seconds`
* `timeout_seconds`

This allows future health monitoring integration.

## 24.6 `permissions` section

Object. Required.

This is one of the most important sections. It should be high-level and understandable, not a raw dump of Linux internals.

### `permissions.platform`

Array of strings. Optional.

These are platform API capabilities the app wants from TheCube Core.

Examples:

* `display.basic`
* `display.foreground`
* `notifications.post`
* `audio.playback`
* `audio.capture`
* `ai.text.generate`
* `presence.read`
* `calendar.read`
* `network.http_client`
* `storage.app.private`

These should map to Core-side authorization, not direct OS access.

### `permissions.filesystem`

Object. Required.

This declares the app's desired file access in abstract form.

Recommended fields:

* `read_only`: array of logical path tokens or relative subpaths
* `read_write`: array of logical path tokens or relative subpaths

Logical path tokens keep developer intent clean and let Core map them to real directories.

Recommended built-in tokens:

* `app://install`
* `app://assets`
* `app://data`
* `app://cache`
* `app://runtime`
* `shared://readonly/<name>`

Avoid allowing arbitrary absolute host paths in normal app manifests.

### `permissions.network`

Object. Optional.

Proposed fields:

* `allow`: boolean
* `inbound`: boolean
* `domains`: array of strings
* `local_network`: boolean

Even if this is declared, TheCube Core should still approve or deny it and potentially reduce the scope.

For the first version, this may simply become a yes/no capability rather than domain filtering if you want to keep implementation simple.

## 24.7 `environment` section

Object. Optional.

Apps often need environment variables, but this must be tightly controlled.

Proposed fields:

* `allow_inherit`: array of env var names that may be inherited from the service environment
* `set`: object of explicit key/value pairs

Rules:

* default should be inherit almost nothing
* deny unsafe variables by default
* Core may inject additional platform variables such as sockets or app runtime paths

## 24.8 `ui` section

Object. Optional.

This is not strictly needed for launching, but it is useful for app registry and frontend integration.

Suggested fields:

* `display_name`
* `icon`
* `foreground`
* `settings_page`
* `default_window_mode`

## 24.9 `resources` section

Object. Optional.

This section is for platform-managed shared services that an app wants Core to provision or broker. The app is declaring intent, not supplying host-level connection details or launch-time setup logic.

The key rule is:

* apps may request managed resources
* Core approves, provisions, and injects them
* apps do not start or administer those services directly

### `resources.postgresql`

Object. Optional.

This requests access to the shared CORE-managed PostgreSQL service, which is expected to run as a single Dockerized infrastructure service under CORE control.

Recommended fields:

* `required`: boolean
* `database`: string
* `extensions`: array of strings
* `schema_mode`: string

Recommended values:

* `database`
  * `app-default`
  * or a logical database name hint such as `notes`
* `schema_mode`
  * `app-owned`
  * `shared-db-app-schema`

Recommended behavior:

* default should be one database and one database role per app
* app manifests should not contain usernames, passwords, host paths, or raw superuser SQL
* apps should never receive cluster-admin credentials
* Core should validate requested extensions against an allowlist
* Core should inject standard connection variables or secret file references at launch time

Example:

```json
{
  "resources": {
    "postgresql": {
      "required": true,
      "database": "app-default",
      "extensions": ["uuid-ossp"],
      "schema_mode": "app-owned"
    }
  }
}
```

## 24.10 Draft JSON Schema example

Below is a simplified draft schema, intentionally incomplete but concrete enough to start implementation.

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://thecube.local/schema/app-manifest-1.0.json",
  "type": "object",
  "required": ["schema_version", "app", "runtime", "permissions"],
  "properties": {
    "schema_version": {
      "type": "string",
      "const": "1.0"
    },
    "app": {
      "type": "object",
      "required": ["id", "name", "version", "type"],
      "properties": {
        "id": {
          "type": "string",
          "pattern": "^[a-z0-9.-]+$"
        },
        "name": { "type": "string", "minLength": 1 },
        "version": { "type": "string", "minLength": 1 },
        "vendor": { "type": "string" },
        "description": { "type": "string" },
        "category": { "type": "string" },
        "args": {
          "type": "array",
          "items": { "type": "string" }
        },
        "working_directory": {
          "type": "string"
        },
        "type": {
          "type": "string",
          "enum": ["interactive", "background", "agent", "oneshot"]
        },
        "autostart": { "type": "boolean" }
      },
      "additionalProperties": false
    },
    "runtime": {
      "type": "object",
      "required": ["type"],
      "properties": {
        "type": {
          "type": "string",
          "enum": ["native", "python", "node", "web-bundle"]
        },
        "distribution": {
          "type": "string",
          "enum": [
            "self-contained",
            "platform-runtime",
            "venv",
            "app-local-deps",
            "frozen-binary",
            "developer-source"
          ]
        },
        "compatibility": {
          "type": "string"
        },
        "native": {
          "type": "object",
          "properties": {
            "entrypoint": {
              "type": "string",
              "minLength": 1,
              "not": { "pattern": "^/" }
            }
          },
          "additionalProperties": false
        },
        "python": {
          "type": "object",
          "properties": {
            "entry_script": {
              "type": "string",
              "minLength": 1,
              "not": { "pattern": "^/" }
            }
          },
          "additionalProperties": false
        },
        "node": {
          "type": "object",
          "properties": {
            "entry_script": {
              "type": "string",
              "minLength": 1,
              "not": { "pattern": "^/" }
            }
          },
          "additionalProperties": false
        },
        "web": {
          "type": "object",
          "properties": {
            "entry": {
              "type": "string",
              "minLength": 1,
              "not": { "pattern": "^/" }
            },
            "backend_service": {
              "type": "string"
            }
          },
          "additionalProperties": false
        },
        "restart_policy": {
          "type": "string",
          "enum": ["never", "on-failure", "always"]
        },
        "max_restart_burst": { "type": "integer", "minimum": 0 },
        "restart_window_seconds": { "type": "integer", "minimum": 1 },
        "stop_timeout_seconds": { "type": "integer", "minimum": 1 },
        "start_timeout_seconds": { "type": "integer", "minimum": 1 }
      },
      "additionalProperties": false
    },
    "permissions": {
      "type": "object",
      "properties": {
        "platform": {
          "type": "array",
          "items": { "type": "string" }
        },
        "filesystem": {
          "type": "object",
          "properties": {
            "read_only": {
              "type": "array",
              "items": { "type": "string" }
            },
            "read_write": {
              "type": "array",
              "items": { "type": "string" }
            }
          },
          "required": ["read_only", "read_write"],
          "additionalProperties": false
        },
        "network": {
          "type": "object",
          "properties": {
            "allow": { "type": "boolean" },
            "inbound": { "type": "boolean" },
            "domains": {
              "type": "array",
              "items": { "type": "string" }
            },
            "local_network": { "type": "boolean" }
          },
          "additionalProperties": false
        }
      },
      "required": ["filesystem"],
      "additionalProperties": false
    },
    "resources": {
      "type": "object",
      "properties": {
        "postgresql": {
          "type": "object",
          "properties": {
            "required": { "type": "boolean" },
            "database": {
              "type": "string",
              "pattern": "^(app-default|[a-z0-9_]+)$"
            },
            "extensions": {
              "type": "array",
              "items": {
                "type": "string",
                "pattern": "^[a-z0-9_-]+$"
              }
            },
            "schema_mode": {
              "type": "string",
              "enum": ["app-owned", "shared-db-app-schema"]
            }
          },
          "additionalProperties": false
        }
      },
      "additionalProperties": false
    },
    "environment": {
      "type": "object",
      "properties": {
        "allow_inherit": {
          "type": "array",
          "items": { "type": "string" }
        },
        "set": {
          "type": "object",
          "additionalProperties": { "type": "string" }
        }
      },
      "additionalProperties": false
    },
    "ui": {
      "type": "object",
      "properties": {
        "display_name": { "type": "string" },
        "icon": { "type": "string" },
        "foreground": { "type": "boolean" }
      },
      "additionalProperties": false
    }
  },
  "additionalProperties": false
}
```

## 24.11 Manifest validation rules beyond JSON Schema

Some important rules are better enforced in Core logic than in pure JSON Schema:

* the runtime-specific launch target must resolve inside the app install directory
* working directory must resolve inside install directory
* path tokens must map to allowed runtime locations only
* `runtime.type`, `runtime.distribution`, and `runtime.compatibility` must map to a platform-supported runtime matrix
* Python `venv` creation or dependency installation must happen at package build time or install time, not at launch time
* Node apps must resolve against a platform-managed Node runtime rather than `nvm` or a shell-profile-dependent launcher
* app ID must be unique in the registry
* requested platform permissions must exist in the permission catalog
* requested network permissions may be denied based on trust tier
* autostart may be rejected for third-party or high-risk apps

## 24.12 Example minimal manifest

```json
{
  "schema_version": "1.0",
  "app": {
    "id": "com.amcd.clock",
    "name": "Clock",
    "version": "0.1.0",
    "type": "interactive"
  },
  "runtime": {
    "type": "native",
    "distribution": "self-contained",
    "native": {
      "entrypoint": "bin/clock"
    }
  },
  "permissions": {
    "filesystem": {
      "read_only": ["app://install"],
      "read_write": ["app://runtime"]
    }
  }
}
```

## 25. Core-to-Launcher Policy File Format

The manifest is developer-facing. The policy file is **Core-generated and launcher-consumed**.

This distinction is critical.

The launcher should never need to interpret vague developer intent. It should receive a concrete, trusted launch specification generated by TheCube Core after validation.

## 25.1 Goals of the policy file

The policy file should:

* be concrete and resolved
* contain only approved values
* map abstract manifest requests to real paths and effective settings
* be safe for direct launcher consumption
* be immutable for the duration of a launch attempt
* be easy to validate in a small C++ launcher

## 25.2 Recommended format

Use **JSON** initially for the launcher policy as well.

Reasons:

* easy to generate from Core
* easy to parse in C++ with libraries such as nlohmann::json
* easy to inspect in logs and debug tools
* sufficient performance for launch-time parsing

Later, this could be compiled to a binary format if startup micro-optimization becomes important, but JSON is the right first step.

## 25.3 Recommended file location

A per-launch runtime directory is a good fit, for example:

* `/run/thecube/launch/<app-id>/policy.json`
* `/run/thecube/launch/<app-id>/env.json`
* `/run/thecube/launch/<app-id>/meta.json`

Or, more simply, a single resolved file:

* `/run/thecube/launch/<app-id>/launch-policy.json`

I recommend a **single file first** unless you find a strong need to split it.

## 25.4 Policy file ownership and trust rules

The launcher should enforce the following assumptions:

* the policy file must be owned by root or the trusted Core service identity
* the containing directory must not be writable by the target app user
* the file must not be symlinked to an unexpected location
* file permissions must be restrictive
* the app process must not be able to modify the policy before `exec`

## 25.5 Proposed top-level policy structure

A draft policy file could look like this:

```json
{
  "policy_version": "1.0",
  "launch_id": "a98a8e36-0d13-4f3d-b2f4-3a0db6d2ea7b",
  "generated_at": "2026-03-14T18:30:00Z",
  "app": {
    "id": "com.amcd.notes",
    "version": "0.9.0",
    "install_root": "/opt/thecube/apps/com.amcd.notes",
    "argv": [
      "/opt/thecube/apps/com.amcd.notes/.venv/bin/python",
      "/opt/thecube/apps/com.amcd.notes/main.py",
      "--mode",
      "cube"
    ],
    "working_directory": "/opt/thecube/apps/com.amcd.notes"
  },
  "runtime": {
    "type": "python",
    "distribution": "venv",
    "compatibility": "3.11",
    "resolved_executable": "/opt/thecube/apps/com.amcd.notes/.venv/bin/python",
    "launch_target": "/opt/thecube/apps/com.amcd.notes/main.py"
  },
  "identity": {
    "run_as_user": "thecube-app",
    "run_as_group": "thecube-app",
    "supplementary_groups": []
  },
  "environment": {
    "clear_inherited": true,
    "set": {
      "THECUBE_APP_ID": "com.amcd.notes",
      "THECUBE_RUNTIME_DIR": "/run/thecube/apps/com.amcd.notes",
      "THECUBE_DATA_DIR": "/var/lib/thecube/apps/com.amcd.notes",
      "THECUBE_CACHE_DIR": "/var/cache/thecube/apps/com.amcd.notes",
      "THECUBE_CORE_SOCKET": "/run/thecube/core/core.sock"
    }
  },
  "filesystem": {
    "landlock": {
      "read_only": [
        "/opt/thecube/apps/com.amcd.notes",
        "/usr/share/zoneinfo"
      ],
      "read_write": [
        "/var/lib/thecube/apps/com.amcd.notes",
        "/var/cache/thecube/apps/com.amcd.notes",
        "/run/thecube/apps/com.amcd.notes"
      ],
      "create": [
        "/var/cache/thecube/apps/com.amcd.notes",
        "/run/thecube/apps/com.amcd.notes"
      ]
    }
  },
  "platform": {
    "allowed_permissions": [
      "display.basic",
      "notifications.post",
      "storage.app.private"
    ],
    "ipc": {
      "core_socket": "/run/thecube/core/core.sock",
      "app_socket": "/run/thecube/apps/com.amcd.notes/app.sock"
    }
  },
  "network": {
    "allow": true,
    "inbound": false,
    "local_network": false,
    "domains": ["sync.notes.example"]
  },
  "limits": {
    "max_open_files": 256,
    "max_processes": 32,
    "core_dump": false
  },
  "integrity": {
    "manifest_digest_sha256": "...",
    "package_digest_sha256": "..."
  }
}
```

## 25.6 Field-by-field policy specification

### `policy_version`

String. Required.

Version of the launcher policy format.

### `launch_id`

String. Required.

Unique identifier for this launch attempt, useful for logging and audit trails.

### `generated_at`

String. Optional but recommended.

UTC timestamp of policy generation.

### `app`

Object. Required.

This section should be fully resolved.

#### `app.id`

String. Required.

#### `app.version`

String. Required.

#### `app.install_root`

Absolute path. Required.

#### `app.argv`

Array of strings. Required.

Fully resolved argv, with argv[0] included.

#### `app.working_directory`

Absolute path. Required.

### `runtime`

Object. Required.

This section carries the fully resolved runtime decision made by Core.

#### `runtime.type`

String. Required.

Examples:

* `native`
* `python`
* `node`
* `web-bundle`

#### `runtime.distribution`

String. Optional but recommended.

Examples:

* `self-contained`
* `venv`
* `app-local-deps`
* `frozen-binary`

#### `runtime.compatibility`

String. Optional.

Useful for diagnostics and audit, especially when Core resolves platform-managed runtimes.

#### `runtime.resolved_executable`

Absolute path. Required.

This is the binary that `execve()` will ultimately target as `argv[0]`. For native or frozen-binary apps, this may be the app binary itself. For Python or Node apps, this may be the interpreter runtime.

#### `runtime.launch_target`

Absolute path. Optional but usually recommended.

For native apps, this may be the same path as `runtime.resolved_executable`. For runtime-managed apps, this is typically the script or bundle entry file inside the app install root.

## 25.7 `identity` section

Object. Optional if systemd already guarantees this, but still useful for cross-checking and logging.

Fields:

* `run_as_user`
* `run_as_group`
* `supplementary_groups`

The launcher may not need to change identity itself if systemd already does it. Even so, carrying this section is useful for verification and diagnostics.

## 25.8 `environment` section

Object. Required.

#### `environment.clear_inherited`

Boolean.

Whether the launcher should clear the inherited environment before applying the allowlisted set.

#### `environment.set`

Object mapping environment variable names to values.

This should already be fully resolved. The launcher should not need to merge complex inheritance logic on its own.

## 25.9 `filesystem` section

Object. Required.

This is where the launcher gets concrete Landlock inputs.

### `filesystem.landlock.read_only`

Array of absolute paths.

Directories or files the process may read.

### `filesystem.landlock.read_write`

Array of absolute paths.

Directories the process may read and write.

### `filesystem.landlock.create`

Array of absolute paths.

Optional list of directories the launcher may create or verify before execution.

This is useful for runtime/cache path setup.

## 25.10 `platform` section

Object. Optional but recommended.

This does not directly drive launcher confinement much, but it is useful to pass app identity and platform permission context into the process environment and diagnostics.

Suggested fields:

* `allowed_permissions`
* `ipc.core_socket`
* `ipc.app_socket`
* optional service endpoints

## 25.11 `network` section

Object. Optional.

For early versions, the launcher may only record this for environment/context while systemd or other mechanisms enforce actual network policy. Still, it should be represented in the effective policy.

Fields:

* `allow`
* `inbound`
* `local_network`
* `domains`

## 25.12 `limits` section

Object. Optional.

Useful if the launcher is responsible for selected rlimit setup.

Possible fields:

* `max_open_files`
* `max_processes`
* `core_dump`
* `max_stack_bytes`
* `max_address_space_bytes`

## 25.13 `integrity` section

Object. Optional but useful.

Possible fields:

* `manifest_digest_sha256`
* `package_digest_sha256`
* `launch_target_digest_sha256`

This can help with diagnostics, attestation, and future signature validation.

## 25.14 Validation rules for the launcher policy

The launcher should enforce strict validation and fail closed.

Recommended rules:

* required fields must exist
* all critical paths must be absolute
* `runtime.resolved_executable` must either be inside `install_root` or inside an approved platform runtime root such as `/opt/thecube/runtimes/`
* `runtime.launch_target`, when present, must be inside `install_root`
* working_directory must be inside install_root or another approved path
* read_write paths must not overlap sensitive system directories
* policy version must be supported
* no unknown critical structure if using strict parsing mode
* environment variable names must match a safe pattern
* if `clear_inherited` is true, launcher must fully replace env

## 25.15 Separation between manifest and effective policy

This is a key design rule:

* **Manifest** = what the developer asks for
* **Effective launch policy** = what TheCube Core approves and resolves

Example:

* Manifest requests `network.allow = true`
* Core trust policy says no network for sideloaded apps
* Launcher policy contains `network.allow = false`

That is exactly how it should work.

## 25.16 Suggested compile pipeline

A clean Core-side compile pipeline could be:

1. Read and parse manifest
2. Validate schema
3. Validate package structure
4. Resolve the runtime family, distribution class, and allowed compatibility version against the platform support matrix
5. Resolve logical paths such as `app://data` into absolute host paths
6. Intersect requested permissions with trust-tier policy
7. Apply Core defaults
8. Generate final argv, env, working directory, and filesystem lists
9. Verify that any controlled install/runtime artifacts already exist, such as an app-local Python `venv`
10. Write launch policy into `/run/thecube/launch/<app-id>/launch-policy.json`
11. Ask systemd to start the service instance

## 25.17 Example manifest-to-policy transformation

### Manifest excerpt

```json
{
  "schema_version": "1.0",
  "app": {
    "id": "com.amcd.notes",
    "name": "Notes",
    "version": "0.9.0",
    "type": "interactive"
  },
  "runtime": {
    "type": "python",
    "distribution": "venv",
    "compatibility": "3.11",
    "python": {
      "entry_script": "main.py"
    }
  },
  "permissions": {
    "platform": ["display.basic", "storage.app.private"],
    "filesystem": {
      "read_only": ["app://install"],
      "read_write": ["app://data", "app://runtime"]
    }
  }
}
```

### Effective launch policy excerpt

```json
{
  "policy_version": "1.0",
  "app": {
    "id": "com.amcd.notes",
    "install_root": "/opt/thecube/apps/com.amcd.notes",
    "argv": [
      "/opt/thecube/apps/com.amcd.notes/.venv/bin/python",
      "/opt/thecube/apps/com.amcd.notes/main.py"
    ],
    "working_directory": "/opt/thecube/apps/com.amcd.notes"
  },
  "runtime": {
    "type": "python",
    "distribution": "venv",
    "compatibility": "3.11",
    "resolved_executable": "/opt/thecube/apps/com.amcd.notes/.venv/bin/python",
    "launch_target": "/opt/thecube/apps/com.amcd.notes/main.py"
  },
  "environment": {
    "clear_inherited": true,
    "set": {
      "THECUBE_APP_ID": "com.amcd.notes",
      "THECUBE_DATA_DIR": "/var/lib/thecube/apps/com.amcd.notes",
      "THECUBE_RUNTIME_DIR": "/run/thecube/apps/com.amcd.notes",
      "THECUBE_CORE_SOCKET": "/run/thecube/core/core.sock"
    }
  },
  "filesystem": {
    "landlock": {
      "read_only": ["/opt/thecube/apps/com.amcd.notes"],
      "read_write": [
        "/var/lib/thecube/apps/com.amcd.notes",
        "/run/thecube/apps/com.amcd.notes"
      ]
    }
  },
  "platform": {
    "allowed_permissions": ["display.basic", "storage.app.private"]
  }
}
```

### Shared PostgreSQL manifest excerpt

```json
{
  "schema_version": "1.0",
  "app": {
    "id": "com.amcd.notes",
    "name": "Notes",
    "version": "0.9.0",
    "type": "interactive"
  },
  "runtime": {
    "type": "python",
    "distribution": "venv",
    "compatibility": "3.11",
    "python": {
      "entry_script": "main.py"
    }
  },
  "permissions": {
    "filesystem": {
      "read_only": ["app://install"],
      "read_write": ["app://data", "app://runtime"]
    }
  },
  "resources": {
    "postgresql": {
      "required": true,
      "database": "app-default",
      "extensions": ["uuid-ossp"],
      "schema_mode": "app-owned"
    }
  }
}
```

### Shared PostgreSQL effective policy and provisioning result

For a manifest like the above, Core should:

1. Ensure the shared Dockerized PostgreSQL service is running.
2. Create or reconcile a per-app database such as `app_com_amcd_notes`.
3. Create or reconcile a per-app database role such as `app_com_amcd_notes`.
4. Apply only the requested and approved extension set.
5. Store credentials in a Core-managed secret store, not in the manifest or `apps.db`.
6. Inject standard libpq-style connection settings into the app launch environment.

Example effective policy excerpt:

```json
{
  "resources": {
    "postgresql": {
      "service": "thecube-postgres",
      "database": "app_com_amcd_notes",
      "role": "app_com_amcd_notes",
      "host": "127.0.0.1",
      "port": 5432,
      "schema_mode": "app-owned",
      "extensions": ["uuid-ossp"],
      "secret_ref": "secret://apps/com.amcd.notes/postgresql"
    }
  },
  "environment": {
    "clear_inherited": true,
    "set": {
      "PGHOST": "127.0.0.1",
      "PGPORT": "5432",
      "PGDATABASE": "app_com_amcd_notes",
      "PGUSER": "app_com_amcd_notes",
      "PGPASSFILE": "/run/thecube/secrets/com.amcd.notes/pgpass"
    }
  }
}
```

### Recommended `apps.db` additions for managed PostgreSQL

Because the manifest on disk remains the source of truth, the app registry should only cache derived PostgreSQL metadata and mutable provisioning state.

Recommended added columns:

* `postgresql_required`
* `postgresql_db_name`
* `postgresql_role_name`
* `postgresql_schema_mode`
* `postgresql_extensions`
* `postgresql_provision_status`
* `postgresql_last_error`

Recommended rules:

* do not store database passwords in `apps.db`
* do not store cluster-admin connection strings in `apps.db`
* treat these fields as Core-derived cache and status, not developer-authored truth
* preserve per-app database identity across ordinary app upgrades unless explicitly purged

This gives Core enough registry context to diagnose install and launch failures while keeping credentials in a separate secret-management path.

## 25.18 Recommended first implementation scope

To avoid overbuilding, the first real implementation should probably support a runtime-aware shape while only enabling a small number of runtime classes at first.

In practice, that means:

* design the manifest and policy schema so they are not native-only
* start with `native` as the mandatory production path
* optionally add `python` with `distribution = "venv"` as the first managed runtime
* treat shared PostgreSQL as an optional platform resource, not a per-app bundled service
* do not introduce global `pip` state, `npm install` at launch, or `nvm` into the device lifecycle

### Manifest v1 minimum

* `schema_version`
* `app.id`
* `app.name`
* `app.version`
* `app.type`
* `runtime.type`
* one runtime-specific target field such as `runtime.native.entrypoint`
* optional `app.args`
* `permissions.filesystem.read_only`
* `permissions.filesystem.read_write`
* optional `permissions.platform`
* optional `resources.postgresql`

### Launcher policy v1 minimum

* `policy_version`
* `app.id`
* `app.install_root`
* `app.argv`
* `app.working_directory`
* `runtime.type`
* `runtime.resolved_executable`
* optional `runtime.launch_target`
* `environment.clear_inherited`
* `environment.set`
* `filesystem.landlock.read_only`
* `filesystem.landlock.read_write`

That is enough to build a meaningful end-to-end prototype without prematurely solving every possible permission class.

## 25.19 Recommended final guidance

For TheCube, the best pattern is:

* keep the **manifest developer-friendly and abstract**
* keep the **launcher policy concrete and resolved**
* let **TheCube Core be the compiler and authority** between the two
* keep launch-time behavior boring, deterministic, and free of package installation side effects
* use Python `venv` only as a controlled packaging/install mechanism
* use fixed platform-managed Node runtimes on-device rather than `nvm`

That gives you clean separation of intent, approval, and execution.

## 26. Open Questions and Future Decisions

There are several design decisions that can be deferred, but should be tracked.

## 26.1 Per-app user versus shared app user

Strong recommendation for eventual per-app identities, but shared-user prototype is acceptable early on.

## 26.2 IPC framework choice

Unix sockets are a good baseline. D-Bus may be worth considering if you want rich introspection and policy integration.

## 26.3 Policy compilation format

Decide whether runtime policy is JSON, TOML, binary, or generated environment plus argv files.

## 26.4 Developer mode

You will likely want a developer mode that slightly relaxes sandboxing for local testing while making the relaxed state obvious in UI and logs.

## 26.5 Multi-process apps

Some apps may spawn helpers. This needs to be supported, but only within the same systemd cgroup and Landlock inheritance model.

## 26.6 Direct network access

Some apps may need internet or LAN access. This should become an explicit permission class and not be assumed by default.

## 26.7 Supported runtime matrix

Decide the first officially supported runtime set and how aggressively TheCube will add more. A reasonable initial answer is `native`, one Python line, one Node line, and `web-bundle`.

## 26.8 Web bundle versus backend service split

Some apps will really be a frontend bundle plus a local service. Decide whether that should be one manifest with two launchable parts or a parent app package that references a backend service package.

## 26.9 Containers

Container support can be deferred. If it is ever added, it should be an explicit platform feature rather than a fallback for apps that do not fit the normal runtime models.

## 26.10 Shared PostgreSQL service lifecycle

If shared PostgreSQL is adopted early, Core will need a clear policy for:

* provisioning and reconciling per-app databases and roles
* credential rotation and secret storage
* backup and restore boundaries
* extension allowlisting
* uninstall behavior: preserve by default or purge on request
* major-version upgrade strategy for the shared Dockerized service

---

## 27. Implementation Roadmap

## 27.1 Phase 1: Basic lifecycle foundation

* create templated `thecube-app@.service`
* build minimal launcher that reads app ID and `exec()`s the resolved target
* integrate Core start/stop/status with systemd
* capture logs and exit codes

## 27.2 Phase 2: Manifest and policy pipeline

* define app manifest schema
* build Core-side validation and policy compilation
* define the initial supported runtime matrix and fixed runtime install roots
* compile runtime declarations into fully resolved `argv`
* generate runtime artifacts in `/run/thecube/launch/`
* add optional provisioning flow for shared PostgreSQL requests

## 27.3 Phase 3: Sandboxing baseline

* add systemd hardening defaults
* add launcher Landlock enforcement
* move apps to dedicated writable directories

## 27.4 Phase 4: Brokered platform APIs

* define IPC interface for notifications, display, audio, sensors, and AI services
* remove need for direct hardware access by apps wherever possible

## 27.5 Phase 5: Developer tooling

* developer docs
* app packaging examples
* controlled packaging helpers for Python `venv` apps and Node app-local dependency layouts
* status and logs UI
* permission inspection tools

## 27.6 Phase 6: Production hardening

* crash-loop containment
* trust tiers
* signed packages or reviewed install flow
* per-app users or stronger identity separation

---

## 28. Recommended Initial Decisions

If you want a clean starting point, the following is a solid initial architecture:

### Immediate recommendations

* Use **TheCube Core** as the policy owner and lifecycle orchestrator.
* Use **templated systemd units** for app instances.
* Use **a custom launcher** as the final execution wrapper.
* Use **Landlock in the launcher** for fine-grained filesystem access control.
* Use **systemd sandboxing** for coarse-grained OS hardening.
* Use **brokered IPC through Core** instead of direct hardware access.
* Keep apps in `/opt/thecube/apps/<app-id>/` and writable state in `/var/lib/thecube/apps/<app-id>/`.
* Treat **runtime family and distribution** as first-class manifest concepts from the beginning.
* Use **Python app-local `venv`s** only as controlled install artifacts, not as mutable runtime state.
* Use **fixed platform-managed Node runtimes** on-device instead of `nvm`.
* Treat **shared PostgreSQL** as a CORE-managed platform resource, not an app-owned service.
* Support **web bundle apps** as a first-class UI model.
* Start with a **minimal manifest schema** and expand later, but do not make it native-only.
* Prefer a **shared low-privilege app user early**, with a roadmap toward **per-app users**.

These decisions preserve flexibility while establishing a clean security and operations model.

---

## 29. Final Assessment

This architecture is a strong fit for TheCube.

Using systemd as the supervisor and a custom launcher as the final sandboxing step gives you a clear, layered design that matches the platform goals already laid out in the broader TheCube documentation: a centralized core, modular applications, strong privacy/security posture, and a future developer ecosystem. fileciteturn0file2turn0file6turn0file7turn0file8

Most importantly, it avoids a common trap: turning the core service into an all-in-one process babysitter and security wrapper. Instead:

* systemd handles supervision well
* the launcher handles confinement well
* the core handles policy and APIs well

That separation should make the platform more maintainable, more secure, and easier to evolve as TheCube grows from a first-party product into an actual app platform.

---

## 30. Short Version

The intended plan is:

* **TheCube Core** decides whether an app may run and prepares an approved runtime policy.
* **systemd** launches and supervises the app instance.
* **The custom launcher** is called by systemd, applies Landlock and execution hygiene, then `exec()`s the resolved target whether that is a native binary or a runtime plus script.
* **Apps** use TheCube Core via IPC for sensitive and shared capabilities rather than touching hardware directly.
* **Python** should use controlled app-local `venv`s when needed, while **Node** should use fixed platform-managed runtimes rather than `nvm`.

That is the right overall direction for a secure, modular TheCube app ecosystem.
