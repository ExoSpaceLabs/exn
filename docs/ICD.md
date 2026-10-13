# ICD — EXN CCSDS/PUS Interface Control Document

This repository is the **complete EXN Interface Control Document authority**.

This master document defines the common wire contract for communication between EXN logical endpoints. It is part of a normative ICD set:

- `docs/ICD.md` — common CCSDS/PUS headers, APIDs, routing and shared wire rules;
- `docs/services.md` — service/subservice allocation and PUS tailoring;
- `docs/component-services.md` — common operational semantics and cross-component payload models;
- `docs/icd/gs.md` — Ground/HIL endpoint contract;
- `docs/icd/mcu-rtos.md` — OBC endpoint contract;
- `docs/icd/pi-cam.md` — Camera endpoint contract;
- `docs/icd/linux-payload.md` — Antenna and PI-MGMT endpoint contracts;
- `docs/icd/fpga-ai.md` — FPGA processing endpoint contract.

Together, these files are the full EXN ICD. Component repositories are implementations of this contract, not additional interface authorities.

If an implementation requires a new command, telemetry item, field, state, event, parameter, sensor, routing behavior, or error code, the normative definition shall be added to this ICD set before the implementation depends on it.

Logical endpoints may be co-located on one physical computer. In particular, PI-CAM, PI-COMMS/Antenna and the PI-MGMT supervisor execute on the same Raspberry Pi while retaining independent endpoint identities. PI-MGMT is management infrastructure, not a third Pi mission purpose.

## 0. Active protocol baseline

EXN uses one normative packet profile for all new and maintained components:

- CCSDS Space Packet Protocol, Packet Version Number `0`;
- CCSDSPack **v2.0.0**, the latest released CCSDSPack baseline;
- **ECSS PUS Revision C** TC/TM secondary headers;
- CRC-16/CCITT-FALSE packet error control;
- big-endian mission Application Data;
- unsegmented CCSDS Space Packets by default;
- six-octet CUC timestamps in PUS-C telemetry;
- Unix epoch as the EXN mission-defined CUC epoch.

There is no active PUS-A mission profile. Historical PUS-A traffic belongs only to repository history and migration notes. Components still using it, including the existing GS/HIL implementation, shall be migrated to this PUS-C contract rather than extended.

### 0.1 Normative endpoint identities

| Node | APID | PUS-C Source/Destination ID | Role |
|---|---:|---:|---|
| GS | `0x0F0` | `0x0010` | Ground station/operator client |
| MCU-RTOS / OBC | `0x100` | `0x0001` | command authority, orchestration and time master |
| PI-CAM | `0x101` | `0x0002` | Camera/image payload service |
| FPGA-AI | `0x102` | `0x0003` | Processing/inference node |
| PI-COMMS / Antenna | `0x103` | `0x0004` | single Ground-link service |
| PI-MGMT | `0x104` | `0x0005` | Pi platform supervisor, restart/reboot/update management |

PUS-C source and destination identifiers are fixed at two octets by the selected header profile. EXN keeps the numeric identity values small for readability; their on-wire width is still 16 bits.

PI-CAM and PI-COMMS are the two Pi mission endpoints. PI-MGMT is supporting infrastructure owned by `exn-platform.service`, allowing the OBC to recover/update Pi mission services even when the failed process cannot receive commands.

## 1. CCSDS Space Packet profile

### 1.1 Primary header

All EXN packets use the standard six-octet CCSDS Space Packet Primary Header.

| Field | Bits | EXN policy |
|---|---:|---|
| Version | 3 | `0` |
| Packet Type | 1 | `1` requesting/TC, `0` reporting/TM |
| Secondary Header Flag | 1 | `1` for current EXN application packets |
| APID | 11 | Mission-managed endpoint/path identifier, policy below |
| Sequence Flags | 2 | `11` / UNSEGMENTED unless a future interface explicitly adopts CCSDS segmentation |
| Sequence Count | 14 | `0..16383`, modulo 16384 per Packet Identification stream |
| Packet Data Length | 16 | Encoded value `N - 1`, where `N` is all octets after the primary header, including CRC when enabled |

