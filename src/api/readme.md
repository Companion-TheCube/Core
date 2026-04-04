# Core API

This document describes the API exposed by `CubeCore`, the currently implemented
authentication behavior, and the intended access model that the auth work is
moving toward.

For the forward-looking auth and authorization plan, see
[`auth-planning.md`](./auth-planning.md).

## Overview

The API currently has two transports:

* HTTP, bound to `HTTP_ADDRESS:HTTP_PORT`
* a local Unix socket, bound to `IPC_SOCKET_PATH`

Default values in code:

* HTTP address: `0.0.0.0`
* HTTP port: `55280`
* IPC socket path: `cube.sock`

There is no separate bootstrap port in the current implementation. Clients talk
directly to the configured HTTP port or the configured Unix socket.

## Intended Access Modes

The auth model is intended to support three distinct ways of talking to CORE:

1. Network clients and remote apps use the existing code-based approval flow to
   obtain a bearer token and then call HTTP endpoints over the network.
2. Apps running locally on TheCube, adjacent to CORE, use the IPC socket and
   are authorized through manifest-driven app permissions rather than the
   code-based approval flow.
3. The web UI uses a username/password based `WebAuth` flow and receives a
   short-lived web session token so it can call CORE endpoints over the network.

Only the first of those is fully implemented today. The second and third are
planned and are documented here so the intended system behavior is clear.

## Configuration

These values are loaded from `.env` through `Config::loadFromDotEnv()`:

* `HTTP_ADDRESS`
* `HTTP_PORT`
* `IPC_SOCKET_PATH`
* `AUTH_ALLOW_RETURN_CODE`
* `AUTH_AUTO_APPROVE_REQUESTS`

Important details:

* `.env` is loaded from the current working directory of the `CubeCore` process
* if `IPC_SOCKET_PATH` is relative, `/getCubeSocketPath` resolves it relative
  to the server process working directory
* `AUTH_ALLOW_RETURN_CODE` defaults to disabled; when enabled and
  `return_code` is requested, `/CubeAuth-initCode` includes the one-time code in
  the response for local dev/test
* `AUTH_AUTO_APPROVE_REQUESTS` defaults to disabled; when enabled, auth
  requests are auto-approved for automated tests and local harnesses

## Discovery Endpoints

These public endpoints are added directly by the API builder:

* `GET /` serves the main UI from `http/index.html`
* `GET /auth.js` serves the auth helper script used by the current developer
  portal flow
* `GET /getEndpoints` returns the currently registered API surface as JSON
* `GET /getCubeSocketPath` returns the resolved IPC socket path as JSON
* `GET /openapi.json` returns an OpenAPI-like description generated from
  registered endpoint metadata

For clients and docs, `/getEndpoints` and `/openapi.json` are the source of
truth for the currently active interface set.

## Interface Endpoints

Interface endpoints are exposed with this path shape:

* `/InterfaceName-endpoint`

Examples:

* `/AudioManager-start`
* `/CubeAuth-authHeader`
* `/GUI-messageBox`

These endpoints are registered dynamically from `AutoRegisterAPI` interfaces
during startup. That means the exact API surface depends on:

* which interfaces are compiled in
* which components are instantiated at runtime
* build flags such as Bluetooth-related options
* platform and runtime availability

## Current Security Behavior

The API server distinguishes between public and non-public endpoints, but the
behavior is not yet uniform across transports.

### HTTP

Current HTTP behavior:

* public endpoints are available without auth
* non-public endpoints are intended to require `Authorization: Bearer <token>`
* `GET /CubeAuth-initCode` creates a short-lived pending approval request and
  triggers device approval UI
* `GET /CubeAuth-authHeader` only returns a bearer token after the exact
  `client_id` and one-time code have been approved
* unapproved, denied, expired, reused, or mismatched approval requests return
  structured `403` JSON errors such as `approval_pending` or
  `approval_expired`
* requesting `return_code=true` does not expose the one-time code unless
  `AUTH_ALLOW_RETURN_CODE` is explicitly enabled

### IPC

Current IPC behavior:

* the IPC server is intended to be the local transport for trusted on-device
  clients and apps
* every IPC request must include `X-TheCube-App-Auth-Id`
* the header value must be the runtime `THECUBE_APP_AUTH_ID`, not manifest
  `app.id`
* requests with missing, empty, stale, or unknown app auth IDs are rejected
  with `403`
* public IPC endpoints are allowed once the app identity resolves to an enabled
  installed app
* non-public IPC endpoints additionally require an explicit grant in the
  `authorized_endpoints` table in `apps.db`

### Static Web Content

Current static web behavior:

* static files under `http/` are exposed as public GET endpoints
* the current developer portal is a public page that runs the existing
  code-based auth bootstrap in the browser
* the current dashboard UI is still mostly placeholder and mock-backed
* `WebAuth` for the web UI is planned, but is not yet implemented

Because this area is still evolving, client code should not assume that every
documented interface endpoint is safe to call unauthenticated over HTTP, and it
should treat IPC access as requiring both app identity and, for non-public
routes, an explicit endpoint grant.

## Current Network Auth Flow

The currently implemented HTTP bootstrap is a two-step, approval-gated flow:

1. `GET /CubeAuth-initCode?client_id=...`
2. approve the request on the device
3. `GET /CubeAuth-authHeader?client_id=...&initial_code=...`

Notes:

* approval requests are stored internally in a dedicated auth-request table and
  expire after about 60 seconds
* the bearer token format returned by `/CubeAuth-authHeader` is unchanged
* direct code return and auto-approval are intended only for local developer
  workflows and automated tests

## Intended Direction

The auth work is moving toward the following model:

* network clients continue using the existing code-based approval flow and then
  call HTTP endpoints with a bearer token
* local on-device apps use the IPC socket and are authorized based on manifest
  permissions plus user-managed per-app grants
* app installation or manifest sync persists requested app permissions into the
  database so CORE can enforce them later
* the web UI uses a dedicated `WebAuth` login flow and receives a short-lived
  web session token, preferably transported in a secure cookie, that is checked
  by the same HTTP-side authorization layer

This means the final system will have separate authentication entrypoints for
network clients, local apps, and browser users, but a shared authorization layer
for deciding whether a principal may call a given CORE endpoint.

## Practical Usage

### HTTP

Example:

```bash
curl http://127.0.0.1:55280/getEndpoints
curl http://127.0.0.1:55280/openapi.json
```

If `HTTP_PORT` is overridden in `.env`, use that port instead.

### IPC

Use `/getCubeSocketPath` to discover the resolved socket path if you are not
already controlling `IPC_SOCKET_PATH`.

IPC requests from local apps must include:

* `X-TheCube-App-Auth-Id: <THECUBE_APP_AUTH_ID>`

Endpoint grants in `authorized_endpoints.endpoint_name` use the full endpoint
ID, for example `GUI-messageBox`.

Example:

```bash
curl --unix-socket /tmp/thecube/cube.sock \
  -H 'X-TheCube-App-Auth-Id: abc123runtimeauthid' \
  http://localhost/getEndpoints
```

## Notes

* Static files under `http/` are currently exposed as public GET endpoints.
* The endpoint list in this repo can drift as interfaces evolve, so this
  document intentionally does not try to mirror every single interface method
  manually.
* If you need the exact current contract for a running build, query
  `/getEndpoints` and `/openapi.json`.
