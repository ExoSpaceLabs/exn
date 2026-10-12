# FPGA Processing Node Device/Service ICD

This document defines the target software-facing contract for the EXN FPGA Processing endpoint.

- APID: `0x102`
- Target source identity: `0x03`
- Mission role: generic image/AI processing endpoint
- Concrete processing algorithms/models: intentionally not frozen yet

Refer to:

- `../component-services.md` for the common component contract;
- `../services.md` for target PUS mapping;
- `../ICD.md` for packet/routing rules;
- `../architecture.md` for result-routing ownership.

Private/commercial FPGA SpaceWire repositories are outside EXN scope. The EXN FPGA exists to process payload data.

## 1. Service set

The FPGA endpoint provides/consumes:

- Service 1 Request Verification;
- Service 3 Housekeeping;
- Service 5 Event Reporting;
- Service 8 Function Management;
- Service 9 Time Management;
- Service 12 On-board Monitoring;
- Service 20 Parameter Management;
- Service 210 Processing;
- Service 220 Payload Object Transfer;
- Service 230 Software Management where the selected FPGA platform supports managed software/bitstream activation.

## 2. Endpoint state

Detailed state:

| State | Meaning |
|---|---|
| `STARTING` | processing endpoint is initializing |
| `READY` | able to accept jobs |
| `BUSY` | one or more jobs active |
| `DEGRADED` | reduced capability available |
| `ERROR` | processing unavailable |
| `IDLE` | deliberately quiesced |
| `UPDATING` | managed software/bitstream update underway |

The capability report declares concurrency/queue limits rather than assuming one job or unlimited jobs.

## 3. Service 210 message catalogue

| Message | Name |
|---|---|
| `TC[210,1]` | Submit Processing Job |
| `TC[210,2]` | Cancel Processing Job |
| `TC[210,3]` | Get Job Status |
| `TM[210,4]` | Job Status Report |
| `TC[210,5]` | Get Processing Capabilities |
| `TM[210,6]` | Processing Capabilities Report |
| `TM[210,7]` | Small Processing Result |

Service 1 provides command acceptance/completion verification.

## 4. Processing capabilities

Because EXN has not selected the final processing workload, the FPGA shall advertise capabilities dynamically.

Each capability contains:

- `operationId:uint16`;
- stable operation name/version;
- accepted input object types/formats;
- generated output type(s);
- minimum/maximum dimensions or object size;
- supported parameter schema/version;
- expected concurrency/queue limits;
- whether the result can be returned inline or only as an object.

Examples of possible future operation classes include preprocessing, classification, segmentation, detection or compression, but none is mandatory merely because this ICD names the category.

## 5. Submit Processing Job

`TC[210,1]` contains or references:

| Field | Type | Meaning |
|---|---|---|
| `jobId` | uint32 | OBC-assigned job identity |
| `operationId` | uint16 | advertised processing capability |
| `inputObjectId` | uint64 | input object already available to FPGA or transferred separately |
| `parameterSetId` | uint16 | optional stored parameter set; zero for command-supplied/default |
| `destinationCount` | uint8 | number of result consumers |
| `destinationApid[]` | uint16[] | OBC, Camera, Antenna, requester, etc. |
| `flags` | uint16 | execution/output policy |

The command never hard-codes one pipeline or result destination.

## 6. Input data

Large input imagery/products use Service 220.

Typical flow:

```text
OBC -> PI-CAM: transfer object X to FPGA
PI-CAM -> FPGA: Service 220 object X
OBC -> FPGA: Submit Job(inputObjectId=X)
```

The implementation may support streaming input later, but the first baseline is object-oriented and bounded.

## 7. Result handling

### 7.1 Small result

Small decision data such as:

- class ID and confidence;
- scalar quality metrics;
- bounding metadata;
- compact status/decision vectors;

may be delivered through `TM[210,7]` directly to the requested endpoint.

Example:

```text
FPGA -> OBC: classification result
OBC: make mission decision
```

### 7.2 Large result

A large output becomes a Service 220 object with its own `objectId`.

Examples:

```text
OBC -> FPGA: downlink result Y
FPGA -> Antenna -> Ground

OBC -> FPGA: return result Y to Camera
FPGA -> PI-CAM
```

The FPGA does not own the Ground link. "Route to Ground" resolves to the Antenna endpoint.

## 8. Job status

`TM[210,4]` reports:

- job ID;
- operation ID;
- state: QUEUED/RUNNING/COMPLETE/CANCELLED/FAILED;
- progress permille when meaningful;
- start/end mission time;
- result code;
- output object IDs, if any;
- small-result availability;
- error/detail code.

The OBC uses this report to manage expected jobs and downstream transfers.

## 9. Housekeeping

### HEARTBEAT

- endpoint state;
- software/bitstream version;
- capability-set version/hash;
- active/queued job count;
- mission time-sync state;
- uptime;
- last error;
- reset count.

### HEALTH

- processing queue depth;
- completed/failed/cancelled job counters;
- input/output byte counters;
- buffer/DMA error counters where applicable;
- last/mean processing duration by available metric;
- resource utilization metrics exposed by the selected platform.

### SENSORS

Report every actual hardware monitor exposed by the selected FPGA platform, such as:

- die temperature;
- rail voltages/currents;
- fan/thermal state;

**only after the board is selected and those monitors are confirmed available**.

The previous speculative fixed VCCINT/VCCAUX/VCCBRAM table is not a cross-platform requirement.

## 10. Monitoring and alarms

Predeclared Service 12 definitions shall cover available FPGA health parameters, including where supported:

- die-temperature warning/critical;
- rail out-of-range;
- repeated DMA/buffer failure;
- processing timeout/error-rate threshold;
- queue/resource saturation.

Limit transitions produce Service 5 events and are included in OBC/System HK state.

## 11. Function management

Service 8 functions include:

| Function | Meaning |
|---|---|
| clear faults | clear recoverable processing errors |
| reinitialize endpoint | reset software-facing processing engine |
| enter idle | stop accepting new jobs after controlled drain/cancel policy |
| leave idle | return to READY |
| reboot/reload node | platform-specific reset/reload where supported |

A hard reset must not be used as normal job cancellation.

## 12. Time

The FPGA consumes OBC `TC[9,128] Set Mission Time` and uses synchronized mission time for:

- job start/end timestamps;
- result metadata;
- Housekeeping/event timestamps.

## 13. Software/bitstream updates

Service 230 applies once the selected FPGA platform provides a safe update/rollback mechanism.

Inventory shall distinguish:

- host/software adapter version;
- FPGA image/bitstream version;
- processing capability-set/model version.

The package arrives via Service 220, is validated, activated, and followed by heartbeat/capability/HK validation.

Until the platform is selected, the exact bitstream reload mechanism remains deliberately undefined.

## 14. Parameter management

Service 20 holds persistent/runtime parameters that are not operation-specific.

Examples may include queue policy, timeout limits and default result routing. Algorithm-specific parameters belong to the Service 210 operation parameter schema rather than a global bag of FPGA settings.
