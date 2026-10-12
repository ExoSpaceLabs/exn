# EXN Component and Service Contract

This document defines the **normative active normative functional service contract** for all EXN components. It is part of the central EXN ICD and is implemented by the component repositories.

It complements:

- `architecture.md` — who owns each system responsibility;
- `services.md` — how those responsibilities map onto PUS services;
- `ICD.md` — exact packet/header/routing rules;
- device ICDs under `docs/icd/` — endpoint-specific fields and capabilities.

The intent is to make the system implementable without inventing behavior independently in each component repository. Component implementations shall not assign or reinterpret mission-visible fields locally; missing contract elements are added here first.

## 1. Endpoint model

The target logical endpoints are:

| Endpoint | APID | Purpose |
|---|---:|---|
| OBC | `0x100` | command authority, orchestration, time authority, health aggregation |
| PI-CAM | `0x101` | Camera Module 3 acquisition and payload-product ownership |
| FPGA | `0x102` | generic image/AI processing endpoint |
| PI-COMMS / Antenna | `0x103` | single Ground-link endpoint |
| PI-MGMT | `0x104` | Raspberry Pi platform supervisor for service restart, host reboot, platform HK and software activation |
| GS | `0x0F0` | Ground/HIL operator endpoint |

The Raspberry Pi still has only **two mission purposes**: Camera and Antenna. PI-MGMT is supporting infrastructure so the OBC can recover or update those services even if one of them is not running.

## 2. Common service set

Every controllable endpoint implements the applicable subset of the following active services.

| Service | Name | EXN use |
|---:|---|---|
| 1 | Request Verification | acceptance/completion of commands |
| 3 | Housekeeping | heartbeat, full HK, sensor reports |
| 5 | Event Reporting | faults, alarms, state transitions |
| 8 | Function Management | restart/reset/reboot/clear-fault operations |
| 9 | Time Management | synchronized mission time reporting and EXN time-set extension |
| 12 | On-board Monitoring | fixed sensor/parameter monitoring definitions and limit state |
| 20 | Parameter Management | persistent/runtime configuration |
| 200 | Camera Control | PI-CAM acquisition, controls, capabilities and product catalog |
| 210 | Processing | FPGA job/capability/result control |
| 220 | Payload Object Transfer | bounded transfer of images, results and other payload objects |
| 230 | Software Management | inventory, staging, activation and rollback of component software |

PUS Service 11 time-based scheduling is deliberately **not required for the first baseline**. The OBC may add it later for autonomous scheduled capture/processing/downlink sequences without changing the services above.

PUS Service 23 remains available for standardized file-system management, but EXN does not expose arbitrary Linux paths merely to move mission payload data. Payload products use Service 220.

## 3. Common endpoint state

The following logical state vocabulary is shared by OBC health/orchestration:

| Value | State | Meaning |
|---:|---|---|
| 0 | `UNKNOWN` | no current state established |
| 1 | `STARTING` | process/node is initializing |
| 2 | `READY` | available for normal commands |
| 3 | `BUSY` | valid operation in progress |
| 4 | `DEGRADED` | responsive, but capability/health is reduced |
| 5 | `ERROR` | responsive but unable to provide normal service |
| 6 | `UPDATING` | software update/activation in progress |
| 7 | `OFFLINE` | expected endpoint is not responding |

Endpoints may expose more detailed internal states, but the OBC shall always be able to reduce them to this set.

## 3.1 Common typed value encoding

Where EXN mission-specific services need a typed scalar/value, use:

| Type | Code | Encoding |
|---|---:|---|
| BOOL | 1 | uint8 0/1 |
| U8 | 2 | uint8 |
| U16 | 3 | big-endian uint16 |
| U32 | 4 | big-endian uint32 |
| U64 | 5 | big-endian uint64 |
| I16 | 6 | big-endian int16 |
| I32 | 7 | big-endian int32 |
| F32 | 8 | IEEE-754 binary32, big-endian bytes |
| STRING | 9 | bounded UTF-8 |
| BYTES | 10 | bounded opaque bytes |

Variable values always carry an explicit length. No target packet definition requires unbounded strings or dynamic allocation.

## 3.2 Common Service 20 parameter format

EXN parameter IDs are `uint16` scoped by endpoint APID.

`TC[20,1]` requests one or more parameter IDs.

`TM[20,2]` reports:

```text
count:u16
repeat count:
    parameter_id:u16
    status:u8
    value_type:u8
    value_length:u16
    value[value_length]
```

`TC[20,3]` uses the same repeated ID/type/length/value structure without the status byte.

## 4. Request verification

Service 1 is used for command verification.

The standard verification reports used by EXN are:

| Message | Meaning |
|---|---|
| `TM[1,1]` | acceptance success |
| `TM[1,2]` | acceptance failure |
| `TM[1,3]` | execution start success |
| `TM[1,4]` | execution start failure |
| `TM[1,5]` | execution progress success |
| `TM[1,6]` | execution progress failure |
| `TM[1,7]` | execution completion success |
| `TM[1,8]` | execution completion failure |

Verification packets identify the original request through the standard PUS request-identification fields. EXN does not add a second mission transaction ID unless the application operation itself needs one, such as `acquisitionId`, `jobId`, or `transferId`.

### 4.1 Verification Application Data

Every Service 1 verification report begins with the standard 32-bit **request ID**, copied from the first four octets of the verified TC's CCSDS primary header:

```text
request_packet_id:u16
request_packet_sequence_control:u16
```

This is the original TC Packet ID word followed by its Packet Sequence Control word, both copied verbatim.

EXN tailors the remaining fields as follows:

- `TM[1,1]`, `TM[1,3]`, `TM[1,7]`: request ID only;
- `TM[1,5]`: request ID + `progress_step_id:u16`;
- failure reports `TM[1,2]`, `TM[1,4]`, `TM[1,6]`, `TM[1,8]`:

```text
request_id[4]
failure_code:u16
failure_data_length:u16
failure_data[failure_data_length]
```

`failure_code` values are allocated by the endpoint ICD/event/error dictionary. `failure_data` is bounded auxiliary information specific to that code and may be empty.

The PUS-C TM Destination ID of the verification report identifies the issuer of the verified TC. Thus an OBC->Camera TC is verified by a PI-CAM TM[1,x] addressed to PUS ID `0x0001`; a Ground->OBC TC is verified by an OBC TM[1,x] addressed to `0x0010`.

Baseline policy:

- every state-changing command requests **acceptance** and **completion** verification;
- long-running commands may additionally request start/progress verification;
- read/report requests require acceptance verification and may use completion verification when generation can fail asynchronously;
- command-specific ACK/NACK telemetry in Services 200/210 is not part of the EXN profile; Service 1 is authoritative.

Completion means the requested operation reached its defined completion point, not merely that it was queued.

Examples:

- Camera single acquisition: completion after requested products are committed to the object store.
- Start burst/time-lapse/stream: completion after the acquisition session has entered RUNNING; later termination/status is reported separately.
- Service restart: completion after the supervisor observes the restarted service as healthy.
- Payload transfer: completion after the producer has completed transfer to the destination endpoint.
- Software activation: completion after post-activation health validation succeeds or rollback is performed.

## 5. Housekeeping and heartbeat

### 5.1 Standard messages

The target uses:

- `TC[3,27]` — generate one-shot HK report;
- `TM[3,25]` — HK report;
- `TC[3,5]` / `TC[3,6]` — enable/disable periodic HK generation.

Each endpoint exposes predeclared HK structure IDs.

### 5.2 EXN HK structure IDs

| ID | Name | Purpose |
|---:|---|---|
| 1 | `HEARTBEAT` | small liveness/state report |
| 2 | `HEALTH` | resource, error and subsystem status |
| 3 | `SENSORS` | all currently available physical sensor readings |
| 4 | `LINKS` | endpoint/network/link status where applicable |
| 5+ | endpoint-specific | Camera, FPGA, storage or other detailed blocks |

`HEARTBEAT` contains at minimum:

- endpoint state;
- software version;
- uptime/monotonic time;
- mission-time synchronization state;
- last-error code;
- restart/reset count.

The OBC may poll or enable periodic heartbeat reports. Recommended integration defaults are **1 s heartbeat** and **10 s full health**, but these are runtime policy rather than wire constants.

The OBC maintains its own **supervision state** for every registered endpoint. This state is authoritative from the spacecraft point of view and is deliberately separate from the endpoint's last self-reported operational state.

| Value | Supervision state | Meaning |
|---:|---|---|
| 0 | `UNKNOWN` | no valid report has ever been received |
| 1 | `OK` | latest required HK is within configured freshness limits |
| 2 | `STALE` | cached HK exists but is older than the configured freshness limit |
| 3 | `OFFLINE` | endpoint exceeded the configured missed-heartbeat/offline threshold |
| 4 | `ERROR` | communication/protocol error prevents reliable supervision |

An endpoint may therefore have last reported `endpoint_state=READY` while the OBC currently marks it `STALE` or `OFFLINE`. Ground shall trust the OBC supervision state for current spacecraft availability.

The OBC continuously refreshes its HK cache according to Service 20 supervision policy. Ground System-HK requests do not normally trigger a new cascade of endpoint requests.

Three consecutive missing heartbeat intervals are the integration default for transition from `OK` to `STALE`. `offline_after_ms` then controls transition to `OFFLINE`. If `auto_recovery_enable` is set and the endpoint remains unavailable for `recovery_after_ms`, the OBC may trigger the endpoint's configured recovery action.

