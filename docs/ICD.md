
# ICD — ASTRAi CCSDS/PUS Interface Control Document

This document defines all TeleCommands (TC) and TeleMetry (TM) messages exchanged between the ASTRAi nodes. All messages are CCSDS Space Packets and use PUS-style secondary headers as implemented by CCSDSPack.

### About this document
- Purpose: Serve as the single source of truth for how nodes in ASTRAi communicate, including packet formats, field sizes, allowed values, and semantics. It is implementation‑agnostic but directly mappable to CCSDSPack templates for generation and validation.
- Scope: CCSDS primary headers, PUS secondary headers (A/B/C), services and subservices, application data definitions, acknowledgements/errors, and segmentation policies.
- Audience: Firmware developers (MCU/FPGA/PI), test/integration engineers, and tool authors creating CCSDSPack configurations.
- Related: Device/service‑specific ICDs with concrete parameter keys and ranges are provided alongside this document at `docs/icd/`.

- Nodes and APIDs:
  - 0x0F0 (240): GS — Ground Station (PC simulator; operator console)
  - 0x100 (256): MCU-RTOS — Control node (TC originator, time master)
  - 0x101 (257): PI-CAM — Camera node (image source)
  - 0x102 (258): FPGA-AI — Processing node (pre/inference/post)

- Endianness: Unless explicitly stated, multi-byte integers in Application Data are big-endian (network order) to align with CCSDS practice.
- CRC: CRC-16/CCITT (poly 0x1021, init 0xFFFF, final XOR 0x0000). CCSDSPack computes and appends CRC in the data field as configured.
- Time: Recommend CCSDS CUC (coarse+fine) 6-byte time where timestamps are included. Alternatively, UNIX epoch seconds(4)/nanos(4) MAY be used if agreed.

---

## 1. CCSDS Primary Header (6 bytes)
All packets shall include the CCSDS Primary Header with the following fields:

| Field                     | Bits | Value/Range                          | Description |
|---------------------------|------|--------------------------------------|-------------|
| Version                   | 3    | 0                                    | CCSDS version |
| Type                      | 1    | 1=TC, 0=TM                           | Telecommand vs Telemetry |
| Secondary Header Flag     | 1    | 1                                    | Always present in ASTRAi |
| APID                      | 11   | 0x100 MCU, 0x101 PI, 0x102 FPGA      | Application Process ID |
| Sequence Flags            | 2    | 3=UNSEG, 1=FIRST, 0=CONT, 2=LAST     | Segmentation state |
| Sequence Count            | 14   | 0..16383 (per-APID counter)          | Increment per packet sent by an APID |
| Data Length               | 16   | 0..65535                              | Bytes in Data Field (Secondary Header + App Data [+ CRC]) |

Policy:
- Sequence counter rolls over per-sender APID. For chunked transfers, flags use FIRST/CONT/LAST; otherwise UNSEGMENTED.

---

## 2. Ground Station Control Model and Routing
The Ground Station (GS, APID 0x0F0, SourceID 0x10) is an operator console simulated on a PC. GS communicates ONLY with MCU-RTOS (APID 0x100). The MCU acts as a bridge/proxy to downstream devices (PI 0x101, FPGA 0x102):
- GS→MCU: GS sends TCs including HK requests (Service 3/1) and System HK requests (Service 3/10), as well as device control (Params, Camera, FPGA, Data Transfer). The MCU proxies device-directed HK/TCs to the target APIDs and manages responses; for System HK, the MCU aggregates device HKs.
- Device→MCU→GS: Devices send TMs to MCU. For GS-initiated operations, MCU forwards relevant ACKs/TMs upstream to GS. The forwarded packets retain the device’s APID in the primary header when link permits; if a single GS link requires one APID, MCU may encapsulate device APID in the Application Data preamble (see Correlation/Transaction ID).
- Authority: By default, GS has authority to request any TC. Implementations may restrict via policy (out of scope of ICD).

