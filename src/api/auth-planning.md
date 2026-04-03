# API Authentication and Authorization Plan

This document is a planning artifact for the authentication and authorization
model around `CubeCore`.

It is intentionally more forward-looking than
[`readme.md`](./readme.md). If the two docs ever disagree, treat
`readme.md` as the current implementation document and this file as the target
design and rollout plan.

## Goals

The auth system should support three access modes:

1. Network clients and remote apps call CORE over HTTP after completing the
   existing code-based approval flow.
2. Local apps running on TheCube call CORE over the IPC socket without using
   the code-based approval flow.
3. The web UI calls CORE over HTTP after completing a dedicated `WebAuth`
   username/password login flow.

The long-term goal is to have different authentication entrypoints per access
mode but one consistent authorization layer deciding what each caller is allowed
to do.

## Current State Summary

Current implementation status:

* HTTP non-public endpoints use the existing bearer-token flow.
* The code-based approval flow is implemented through `CubeAuth`.
* Static web files under `http/` are still public.
* The current developer portal drives the existing `CubeAuth` flow in the
  browser.
* The dashboard is still mostly placeholder and mock-backed.
* IPC endpoints are still effectively trusted-local and do not yet enforce
  manifest-driven permissions.
* App manifests already contain permission data, but CORE does not yet persist
  endpoint permissions in a way that can be enforced at request time.

## Target Architecture

### 1. Network Client Auth

This remains the existing `CubeAuth` flow:

1. client requests an approval code with `GET /CubeAuth-initCode`
2. user approves the request on-device
3. client exchanges the approved code at `GET /CubeAuth-authHeader`
4. client calls non-public HTTP endpoints with `Authorization: Bearer <token>`

This path is intended for:

* external devices on the network
* remote apps not co-located with CORE
* debugging and automation flows that intentionally operate over HTTP

### 2. Local App Auth Over IPC

This path is intended for apps installed on TheCube and launched next to CORE.

Desired behavior:

* local apps use the IPC socket rather than the network HTTP listener
* local apps do not use the code-based approval flow
* local apps authenticate as an app principal
* local apps are authorized based on their manifest-declared permissions plus
  any user overrides stored in CORE

This path needs two major pieces:

* app identity at request time [EDIT] The 
* app authorization at endpoint dispatch time

### 3. Web UI Auth

This path is intended for browser access to the dashboard and developer portal.

Desired behavior:

* the web UI uses a dedicated `WebAuth` login flow
* login is based on username and password
* successful login issues a short-lived web session token
* the web session token is checked by the same HTTP-side authorization layer
  that currently checks bearer tokens
* the preferred transport for the web session token is an `HttpOnly` secure
  cookie, even if the internal auth layer treats it as a token

This path should replace the current need for browser users to run the
code-based developer auth flow just to use the web UI.

## Shared Authorization Model

The auth system should resolve every incoming request to a principal and then
decide whether that principal can access the requested endpoint.

Suggested principal types:

* `network_client`
* `local_app`
* `web_user`
* `internal_system`

Suggested authorization inputs:

* principal type
* principal id
* transport: HTTP or IPC
* endpoint id, for example `GUI-messageBox`
* method, for example `GET` or `POST`

Suggested rule:

* allow public endpoints unless explicitly restricted later
* for protected endpoints, require a resolved principal and an authorization
  decision

## App Permission Model

### Requested Permissions

App manifests should declare the permissions an app wants. This already exists
for filesystem, platform, network, and PostgreSQL access.

A new manifest section should be added for API access. Suggested shape:

```json
{
  "permissions": {
    "api": {
      "endpoints": [
        "GUI-messageBox",
        "CubeDB-saveBlob"
      ]
    }
  }
}
```

The exact manifest shape is still open, but the important point is that app API
access should be declared explicitly and in a stable way.

### Stored Permissions

During install or manifest sync, requested app permissions should be saved to
the database by the app management code.

The stored model should distinguish between:

* permissions requested by the manifest
* permissions granted by the system or user
* effective permissions after combining those two inputs

Recommended rule:

* an app may only call an endpoint if the manifest requested it and CORE has
  granted it

This gives the future permissions UI a clean model:

* the manifest states what the app asked for
* the user decides what is actually granted

### Suggested Database Shape

Exact schema is still open, but something in this shape is expected:

* `app_requested_permissions`
* `app_permission_grants`
* optional derived or cached effective permissions table or view

At minimum, the stored data needs to answer:

* which app requested which endpoint permission
* whether that permission is currently granted, denied, or revoked
* when the decision changed

## App Identity Over IPC

Manifest-driven authorization only works if CORE can identify which app made a
request over IPC.

That identity mechanism is not settled yet. Options include:

* launcher-injected app credential sent with each IPC request
* per-app IPC socket or per-app proxy process
* OS-level peer credential inspection combined with launch metadata
* short-lived app session token minted at app startup

Whichever approach is chosen, it needs to satisfy all of these:

* CORE can reliably map a request to a single installed app id
* the app cannot spoof another app id
* revocation is possible without reinstalling the app
* the implementation works for all supported app runtime types

## WebAuth Design Expectations

`WebAuth` should be separate from `CubeAuth`.

Recommended behavior:

* store password hashes, not plaintext passwords
* create short-lived web sessions
* support logout and session expiration
* integrate with the same authorization layer used for bearer tokens
* allow the web UI to call HTTP endpoints without reusing the device approval
  flow

The web session token should be short-lived and renewable. If the browser-only
use case is dominant, prefer an `HttpOnly` cookie to exposing a long-lived token
to page JavaScript.

## Rollout Plan

### Phase 1: Document and Normalize the Model

* clarify the distinction between current behavior and intended behavior
* define the principal model and endpoint permission vocabulary
* define the manifest shape for API endpoint permissions

### Phase 2: Persist Requested App Permissions

* extend app manifest parsing and sync so requested API permissions are stored
  in the database
* decide where this data lives and how manifest updates reconcile with prior
  user grants

### Phase 3: Build Shared Authorization Layer

* introduce a shared authorization path for HTTP and IPC
* resolve request principal first
* authorize endpoint access second
* stop treating IPC as implicitly trusted

### Phase 4: Implement WebAuth

* add username/password login
* add short-lived web session issuance
* hook web sessions into the shared HTTP authorization path
* gate the web UI pages behind the new web auth flow

### Phase 5: Enforce App Permissions

* add app identity to IPC requests
* enforce manifest and grant-based endpoint access for local apps
* return clear authorization failures for denied endpoint calls

### Phase 6: Build User-Facing Permissions Management

* add a UI entry for per-app permission review
* show requested permissions from the manifest
* allow endpoint-level grant, deny, and revoke actions

## Open Questions

These need concrete answers before implementation is complete:

* What is the exact manifest schema for app API endpoint permissions?
* Are endpoint permissions granted per endpoint id only, or per endpoint id plus
  HTTP method?
* Should there also be permission groups, or only explicit endpoint entries?
* What is the exact app identity mechanism for IPC requests?
* Should IPC auth use launcher-issued tokens, peer credentials, or something
  else?
* Where should requested and granted app permissions be stored: `auth.db`,
  `apps.db`, or a new dedicated database/table set?
* What should happen when a manifest changes and an app requests new
  permissions?
* Should system apps receive default grants that third-party apps do not?
* What is the bootstrap path for creating the first web user and rotating that
  password?
* Should the web UI rely only on secure cookies, or should it also be able to
  request a header-style token explicitly?
* Do web users need roles such as admin vs read-only, or is there only one web
  user class initially?

## Implementation Note

One concrete requirement that should be treated as part of app installation and
manifest sync:

* installation of an app must persist the manifest-declared permissions to the
  database so those permissions can later be surfaced in the UI and enforced by
  CORE