The complete packet size is therefore:

`6 + Packet Data Length + 1`

Do not treat Packet Data Length as a literal byte count. Earlier EXN material did so and was off by one.

### 1.2 APID naming policy

CCSDS permits APID naming to be mission-specific. EXN fixes the following convention:

- **TC/request packet:** APID identifies the **destination EXN endpoint**.
- **TM/report packet:** APID identifies the **producing EXN endpoint**.

Examples:

- GS -> OBC System HK request: TC APID `0x100`, PUS-C Source ID `0x0010`.
- OBC -> PI-CAM Camera command: TC APID `0x101`, PUS-C Source ID `0x0001`.
- OBC -> Antenna command: TC APID `0x103`, PUS-C Source ID `0x0001`.
- OBC -> PI-MGMT command: TC APID `0x104`, PUS-C Source ID `0x0001`.
- PI-CAM -> OBC Camera report: TM APID `0x101`, PUS-C Destination ID `0x0001`.
- Antenna -> OBC link HK: TM APID `0x103`, Destination ID `0x0001`.
- PI-MGMT -> OBC platform HK: TM APID `0x104`, Destination ID `0x0001`.
- OBC -> GS System HK report: TM APID `0x100`, Destination ID `0x0010`.

The Packet Sequence Count authority follows the complete managed Packet Identification stream, not a vague "per sender" rule.

### 1.3 Packet error control

Current EXN interfaces use CCSDSPack `PacketErrorControlMode::CRC16`, corresponding to CRC-16/CCITT-FALSE:

- polynomial `0x1021`;
- initial value `0xFFFF`;
- no reflection;
- final XOR `0x0000`.

The two CRC octets are part of the CCSDS Packet Data Field length calculation.

---

## 2. PUS Revision C profile

EXN uses PUS-C everywhere. A packet is not considered EXN-conformant merely because it carries a valid CCSDS primary header; its PUS-C direction, service/subservice, identifiers, acknowledgement policy and telemetry timestamp shall match this ICD.

### 2.1 Telecommand secondary header

EXN telecommands use CCSDSPack `ccsds::pus::rev_c::TcHeader` / selector `PUS:revC:TC`.

The PUS-C TC header is:

| Field | Width | EXN policy |
|---|---:|---|
| PUS version | 4 bits | Revision C encoding |
| acknowledgement flags | 4 bits | command-specific, below |
| service type | 8 bits | `docs/services.md` |
| service subtype | 8 bits | `docs/services.md` |
| source ID | 16 bits | endpoint ID from Section 0.1 |

No source-ID-width tailoring exists in EXN PUS-C.

Acknowledgement policy:

- **read/report requests:** acceptance verification, flag mask `0x8`;
- **state-changing finite commands:** acceptance + completion verification, mask `0x9`;
- **long-running commands:** acceptance + start + completion (`0xD`) where start-of-execution has useful semantics;
- **progress verification:** enabled only for commands whose endpoint ICD defines progress semantics.

The bit values follow the PUS-C acknowledgement field; EXN does not define a second ACK convention.

### 2.2 Telemetry secondary header

EXN telemetry uses CCSDSPack `ccsds::pus::rev_c::TmHeader` / selector `PUS:revC:TM`.

The PUS-C TM header carries:

| Field | Width | EXN policy |
|---|---:|---|
| PUS version | 4 bits | Revision C encoding |
| spacecraft time reference status | 4 bits | EXN mapping below |
| service type | 8 bits | producer service |
| service subtype | 8 bits | producer subtype |
| message type counter | 16 bits | modulo-65536 per producer/service/subservice stream |
| destination ID | 16 bits | logical intended consumer |
| time | 6 octets | CUC, 4 coarse + 2 fine |