### 2.1 Correlation/Transaction ID (optional but RECOMMENDED for GS-driven flows)
To correlate multi-packet sequences initiated by GS, include a `transactionId` field as the first item in Application Data of the initiating TC, and echo it in all related ACKs/TMs:
- Type/Size: `transactionId` is `uint16` (16 bits). Range 1..65535; 0 is reserved.
- Placement: If present, it MUST be the first field of the initiating TC’s Application Data. Responders MUST echo it as the first field in corresponding ACKs and completion TMs for that transaction.
- Mapping: For data transfers or executes that already have `imageId`/`jobId`, `transactionId` may be the same as those IDs or distinct.

## 3. PUS Secondary Headers (CCSDSPack)
ASTRAi uses PUS-like headers provided by CCSDSPack.

### 3.1 PUS-A (6 bytes)
| Field          | Bits | Description |
|----------------|------|-------------|
| Version        | 3    | PUS version (set to 1) |
| Service Type   | 8    | Functional service (e.g., 3=HK, 23=Data) |
| Subservice     | 8    | Specific request/indication under the service |
| Source ID      | 8    | Origin logical ID (MCU=0x01, PI=0x02, FPGA=0x03) |
| Data Length    | 16   | Length of application data in bytes |

Usage: General TC/TM requests and reports (HK, parameters, device control, data transfer).

### 3.2 PUS-B (8 bytes)
| Field          | Bits | Description |
|----------------|------|-------------|
| Version        | 3    | PUS version (set to 1) |
| Service Type   | 8    | 5=Event Reporting |
| Subservice     | 8    | Severity/type (1=Info,2=Warn,3=Error) |
| Source ID      | 8    | Origin logical ID |
| Event ID       | 16   | Event identifier |
| Data Length    | 16   | Length of event data |

Usage: Errors, warnings, and notable events.

### 3.3 PUS-C (variable, ≥6 bytes)
| Field          | Bits | Description |
|----------------|------|-------------|
| Version        | 3    | PUS version (set to 1) |
| Service Type   | 8    | 17=Time Management |
| Subservice     | 8    | 1=SetTime, 2=TimeReport |
| Source ID      | 8    | Origin logical ID |
| Time Code      | n×8  | Time code bytes (e.g., 6-byte CUC) |
| Data Length    | 16   | Length of time payload (not counting PUS-C header itself) |

Usage: Time distribution and synchronization when used.

---

## 4. Common Enumerations and Conventions
- Source IDs: GS=0x10, MCU=0x01, PI=0x02, FPGA=0x03
- Result Codes (ACK/NACK): 0=OK, 1=INVALID, 2=BUSY, 3=UNSUPPORTED, 4=TIMEOUT, 5=INTERNAL
- Pixel Types:
  - 1=RGB888, 2=GRAY8, 3=GRAY16, 4=YUV420, 5=BAYER_RGGB8
- Endianness of Application Data fields: Big-endian unless explicitly noted.

---

### 4.1 Quick TC/TM List
Below is a compact list of all TeleCommands (TC) and TeleMetry (TM). See Section 5 for full field tables.

