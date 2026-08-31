# MCU-RTOS Device/Service ICD

This document specializes the EXN master ICD for the STM32 MCU-RTOS control node. The wire-level packet profile, APID policy, PUS revision, CRC policy, proxy preamble, and common service payloads are authoritative in `../ICD.md`.

## 1. Role and identifiers

- Node APID: `0x100`.
- PUS-A TC Source ID: `0x01`.
- Acts as the northbound GS endpoint, system controller, downstream command router, System HK aggregator, and time master.
- Issues device TCs to PI-CAM (`0x101`) and FPGA-AI (`0x102`).
- Receives and forwards device TMs while retaining the producing device APID.

All currently defined MCU packets use the EXN **PUS revision-A** profile from the master ICD. Service numbers do not select a different PUS revision.

## 2. Packet/APID behavior

EXN APID semantics are direction-specific:

- TCs emitted by MCU use the **destination APID**: PI `0x101` or FPGA `0x102`.
- TMs produced by MCU use APID `0x100`.
- GS-originated TCs received by MCU use destination APID `0x100` and PUS-A TC Source ID `0x10`.

The MCU maintains sequence state for each Packet Identification stream it originates.

## 3. GS northbound proxying

GS physically communicates with MCU. Device-directed GS operations therefore use two packet hops.

### 3.1 Device-directed operation

1. GS sends a TC to APID `0x100`, Source ID `0x10`.
2. Application Data begins with the four-octet proxy preamble from `../ICD.md`:
   `{transactionId:u16,target:u8,options:u8}`.
3. MCU consumes the preamble.
4. MCU constructs a new TC addressed to the target APID with Source ID `0x01` and only the service-specific payload.
5. MCU correlates downstream responses to `transactionId` and forwards them to GS.

Valid proxy targets are `1=PI` and `2=FPGA`. MCU-local operations do not use this preamble.

### 3.2 Direct MCU services

System HK TC `3/10` is a direct GS -> MCU command and **must not** carry the proxy preamble. Its Application Data is exactly:

`{transactionId:u16, include_mask:u8, detailMask:u16}`

A GS request for MCU-local TC `3/1` HK is likewise direct.

## 4. System HK aggregation

For TC `3/10`, MCU:

1. records `transactionId`, `include_mask`, and `detailMask`;
2. collects MCU self-HK when bit0 is set;
3. issues TC `3/1` to PI and/or FPGA when bits1/2 are set;
4. waits for the selected TM `3/2` reports up to the configured aggregation timeout;
5. produces TM `3/100` using APID `0x100` and echoes `transactionId`;
6. reports `present_mask` and `status=OK/PARTIAL/TIMEOUT/ERROR` according to collected data.

Recommended integration defaults remain approximately 200 ms per device and no more than 600 ms overall. These values are policy defaults, not packet-format requirements.

## 5. MCU packet scope

### Emits TCs

- `3/1` HK Request to PI/FPGA.
- `17/1` Set Time to each target node.
- `20/1` Set Parameter and `20/2` Get Parameter.
- `23/1` Start Transfer and `23/2` Stop Transfer.
- `200/1..3` Camera control to PI.
- `210/1..3` processing control to FPGA.

### Produces TMs/reports

- `3/2` MCU HK Report.
- `3/100` System HK Report.
- `5/1..3` MCU event reports.
- `17/2` MCU Time Report when requested/used.

### Receives device TMs

- `3/2` HK Report.
- `5/1..3` Event reports.
- `17/2` Time Report.
- `20/3` Parameter Value.
- `23/10..12` transfer packets.
- `200/4..5` camera reports/ACKs.
- `210/4..5` FPGA reports/ACKs.

## 6. MCU housekeeping payload

When MCU produces TM `3/2`, Application Data uses big-endian encoding:

| Name | Type | Bits | Description |
|---|---|---:|---|
| `uptime_ms` | uint64 | 64 | System uptime in milliseconds |
| `reset_cause` | uint8 | 8 | `0=POR,1=PIN,2=IWDG,3=WWDG,4=SW,5=LPWR,6=BOR` |
| `fw_version` | uint32 | 32 | `major<<24 | minor<<16 | patch<<8 | build` |
| `heap_used_bytes` | uint32 | 32 | Current heap usage |
| `heap_free_bytes` | uint32 | 32 | Free heap |
| `task_count` | uint16 | 16 | Number of RTOS tasks |
| `stack_low_water_min` | uint16 | 16 | Lowest remaining stack margin |
| `link_status` | uint16 | 16 | Platform-defined active-link bitmask |
| `tc_sent` | uint32 | 32 | TCs emitted |
| `tm_rcvd` | uint32 | 32 | TMs received |
| `err_count` | uint32 | 32 | Error counter |
| `ts_cuc` | bytes[6] | 48 | Mission CUC timestamp in Application Data |

An optional diagnostic TLV extension may follow when requested by `detailMask`.

## 7. Time management, Service 17

Service 17 uses the same **PUS revision-A** secondary header as every current EXN service.

- TC `17/1`: Application Data contains `time_cuc[6]`.
- TM `17/2`: Application Data contains `time_cuc[6]` and optional quality/status fields.

The six-octet CUC value is mission Application Data. It is not a PUS-C secondary header.

## 8. Event reporting, Service 5

Service 5 also uses PUS revision A. Event ID is Application Data, not a separate PUS-B field.

Suggested MCU event IDs:

| Event ID | Severity/subservice | Description |
|---:|---|---|
| `0x0001` | `5/1` Info | Boot completed |
| `0x0002` | `5/2` Warn | Link degraded |
| `0x0003` | `5/3` Error | Watchdog reset occurred |

Application Data starts with `eventId:uint16`, followed by event-specific bytes.

## 9. Parameter keys, Service 20

Keys are 8-bit and values use the master ICD TLV encoding.

| Key | Name | TLV | Range/Enum | Default | Notes |
|---:|---|---|---|---|---|
| 1 | `link.primary` | U8 | `0=SPI,1=UART,2=CAN,3=UDP` | 0 | Preferred control transport |
| 2 | `link.baud` | U32 | `9600..3000000` | 115200 | UART only |
| 3 | `can.bitrate` | U32 | `125000..1000000` | 500000 | CAN only |
| 4 | `time.distribution_enable` | BOOL | `0/1` | 1 | Enables Service 17 distribution using current Rev-A profile |
| 5 | `crc.enable` | BOOL | `0/1` | 1 | Current baseline requires enabled CRC16 |

## 10. CCSDSPack v2 mapping

MCU-hosted/filesystem consumers use the `.cfg` templates under `interfaces/ccsdspack/`. Bare-metal consumers use `interfaces/mcu-rtos/exn_interfaces.h` and construct packets programmatically.

Key rules:

- downstream TC template APID is the destination (`0x101` or `0x102`);
- MCU-produced TM APID is `0x100`;
- TC selector is `PUS:revA:TC`, source-ID width one octet, Source ID `0x01`;
- TM selector is `PUS:revA:TM`, destination-ID width zero octets;
- packet error control is CRC16;
- service-specific timestamps remain Application Data.
