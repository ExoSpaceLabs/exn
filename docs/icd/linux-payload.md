# Raspberry Pi Payload Node ICD

This document defines the EXN **Raspberry Pi 5 Payload Node** boundary. In the initial deployment the Pi has two required mission purposes: **Camera** and **Antenna**. Logical endpoint identity remains separate from physical host identity so the two functions are independently addressable even while co-located.

Refer to `../architecture.md` for system architecture, `../ICD.md` for the current wire contract, and `../services.md` for the target PUS service profile.

## 1. Dual mission role

The Raspberry Pi 5 is required to host two primary EXN mission services in the initial architecture:

```text
Raspberry Pi 5
 |
 +-- Camera Payload Service        APID 0x101 / Source ID 0x02
 +-- Antenna Service               APID 0x103 / Source ID 0x04
 +-- Platform Management           local host supervision; no independent APID
```

The Pi therefore serves **both** as the camera payload computer and as the spacecraft Antenna/Ground-link endpoint. Camera and Antenna remain separate logical endpoints despite sharing the same processor, OS, storage and network hardware. Either service may later move to another Linux computer without changing its mission responsibility.

The Camera service contract remains defined in `pi-cam.md`. Platform Management is currently an implementation support service rather than a third mission packet endpoint; logical endpoints obtain host/watchdog state from it and include relevant health in their own housekeeping.

## 2. Linux service model

A representative deployment is:

```text
exn-camera.service
exn-antenna.service
exn-platform.service
```

The implementation may use `systemd` for process supervision, dependency ordering, restart policy, watchdog integration, and resource limits. Mission-facing health is reported through EXN Housekeeping/Event services rather than Linux-local logs alone.

## 3. Antenna endpoint

Responsibilities:

- act as the spacecraft Ground-facing Antenna endpoint for the demonstrator;
- maintain the deployed RF/Wi-Fi/network communications path to Ground;
- communicate with the STM32 OBC and deliver Ground telecommands toward it;
- carry OBC telemetry back toward Ground;
- downlink Camera and FPGA payload products directly without routing the bulk bytes through the OBC;
- report communications-link/session health, counters and faults.

The Antenna endpoint is not the spacecraft command authority. The STM32 OBC remains responsible for command supervision and mission decisions. PI-COMMS is simply the logical/APID name of this Antenna service, not a separate communications subsystem.

Endpoint identity:

| Item | Value |
|---|---|
| Logical name | `PI-COMMS` (Antenna) |
| APID | `0x103` |
| Transitional PUS-A TC Source ID | `0x04` |

The logical identity remains valid if the service later moves away from the Raspberry Pi.

## 4. Control path

The nominal flight-like path is:

```text
Ground -> Antenna -> STM32 OBC -> selected endpoint
```

A GS/HIL deployment may bypass the Antenna endpoint and connect directly to the OBC. Both deployment profiles use the same mission packet semantics.

## 5. Payload-data path

The Antenna endpoint accepts payload data directly from Camera or FPGA when the OBC or mission policy requests a Ground downlink.

Representative routes include:

```text
Camera -> Antenna -> Ground
FPGA -> Antenna -> Ground
Camera -> FPGA -> Antenna -> Ground
```

Here, `direct` means the payload producer sends bulk data straight to the Antenna service rather than through the OBC. The Antenna service is still the physical/logical path to Ground. The OBC may still receive metadata, status, classification results, events, or completion reports.

## 6. PUS service support

The PI-COMMS target service set is deliberately small:

- Service 1 request verification;
- Service 3 housekeeping;
- Service 5 event reporting;
- Service 9 time reporting/synchronization where applicable;
- Service 20 parameter management;
- Service 220 payload-object transfer/forwarding where packetized mission objects are carried through the Antenna endpoint.

A dedicated custom Antenna-control PUS service is **not allocated yet**. It should only be introduced if concrete radio/link operations cannot be represented coherently through housekeeping, events and parameters.

The current executable HIL profile still uses legacy PUS-A service identifiers described in `../ICD.md`; new Antenna implementation work targets `../services.md`.

## 7. Housekeeping

Antenna HK should include the master common fields plus link-specific information such as link state, session state, TX/RX counters, dropped/error counters, current mode, last error, restart count, and timestamp.

The Linux platform-management service should also track host-level metrics such as CPU temperature/load, memory, storage, watchdog state, and service health. Exact binary extensions will be frozen with the corresponding implementation/interface revision.

## 8. Local service communication

Because Camera and Antenna execute on the same Raspberry Pi 5 in the initial deployment, their local bulk-data path should exploit co-location where useful, using Unix-domain sockets, shared memory, file/object references, or another local IPC mechanism.

The mission-level Camera-to-Antenna interface must not require co-location. A later deployment may map the same logical relationship onto SpWKit or another inter-node transport.

## 9. SpWKit deployment option

Where SpWKit is used, Camera, Antenna, MCU and FPGA may expose independent logical links over one shared Ethernet carrier. SpWKit remains below mission semantics. Private/commercial FPGA SpaceWire implementation repositories are outside EXN scope.

## 10. Supervision boundary

`exn-platform.service` owns Linux-host supervision concerns such as watchdog feeding, restart accounting, storage health and process state. It does not become spacecraft command authority and does not need its own APID unless a future requirement establishes it as an independently addressable application process. Camera and Antenna remain restartable logical services with their own mission identities.
