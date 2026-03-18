# Core API

This document describes the device-local API exposed by `CubeCore`.

## Overview

The API has two transports:

* HTTP, bound to `HTTP_ADDRESS:HTTP_PORT`
* a local Unix socket, bound to `IPC_SOCKET_PATH`

Default values in code:

* HTTP address: `0.0.0.0`
* HTTP port: `55280`
* IPC socket path: `cube.sock`

There is no separate bootstrap port in the current implementation. Clients talk directly to the configured HTTP port or the configured Unix socket.

## Configuration

These values are loaded from `.env` through `Config::loadFromDotEnv()`:

* `HTTP_ADDRESS`
* `HTTP_PORT`
* `IPC_SOCKET_PATH`

Important detail:

* `.env` is loaded from the current working directory of the `CubeCore` process
* if `IPC_SOCKET_PATH` is relative, `/getCubeSocketPath` resolves it relative to the server process working directory

## Discovery Endpoints

These public endpoints are added directly by the API builder:

* `GET /` serves the main UI from `http/index.html`
* `GET /auth.js` serves the auth helper script
* `GET /getEndpoints` returns the currently registered API surface as JSON
* `GET /getCubeSocketPath` returns the resolved IPC socket path as JSON
* `GET /openapi.json` returns an OpenAPI-like description generated from registered endpoint metadata

For clients and docs, `/getEndpoints` and `/openapi.json` are the source of truth for the currently active interface set.

## Interface Endpoints

Interface endpoints are exposed with this path shape:

* `/InterfaceName-endpoint`

Examples:

* `/AudioManager-start`
* `/CubeAuth-authHeader`
* `/GUI-messageBox`

These endpoints are registered dynamically from `AutoRegisterAPI` interfaces during startup. That means the exact API surface depends on:

* which interfaces are compiled in
* which components are instantiated at runtime
* build flags such as Bluetooth-related options
* platform/runtime availability

## Security Model

The API server distinguishes between public and non-public endpoints.

Current behavior:

* public endpoints are available on the HTTP server and the IPC server
* non-public endpoints are intended to require auth on the HTTP side
* the IPC server is the intended local transport for trusted on-device clients and apps

This area is still evolving, so client code should not assume that every documented interface endpoint is safe to call unauthenticated over HTTP.

## Practical Usage

### HTTP

Example:

```bash
curl http://127.0.0.1:55280/getEndpoints
curl http://127.0.0.1:55280/openapi.json
```

If `HTTP_PORT` is overridden in `.env`, use that port instead.

### IPC

Use `/getCubeSocketPath` to discover the resolved socket path if you are not already controlling `IPC_SOCKET_PATH`.

## Notes

* Static files under `http/` are exposed as public GET endpoints.
* The endpoint list in this repo can drift as interfaces evolve, so this document intentionally does not try to mirror every single interface method manually.
* If you need the exact current contract for a running build, query `/getEndpoints` and `/openapi.json`.
