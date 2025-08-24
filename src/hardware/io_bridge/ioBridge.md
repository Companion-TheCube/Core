# **CubeBridge Protocol (CBP) v0.1**

Messaging between CORE (Raspberry Pi) ⇄ Bridge (RP2354)

Below is a complete, implementation‑ready protocol for moving commands, data, and events across SPI (primary) and UART/I²C (fallback) between your CORE and the RP2354 “expansion port manager.” It covers framing, multiplexing, flow control, error handling, firmware \+ PIO program delivery, GPIO and bus control, and interrupt/event propagation.

---

## **0\) Design goals (why this shape)**

* **One transport, many “virtual endpoints.”** SPI carries a multiplexed, reliable byte stream; endpoints expose I²C, SPI-proxy, UARTs, CAN, GPIO, PIO loader, FW update, etc.

* **Request/Response \+ Events.** Synchronous command/reply plus unsolicited async events (interrupts, RX watermarks, fault reports).

* **Backpressure & latency control.** Credit-based per-endpoint flow \+ small sliding window so a slow endpoint (e.g., I²C @ 10 kHz) can’t starve others.

* **Chunked large transfers.** MTU-negotiated fragmentation for PIO/FW blobs.

* **Version & capability negotiation.** So you can extend later without breaking older loaders.

---

## **1\) Physical/link layer**

**Primary transport:** SPI mode 0, CORE \= master, BRIDGE \= slave.  
 **Recommended pins:** CS, SCK, MOSI, MISO, plus two sideband GPIOs:

* `BRIDGE_IRQ` (bridge→core): asserted when bridge has events or RX credits available.

* `BRIDGE_RDY` (bridge→core): bridge ready after reset/boot; also drops low during deep sleep.

**Clocking:** CORE may change SCK at runtime; min/max advertised by bridge during handshake.

**Move events from slave?** SPI is master‑clocked, so CORE must clock data out. Strategy:

* When `BRIDGE_IRQ=1`, CORE performs a **drain cycle** (send NOP frame; receive event frames).

* During normal traffic, events can piggyback in the RX direction of any transaction.

**Fallbacks:** Same frame format can run over UART (rescue console) and I²C (low-speed maintenance). Set unique **Link IDs** in the header so both sides know which physical link they’re on.

---

## **2\) Frame format (common to SPI/UART/I²C)**

All integers little‑endian.

```
+-----------+-----------+-----------+-----------+--------------------+-----------+-----------+-----------+-----------+-----------+-------+  
|  Sync(2)  |  Ver(1)   |  HdrLen   |  Flags    |   MsgType          |   EPID    |   Seq     |   Ack     |   Length  |   HdrCRC  |  ...  |  
|  0xA5,5A  |  0x01     |  0x10     |  bitfield |   REQ/RSP/EVT/NOP  | endpoint  |   u8      |   u8      |   u16     |   u8      |       |  
+-----------+-----------+-----------+-----------+--------------------+-----------+-----------+-----------+-----------+-----------+-------+  
| Payload (Length bytes, may include TLVs)                                                                                               |  
+----------------------------------------------------------------------------------------------------------------------------------------+  
| CRC32 (payload only)                                                                                                                   |  
+----------------------------------------------------------------------------------------------------------------------------------------+
```

* **Ver:** protocol version (start at 1).

* **HdrLen:** bytes from Ver..HdrCRC inclusive (default 0x10).

* **Flags (u8):**

  * b0: `FRAG` (this is a fragment)

  * b1: `LAST` (last fragment in message)

  * b2: `PRIO` (high priority—e.g., safety/fault)

  * b3: `ENCRYPT` (reserved; default off)

  * b4: `ACK_ONLY` (header-only ack/keepalive)

* **MsgType (u8):** 0=NOP, 1=REQ, 2=RSP, 3=EVT, 4=ACK (explicit, rare)

* **EPID (u8):** virtual endpoint id (see §4).

* **Seq/Ack (u8):** sliding window (0–15 recommended).

* **Length (u16):** payload length (0..MTU).

* **HdrCRC (u8):** CRC-8 over Ver..Length (poly 0x07).

* **CRC32:** IEEE 802.3 over payload only (skip when Length=0).

