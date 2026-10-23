# EXN Interface Control Document Index

This directory contains the **component-to-component contracts of the complete EXN ICD**. A component boundary exists between distinct APIDs even when the components occupy the same Raspberry Pi.

The normative specification hierarchy is:

1. [`../ICD.md`](../ICD.md) — common CCSDS/PUS wire profile, APIDs, routing, CRC and shared packet rules.
2. [`../services.md`](../services.md) — normative PUS service/subservice catalogue and mission tailoring.
3. [`../component-services.md`](../component-services.md) — common service semantics, shared states, timing, HK, monitoring, transfer and update models.
4. [`component-links.md`](component-links.md) — **normative inter-component interaction map**, including PI-CAM ↔ PI-COMMS on the same Raspberry Pi and its Service 220 handoff semantics.
5. [`inter-node-links.md`](inter-node-links.md) — reference switchless Pi UART / KV260 USB CDC-ACM control, direct Pi↔KV260 Ethernet, Wi-Fi Ground link and failure behavior.
6. [`hardware-sensors.md`](hardware-sensors.md) — normative sensor/health profiles for selected STM32H755, Raspberry Pi 5 and AMD Kria K26/KV260 hardware.
7. Endpoint ICDs in this directory — endpoint-specific commands, fields, IDs, capabilities, states and payloads.

## Endpoint ICDs

| Endpoint | APID | Normative ICD | Implementation repository |
|---|---:|---|---|
| Ground / HIL | `0x0F0` | [`gs.md`](gs.md) | `ExoSpaceLabs/exn-gs` |
| STM32 OBC | `0x100` | [`mcu-rtos.md`](mcu-rtos.md) | `ExoSpaceLabs/exn-mcu-rtos` |
| PI-CAM | `0x101` | [`pi-cam.md`](pi-cam.md) | `ExoSpaceLabs/exn-pi-cam` |
| FPGA processing | `0x102` | [`fpga-ai.md`](fpga-ai.md) | `ExoSpaceLabs/exn-fpga-ai` |
| PI-COMMS / Antenna | `0x103` | [`linux-payload.md`](linux-payload.md) | `ExoSpaceLabs/exn-pi-cam` |
| PI-MGMT | `0x104` | [`linux-payload.md`](linux-payload.md) | `ExoSpaceLabs/exn-pi-cam` |

## Ownership rule: inter-device vs component-internal

**EXN is the system-level component-to-component ICD, not a collection of component internal design documents.** The normative contract applies to distinct logical components **irrespective of physical co-location**: what they request, send, receive, acknowledge, interpret or observe, including endpoint identities, packet/PUS fields, shared services, routing/authorization, HK/time/FDIR, object transfers and updates.

| Component repository | Examples of documentation owned locally, not here |
|---|---|
| [exn-mcu-rtos](https://github.com/ExoSpaceLabs/exn-mcu-rtos/blob/main/docs/CM4_WORKER.md) | CM7↔CM4 job/IPC protocol, core boot, HSEM/cache, HardRT task plan and hardware driver integration |
| [exn-pi-cam](https://github.com/ExoSpaceLabs/exn-pi-cam/blob/main/docs/PI_NODE_ARCHITECTURE.md) | `systemd`/process topology, **local IPC transport/serialization implementation** of centrally defined PI-CAM ↔ PI-COMMS behavior, storage and restart mechanisms |
| [exn-fpga-ai](https://github.com/ExoSpaceLabs/exn-fpga-ai) | PS/PL split, accelerator datapaths, DMA and buffering, bitstream loading/rollback mechanism |
| [exn-gs](https://github.com/ExoSpaceLabs/exn-gs/blob/main/docs/architecture.md) | GS daemon/UI/CLI responsibility split and Ground-local IPC |

When the *externally observable* packet/command/status/result of those implementations is affected, the related wire and behavioral contract **must be specified in EXN first**, then implemented in components. Component docs may link to EXN or describe implementation conformance; they shall not independently allocate or redefine APIDs, PUS IDs, message fields, parameter IDs, event IDs, HK layouts, timing, routing or software-update wire protocols.

Do not infer a new component/APID from internal cores, tasks or router daemons. **CM7 and CM4 are one OBC component/APID.** **PI-CAM and PI-COMMS are separate EXN components** even on one Pi (PI-MGMT is a separately addressable support component). Their **cross-component behavior belongs here** in [component-links.md](component-links.md); their local Unix-socket/shared-memory framing and process implementation belong in the Pi repository.

## Interface artifacts

Files under `../../interfaces/` are machine-readable/shared artifacts that must agree with this ICD. They are not an independent source of mission semantics.

The long-term goal is to generate or cross-validate those artifacts from a single machine-readable representation of the normative ICD.
