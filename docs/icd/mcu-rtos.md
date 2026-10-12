# STM32 OBC Device/Service ICD

This document is the normative EXN ICD for the STM32 OBC endpoint.

Common PUS-C packet rules are defined in `../ICD.md`, service semantics in `../services.md`, and shared component behavior in `../component-services.md`.

## 1. Identity and role

| Item | Value |
|---|---|
| APID | `0x100` |
| PUS-C source/destination ID | `0x0001` |
| PUS revision | C |

The OBC is the spacecraft command authority, component orchestrator, System-HK aggregator, time authority and health/fault supervisor.

The intended implementation baseline is HardRT 0.5.1 + DAS v0.1.0 on STM32H755 / NUCLEO-H755ZI-Q.

## 2. Packet behavior

The OBC:

- receives Ground-directed TCs at APID `0x100`;
- produces OBC TMs with APID `0x100`;
- emits downstream TCs with the target endpoint APID and PUS-C Source ID `0x0001`;
- consumes target TMs while preserving their producer APID;
- maintains CCSDS sequence counters and PUS-C TM message-type counters per required stream.

All OBC packet handling uses PUS Revision C.

## 3. Ground-routed command handling

For a routed Ground request, the OBC consumes the routing envelope defined in `gs.md`:

```text
transaction_id:u16
target:u8
options:u8
target_payload[...]
```

The OBC shall:

1. validate the Ground TC and Service 1 acceptance semantics;
2. authorize/validate the target operation;
3. remove the routing envelope;
4. construct a new PUS-C TC for the target endpoint;
5. track downstream verification/results against `transaction_id`;
6. forward/report the result to Ground according to the operation and options.

The OBC shall not forward malformed or unauthorized requests merely because the APID exists.

## 4. Endpoint registry and supervision

The OBC tracks:

- PI-CAM;
- FPGA;
- Antenna;
- PI-MGMT.

Per endpoint:

- last heartbeat time;
- common state;
- software version;
- time-sync state;
- active alarms;
- last error;
- restart/reset count.

Normative default supervision parameters are defined in `../component-services.md` and exposed through Service 20.

Three missed heartbeat periods mark an endpoint OFFLINE and produce an OBC Service 5 event unless configured otherwise.

## 5. Housekeeping

### 5.1 OBC one-shot HK

Ground or another authorized endpoint requests OBC HK with `TC[3,27]`. OBC returns `TM[3,25]` using the common HEARTBEAT/HEALTH/SENSORS/LINKS structures from `../component-services.md`.

OBC SENSORS shall include only values actually measurable through the selected STM32/DAS implementation, for example MCU internal temperature/reference channels once implemented.

### 5.2 System HK

System HK is OBC-owned:

- `TC[3,128]` Request System HK;
- `TM[3,129]` System HK Report.

On request, the OBC obtains or uses sufficiently fresh `TM[3,25]` data for each requested endpoint and builds the aggregate report defined in `gs.md`.

The OBC reports missing endpoints as PARTIAL/TIMEOUT rather than blocking indefinitely.

## 6. Service 5 events

OBC event IDs occupy `0x1000..0x1FFF`.

Examples:

| Event | Severity |
|---|---|
| boot completed | 5/1 informative |
| endpoint heartbeat lost | 5/2 low or 5/3 medium according to policy |
| watchdog reset detected | 5/4 high |
| time synchronization lost | 5/2 or 5/3 according to duration |
| component recovery failed | 5/3 or 5/4 |

Event payload format is the common Service 5 model in `../component-services.md`.

## 7. Service 8 component management

The OBC may perform/route Service 8 functions including:

```text
OBC -> PI-MGMT: restart Camera
OBC -> PI-MGMT: restart Antenna
OBC -> PI-MGMT: reboot Pi
OBC -> FPGA: reinitialize processing endpoint
Ground -> OBC: reboot OBC
```

A Pi mission-service restart is addressed to PI-MGMT, not to the failing service itself.

## 8. Service 9 time authority

The OBC maintains:

- `monotonic_ms:uint64`: milliseconds since OBC boot, never stepped backwards;
- `unix_ms:uint64`: synchronized Unix-epoch milliseconds.

Ground synchronizes the OBC with `TC[9,128] Set Mission Time`.

OBC propagates that command to PI-CAM, FPGA, Antenna and PI-MGMT after synchronization and periodically according to Service 20 policy.

PUS-C OBC telemetry uses the normative CUC timestamp defined in `../ICD.md`.

## 9. Service 12 monitoring

The OBC consumes endpoint monitoring/alarm state and also monitors its own declared health values.

Monitoring covers actual available measurements and operational health conditions only. Threshold values are Service 20 configuration parameters.

The OBC records active alarms in System HK and emits Service 5 events on relevant transitions.

## 10. Service 20 runtime configuration

Service 20 contains OBC **settings**, not actions.

Initial OBC parameters are defined in `../component-services.md`, including:

- heartbeat period;
- health-report period;
- missed-heartbeat limit;
- automatic-restart enable;
- time resynchronization period;
- maximum accepted synchronization age;
- default transfer chunk size.

Restarting a component, synchronizing time, starting Camera acquisition, or submitting an FPGA job is not Service 20.

## 11. Camera orchestration

The OBC shall be able to:

- query Camera capabilities;
- set/get Service 200 Camera controls;
- start/stop acquisition;
- inspect acquisition status;
- list Camera products;
- command a Camera object to OBC, FPGA or Ground.

Bulk Camera data does not pass through OBC unless OBC itself is the selected destination.

## 12. FPGA orchestration

The OBC shall be able to:

- query Service 210 processing capabilities;
- submit/cancel jobs;
- query job status;
- consume small decision results;
- command large result objects to OBC, Camera or Ground.

The OBC does not compile in assumptions about the final FPGA algorithms; it uses advertised operation IDs.

## 13. Payload-transfer registry

For every OBC-commanded Service 220 transfer the OBC tracks:

```text
transfer_id
source_endpoint
object_id
destination
expected_size
started_unix_ms
state
result
```

Ground downlink is OBC-controlled but source-to-Antenna:

```text
OBC -> Camera/FPGA: transfer object to Ground
Camera/FPGA -> Antenna -> Ground
```

## 14. Software management

The OBC coordinates Service 230 inventory/update operations.

It may manage:

- OBC firmware;
- PI-CAM;
- Antenna;
- PI-MGMT;
- FPGA node software;
- FPGA bitstream/model bundle where supported.

Package bytes use Service 220. Activation/rollback state uses Service 230. An update is successful only after post-activation heartbeat/HK validation.

## 15. PUS-C construction

OBC TCs use `PUS:revC:TC`, Source ID `0x0001`, CRC16 and command-specific verification flags.

OBC TMs use `PUS:revC:TM`, the appropriate destination ID, message-type counter and the normative CUC timestamp.

The OBC shall not implement PUS-A packet paths or legacy Service 17/23/250 semantics.
