# EXN PUS Service Profile

This document is the **normative EXN PUS service catalogue and tailoring specification** for CCSDS/PUS application traffic.

It separates three things that must remain distinct:

1. the **ECSS PUS service catalogue**, whose standardized service and message identifiers have fixed meanings;
2. the **active EXN PUS-C mission profile**, which selects standard PUS services where they fit and mission-specific identifiers where EXN genuinely needs custom behavior;
3. the endpoint-specific operational payloads and state models defined by the central EXN ICD.

The exact common packet/header rules remain controlled by [ICD.md](ICD.md); endpoint-specific payloads and IDs are frozen in [component-services.md](component-services.md) and `docs/icd/`. Component repositories consume these definitions and shall not maintain independent service-number or payload-layout specifications.

## 1. Standards baseline

EXN uses **ECSS-E-ST-70-41C (15 April 2016), PUS Revision C** as its only active PUS profile. This is the currently published active ECSS Packet Utilisation Standard; later public-review drafts are not treated as normative until formally published.

CCSDSPack v2.0.0 provides the required PUS-C TC/TM secondary headers, fixed two-octet TC source ID and TM destination ID, acknowledgement flags, message type counter and CUC telemetry time support.

PUS-A is historical for EXN. Existing code that still uses Revision A is migration work, not a second supported mission profile.

The normative packet/header tailoring is defined in `ICD.md`; this document defines what each PUS service means in EXN.

## 2. Service identifier rules

EXN follows the PUS-C identifier allocation rules:

- standard service types use identifiers in the standard range;
- mission-specific service types use identifiers **128..255**;
- within a standard service, standard message subtypes use the standardized identifiers;
- EXN-specific message types added to a standard service use subtypes **128..255**;
- all subtypes are available inside a mission-specific service type.

Consequently, EXN shall not redefine a standardized service/subservice identifier with unrelated semantics.

## 3. Service-definition rule

### 3.1 Command granularity rule

EXN allocates subservices by **operation class**, not by individual configuration field.

A component shall prefer one bounded structured TC that changes a coherent configuration set over many nearly-identical TCs such as `SetExposure`, `SetGain`, `SetFocus`, and `SetJpegQuality`.

Examples:

- Camera image configuration -> one `TC[200,3]` batch;
- Camera acquisition -> one Start Acquisition TC;
- FPGA processing -> one Submit Job TC with operation/parameter descriptors;
- runtime policy -> Service 20 parameter batches;
- recovery action -> one Service 8 function TC with function ID/arguments.

Separate subservices are justified when the operation has meaningfully different lifecycle, verification, timing, safety, or result semantics.

EXN shall not freeze a service merely by assigning it a number and a short name.

Every service definition shall explain:

1. why the service exists;
2. which endpoints provide/consume it;
3. what belongs in it, with concrete examples;
4. what similar operations explicitly belong elsewhere;
5. its service/subservice allocation;
6. its payload/state/error semantics.

This is especially important for generic PUS services such as Service 20. A generic standard service is used only when its responsibility is clearer than a mission-specific alternative.

## 4. Active EXN service catalogue

### 4.1 Service 1: Request Verification

Use standard PUS Service 1 for request verification instead of inventing per-service ACK/NACK packet types.

EXN initially needs:

- acceptance success/failure;
- completion success/failure;
- start/progress verification only where asynchronous operations benefit from them.

The TC acknowledgement flags are selected deliberately per command class as defined in `ICD.md`. EXN does not use mission-specific Camera/FPGA ACK packets; Service 1 is the single request-verification mechanism.

### 4.2 Service 3: Housekeeping

Use the standard housekeeping messages needed by EXN:

- **TC[3,5]**: enable periodic housekeeping report generation;
- **TC[3,6]**: disable periodic housekeeping report generation;
- **TC[3,27]**: generate a one-shot housekeeping parameter report;
- **TM[3,25]**: housekeeping parameter report.

EXN uses predeclared structure IDs HEARTBEAT/HEALTH/SENSORS/LINKS rather than requiring Ground to create arbitrary report definitions in the first baseline.

**Purpose:** Service 3 reports what a component *is doing and what its hardware is measuring*. The `SENSORS` structure is not generic hand-waving: for selected reference hardware its sensor IDs, units and required availability are defined centrally in `icd/hardware-sensors.md`. `HEALTH` contains software/resource state such as CPU load, storage, throttling flags and reset reason. Threshold evaluation belongs to Service 12; changing threshold/policy values belongs to Service 20.

EXN's aggregated System HK remains useful but is mission-specific behavior added to the standard service using mission-specific subtype identifiers:

- **TC[3,128]**: Request System HK;
- **TM[3,129]**: System HK Report.

`TC[3,128]` has no Application Data: Ground asks for the complete current spacecraft snapshot. `TM[3,129]` contains the OBC's cached HK view of every registered endpoint, including explicit supervision state, last-report ages and compact recovery-policy state for stale/offline/non-responsive devices. Poll cadence, stale/offline thresholds and automatic-recovery timing are OBC Service 20 configuration, not per-request Ground options. Detailed recovery journals are exported as Service 220 diagnostic-log objects.

### 4.3 Service 5: Event Reporting

Use the standard severity meanings:

- **TM[5,1]**: informative event;
- **TM[5,2]**: low-severity anomaly;
- **TM[5,3]**: medium-severity anomaly;
- **TM[5,4]**: high-severity anomaly.

EXN event-definition identifiers and auxiliary payloads are mission data layered onto this service.

### 4.4 Service 8: Function Management

Use standard PUS Service 8 for endpoint/platform management functions.

The EXN baseline requires `TC[8,1]` Perform a Function for:

- clear/reinitialize actions;
- Camera/Antenna service recovery through PI-MGMT;
- OBC/FPGA/Pi reboot where supported;
- entry/exit from endpoint idle/safe application state.

Function IDs and target-specific behavior are defined in `component-services.md`. Request verification uses Service 1.

### 4.5 Service 9: Time Management

Use PUS Service 9 for time management.

The standard profile provides, among other messages:

- **TC[9,1]**: set the time-report generation rate;
- **TM[9,2]**: CUC time report.

EXN also requires a way to initialize/correct endpoint mission time. Because that operation is not represented by the current standard message pair, reserve an EXN-specific message inside Service 9:

- **TC[9,128]**: Set Mission Time.

Its EXN Application Data is fixed as:

```text
unix_ms:uint64
source:uint8
uncertainty_ms:uint16
sequence:uint16
```

The OBC is the time authority and distributes this command to PI-CAM, Antenna, PI-MGMT and FPGA. Endpoint application clocks derive synchronized Unix time from a monotonic clock plus offset. The PUS-C **telemetry timestamp** is the mission CUC representation defined in `ICD.md` and `component-services.md`.

Verification is provided through Service 1.

The old EXN 17/1 and 17/2 assignments shall not be used by new implementations.

### 4.6 Service 12: On-board Monitoring

Use a deliberately small PUS Service 12 profile for sensor/health limit monitoring.

The initial message subset is:

- **TC[12,15]** / **TC[12,16]**: enable/disable the parameter monitoring function;
- **TC[12,1]** / **TC[12,2]**: enable/disable predeclared parameter monitoring definitions;
- **TC[12,10]** / **TM[12,11]**: request/report current out-of-limits state;
- **TC[12,13]** / **TM[12,14]**: request/report monitoring-definition status.

The first EXN baseline uses **predeclared monitoring definitions** rather than dynamic creation of arbitrary definitions from Ground. Monitoring covers temperature, voltage/power-state indicators, storage thresholds, link loss and other endpoint-specific health parameters actually available on the selected hardware.

Thresholds, hysteresis and dwell/debounce values are configurable parameters. Limit transitions emit Service 5 events and are reflected in Housekeeping/System HK.

The target may later expand to additional Service 12 capabilities without changing the endpoint ownership model.

### 4.7 Service 20: On-board Parameter Management
#### EXN use: runtime configuration parameters

**Purpose:** provide one standard way to read and change bounded runtime or persistent **configuration values**.

A Service 20 parameter is a setting. It is not an operation.

Examples that belong here:

- OBC heartbeat period and missed-heartbeat limit;
- OBC automatic-restart enable;
- OBC time-resynchronization period;
- Antenna uplink/downlink/payload-forwarding enable flags;
- Antenna software rate limit;
- PI-MGMT restart-storm, storage and thermal thresholds;
- FPGA default timeout/queue policy when not tied to one processing operation.

Examples that do **not** belong here:

- Camera exposure, autofocus, sensor mode or JPEG capture controls -> Service 200;
- start/stop acquisition -> Service 200;
- restart/reboot -> Service 8;
- synchronize time -> Service 9;
- submit/cancel FPGA job -> Service 210;
- transfer an image/result/package -> Service 220;
- activate/rollback software -> Service 230;
- request HK -> Service 3.

EXN uses the standard PUS-C message semantics:

- **TC[20,1]** request/report parameter values;
- **TM[20,2]** parameter value report;
- **TC[20,3]** set parameter values.

The common EXN parameter-ID/value encoding is defined in `component-services.md`. An endpoint may legitimately implement zero Service 20 parameters if all of its meaningful configuration belongs to a mission-specific service.

