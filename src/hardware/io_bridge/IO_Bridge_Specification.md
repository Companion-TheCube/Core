# IO Bridge Specification

Status: draft system specification

Applies to: Companion CORE running on Raspberry Pi and the external IO Bridge implemented on an RP2354-class bridge MCU

Companion document: `src/hardware/io_bridge/ioBridge.md` defines the packet protocol in more detail. This document is the broader product and firmware specification for the IO Bridge as a subsystem.

## 1. Purpose

The IO Bridge exists to give developers expandable hardware IO without tying that expansion traffic directly to the Raspberry Pi's native peripherals.

The main system already consumes critical native buses:

- Raspberry Pi I2C is reserved for internal functions such as NFC and touchscreen connectivity.
- Raspberry Pi UART is reserved for internal hardware such as mmWave.
- Some Raspberry Pi GPIO will still be used directly by the main system where that is the correct architectural choice.

The bridge solves two related problems:

1. It exposes developer-facing IO such as SPI, I2C, UART, GPIO, and later custom PIO-backed interfaces.
2. It rate-isolates those interfaces so a slow external peripheral does not stall the rest of the system.

The key example is SPI. If the CORE had to speak directly to a slow external SPI target, the bus transaction itself would occupy the host-side path for the full transfer. By pushing downstream IO execution into an RP2354, the CORE can keep a single fast uplink to the bridge while the bridge clocks each external bus at whatever rate that peripheral requires.

## 2. Design Goals

### 2.1 Primary goals

- Present one uniform control plane from the CORE to all expansion interfaces.
- Decouple host uplink speed from downstream peripheral speed.
- Make slow developer SPI, I2C, and UART operations non-blocking from the CORE's perspective.
- Support future protocols without changing the physical host link by using RP2354 PIO.
- Support robust firmware updates and field recovery for the bridge itself.
- Provide a host API that Companion can consume instead of talking to Linux device nodes directly.

### 2.2 Secondary goals

- Allow deterministic event delivery from bridge to host.
- Preserve fairness so one noisy or slow endpoint does not starve others.
- Expose enough diagnostics to debug hardware, firmware, and external accessories.
- Support clean capability negotiation so different bridge revisions can be used safely.

### 2.3 Non-goals

- The bridge is not intended to replace every Raspberry Pi peripheral.
- The bridge is not intended to run arbitrary user applications.
- The first revision does not require encrypted transport on the internal CORE-to-bridge link, but the protocol should reserve space for it.

## 3. High-Level Architecture

### 3.1 Actors

- `CORE`: Raspberry Pi running Companion.
- `BRIDGE`: RP2354 connected to the CORE over SPI and responsible for downstream expansion IO.
- `EXPANSION DEVICE`: any developer-attached peripheral connected through bridge-managed ports.

### 3.2 Topology

The system uses one high-speed SPI link between CORE and BRIDGE. The CORE always speaks to the bridge at a fixed, fast rate negotiated during bring-up. The bridge then translates requests into per-port operations on downstream buses.

Conceptually:

`CORE SPI master -> BRIDGE SPI slave -> { developer SPI, I2C, UART, GPIO, PIO-defined protocols }`

### 3.3 Architectural consequences

- The host link remains fast even when a downstream target must run slowly.
- Queueing and fairness are enforced inside the bridge rather than relying on Linux device scheduling.
- The bridge becomes the single hardware abstraction layer for expansion ports.
- Protocol changes and new interfaces can be added in bridge firmware without redesigning the host transport.

## 4. Hardware Scope

### 4.1 CORE to BRIDGE link

Primary physical link:

- SPI mode 0
- CORE is master
- BRIDGE is slave

Required signals:

- `SCK`
- `MOSI`
- `MISO`
- `CS`

Current board constraint:

- The current bridge board revision does not expose dedicated `BRIDGE_IRQ`,
  `BRIDGE_RDY`, or `BRIDGE_RST` signals.

Software replacements for those functions:

- The CORE discovers readiness by sending `HELLO` at a conservative SPI rate
  until a valid `HELLO_RSP` is received.
- The CORE drains pending events and credit updates by scheduling periodic
  poll/drain SPI transactions when idle.
- Reset and reboot behavior are requested through control packets such as
  `CTRL.RESET` or `CTRL.REBOOT`; if the link is completely wedged, recovery is
  by full system power cycle rather than a dedicated bridge reset pin.

The SPI clock is expected to run in a high-speed range suitable for board layout and cable integrity. The exact supported range is advertised by the bridge during handshake.

### 4.2 Downstream bridge-managed interfaces

The bridge should expose the following logical interfaces at minimum:

- `GPIO`
- `I2C`
- `SPI proxy` for arbitrary developer SPI targets at configurable speed and mode
- `UART`
- `PIO loader / runtime-managed PIO services`
- `Firmware update / maintenance`

Optional later interfaces:

- `CAN` / `CAN-FD`
- One-wire or other custom serial buses implemented in PIO
- Protocol analyzers, soft peripherals, or timing-specialized link layers

### 4.3 Electrical expectations

- Main logic domain is 3.3 V.
- Inputs may be designed with 5 V tolerance only where explicitly supported by the chosen bridge hardware and board design. No software should assume universal 5 V tolerance.
- External headers should include ESD protection.
- Fast lines should use series damping resistors where signal integrity requires it.
- I2C pull-ups should live on the bridge board, not be assumed elsewhere.
- If the expansion ecosystem needs true 5 V logic-high signaling, dedicated level shifting must be present on the relevant ports.

## 5. Why an RP2354-Based Bridge

The RP2354 is appropriate because it provides:

- Dedicated hardware SPI, I2C, and UART blocks for common traffic
- PIO for custom or timing-sensitive interfaces
- DMA for high-throughput, low-jitter data movement
- Dual-core execution for separating time-critical link handling from downstream bus work
- XIP and flash features that support robust update and recovery strategies

The bridge is deliberately a small coprocessor, not a collection of fixed-function bridge chips, because the MCU approach gives the project one uniform software-defined IO surface. That matters more than shaving firmware complexity in the short term.

## 6. System Responsibilities

### 6.1 CORE responsibilities

- Discover bridge capabilities at boot
- Configure ports as needed by Companion subsystems or third-party developer features
- Submit requests and data to the bridge
- Perform scheduled poll/drain SPI cycles so pending events and credit updates
  are retrieved even when the host is otherwise idle
- Manage timeouts, retries, and policy decisions at the application level
- Route higher-level APIs such as Companion SPI / I2C / UART wrappers through the bridge instead of raw Linux devices where applicable

### 6.2 BRIDGE responsibilities

- Provide a reliable framed transport endpoint over SPI
- Maintain per-endpoint queues and fairness
- Execute downstream bus transactions at endpoint-specific speeds and modes
- Deliver unsolicited events back to the CORE
- Prevent invalid or unsafe use of reserved pins / ports
- Support firmware update, rollback, and recovery
- Expose diagnostics, counters, and fault state

## 7. Firmware Architecture

### 7.1 Execution model

Recommended split:

- Core 0: host link service, framing, DMA, IRQ handling, watchdog, flash-update path, global control plane
- Core 1: endpoint workers for SPI proxy, I2C, UART, GPIO event processing, PIO-managed services

This is a recommendation rather than a hard requirement, but the implementation should preserve the same separation of concerns even if task placement changes.

### 7.2 Internal subsystems

The bridge firmware should be divided into these modules:

- `link`: SPI slave service, frame RX/TX, CRC, sequencing, flow control
- `router`: maps frames to endpoint handlers
- `scheduler`: arbitrates endpoint work and prevents starvation
- `gpio`: pin mode, read/write, interrupt configuration, snapshots
- `i2c`: port config and segmented I2C transactions
- `spix`: developer-facing SPI proxy with independent downstream timing
- `uart`: port config, ring buffers, watermark events, status events
- `pio`: program loading, SM allocation, runtime ownership, custom transport support
- `fwup`: erase/write/verify/activate/reboot flows
- `mgmt`: hello, stats, time sync, logging, capabilities
- `ctrl`: reset, sleep, watchdog, safe-mode entry
- `diag`: loopback, tracing, internal counters

### 7.3 Scheduling model

The bridge must not run as a single blocking command loop. It should use:

- ring buffers for ingress and egress
- per-endpoint work queues
- DMA where available
- short ISR top halves
- explicit timeouts for slow buses

Priority order should favor:

1. Link health and frame handling
2. High-priority fault and watchdog events
3. UART RX preservation
4. Completion of outstanding downstream transactions
5. Background management and diagnostics

### 7.4 Rate isolation

Rate isolation is the core requirement of this subsystem.

Examples:

- A developer SPI target running at 125 kHz must not prevent GPIO events or UART RX from being reported.
- An I2C device that stretches the clock must not block unrelated UART activity.
- A bursty UART stream must not starve management traffic or make firmware update impossible.

The bridge achieves this through queueing, per-endpoint credits, DMA-backed buffering, and asynchronous completion events.

## 8. Dynamic PIO Support

PIO support is a first-class feature of the bridge, not just an implementation detail.

