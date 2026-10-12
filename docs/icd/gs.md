# Ground Station (GS) Endpoint ICD

This document is the normative EXN ICD for the Ground/HIL endpoint.

Common packet rules are defined in `../ICD.md`, service semantics in `../services.md`, and shared payload models in `../component-services.md`.

## 1. Identity

| Item | Value |
|---|---|
| APID | `0x0F0` |
| PUS-C source/destination ID | `0x0010` |
| PUS revision | C |

Ground is an operator endpoint, not spacecraft command authority. Nominal flight-like control traffic enters through the Antenna and is supervised by the OBC.

## 2. Ground transport profiles

### 2.1 Flight-like profile

```text
Ground -> Antenna -> OBC
```

The Ground-facing physical/network transport terminates at PI-COMMS/Antenna. Complete EXN Space Packets are forwarded toward the spacecraft-side destination.

### 2.2 HIL profile

```text
Ground -> OBC simulator / physical OBC
```

Direct HIL connection changes only the transport topology. Packet/APID/PUS semantics remain identical to the flight-like profile.

## 3. Ground software responsibility split

The transport daemon/router owns:

- Serial/TCP/other selected Ground-device transport;
- CCSDS Space Packet framing;
- structural packet validation;
- routing complete packets between local clients and the device link;
- transport/link state and counters.

It does **not** own mission-service semantics, periodic HK policy, time synchronization policy, service IDs, or packet payload construction.

The operator/application layer owns:

- PUS-C packet construction/decoding according to this ICD;
- operator actions;
- TC sequence counters;
- scheduled operator requests where configured;
- display of HK, events, alarms, transfers and update state.

## 4. OBC-routed commands

Ground operations that target PI-CAM, FPGA, Antenna, or PI-MGMT are sent to the OBC for supervision.

The GS -> OBC Application Data begins with this EXN routing envelope before the target-service payload:

| Field | Type | Meaning |
|---|---|---|
| `transaction_id` | uint16 | Ground/OBC end-to-end correlation; 0 reserved |
| `target` | uint8 | 1=PI-CAM, 2=FPGA, 3=Antenna, 4=PI-MGMT |
| `options` | uint8 | bit0 mirror target response to Ground; other bits reserved |

Wire path:

```text
GS TC: APID 0x100, Source ID 0x0010
        routing envelope + target payload
                     |
                     v
OBC validates / authorizes / removes envelope
                     |
                     v
new TC: target APID, Source ID 0x0001
        target-service payload only
```

The OBC tracks the relationship between the original Ground request and the downstream request. PUS Service 1 verifies each actual TC hop; `transaction_id` exists only for the OBC's end-to-end routed-operation bookkeeping.

OBC-local requests do not carry this envelope.

## 5. OBC-local Ground operations

Examples of direct Ground -> OBC requests include:

- `TC[3,27]` one-shot OBC HK;
- `TC[3,128]` EXN System HK;
- `TC[9,128]` Set Mission Time;
- OBC Service 20 configuration;
- OBC Service 8 functions;
- OBC Service 230 software-management requests.

All use APID `0x100` and PUS-C Source ID `0x0010`.

## 6. System HK

### 6.1 Request

`TC[3,128]` Application Data:

```text
transaction_id:u16
include_mask:u8
detail_mask:u16
```

`include_mask`:

- bit0 OBC;
- bit1 PI-CAM;
- bit2 FPGA;
- bit3 Antenna;
- bit4 PI-MGMT;
- bits5..7 reserved.

### 6.2 Report

`TM[3,129]` is produced by OBC and addressed to Ground.

Application Data:

```text
transaction_id:u16
present_mask:u8
status:u8
block_count:u8
repeat block_count:
    producer_apid:u16
    block_length:u16
    block[block_length]
```

`status`:

| Value | Meaning |
|---:|---|
| 0 | OK |
| 1 | PARTIAL |
| 2 | TIMEOUT |
| 3 | ERROR |

Each embedded block is the latest valid endpoint HK data selected by the OBC aggregation policy.

## 7. Parameter management from Ground

Service 20 is **runtime configuration**, not a generic command service.

For an OBC parameter, Ground sends the standard Service 20 payload directly to APID `0x100`.

For Camera/FPGA/Antenna/PI-MGMT parameters, Ground sends the same target Service 20 payload behind the routing envelope from Section 4.

Examples:

- read OBC heartbeat period -> Service 20;
- set Antenna `payload_forwarding.enable` -> routed Service 20;
- set Camera exposure -> **not** Service 20; routed Service 200;
- restart Camera -> **not** Service 20; routed Service 8 to PI-MGMT.

## 8. Operator-visible service coverage

The Ground application shall support the central EXN service catalogue rather than maintaining a separate local one.

Required operator coverage:

- Service 1 request verification;
- Service 3 heartbeat/HK/System HK;
- Service 5 event/alarm display;
- Service 8 recovery/reboot functions;
- Service 9 OBC time synchronization/status;
- Service 12 monitoring/alarm state;
- Service 20 runtime configuration;
- Service 200 Camera control/capability/product catalog;
- Service 210 FPGA capabilities/jobs/results;
- Service 220 object transfer and transfer status;
- Service 230 software inventory/update lifecycle.

## 9. PUS-C construction

GS telecommands use:

- `PUS:revC:TC`;
- Source ID `0x0010`;
- CRC16;
- destination APID according to the direct hop;
- acknowledgement flags from `../ICD.md`.

GS decodes telemetry using `PUS:revC:TM` with the EXN six-octet CUC timestamp profile.

There is no GS Service 250 packet. Local daemon acknowledgements/status remain local IPC, while spacecraft request verification uses PUS Service 1.
