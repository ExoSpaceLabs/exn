<div style="text-align: center;">
    <img alt="EXN logo" src="docs/images/exn_logo_transparent.png" width="256" />
</div>

# EXN — Modular Satellite Avionics Demonstration Platform

**EXN** is an ExoSpaceLabs system-integration project for demonstrating a modular Earth-observation avionics and payload architecture across an **STM32 control computer**, an **Embedded Linux payload platform**, an **FPGA processing node**, and a **ground/HIL environment**.

The platform exercises realistic spacecraft engineering boundaries: CCSDS/ECSS packet interfaces, embedded supervision, image acquisition, hardware-accelerated image/AI processing, Ground communications, telemetry/telecommand workflows, flexible payload-data routing, and end-to-end integration testing.

The detailed logical architecture is defined in [docs/architecture.md](docs/architecture.md). The wire-level packet contract remains [docs/ICD.md](docs/ICD.md), while [docs/services.md](docs/services.md) defines the PUS service-tailoring profile and migration direction.

> [!IMPORTANT]
> EXN is the **system architecture and interface authority**, not a monolithic firmware repository. Logical endpoint responsibilities must remain independent from the physical computer that happens to host them in the current demonstrator.

## System Scope

EXN separates responsibilities across independently testable logical components:

- **STM32 Control Node / OBC:** system supervision, command/telemetry handling, health and fault management, time distribution, configuration, and control-plane routing/orchestration.
- **Raspberry Pi 5 Payload Node:** one physical Embedded Linux computer with **two first-class mission roles**.
  - **Camera role / PI-CAM:** image acquisition, payload-product ownership, storage/preparation, compression, and image transfer.
  - **Antenna role / PI-COMMS:** the Pi provides the spacecraft Ground link. The Antenna service exchanges TCs/TMs with the OBC and sends payload data from PI-CAM or FPGA to Ground without routing those bulk bytes through the OBC.
  - **Platform Management:** local Linux watchdog/supervision, service health, logging, storage and host housekeeping supporting both mission roles; this is not a third primary payload role.
- **FPGA Processing Node:** hardware-accelerated image preprocessing, inference, postprocessing, and other payload computation. The FPGA is a processing node in EXN, not a communications product.
- **Ground/HIL:** operator tooling, command/telemetry inspection, subsystem simulation, fault injection, and integration validation.

In the initial EXN deployment, **Camera and Antenna execute on the same Raspberry Pi 5**. They remain separate logical endpoints and services because they have different mission responsibilities, even though one physical Pi performs both roles. A later deployment may split them across computers without changing the interfaces.

## Routing Model

EXN distinguishes the **control plane** from the **bulk data plane**.

The nominal flight-like command path is:

```text
Ground -> Antenna -> STM32 OBC -> selected payload endpoint
```

The current HIL environment may still connect the Ground Station directly to the MCU. That is a deployment profile, not a change of command authority.

Bulk payload data does not need to pass through the MCU. **Direct to Ground** means the producing payload endpoint sends the data to the Pi Antenna service, which transmits it over the Ground link; it does not mean bypassing the spacecraft antenna. Valid flows include:

```text
Camera -> Antenna -> Ground
Camera -> FPGA -> Camera -> Antenna -> Ground
FPGA -> Antenna -> Ground
FPGA -> OBC
```

The **FPGA result destination is controllable**. A processing result may be routed to the OBC for autonomous decision-making, back to the Camera service for product ownership/storage, or to the Antenna service for immediate Ground downlink. Multiple destinations are allowed when policy requires them.

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
- **exn-pi-cam:** Raspberry Pi payload-node software. The repository name is historical; the target node has both PI-CAM and PI-COMMS responsibilities on the same Raspberry Pi 5.
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
    ANT[Antenna Service]
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
- **Raspberry Pi payload node / Antenna service:** [docs/icd/linux-payload.md](docs/icd/linux-payload.md)
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
| Raspberry Pi dual-role payload node | existing repository is legacy/experimental; target implementation must provide both PI-CAM and PI-COMMS on the same Pi 5 |
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
5. evolve the Pi repository from its camera-only legacy implementation into the **dual-role Raspberry Pi payload node**, implementing both PI-CAM and PI-COMMS under Linux supervision;
6. keep the FPGA focused on image/AI processing with configurable result routing;
7. use direct payload-data paths where they reduce unnecessary MCU traffic;
8. adopt **SpWKit v0.7.x** only where its logical-link/transport model improves EXN integration, and validate that integration explicitly;
9. restore complete node-to-node HIL/system regression before freezing an EXN system release.

## License

Apache License 2.0. See [LICENSE](LICENSE) for the authoritative terms.
