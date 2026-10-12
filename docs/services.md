# EXN PUS Service Profile

This document defines the EXN service-tailoring direction for CCSDS/PUS application traffic.

It separates three things that were previously mixed together:

1. the **current transitional wire profile**, which is already used by EXN-GS and the shared packet templates;
2. the **ECSS PUS service catalogue**, whose standardized service and message identifiers have fixed meanings;
3. the **target EXN mission profile**, which selects standard PUS services where they fit and uses mission-specific identifiers where EXN genuinely needs mission-specific behavior.

The exact packet layouts remain controlled by [ICD.md](ICD.md). This document is the service-level authority and migration plan.

## 1. Standards baseline

The service audit uses the published **ECSS-E-ST-70-41C, 15 April 2016** Packet Utilisation Standard as its normative PUS service reference.

CCSDSPack v2 supports standards-oriented PUS revision A and revision C secondary headers. EXN originally migrated its existing traffic to the PUS-A header profile because that minimized immediate changes to the ground/HIL implementation. That header choice did **not** make the existing EXN service-number assignments standard PUS services.

For a new EXN system baseline, the target is **PUS revision C service semantics and headers**, with mission tailoring documented here and in the master ICD.

This is a coordinated wire migration. Until the corresponding interface definitions, EXN-GS, simulator, and endpoint implementations are migrated together, the current PUS-A packet templates remain the executable compatibility baseline.

## 2. Service identifier rules

EXN follows the PUS-C identifier allocation rules:

- standard service types use identifiers in the standard range;
- mission-specific service types use identifiers **128..255**;
- within a standard service, standard message subtypes use the standardized identifiers;
- EXN-specific message types added to a standard service use subtypes **128..255**;
- all subtypes are available inside a mission-specific service type.

Consequently, EXN shall not redefine a standardized service/subservice identifier with unrelated semantics.

## 3. Audit of the current transitional profile

The current EXN interface set is internally understandable, but several assignments collide with standardized PUS-C meanings.