### 8.1 Purpose

PIO allows the bridge to add future interfaces without changing the host link or external API shape. It can be used for:

- custom SPI variants
- soft UARTs
- single-wire buses
- timing-critical experimental interfaces
- specialized accessory protocols

### 8.2 Required capabilities

The bridge firmware should support:

- loading a PIO program received from the CORE
- validating and staging the program before commit
- allocating instruction memory per PIO instance
- relocating instructions if required by the placement offset
- configuring state machines
- claiming and releasing pin ownership
- starting and stopping programs safely
- surfacing PIO IRQ and FIFO watermark events to the CORE

### 8.3 Safety rules for dynamic PIO

- PIO programs may not use pins reserved for critical system functions.
- The bridge must track pin ownership across GPIO, hardware peripheral, and PIO usage.
- A PIO program may be loaded while unrelated state machines continue running, but any state machine using the overwritten program region must be stopped first.
- Resource limits such as instruction count, state machine count, and allowed pin windows must be enforced by policy.

## 9. Boot, Update, and Recovery

### 9.1 Boot strategy

The bridge should boot through a minimal, reliable loader and then enter the main bridge application. The loader is responsible for selecting the active application slot, validating image metadata, and providing recovery entry points.

### 9.2 A/B update policy

Two layers of A/B are desirable:

- Internal loader A/B slots for the small trusted loader image
- Main program A/B slots for the larger bridge application image

Core policy:

- Write only to inactive slot
- Verify fully before activation
- Mark pending on reboot
- Require "boot good" confirmation after successful startup
- Roll back automatically on repeated failure

### 9.3 Update transport

The bridge must be updatable entirely over the CORE-to-bridge SPI link. USB or UART recovery paths are valuable for bring-up and dead-board recovery, but they are not required for normal field updates.

### 9.4 Firmware update operations

At minimum, the bridge should support:

- query flash layout
- erase region / slot
- write chunk
- verify digest
- set active slot
- commit manifest
- reboot into requested mode
- mark boot good

### 9.5 Recovery

The design should preserve last-resort recovery through bootloader / BOOTSEL mechanisms exposed by hardware. Software must also expose the reason for the most recent reboot to the CORE.

## 10. Protocol Overview

The transport between CORE and BRIDGE is called the CubeBridge Protocol, or `CBP`.

This document summarizes the protocol. The companion `ioBridge.md` contains the current packet-level reference.

### 10.1 Protocol goals

- one physical link, many logical endpoints
- request / response semantics
- unsolicited event delivery
- per-endpoint flow control
- chunking for large transfers
- version and capability negotiation

### 10.2 Physical transport assumptions

- SPI is the primary transport
- UART and I2C fallbacks may reuse the same frame structure for rescue or maintenance
- The CORE must actively clock event data out of the BRIDGE because the BRIDGE is SPI slave
- There is no dedicated bridge-to-host interrupt or ready line in the current
  board revision, so event draining and readiness detection are software-driven
  over SPI packets

### 10.3 Session bring-up

Expected sequence:

1. Reset or power-up
2. Send `HELLO` at a conservative SPI rate until the BRIDGE responds
3. Receive `HELLO_RSP`
4. Negotiate MTU, window size, endpoint counts, and capabilities
5. Optionally time-sync
6. Enter normal operation

## 11. Frame Structure

All integers are little-endian.

Header:

```text
+-----------+------+--------+-------+---------+------+-----+-----+--------+--------+
| Sync(2)   | Ver  | HdrLen | Flags | MsgType | EPID | Seq | Ack | Length | HdrCRC |
+-----------+------+--------+-------+---------+------+-----+-----+--------+--------+
```

Payload:

- variable length endpoint-specific data
- followed by payload CRC32 when payload length is non-zero

Required fields:

- `Sync`: fixed frame marker
- `Ver`: protocol version
- `HdrLen`: header size
- `Flags`: fragment, last-fragment, priority, reserved encryption, ack-only
- `MsgType`: `NOP`, `REQ`, `RSP`, `EVT`, optional explicit `ACK`
- `EPID`: endpoint id
- `Seq` / `Ack`: sliding window support
- `Length`: payload length
- `HdrCRC`: header CRC

### 11.1 Reliability model

- Sliding window with small bounded outstanding depth
- Piggyback acknowledgements
- Retransmission on timeout
- CRC validation on header and payload
- Safe-mode downgrade if framing errors become excessive

### 11.2 Flow control

The bridge advertises credits per endpoint. The CORE must not overrun those credits. Credit replenishment is returned either in flow-control events or piggybacked in normal traffic.

### 11.3 Fragmentation