The OBC emits a Service 5 event on every supervision-state and recovery-state transition. Automatic recovery is disabled by default until the platform-specific reset path has been verified.

### 5.3 Common HEARTBEAT / HEALTH wire prefix

Every `TM[3,25]` starts its Application Data with:

```text
structure_id:u16
sample_unix_ms:u64
```

For `HEARTBEAT (1)`, the remaining fields are:

```text
endpoint_state:u8
time_state:u8
software_version_major:u16
software_version_minor:u16
software_version_patch:u16
uptime_ms:u64
last_error:u16
restart_or_reset_count:u32
```

For `HEALTH (2)`, the prefix is followed by:

```text
health_flags:u32
last_error:u16
error_count:u32
extension_length:u16
extension[extension_length]
```

The extension is endpoint-defined and bounded by that endpoint ICD.

`TC[3,27]` requests a predeclared `structure_id:u16`. Periodic-enable/disable messages reference the same structure ID and generation policy.

### 5.4 Sensor registry

Every endpoint shall expose the physical sensors defined for its active reference-hardware profile in `icd/hardware-sensors.md`. The ICD shall not invent sensors that the board cannot measure, but once a reference profile marks a sensor **required**, failure to expose it is a platform/driver degradation rather than an excuse to omit the field.

A sensor descriptor contains:

- sensor ID;
- name;
- unit;
- numeric representation;
- valid range if known;
- warning/critical monitoring definition IDs;
- availability flag.

The SENSORS HK report contains current values plus validity/alarm state.

Examples:

- OBC/STM32: MCU internal temperature and internal reference/voltage measurements when enabled by the selected MCU/DAS implementation;
- Raspberry Pi: SoC temperature and power/throttling status; camera-sensor temperature only if exposed by the active camera stack;
- Antenna: signal/link metrics only when the selected Ground-link technology exposes them;
- FPGA: die temperature/voltage rails only if the selected FPGA platform exposes those monitors.

### 5.5 Sensor report wire model

The SENSORS HK block contains the active hardware profile followed by bounded entries:

```text
hardware_profile_id:u16
count:u16
repeat count:
    sensor_id:u16
    status:u8          # VALID/UNAVAILABLE/STALE/FAULT
    alarm_state:u8     # NORMAL/WARN_LOW/WARN_HIGH/CRIT_LOW/CRIT_HIGH
    value_type:u8
    value_length:u16
    value[value_length]
```

Sensor IDs and engineering units are frozen in the reference-hardware sensor ICD. Unsupported optional sensors are omitted or explicitly marked UNAVAILABLE. A required profile sensor that becomes unavailable shall be marked UNAVAILABLE and reflected as degraded platform health; zero remains a valid measurement and never means "unsupported".

## 5.6 Event report payload

All Service 5 reports use a common EXN Application Data prefix:

```text
event_id:u16
event_unix_ms:u64
source_state:u8
data_length:u16
data[data_length]
```

`data` is event-specific and bounded. Severity is carried by the Service 5 subtype, not duplicated in the event ID or payload.

## 6. Monitoring and alarms

EXN uses a **minimal Service 12 profile** for monitoring declared parameters/sensors.

The first baseline uses predeclared monitoring definitions rather than dynamic creation from Ground.

Each definition has:

- monitored parameter/sensor ID;
- low/high warning thresholds where meaningful;
- low/high critical thresholds where meaningful;
- hysteresis;
- dwell/debounce time;
- enabled state.

Threshold values are configuration parameters and are not hard-coded into the protocol.

On a limit transition:

1. the endpoint updates Service 12 monitor state;
2. the endpoint emits the appropriate Service 5 event;
3. the OBC records the alarm in System HK;
4. automatic recovery is only performed where an explicit policy exists.

Temperature, undervoltage, storage-low, link-loss and repeated service-crash alarms use this model.

## 7. Time model

### 7.1 OBC clock

The OBC is the on-board time authority.

It maintains two clocks:

- `monotonic_ms:uint64` — milliseconds since OBC boot, never stepped backwards;
- `unix_ms:uint64` — UTC milliseconds since Unix epoch, valid only after synchronization.

The mission clock therefore has **millisecond resolution**. Resolution is not a claim of millisecond synchronization accuracy; actual synchronization error is tracked separately.

### 7.2 Synchronization state

Common states:

- `UNSYNCED`;
- `SYNCED`;
- `HOLDOVER`;
- `INVALID`.

The OBC receives authoritative time from Ground or a future time source, then propagates it to all on-board endpoints.

EXN reserves:

- `TC[9,128]` — Set Mission Time.

Payload:

- `unix_ms:uint64`;
- `source:uint8`;
- `uncertainty_ms:uint16`;
- `sequence:uint16`.

