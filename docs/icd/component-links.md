# EXN component-to-component interface contract

**Status:** normative system-level contract. This document applies to **logical EXN components**, irrespective of whether they are on different circuit boards, within one Linux host, or reached through a router. It does not prescribe private software APIs, OS IPC framing, filesystem layout or scheduling.

**Sources of truth:** [master CCSDS/PUS-C ICD](../ICD.md), [service catalogue](../services.md), [component/service fields](../component-services.md), [endpoint ICD index](README.md) and [reference physical links](inter-node-links.md). These remain authoritative for **exact byte fields, IDs and transport allocations**; this document defines how their interfaces apply between components. No new APIDs, TC/TM subservices or payload fields are introduced here.

## 1. Component identities are not physical-device identities

| Component | APID | Physical reference | External responsibility |
|---|---|---|---|
| Ground / HIL | `0x0F0` | PC / Ground | Operator-originated commands and downlink reception |
| OBC | `0x100` | STM32H755 | Spacecraft command authority, orchestration, supervision, time |
| PI-CAM | `0x101` | Raspberry Pi 5 | Acquisition, payload-object production and ownership |
| FPGA processing | `0x102` | KV260 | Processing services, compact results and processed objects |
| PI-COMMS / Antenna | `0x103` | **Same Raspberry Pi 5 as PI-CAM** | Ground gateway, packet relay and downlink |
| PI-MGMT | `0x104` | Same Raspberry Pi 5 | Platform management and service recovery |

**PI-CAM and PI-COMMS are two distinct EXN components, not one combined component with private mission behavior.** PI-MGMT is a third *supporting logical endpoint*, even though it is not a third primary payload mission function. Co-location does not merge their APIDs, ownership, reporting or compatibility requirements. The Pi packet-router process has **no** mission APID, so it is infrastructure rather than another EXN component.

CM7 and CM4, in contrast, are **internal resources within one OBC component** and have no separate EXN APID or component-to-component ICD. Their private interface belongs in `exn-mcu-rtos`.

## 2. Normative logical component interfaces