Large objects such as firmware images and PIO programs must support fragmentation across multiple frames. Reassembly should be tied to endpoint transaction ids rather than implicit transport ordering alone.

## 12. Endpoint Map

Recommended logical endpoint identifiers:

- `MGMT` (`0x00`): management, hello, stats, logs, time sync
- `FWUP` (`0x01`): firmware update and slot management
- `PIO` (`0x02`): PIO program load, state machine control, FIFO access
- `GPIO` (`0x10`): mode, read, write, IRQ config, snapshots
- `I2C` (`0x20`): I2C transfers and configuration
- `SPIX` (`0x21`): downstream developer SPI proxy
- `UART` (`0x22`): UART configuration and byte streams
- `CAN` (`0x23`): optional CAN / CAN-FD transport
- `INT` (`0x30`): consolidated async event bucket
- `DBG` (`0xFE`): diagnostics, loopback, internal peeks if allowed
- `CTRL` (`0xFF`): reset, sleep, watchdog, safe-mode control

Each endpoint payload should begin with a small endpoint header:

```c
struct EPHeader {
    uint8_t  op;
    uint8_t  port;
    uint16_t txn;
};
```

This keeps per-endpoint request parsing regular across the whole bridge.

## 13. Endpoint Specifications

### 13.1 MGMT

Required operations:

- `HELLO`
- `HELLO_RSP`
- `GET_STATS`
- `TIME_SYNC`
- `LOG_SUBSCRIBE`
- `SET_PARAM` for negotiated runtime-safe parameters

Required data exposed by `HELLO_RSP`:

- protocol version
- bridge firmware version
- silicon / board identifier
- supported SPI frequency range
- negotiated MTU
- queue depths / credits
- endpoint counts
- feature flags such as PIO support, watchdog support, flash layout support, optional CAN support

### 13.2 GPIO

Required operations:

- configure mode
- configure pulls / drive
- write
- read
- snapshot
- interrupt configuration

Required event support:

- pin interrupt event with timestamp
- optional batching to reduce overhead

### 13.3 I2C

Required behavior:

- multiple logical ports if supported by hardware or PIO
- 7-bit and optional 10-bit addressing
- write, read, and combined transfer support
- repeated start support
- explicit timeout control
- clear status reporting for NACK, arbitration loss, timeout, and bus-stuck conditions

The bridge should report completion either inline in the response or asynchronously for queued operations.

### 13.4 SPIX

`SPIX` is the bridge-managed developer SPI, separate from the host uplink SPI.

Required operations:

- open / reserve a device profile
- configure mode, bits-per-word, clock rate, chip-select polarity, setup / hold
- transfer full-duplex or asymmetric TX / RX length
- close / release

Key property:

- A slow `SPIX` transaction must not stall unrelated bridge traffic because the downstream transaction happens on the bridge side, not directly on the host link.

### 13.5 UART

Required behavior:

- open with baud, parity, stop bits, flow control, inversion options where supported
- write bytes
- read bytes on demand
- subscribe to RX watermark events
- expose status events such as overrun, framing error, break, and modem-control changes

The bridge should keep ring buffers per UART port and let the CORE tune watermarks based on latency versus overhead tradeoffs.

### 13.6 CAN

Optional for early revisions, but the protocol should reserve a stable endpoint for it.

Required operations if implemented:

- open with bit timing
- add / remove filters
- transmit frame
- receive events
- bus state and error reporting

### 13.7 PIO

Required operations:

- query capabilities
- load program
- configure state machine
- start
- stop
- push / pull FIFOs
- execute one-off instruction if needed

Required event support:

- PIO IRQ
- optional FIFO watermark events

### 13.8 FWUP

Required operations:

- query layout
- erase
- write
- verify
- set active slot
- commit
- reboot
- mark boot good

### 13.9 CTRL and DBG

`CTRL` should cover actions such as reset, sleep, watchdog pet, watchdog configuration, safe-mode entry, and controlled reboot modes.

`DBG` should be restricted and used for bring-up, loopback tests, or controlled diagnostics. It should not become a permanent escape hatch that bypasses the rest of the architecture.

## 14. Event Model

The bridge must support unsolicited events from BRIDGE to CORE.

Typical events:

- GPIO interrupt
- UART RX watermark reached
- UART status change
- I2C transaction complete
- SPIX transaction complete
- PIO IRQ
- flow-control credit update
- watchdog warning
- fault or brownout report
- boot reason

Event requirements:

- timestamped
- routable to the correct endpoint / port
- drainable during normal traffic or explicit host-initiated poll/drain cycles
- safe to batch when multiple events are pending