| Svc | Sub | Name                         | Direction        | APID (src→dst)              | PUS |
|-----|-----|------------------------------|------------------|-----------------------------|-----|
| 3   | 1   | Request HK                   | MCU→Device, GS→MCU (proxy) | 0x100→0x101/0x102, 0x0F0→0x100 | A   |
| 3   | 2   | Report HK                    | Device→MCU→GS    | 0x101/0x102→0x100(bridge to 0x0F0) | A   |
| 5   | 1   | Event Info                   | Device→MCU→GS    | any→0x100(bridge to 0x0F0)  | B   |
| 5   | 2   | Event Warn                   | Device→MCU→GS    | any→0x100(bridge to 0x0F0)  | B   |
| 5   | 3   | Event Error                  | Device→MCU→GS    | any→0x100(bridge to 0x0F0)  | B   |
| 17  | 1   | Set Time                     | MCU→All          | 0x100→all                   | C   |
| 17  | 2   | Time Report                  | Any→MCU→GS       | any→0x100(bridge to 0x0F0)  | C   |
| 3   | 10  | Request System HK            | GS→MCU           | 0x0F0→0x100                 | A   |
| 3   | 100 | System HK Report             | MCU→GS           | 0x100→0x0F0                 | A   |
| 20  | 1   | Set Parameter                | MCU→Device, GS→MCU (proxy) | 0x100→0x101/0x102, 0x0F0→0x100 | A   |
| 20  | 2   | Get Parameter                | MCU→Device, GS→MCU (proxy) | 0x100→0x101/0x102, 0x0F0→0x100 | A   |
| 20  | 3   | Parameter Value              | Device→MCU→GS    | 0x101/0x102→0x100(bridge to 0x0F0) | A   |
| 23  | 1   | Start Transfer               | MCU→PI/FPGA, GS→MCU (proxy) | 0x100→0x101/0x102, 0x0F0→0x100 | A   |
| 23  | 2   | Stop Transfer                | MCU→PI/FPGA, GS→MCU (proxy) | 0x100→0x101/0x102, 0x0F0→0x100 | A   |
| 23  | 10  | Metadata                     | PI→MCU→GS/FPGA   | 0x101→0x100(bridge to 0x0F0/0x102) | A   |
| 23  | 11  | Data Chunk                   | PI→MCU→GS/FPGA   | 0x101→0x100(bridge to 0x0F0/0x102) | A   |
| 23  | 12  | Transfer Complete            | PI→MCU→GS/FPGA   | 0x101→0x100(bridge to 0x0F0/0x102) | A   |
| 200 | 1   | Capture                      | MCU→PI, GS→MCU (proxy) | 0x100→0x101, 0x0F0→0x100 | A   |
| 200 | 2   | Camera Settings Set          | MCU→PI, GS→MCU (proxy) | 0x100→0x101, 0x0F0→0x100 | A   |
| 200 | 3   | Camera Settings Get          | MCU→PI, GS→MCU (proxy) | 0x100→0x101, 0x0F0→0x100 | A   |
| 200 | 4   | Camera Settings Report       | PI→MCU→GS        | 0x101→0x100(bridge to 0x0F0) | A   |
| 200 | 5   | Camera ACK/NACK              | PI→MCU→GS        | 0x101→0x100(bridge to 0x0F0) | A   |
| 210 | 1   | Execute                      | MCU→FPGA, GS→MCU (proxy) | 0x100→0x102, 0x0F0→0x100 | A   |
| 210 | 2   | Proc Settings Set            | MCU→FPGA, GS→MCU (proxy) | 0x100→0x102, 0x0F0→0x100 | A   |
| 210 | 3   | Proc Settings Get            | MCU→FPGA, GS→MCU (proxy) | 0x100→0x102, 0x0F0→0x100 | A   |
| 210 | 4   | Proc Settings Report         | FPGA→MCU→GS      | 0x102→0x100(bridge to 0x0F0) | A   |
| 210 | 5   | FPGA ACK/NACK                | FPGA→MCU→GS      | 0x102→0x100(bridge to 0x0F0) | A   |
| 250 | 1   | Link/Proxy ACK               | GS→MCU           | 0x0F0→0x100                  | A   |

See device/service‑specific details:
- MCU-RTOS: docs/icd/mcu-rtos.md
- PI-CAM (Raspberry Pi 5 + Camera Module 3): docs/icd/pi-cam.md
- FPGA-AI: docs/icd/fpga-ai.md
- GS (Ground Station Simulator): docs/icd/gs.md