| Source ↔ destination | Interface and ownership | Reference |
|---|---|---|
| Ground ↔ PI-COMMS | Ground uplink/downlink session; PI-COMMS is gateway, not spacecraft TC authority | [Ground](gs.md), [PI-COMMS](linux-payload.md), [links](inter-node-links.md) |
| PI-COMMS ↔ OBC | Forward complete Ground TCs and OBC TM; preserve packet identity; UART/COBS in P0 | [OBC](mcu-rtos.md), [links](inter-node-links.md) |
| OBC ↔ PI-CAM | OBC-authorized acquisition (Service 200), configuration, HK/time, events, Service 220 transfer control | [Camera](pi-cam.md), [services](../component-services.md) |
| **PI-CAM ↔ PI-COMMS** | **Service 220 producer-to-Antenna object handoff and downlink, with separate producer and gateway identities** | [Section 3](#3-pi-cam--pi-comms-camera-to-antenna-contract), [Service 220](../component-services.md#12-payload-object-transfer-service-220) |
| OBC ↔ FPGA | Service 210 jobs/results, Service 220 transfer control, time/HK/recovery | [FPGA](fpga-ai.md) |
| PI-CAM ↔ FPGA | Service 220 input/processed object transfer over the selected Ethernet data plane | [FPGA](fpga-ai.md), [links](inter-node-links.md) |
| FPGA ↔ PI-COMMS | OBC-authorized Service 220 processed-object downlink via Antenna | [FPGA](fpga-ai.md), [Antenna](linux-payload.md) |
| OBC ↔ PI-MGMT | Service 8 restart/recovery, Service 230 software update, HK/time/monitoring | [PI-MGMT](linux-payload.md) |
| PI-MGMT ↔ PI-CAM / PI-COMMS | Externally commanded lifecycle effects and status; implementation may use Linux supervision internally | [PI-MGMT](linux-payload.md), [Service 230](../component-services.md#14-software-and-service-updates-service-230) |

This table is a **logical component interface map**. Components remain independent even when one operating system supplies the physical transport or restart facilities. Every active inter-component exchange must conform to its referenced central EXN semantic contract.

## 3. PI-CAM ↔ PI-COMMS: Camera-to-Antenna contract

**Logical producer:** PI-CAM `0x101`. **Logical receiver/gateway:** PI-COMMS `0x103`. **Ground destination:** `0x0F0` when specified. The current reference physically co-locates the two Pi components but does not collapse this boundary.

### 3.1 Authorization and object selection

1. The **OBC** admits and authorizes the requested operation. For a Camera product downlink it directs the producer using existing `TC[220,1] Start Object Transfer` with the centrally defined `transfer_id`, `object_id`, destination, chunk size and flags. A requested Ground destination is resolved through PI-COMMS.
2. PI-CAM is the **producer and object owner**. It supplies the selected object and the existing `TM[220,10] Object Metadata`, `TM[220,11] Object Chunk`, `TM[220,12] Transfer Complete`, and status as applicable. Their layouts remain defined in the [Service 220 contract](../component-services.md#12-payload-object-transfer-service-220).
3. PI-COMMS accepts the authorized object transfer, enforces its forwarding enable/link capability and moves the corresponding bytes through the Ground-facing data path. It **shall not independently authorize** a new Camera acquisition, silently alter the destination, impersonate the producer or promote an unrequested local object to Ground.
4. Ground-facing complete PUS packets retain the **producer identity** and the centrally defined transfer/object IDs. A gateway does not rewrite PI-CAM TM as PI-COMMS TM. PI-COMMS's own HK/events and link-status reports use `0x103`.

The same contract applies if Camera and Antenna are later placed on different physical hosts.

### 3.2 Status, flow control and failure semantics

- **Local handoff is not completion.** A file descriptor, shared-memory reference, queue enqueue or accepted internal request does not by itself prove that bytes reached the requested destination. The producer and gateway must preserve the meaning of centrally defined Service 220 transfer status/completion and Service 1 verification; a local optimization cannot manufacture `COMPLETE`.
- The handoff must preserve `transfer_id`, `object_id`, producer, authorized destination, metadata (including expected size/checksum where specified) and transfer outcome across any local transport optimization. Receiver-side failure/backpressure must result in the **same externally visible failure/status semantics** as a non-co-located transfer, not silent loss.
- Only complete, well-framed packets are forwarded on the Ground stream. PI-COMMS must not allow Service 220 bulk traffic to starve higher-priority Service 1 verification or Service 5 events.
- An independently restarting Camera or Antenna retains its own logical identity. Existing transactions must be reconciled, failed or retried according to the centrally defined Service 220/Service 1 state and authorization rules; **never infer delivery solely from surviving shared files or local queue entries**.
- PI-CAM may report completion only at the completion point defined by Service 220 for the selected destination. If the existing transfer-completion contract needs stronger receiver acknowledgement or unambiguous final-delivery semantics, **revise Service 220 centrally before the implementation invents a private incompatible protocol**. Do not silently add a new wire ACK here.

### 3.3 Mapping onto one Pi

**Normative at EXN level:** the *component-to-component semantics* above, identifiers, data-transfer messages, result/status/error meaning, component authority, and the relationship to Service 220/Service 1/5.

**Implementation-specific in `exn-pi-cam`:** whether PI-CAM and PI-COMMS exchange full local CCSDS packets, immutable object handles, a Unix-domain socket message, file/object references or shared memory; how the chosen adapter maps local transfer state to the EXN message/state model; Linux process names, threading, buffers and storage.

A local optimized transport **need not physically copy or re-serialize every Service 220 chunk through a Unix socket** if it implements the *same defined logical transfer*, preserves identity and can demonstrate the same externally visible completion/error behavior. It shall not create a second independent authoritative mission service schema solely because the components are co-located.

## 4. Other same-host boundaries

- **PI-MGMT ↔ PI-CAM/PI-COMMS:** the externally observable restart/update and health contracts are defined centrally through Service 8/230 and endpoint HK. The platform may execute them through `systemd` or other local mechanisms; it cannot redefine mission update states.
- **Pi UART router ↔ PI-CAM/PI-COMMS/PI-MGMT:** internal Linux IPC and daemon framing are a transport implementation, not a new PUS service. When conveying a centrally defined component message, routing shall preserve its complete logical identity and EXN semantics. The router does not gain authorization or a source APID.
- **CM7 ↔ CM4, Linux driver ↔ service, KV260 PS ↔ PL, GS UI ↔ daemon:** these are internal boundaries within a component implementation, not separately allocated EXN components; their internal interfaces belong to their repositories.

## 5. Change-control and acceptance

Changes to **what one EXN component requests, sends, receives, acknowledges, interprets or reports to another**, including PI-CAM ↔ PI-COMMS, are proposed in `ExoSpaceLabs/exn` first. The participating component implementations must then follow the same reviewed version.

For acceptance, test PI-CAM ↔ PI-COMMS against this contract with a mock of either component and with the real co-located Pi pair. Cover authorized object selection, identity retention, metadata/chunk integrity, receiver backpressure, process restart/disconnect, completion/error propagation and concurrent high-priority TC/TM flow. The Pi repository owns test code and local mechanisms; EXN owns expected interface outcomes.
