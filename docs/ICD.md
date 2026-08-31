# ICD — EXN CCSDS/PUS Interface Control Document

This document is the wire-level source of truth for communication between EXN nodes. Device-specific behavior and parameter ranges live under `docs/icd/`; packet identifiers, header policy, routing rules, service identifiers, and common application-data layouts are defined here.

## 0. Current protocol baseline

EXN currently uses:

- CCSDS Space Packet Protocol, Packet Version Number `0`;
- CCSDSPack **v2.x**, with **v2.0.0** as the initial migration baseline;
- **PUS revision A** TC/TM secondary headers for all currently defined EXN services;
- CRC-16/CCITT-FALSE packet error control;
- big-endian application-data integers;
- unsegmented CCSDS Space Packets by default. Service 23 chunking is application-level chunking, not CCSDS packet segmentation.

PUS revision and PUS service number are separate concepts. Service 5 (events) and Service 17 (time) therefore use the same PUS revision-A secondary-header profile as the other current services. EXN does not define a PUS-B secondary-header format, and Service 17 does not imply a PUS-C header.

### Nodes

| Node | APID | PUS-A TC Source ID | Role |
|---|---:|---:|---|
| GS | `0x0F0` | `0x10` | Ground station/operator client |
| MCU-RTOS | `0x100` | `0x01` | Control/router node and time master |
| PI-CAM | `0x101` | `0x02` | Camera/image source |
| FPGA-AI | `0x102` | `0x03` | Processing/inference node |

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
- PI -> MCU/GS camera ACK: TM APID `0x101`.
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

## 3. GS/MCU routing and correlation

The GS physical/simulator link terminates at MCU-RTOS. Device-directed commands are therefore two packet hops:

1. **GS -> MCU:** TC APID `0x100`, Source ID `0x10`. For downstream-device operations, Application Data begins with the proxy preamble.
2. **MCU -> target:** MCU consumes the proxy preamble and creates a new TC using target APID `0x101` or `0x102` and Source ID `0x01`.

Device TMs retain their source APID while MCU forwards/routes them to GS where the transport permits it.

### 3.1 Proxy preamble

The proxy preamble is exactly four octets and is used only on GS-originated TCs that MCU must re-issue to PI-CAM or FPGA-AI.

| Field | Type | Bits | Meaning |
|---|---|---:|---|
| `transactionId` | uint16 | 16 | Correlation ID, `1..65535`; `0` reserved |
| `target` | uint8 | 8 | `1=PI`, `2=FPGA` |
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

## 5. Service catalogue

All entries below use the PUS revision-A profile from Section 2.

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
| 200/1 | Camera Capture | TC | PI `0x101`; GS request uses MCU proxy hop |
| 200/2 | Camera Settings Set | TC | PI `0x101`; GS request uses MCU proxy hop |
| 200/3 | Camera Settings Get | TC | PI `0x101`; GS request uses MCU proxy hop |
| 200/4 | Camera Settings Report | TM | PI `0x101` |
| 200/5 | Camera ACK/NACK | TM | PI `0x101` |
| 210/1 | Execute | TC | FPGA `0x102`; GS request uses MCU proxy hop |
| 210/2 | Processing Settings Set | TC | FPGA `0x102`; GS request uses MCU proxy hop |
| 210/3 | Processing Settings Get | TC | FPGA `0x102`; GS request uses MCU proxy hop |
| 210/4 | Processing Settings Report | TM | FPGA `0x102` |
| 210/5 | FPGA ACK/NACK | TM | FPGA `0x102` |
| 250/1 | GS Link/Proxy ACK | TM/report | producer GS APID `0x0F0`, routed to MCU |

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
| `include_mask` | uint8 | bit0 MCU, bit1 PI, bit2 FPGA |
| `detailMask` | uint16 | common HK detail selection |

#### TM 3/100 System HK Report

Header fields:

| Field | Type | Notes |
|---|---|---|
| `transactionId` | uint16 | echoes request |
| `present_mask` | uint8 | bit0 MCU, bit1 PI, bit2 FPGA |
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

### 5.3 Time management, Service 17

#### TC 17/1 Set Time

Application Data: mission-defined `time_cuc[6]`.

#### TM 17/2 Time Report

Application Data: mission-defined `time_cuc[6]` plus any node-specific quality/status fields defined by the device ICD.

### 5.4 Parameter management, Service 20

- TC 20/1 Set Parameter: `key:uint8` + `value:TLV`.
- TC 20/2 Get Parameter: `key:uint8`.
- TM 20/3 Parameter Value: `key:uint8` + `value:TLV`.

For GS device-directed requests, the four-octet proxy preamble precedes these service fields on the GS -> MCU hop.

### 5.5 Data transfer, Service 23

- TC 23/1 Start Transfer: `{imageId:uint16, dest:uint8, chunk_size:uint16}`.
- TC 23/2 Stop Transfer: `{imageId:uint16}`.
- TM 23/10 Metadata: `{imageId:uint16,height:uint16,width:uint16,channels:uint8,pixel_type:uint8,total_size:uint32,chunk_size:uint16,ts_cuc[6]}`.
- TM 23/11 Chunk: `{imageId:uint16,offset:uint32,data[]}`.
- TM 23/12 Complete: `{imageId:uint16,totalChunks:uint16}`.

Each 23/11 packet is currently an independent UNSEGMENTED CCSDS Space Packet. `offset` and `imageId` provide application-level reassembly.

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

### 5.8 GS link/proxy acknowledgement, Service 250

TM/report 250/1 is produced by GS when an application-level GS acknowledgement is required:

| Field | Type | Notes |
|---|---|---|
| `transactionId` | uint16 | associated proxy transaction |
| `ackCode` | uint8 | `0=Accepted,1=Rejected,2=Cancelled,3=Invalid,4=Timeout` |
| `detail` | uint16 | implementation-defined detail |

Because EXN Packet Type describes requesting vs reporting semantics rather than physical link direction, a GS-originated reporting packet may have Packet Type `TM` and APID `0x0F0`.

---

## 6. CCSDSPack v2 configuration mapping

Authoritative `.cfg` templates are under `interfaces/ccsdspack/`.

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
3. Current EXN application packets have a PUS revision-A secondary header.
4. TC source-ID width is one octet; TM destination-ID width is zero octets.
5. CRC16 is included and validated at packet endpoints.
6. TC APID follows destination-endpoint policy; TM APID follows producer-endpoint policy.
7. GS -> MCU System HK `3/10` carries exactly the five service payload octets and no proxy preamble.
8. The GS transport daemon may route packets, but mission scheduling and periodic command generation belong to clients/applications, not the transport layer.

Device-specific constraints remain in:

- `docs/icd/gs.md`
- `docs/icd/mcu-rtos.md`
- `docs/icd/pi-cam.md`
- `docs/icd/fpga-ai.md`