### 4.2 GS Service Usage Profile
- GS sends: 3/1 (HK Request), 3/10 (System HK Request), 20/1–2, 200/1–3, 210/1–3, 23/1–2 as TeleCommands to MCU (APID 0x0F0→0x100). The MCU proxies device-directed TCs to PI/FPGA and aggregates System HK.
- GS receives: 3/2 (forwarded device HK), 3/100 (System HK Report), 20/3, 200/4–5, 210/4–5, 23/10–12, and 5/1–3 from MCU as forwarded/aggregated TeleMetry.
- GS returns link/proxy ACK/NACK to MCU using Service 250/1 (see Section 5.7) to indicate UI-side acceptance or command cancellation.
- Correlation: If `transactionId` is present in a GS TC, MCU MUST echo it in proxied TCs and in any forwarded ACK/TM as first field of Application Data.
- Timing: Unless specified otherwise in device ICDs, immediate device ACK expected within 100 ms on MCU link; completion/aggregation timing depends on operation (e.g., image capture size; System HK aggregation timeout ~200–600 ms).

## 5. Service Catalog (TC/TM)
Each entry defines: Direction, APID, PUS header type, and Application Data fields.

### 5.1 Housekeeping (Service 3, PUS-A)
- TC 3/1 Request HK
  - Direction: MCU→Device (APID target: 0x101 or 0x102); GS→MCU (proxy) allowed using Proxy Preamble (see GS ICD)
  - App Data: (optional) `detailMask` (uint16, 16 bits) — which groups to include
- TM 3/2 Report HK
  - Direction: Device→MCU (APID source: PI=0x101, FPGA=0x102)
  - App Data:
  
    | Name            | Type   | Bits | Description |
    |-----------------|--------|------|-------------|
    | uptime_ms       | uint64 | 64   | Device uptime |
    | temperature_cC  | int16  | 16   | Temperature in centi-deg C |
    | status_flags    | uint16 | 16   | Bitmask (health flags) |
    | last_error      | uint16 | 16   | Last error code |
    | ts_cuc          | bytes  | 48   | 6-byte CUC timestamp |

- TC 3/10 Request System HK (GS-driven)
  - Direction: GS→MCU (APID 0x0F0→0x100)
  - App Data (big-endian):
  
    | Name          | Type   | Bits | Description |
    |---------------|--------|------|-------------|
    | transactionId | uint16 | 16   | Correlates with response; 0 if unused |
    | include_mask  | uint8  | 8    | Bitmask: [0]=MCU, [1]=PI, [2]=FPGA; other bits reserved 0 |
    | detailMask    | uint16 | 16   | Optional per-device HK detail mask (applied to all) |

- TM 3/100 System HK Report (aggregated)
  - Direction: MCU→GS (APID 0x100→0x0F0)
  - App Data (big-endian):
  
    | Name          | Type   | Bits | Description |
    |---------------|--------|------|-------------|
    | transactionId | uint16 | 16   | Echo of request, 0 if unused |
    | present_mask  | uint8  | 8    | Bitmask of which blocks are present: [0]=MCU, [1]=PI, [2]=FPGA |
    | status        | uint8  | 8    | 0=OK, 1=PARTIAL, 2=TIMEOUT, 3=ERROR |
    | reserved      | uint8  | 8    | 0 |
    | blocks        | bytes  | var  | Concatenated node blocks in order MCU, PI, FPGA when present |
  
  - Node block encoding (when present), each prefixed with `block_len:uint16` then the node’s TM 3/2 HK payload as defined in the respective device ICD. If a node timed out, a minimal block MAY be included with `block_len=0`.
  - Timing: MCU SHOULD wait up to `T_hk_timeout` per device (recommended 200 ms) with a total cap of 600 ms before emitting TM 3/100. Partial results are allowed with `status=PARTIAL` and appropriate `present_mask` bits set only for received HKs.

