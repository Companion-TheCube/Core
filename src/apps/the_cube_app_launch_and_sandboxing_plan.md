# TheCube App Launch and Sandboxing Plan

## 1. Purpose

This document describes the app launch and sandboxing model that TheCube implements today in `AppsManager` and the standalone `CubeAppLauncher`, along with the intended near-term direction.

The current runtime chain is:

**CubeCore -> systemd -> CubeAppLauncher -> app**

Responsibilities are split this way:

* **CubeCore** discovers manifests, validates them, prepares runtimes, compiles launch policy, and requests app start/stop.
* **systemd** supervises app processes and owns lifecycle state.
* **CubeAppLauncher** reads the generated launch policy, prepares the execution environment, applies Landlock, and `execve()`s the real app.

This is not a speculative design anymore. The current implementation exists in:

* `src/apps/appsManager.cpp`
* the standalone `App-Launcher` project
* app manifests under install roots such as `build/bin/apps/<app-id>/manifest.json`

## 2. Current Runtime Model

### 2.1 Manifest discovery

`AppsManager` discovers app manifests from configured install roots.

Default behavior:

* non-root/dev mode: `<CubeCore executable directory>/apps`
* root/system mode: `/opt/thecube/apps`

Each app directory is expected to contain a `manifest.json`.

### 2.2 App registry

On initialization, `AppsManager`:

1. scans manifests
2. parses and validates them
3. upserts app metadata into the apps database
4. compiles launch policy when an app is started
5. starts enabled apps that are either `system_app: true` or `autostart: true`

### 2.3 Launch policy generation

CubeCore does not hand the raw manifest directly to the launcher. It compiles a resolved runtime policy at:

* system mode: `/run/thecube/launch/<app-id>/launch-policy.json`
* local/dev mode: `<CubeCore executable directory>/run/thecube/launch/<app-id>/launch-policy.json`

The local/dev path is the current default when CubeCore is running as a normal user without an explicit `THECUBE_LAUNCH_ROOT` override. This is intentional so local development does not require writing to the system `/run` tree.

That policy contains:

* resolved `argv`
* working directory
* resolved executable path
* generated environment
* resolved Landlock read-only/read-write paths
* app runtime directories to create before launch
* platform and network metadata

### 2.4 Launcher location

CubeCore resolves the launcher binary in this order:

1. `THECUBE_APP_LAUNCHER_BIN`
2. `CubeAppLauncher` found on `PATH`
3. `/opt/thecube/bin/CubeAppLauncher`

The launcher is now a separate application, not a Core target.

## 3. Supported Manifest Schema Today

The currently supported manifest shape is JSON with these top-level sections:

* `schema_version`
* `app`
* `runtime`
* `permissions`
* `environment`
* optional `ui`

### 3.1 `app`

Required or expected fields:

* `id`
* `name`
* `version`
* `description`
* `category`
* `working_directory`
* `type`
* `autostart`
* `system_app`
* optional `args`

Notes:

* `id` must match the current app ID pattern used by Core.
* `working_directory` must resolve inside the app install root.

### 3.2 `runtime`

Runtime types currently recognized by Core:

* `native`
* `python`
* `node`
* `docker`
* `web-bundle`

Current implementation status:

* `native`: supported
* `python`: supported
* `node`: supported structurally
* `docker`: supported
* `web-bundle`: recognized, but startup is not implemented yet

Runtime-specific fields:

* `runtime.native.entrypoint`
* `runtime.python.entry_script`
* `runtime.python.package_name` optional
* `runtime.node.entry_script`
* `runtime.docker.image`

Common lifecycle fields used today:

* `restart_policy`
* `max_restart_burst`
* `restart_window_seconds`
* `stop_timeout_seconds`
* `start_timeout_seconds`

These fields are part of the manifest contract, even where some of the systemd-side tuning is still evolving.

### 3.3 `permissions`

Current manifest validation requires:

* `permissions.filesystem.read_only`
* `permissions.filesystem.read_write`

Other supported sections:

* `permissions.platform`
* `permissions.network`

### 3.4 `environment`

Supported fields:

* `allow_inherit`: array of inherited variable names
* `set`: object of explicit environment variables

At launch time, Core also injects:

* `THECUBE_APP_ID`
* `THECUBE_DATA_DIR`
* `THECUBE_CACHE_DIR`
* `THECUBE_RUNTIME_DIR`
* `THECUBE_CORE_SOCKET`

## 4. Filesystem Permission Tokens

`AppsManager` currently resolves these manifest filesystem tokens:

### 4.1 App-scoped tokens

* `app://install`
* `app://assets`
* `app://data`
* `app://cache`
* `app://runtime`

These map to the app install root and the per-app runtime directories managed by Core.

### 4.2 Shared read-only token

* `shared://readonly/<relative-path>`

This maps to:

* `/usr/share/thecube/<relative-path>`

### 4.3 Host system token

* `system://<relative-host-path>`

Examples:

* `system://usr`
* `system://lib`
* `system://usr/share/zoneinfo`
* `system://tmp`
* `system://proc`

Rules:

* it must be host-relative, not absolute
* `.` and `..` components are rejected
* this is the mechanism for explicitly granting read-only or read-write access to host paths

