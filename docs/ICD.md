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

## 0. Current protocol baseline

EXN currently uses:

- CCSDS Space Packet Protocol, Packet Version Number `0`;
- CCSDSPack **v2.x**, with released **v2.0.0** as the current reproducible baseline;
- **PUS revision A** TC/TM secondary headers for the current transitional EXN wire profile;
- CRC-16/CCITT-FALSE packet error control;
- big-endian application-data integers;
- unsegmented CCSDS Space Packets by default. Service 23 chunking is application-level chunking, not CCSDS packet segmentation.

PUS revision and PUS service number are separate concepts. The current PUS-A header profile describes the bytes emitted by the existing EXN-GS/HIL baseline; it does **not** mean that every current EXN service-number assignment is a standard PUS-A/PUS-C service realization.

### Service-tailoring status

The service audit identified legacy EXN assignments that conflict with the published PUS-C catalogue. In particular:

- PUS Service 9 is Time Management; EXN currently uses legacy Service 17 for time;
- PUS Service 17 is Test;
- PUS Service 13 is Large Packet Transfer;
- PUS Service 20 is Parameter Management, but the current EXN subtype meanings do not match the standard;
- PUS Service 23 is File Management; EXN currently uses it for image/result chunk transfer.

The **target EXN system profile is PUS-C with explicit mission tailoring**, defined in [`services.md`](services.md). The current PUS-A definitions below remain normative only for the transitional compatibility baseline until EXN-GS, shared interface artifacts and endpoint implementations migrate together.

No new endpoint implementation shall extend the legacy Service 17/20/23 assignments. Mission-specific behavior shall use the standard extension rules or mission-specific service types in the 128..255 range.

### Nodes

| Node | APID | Transitional/target Source ID | Role |
|---|---:|---:|---|
| GS | `0x0F0` | `0x10` | Ground station/operator client |
| MCU-RTOS / OBC | `0x100` | `0x01` | command authority, orchestration and time master |
| PI-CAM | `0x101` | `0x02` | Camera/image payload service |
| FPGA-AI | `0x102` | `0x03` | Processing/inference node |
| PI-COMMS / Antenna | `0x103` | `0x04` | single Ground-link service |
| PI-MGMT | `0x104` | `0x05` | Pi platform supervisor, restart/reboot/update management |

PI-CAM and PI-COMMS are the two Pi mission endpoints. PI-MGMT is a supporting management endpoint owned by `exn-platform.service`; it exists so the OBC can restart/update a failed Pi mission service without addressing the failed process itself.

---

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

- GS -> MCU System HK request: TC APID `0x100`, PUS-A TC Source ID `0x10`.
- MCU -> PI camera command: TC APID `0x101`, PUS-A TC Source ID `0x01`.
- MCU -> PI Antenna command: TC APID `0x103`, PUS-A TC Source ID `0x01`.
- MCU -> Pi platform-management command: TC APID `0x104`, Source ID `0x01`.
- PI-CAM -> MCU/GS camera ACK: TM APID `0x101`.
- PI-COMMS/Antenna -> MCU link HK: TM APID `0x103`.
- PI-MGMT -> MCU platform/service-supervision HK: TM APID `0x104`.
- MCU -> GS System HK report: TM APID `0x100`.

The Packet Sequence Count authority follows the complete managed Packet Identification stream, not a vague "per sender" rule.

### 1.3 Packet error control

Current EXN interfaces use CCSDSPack `PacketErrorControlMode::CRC16`, corresponding to CRC-16/CCITT-FALSE:

- polynomial `0x1021`;
- initial value `0xFFFF`;
- no reflection;
- final XOR `0x0000`.

The two CRC octets are part of the CCSDS Packet Data Field length calculation.

---

## 2. PUS revision-A profile

### 2.1 Telecommand secondary header

EXN uses CCSDSPack selector `PUS:revA:TC` with:

- source-ID width: **1 octet**;
- secondary-header spare octets: `0`;
- acknowledgement flags: `0` by default;
- service type: one octet;
- service subtype: one octet.

With this tailoring, the current TC secondary header is four octets on wire: version/ACK field, Service, Subservice, Source ID.

Application data follows immediately after the secondary header. There is no EXN/PUS-A secondary-header application-data length field; CCSDS Packet Data Length owns packet sizing.