| Current EXN assignment | Current EXN meaning | PUS-C status | Target treatment |
|---|---|---|---|
| 3/1, 3/2 | request/report HK | Not the standard one-shot HK pair | Migrate to TC[3,27] / TM[3,25] |
| 3/10, 3/100 | System HK aggregation | EXN-specific subtypes inside a standard service | Move to mission-specific 3/128 and 3/129 |
| 5/1 | info event | Matches informative event report | Keep |
| 5/2 | warn event | Standard meaning is low-severity anomaly | Rename/align semantics |
| 5/3 | error event | Standard meaning is medium-severity anomaly | Align and add 5/4 high severity |
| 17/1, 17/2 | set/report time | PUS-C Service 17 is Test | Retire; use Service 9 |
| 20/1, 20/2, 20/3 | set/get/value | Standard Service 20 uses different subtype semantics | Migrate to standard 20/1, 20/2, 20/3 meanings |
| 23/* | image/result transfer | PUS-C Service 23 is File Management | Retire; use mission-specific Payload Object Transfer |
| 200/* | Camera control | Mission-specific service type | Keep and refine |
| 210/* | FPGA processing | Mission-specific service type | Keep and refine |
| 250/1 | GS link/proxy ACK | Mission-specific, but duplicates verification/link-local concerns | Deprecate unless a concrete mission requirement remains |

The current profile must therefore be described as **EXN legacy/transitional service numbering transported in a PUS-A secondary header**, not as a conformant realization of the corresponding PUS standard services.

## 4. Target EXN service catalogue

### 4.1 Service 1: Request Verification

Use standard PUS Service 1 for request verification instead of inventing per-service ACK/NACK packet types.

EXN initially needs:

- acceptance success/failure;
- completion success/failure;
- start/progress verification only where asynchronous operations benefit from them.

The TC acknowledgement flags shall be selected deliberately per command class. The current blanket value of zero is transitional behavior.

Existing Camera 200/5 and FPGA 210/5 ACK/NACK messages are retained only for compatibility until Service 1 migration is complete.

### 4.2 Service 3: Housekeeping

Use the standard one-shot housekeeping request/report pair:

- **TC[3,27]**: generate a one-shot housekeeping parameter report;
- **TM[3,25]**: housekeeping parameter report.

EXN's aggregated System HK remains useful but is mission-specific behavior added to the standard service using mission-specific subtype identifiers:

- **TC[3,128]**: Request System HK;
- **TM[3,129]**: System HK Report.

The current System HK application-data structure may be retained unless implementation work reveals a better aggregate representation.

### 4.3 Service 5: Event Reporting

Use the standard severity meanings:

- **TM[5,1]**: informative event;
- **TM[5,2]**: low-severity anomaly;
- **TM[5,3]**: medium-severity anomaly;
- **TM[5,4]**: high-severity anomaly.

EXN event-definition identifiers and auxiliary payloads are mission data layered onto this service.

### 4.4 Service 9: Time Management

Use PUS Service 9 for time management.

The standard profile provides, among other messages:

- **TC[9,1]**: set the time-report generation rate;
- **TM[9,2]**: CUC time report.

EXN also requires a way to initialize/correct endpoint mission time. Because that operation is not represented by the current standard message pair, reserve an EXN-specific message inside Service 9:

- **TC[9,128]**: Set Mission Time.

Its payload is the mission-selected CUC time representation plus any time-quality/source fields frozen by the final ICD. Verification is provided through Service 1.

The old EXN 17/1 and 17/2 assignments shall not be used by new implementations.

### 4.5 Service 20: Parameter Management

Adopt the standardized Service 20 message semantics:

- **TC[20,1]**: request/report parameter values;
- **TM[20,2]**: parameter value report;
- **TC[20,3]**: set parameter values.

The exact EXN parameter identifier and value representation shall be documented as mission tailoring. Existing one-byte keys may remain a valid implementation choice if they are represented consistently in all endpoints and tools.

The current EXN meaning of 20/1 = Set, 20/2 = Get, and 20/3 = Value is transitional and shall be migrated before the flight/payload nodes freeze their APIs.

### 4.6 Service 200: Camera Control

Service 200 is mission-specific and remains the Camera service.

Current functional scope is valid:

- capture;
- camera configuration;
- camera state/settings reporting.

The final profile shall use Service 1 for command verification, allowing the legacy Camera 200/5 ACK/NACK message to be retired.

### 4.7 Service 210: Payload Processing

Service 210 is mission-specific and remains the FPGA/image/AI processing service.

It owns:

- processing job submission;
- processing configuration;
- processing status/result metadata;
- controllable result routing.

Processing results are not hard-wired to a single endpoint. The job or mission policy may route a result to the OBC, Camera service, Antenna service, requester, or multiple endpoints.

The final Execute request shall include or reference:

- job identifier;
- processing/pipeline identifier;
- input object/reference;
- processing parameters;
- result route;
- delivery policy.

Service 1 provides command verification. The legacy FPGA 210/5 ACK/NACK message is therefore transitional.

### 4.8 Service 220: Payload Object Transfer

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
| TM[220,10] | Object Metadata |
| TM[220,11] | Object Chunk |
| TM[220,12] | Object Transfer Complete |

The common identifier is **objectId** or **transferId**, not imageId. Object metadata carries the object kind and any image-specific dimensions only when applicable.

PUS Service 13 remains available when EXN specifically needs the standardized **large PUS packet transfer** procedure. It is not used as a substitute for the mission-level payload-object model.

### 4.9 Service 23: File Management

Service 23 is reserved for its standardized PUS-C File Management meaning.

EXN shall not use Service 23 for generic image or result chunking in the target profile.

### 4.10 Antenna endpoint

PI-COMMS initially requires no dedicated mission-specific service.

Its management should use:

- Service 3 for housekeeping;
- Service 5 for link/service events;
- Service 9 for time where applicable;
- Service 20 for configurable parameters;
- Service 220 for payload-object forwarding where the packetized object-transfer path is used.

No separate 'communications' subsystem is introduced: PI-COMMS is the logical identifier for the Pi Antenna service. A dedicated antenna-control service shall only be allocated if concrete radio/link operations cannot be represented coherently by the common services.

### 4.11 Legacy Service 250

Service 250/1 currently represents a GS-originated link/proxy acknowledgement.

This is not part of the target service profile. Ground-daemon/link acknowledgement belongs to local ground IPC, while spacecraft request verification belongs to Service 1.

Service 250 remains reserved during migration so old HIL traffic can be recognized, but new mission behavior shall not depend on it.

## 5. Endpoint service matrix

The target service ownership is:

| Service | GS/HIL | OBC | Camera | FPGA processing | Antenna |
|---|---|---|---|---|---|
| 1 Request verification | consume | provide/consume | provide/consume | provide/consume | provide/consume |
| 3 Housekeeping | request/consume | provide + aggregate | provide | provide | provide |
| 5 Event reporting | consume | provide | provide | provide | provide |
| 9 Time management | request/consume | master/provider | consume/report | consume/report | consume/report |
| 20 Parameter management | request/consume | provide | provide | provide | provide |
| 200 Camera control | request | route | provide | - | transport only |
| 210 Processing | request | route/use results | request/use results | provide | transport/use routed result |
| 220 Payload object transfer | receive/request | optional route/receive | provide/receive | provide/receive | provide/receive/downlink |

"Route" does not mean that the OBC must carry bulk object bytes. The architecture continues to permit direct payload data paths.

## 6. PUS-C wire migration

The service normalization and the secondary-header migration must be performed as one controlled interface change.

The target baseline shall use CCSDSPack's PUS-C TC/TM types. At minimum the migration must freeze:

- PUS-C TC source identifiers;
- PUS-C TM destination identifiers;
- telemetry timestamp policy;
- acknowledgement-flag policy;
- service/subservice identifiers;
- packet sequence ownership;
- APID policy;
- CRC policy;
- application-data schemas.

Existing APID allocation remains valid unless a separate system decision changes it.

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

## 8. Migration sequence

1. Freeze this service-tailoring plan.
2. Freeze PUS-C header tailoring and request-verification policy.
3. Replace the legacy Service 17, 20 and 23 mappings in the master ICD.
4. Generalize payload transfer from `imageId` to object/transfer semantics under Service 220.
5. Migrate shared CCSDSPack templates, JSON/tooling definitions and MCU headers.
6. Migrate EXN-GS packet builders and simulator together.
7. Implement MCU, Camera, Antenna and FPGA endpoints against the normalized profile.
8. Add golden-vector and cross-repository interoperability tests.
9. Remove legacy service aliases only after no supported component depends on them.

## 9. Release gate

EXN shall not declare a coherent flight/payload integration baseline until:

- the target PUS service profile is frozen;
- all supported packet artifacts agree with it;
- EXN-GS and simulators interoperate with the same profile;
- each implemented endpoint declares which standard and mission-specific services it supports;
- request verification, error behavior and routing are covered by integration tests.

The architecture is already sufficiently stable to continue implementation. The service profile is the interface work that must be normalized before the currently placeholder MCU/FPGA repositories acquire incompatible APIs.
