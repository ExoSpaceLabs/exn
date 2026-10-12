# Linux Payload Platform ICD

This document defines the EXN Embedded Linux payload platform boundary. The initial deployment target is a Raspberry Pi 5, while logical endpoint identity remains separate from physical host identity.

Refer to `../architecture.md` for system architecture, `../ICD.md` for the current wire contract, and `../services.md` for the target PUS service profile.

## 1. Logical services

The Raspberry Pi may host multiple independent EXN services:

```text
Raspberry Pi 5
 |
 +-- Camera Payload Service        APID 0x101 / Source ID 0x02
 +-- Antenna/Communications        APID 0x103 / Source ID 0x04
 +-- Platform Management           local host supervision; no independent APID
```

Camera and Antenna/Communications remain separate logical endpoints even when co-located. Either service may later move to another Linux computer without changing its mission responsibility.

The Camera service contract remains defined in `pi-cam.md`. Platform Management is currently an implementation support service rather than a third mission packet endpoint; logical endpoints obtain host/watchdog state from it and include relevant health in their own housekeeping.

## 2. Linux service model

A representative deployment is:

```text
exn-camera.service
exn-antenna.service
exn-platform.service
```

The implementation may use `systemd` for process supervision, dependency ordering, restart policy, watchdog integration, and resource limits. Mission-facing health is reported through EXN Housekeeping/Event services rather than Linux-local logs alone.

## 3. Antenna / Communications endpoint

Responsibilities:

- ground-link ingress and egress;
- delivery of ground telecommands toward the STM32 OBC;
- downlink of telemetry and payload products;
- communications-link health reporting;
- direct payload-data delivery when routing policy permits it.

The communications endpoint is not the spacecraft command authority. The STM32 OBC remains responsible for command supervision and mission decisions.

Endpoint identity:

| Item | Value |
|---|---|
| Logical name | `PI-COMMS` / Antenna-Communications |
| APID | `0x103` |
| Transitional PUS-A TC Source ID | `0x04` |

The logical identity remains valid if the service later moves away from the Raspberry Pi.

## 4. Control path

The nominal flight-like path is:

```text
Ground -> Antenna/Communications -> STM32 OBC -> selected endpoint
```

A GS/HIL deployment may bypass the communications endpoint and connect directly to the OBC. Both deployment profiles use the same mission packet semantics.

## 5. Payload-data path

The communications endpoint may accept payload data directly from Camera or FPGA when mission policy permits it.

Representative routes include:

```text
Camera -> Antenna -> Ground
FPGA -> Antenna -> Ground
Camera -> FPGA -> Antenna -> Ground
```

Direct payload downlink does not require the MCU to carry bulk data. The OBC may still receive metadata, status, classification results, events, or completion reports as required.

## 6. PUS service support

The PI-COMMS target service set is deliberately small:

- Service 1 request verification;
- Service 3 housekeeping;
- Service 5 event reporting;
- Service 9 time reporting/synchronization where applicable;
- Service 20 parameter management;
- Service 220 payload-object transfer/forwarding where packetized mission objects are carried through the communications endpoint.

A dedicated communications-control service is **not allocated yet**. It should only be introduced if concrete radio/link operations cannot be represented coherently through housekeeping, events and parameters. This avoids creating a custom service merely because the process happens to be called `exn-antenna`.

The current executable HIL profile still uses legacy PUS-A service identifiers described in `../ICD.md`; new communications implementation work targets `../services.md`.

## 7. Housekeeping

Communications HK should include the master common fields plus communications-specific information such as link state, session state, TX/RX counters, dropped/error counters, current mode, last error, restart count, and timestamp.

The Linux platform-management service should also track host-level metrics such as CPU temperature/load, memory, storage, watchdog state, and service health. Exact binary extensions will be frozen with the corresponding implementation/interface revision.

## 8. Local service communication

When Camera and Antenna execute on the same Linux host, their local data path may use Unix-domain sockets, shared memory, file/object references, or another local IPC mechanism.

The mission-level Camera-to-Antenna interface must not require co-location. A later deployment may map the same logical relationship onto SpWKit or another inter-node transport.

## 9. SpWKit deployment option

Where SpWKit is used, Camera, Antenna, MCU and FPGA may expose independent logical links over one shared Ethernet carrier. SpWKit remains below mission semantics. Private/commercial FPGA SpaceWire implementation repositories are outside EXN scope.

## 10. Supervision boundary

`exn-platform.service` owns Linux-host supervision concerns such as watchdog feeding, restart accounting, storage health and process state. It does not become spacecraft command authority and does not need its own APID unless a future requirement establishes it as an independently addressable application process. Camera and Communications remain restartable logical services with their own mission identities.
