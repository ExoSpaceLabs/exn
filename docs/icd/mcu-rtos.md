# MCU-RTOS Device/Service ICD

This document specializes the ASTRAi ICD for the STM32 MCU-RTOS control node (APID 0x100). It details the TCs it emits, TMs it receives, its own TMs (e.g., events/time), and the housekeeping (HK) parameters it reports when queried.

See the master CCSDS/PUS conventions and service catalog in `../ICD.md`.

---

## 1. Role and Interfaces
- Acts as the system controller and time master.
- Issues TeleCommands to PI-CAM (APID 0x101) and FPGA-AI (APID 0x102).
- Aggregates TeleMetry and relays events upward.
- Provides time distribution (PUS-C Service 17).

Transports: SPI, UART, CAN, or UDP, as integrated per platform build. Framing may be SLIP or raw; see `ICD.md` Transport section.

---

## 2. TCs/TMs Summary (MCU perspective)
- Emits:
  - 3/1 HK Request (to PI, FPGA)
  - 17/1 Set Time (to all)
  - 20/1 Set Param, 20/2 Get Param (to PI, FPGA)
  - 200/x Camera Control (to PI)
  - 210/x Processing Control (to FPGA)
  - 23/1 Start Transfer, 23/2 Stop Transfer (to PI/FPGA)
- Receives:
  - 3/2 HK Report (from PI, FPGA)
  - 5/x Events (from any)
  - 17/2 Time Report (from any)
  - 20/3 Parameter Value (from PI, FPGA)
  - 200/4 Settings Report, 200/5 ACK (from PI)
  - 210/4 Settings Report, 210/5 ACK (from FPGA)
  - 23/10..12 Data Transfer packets (from PI/FPGA)

---

## 2.5 GS Northbound Interface and Proxying
MCU-RTOS interfaces with the Ground Station (GS, APID 0x0F0, SourceID 0x10) over a northbound link. GS-originated TCs targeting devices include a Proxy Preamble (see `../ICD.md` and `gs.md`). MCU behavior:

- Accept TCs from GS (APID src=0x0F0→dst=0x100) for Services 3/1 (HK Request), 3/10 (System HK Request), 20, 23, 200, 210.
- Accept GS Link/Proxy ACK as TM 250/1 from GS to confirm UI/link acceptance or cancellation (see `../ICD.md` Section 4.7).
- Parse Proxy Preamble fields at the head of Application Data:
  - `transactionId:uint16`, `target:uint8` (0=ALL (System HK only), 1=PI,2=FPGA, 3=MCU), `options:uint8`.
- Re-issue device TC to the target APID with the remainder payload (without the preamble).
- Correlate responses: For any downstream ACK/TM related to the proxied operation, forward upstream to GS and prepend `transactionId` as the first field in the forwarded Application Data. Preserve the device APID when link/protocol permits; otherwise include APID in a small preamble field `orig_apid:uint16` after `transactionId`.
- Fan-out requests: For System HK (3/10), MCU issues internal 3/1 HK requests to each device selected by `include_mask` and also collects its own HK.
- Timeouts/Retry (recommended default):
  - Immediate device ACK expected within 100 ms; retry TC up to 2 times if no ACK.
  - Operation timeout per service: Capture/Transfer 10 s (configurable), Execute 5 s; System HK aggregation up to 600 ms total.

### 2.6 System HK Aggregation (TC 3/10 → TM 3/100)
- Upon receiving TC 3/10 from GS, MCU:
  1) Records `transactionId` and `include_mask`.
  2) Issues TC 3/1 to PI and/or FPGA per `include_mask` and gathers TM 3/2; collects MCU self-HK.
  3) Waits up to `T_hk_timeout` (≈200 ms per device) with an overall cap of 600 ms.
  4) Builds TM 3/100 with `present_mask` indicating which blocks are included and `status=OK/PARTIAL/TIMEOUT`.
  5) Sends TM 3/100 to GS, echoing `transactionId`.
- Partial results policy: include any received HK; missing devices are omitted from `present_mask`.
- Error handling: severe errors are also reported as PUS-B events (Service 5).

