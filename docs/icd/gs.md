# Ground Station (GS) Device/Service ICD

This document specializes the EXN master ICD for the Ground Station. The wire profile is defined in `../ICD.md`; this file defines GS routing, client responsibilities, and GS-originated application flows.

## 1. GS identity and topology

- GS APID: `0x0F0`.
- PUS-A TC Source ID: `0x10`.
- The GS transport link terminates at MCU-RTOS (`0x100`).
- GS-originated TC/request packets sent over that link therefore use destination APID `0x100`.
- MCU consumes device-proxy metadata and re-issues a new TC to PI (`0x101`) or FPGA (`0x102`) when required.
- Forwarded node TMs retain the producing node APID where the link permits raw Space Packet routing.

All current GS mission packets use the PUS revision-A profile defined by the master ICD.

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

A GS TC that is intended for PI or FPGA begins its Application Data with the four-octet proxy preamble:

| Field | Type | Bits | Description |
|---|---|---:|---|
| `transactionId` | uint16 | 16 | `1..65535`; `0` reserved |
| `target` | uint8 | 8 | `1=PI`, `2=FPGA` |
| `options` | uint8 | 8 | bit0 mirror response to GS; bit1 mirror to peer; remaining bits zero |

The GS -> MCU packet still has TC APID `0x100` and PUS-A Source ID `0x10` because MCU is the direct endpoint of that hop.

MCU removes the preamble before creating the downstream packet. The downstream TC then uses target APID `0x101` or `0x102` and PUS-A Source ID `0x01`.

### 3.1 Services that use the preamble

The preamble is required for GS-originated device-directed requests in Services 3, 20, 23, 200, and 210 when the actual service endpoint is PI or FPGA.

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

For PI/FPGA HK through MCU:

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
| `include_mask` | uint8 | bit0 MCU, bit1 PI, bit2 FPGA |
| `detailMask` | uint16 | common detail selection |

MCU -> GS TM `3/100` uses producer APID `0x100` and echoes `transactionId`.

The GS UI's periodic HK job sends this `3/10` packet. The daemon only routes it.

---

## 5. Other GS command flows

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

## 6. GS reporting packet, Service 250/1

GS may produce reporting packet `250/1` when application-level UI/proxy acknowledgement is required.

- Packet Type: TM/report (`0`).
- Producer APID: GS `0x0F0`.
- PUS profile: `PUS:revA:TM`.
- Application Data: `{transactionId:uint16, ackCode:uint8, detail:uint16}`.

The physical direction is still GS -> MCU. Packet Type expresses EXN request/report semantics rather than being hard-bound to link direction. Consequently, the GS daemon/router must route structurally valid Space Packets without assuming every outbound packet has Packet Type TC.

---

## 7. CCSDSPack v2 construction hints

GS TC construction uses:

- selector `PUS:revA:TC`;
- one-octet Source ID width;
- Source ID `0x10`;
- CRC16 enabled;
- sequence flags UNSEGMENTED;
- destination APID according to the direct packet endpoint.

System HK is the first GS client flow migrated to this API. Other UI command builders should reuse the same shared packet-construction layer rather than adding command-specific packet synthesis to the daemon.