## 15. Error Model

Every endpoint response should begin with a small common status structure:

```c
struct Status {
    int16_t  code;
    uint16_t detail;
};
```

Common status codes should include:

- `OK`
- `TIMEOUT`
- `BUSY`
- `NACK`
- `PARAM`
- `CRC`
- `PERM`
- `UNSUP`
- `NO_MEM`
- `IO_ERR`
- `STATE`

The `detail` field is endpoint-specific and should carry the precise hardware or policy reason where useful.

## 16. Host-Side Software Integration

The bridge is only useful if Companion can consume it cleanly.

### 16.1 Expected integration direction

The existing host-side abstractions in `src/hardware/io_bridge/` should evolve toward bridge-backed implementations:

- `SPI` should stop talking directly to Linux `spidev` for expansion-port devices and instead send `SPIX` operations to the bridge.
- `I2C`, `UART`, and `GPIO` wrappers should follow the same pattern where those operations are intended for bridge-managed ports.
- Higher-level code should not need to know whether an operation is served by a native Raspberry Pi peripheral or by the bridge, except where explicitly required by system policy.

### 16.2 Interface boundary

Recommended host library responsibilities:

- session bring-up and capability negotiation
- frame encode / decode
- request / response matching
- retry policy
- event draining
- endpoint-specific convenience APIs
- translation between Companion data types and CBP payloads

### 16.3 Event delivery into Companion

The host bridge client should present asynchronous events in a way that Companion can consume safely, for example via callbacks, queues, or event dispatch objects. Event handling must not depend on ad hoc polling buried inside unrelated device classes.

## 17. Safety, Security, and Policy

### 17.1 Safety policy

- Reserved pins and ports must be blocked by firmware policy.
- Bridge code must validate all lengths, counts, rates, and pin ranges before touching hardware.
- Timeouts must be enforced on all external-bus transactions.
- Safe-mode should activate after repeated framing or integrity failures.

### 17.2 Security policy

Near-term:

- signed firmware images are strongly recommended
- slot metadata should include version, size, digest, and rollback counter
- the bridge should refuse obviously invalid or downgraded images

Future-ready hooks:

- encrypted transport flag in protocol
- authenticated key exchange for link-level encryption
- OTP-stored trust anchors or key fingerprints

## 18. Performance Targets

These are design targets, not final measured guarantees:

- Small SPI request / response round-trip on the host link should be on the order of tens to a few hundreds of microseconds once the system is up.
- Event production on the bridge should be low-latency enough that GPIO, UART, and completion events are practical under normal load.
- UART buffering should tolerate short bursts without dropping data.
- Slow `SPIX` or I2C operations should not materially disturb unrelated endpoint responsiveness.

## 19. Testing Requirements

The bridge should be validated with both unit-level and hardware-in-the-loop tests.

Minimum coverage:

- frame parser fuzzing
- CRC failure handling
- sliding-window retransmission behavior
- event drain behavior under host polling load and during active traffic
- simultaneous slow `SPIX` plus active UART RX
- I2C clock-stretch and timeout cases
- GPIO interrupt batching
- PIO load / start / stop and invalid-program rejection
- firmware update with interrupted power or reset
- slot rollback after failed boot

## 20. Implementation Order

Recommended delivery sequence:

1. Link layer and framing
2. `MGMT` handshake and stats
3. `GPIO`
4. `UART`
5. `I2C`
6. `SPIX`
7. `PIO`
8. `FWUP`
9. Optional `CAN`

This order gives the project a debuggable transport first, then simple observable endpoints, then the more timing-sensitive buses.

## 21. Open Design Decisions

These points still need concrete project decisions even though the architecture direction is clear:

- Final count and pinout of developer-facing ports
- Exact electrical protections and level-shifting policy on expansion headers
- Whether the first bridge revision exposes CAN in hardware or only reserves protocol space
- The exact loader / application flash map
- Whether bridge updates are implemented in onsrc/hardware/io_bridge/ioBridge.mde combined image first, then split into loader and app later, or designed as A/B from day one
- The exact host-side API surface inside Companion

## 22. Summary

The IO Bridge is a dedicated RP2354-based expansion coprocessor that turns one fast SPI uplink from the Raspberry Pi into a multiplexed, non-blocking, firmware-defined hardware expansion subsystem. Its job is not simply to "add ports"; it is to isolate timing domains, centralize peripheral policy, provide a stable transport for expansion features, and create a path for future interfaces through PIO.

The most important rule for every implementation decision is this: the CORE should always experience the bridge as a fast, reliable message endpoint even when the external world behind the bridge is slow, noisy, or experimental.