The OBC sends the command to PI-CAM, PI-COMMS, PI-MGMT and FPGA after acquiring synchronization and periodically thereafter. Each endpoint should implement mission time as a local monotonic clock plus synchronized offset rather than depending on arbitrary wall-clock steps.

### 7.3 Packet timestamp

The normative PUS-C telemetry timestamp is six-octet CUC:

- four octets coarse seconds;
- two octets fine fraction;
- EXN epoch: Unix epoch, 1970-01-01T00:00:00Z.

This provides finer wire resolution than the application-level millisecond clock while preserving straightforward conversion to `unix_ms`.

## 8. Function management and component recovery

Standard `TC[8,1] Perform a Function` is used for management actions.

### 8.1 Function request encoding

EXN uses the standard Service 8 function identifier plus bounded arguments.

Function arguments follow:

```text
argument_count:u8
repeat argument_count:
    argument_id:u16
    value_type:u8
    value_length:u16
    value[value_length]
```

A function with no arguments has `argument_count=0`.

### 8.2 Common function IDs

| ID | Function | Typical target |
|---:|---|---|
| `0x0001` | clear recoverable faults | any managed endpoint |
| `0x0002` | reinitialize endpoint/application | Camera, Antenna, FPGA |
| `0x0003` | reboot node/platform | OBC, PI-MGMT, FPGA platform |
| `0x0004` | enter idle/safe application state | Camera, FPGA |
| `0x0005` | leave idle/safe application state | Camera, FPGA |
| `0x0006` | create diagnostic-log object | OBC and managed endpoints where supported |

### 8.3 Pi supervisor functions

PI-MGMT additionally provides:

| ID | Function |
|---:|---|
| `0x0101` | restart `exn-camera.service` |
| `0x0102` | restart `exn-antenna.service` |
| `0x0103` | restart both mission services |
| `0x0104` | reboot Raspberry Pi |
| `0x0105` | orderly Raspberry Pi shutdown |

OBC uses PI-MGMT for restart/recovery so a failed Camera or Antenna process is not required to receive its own restart request.

### 8.4 Recovery policy and state

The OBC owns automatic recovery policy. Endpoint self-protection may exist, but spacecraft-level recovery decisions are recorded and supervised by the OBC.

Recovery state:

| Value | State | Meaning |
|---:|---|---|
| 0 | `NONE` | no recovery active |
| 1 | `PENDING` | trigger condition met; waiting for configured recovery delay/cooldown |
| 2 | `ACTION_SENT` | recovery command/reset request issued |
| 3 | `WAITING_FOR_HEALTH` | waiting for heartbeat/HK after recovery action |
| 4 | `RECOVERED` | endpoint returned healthy |
| 5 | `FAILED` | recovery attempt failed or timed out |
| 6 | `SUPPRESSED` | policy wanted recovery but action was disabled/unavailable/exhausted |

Recovery reason:

| Value | Reason |
|---:|---|
| 0 | `NONE` |
| 1 | `HEARTBEAT_TIMEOUT` |
| 2 | `HEALTH_TIMEOUT` |
| 3 | `PROTOCOL_ERROR` |
| 4 | `CRASH_LOOP` |
| 5 | `THERMAL` |
| 6 | `POWER_FAULT` |
| 7 | `MANUAL` |

Recovery action:

| Value | Action | Requirement |
|---:|---|---|
| 0 | `NONE` | no action |
| 1 | `REINITIALIZE` | endpoint/service command path responsive |
| 2 | `RESTART_SERVICE` | supervisor such as PI-MGMT responsive |
| 3 | `SOFT_REBOOT` | endpoint management path responsive |
| 4 | `HARD_RESET` | independent reset control available |
| 5 | `POWER_CYCLE` | independent EPS/power-switch control available |

Last recovery result:

| Value | Result |
|---:|---|
| 0 | `NONE` |
| 1 | `SUCCESS` |
| 2 | `FAILED` |
| 3 | `SUPPRESSED` |

A completely unreachable endpoint cannot be recovered by a soft reboot command sent over the failed communication path. EXN shall therefore report `SUPPRESSED` when the configured action requires an independent reset/power path that is not implemented.

Recovery flags in System HK:

| Bit | Meaning |
|---:|---|
| 0 | automatic recovery enabled for this endpoint |
| 1 | recovery cooldown currently active |
| 2 | automatic retry limit reached |
| 3 | independent hard-reset path available |
| 4 | independent power-cycle path available |
| 5..7 | reserved |

Each endpoint supervision record stores:

- current recovery state/reason/action;
- cumulative recovery-attempt count;
- successful-recovery count;
- last recovery result;
- last recovery start/completion time;
- last recovery result/error;
- whether automatic recovery is currently inhibited by cooldown or retry limit.

### 8.5 Recovery journal and diagnostic logs

The OBC maintains a bounded persistent or retained **recovery journal** containing at minimum:

