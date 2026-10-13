# EXN System Architecture

This document defines the logical EXN system architecture and the ownership boundaries between the control computer, Linux payload services, FPGA processing, ground communications, and reusable platform libraries.

The master wire-level contract remains [`docs/ICD.md`](ICD.md), [`docs/services.md`](services.md) defines the mission PUS service tailoring, and [`docs/component-services.md`](component-services.md) defines the complete endpoint behavior. This document describes how EXN components are intended to be composed and how control/data may flow between them without forcing one fixed mission pipeline.

## 1. Architectural principles

EXN is a modular avionics and payload demonstrator. The architecture intentionally separates **logical spacecraft endpoints** from the **physical computers** that currently host them.

The main rules are:

1. **STM32/OBC is the spacecraft platform authority.** It owns spacecraft mode, mission sequencing, command authorization/dispatch, time authority, spacecraft-wide supervision and FDIR. Platform and payload subsystems may execute local control/processing, but they remain subordinate to OBC authority.
2. **The Raspberry Pi 5 is a dual-role Embedded Linux payload node.** In the initial EXN deployment it performs both the Camera role and the Antenna role. Those roles are separate logical endpoints/services even though they share one physical computer.
3. **The FPGA is a processing node.** Its EXN role is image preprocessing, inference, postprocessing, and other hardware-accelerated payload computation.
4. **Processing results do not have one fixed destination.** The requester or mission policy selects where a result is delivered.
5. **Bulk payload data need not transit the MCU.** Control traffic should remain MCU-centric, while high-volume data paths may connect payload endpoints directly.
6. **Transport is below mission semantics.** Packet/service behavior must not depend on whether the current carrier is Ethernet, a simulated SpaceWire link, local Linux IPC, or a future physical transport.
7. **Reusable libraries remain independent projects.** EXN consumes CCSDSPack, HardRT, DAS, and optionally SpWKit. Their internal implementation scope is not part of the EXN mission architecture.
8. **PUS services are explicitly tailored.** CCSDSPack supplies the packet/header mechanisms; EXN defines which standard PUS services it realizes and which mission-specific services it adds. Service identifiers shall not be repurposed with unrelated semantics.

## 1.1 Centralized authority, distributed execution

EXN distinguishes **authority** from **execution location**.

The OBC does not need to execute every algorithm itself. A future AOCS controller may run attitude estimation and high-rate wheel/torquer loops; an EPS controller may perform power switching/protection; a thermal controller may run heater loops; an HPC may perform image/AI processing. Those are distributed execution resources.

The OBC remains responsible for:

- spacecraft mode and safe-state ownership;
- mission/platform sequencing;
- telecommand authorization and dispatch;
- subsystem enable/disable and high-level set-points;
- health aggregation and FDIR;
- on-board time authority and distribution;
- platform-wide recovery decisions;
- deciding when payload products/processors may influence spacecraft actions.

A subsystem may autonomously protect itself locally, but it shall not independently redefine spacecraft mission state or safety policy.

The current EXN stage focuses on the payload/data-handling subset: Camera, Antenna, FPGA processing and Pi management. AOCS/ADCS, EPS, thermal and other platform subsystems are future scope and therefore intentionally lack detailed service ICDs today.

## 2. Logical system

```mermaid
graph LR
  GS[Ground Station]
  OBC[STM32 OBC / Control Node]
  CAM[Camera Payload Service]
  ANT[Antenna Service]
  PM[PI-MGMT / Platform Supervisor]
  FPGA[FPGA AI / Processing Node]

  GS <--> ANT
  ANT <--> OBC
  OBC <--> CAM
  OBC <--> PM
  OBC <--> FPGA
  PM --- CAM
  PM --- ANT
  CAM <--> FPGA
  CAM <--> ANT
  FPGA <--> ANT
```

The arrows above represent **permitted logical paths**, not a requirement that every deployment instantiate every path or transport.

The control architecture remains centered on the OBC, but the data plane is intentionally more flexible.

## 3. Physical deployment

The initial intended deployment is:

```text
Ground Station
      |
      | RF / Wi-Fi / network link as deployed
      v
+------------------------------------------------+
| Raspberry Pi 5 - Embedded Linux Payload Node   |
|                                                |
|  Camera Service       Antenna Service          |
|        |                        |               |
|        +-------- PI-MGMT -------+               |
|          watchdog / restart / health / update  |
+----------------------+-------------------------+
                       |
                 Ethernet / other carrier
              +--------+---------+
              |                  |
              v                  v
       STM32 Control Node     FPGA Processing Node
       HardRT + DAS           Image/AI acceleration
```

Camera and Antenna are therefore **two first-class responsibilities of the initial Raspberry Pi 5** while remaining independent logical services. The Pi is expected to do both: acquire/manage payload imagery and provide the spacecraft communications endpoint toward Ground/OBC. A later deployment may move either service to a different Linux computer without changing its mission responsibility.

## 4. Raspberry Pi dual-role payload node

