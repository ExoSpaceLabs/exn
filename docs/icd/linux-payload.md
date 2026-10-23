# Raspberry Pi Payload Node ICD

This document defines the EXN **Raspberry Pi 5 Payload Node** boundary.

The Pi has exactly **two mission purposes**:

1. **PI-CAM** — Camera / payload acquisition.
2. **PI-COMMS** — Antenna / Ground-link service.

A third process, **PI-MGMT**, exists only as platform-management infrastructure. It is not a third payload function; it gives the OBC a reliable way to supervise, restart and update the two mission services.

Refer to:

- `../architecture.md` for system architecture;
- `../component-services.md` for common component behavior;
- `../services.md` for the normative PUS Revision C profile;
- `../ICD.md` for packet/routing rules;
- `pi-cam.md` for Camera-specific behavior;
- `hardware-sensors.md` for the normative Raspberry Pi 5 platform sensor IDs and engineering units;
- `inter-node-links.md` for the reference Pi 5 Wi-Fi/Ground gateway, OBC-facing UART packet router and direct KV260 Ethernet data link.

## 1. Logical endpoints

| Endpoint | APID | Source ID | Purpose |
|---|---:|---:|---|
| PI-CAM | `0x101` | `0x02` | Camera / payload acquisition |
| PI-COMMS | `0x103` | `0x04` | Antenna / Ground link |
| PI-MGMT | `0x104` | `0x05` | Pi platform supervision and software lifecycle |

PI-CAM and PI-COMMS are **two separate mission components** despite sharing Raspberry Pi hardware. PI-MGMT is a separately addressable **supporting EXN component**, not a third primary payload function. All interfaces between these components, including Camera ↔ Antenna object handoff, are governed by the [central component-to-component ICD](component-links.md).

## 2. Logical endpoint separation

PI-CAM (`0x101`), PI-COMMS (`0x103`) and PI-MGMT (`0x104`) are separate service identities even while co-located on a single Raspberry Pi. They must preserve the externally specified routing, HK, event, command and recovery behavior independently of how Linux services are implemented.