**MTU:** negotiate at handshake; default 512 bytes SPI, 256 UART, 128 I²C.

**Fragmentation:** If payload \> MTU, split across frames with `FRAG` set; last fragment has `LAST`.

---

## **3\) Session bring‑up**

1. **RESET** (hardware or soft).

2. `HELLO` (EPID=MGMT, MsgType=REQ): CORE→BRIDGE with desired features:

   * max SPI Hz, desired MTU, window size, supported endpoints.

3. `HELLO_RSP` (RSP): BRIDGE returns:

   * allowed SPI Hz range, actual MTU, window, per‑endpoint queue depths, silicon id, FW version, capabilities bitmask (I²C count, UART count, CAN present, PIO count, XIP slots, watchdog present, etc.).

4. **OPTIONAL**: `TIME_SYNC` (monotonic us).

5. CORE switches SPI clk to agreed rate and begins normal operations.

---

## **4\) Endpoint map (EPID)**

* 0x00 `MGMT` — management, logging, stats, time sync.

* 0x01 `FWUP` — firmware & loader update into internal/external flash.

* 0x02 `PIO` — PIO program/image load & state machine control.

* 0x10 `GPIO` — pin mode, read/write, IRQ config, bulk snaps.

* 0x20 `I2C` — logical I²C controller (multi‑port via PortId field).

* 0x21 `SPIX` — **SPI proxy** (the developer SPI bridged at arbitrary speed).

* 0x22 `UART` — byte stream w/ watermarks, modem lines.

* 0x23 `CAN` — CAN/CAN‑FD frame IO & filters.

* 0x30 `INT` — async interrupt/events bucket (BRIDGE→CORE).

* 0xFE `DBG` — diagnostics, loopback, register peek/poke.

* 0xFF `CTRL` — hard reset, sleep, watchdog pet/arm, safe‑mode.

Each endpoint payload begins with a **common mini‑header** to keep things regular:

```cpp
struct EPHeader {
    uint8_t  op;       // operation code (enum per EP)`  
    uint8_t  port;     // physical instance (e.g., I2C0=0, I2C1=1, UART2=2); 0xFF = N/A`  
    uint16_t txn;      // transaction id for matching multi-frag ops inside a Seq/Ack exchange`  
    // followed by op-specific fields / TLVs`  
};
```

---

## **5\) Flow control & reliability**

* **Sliding window:** up to W outstanding frames (W negotiated; default 4).

* **Acking:** piggyback `Ack` field acknowledges last good Seq from peer. Timeout → retransmit unacked frames.

* **Per-endpoint credits:** Bridge advertises RX credits per EP in `HELLO_RSP`; CORE tracks and won’t exceed. Bridge replenishes via `EVT:FLOW` when buffers drain (or piggyback credit counts in regular EVTs).

* **Priority:** Frames with `PRIO` bit get transmitted first; reserve for INT/fault, watermarks, watchdog pings.

---

## **6\) Management (EPID=MGMT)**

**Ops:**

* `HELLO` / `HELLO_RSP` (capabilities & negotiation)

* `GET_STATS` → TX/RX counts, CRC errors, overruns, per‑EP queue fill, max latencies

* `LOG_SUBSCRIBE` (levels, modules) → EVT:LOG lines (ring‑buffered)

* `TIME_SYNC` set/get

* `SET_PARAM` (e.g., change SPI divisor live if allowed)

---

## **7\) GPIO (EPID=GPIO)**

**Ops:**

* `CFG` (pin, mode: IN/OUT/ALT, pull, drive)

* `WRITE` (mask,value) and `READ` (mask)

* `IRQ_CFG` (pin, edge: rising/falling/both/level, debounce, evt-rate-limit)

* **Events:** `EVT:GPIO` with `(pin, state, ts_us)` batched.

* `SNAPSHOT` (return all pins bitmap \+ ts) for fast sync.

---

## **8\) I²C (EPID=I2C)**

**Ops:** `XFER` supports batched transactions (write/read segments) with 7/10‑bit addr, `stop` control, and timeout. Optional `retries`.  
 **Clock:** per‑port config via `CFG` (100k/400k/1M or divisor).  
 **Events:** `EVT:I2C_DONE` (txn, status) for async mode; otherwise RSP carries result.  
 **Errors:** NACK, arbitration lost, timeout, bus stuck (with bus‑recovery attempt flag).

Payload sketch:
```
op=XFER, port=I2C1
[ addr7, flags (RD/WR/STOP/REPEATED_START), seg_count, seg0_len, seg0_data..., seg1_len, ... ]
```
---

## **9\) SPI proxy (EPID=SPIX)**

This is the **developer SPI** the bridge exposes downstream (separate from the CORE↔BRIDGE SPI).

**Ops:**

* `OPEN` (mode 0/1/2/3, bits=8/16/32, **clk\_hz** exact or divisor, cs\_polarity, cs\_hold/setup in ns)

* `XFER` (full‑duplex; optional separate tx/rx lengths; repeated `CS` hold across segments)

* `CLOSE`

* `CFG` (change only safe params between transfers)

**Note (your slow‑clock concern):** Because all downstream SPI happens inside the RP2354, the CORE can continue issuing other endpoint commands while the bridge service routine clocks the slow device; progress/completion is signaled via `EVT:SPIX_DONE` (non‑blocking). The per‑endpoint credits plus window keep others flowing.

---

## **10\) UART (EPID=UART)**

**Ops:**

* `OPEN` (baud, data bits, parity, stop bits, RTS/CTS enable, invert)

* `WRITE` (bytes; chunked)

* `READ` (pull mode) or `SUBSCRIBE_RX` to get push `EVT:UART_RX` packets when RX buffer crosses watermark

* `SET_WM` (RX/TX watermarks)

* `CLOSE`

**Events:**

* `EVT:UART_RX` (port, count, data, ts\_us),

* `EVT:UART_STATUS` (overrun, framing, break, CTS change, etc.)

Bridge keeps **ring buffers**; CORE can tune watermarks per latency vs overhead.

---

## **11\) CAN / CAN‑FD (EPID=CAN) *(if you add a transceiver)***

**Ops:** `OPEN` (bit timing), `FILTER_ADD`, `TX_FRAME`, `CLOSE`.  
 **Events:** `EVT:CAN_RX`, `EVT:CAN_ERR` (bus off, error passive), `EVT:CAN_TXC` (complete).

---

## **12\) PIO loader (EPID=PIO)**

**Goal:** Load/replace PIO programs and configure state machines on the fly.

**Ops:**

* `LOAD_PROG`

  * fields: `pio_idx (0/1)`, `offset`, `wrap_target`, `wrap`, `instr_count`, `instr[]` (16‑bit each), `pin_base`, `pin_count` (for sideset/set/out defaults).

  * Writes to PIO instruction RAM (halt any affected SMs first).

* `SM_CFG`

  * `sm_id`, `clkdiv`, `shift_cfg` (autopush/autopull, thresholds), `join` (RX/TX), `pindir` setup, `jmp_pin`, `in_pin`, `out_pin`, `sideset_count`, `fifo_join`, `status_sel`.

* `SM_START` / `SM_STOP`

* `PUSH` / `PULL` (fifo access for data planes)

* `EXEC` (single instruction injection)

* `CAPS` (query PIO RAM size, SM count)

**Events:** `EVT:PIO_IRQ` (which IRQ flag, pio\_idx, mask, ts\_us), optional `EVT:PIO_FIFO_WM`.

All writes are idempotent; loader uses SRAM staging and validates instruction bounds before committing.

---

## **13\) Firmware update (EPID=FWUP)**

Covers **internal** RP2354 loader updates (your A/B) and **external** QSPI image slotting for the main program.

**Ops:**

* `QUERY_LAYOUT` → regions: `{ name, base, size, slot_count, active_slot }`

* `ERASE` (region, offset, size)

* `WRITE` (region, offset, chunk) — chunk ≤ MTU‑overhead; CRC32 over chunk

* `SET_ACTIVE_SLOT` (region, slot\_id)

* `VERIFY` (region, expected\_crc/sha) → RSP with computed digest

* `COMMIT` (fsync/flush; optional manifest write TLV)

* `REBOOT` (mode: normal, bootloader, slotA, slotB, safe)

* `WDT_MARK_GOOD` (explicit “boot ok” handshake you described)

**Manifests:** Small TLV header per slot: `ver`, `build_id`, `timestamp`, `min_loader_ver`, `image_crc32`, `len`, `rollback_counter`. Bridge refuses activation if `min_loader_ver` not met.

---

## **14\) Interrupts & events (EPID=INT)**

All unsolicited notifications (aside from per‑EP push messages) also funnel here.

**Event types (TLV):** `GPIO`, `UART_RX`, `I2C_DONE`, `SPIX_DONE`, `CAN_RX`, `PIO_IRQ`, `FLOW` (credit update), `FAULT` (brownout, overtemp, flash error), `WDT_WARN`, `BOOT_REASON`.

**General event TLV format:**

`[ type(u8) | len(u8) | ts_us(u48 or u32) | payload... ]`

`BRIDGE_IRQ` is asserted until at least one EVT frame is drained (bridge may coalesce multiple TLVs into a single EVT payload).

---

## **15\) Error model (uniform across endpoints)**

**RSP** payload begins with:

```
struct Status {
    int16_t  code;      // 0=OK; <0 = error
    uint16_t detail;    // endpoint-specific subcode
}
```

Common codes:  
 `-1 TIMEOUT`, `-2 BUSY`, `-3 NACK`, `-4 PARAM`, `-5 CRC`, `-6 PERM`, `-7 UNSUP`, `-8 NO_MEM`, `-9 IO_ERR`, `-10 STATE`.

When `PRIO` \+ `EVT:FAULT` fire, include a compact **fault TLV**: class (power/therm/flash), code, value, ts.

---

## **16\) Security hooks (future-proof)**

* **Image signing TLV** in FWUP: optional ECDSA signature over manifest+image hash; store public key in OTP or protected flash region.

* **Session keys**: reserve `Flags.ENCRYPT`; keys established via MGMT `KEYX` (X25519) → enable ChaCha20‑Poly1305 framing for payload. (Keep off initially; include version gates.)

---

## **17\) Example flows**

### **17.1 Open UART2 and stream RX to CORE**

1. CORE→BRIDGE: `UART.OPEN {port=2, 115200 8N1, RTS/CTS=off}`

2. CORE→BRIDGE: `UART.SET_WM {rx=64, tx=64}`

3. BRIDGE→CORE (as data arrives): `EVT:UART_RX {port=2, data[...]}` (multiple per second)

4. CORE→BRIDGE: `UART.WRITE { "AT\r\n" }`

5. BRIDGE→CORE: `EVT:UART_STATUS {CTS change}` (if relevant)

### **17.2 SPI proxy transfer at 125 kHz while other endpoints keep flowing**

1. CORE→BRIDGE: `SPIX.OPEN {port=0, mode=0, clk=125000}`

2. CORE→BRIDGE: `SPIX.XFER {cs_hold=1, tx=[0x9F], rx_len=3}` (JEDEC ID)

3. BRIDGE keeps CS and clocks at 125 kHz internally; **other endpoints keep servicing** because SPIX work sits in its own queue; completion appears as `EVT:SPIX_DONE {status, rx=[...]}`.

4. Meanwhile, CORE can issue `I2C.XFER` or receive `UART_RX` events—no global stall.

### **17.3 PIO program load \+ start**

1. CORE→BRIDGE: `PIO.LOAD_PROG {pio=0, offset=0, wrap_target=0, wrap=31, instr_count=12, instr[]=...}`

2. CORE→BRIDGE: `PIO.SM_CFG {sm=0, clkdiv=2.0, pins..., shift...}`

3. CORE→BRIDGE: `PIO.SM_START {sm=0}`

4. BRIDGE→CORE: `EVT:PIO_IRQ {pio=0, mask=0x1}` when ISR triggers.

### **17.4 Firmware update to external QSPI slot B**

1. `FWUP.QUERY_LAYOUT` → discover `ext_qspi_program {base, size, slot_count=2, active=A}`

2. `FWUP.ERASE {region=ext_qspi, offset=slotB_off, size=image_len_rounded}`

3. Repeated `FWUP.WRITE` chunks with CRC32, then `FWUP.VERIFY {crc}`

4. `FWUP.SET_ACTIVE_SLOT {slot=B}`

5. `CTRL.REBOOT {mode=slotB}`; on next boot loader marks good via `WDT_MARK_GOOD`.

---

## **18\) Header & a few op enums (C sketches)**

```cpp
#pragma pack(push,1)
typedef struct {
    uint16_t sync;    // 0x5AA5
    uint8_t  ver;     // 1
    uint8_t  hdr_len; // 0x10
    uint8_t  flags;   // FRAG/LAST/PRIO/...
    uint8_t  type;    // 0=NOP 1=REQ 2=RSP 3=EVT
    uint8_t  epid;    // endpoint id
    uint8_t  seq;     // 0..15
    uint8_t  ack;     // last received seq
    uint16_t len;     // payload length
    uint8_t  hdr_crc; // CRC-8 over ver..len
} cbp_hdr_t;