EXN mission tailoring for the PUS-C TM timestamp:

- `timestampPresent = true`;
- CUC epoch: **agency-defined = Unix epoch, 1970-01-01T00:00:00Z**;
- P-field: implicit;
- coarse octets: 4;
- fine octets: 2.

The destination ID is the PUS-C logical destination identifier, not the CCSDS APID. Typical values are OBC `0x0001` and Ground `0x0010`.

EXN defines the four time-reference-status values currently used:

| Value | Meaning |
|---:|---|
| 0 | UNSYNCED |
| 1 | SYNCED |
| 2 | HOLDOVER |
| 3 | INVALID |

Other values are reserved.

### 2.3 Time representation

The OBC is the on-board time authority.

Application logic uses:

- `monotonic_ms:uint64` for timers and elapsed time;
- `unix_ms:uint64` for synchronized mission time.

PUS-C telemetry carries the six-octet CUC timestamp defined above. Endpoint application code shall derive that timestamp from its synchronized mission-time state; filesystem or host wall-clock timestamps are not mission authority.

### 2.4 Service-definition clarity requirement

No EXN service or subservice is considered frozen until its normative documentation states:

1. **Purpose** — why the service exists;
2. **Owner/users** — which endpoints provide and consume it;
3. **What belongs in it** — concrete examples;
4. **What does not belong in it** — nearby concepts owned by another service;
5. **Messages/payloads** — service/subservice allocation and wire fields;
6. **state/error semantics** where applicable.

A service name alone is not an interface definition.

## 3. Ground ingress, MCU routing and correlation

EXN supports two ground-link deployment profiles without changing control ownership:

1. **HIL/direct profile:** GS connects directly to MCU-RTOS.
2. **Flight-like profile:** GS connects to the PI-COMMS Antenna service, which delivers ground traffic toward MCU-RTOS. The Antenna is the Ground link, not the spacecraft command authority.

In both profiles, device-directed commands are supervised/routed by MCU-RTOS before being re-issued to controlled payload endpoints.

For downstream-device operations handled by the MCU proxy path:

1. **GS -> MCU:** TC APID `0x100`, Source ID `0x10`. In the flight-like profile this packet is transported through PI-COMMS without changing its logical destination.
2. **MCU -> target:** MCU consumes the proxy preamble and creates a new TC using the target APID (`0x101`, `0x102`, `0x103`, or `0x104`) and Source ID `0x01`.

Device TMs retain their producer APID while MCU and/or the communications path routes them toward GS where the deployment permits it.

### 3.1 Proxy preamble

The proxy preamble is exactly four octets and is used on GS-originated TCs that MCU must re-issue to a payload endpoint.

| Field | Type | Bits | Meaning |
|---|---|---:|---|
| `transactionId` | uint16 | 16 | Correlation ID, `1..65535`; `0` reserved |
| `target` | uint8 | 8 | `1=PI-CAM`, `2=FPGA-AI`, `3=PI-COMMS/Antenna`, `4=PI-MGMT` |
| `options` | uint8 | 8 | bit0 mirror response to GS; bit1 mirror to peer; remaining bits zero |

The preamble is not part of the downstream packet. MCU removes it before re-issuing the command.

### 3.2 Direct MCU services

Commands whose service endpoint is MCU itself do **not** use the proxy preamble. In particular:

- `TC[3,128]` Request System HK is GS -> OBC and has **no Application Data**. It asks the OBC for its current cached spacecraft snapshot.
- A GS request for OBC-local one-shot HK uses standard `TC[3,27]` and likewise needs no routing envelope.

System-HK correlation uses the standard PUS request ID carried in the resulting `TM[3,129]`; no additional System-HK transaction identifier is defined.

---

## 4. Common enumerations

### 4.1 Result codes