### 5.2 Time Management (Service 17, PUS-C)
- TC 17/1 Set Time
  - Direction: MCU→All
  - PUS-C Time Code: 6-byte CUC (recommended) or agreed format
  - App Data: none
- TM 17/2 Time Report (optional periodic)
  - Direction: Any→MCU
  - PUS-C Time Code: current device time
  - App Data: none

### 5.3 Parameter Management (Service 20, PUS-A)
- TC 20/1 Set Parameter
  - Direction: MCU→Device
  - App Data:
  
    | Name  | Type  | Bits | Description |
    |-------|-------|------|-------------|
    | key   | uint8 | 8    | Parameter key (device-specific) |
    | value | TLV   | var  | Encoded value (Type,Length,Value) |
  
- TC 20/2 Get Parameter
  - Direction: MCU→Device
  - App Data: `key` (uint8, 8)
- TM 20/3 Parameter Value
  - Direction: Device→MCU
  - App Data:

    | Name   | Type  | Bits | Description |
    |--------|-------|------|-------------|
    | key    | uint8 | 8    | Parameter key |
    | status | uint8 | 8    | 0=OK or error code |
    | value  | TLV   | var  | Encoded value |

### 5.4 Camera Control (Service 200, PUS-A, APID 0x101)
- TC 200/1 Capture
  - App Data:
  
    | Name         | Type   | Bits | Description |
    |--------------|--------|------|-------------|
    | mode         | uint8  | 8    | 0=single, 1=burst |
    | burst_count  | uint16 | 16   | Number of frames if burst |
    | exposure_us  | uint32 | 32   | Optional exposure (0=default) |

- TC 200/2 Camera Settings Set
  - App Data: `key`(uint8), `value`(TLV)
- TC 200/3 Camera Settings Get
  - App Data: `key`(uint8)
- TM 200/4 Camera Settings Report
  - App Data: `key`(uint8), `status`(uint8), `value`(TLV)
- TM 200/5 ACK/NACK
  - App Data:

    | Name         | Type  | Bits | Description |
    |--------------|-------|------|-------------|
    | orig_service | uint8 | 8    | Service of original TC |
    | orig_sub     | uint8 | 8    | Subservice of original TC |
    | resultCode   | uint8 | 8    | 0=OK, >0=error |
    | detail       | uint16| 16   | Optional error detail |

### 5.5 FPGA Control (Service 210, PUS-A, APID 0x102)
- TC 210/1 Execute
  - App Data:

    | Name     | Type   | Bits | Description |
    |----------|--------|------|-------------|
    | pipeline | uint8  | 8    | 0=pre,1=infer,2=post,3=full |
    | modelId  | uint16 | 16   | Model selection |
    | flags    | uint16 | 16   | Bitfield (options) |
  
- TC 210/2 Proc Settings Set — `key`(uint8), `value`(TLV)
- TC 210/3 Proc Settings Get — `key`(uint8)
- TM 210/4 Proc Settings Report — `key`(uint8), `status`(uint8), `value`(TLV)
- TM 210/5 ACK/NACK — same structure as 200/5

### 5.6 Data Transfer (Service 23, PUS-A)
Used by PI-CAM (and optionally FPGA) to stream large data as a series of packets.

- TC 23/1 Start Transfer
  - Direction: MCU→PI (or MCU→FPGA)
  - App Data:

    | Name       | Type   | Bits | Description |
    |------------|--------|------|-------------|
    | imageId    | uint16 | 16   | Transfer identifier |
    | dest       | uint8  | 8    | 0=FPGA, 1=MCU |
    | chunk_size | uint16 | 16   | Preferred chunk size (bytes) |
  
- TC 23/2 Stop Transfer
  - App Data: `imageId` (uint16)
