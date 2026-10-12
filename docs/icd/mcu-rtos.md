# MCU-RTOS Device/Service ICD

This document specializes the EXN master ICD for the STM32 MCU-RTOS/OBC control node. The wire-level packet profile, APID policy, CRC policy, proxy preamble and current compatibility payloads are authoritative in `../ICD.md`; the target PUS service tailoring is authoritative in `../services.md`.

## 1. Role and identifiers

- Node APID: `0x100`.
- PUS-A TC Source ID: `0x01`.
- Acts as the northbound logical Ground endpoint, system controller, downstream command router, System HK aggregator, and time master.
- Issues device TCs to PI-CAM (`0x101`), FPGA-AI (`0x102`), PI-COMMS/Antenna (`0x103`) and PI-MGMT (`0x104`).
- Receives and forwards device TMs while retaining the producing device APID.
- Intended implementation baseline: **HardRT 0.5.1** for runtime/scheduling and **DAS v0.1.0** for STM32H755 device/board access.

All currently executable MCU/HIL packet definitions use the EXN **transitional PUS revision-A** profile. The target OBC implementation shall migrate to the PUS-C service profile in `../services.md` before its public packet API is frozen.

## 2. Packet/APID behavior

EXN APID semantics are direction-specific:

- TCs emitted by MCU use the **destination APID**: PI-CAM `0x101`, FPGA `0x102`, PI-COMMS/Antenna `0x103`, or PI-MGMT `0x104`.
- TMs produced by MCU use APID `0x100`.
- GS-originated TCs received by MCU use destination APID `0x100` and PUS-A TC Source ID `0x10`.

The MCU maintains sequence state for each Packet Identification stream it originates.

## 3. GS northbound proxying

Ground traffic logically terminates at MCU for supervised control. The HIL profile connects GS directly to MCU; the flight-like profile carries the same OBC-destined packets through PI-COMMS. Device-directed GS operations therefore use an MCU routing hop.

### 3.1 Device-directed operation

1. GS sends a TC to APID `0x100`, Source ID `0x10`.
2. Application Data begins with the four-octet proxy preamble from `../ICD.md`:
   `{transactionId:u16,target:u8,options:u8}`.
3. MCU consumes the preamble.
4. MCU constructs a new TC addressed to the target APID with Source ID `0x01` and only the service-specific payload.
5. MCU correlates downstream responses to `transactionId` and forwards them to GS.

Valid proxy targets are `1=PI-CAM`, `2=FPGA-AI`, `3=PI-COMMS/Antenna`, and `4=PI-MGMT`. MCU-local operations do not use this preamble.

### 3.2 Direct MCU services

System HK TC `3/10` is a direct GS -> MCU command and **must not** carry the proxy preamble. Its Application Data is exactly:

`{transactionId:u16, include_mask:u8, detailMask:u16}`

A GS request for MCU-local TC `3/1` HK is likewise direct.

## 4. System HK aggregation

For TC `3/10`, MCU:

1. records `transactionId`, `include_mask`, and `detailMask`;
2. collects MCU self-HK when bit0 is set;
3. issues TC `3/1` to PI-CAM, FPGA, PI-COMMS and/or PI-MGMT when bits1/2/3/4 are set;
4. waits for the selected TM `3/2` reports up to the configured aggregation timeout;
5. produces TM `3/100` using APID `0x100` and echoes `transactionId`;
6. reports `present_mask` and `status=OK/PARTIAL/TIMEOUT/ERROR` according to collected data.

Recommended integration defaults remain approximately 200 ms per device and no more than 600 ms overall. These values are policy defaults, not packet-format requirements.

## 5. MCU packet scope

The packet numbers in this section describe the **transitional compatibility profile**. Target service numbers and verification behavior are defined in `../services.md`.

### Emits TCs