```text
record_id:u32
event_unix_ms:u64
endpoint_apid:u16
previous_supervision_state:u8
new_supervision_state:u8
recovery_reason:u8
recovery_action:u8
recovery_result:u8
heartbeat_age_ms:u32
health_age_ms:u32
endpoint_uptime_ms:u64      # last valid value if available
endpoint_restart_count:u32  # last valid value if available
error_code:u16
```

The latest recovery summary is visible in System HK. Detailed journal data is downloadable as a Service 220 object of type `DIAGNOSTIC_LOG` (`object_type=0x0020`). The baseline serialization is CBOR (`format=0x0007`) containing a bounded array of the recovery records above plus OBC software/version metadata.

Common Service 8 function `0x0006 CREATE_DIAGNOSTIC_LOG` asks the target/OBC to snapshot its available diagnostics into an immutable Service 220 object. Ground then downloads that object using the ordinary payload-object transfer mechanism.

A dead endpoint is not required to provide its own post-mortem log. The OBC recovery journal remains available even when the failed endpoint never returns.

## 8.6 Event ID namespaces

Service 5 event IDs are partitioned so logs can be decoded without guessing the producer-specific dictionary.

| Range | Owner |
|---|---|
| `0x1000..0x1FFF` | OBC |
| `0x2000..0x2FFF` | PI-CAM |
| `0x3000..0x3FFF` | FPGA |
| `0x4000..0x4FFF` | Antenna |
| `0x5000..0x5FFF` | PI-MGMT |

Each device ICD may allocate concrete IDs inside its range. Severity is carried by the standard Service 5 subtype rather than encoded in the event ID.

## 9. Camera service, Service 200

PI-CAM is designed around Raspberry Pi Camera Module 3 / IMX708, while capability reporting prevents the protocol from assuming every control is supported forever.

### 9.1 Camera messages

| Message | Name |
|---|---|
| `TC[200,1]` | Start Acquisition |
| `TC[200,2]` | Stop Acquisition |
| `TC[200,3]` | Set Camera Controls |
| `TC[200,4]` | Get Camera Controls |
| `TM[200,5]` | Camera Controls Report |
| `TC[200,6]` | Get Camera Capabilities |
| `TM[200,7]` | Camera Capabilities Report |
| `TC[200,8]` | List Products |
| `TM[200,9]` | Product Catalog Report |
| `TC[200,10]` | Delete Product |
| `TC[200,11]` | Get Acquisition Status |
| `TM[200,12]` | Acquisition Status Report |

### 9.2 Acquisition modes

`TC[200,1]` contains:

- `acquisitionId:uint32`;
- `mode:uint8`;
- `count:uint16`;
- `interval_ms:uint32`;
- `duration_ms:uint32`;
- `product_mask:uint16`;
- `flags:uint16`.

Modes:

| Value | Mode | Baseline |
|---:|---|---|
| 0 | SINGLE | required |
| 1 | BURST | required |
| 2 | TIMELAPSE | required |
| 3 | STREAM | reserved/optional initial implementation |

For SINGLE, `count=1`. For BURST, `count>=1`. TIMELAPSE uses `count` and `interval_ms`. STREAM is a long-running acquisition terminated by `TC[200,2]` or `duration_ms`.

### 9.3 Camera controls

The target control model includes:

- sensor mode/resolution;
- frame rate;
- crop/ROI;
- horizontal/vertical transform;
- AE enable/mode;
- exposure time;
- analogue gain;
- exposure compensation;
- metering mode;
- AWB enable/mode;
- manual red/blue gains;
- AF mode: manual / auto / continuous;
- AF trigger;
- lens position in **dioptres**;
- AF window/range where supported;
- HDR enable;
- brightness;
- contrast;
- saturation;
- sharpness;
- denoise mode;
- JPEG quality.

The Camera service shall report the actual supported controls, types and min/max/default values from the running camera/libcamera stack. The protocol does not fabricate a fixed range when the driver provides a different one.

Camera Module 3 reference capabilities include powered autofocus, non-HDR resolution up to 4608x2592 and RAW10 sensor output. These are capabilities of the selected reference camera, not universal protocol constraints.

### 9.4 Product creation

Each completed still acquisition has a `captureId` and may create one or more products from the same capture.

Product types:

| Bit | Product |
|---:|---|
| 0 | sensor RAW / Bayer object |
| 1 | DNG RAW container where supported |
| 2 | TIFF lossless image |
| 3 | JPEG compressed image |
| 4 | PNG image |
| 5 | preview/thumbnail |
| 6 | future video/stream artifact |

The implementation should preserve the sensor/raw product when requested, then derive TIFF/JPEG/etc. without overwriting the original.

Every product receives an opaque `objectId:uint64` and catalog metadata:

- capture/acquisition ID;
- object ID;
- creation `unix_ms`;
- format/type;
- width/height;
- pixel format;
- byte size;
- checksum;
- storage state.

Mission interfaces use object IDs rather than exposing Linux filesystem paths.

## 10. Antenna service

The Antenna (PI-COMMS) has no custom mission service in the first baseline.

It performs three routing functions:

```text
Ground TC -> Antenna -> OBC
OBC TM    -> Antenna -> Ground
payload   -> Antenna -> Ground
```

Payload sources are PI-CAM, FPGA, or OBC itself.

The OBC controls the downlink decision. The payload producer sends the bytes directly to Antenna using Service 220; the OBC does not relay the bulk object.

Antenna Service 20 parameters are:

| ID | Name | Type | Default |
|---:|---|---|---|
| `0x0001` | `link.enable` | BOOL | true |
| `0x0002` | `uplink.enable` | BOOL | true |
| `0x0003` | `downlink.enable` | BOOL | true |
| `0x0004` | `payload_forwarding.enable` | BOOL | true |
| `0x0005` | `rate_limit_bps` | U32 | 0 = deployment default/unlimited by software |

Technology-specific Wi-Fi/RF/network configuration remains deployment configuration unless a concrete operational requirement needs remote mutation.

The functional minimum therefore includes:

- link enable/disable;
- uplink enable/disable;
- downlink enable/disable;
- payload forwarding enable/disable;
- optional software rate limiting.

Antenna HK includes link state, Ground-session state, TX/RX packet and byte counters, error/drop counters, throughput and technology-specific signal metrics when available.

## 11. FPGA processing service, Service 210

The processing algorithm is intentionally not frozen yet. Service 210 therefore describes a generic job engine.

### 11.1 Processing messages

| Message | Name |
|---|---|
| `TC[210,1]` | Submit Processing Job |
| `TC[210,2]` | Cancel Processing Job |
| `TC[210,3]` | Get Job Status |
| `TM[210,4]` | Job Status Report |
| `TC[210,5]` | Get Processing Capabilities |
| `TM[210,6]` | Processing Capabilities Report |
| `TM[210,7]` | Small Processing Result |

A job contains:

- `jobId:uint32`;
- `operationId:uint16`;
- `inputObjectId:uint64`;
- parameter-set/reference;
- output policy;
- destination APID list;
- flags.

Capabilities report all available `operationId` values plus supported input/output object types and limits.

### 11.2 Result handling

A small result, such as a classification tuple or decision metadata, may use `TM[210,7]`.

A large result becomes a new Service 220 object and may be routed to:

- OBC for decision/control;
- PI-CAM for association/storage;
- Antenna for Ground downlink;
- more than one destination when explicitly requested.

The OBC may command:

```text
OBC -> FPGA: send result object Y to Ground
FPGA -> Antenna -> Ground
```

without relaying Y through the MCU.

## 11.3 Common payload object types and formats

`object_type:uint16` identifies mission meaning:

| Value | Object type |
|---:|---|
| `0x0001` | captured image |
| `0x0002` | derived/processed image |
| `0x0003` | processing mask/map |
| `0x0004` | processing result blob |
| `0x0005` | thumbnail/preview |
| `0x0010` | software update package |
| `0x0020` | diagnostic/recovery log |
| `0x7FFF` | mission-defined opaque object |

`format:uint16` identifies serialization:

| Value | Format |
|---:|---|
| `0x0001` | sensor RAW/Bayer |
| `0x0002` | DNG |
| `0x0003` | TIFF |
| `0x0004` | JPEG |
| `0x0005` | PNG |
| `0x0006` | binary/opaque |
| `0x0007` | CBOR |
| `0x0008` | JSON UTF-8 |
| `0x0100..0x7FFF` | mission/component-specific |

The Camera product catalogue maps its product-type mask to these object/format IDs.

## 12. Payload Object Transfer, Service 220

Service 220 is the common bounded object-transfer mechanism.

### 12.1 Messages

| Message | Name |
|---|---|
| `TC[220,1]` | Start Object Transfer |
| `TC[220,2]` | Stop Object Transfer |
| `TC[220,3]` | Get Transfer Status |
| `TM[220,10]` | Object Metadata |
| `TM[220,11]` | Object Chunk |
| `TM[220,12]` | Transfer Complete |
| `TM[220,13]` | Transfer Status |

`TC[220,1]` contains:

- `transferId:uint32`;
- `objectId:uint64`;
- destination APID;
- chunk size;
- flags.

The producer sends Metadata, Chunks and Complete directly to the requested destination.

### 12.2 Message payloads

`TC[220,1] Start Object Transfer`:

```text
transfer_id:u32
object_id:u64
destination_apid:u16
chunk_size:u16
flags:u16
```

`destination_apid=0x0F0` means Ground. On-board routing resolves that destination through PI-COMMS/Antenna.

