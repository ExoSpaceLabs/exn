# Ground Station (GS) Device/Service ICD

This document specializes the EXN ICD for the Ground Station node simulated on a PC.
It defines the GS role and the set of TeleCommands it can issue to the MCU-RTOS for proxying to downstream devices (PI-CAM and FPGA-AI). The GS can request Housekeeping (HK) individually or as a System HK aggregation via the MCU, and can send link/proxy ACK/NACK to MCU.

See master CCSDS/PUS conventions and service catalog in `../ICD.md`.

---

## 1. Role and Interfaces
- Acts as the operator console and test harness.
- Sends TCs to MCU-RTOS (APID 0x100). MCU proxies these to PI (0x101) and FPGA (0x102).
- Receives forwarded TMs (ACKs, reports, data) from MCU.
- APID: 0x0F0 (240)
- PUS Source ID: 0x10
- Transport: typically UDP or serial. Framing per `ICD.md` Transport section.

Authority and safety: GS is assumed authorized to issue all TCs during development; deployments may restrict via policy (out of scope for this ICD).

---

## 2. GS↔MCU Proxy Preamble
When GS sends a TC that is intended for a downstream device (HK, Params, Camera, FPGA, Transfer), it SHOULD prepend a short proxy preamble to the Application Data so MCU can route and correlate responses.

- Fields (big-endian):

| Name           | Type   | Bits | Description |
|----------------|--------|------|-------------|
| transactionId  | uint16 | 16   | Correlates multi-packet flows. Range 1..65535; 0 reserved. Echoed in all ACKs/TMs for this transaction. |
| target         | uint8  | 8    | 1=PI-CAM (0x101), 2=FPGA-AI (0x102) |
| options        | uint8  | 8    | Bitmask: [0]=mirror TM to GS, [1]=mirror TM to device peer (for PI→FPGA direct assist), others reserved=0 |

Placement: This preamble MUST be the first bytes of the Application Data in GS-originated TCs for Services 3, 20, 23, 200, 210. The remainder of the payload follows the service-specific schema as defined in `ICD.md`.

MCU behavior: MCU consumes the preamble, re-issues a device TC with the remainder payload (without the preamble), and stores `transactionId` for mapping and forwarding of downstream ACKs/TMs back to GS, re-inserting `transactionId` as the first field of forwarded Application Data.

---

## 3. GS Command Scope and ACK/NACK Behavior
- GS MAY send Housekeeping (HK) requests (Service 3/1) to MCU for a specific target (MCU/PI/FPGA) using the Proxy Preamble, and MAY request a System HK aggregation (Service 3/10) from MCU.
- Devices publish their HK (3/2) to MCU; MCU forwards to GS as appropriate. System HK is returned by MCU as a single aggregated TM (3/100).
- GS MAY issue control TCs via MCU proxy for device control/services (Params, Camera, FPGA, Data Transfer) as defined below.
- GS SHOULD send minimal ACK/NACK back to MCU for GS→MCU transactions using Service 250 (Link/Proxy ACK), defined in `../ICD.md`.

---

## 4. Commands the GS Can Issue
For each of the following, prepend the Proxy Preamble (Section 2), then append the service-specific payload defined in `ICD.md`.

- Housekeeping
  - TC 3/1 Request HK — Preamble + optional `detailMask:uint16`; target must be one of MCU/PI/FPGA
  - Expected: TM 3/2 Report HK forwarded from target device
  - TC 3/10 Request System HK — Preamble (target MUST be MCU) + `{ transactionId:uint16, include_mask:uint8, detailMask:uint16 }`
  - Expected: TM 3/100 System HK Report from MCU (aggregated)

- Parameter Management
  - TC 20/1 Set Parameter — Preamble + `key:uint8` + `value:TLV`
  - TC 20/2 Get Parameter — Preamble + `key:uint8`
  - Expected: TM 20/3 Parameter Value (forwarded)

- Camera Control (target=PI)
  - TC 200/1 Capture — Preamble + `{ mode:uint8, burst_count:uint16, exposure_us:uint32 }`
  - TC 200/2 Cam Settings Set — Preamble + `key:uint8` + `value:TLV`
  - TC 200/3 Cam Settings Get — Preamble + `key:uint8`
  - Expected: TM 200/5 ACK, TM 23/10..12 (image stream), optional events

- FPGA Control (target=FPGA)
  - TC 210/1 Execute — Preamble + `{ pipeline:uint8, modelId:uint16, flags:uint16 }`
  - TC 210/2 Proc Settings Set — Preamble + `key:uint8` + `value:TLV`
  - TC 210/3 Proc Settings Get — Preamble + `key:uint8`
  - Expected: TM 210/5 ACK, result TMs/Events as defined in FPGA ICD

- Data Transfer
  - TC 23/1 Start Transfer — Preamble + `{ imageId:uint16, dest:uint8(0=FPGA,1=MCU), chunk_size:uint16 }`
  - TC 23/2 Stop Transfer — Preamble + `{ imageId:uint16 }`
  - Expected: TM 23/10 Metadata, TM 23/11 Chunks, TM 23/12 Complete

---

## 5. End-to-End Example Workflows

### 5.1 Capture → Transfer to FPGA → Run Inference → Return Result (via MCU)
Assume transactionId=42.

1) GS→MCU: TC 200/1 Capture (target=PI)
   - App Data: `[preamble{txId=42,target=PI,options=0}] + {mode=0,burst_count=1,exposure_us=0}`
   - MCU proxies to PI; PI→MCU sends TM 200/5 ACK which MCU forwards to GS with `txId` echoed.

2) GS→MCU: TC 23/1 Start Transfer (target=PI)
   - App Data: `[preamble{42,PI,options bit0=1 (mirror to GS)}] + {imageId=100, dest=0 /*FPGA*/, chunk_size=900}`
   - PI emits TM 23/10 Metadata, followed by TM 23/11 chunks to MCU; MCU forwards to FPGA (dest=FPGA) and, because options bit0=1, mirrors the same TMs to GS. Completion TM 23/12 forwarded likewise.

3) GS→MCU: TC 210/1 Execute (target=FPGA)
   - App Data: `[preamble{42,FPGA,0}] + {pipeline=3, modelId=1, flags=0}`
   - FPGA→MCU sends TM 210/5 ACK then result TMs (e.g., TM 23/11 with logits or TM 3/2 HK update); MCU forwards to GS with `txId=42` echoed.

4) Finalization: MCU may send Event (5/1..3) on errors; GS correlates by `txId` (and `imageId`/`jobId` if present).

---

## 6. CCSDSPack Interface Hints (GS)
Recommended names for GS-originated TCs (APID 0x0F0, Type=TC, PUS-A/C as per service). Application Data must start with the Proxy Preamble when targeting devices.
- `gs_hk_req_tc` (Svc 3/1), `gs_system_hk_req_tc` (Svc 3/10)
- `gs_param_set_tc`, `gs_param_get_tc`
- `gs_cam_capture_tc`, `gs_cam_setting_set_tc`, `gs_cam_setting_get_tc`
- `gs_fpga_exec_tc`, `gs_fpga_setting_set_tc`, `gs_fpga_setting_get_tc`
- `gs_xfer_start_tc`, `gs_xfer_stop_tc`
- `gs_link_ack_tm` (Svc 250/1) — GS→MCU link/proxy ACK/NACK

These interfaces should include configurable insertion of the Proxy Preamble fields before the service payload (for device-directed TCs).