- `3/1` HK Request to PI-CAM/FPGA/PI-COMMS/PI-MGMT.
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
| `static_ram_used_bytes` | uint32 | 32 | Static RAM committed by the firmware/runtime configuration |
| `static_ram_free_bytes` | uint32 | 32 | Remaining RAM budget not committed to static regions/stacks |
| `task_count` | uint16 | 16 | Number of HardRT application tasks |
| `stack_low_water_min` | uint16 | 16 | Lowest remaining stack margin |
| `link_status` | uint16 | 16 | Platform-defined active-link bitmask |
| `tc_sent` | uint32 | 32 | TCs emitted |
| `tm_rcvd` | uint32 | 32 | TMs received |
| `err_count` | uint32 | 32 | Error counter |
| `ts_cuc` | bytes[6] | 48 | Mission CUC timestamp in Application Data |

An optional diagnostic TLV extension may follow when requested by `detailMask`.

## 7. Legacy time assignment, Service 17

The current compatibility profile uses Service 17 for time messages. PUS-C standardizes **Service 9** for Time Management and **Service 17** for Test; new OBC implementation work shall therefore use the Service 9 target profile from `../services.md`.

- TC `17/1`: Application Data contains `time_cuc[6]`.
- TM `17/2`: Application Data contains `time_cuc[6]` and optional quality/status fields.

The six-octet CUC value is mission Application Data. It is not a PUS-C secondary header.

## 8. Event reporting, Service 5

Service 5 also uses PUS revision A. Event ID is Application Data, not a separate PUS-B field.

Target MCU event severity follows PUS-C Service 5:

| Event ID | Severity/subservice | Description |
|---:|---|---|
| `0x0001` | `5/1` informative | Boot completed |
| `0x0002` | `5/2` low severity | Link degraded |
| `0x0003` | `5/4` high severity | Watchdog reset detected after reboot |

Subtype `5/3` is reserved for medium-severity anomalies in the target profile.

Application Data starts with `eventId:uint16`, followed by event-specific bytes.

## 9. Parameter management

The previous design listed transport-selection keys such as `SPI/UART/CAN/UDP` and a runtime `crc.enable` switch. Those are **not frozen mission parameters**:

- physical/link transport selection belongs to deployment/integration policy unless a real runtime-switching requirement is demonstrated;
- SpWKit is a logical transport option, not another MCU peripheral enum;
- CRC16 is part of the selected packet profile and shall not be casually disabled through a mission parameter;
- Service 20 message meanings shall follow the normalized PUS-C profile before parameter IDs are frozen.

MCU parameter identifiers will be allocated with the OBC implementation and recorded in this ICD once the associated runtime behavior exists. This avoids publishing configuration knobs for software that does not yet exist.
## 10. CCSDSPack v2 mapping

The `.cfg` templates and `interfaces/mcu-rtos/exn_interfaces.h` currently mirror the transitional PUS-A profile for EXN-GS/HIL compatibility. They are not the final OBC service API.

Key rules:

- downstream TC template APID is the destination (`0x101`, `0x102`, `0x103`, or target PI-MGMT `0x104`);
- MCU-produced TM APID is `0x100`;
- TC selector is `PUS:revA:TC`, source-ID width one octet, Source ID `0x01`;
- TM selector is `PUS:revA:TM`, destination-ID width zero octets;
- packet error control is CRC16;
- service-specific timestamps remain Application Data.


The target OBC interface migrates to CCSDSPack PUS-C TC/TM headers, standard Service 1 verification, normalized Services 3/5/9/20, and mission-specific Services 200/210/220. That migration shall be performed together with EXN-GS and the shared interface artifacts rather than piecemeal.


## 11. Target OBC component-management contract

The OBC is the **system command authority and component orchestrator**.

It maintains an endpoint registry for:

- PI-CAM;
- FPGA;
- PI-COMMS/Antenna;
- PI-MGMT.

For each endpoint it tracks:

- last heartbeat time;
- logical state `UNKNOWN/STARTING/READY/BUSY/DEGRADED/ERROR/UPDATING/OFFLINE`;
- software version;
- time-sync state;
- active alarms;
- last error;
- restart/reset count.

Recommended integration policy is a 1 s HEARTBEAT and 10 s HEALTH cycle. Three missed heartbeat intervals mark an endpoint OFFLINE and generate an OBC Service 5 event. Automatic recovery is policy-controlled rather than implicit.

## 12. OBC management operations

The target OBC consumes standard Service 8 Function Management.

Typical operations include:

```text
OBC -> PI-MGMT: restart Camera service
OBC -> PI-MGMT: restart Antenna service
OBC -> PI-MGMT: reboot Pi
OBC -> FPGA: reinitialize/reset processing endpoint
GS -> OBC: reboot OBC
```

Pi mission-service restart is always addressed to PI-MGMT so recovery does not depend on the failed process receiving its own restart command.

## 13. OBC time authority

The OBC owns the on-board mission clock.

Runtime clocks:

| Clock | Type | Meaning |
|---|---|---|
| `monotonic_ms` | uint64 | milliseconds since OBC boot; never steps backwards |
| `unix_ms` | uint64 | UTC milliseconds since 1970-01-01; valid after synchronization |

The OBC receives authoritative time from Ground or a future external source, then distributes `TC[9,128] Set Mission Time` to PI-CAM, PI-COMMS, PI-MGMT and FPGA.

A target endpoint shall maintain mission time as a synchronized offset from its monotonic clock. Repeated synchronization shall not make application timers depend on wall-clock steps.

The target telemetry timestamp is six-octet CUC using Unix epoch, with four coarse-second octets and two fine-fraction octets.

## 14. Housekeeping and monitoring

The target OBC consumes predeclared Service 3 structures:

- HEARTBEAT;
- HEALTH;
- SENSORS;
- LINKS;
- endpoint-specific detail.

System HK `3/128 -> 3/129` aggregates the latest valid reports and endpoint state.

The OBC also consumes Service 12 monitor state and Service 5 alarm events. The OBC itself shall report all sensor/value sources actually measurable through the STM32/DAS target, including MCU internal temperature and internal voltage/reference channels when those measurements are implemented.

No board sensor shall be documented as available until the selected hardware path can measure it.

## 15. OBC payload-transfer control

The OBC controls transfers without becoming the bulk-data router.

For every requested Service 220 transfer it records:

- transfer ID;
- source endpoint;
- object ID;
- destination;
- expected size when known;
- start time;
- transfer state/result.

Examples:

```text
OBC -> PI-CAM: transfer object X to Ground
PI-CAM -> Antenna -> Ground

OBC -> FPGA: transfer result object Y to Ground
FPGA -> Antenna -> Ground

OBC -> PI-CAM: transfer object Z to OBC
PI-CAM -> OBC
```

The first two preserve OBC control while removing image/result bytes from the MCU data path.

## 16. Camera and processing orchestration

The OBC shall be able to:

- query Camera capabilities;
- set/get Camera controls;
- start/stop acquisition;
- query acquisition status;
- list Camera products;
- command a selected product to OBC, FPGA or Ground;
- query FPGA processing capabilities;
- submit/cancel processing jobs;
- query job status;
- consume small decision results;
- command large FPGA result objects to Camera, OBC or Ground.

The OBC does not need to know which processing algorithms exist at compile time; it uses FPGA capability/operation IDs from Service 210.

## 17. Software inventory and update control

The OBC coordinates Service 230 software management.

It shall be able to query:

- OBC firmware version;
- PI-CAM service version;
- Antenna service version;
- PI-MGMT version;
- FPGA software/bitstream version as applicable.

Pi update activation is commanded through PI-MGMT. The update package is staged as a Service 220 object, validated, activated, and followed by heartbeat/HK health verification.

An update is not recorded successful merely because an executable was copied; the updated endpoint must return healthy after activation.
