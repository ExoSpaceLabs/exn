<div style="text-align: center;">
    <img alt="EXN logo" src="docs/images/exn_logo_transparent.png" width="256" />
</div>

# EXN — Modular Satellite Avionics Demonstration Platform

**EXN** is an ExoSpaceLabs system-integration project for demonstrating a modular Earth-observation avionics and payload architecture across an **STM32 control computer**, an **Embedded Linux payload platform**, an **FPGA processing node**, and a **ground/HIL environment**.

The platform exercises realistic spacecraft engineering boundaries: CCSDS/ECSS packet interfaces, embedded supervision, image acquisition, hardware-accelerated image/AI processing, communications services, telemetry/telecommand workflows, flexible payload-data routing, and end-to-end integration testing.

The detailed logical architecture is defined in [docs/architecture.md](docs/architecture.md). The wire-level packet contract remains [docs/ICD.md](docs/ICD.md), while [docs/services.md](docs/services.md) defines the PUS service-tailoring profile and migration direction.

> [!IMPORTANT]
> EXN is the **system architecture and interface authority**, not a monolithic firmware repository. Logical endpoint responsibilities must remain independent from the physical computer that happens to host them in the current demonstrator.

## System Scope

EXN separates responsibilities across independently testable logical components:

- **STM32 Control Node / OBC:** system supervision, command/telemetry handling, health and fault management, time distribution, configuration, and control-plane routing/orchestration.
- **Linux Payload Node:** Raspberry Pi-based Embedded Linux platform that may host multiple independent payload services.
  - **Camera Service:** image acquisition, payload-product ownership, storage/preparation, compression, and image transfer.
  - **Antenna / Communications Service:** ground-link ingress/egress, TC delivery toward the OBC, payload downlink, and communications-link health.
  - **Platform Management:** Linux watchdog/supervision, service health, logging, storage and system housekeeping.
- **FPGA Processing Node:** hardware-accelerated image preprocessing, inference, postprocessing, and other payload computation. The FPGA is a processing node in EXN, not a communications product.
- **Ground/HIL:** operator tooling, command/telemetry inspection, subsystem simulation, fault injection, and integration validation.

Camera and Antenna/Communications may initially execute on the **same Raspberry Pi 5** while remaining separate logical services. A later deployment may place them on different computers without changing their mission responsibilities.

## Routing Model

EXN distinguishes the **control plane** from the **bulk data plane**.

The nominal flight-like command path is:

```text
Ground -> Antenna/Communications -> STM32 OBC -> selected payload endpoint
```

The current HIL environment may still connect the Ground Station directly to the MCU. That is a deployment profile, not a change of command authority.

Bulk payload data does not need to pass through the MCU. Valid flows include:

```text
Camera -> Antenna -> Ground
Camera -> FPGA -> Camera -> Antenna -> Ground
FPGA -> Antenna -> Ground
FPGA -> OBC
```

The **FPGA result destination is controllable**. A processing result may be routed to the OBC for autonomous decision-making, back to the Camera service for product ownership/storage, to the Antenna service for immediate downlink, to the requesting endpoint, or to multiple consumers when policy requires it.

The exact wire encoding of per-job processing-result routing will be introduced through the corresponding Service 210/interface revision. The architectural requirement is already fixed: the FPGA must not impose one hard-coded result path.

## Repository Role

This repository contains the cross-component architecture, ICDs, diagrams, and shared interface definitions rather than component implementations.

```text
exn/
├── interfaces/             # Shared packet/interface definitions
├── docs/                   # Architecture, ICDs, diagrams, and design notes
├── CHANGELOG.md
├── LICENSE
└── README.md
```

### Component repositories