| Value | Meaning |
|---:|---|
| 0 | OK |
| 1 | INVALID |
| 2 | BUSY |
| 3 | UNSUPPORTED |
| 4 | TIMEOUT |
| 5 | INTERNAL |

### 4.2 Pixel types

| Value | Meaning |
|---:|---|
| 1 | RGB888 |
| 2 | GRAY8 |
| 3 | GRAY16 |
| 4 | YUV420 |
| 5 | BAYER_RGGB8 |

---

## 5. Normative service catalogue

The authoritative service/subservice allocation is `docs/services.md`; complete operational payloads are in `docs/component-services.md` and the endpoint ICDs.

This section exists as the high-level wire-facing map and, deliberately, explains the purpose of each service instead of assuming the number is self-explanatory.

### 5.1 Service 1 — Request Verification

**Purpose:** report whether a TC was accepted and whether execution started/progressed/completed successfully.

**Belongs here:** command acceptance/completion status.

**Does not belong here:** Camera-specific results, FPGA job results, or Ground-daemon link acknowledgements.

EXN uses standard PUS-C TM[1,1..8] verification reports according to the acknowledgement flags requested by the TC.

### 5.2 Service 3 — Housekeeping

**Purpose:** obtain and periodically report component liveness, health, sensors and link state.

**Belongs here:** HEARTBEAT, HEALTH, SENSORS, LINKS and System-HK aggregation.

**Does not belong here:** threshold-definition logic (Service 12), arbitrary configuration changes (Service 20), or operational commands.

Standard one-shot messages:

- `TC[3,27]` Generate one-shot HK report;
- `TM[3,25]` HK report.

EXN System HK extensions:

- `TC[3,128]` Request System HK;
- `TM[3,129]` System HK Report.

### 5.3 Service 5 — Event Reporting

**Purpose:** asynchronous notification of state changes, warnings, faults and alarms.

**Belongs here:** boot/ready, link loss/restoration, thermal/storage alarms, capture failures, processing failures.

**Does not belong here:** sampled HK values or command verification.

EXN uses the PUS-C severity subtypes 5/1 through 5/4.

### 5.4 Service 8 — Function Management

**Purpose:** invoke named management functions whose semantics are actions rather than configuration values.

**Belongs here:** reinitialize endpoint, restart Camera/Antenna through PI-MGMT, reboot Pi, clear recoverable faults.

**Does not belong here:** setting heartbeat periods or thresholds (Service 20), software-package lifecycle (Service 230).

EXN uses standard `TC[8,1]` Perform a Function with centrally assigned function IDs.

### 5.5 Service 9 — Time Management

**Purpose:** establish and report mission time.

**Belongs here:** OBC time synchronization/distribution and time-report policy.

**Does not belong here:** timer periods used by unrelated services; those are service configuration parameters.

EXN adds `TC[9,128]` Set Mission Time with the payload defined in `docs/component-services.md`.

### 5.6 Service 12 — On-board Monitoring

**Purpose:** monitor declared health/sensor values against configured limits and expose current out-of-limit state.

**Belongs here:** temperature/storage/voltage/link-loss monitor state, warning/critical threshold evaluation.

**Does not belong here:** the sampled sensor value itself (Service 3) or changing the threshold value (Service 20).

EXN initially uses predeclared monitoring definitions rather than dynamic definition creation.

### 5.7 Service 20 — On-board Parameter Management
#### EXN meaning: runtime configuration parameters

**Purpose:** read and change bounded runtime/persistent configuration values that are **settings**, not operations.

This service is intentionally narrow. It prevents every component from inventing its own generic setter/getter service while also preventing Service 20 from becoming a miscellaneous command drawer.

**Belongs here:**

- OBC heartbeat period, missed-heartbeat limit, automatic-restart enable;
- OBC time-resynchronization period;
- Antenna link/uplink/downlink enable flags and software rate limit;
- PI-MGMT restart-storm/storage/thermal thresholds;
- FPGA default queue/timeout policy when not operation-specific.