The Raspberry Pi shall be treated as one physical spacecraft payload computer with two mission-facing functions rather than as one camera executable with optional extras:

1. **PI-CAM** acquires and owns payload imagery/products.
2. **PI-COMMS is the Antenna service.** It owns the Ground-facing link, exchanges TCs/TMs with the OBC, and transmits payload products from Camera or FPGA to Ground.

Linux platform management supervises both functions but is not itself a third primary mission role. It is exposed as supporting endpoint **PI-MGMT (`0x104`)** so the OBC can restart/update Camera or Antenna even when the target mission process is not responsive.

A representative service layout is:

```text
Linux
 |
 +-- exn-camera.service
 +-- exn-antenna.service
 +-- exn-platform.service      PI-MGMT / 0x104
 +-- system watchdog
 +-- journald / telemetry collection
 +-- persistent payload storage
```

### 4.1 Camera service

Responsibilities include:

- sensor acquisition;
- image metadata and product ownership;
- capture configuration;
- local buffering/storage;
- compression or host-side preparation;
- transfer to FPGA for processing;
- reception/association of processed products;
- direct handoff to the Antenna service when appropriate.

The Camera service remains the natural owner of captured payload products even when computation is delegated to FPGA.

### 4.2 Antenna service

Responsibilities include:

- own the Ground-facing RF/Wi-Fi/network link;
- receive Ground TCs and forward them to the OBC;
- receive OBC TMs and forward them to Ground;
- accept payload data directly from PI-CAM or FPGA and downlink it to Ground when commanded/policy permits;
- report link/session health and status.

The Antenna service is **not** the spacecraft command authority. The OBC remains the authority. Ground TCs pass `Ground -> Antenna -> OBC`; OBC TMs pass `OBC -> Antenna -> Ground`. For bulk data, the OBC may command PI-CAM or FPGA to send an object to the Antenna service, which then transmits it to Ground. This bypasses the OBC as a bulk-data router while preserving OBC control.

### 4.3 Platform management / PI-MGMT

The Linux platform-management layer should provide or aggregate:

- hardware/system watchdog integration;
- service restart/supervision state;
- CPU load and temperature;
- memory and storage state;
- process/service health;
- network/link state;
- restart/error counters;
- uptime and system-level events.

`systemd` may provide process supervision and watchdog integration. PI-MGMT turns those host capabilities into an OBC-addressable management endpoint for service restart, Pi reboot, platform HK/alarms and software activation.

## 5. Control plane and data plane

EXN deliberately distinguishes the control plane from the bulk data plane.

### 5.1 Control plane

The nominal flight-like command path is:

```text
Ground
  -> Antenna Service
  -> STM32 OBC
  -> selected payload endpoint
```

For example:

```text
Ground -> Antenna -> OBC -> Camera
Ground -> Antenna -> OBC -> FPGA
```

The existing HIL/ground implementation may still connect GS directly to the MCU. That is a deployment profile, not a change in control ownership.

### 5.2 Data plane

Payload data may bypass the MCU as a **bulk-data router** while the OBC still controls what is downlinked.

Typical commanded flow:

```text
OBC -> Camera: downlink object X
Camera -> Antenna -> Ground

OBC -> FPGA: downlink result Y
FPGA -> Antenna -> Ground
```

Other valid data paths include:

```text
Camera -> FPGA -> Camera -> Antenna -> Ground
FPGA -> OBC                         decision metadata / small result
```

The architectural rule is therefore **OBC-controlled, source-to-Antenna data delivery**. The producer sends the bulk bytes directly to the Antenna service, and the Antenna transmits them to Ground. The OBC may still receive metadata, classification results, status, or completion events without forwarding the payload itself.

## 6. FPGA processing and result routing

The FPGA is a **processing service**, not a fixed stage in a hard-coded `Camera -> FPGA -> Camera` pipeline.

A processing request conceptually contains:

```text
job identifier
processing/pipeline identifier
input reference or input stream
processing parameters
result destination(s)
delivery policy
```

The exact wire encoding of the routing fields is intentionally left to the Service 210/interface revision that implements this architecture. The architectural requirement is already fixed: **result routing must be controllable per processing request or by an explicitly configured mission policy.**

### 6.1 Typical destinations

A processing result may be routed to one or more of:

- **OBC**, for autonomous decisions, event generation, or subsequent commanding;
- **Camera service**, to associate the result with the source image, store it, or generate another payload product;
- **Antenna service**, which performs the actual Ground downlink;
- **requesting endpoint**, for request/response-style processing;
- **multiple endpoints**, when both control and payload consumers need the result.

### 6.2 Example: autonomous classification

```mermaid
sequenceDiagram
  participant CAM as Camera
  participant FPGA as FPGA Processing
  participant OBC as STM32 OBC

  CAM->>FPGA: image + classification request
  FPGA-->>OBC: classification result
  OBC->>OBC: mission decision
  OBC-->>CAM: discard / retain / recapture / process / downlink policy
```