### 4.8 Service 200: Camera Control

Service 200 is mission-specific and defines the PI-CAM application interface.

Target functions include:

- start/stop acquisition;
- SINGLE, BURST and TIMELAPSE acquisition, with STREAM reserved as an optional/future mode;
- atomically set a bounded batch of Camera configuration fields and query active configuration by scope;
- report camera capabilities and driver-supported ranges;
- list/delete stored camera products;
- report acquisition status.

A completed acquisition may retain sensor RAW and derive DNG/TIFF/JPEG/PNG/preview products without overwriting the source object. Product transfer itself uses Service 220.

Target message allocation:

| Message | Meaning |
|---|---|
| TC[200,1] | Start Acquisition |
| TC[200,2] | Stop Acquisition |
| TC[200,3] | Set Camera Configuration |
| TC[200,4] | Get Camera Configuration |
| TM[200,5] | Camera Configuration Report |
| TC[200,6] | Get Camera Capabilities |
| TM[200,7] | Camera Capabilities Report |
| TC[200,8] | List Products |
| TM[200,9] | Product Catalog Report |
| TC[200,10] | Delete Product |
| TC[200,11] | Get Acquisition Status |
| TM[200,12] | Acquisition Status Report |

The configuration model is defined in `component-services.md` and the PI-CAM ICD. Exposure, gain, focus, white balance, crop, HDR and encoding values are fields inside the single Camera configuration transaction rather than independent TC subservices.

Service 1 provides verification for the complete configuration transaction.

### 4.9 Service 210: Payload Processing

Service 210 is mission-specific and remains the FPGA/image/AI processing service.

It owns:

- processing job submission;
- processing configuration;
- processing status/result metadata;
- controllable result routing.

Processing results are not hard-wired to a single endpoint. The job or mission policy may route a result to the OBC, Camera service, Antenna service, requester, or multiple endpoints.

The target job request includes or references:

- job identifier;
- operation/capability identifier;
- input object/reference;
- processing parameters;
- destination endpoint list;
- output/delivery policy.

Target message allocation:

| Message | Meaning |
|---|---|
| TC[210,1] | Submit Processing Job |
| TC[210,2] | Cancel Processing Job |
| TC[210,3] | Get Job Status |
| TM[210,4] | Job Status Report |
| TC[210,5] | Get Processing Capabilities |
| TM[210,6] | Processing Capabilities Report |
| TM[210,7] | Small Processing Result |

Service 210 reports supported processing capabilities because the concrete FPGA workload is intentionally not frozen yet.

Small results may be returned directly through Service 210. Large results become Service 220 objects.

Service 1 provides command verification. Service 210 does not define a parallel FPGA-specific ACK/NACK packet.

### 4.10 Service 220: Payload Object Transfer

Reserve **Service 220** as the EXN mission-specific Payload Object Transfer service.

This replaces the legacy use of Service 23 for images and processing results.

The service transports mission payload objects such as:

- captured images;
- processed images;
- masks;
- inference result blobs;
- thumbnails;
- other bounded payload products.

Initial message allocation:

| Message | Meaning |
|---|---|
| TC[220,1] | Start Object Transfer |
| TC[220,2] | Stop Object Transfer |
| TC[220,3] | Get Transfer Status |
| TM[220,10] | Object Metadata |
| TM[220,11] | Object Chunk |
| TM[220,12] | Object Transfer Complete |
| TM[220,13] | Transfer Status |

The common identifiers are **objectId** and **transferId**, not imageId. Object metadata carries the object kind and any image-specific dimensions only when applicable.

For OBC-commanded Ground downlink, the payload producer sends the object directly to the Antenna endpoint and the Antenna forwards it to Ground. The OBC tracks the transfer but does not relay bulk bytes.

PUS Service 13 remains available when EXN specifically needs the standardized **large PUS packet transfer** procedure. It is not used as a substitute for the mission-level payload-object model.

### 4.11 Service 23: File Management

PUS-C Service 23 keeps its standardized File Management meaning.

EXN does **not** currently require a mission-facing arbitrary file-management interface. Mission payload products use Service 220 object transfer, and software activation uses Service 230.

Service 23 is therefore **not part of the initial implemented EXN service set**. It remains reserved for future use only if a concrete file-management requirement appears.

### 4.12 Antenna endpoint

PI-COMMS initially requires no dedicated mission-specific service.

Its management should use:

- Service 3 for housekeeping;
- Service 5 for link/service events;
- Service 9 for time where applicable;
- Service 20 for configurable parameters;
- Service 220 for payload-object forwarding where the packetized object-transfer path is used.