Note: For Data Transfer (23/10..12), if GS preamble `options bit0` is set, MCU mirrors PI→MCU stream to GS while also forwarding to FPGA when `dest=0`. 

---

## 3. MCU-RTOS Housekeeping (Service 3)
When MCU is queried by another node (less common) or for self-test logs, the following HK set is used. Use big-endian for multi-byte fields.

- TM 3/2 Report HK — Application Data

| Name                | Type   | Bits | Description |
|---------------------|--------|------|-------------|
| uptime_ms           | uint64 | 64   | System uptime in milliseconds |
| reset_cause         | uint8  | 8    | 0=POR,1=PIN,2=IWDG,3=WWDG,4=SW,5=LPWR,6=BOR |
| fw_version          | uint32 | 32   | Firmware version encoded (major<<24|minor<<16|patch<<8|build) |
| heap_used_bytes     | uint32 | 32   | Current heap in bytes |
| heap_free_bytes     | uint32 | 32   | Free heap in bytes |
| task_count          | uint16 | 16   | Number of RTOS tasks |
| stack_low_water_min | uint16 | 16   | Minimum remaining stack (worst task) in bytes |
| link_status         | uint16 | 16   | Bitmask: [0]SPI, [1]UART, [2]CAN, [3]UDP up |
| tc_sent             | uint32 | 32   | Total TCs sent |
| tm_rcvd             | uint32 | 32   | Total TMs received |
| err_count           | uint32 | 32   | Error counter |
| ts_cuc              | bytes  | 48   | 6-byte CUC timestamp |

Notes:
- Add optional diagnostic extension as a TLV block at the end if `detailMask` in TC 3/1 requests it.

---

## 4. Time Management (Service 17, PUS-C)
- TC 17/1 Set Time: MCU sends 6-byte CUC time code to sync nodes.
- TM 17/2 Time Report: Optional echo/periodic from devices; MCU may also issue a self-report during testing.

Time code: Recommended CUC coarse(4 bytes seconds) + fine(2 bytes 1/65536 s).

---

## 5. Event Reporting (Service 5, PUS-B)
MCU can emit its own events, but typically aggregates device events. Event payload follows `ICD.md` Section 5.1. Suggested MCU event IDs:

| Event ID | Severity | Description |
|----------|----------|-------------|
| 0x0001   | Info     | Boot completed |
| 0x0002   | Warn     | Link degraded (detail bitmask) |
| 0x0003   | Error    | Watchdog reset occurred |

---

## 6. Parameter Keys (Service 20)
Keys are 8-bit. Values are TLV in Application Data. TLV Types suggested: 1=U8,2=U16,3=U32,4=I32,5=F32,6=STR,7=BYTES.

| Key | Name              | TLV Type | Units | Range/Enum | Default | Notes |
|-----|-------------------|----------|-------|------------|---------|-------|
| 1   | link.primary      | U8       | enum  | 0=SPI,1=UART,2=CAN,3=UDP | 0 | Preferred control link |
| 2   | link.baud         | U32      | bps   | 9600..3e6  | 115200  | UART only |
| 3   | can.bitrate       | U32      | bps   | 125k..1M   | 500000  | CAN only |
| 4   | time.use_pus_c    | U8       | bool  | 0/1        | 1       | Use PUS-C time distribution |
| 5   | crc.enable        | U8       | bool  | 0/1        | 1       | Enable CRC appending |

---

## 7. CCSDSPack Templates
Suggested names (align with `ICD.md` Section 7) for MCU-owned packets:
- `pkt_hk_req_tc`, `pkt_time_set_tc`, `pkt_param_set_tc`, `pkt_param_get_tc`
- `pkt_cam_capture_tc`, `pkt_cam_settings_set_tc`, `pkt_cam_settings_get_tc`
- `pkt_fpga_exec_tc`, `pkt_fpga_settings_set_tc`, `pkt_fpga_settings_get_tc`
- `pkt_xfer_start_tc`, `pkt_xfer_stop_tc`

These set Primary Header Type=TC, APID=0x100, SeqFlags=UNSEG; PUS headers per service/subservice as listed.