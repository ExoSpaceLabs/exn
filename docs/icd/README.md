# EXN Interface Control Document Index

This directory contains the **endpoint-specific parts of the complete EXN ICD**.

The normative specification hierarchy is:

1. [`../ICD.md`](../ICD.md) — common CCSDS/PUS wire profile, APIDs, routing, CRC and shared packet rules.
2. [`../services.md`](../services.md) — normative PUS service/subservice catalogue and mission tailoring.
3. [`../component-services.md`](../component-services.md) — common service semantics, shared states, timing, HK, monitoring, transfer and update models.
4. [`inter-node-links.md`](inter-node-links.md) — reference switchless Pi UART / KV260 USB CDC-ACM control, direct Pi↔KV260 Ethernet, Wi-Fi Ground link and failure behavior.
5. [`hardware-sensors.md`](hardware-sensors.md) — normative sensor/health profiles for selected STM32H755, Raspberry Pi 5 and AMD Kria K26/KV260 hardware.
6. Endpoint ICDs in this directory — endpoint-specific commands, fields, IDs, capabilities, states and payloads.

## Endpoint ICDs

| Endpoint | APID | Normative ICD | Implementation repository |
|---|---:|---|---|
| Ground / HIL | `0x0F0` | [`gs.md`](gs.md) | `ExoSpaceLabs/exn-gs` |
| STM32 OBC | `0x100` | [`mcu-rtos.md`](mcu-rtos.md) | `ExoSpaceLabs/exn-mcu-rtos` |
| PI-CAM | `0x101` | [`pi-cam.md`](pi-cam.md) | `ExoSpaceLabs/exn-pi-cam` |
| FPGA processing | `0x102` | [`fpga-ai.md`](fpga-ai.md) | `ExoSpaceLabs/exn-fpga-ai` |
| PI-COMMS / Antenna | `0x103` | [`linux-payload.md`](linux-payload.md) | `ExoSpaceLabs/exn-pi-cam` |
| PI-MGMT | `0x104` | [`linux-payload.md`](linux-payload.md) | `ExoSpaceLabs/exn-pi-cam` |

## Ownership rule

The `ExoSpaceLabs/exn` repository defines the mission-visible contract.

Component repositories:

- implement these interfaces;
- may document internal architecture, source layout, tasks/threads/processes, drivers, build configuration, hardware realization and test strategy;
- may report implementation support/status against the ICD;
- **shall not independently allocate or redefine** APIDs, PUS services/subservices, message fields, parameter IDs, control IDs, event IDs, HK layouts, routing semantics, timing semantics or software-update protocol fields.

If implementation work discovers a missing contract element, the change is made in `ExoSpaceLabs/exn` first and then implemented by the affected repositories.

## Interface artifacts

Files under `../../interfaces/` are machine-readable/shared artifacts that must agree with this ICD. They are not an independent source of mission semantics.

The long-term goal is to generate or cross-validate those artifacts from a single machine-readable representation of the normative ICD.