`TM[220,10] Object Metadata`:

```text
transfer_id:u32
object_id:u64
object_type:u16
format:u16
created_unix_ms:u64
total_size:u64
checksum_type:u8      # 1 = SHA-256 in baseline
checksum[32]
metadata_length:u16
metadata[metadata_length]
```

`TM[220,11] Object Chunk`:

```text
transfer_id:u32
object_id:u64
offset:u64
data_length:u16
data[data_length]
```

`TM[220,12] Transfer Complete`:

```text
transfer_id:u32
object_id:u64
result:u8
bytes_transferred:u64
chunk_count:u32
checksum[32]
```

`TC[220,3] Get Transfer Status` contains `transfer_id:u32`.

`TM[220,13] Transfer Status`:

```text
transfer_id:u32
object_id:u64
state:u8             # QUEUED/RUNNING/COMPLETE/CANCELLED/FAILED
destination_apid:u16
bytes_transferred:u64
total_size:u64
last_error:u16
updated_unix_ms:u64
```

Chunking is application-level. CCSDS packets remain unsegmented unless a future interface explicitly adopts CCSDS segmentation.

### 12.3 Ground destination

The OBC never commands Camera/FPGA to open a Ground transport themselves.

A Ground transfer means:

```text
OBC -> producer: Start Object Transfer(objectId, destination=GROUND)
producer -> Antenna: Service 220 object
Antenna -> Ground: forward object packets
```

Routing resolves logical GROUND through the Antenna endpoint.

The Antenna preserves the original producer identity when forwarding complete EXN packets where the deployed link permits it.

### 12.4 OBC transfer registry

For every OBC-commanded transfer, the OBC records:

- transfer ID;
- source endpoint;
- object ID;
- destination;
- expected size if known;
- start time;
- current state;
- completion/error result.

This gives the OBC control/observability without making it the bulk-data path.

## 13. OBC orchestration service behavior

The OBC is the **spacecraft platform authority** and primarily a consumer/controller of subsystem services rather than another custom payload-service provider.

This contract currently enumerates the payload/data-handling components that exist in EXN. It does not limit future OBC authority. AOCS/ADCS, EPS, thermal, mechanisms and other platform controllers may later be added as subordinate services while keeping the same authority model: OBC owns spacecraft state/safety/mission decisions; distributed controllers own local execution.

It shall:

- maintain the endpoint registry and state;
- request/consume heartbeat and HK from every endpoint;
- aggregate System HK;
- synchronize all endpoint time;
- issue Service 8 recovery/restart commands;
- configure monitoring and parameters;
- control Camera acquisition;
- control FPGA processing;
- command Service 220 transfers;
- track expected transfers and completions;
- route Ground requests only after OBC supervision.

### 13.1 OBC orchestration parameters

OBC Service 20 parameters include:

| ID | Name | Type | Default |
|---:|---|---|---|
| `0x0001` | `supervision.heartbeat_period_ms` | U32 | 2000 |
| `0x0002` | `supervision.health_period_ms` | U32 | 10000 |
| `0x0003` | `supervision.missed_heartbeat_limit` | U8 | 3 |
| `0x0004` | `supervision.auto_recovery_enable` | BOOL | false |
| `0x0005` | `supervision.offline_after_ms` | U32 | 15000 |
| `0x0006` | `supervision.recovery_after_ms` | U32 | 60000 |
| `0x0007` | `supervision.recovery_cooldown_ms` | U32 | 120000 |
| `0x0008` | `supervision.max_auto_recoveries` | U8 | 3 |
| `0x0010` | `time.resync_period_ms` | U32 | 60000 |
| `0x0011` | `time.max_sync_age_ms` | U32 | deployment-defined |
| `0x0020` | `transfer.default_chunk_size` | U16 | deployment-defined |

Defaults are integration defaults and may be changed after measurement.

### 13.2 OBC HK cache and spacecraft snapshot

The OBC maintains the latest valid HEARTBEAT, HEALTH, SENSORS and LINKS structures received from every registered endpoint.

This cache is populated by normal spacecraft supervision, independently of Ground requests:

```text
OBC -> endpoint: TC[3,27]
endpoint -> OBC: TM[3,25]
OBC -> cache: update sample + supervision state
```

The OBC may use periodic endpoint-generated HK instead where configured, but the ownership rule is unchanged: the OBC decides the supervision cadence and owns the authoritative cache.

A Ground `TC[3,128]` does **not** select components and does not normally cause immediate downstream polling. It asks the OBC to serialize its current spacecraft snapshot into `TM[3,129]`.

If a subordinate endpoint is stale or offline, its entry remains in the snapshot with the corresponding OBC supervision state and its last valid sample time. Missing data is therefore itself telemetry rather than a reason for the complete System-HK response to fail.