This token exists specifically so apps must declare system path access in the manifest instead of getting broad implicit access from the launcher.

## 5. Landlock Behavior Today

The launcher reads `filesystem.landlock` from the generated policy and applies:

* `read_only`
* `read_write`
* `create`

Before Landlock is enabled, the launcher creates all directories listed in `create`.

### 5.1 Implicit launcher allowances

The launcher no longer auto-allows broad host paths such as `/usr`, `/lib`, `/etc`, `/proc`, or `/dev`.

It still implicitly allows a small set of launch-required paths:

* the resolved executable parent
* the resolved executable grandparent
* the resolved working directory

These are implementation details needed so the launcher can reliably start platform-managed runtimes such as app venv interpreters.

### 5.2 Practical consequence

If an app needs host files, the manifest must declare them explicitly through `system://...`.

Examples:

* Python runtime symlink target under `/usr/bin`
* shared libraries under `/lib` or `/lib64`
* timezone data under `/usr/share/zoneinfo`
* `/tmp` if the app deliberately uses a host tmp socket

## 6. Runtime Directories

CubeCore prepares per-app directories before launch:

* data: `/var/lib/thecube/apps/<app-id>/` or local/dev equivalent
* cache: `/var/cache/thecube/apps/<app-id>/` or local/dev equivalent
* runtime: `/run/thecube/apps/<app-id>/` or local/dev equivalent

In dev mode the local equivalents are rooted next to the running CubeCore executable, for example:

* `<CubeCore executable directory>/data/apps/<app-id>/`
* `<CubeCore executable directory>/cache/apps/<app-id>/`
* `<CubeCore executable directory>/run/thecube/apps/<app-id>/`

Apps should strongly prefer these directories over arbitrary host locations.

## 7. systemd Model

### 7.1 Current control path

Core currently shells out to:

* `systemctl` or `systemctl --user`
* `systemd-run` or `systemd-run --user`

That is an implementation choice, not a long-term architectural requirement. Moving lifecycle control to D-Bus later would be reasonable, but it is not how the current code works.

### 7.2 Unit naming

Apps run as transient units named:

* `thecube-app@<app-id>.service`

### 7.3 Startup behavior

`SystemdAppRuntimeController::startUnit()` currently:

1. inspects an existing transient unit, if present
2. recreates stale transient units when the launcher path or launch root changed
3. uses `systemctl start` when a matching unit already exists
4. falls back to `systemd-run` when a transient unit must be created

Important detail:

* stale unit replacement now clears old transient unit state with `stop` and `reset-failed`, then recreates it without `systemd-run --replace`

That behavior matters because `--replace` can cause failed transient units to disappear instead of remaining visible in `systemctl --user list-units --all`.

### 7.4 Restart behavior

The transient units created by Core currently use:

* `Restart=on-failure`
* `RestartSec=1`

systemd remains the source of truth for service state.

## 8. Runtime Support Notes

### 8.1 Python

Python apps currently use a managed venv workflow when `runtime.distribution` is `venv`.

Core prepares the app runtime under the app data directory and resolves the interpreter path into the launch policy.

Because many venv interpreters are symlinks to host Python binaries, Python apps often need explicit `system://usr`, `system://lib`, and `system://lib64` access in their manifests.

### 8.2 Node

Node runtime support is structured similarly, with the launch policy resolving the Node executable and entry script.

### 8.3 Docker

Docker apps are supported through `runtime.type: docker`.

For Docker apps:

* the launcher still handles policy loading and environment
* Landlock is skipped
* Docker volume host paths are resolved through the same manifest token mechanism as filesystem permissions

### 8.4 Web bundles

`web-bundle` is part of the manifest direction, but app startup is not implemented in `AppsManager` yet.

## 9. Example Manifest Guidance

When authoring manifests today:

* use `app://install`, `app://data`, `app://cache`, and `app://runtime` as the baseline
* add `shared://readonly/...` only for well-defined platform-owned shared content
* add `system://...` only when host access is genuinely required
* prefer `THECUBE_RUNTIME_DIR` over `/tmp`
* keep `allow_inherit` minimal
* assume Core will compile a stricter launch policy than the raw manifest representation

## 10. Current Gaps and Near-Term Work

The launch model is functional, but several items are still evolving:

* `web-bundle` execution is not implemented
* launcher policy file ownership and integrity checks can be stricter
* systemd hardening properties are still minimal compared with the long-term target
* host path grants should be reviewed app by app and narrowed where possible
* some first-party apps still need manifest cleanup so they use `THECUBE_RUNTIME_DIR` instead of `/tmp`
* the docs and examples should continue to track the actual supported manifest contract, not a future aspirational schema

## 11. Recommended Authoring Rules

If someone is writing a new app today, the practical rules are:

1. Put code and immutable assets under the app install root.
2. Use app-private data, cache, and runtime directories, not arbitrary host paths.
3. Declare every host path you need through `system://...`.
4. Expect CubeCore to generate `launch-policy.json` and treat that generated policy as the real launch contract.
5. Expect systemd to supervise the app and keep failure state visible through transient units.
6. Treat direct hardware access as exceptional; prefer Core-brokered APIs wherever possible.