- TM 23/10 Metadata (first packet for a stream)
  - Direction: PI→Destination
  - App Data:
  
    | Name        | Type   | Bits | Description |
    |-------------|--------|------|-------------|
    | imageId     | uint16 | 16   | Transfer identifier |
    | height      | uint16 | 16   | Pixels |
    | width       | uint16 | 16   | Pixels |
    | channels    | uint8  | 8    | 1..4 |
    | pixel_type  | uint8  | 8    | See Pixel Types |
    | total_size  | uint32 | 32   | Bytes of image payload |
    | chunk_size  | uint16 | 16   | Bytes per data chunk |
    | ts_cuc      | bytes  | 48   | 6-byte timestamp |
  
- TM 23/11 Data Chunk (repeating)
  - Direction: PI→Destination
  - Primary Header Sequence Flags: typically UNSEGMENTED, app-level chunking controls order. Optionally use FIRST/CONT/LAST for link-layer segmentation coherence.
  - App Data:
  
    | Name    | Type   | Bits | Description |
    |---------|--------|------|-------------|
    | imageId | uint16 | 16   | Transfer identifier |
    | offset  | uint32 | 32   | Byte offset into image |
    | data    | bytes  | var  | Up to negotiated `chunk_size` |
  
- TM 23/12 Transfer Complete
  - Direction: PI→Destination
  - App Data:
  
    | Name        | Type   | Bits | Description |
    |-------------|--------|------|-------------|
    | imageId     | uint16 | 16   | Transfer identifier |
    | totalChunks | uint16 | 16   | Number of chunks sent |

---

### 5.7 GS Link/Proxy ACK (Service 250, PUS-A)
- Purpose: GS acknowledges receipt/handling of a GS-originated TC at the UI/link layer (not device execution status).
- Direction: GS→MCU (APID 0x0F0→0x100)
- Subservice: 1 (ACK/NACK)
- Application Data:

| Name           | Type   | Bits | Description |
|----------------|--------|------|-------------|
| transactionId  | uint16 | 16   | Correlates with GS-initiated TC (if used). 0 if not applicable. |
| ackCode        | uint8  | 8    | 0=Accepted, 1=Rejected, 2=Cancelled, 3=Invalid, 4=Timeout |
| detail         | uint16 | 16   | Optional reason/detail code |

Notes:
- Does not replace device ACKs (e.g., 200/5, 210/5); it complements them to reflect GS-side state.

---

## 6. Acknowledgement & Error Handling
- Every TC SHOULD receive an immediate ACK/NACK TM (Service 200/5 or 210/5, or Service-specific ACK if defined) with `resultCode` and optional `detail`.
- Completion reports are sent as the final TM of a sequence (e.g., 23/12), or as Events (Service 5) when errors occur mid-flow.

### 6.1 Event Reporting (Service 5, PUS-B)
- TM 5/1 Info, 5/2 Warn, 5/3 Error
  - App Data (examples):
  
    | Name       | Type   | Bits | Description |
    |------------|--------|------|-------------|
    | eventId    | uint16 | 16   | Event identifier |
    | code       | uint16 | 16   | Error/Info code |
    | detail     | uint32 | 32   | Extra context |
    | ts_cuc     | bytes  | 48   | 6-byte timestamp |

---

## 7. Transport Bindings & Segmentation
- Links: SPI, UART, CAN, or UDP. Implementations MAY wrap CCSDS packets in a simple framing (e.g., SLIP) per link.
- Recommended application data chunk size: 800–1000 bytes for UART/CAN; higher acceptable for SPI/UDP by agreement.
- Primary Header `Sequence Flags` MAY be used to mirror multi-packet segmentation at CCSDS level when underlying link needs it; otherwise keep UNSEGMENTED and rely on app-level chunking with `offset`.

---

## 8. Interfaces (CCSDSPack & JSON)
Predefined packet interfaces are provided in the repository to accelerate development and ensure alignment with this ICD.

- CCSDSPack .cfg interfaces:
  - TeleCommands: `interfaces/ccsdspack/tc/`
  - TeleMetry:    `interfaces/ccsdspack/tm/`