- **[exn-gs](https://github.com/ExoSpaceLabs/exn-gs):** ground-control and hardware-in-the-loop environment.
- **exn-mcu-rtos:** STM32 control/flight-software component.
- **exn-pi-cam:** Raspberry Pi payload software; currently camera-focused and intended to evolve into the Linux payload-service implementation.
- **exn-fpga-ai:** FPGA image-processing and AI-acceleration component.

### Reusable platform repositories

- **[CCSDSPack](https://github.com/ExoSpaceLabs/CCSDSPack):** CCSDS Space Packet / ECSS PUS packet library. Current released EXN baseline: **v2.0.0**.
- **[HardRT](https://github.com/ExoSpaceLabs/hardrt):** RTOS/runtime for STM32 scheduling, synchronization, timers, signalling and execution control. Current released baseline: **0.5.1**.
- **[DAS](https://github.com/Inczert/device-abstraction-stack):** MCU/device/board abstraction layer for STM32H755 / NUCLEO-H755ZI-Q. Current released baseline: **v0.1.0**.
- **[SpWKit](https://github.com/ExoSpaceLabs/spwkit):** optional SpaceWire-style transport/simulation toolkit for logical EXN links. Current released baseline: **v0.7.0**.

Private/commercial FPGA SpaceWire implementation repositories are **outside EXN scope**. The EXN FPGA node exists for payload image/AI processing. Reusing FPGA hardware for separate SpaceWire development is a different product/integration concern.

## Software and Platform Composition

The reusable libraries are composed by each EXN application; they do not form one ownership chain.

For the STM32 control node, the intended relationship is:

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

HardRT and DAS are peers from the application point of view: HardRT owns runtime/scheduling primitives, while DAS owns hardware/device access. SpWKit must not depend directly on DAS; an embedded transport-provider implementation may use DAS below the SpWKit boundary.

## SpWKit and Logical Links

If adopted by an EXN deployment, SpWKit may model multiple independent logical links over a shared physical carrier such as Ethernet.

For example, one Ethernet network may carry logical relationships such as:

```text
Camera <-> OBC
Antenna <-> OBC
Camera <-> FPGA
Camera <-> Antenna
FPGA <-> Antenna
```

This allows the demonstrator to simulate more than one spacecraft link without requiring one physical cable for every logical endpoint pair.

Mission packet/service semantics remain above that transport choice.

## System Graph

```mermaid
graph LR
  subgraph "Ground Segment"
    GS[Ground Station / HIL]
  end

  subgraph "Linux Payload Node - Raspberry Pi"
    CAM[Camera Service]
    ANT[Antenna / Communications Service]
    PM[Platform Management<br/>watchdog / health / logging]
  end

  subgraph "STM32 Control Node"
    OBC[OBC / MCU-RTOS]
  end

  subgraph "FPGA Processing Node"
    FPGA[Image / AI Processing]
  end

  GS <--> ANT
  ANT <--> OBC
  OBC <--> CAM
  OBC <--> FPGA
  CAM <--> FPGA
  CAM <--> ANT
  FPGA <--> ANT
  PM --- CAM
  PM --- ANT
```

The graph shows **allowed logical paths**, not mandatory routing for every operation. The OBC remains the supervisory control point while payload-data routing may bypass it when appropriate.

Additional sequence diagrams are maintained in [docs/diagrams.md](docs/diagrams.md).

## Interfaces and ICDs

- **System architecture:** [docs/architecture.md](docs/architecture.md)
- **PUS service profile:** [docs/services.md](docs/services.md)
- **Master wire-level ICD:** [docs/ICD.md](docs/ICD.md)
- **Ground/HIL:** [docs/icd/gs.md](docs/icd/gs.md)
- **MCU control node:** [docs/icd/mcu-rtos.md](docs/icd/mcu-rtos.md)
- **Linux payload platform / communications service:** [docs/icd/linux-payload.md](docs/icd/linux-payload.md)
- **Pi camera service:** [docs/icd/pi-cam.md](docs/icd/pi-cam.md)
- **FPGA processing node:** [docs/icd/fpga-ai.md](docs/icd/fpga-ai.md)

### Packet/interface definitions

- CCSDSPack TeleCommands: [interfaces/ccsdspack/tc/](interfaces/ccsdspack/tc/)
- CCSDSPack TeleMetry: [interfaces/ccsdspack/tm/](interfaces/ccsdspack/tm/)
- JSON mirrors for tooling: [interfaces/json/](interfaces/json/)
- MCU header-only definitions: [interfaces/mcu-rtos/](interfaces/mcu-rtos/)

Wire-level changes must remain synchronized with these shared definitions. Architecture changes alone must not silently change packet layouts.

## Current implementation status

The architecture is ahead of the component implementations, deliberately:

| Area | Current state |
|---|---|
| System architecture / ICD | defined; service profile normalization in progress |
| CCSDSPack | released and qualified; v2.0.0 remains the current stable EXN baseline |
| EXN-GS / HIL | implemented on the transitional PUS-A wire profile |
| STM32 OBC | repository still requires implementation against HardRT + DAS |
| Linux Camera / Communications | camera repository is legacy/experimental; multi-service Linux platform remains to be implemented |
| FPGA processing | architecture defined; implementation repository remains a placeholder |
| HardRT | released 0.5.1 |
| DAS | released v0.1.0 for STM32H755 / NUCLEO-H755ZI-Q |
| SpWKit | released v0.7.0; optional EXN transport integration, not a mandatory architecture dependency |

The current PUS-A templates are a **transitional compatibility profile**, not the final service-tailoring baseline. Several legacy EXN service identifiers collide with standardized PUS-C meanings. New flight/payload implementation work shall follow [docs/services.md](docs/services.md) rather than extending those legacy assignments.

## Integration Direction

The current integration direction is:

1. freeze the EXN PUS-C service tailoring and request-verification policy defined in `docs/services.md`;
2. keep released CCSDSPack v2.0.0 as the reproducible dependency until a newer release is actually published and adopted;
3. migrate shared packet templates, MCU headers, EXN-GS and simulators together to the normalized service profile;
4. bring the MCU application up on **HardRT 0.5.1 + DAS v0.1.0** against the central EXN architecture/ICD;
5. evolve the Pi from a camera-only implementation into the supervised Linux payload-service platform;
6. keep the FPGA focused on image/AI processing with configurable result routing;
7. use direct payload-data paths where they reduce unnecessary MCU traffic;
8. adopt **SpWKit v0.7.x** only where its logical-link/transport model improves EXN integration, and validate that integration explicitly;
9. restore complete node-to-node HIL/system regression before freezing an EXN system release.

## License

Apache License 2.0. See [LICENSE](LICENSE) for the authoritative terms.