Typical flow:

```text
Ground -> Antenna -> OBC
OBC -> Camera: acquire
Camera -> OBC: acquisition/product status
OBC -> FPGA: process object
FPGA -> OBC: classification result
OBC -> Camera/FPGA: transfer selected object to Ground
Camera/FPGA -> Antenna -> Ground
```

## 13.2 PI-MGMT platform parameters

PI-MGMT Service 20 parameters include:

| ID | Name | Type | Purpose |
|---:|---|---|---|
| `0x0001` | `watchdog.enable` | BOOL | platform watchdog policy |
| `0x0002` | `service.restart_limit` | U16 | restart-storm threshold |
| `0x0003` | `service.restart_window_ms` | U32 | restart-storm window |
| `0x0010` | `storage.payload_warn_free_bytes` | U64 | payload-store warning |
| `0x0011` | `storage.payload_critical_free_bytes` | U64 | payload-store critical |
| `0x0020` | `thermal.soc_warn_cC` | I16 | Pi SoC warning threshold |
| `0x0021` | `thermal.soc_critical_cC` | I16 | Pi SoC critical threshold |

Thermal values are deployment parameters, not protocol constants.

## 14. Software and service updates, Service 230

Software updates require explicit lifecycle control rather than treating an uploaded binary as executable merely because it arrived.

### 14.1 Software component IDs

| ID | Component |
|---:|---|
| `0x0001` | OBC firmware |
| `0x0101` | PI-CAM service |
| `0x0102` | Antenna service |
| `0x0103` | PI-MGMT/platform service |
| `0x0201` | FPGA node software/adapter |
| `0x0202` | FPGA bitstream/image |
| `0x0203` | FPGA processing capability/model bundle |

The last two/three IDs only become activatable when the selected FPGA platform defines a safe lifecycle for them.

### 14.2 Messages

| Message | Name |
|---|---|
| `TC[230,1]` | Get Software Inventory |
| `TM[230,2]` | Software Inventory Report |
| `TC[230,3]` | Stage Update |
| `TC[230,4]` | Activate Update |
| `TC[230,5]` | Roll Back Update |
| `TC[230,6]` | Get Update Status |
| `TM[230,7]` | Update Status Report |

Component IDs include:

- OBC firmware;
- PI-CAM service;
- Antenna service;
- Pi platform supervisor;
- FPGA node software;
- FPGA image/bitstream when that platform is selected.

### 14.3 Software-management payloads

`TC[230,1] Get Software Inventory` contains an optional bounded component-ID list; an empty list requests all managed components.

`TM[230,2] Software Inventory Report` contains bounded entries:

```text
count:u16
repeat count:
    component_id:u16
    state:u8
    version_length:u8
    version[version_length]       # max 32 bytes
    build_id:u32
    active_slot:u8
    rollback_available:u8
    package_hash[32]
```

`TC[230,3] Stage Update`:

```text
component_id:u16
package_object_id:u64
version_length:u8
version[version_length]           # max 32 bytes
expected_sha256[32]
flags:u16
```

`TC[230,4] Activate Update`:

```text
component_id:u16
activation_mode:u8                # immediate / next-restart
flags:u16
```

`TC[230,5] Roll Back Update` contains `component_id:u16`.

`TC[230,6] Get Update Status` contains `component_id:u16`.

`TM[230,7] Update Status Report`:

```text
component_id:u16
state:u8                         # IDLE/STAGED/VALIDATING/ACTIVATING/VERIFYING/COMPLETE/FAILED/ROLLED_BACK
progress_permille:u16
active_version_length:u8
active_version[...]
staged_version_length:u8
staged_version[...]
last_error:u16
updated_unix_ms:u64
```

All variable fields are bounded. Embedded endpoints may use fixed-size backing buffers and need not allocate dynamically.

### 14.4 Update flow

1. Transfer the update package as a Service 220 object to the update-owning endpoint.
2. `Stage Update` binds the object to a component/version.
3. Verify size, SHA-256 and compatibility metadata.
4. Verify package authenticity/signature when the signed-package profile is enabled.
5. `Activate Update`.
6. Restart/reboot only the smallest required component.
7. Perform post-activation heartbeat/HK validation.
8. Mark update successful or automatically restore the previous version when rollback support exists.

PI service updates are owned by PI-MGMT so Camera or Antenna can be updated/restarted independently.

No component update is considered complete until the target returns healthy after activation.

## 15. Scope intentionally left open

The following are intentionally not frozen yet:

- exact FPGA algorithms/models;
- physical Ground radio technology;
- automatic event-action recovery policies;
- autonomous Service 11 scheduling;
- FPGA bitstream update implementation details;
- cryptographic package-signing implementation.

Their interfaces have reserved architectural places, so selecting them later does not require reworking Camera/Antenna/OBC ownership.