- JSON mirrors (for tooling/generation):
  - TeleCommands: `interfaces/json/tc/`
  - TeleMetry:    `interfaces/json/tm/`
- MCU-RTOS (no filesystem):
  - Header-only definitions: `interfaces/mcu-rtos/`

Examples (by filename):
- TC: `pkt_hk_req_tc.cfg`, `pkt_system_hk_req_tc.cfg`, `pkt_time_set_tc.cfg`, `pkt_param_set_tc.cfg`, `pkt_param_get_tc.cfg`, `pkt_cam_capture_tc.cfg`, `pkt_fpga_exec_tc.cfg`, `pkt_xfer_start_tc.cfg`, `pkt_xfer_stop_tc.cfg`
- TM: `pkt_hk_report_tm.cfg`, `pkt_system_hk_tm.cfg`, `pkt_param_val_tm.cfg`, `pkt_cam_ack_tm.cfg`, `pkt_xfer_meta_tm.cfg`, `pkt_xfer_chunk_tm.cfg`, `pkt_xfer_done_tm.cfg`, `pkt_gs_link_ack_tm.cfg`

Notes:
- Primary Header defaults reflect typical APIDs; adjust as needed per node.
- Secondary Header fields (PUS-A/B/C) and Application Data placeholders match the field tables in this ICD.
- On STM32 MCU builds, include the header-only interfaces from `interfaces/mcu-rtos/` to construct packets programmatically without loading configs.

---

## 9. Operational Scenarios

### 9.1 Capture → Transfer to FPGA → Inference → Result (GS-driven)
Assumptions: GS APID 0x0F0, MCU 0x100, PI 0x101, FPGA 0x102; `transactionId=42`, `imageId=100`.

1) GS→MCU: TC 200/1 Capture (PUS-A)
   - Primary: Type=TC, APID=0x0F0
   - App Data: `[transactionId:uint16=42][target:uint8=1][options:uint8=0] + {mode=0, burst_count=1, exposure_us=0}`
   - MCU proxies to PI as TC 200/1 (without preamble). PI→MCU emits TM 200/5 ACK; MCU forwards to GS and prepends `[transactionId=42]`.

2) GS→MCU: TC 23/1 Start Transfer (PUS-A)
   - App Data: `[42][1][options bit0=1] + {imageId=100, dest=0 /*FPGA*/, chunk_size=900}`
   - PI→MCU: TM 23/10 Metadata, then TM 23/11 Chunks, then TM 23/12 Complete.
   - MCU forwards to FPGA (dest=FPGA) and mirrors to GS because options bit0=1; all forwarded TMs include `[transactionId=42]` as the first field of Application Data.

3) GS→MCU: TC 210/1 Execute (PUS-A)
   - App Data: `[42][2][0] + {pipeline=3, modelId=1, flags=0}`
   - FPGA→MCU: TM 210/5 ACK, followed by result TMs per FPGA ICD; MCU forwards to GS with `[transactionId=42]`.

4) Completion/Errors
   - Final TM 23/12 indicates image transfer completion; FPGA result TM indicates inference complete. Any errors: TM 5/x Event forwarded to GS with `[transactionId=42]` if applicable.

Timing: Immediate ACKs within ~100 ms on MCU link; image transfer timing depends on chunk size/MTU; inference timing is model-dependent.

---

## 10. Interfaces Reference
For predefined packet interfaces (CCSDSPack .cfg, JSON, and MCU header-only), see Section 8 and the repository paths under `interfaces/`.

---

## 11. Revision Notes
- This ICD corrects typos and clarifies CCSDS/PUS usage compared to the initial draft.
- Added GS node (APID 0x0F0) and routing/correlation model.
- Added operational scenario for GS-driven end-to-end flow.
- Future extensions: define comprehensive parameter key tables for Camera/FPGA, event ID registry, and per-link framing details.