A classification can therefore influence spacecraft behavior without requiring a full processed image to return through the MCU.

### 6.3 Example: processed payload product

```mermaid
sequenceDiagram
  participant CAM as Camera
  participant FPGA as FPGA Processing
  participant ANT as Antenna/Comms
  participant GS as Ground

  CAM->>FPGA: image + processing request
  FPGA-->>CAM: processed product
  CAM->>ANT: payload product
  ANT-->>GS: downlink
```

### 6.4 Example: direct result downlink

```mermaid
sequenceDiagram
  participant CAM as Camera
  participant FPGA as FPGA Processing
  participant ANT as Antenna/Comms
  participant GS as Ground

  CAM->>FPGA: image + request, destination=communications
  FPGA-->>ANT: result
  ANT-->>GS: result downlink
```

Direct downlink is conditional on Antenna link availability and mission policy. The FPGA routes the result to the Antenna service; it does not own a Ground link itself. Loss of the Ground link must not make FPGA execution semantics depend on the ground station being present.

## 7. SpWKit in EXN

SpWKit may be used as a logical SpaceWire-style interconnect and simulation layer between EXN endpoints.

A useful deployment can expose several independent logical links over one real Ethernet carrier, for example:

```text
Physical Ethernet
      |
      +-- Camera <-> OBC
      +-- Antenna <-> OBC
      +-- Camera <-> FPGA
      +-- Camera <-> Antenna
      +-- FPGA <-> Antenna
```

This lets EXN model multiple spacecraft communication relationships without requiring one physical cable per logical link.

SpWKit remains a reusable independent project. EXN should consume only the public transport/link behavior that it needs. **Commercial/private FPGA SpaceWire implementation repositories are outside EXN scope.** The EXN FPGA node is defined by its payload-processing role, not by any separate SpaceWire FPGA product.

On STM32, any future SpWKit-over-Ethernet binding should preserve the library boundaries:

```text
EXN application
      |
   SpWKit
      |
transport-provider boundary
      |
     DAS
      |
STM32 Ethernet hardware
```

SpWKit must not require DAS directly, and DAS must not know about SpWKit. HardRT provides scheduling, synchronization, timing, IRQ-to-task signalling, and timeout/retry execution around the application and transport integration.

## 8. PUS service architecture

EXN treats the PUS service catalogue as an application-interface layer above CCSDS packet construction and below endpoint mission logic.

The normative EXN wire profile is **PUS Revision C** as defined in [`ICD.md`](ICD.md) and [`services.md`](services.md). Any remaining PUS-A code in component repositories is migration debt and shall be replaced rather than extended.

The target service split is:

- standard Service 1 for request verification;
- standard Service 3 for housekeeping, with EXN System HK using mission-specific subtypes;
- standard Service 5 for event reporting;
- standard Service 9 for time management, with a mission-specific time-set request if required;
- standard Service 20 for parameter management;
- mission-specific Service 200 for Camera control;
- mission-specific Service 210 for payload/AI processing;
- mission-specific Service 220 for payload-object transfer;
- standard Service 23 reserved for its PUS file-management meaning.

This distinction is architectural, not cosmetic. A transport implementation such as SpWKit carries complete packet bytes and does not redefine PUS services; likewise, FPGA image-processing logic does not acquire communications-service ownership merely because it can physically emit results.

The service profile shall be frozen before the placeholder MCU and FPGA repositories become implementation baselines, so those components do not encode the current legacy collisions into new APIs.

## 9. Reusable software stack

```text
EXN mission applications / services
      |
      +-- CCSDSPack      CCSDS/PUS packet model
      +-- SpWKit         optional logical SpaceWire/interconnect layer
      +-- HardRT         STM32 RTOS/synchronization/runtime
      +-- DAS            MCU/device/board abstraction
```

The dependencies are composed by the EXN component application. They are not one vertical framework that owns the layer below it.

For the STM32 control node in particular:

```text
                 exn-mcu-rtos
                      |
        +-------------+-------------+
        |             |             |
    CCSDSPack      HardRT         SpWKit
                                    |
                             transport provider
                                    |
                                   DAS
                                    |
                                STM32H755
```

## 10. Interface ownership

The documentation boundary is:

- **`docs/architecture.md`**: logical nodes, responsibilities, control/data paths, routing policy, and reusable-stack composition;
- **`docs/component-services.md`**: complete endpoint functions, management, heartbeat, time, acquisition, processing, transfer and update behavior;
- **`docs/services.md`**: PUS service tailoring, standard/mission-specific allocation, and migration policy;
- **`docs/ICD.md`**: exact CCSDS/PUS wire contract and routing fields;
- **`docs/icd/`**: endpoint-specific service behavior and payload layouts;
- **component repositories**: implementation details;
- **CCSDSPack / HardRT / DAS / SpWKit repositories**: reusable library APIs and implementation-specific qualification.

This separation allows EXN to change deployment topology without rewriting mission semantics and allows reusable libraries to evolve without becoming EXN-specific.