### 2.2 Telemetry secondary header

EXN uses CCSDSPack selector `PUS:revA:TM` with:

- destination-ID width: **0 octets**;
- packet subcounter: absent;
- timestamp in secondary header: absent;
- spare octets: `0`;
- service type and subtype present.

With this tailoring, the current TM secondary header is three octets on wire: version/spare field, Service, Subservice.

TM producer identity is carried by APID. Mission timestamps, when required, remain explicit Application Data fields.

### 2.3 Time representation

Where this ICD specifies `ts_cuc`, EXN currently uses a six-octet mission CUC representation in Application Data. Service 17 distributes or reports that application time value. It does not switch the packet to a PUS-C secondary header.

---

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

- TC `3/10` Request System HK is GS -> MCU and its Application Data is exactly `{transactionId, include_mask, detailMask}`.
- A GS request for MCU-local `3/1` HK likewise needs no downstream proxy preamble.

This avoids the previous duplicate `transactionId` definition around System HK.

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

## 5. Current transitional service catalogue

All entries below describe the **currently executable compatibility profile** and use the PUS revision-A header from Section 2. They are preserved so existing EXN-GS/HIL traffic remains documented. They are not the target service allocation for new flight/payload implementations; see [`services.md`](services.md).

| Service/Sub | Name | Packet type | Direct endpoint / APID policy |
|---|---|---|---|
| 3/1 | Request HK | TC | target node APID; GS device request first targets MCU `0x100` with proxy preamble |
| 3/2 | Report HK | TM | producer node APID |
| 3/10 | Request System HK | TC | MCU `0x100`, direct, no proxy preamble |
| 3/100 | System HK Report | TM | MCU `0x100` |
| 5/1 | Event Info | TM | producer node APID |
| 5/2 | Event Warn | TM | producer node APID |
| 5/3 | Event Error | TM | producer node APID |
| 17/1 | Set Time | TC | target node APID; one packet per target |
| 17/2 | Time Report | TM | producer node APID |
| 20/1 | Set Parameter | TC | target node APID; GS device request uses MCU proxy hop |
| 20/2 | Get Parameter | TC | target node APID; GS device request uses MCU proxy hop |
| 20/3 | Parameter Value | TM | producer node APID |
| 23/1 | Start Transfer | TC | data-source node APID; GS request uses MCU proxy hop |
| 23/2 | Stop Transfer | TC | data-source node APID; GS request uses MCU proxy hop |
| 23/10 | Transfer Metadata | TM | data-source node APID |
| 23/11 | Transfer Chunk | TM | data-source node APID |
| 23/12 | Transfer Complete | TM | data-source node APID |
| 200/1 | Camera Capture | TC | PI-CAM `0x101`; GS request uses MCU proxy hop |
| 200/2 | Camera Settings Set | TC | PI-CAM `0x101`; GS request uses MCU proxy hop |
| 200/3 | Camera Settings Get | TC | PI-CAM `0x101`; GS request uses MCU proxy hop |
| 200/4 | Camera Settings Report | TM | PI-CAM `0x101` |
| 200/5 | Camera ACK/NACK | TM | PI-CAM `0x101` |
| 210/1 | Execute | TC | FPGA `0x102`; GS request uses MCU proxy hop |
| 210/2 | Processing Settings Set | TC | FPGA `0x102`; GS request uses MCU proxy hop |
| 210/3 | Processing Settings Get | TC | FPGA `0x102`; GS request uses MCU proxy hop |
| 210/4 | Processing Settings Report | TM | FPGA `0x102` |
| 210/5 | FPGA ACK/NACK | TM | FPGA `0x102` |
| 250/1 | GS Link/Proxy ACK | TM/report | producer GS APID `0x0F0`, routed to MCU |

PI-COMMS/Antenna currently uses common compatibility services where applicable. The target profile adds PI-MGMT, Service 8 recovery, Service 12 monitoring, Service 220 object transfer and Service 230 software management as defined in `services.md` and `component-services.md`.

### 5.1 Housekeeping, Service 3

#### TC 3/1 Request HK

Application Data after any required GS proxy preamble:

| Field | Type | Notes |
|---|---|---|
| `detailMask` | uint16, optional | `0` means default/all groups |

#### TM 3/2 Report HK

