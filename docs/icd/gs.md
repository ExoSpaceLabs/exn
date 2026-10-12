# Ground Station (GS) Device/Service ICD

This document specializes the EXN master ICD for the Ground Station. The wire profile is defined in `../ICD.md`; the target service tailoring is defined in `../services.md`. This file defines GS routing, client responsibilities, and GS-originated application flows.

## 1. GS identity and topology

- GS APID: `0x0F0`.
- PUS-A TC Source ID: `0x10`.
- EXN supports a direct HIL profile where the GS device link terminates at MCU-RTOS (`0x100`).
- The flight-like profile inserts PI-COMMS (`0x103`) as the communications transport between Ground and the OBC, without changing the logical destination of OBC-directed commands.
- GS-originated control requests therefore normally target MCU APID `0x100`.
- MCU consumes device-proxy metadata and may re-issue a new TC to PI-CAM (`0x101`), FPGA-AI (`0x102`), or PI-COMMS (`0x103`) when required.
- Forwarded node TMs retain the producing node APID where the active transport permits complete Space Packet routing.

All currently implemented GS mission packets use the **transitional PUS-A compatibility profile** defined by the master ICD. New service development targets the PUS-C profile in `../services.md`.

---

## 2. Ground-segment software responsibility split

The EXN ground segment separates transport ownership from mission/application behavior.

### 2.1 Daemon/router

The GS daemon is the common connection point for local clients and owns the physical/simulator link. Its responsibilities are limited to:

- open/close and maintain the Serial/TCP device link;
- frame incoming CCSDS Space Packets;
- validate packet boundaries and expose packet metadata;
- route complete client-supplied Space Packets to the device link;
- broadcast received/transmitted packet metadata to connected clients;
- maintain link state and packet logging.

The daemon shall **not**:

- generate periodic HK requests;
- synthesize time-setting packets as liveness checks;
- own mission command sequence/correlation counters;
- schedule application services.

A daemon `PING` is an IPC/link-state liveness operation only and does not create a spacecraft packet.

### 2.2 Operator client/UI

Mission behavior belongs to the client application. The UI/client is responsible for:

- constructing CCSDSPack v2 packets according to this ICD;
- owning TC sequence counts for the packet streams it generates;
- owning GS transaction/correlation IDs;
- scheduling periodic requests such as System HK;
- enabling/disabling those schedules as part of the operator workflow.

The current terminal UI uses a default **2 s** periodic System HK cadence when HK scheduling is enabled. This cadence is an application setting, not a daemon protocol requirement.

---

## 3. GS -> MCU proxy preamble

A GS TC that MCU must re-issue to another EXN endpoint begins its Application Data with the four-octet proxy preamble:

| Field | Type | Bits | Description |
|---|---|---:|---|
| `transactionId` | uint16 | 16 | `1..65535`; `0` reserved |
| `target` | uint8 | 8 | `1=PI-CAM`, `2=FPGA-AI`, `3=PI-COMMS` |
| `options` | uint8 | 8 | bit0 mirror response to GS; bit1 mirror to peer; remaining bits zero |

The GS -> MCU packet still has TC APID `0x100` and PUS-A Source ID `0x10` because MCU is the direct endpoint of that hop.

MCU removes the preamble before creating the downstream packet. The downstream TC then uses target APID `0x101`, `0x102`, or `0x103` and MCU Source ID `0x01` under the current compatibility profile.

### 3.1 Services that use the preamble

The preamble is required for GS-originated device-directed requests when the actual service endpoint is not MCU. The currently implemented examples use legacy Services 3, 20 and 23 plus mission-specific Services 200 and 210. The normalized target service numbers are defined in `../services.md`.

It is **not** used merely because a packet originated at GS.

### 3.2 Direct MCU requests

MCU-local services have no proxy preamble. In particular:

- TC `3/10` System HK Request is direct GS -> MCU.
- GS MCU-local TC `3/1` HK Request is direct GS -> MCU.

---

## 4. Housekeeping behavior

### 4.1 Single-node HK, 3/1

For MCU-local HK:

- TC APID: `0x100`;
- PUS-A Source ID: `0x10`;
- App Data: optional `detailMask:uint16`;
- no proxy preamble.

For PI-CAM/FPGA/PI-COMMS HK through MCU:

- GS -> MCU App Data: proxy preamble + optional `detailMask:uint16`;
- MCU re-issues TC `3/1` to the selected target APID.

Expected response is TM `3/2` with the target node as producer APID.

### 4.2 System HK, 3/10 and 3/100

System HK is an MCU aggregation service and is therefore direct.

GS -> MCU TC `3/10`:

- APID `0x100`;
- Source ID `0x10`;
- no proxy preamble;
- exactly five Application Data octets:

| Field | Type | Description |
|---|---|---|
| `transactionId` | uint16 | request/response correlation |
| `include_mask` | uint8 | bit0 MCU, bit1 PI-CAM, bit2 FPGA, bit3 PI-COMMS |
| `detailMask` | uint16 | common detail selection |

MCU -> GS TM `3/100` uses producer APID `0x100` and echoes `transactionId`.

The GS UI's periodic HK job sends this `3/10` packet. The daemon only routes it.

---

## 5. Other GS command flows

The packet numbers in this section document the **current transitional profile**. They remain useful for the existing GS/HIL implementation but shall migrate according to `../services.md`.

All device-directed examples below are GS -> MCU packets with APID `0x100`, PUS-A Source ID `0x10`, and the proxy preamble before the service payload.

### Parameter management

- TC `20/1` Set Parameter: preamble + `key:uint8` + `value:TLV`.
- TC `20/2` Get Parameter: preamble + `key:uint8`.
- Expected TM `20/3` from the producing target APID.

### Camera control, target PI

- TC `200/1` Capture: preamble + `{mode:uint8, burst_count:uint16, exposure_us:uint32}`.
- TC `200/2` Camera Settings Set: preamble + `key:uint8` + `value:TLV`.
- TC `200/3` Camera Settings Get: preamble + `key:uint8`.
- Expected TM `200/4`, `200/5`, and Service 23 image-transfer TMs as applicable.

### FPGA control, target FPGA

- TC `210/1` Execute: preamble + `{pipeline:uint8, modelId:uint16, flags:uint16}`.
- TC `210/2` Processing Settings Set: preamble + `key:uint8` + `value:TLV`.
- TC `210/3` Processing Settings Get: preamble + `key:uint8`.
- Expected TM `210/4`/`210/5` and event/report packets as applicable.

### Data transfer

- TC `23/1` Start Transfer: preamble + `{imageId:uint16, dest:uint8, chunk_size:uint16}`.
- TC `23/2` Stop Transfer: preamble + `{imageId:uint16}`.
- Expected TM `23/10` Metadata, `23/11` Chunks, and `23/12` Complete.

---

## 6. Legacy GS reporting packet, Service 250/1

GS may produce reporting packet `250/1` when application-level UI/proxy acknowledgement is required.

- Packet Type: TM/report (`0`).
- Producer APID: GS `0x0F0`.
- PUS profile: `PUS:revA:TM`.
- Application Data: `{transactionId:uint16, ackCode:uint8, detail:uint16}`.

The physical direction is still GS -> MCU. Packet Type expresses the current EXN request/report convention rather than being hard-bound to link direction. This packet remains supported only for compatibility: the target profile uses PUS Service 1 for spacecraft request verification and keeps daemon/link acknowledgements in the local GS IPC protocol.

---

## 7. CCSDSPack v2 construction hints

Current GS compatibility-profile TC construction uses:

- selector `PUS:revA:TC`;
- one-octet Source ID width;
- Source ID `0x10`;
- CRC16 enabled;
- sequence flags UNSEGMENTED;
- destination APID according to the direct packet endpoint.

System HK is the first GS client flow migrated to this API. Other UI command builders should reuse the same shared packet-construction layer rather than adding command-specific packet synthesis to the daemon.


The PUS-C migration shall update GS packet builders, simulator fixtures, acknowledgement flags and golden vectors in the same change as the shared interface definitions. The GS daemon remains service-agnostic and should require little or no service-specific change.