No separate 'communications' subsystem is introduced: PI-COMMS is the logical identifier for the Pi Antenna service. A dedicated antenna-control service shall only be allocated if concrete radio/link operations cannot be represented coherently by the common services.

### 4.13 Service 230: Software Management

Reserve **Service 230** as the EXN mission-specific Software Management service.

Target message allocation:

| Message | Meaning |
|---|---|
| TC[230,1] | Get Software Inventory |
| TM[230,2] | Software Inventory Report |
| TC[230,3] | Stage Update |
| TC[230,4] | Activate Update |
| TC[230,5] | Roll Back Update |
| TC[230,6] | Get Update Status |
| TM[230,7] | Update Status Report |

Update package bytes are transferred as bounded objects using Service 220; Service 230 controls the lifecycle and activation.

PI-MGMT owns Camera/Antenna/platform service updates so a mission service does not need to update or restart itself. OBC and FPGA software providers own their corresponding update targets.

An update is complete only after post-activation heartbeat/HK validation succeeds.

## 5. Endpoint service matrix

The target service ownership is:

| Service | GS/HIL | OBC | Camera | FPGA processing | Antenna | PI-MGMT |
|---|---|---|---|---|---|---|
| 1 Request verification | consume | provide/consume | provide/consume | provide/consume | provide/consume | provide/consume |
| 3 Housekeeping | request/consume | provide + aggregate | provide | provide | provide | provide |
| 5 Event reporting | consume | provide | provide | provide | provide | provide |
| 8 Function management | request | provide/consume | provide limited | provide | provide limited | provide platform/service recovery |
| 9 Time management | request/consume | **master/provider** | consume/report | consume/report | consume/report | consume/report |
| 12 On-board monitoring | request/consume | provide/aggregate | provide | provide | provide | provide |
| 20 Parameter management | request/consume | provide | provide | provide | provide | provide |
| 200 Camera control | request | route/control | provide | - | transport only | supervise only |
| 210 Processing | request | route/control/use results | request/use results | provide | downlink routed result | - |
| 220 Payload object transfer | receive/request | control/optional receive | provide/receive | provide/receive | downlink/receive | receive update packages |
| 230 Software management | request | provide/control | via PI-MGMT | provide | via PI-MGMT | provide for Pi services |

"Route" does not mean that the OBC must carry bulk object bytes. The architecture continues to permit direct payload data paths.

## 6. PUS-C wire profile requirement

All active EXN interface artifacts and component implementations use the PUS-C profile defined in `ICD.md`.

Required properties include:

- `PUS:revC:TC` / `ccsds::pus::rev_c::TcHeader`;
- `PUS:revC:TM` / `ccsds::pus::rev_c::TmHeader`;
- fixed two-octet PUS-C source/destination IDs;
- command-specific PUS-C acknowledgement flags;
- PUS-C message type counters;
- six-octet CUC TM timestamps using the EXN mission epoch.

Revision A shall not be added to new tests, fixtures, templates or implementation paths.

## 7. Interface source of truth

The long-term interface chain shall be:

```text
service/profile specification
        |
        v
machine-readable mission interface definitions
        |
        +--> CCSDSPack templates/tests
        +--> MCU C headers
        +--> GS constants/builders
        +--> simulator fixtures
        +--> documentation tables
```

The current manually maintained `.cfg`, JSON and C-header mirrors are acceptable during migration, but the target baseline should generate or cross-validate them from one machine-readable definition.

No new interface shall be added using a legacy-colliding assignment merely because an older EXN packet already used that number.

## 8. Implementation order

1. Maintain the full normative interface in this repository.
2. Keep CCSDSPack templates, JSON/tooling artifacts and MCU headers synchronized with the PUS-C ICD.
3. Implement/migrate EXN-GS and its simulator to the same PUS-C definitions.
4. Implement the OBC against the same shared contract.
5. Implement Camera, Antenna and PI-MGMT against their endpoint ICDs.
6. Implement FPGA processing against Service 210/220 and the common management services.
7. Add cross-repository golden-vector and interoperability tests.
8. Reject any implementation-local protocol extension that has not first been defined here.

## 9. Release gate

EXN shall not declare a coherent integration baseline until:

- all supported packet artifacts use PUS Revision C;
- GS/HIL, simulator and endpoint implementations consume the same service/subservice catalogue;
- request verification uses Service 1 rather than mission-specific ACK packets;
- Service 20 parameters are explicitly documented as configuration values with stable IDs;
- heartbeat/HK, monitoring, time synchronization, recovery, object transfer and software-update behavior have integration tests;
- no active component depends on a PUS-A compatibility path.