typedef struct {
    uint8_t  op;  
    uint8_t  port;    // instance or 0xFF
    uint16_t txn;     // multi-frag correlator
} ep_hdr_t;
#pragma pack(pop)

enum EPID { EP_MGMT=0x00, EP_FWUP=0x01, EP_PIO=0x02, EP_GPIO=0x10, EP_I2C=0x20,
            `EP_SPIX=0x21, EP_UART=0x22, EP_CAN=0x23, EP_INT=0x30, EP_DBG=0xFE, EP_CTRL=0xFF };

enum I2C_OP { I2C_CFG=0x01, I2C_XFER=0x02 };
enum SPIX_OP{ SPIX_OPEN=0x01, SPIX_XFER=0x02, SPIX_CLOSE=0x03, SPIX_CFG=0x04 };
enum UART_OP{ UART_OPEN=0x01, UART_WRITE=0x02, UART_READ=0x03, UART_SET_WM=0x04, UART_CLOSE=0x05 };
enum PIO_OP { PIO_LOAD_PROG=0x01, PIO_SM_CFG=0x02, PIO_SM_START=0x03, PIO_SM_STOP=0x04, PIO_PUSH=0x05, PIO_PULL=0x06 };
enum FW_OP  { FW_QUERY=0x01, FW_ERASE=0x02, FW_WRITE=0x03, FW_VERIFY=0x04, FW_SET_ACTIVE=0x05, FW_COMMIT=0x06, FW_REBOOT=0x07 };
```

---

## **19\) Timing/latency notes (targets)**

* **Round‑trip (REQ→RSP)** on SPI at ≥8 MHz, small payloads: \~50–200 µs typical.

* **Event latency** from ISR to EVT frame ready: \<100 µs (bridge side), then depends on CORE drain cadence (IRQ‑driven).

* **UART RX push**: watermark 64B → EVT within 1 ms typical.

---

## **20\) Watchdog & safety**

* Bridge runs its own WDT; CORE must **pet** via `CTRL.WDT_PET` at negotiated interval (e.g., 1 s).

* On bridge reboot, it asserts `BRIDGE_RDY=0→1`, then sends `EVT:BOOT_REASON`. CORE should re‑`HELLO`.

* **Safe‑mode:** If repeated CRC or framing errors \> N in T ms, bridge enters degraded mode (lower SPI freq, drop to single outstanding frame) and signals `EVT:FAULT{FRAMING_STORM}`.

---

## **21\) Testing checklist**

* **Link fuzz:** flip random bits → verify CRC catches and retransmits.

* **Backpressure:** saturate UART RX \+ start slow SPI proxy → ensure I²C and GPIO still respond within bound.

* **Fragmentation:** send 64 KiB FW blob in random chunk sizes; verify reassembly & CRC.

* **Power events:** brown‑out injection → `EVT:FAULT` \+ recovery.

* **PIO hot‑swap:** SM running; load new program into a different offset and switch SMs cleanly.

---

## **22\) What to implement first (MVP order)**

1. Link layer (framing, Seq/Ack, CRC, window=2).

2. MGMT `HELLO`, `GET_STATS`, IRQ/ready handling.

3. GPIO \+ UART (with RX watermark EVTs).

4. I²C \+ SPI proxy (non‑blocking, EVT completion).

5. PIO loader (LOAD/CFG/START/STOP).

6. FWUP (external slot first; internal loader later).

7. CAN (if needed), encryption (optional).