Process names, `systemd` configuration, watchdog implementation, local IPC and service restart mechanics belong in [exn-pi-cam](https://github.com/ExoSpaceLabs/exn-pi-cam/blob/main/docs/PI_NODE_ARCHITECTURE.md); they are **not** normative EXN ICD allocations.

## 3. Antenna endpoint, PI-COMMS

PI-COMMS is the **Antenna service**. Antenna and communications are the same EXN function.

Responsibilities:

- own the Ground-facing RF/Wi-Fi/network link;
- receive Ground TCs and forward them to the OBC;
- receive OBC TMs and forward them to Ground;
- receive payload objects from PI-CAM or FPGA and transmit them to Ground;
- maintain link/session state and forwarding counters;
- report Antenna health/events.

The Antenna is not command authority. The OBC decides what the spacecraft does and what payload data should be downlinked.

## 4. Antenna routing behavior

### 4.1 Ground uplink

The nominal uplink is **Ground → Antenna → OBC**. Ground control traffic is delivered to the OBC logical destination; the Antenna does not independently execute Camera or FPGA commands.

### 4.2 OBC telemetry downlink

The nominal downlink is **OBC → Antenna → Ground**. The Antenna forwards valid OBC reporting packets and preserves packet identity as defined by the deployed transport.

### 4.3 Direct payload downlink

For bulk data, the OBC instructs **PI-CAM** or **FPGA** to transfer an object to Ground; the producing endpoint delivers the bytes **via Antenna**. This path bypasses the OBC as a bulk router, not the Ground-link service. See the [data-plane diagram](../architecture.md#52-data-plane) and [direct-downlink sequences](../diagrams.md#5-obc-controlled-direct-fpga-result-downlink).

The Antenna shall accept Service 220 object transfers addressed to Ground/downlink policy and forward them to the active Ground session.

## 5. Antenna services

PI-COMMS provides/consumes:

- Service 1 Request Verification where the Antenna itself is commanded;
- Service 3 Housekeeping;
- Service 5 Event Reporting;
- Service 9 Time Management;
- Service 12 On-board Monitoring for link/forwarding health;
- Service 20 Parameter Management;
- Service 220 Payload Object Transfer / Ground forwarding.

A custom Antenna PUS service is not required in the first baseline.

### 5.1 Antenna parameters

At minimum:

| Parameter | Meaning |
|---|---|
| `link.enable` | enable/disable Ground link processing |
| `uplink.enable` | permit Ground uplink |
| `downlink.enable` | permit downlink |
| `payload_forwarding.enable` | permit direct payload-object forwarding |
| `rate_limit_bps` | optional software forwarding limit |

Technology-specific network/radio setup remains deployment configuration until a real operational requirement needs it remotely mutable.

### 5.2 Antenna HK

HEARTBEAT:

- service state;
- software version;
- Ground link/session state;
- time-sync state;
- uptime;
- last error;
- restart count.

HEALTH/LINKS:

- RX/TX packet counters;
- RX/TX byte counters;
- drop/error counters;
- current throughput;
- Ground session connected/disconnected state;
- forwarding queue depth;
- last receive/transmit time;
- technology-specific signal quality/RSSI only when the selected Ground-link implementation exposes it.

Representative events:

- Ground link established/lost;
- uplink rejected/disabled;
- downlink disabled;
- forwarding queue overflow;
- repeated transport error;
- Ground session restored.

## 6. PI-MGMT endpoint

PI-MGMT is implemented by `exn-platform.service`.

It exists because the OBC must be able to recover a Camera or Antenna service that is itself unresponsive.

PI-MGMT provides:

- Service 1 Request Verification;
- Service 3 platform Housekeeping;
- Service 5 platform/service events;
- Service 8 Function Management;
- Service 9 Time Management;
- Service 12 platform monitoring;
- Service 20 platform parameters;
- Service 220 receipt/storage of update packages;
- Service 230 Software Management.

## 7. PI-MGMT function management

Required Service 8 functions:

| Function ID | Action |
|---:|---|
| `0x0101` | restart `exn-camera.service` |
| `0x0102` | restart `exn-antenna.service` |
| `0x0103` | restart both mission services |
| `0x0104` | reboot Raspberry Pi |
| `0x0105` | orderly Raspberry Pi shutdown |
| `0x0106` | clear service restart/fault counters |

Restart completion requires the selected systemd service to return active **and** its EXN heartbeat to recover.

## 8. PI-MGMT Housekeeping

### HEARTBEAT

- platform supervisor state/version;
- uptime;
- mission time-sync state;
- Camera service active state;
- Antenna service active state;
- last platform error;
- reboot count.

### HEALTH

- CPU load;
- memory used/free;
- payload-store used/free;
- update-store used/free;
- service restart counters;
- watchdog state;
- systemd failed-unit count;
- network-interface states.

### SENSORS

For the Raspberry Pi 5 reference profile, PI-MGMT shall implement the physical sensor catalogue in [hardware-sensors.md](hardware-sensors.md), including at minimum:

- `0x1101 pi5.soc_temp` — SoC temperature;
- `0x1102 pi5.ext5v` — PMIC-measured external 5 V input;
- `0x1103 pi5.core_voltage` — optional PMIC core-voltage channel when exposed by the installed PMIC/firmware.

Throttling, undervoltage, CPU frequency, memory/storage state and reset reason are platform **HEALTH** fields, not physical SENSORS.

Camera-sensor-specific values remain Camera-owned unless they are only obtainable through the platform layer.

## 9. Platform monitoring

PI-MGMT Service 12 predeclared monitoring definitions include:

- Pi SoC temperature warning/critical using sensor `0x1101`;
- undervoltage and thermal-throttling HEALTH conditions;
- payload storage low/critical;
- update storage low/critical;
- Camera service crash/restart storm;
- Antenna service crash/restart storm;
- watchdog failure;
- selected spacecraft-side network link loss.

Limit transitions produce Service 5 events consumed by the OBC.

Thresholds and restart-storm windows are Service 20 parameters.

## 10. Software inventory and update

PI-MGMT owns Software Management for:

- PI-CAM service;
- Antenna service;
- PI-MGMT itself;
- Pi-level supporting packages/configuration explicitly included in an EXN update bundle.

The software-update sequence is defined in [OBC-controlled Pi software update](../diagrams.md#12-software-update-of-pi-camera-service). In brief, Service 220 transfers the update package, Service 230 stages and activates it, PI-MGMT restarts only affected services and verifies post-update heartbeat/HK, and Service 1 reports the result.

Camera and Antenna service updates shall not require a full Pi reboot unless the package explicitly changes shared platform dependencies.

A failed post-activation health check triggers rollback where a previous version is available.

## 11. Time

PI-CAM, PI-COMMS and PI-MGMT consume OBC `TC[9,128] Set Mission Time`.

PI-MGMT may also synchronize the Linux wall clock as an implementation convenience, but EXN mission timestamps shall use the synchronized mission-time abstraction rather than assuming Linux wall clock is authoritative.

## 12. Camera ↔ Antenna logical data path

**PI-CAM ↔ PI-COMMS is an EXN component-to-component interface**, not merely a private local data path. The [component-to-component ICD](component-links.md#3-pi-cam--pi-comms-camera-to-antenna-contract) and [Service 220](../component-services.md#12-payload-object-transfer-service-220) centrally define authorization, producer/gateway identities, object/transfer metadata, transfer status/completion, error/backpressure semantics and Ground downlink behavior. These contracts apply **unchanged** when the two components are placed on different hosts.

The Pi repository may optimize the **implementation** using file/object references, Unix sockets or shared memory, provided it implements the same normative logical transfer and externally visible result. Local IPC framing, storage and data-copy strategy belong in [exn-pi-cam](https://github.com/ExoSpaceLabs/exn-pi-cam/blob/main/docs/PLATFORM.md), but the two components may not privately redefine mission transfer semantics.

## 13. SpWKit deployment option

Where SpWKit is used, Camera, Antenna, PI-MGMT, OBC and FPGA may expose separate logical relationships over shared Ethernet.

SpWKit remains below mission semantics. EXN does not depend on any private/commercial FPGA SpaceWire implementation.