| Field | Type | Notes |
|---|---|---|
| `uptime_ms` | uint64 | device uptime |
| `temperature_cC` | int16 | centi-degrees Celsius |
| `status_flags` | uint16 | node-defined health flags |
| `last_error` | uint16 | last error code |
| `ts_cuc` | bytes[6] | report timestamp |

#### TC 3/10 Request System HK

Exactly five Application Data octets, no proxy preamble:

| Field | Type | Notes |
|---|---|---|
| `transactionId` | uint16 | correlation ID; current GS client uses non-zero IDs |
| `include_mask` | uint8 | bit0 MCU, bit1 PI-CAM, bit2 FPGA, bit3 PI-COMMS/Antenna, bit4 PI-MGMT |
| `detailMask` | uint16 | common HK detail selection |

#### TM 3/100 System HK Report

Header fields:

| Field | Type | Notes |
|---|---|---|
| `transactionId` | uint16 | echoes request |
| `present_mask` | uint8 | bit0 MCU, bit1 PI-CAM, bit2 FPGA, bit3 PI-COMMS/Antenna, bit4 PI-MGMT |
| `status` | uint8 | `0=OK,1=PARTIAL,2=TIMEOUT,3=ERROR` |
| `reserved` | uint8 | zero |

Then repeat `[block_len:uint16][node_hk_bytes...]` for each included node.

### 5.2 Event reporting, Service 5

Subservice defines severity: `1=Info`, `2=Warn`, `3=Error`.

Application Data begins with:

| Field | Type | Notes |
|---|---|---|
| `eventId` | uint16 | event identifier |
| `eventData` | bytes[] | event-specific payload |

`eventId` is application data, not a separate PUS-B secondary-header field.

### 5.3 Legacy EXN time assignment, Service 17

Service 17 is **not** Time Management in PUS-C; it is the standardized Test service. The following definitions are therefore transitional EXN assignments and shall migrate to Service 9.

#### TC 17/1 Set Time

Application Data: mission-defined `time_cuc[6]`.

#### TM 17/2 Time Report

Application Data: mission-defined `time_cuc[6]` plus any node-specific quality/status fields defined by the device ICD.

### 5.4 Legacy EXN parameter assignment, Service 20

The current subtype allocation does not match standardized PUS-C Service 20 semantics. It is retained only for compatibility:

- TC 20/1 Set Parameter: `key:uint8` + `value:TLV`.
- TC 20/2 Get Parameter: `key:uint8`.
- TM 20/3 Parameter Value: `key:uint8` + `value:TLV`.

The target mapping uses TC[20,1] to request/report parameter values, TM[20,2] for the value report, and TC[20,3] to set values.

For GS device-directed requests, the four-octet proxy preamble precedes these service fields on the GS -> MCU hop.

### 5.5 Legacy payload transfer assignment, Service 23

Service 23 is standardized as File Management in PUS-C. EXN's current image/result transfer messages are transitional and migrate to mission-specific Service 220 Payload Object Transfer.

- TC 23/1 Start Transfer: `{imageId:uint16, dest:uint8, chunk_size:uint16}`.
- TC 23/2 Stop Transfer: `{imageId:uint16}`.
- TM 23/10 Metadata: `{imageId:uint16,height:uint16,width:uint16,channels:uint8,pixel_type:uint8,total_size:uint32,chunk_size:uint16,ts_cuc[6]}`.
- TM 23/11 Chunk: `{imageId:uint16,offset:uint32,data[]}`.
- TM 23/12 Complete: `{imageId:uint16,totalChunks:uint16}`.

Each 23/11 packet is currently an independent UNSEGMENTED CCSDS Space Packet. `offset` and `imageId` provide application-level reassembly.

For PI-CAM transfers, the current `dest` endpoint codes are `0=FPGA`, `1=MCU`, `2=PI-COMMS`. Other producers may specialize destination semantics in their device ICDs until Service 23 is generalized around a transport-independent object/transfer identifier.

### 5.6 Camera control, Service 200

- TC 200/1 Capture: `{mode:uint8,burst_count:uint16,exposure_us:uint32}`.
- TC 200/2 Settings Set: `key:uint8 + value:TLV`.
- TC 200/3 Settings Get: `key:uint8`.
- TM 200/4 Settings Report: device-defined key/value report.
- TM 200/5 ACK/NACK: `{orig_service:uint8,orig_sub:uint8,resultCode:uint8,detail:uint16}`.

