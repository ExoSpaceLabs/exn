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
- `../architecture.md` for result-routing ownership;
- `hardware-sensors.md` for the normative K26/KV260 sensor catalogue;
- `inter-node-links.md` for the KV260 PL/Pmod UART control interface from OBC and dedicated Pi↔KV260 Gigabit Ethernet image/result data path.

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

The OBC instructs PI-CAM to transfer input object X directly to FPGA using Service 220, then submits `TC[210,1]` with `inputObjectId=X`. See the [processing-data sequence](../diagrams.md#3-capture---fpga-processing---camera-product---ground).

The implementation may support streaming input later, but the first baseline is object-oriented and bounded.

## 7. Result handling

### 7.1 Small result

Small decision data such as:

- class ID and confidence;
- scalar quality metrics;
- bounding metadata;
- compact status/decision vectors;

may be delivered through `TM[210,7]` directly to the requested endpoint.

For example, FPGA sends a compact classification result to OBC, which evaluates it against mission policy. See the [classification sequence](../diagrams.md#4-classification-result-routed-to-obc).

### 7.2 Large result

A large output becomes a Service 220 object with its own `objectId`.

The OBC can direct a result object to **Ground via Antenna**, to **PI-CAM** for storage/association, or to another configured endpoint. See [direct FPGA downlink](../diagrams.md#5-obc-controlled-direct-fpga-result-downlink).

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

The EXN reference FPGA hardware is profiled explicitly:

- **Kria K26 SOM profile `0x0201`**: junction temperature and required PS/PL supply-voltage measurements from the Zynq UltraScale+ System Monitor;
- **KV260 profile `0x0202`**: all K26 sensors plus SOM 5 V voltage/current/power from the carrier INA260.

Stable sensor IDs, units and required/optional status are defined in [hardware-sensors.md](hardware-sensors.md).

If a different FPGA board is adopted later, it receives a new hardware profile and sensor table in the central ICD before its telemetry is treated as normative.

## 10. Monitoring and alarms

Predeclared Service 12 definitions shall cover the active hardware-profile sensors, including:

- K26 junction-temperature warning/critical;
- required PS/PL rail out-of-range;
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