**Does not belong here:**

- Camera exposure, autofocus, sensor mode or acquisition controls → **Service 200**;
- starting/stopping acquisition → **Service 200**;
- restart/reboot actions → **Service 8**;
- time synchronization → **Service 9**;
- FPGA job submission/cancellation → **Service 210**;
- payload transfer → **Service 220**;
- software activation/rollback → **Service 230**;
- housekeeping request → **Service 3**.

Standard PUS-C message meanings used by EXN:

- `TC[20,1]` request/report parameter values;
- `TM[20,2]` parameter value report;
- `TC[20,3]` set parameter values.

An endpoint does **not** implement Service 20 merely because it has setters. If all meaningful configuration belongs to a mission-specific service, its Service 20 parameter set may be empty.

### 5.8 Service 200 — Camera Control

**Purpose:** operate PI-CAM as a Camera Module 3 payload service.

**Belongs here:** acquisition modes, exposure/AE, autofocus, Camera capabilities, stored-product catalog.

**Does not belong here:** moving an already-created object between endpoints (Service 220) or restarting the Camera process (Service 8 through PI-MGMT).

Normative subtypes and payloads are defined in `docs/icd/pi-cam.md`.

### 5.9 Service 210 — Processing

**Purpose:** operate the FPGA as a generic advertised processing-job engine.

**Belongs here:** capability discovery, submit/cancel job, job status, small processing result.

**Does not belong here:** bulk object bytes (Service 220), generic FPGA runtime configuration (Service 20), reset/reboot (Service 8).

### 5.10 Service 220 — Payload Object Transfer

**Purpose:** transfer bounded mission objects such as RAW/TIFF/JPEG images, masks, processed products, result blobs and update packages.

**Belongs here:** object metadata, chunk data, transfer completion/status.

**Does not belong here:** Camera capture semantics (Service 200), processing semantics (Service 210), software activation (Service 230).

### 5.11 Service 230 — Software Management

**Purpose:** control the lifecycle of software/update packages after the package bytes have been transferred.

**Belongs here:** inventory, stage, activate, rollback, update status.

**Does not belong here:** update-package bytes themselves (Service 220) or ordinary process restart (Service 8).

## 6. CCSDSPack v2 configuration mapping

The active templates under `interfaces/ccsdspack/` use the same normative PUS-C profile as this ICD:

- TC selector: `PUS:revC:TC`;
- TM selector: `PUS:revC:TM`;
- PUS-C fixed 16-bit source/destination identifiers;
- CRC-16 packet error control;
- PUS-C TM message-type counter;
- six-octet CUC telemetry timestamp using the EXN Unix agency-defined epoch.

The templates are machine-readable realizations of this ICD, not a compatibility layer for another revision.

The JSON mirrors and MCU C header shall carry the same service/subservice assignments. Interface CI shall reject reintroduction of `PUS:revA:TC` or `PUS:revA:TM` in active EXN interface artifacts.

## 7. Implementation invariants

1. All maintained EXN application packets use **PUS Revision C**.
2. TC APID identifies the destination endpoint; TM APID identifies the producing endpoint.
3. PUS-C source/destination IDs are 16-bit endpoint identities from Section 0.1.
4. TM packets carry the EXN six-octet CUC timestamp.
5. Request verification uses Service 1; mission services do not invent parallel ACK packets.
6. The OBC remains command authority even when bulk payload bytes bypass it.
7. Ground downlink of Camera/FPGA data means producer -> Antenna -> Ground.
8. Service 20 is limited to runtime/persistent configuration parameters; operational actions remain in their owning service.
9. Physical co-location of PI-CAM, PI-COMMS and PI-MGMT shall not collapse their logical identities.
10. Every mission-visible ID, payload, state and semantic is defined in this repository before component implementations rely on it.
11. Interface artifacts under `interfaces/` shall agree with this ICD and shall not preserve an alternate legacy profile.