### 5.7 FPGA processing, Service 210

- TC 210/1 Execute: `{pipeline:uint8,modelId:uint16,flags:uint16}`.
- TC 210/2 Settings Set: `key:uint8 + value:TLV`.
- TC 210/3 Settings Get: `key:uint8`.
- TM 210/4 Settings/Result Report: device ICD defines payload.
- TM 210/5 ACK/NACK: `{orig_service:uint8,orig_sub:uint8,resultCode:uint8,detail:uint16}`.

FPGA processing results shall **not** be hard-wired to one consumer. Service 210 remains a valid mission-specific service type. The target profile moves request verification to standard Service 1 and makes result routing an explicit job/policy property; see `services.md`.

### 5.8 GS link/proxy acknowledgement, Service 250

TM/report 250/1 is produced by GS when an application-level GS acknowledgement is required:

| Field | Type | Notes |
|---|---|---|
| `transactionId` | uint16 | associated proxy transaction |
| `ackCode` | uint8 | `0=Accepted,1=Rejected,2=Cancelled,3=Invalid,4=Timeout` |
| `detail` | uint16 | implementation-defined detail |

Because EXN Packet Type describes requesting vs reporting semantics rather than physical link direction, the transitional profile permits a GS-originated reporting packet with Packet Type `TM` and APID `0x0F0`. This Service 250 behavior is **legacy/HIL compatibility**, not part of the target PUS-C mission profile; request verification migrates to Service 1 and ground-daemon acknowledgements remain local IPC.

---

## 6. CCSDSPack v2 configuration mapping

Authoritative `.cfg` templates for the **transitional PUS-A compatibility profile** are under `interfaces/ccsdspack/`. They shall be migrated as a coordinated interface change after the PUS-C service profile is frozen.

Current TC templates use:

```text
ccsds_packet_error_control:string=crc16
ccsds_version_number:int=0
ccsds_segmented:bool=false
define_secondary_header:bool=true
secondary_header_type:string=PUS:revA:TC
pus_source_id_octets:int=1
secondary_header_spare_octets:int=0
pus_acknowledgement_flags:uint=0
```

Current TM templates use:

```text
ccsds_packet_error_control:string=crc16
ccsds_version_number:int=0
ccsds_segmented:bool=false
define_secondary_header:bool=true
secondary_header_type:string=PUS:revA:TM
pus_destination_id_octets:int=0
secondary_header_spare_octets:int=0
pus_destination_id:uint=0
```

Direction and Secondary Header Flag come from the installed directional secondary-header object. Legacy EXN keys such as `ccsds_type`, `ccsds_data_field_header_flag`, `pus_version`, `pus_service_sub_type`, and selectors `PusA/PusB/PusC` are not part of the EXN v2 profile.

---

## 7. Implementation invariants

Implementations and tests shall enforce at least the following:

1. CCSDS Packet Version Number is `0`.
2. Packet Data Length equals `packet_size - 7` on a complete packet.
3. Packets in the current compatibility baseline have a PUS revision-A secondary header; the target coherent profile migrates to PUS-C.
4. TC source-ID width is one octet; TM destination-ID width is zero octets.
5. CRC16 is included and validated at packet endpoints.
6. TC APID follows destination-endpoint policy; TM APID follows producer-endpoint policy.
7. GS -> MCU System HK `3/10` carries exactly the five service payload octets and no proxy preamble.
8. The GS transport daemon and PI-COMMS service may move packets, but mission scheduling and control decisions remain application/OBC responsibilities.
9. FPGA processing-result routing is configurable and shall not be implemented as one fixed destination.
10. Physical co-location of PI-CAM, PI-COMMS and PI-MGMT shall not collapse their logical endpoint identities.
11. No new implementation shall introduce additional use of the legacy Service 17 time, Service 20 subtype, or Service 23 payload-transfer assignments.
12. Target service semantics and mission-specific allocations shall conform to `docs/services.md`.

Device-specific constraints remain in:

- `docs/icd/gs.md`
- `docs/icd/mcu-rtos.md`
- `docs/icd/linux-payload.md`
- `docs/icd/pi-cam.md`
- `docs/icd/fpga-ai.md`
