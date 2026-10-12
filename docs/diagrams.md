# EXN — Sequence Diagrams

This page collects representative EXN interactions among Ground Station (GS), STM32/OBC, Linux payload services, and FPGA processing.

The diagrams show **valid flows**, not one mandatory pipeline. EXN separates control authority from payload-data routing, and FPGA processing results may be routed differently depending on the request and mission policy.

> [!NOTE]
> All packet identifiers in these diagrams use the normative **PUS Revision C** EXN service profile. One-shot endpoint HK uses TC[3,27] / TM[3,25]; EXN System HK uses TC[3,128] / TM[3,129].

---

## 1) System Housekeeping Snapshot

The OBC supervises endpoints continuously and maintains the authoritative spacecraft HK cache. A Ground System-HK request does not normally create a downstream TC fan-out; it asks the OBC to serialize its current snapshot.

```mermaid
sequenceDiagram
  autonumber
  participant GS as Ground
  participant OBC as STM32 OBC
  participant CAM as PI-CAM
  participant FPGA as FPGA
  participant ANT as Antenna
  participant PM as PI-MGMT

  loop OBC supervision policy
    par Heartbeat / health refresh
      OBC->>CAM: TC[3,27]
      OBC->>FPGA: TC[3,27]
      OBC->>ANT: TC[3,27]
      OBC->>PM: TC[3,27]
    end
    CAM-->>OBC: TM[3,25]
    FPGA-->>OBC: TM[3,25]
    ANT-->>OBC: TM[3,25]
    PM-->>OBC: TM[3,25]
    OBC->>OBC: update HK cache + supervision/recovery state
  end

  GS->>OBC: TC[3,128] System HK (empty payload)
  OBC->>OBC: snapshot own HK + cached endpoint HK
  OBC-->>GS: TM[3,129] complete spacecraft snapshot
```

An endpoint that does not respond remains represented in the snapshot as STALE/OFFLINE/ERROR, together with its last valid cached data and OBC recovery state.

---

## 2) Flight-like Ground Command Path

Ground traffic enters through the Pi Antenna service, while the OBC remains the command authority. The same Antenna service carries OBC telemetry back to Ground.

```mermaid
sequenceDiagram
  autonumber
  participant GS as Ground
  participant ANT as Antenna
  participant MCU as STM32 OBC
  participant CAM as Camera Service

  GS->>ANT: uplink TC
  ANT->>MCU: deliver TC
  MCU->>CAM: routed camera command
  CAM-->>MCU: ACK / report
  MCU-->>ANT: downlinkable TM
  ANT-->>GS: TM
```

---

## 3) Capture -> FPGA Processing -> Camera Product -> Ground

The Camera service owns the payload product and delegates computation to FPGA.

```mermaid
sequenceDiagram
  autonumber
  participant CAM as Camera Service
  participant FPGA as FPGA Processing
  participant ANT as Antenna
  participant GS as Ground

  CAM->>CAM: capture image
  CAM->>FPGA: image + processing request
  FPGA-->>CAM: processed product / result
  CAM->>CAM: associate/store/package product
  CAM->>ANT: payload product
  ANT-->>GS: downlink
```

---

## 4) Classification Result Routed to OBC

A low-volume inference result may be sent directly to the OBC so software can make an autonomous mission decision.

```mermaid
sequenceDiagram
  autonumber
  participant CAM as Camera Service
  participant FPGA as FPGA Processing
  participant MCU as STM32 OBC

  CAM->>FPGA: image + classification request
  FPGA-->>MCU: classification result
  MCU->>MCU: evaluate mission policy
  alt retain/downlink
    MCU-->>CAM: retain / prepare for downlink
  else recapture
    MCU-->>CAM: capture again
  else discard
    MCU-->>CAM: release product
  end
```

---

## 5) OBC-Controlled Direct FPGA Result Downlink

When a ground link exists and policy permits it, a processing result may bypass the OBC data path.

```mermaid
sequenceDiagram
  autonumber
  participant MCU as STM32 OBC
  participant FPGA as FPGA Processing
  participant ANT as Antenna
  participant GS as Ground

  MCU->>FPGA: downlink result/job via Antenna
  FPGA-->>ANT: processing result
  ANT-->>GS: result downlink
```

The OBC may still receive separate status, metadata, or decision-relevant information. `Direct` here means FPGA -> Antenna -> Ground without routing the bulk result through the OBC; it does not bypass the Antenna link.

---

## 6) OBC-Controlled Direct Camera Downlink

The OBC controls the operation, but the image bytes do not transit the MCU.

```mermaid
sequenceDiagram
  autonumber
  participant MCU as STM32 OBC
  participant CAM as Camera Service
  participant ANT as Antenna
  participant GS as Ground

  MCU->>CAM: downlink payload object via Antenna
  CAM-->>ANT: payload object
  ANT-->>GS: payload downlink
  CAM-->>MCU: status / completion metadata
```

---

## 7) Logical Links over Shared Ethernet

SpWKit may represent multiple logical relationships over one physical Ethernet carrier.

```mermaid
graph LR
  ETH[Shared Ethernet Carrier]
  CAM[Camera]
  ANT[Antenna]
  MCU[STM32 OBC]
  FPGA[FPGA Processing]

  CAM --- ETH
  ANT --- ETH
  MCU --- ETH
  FPGA --- ETH

  CAM -. logical link .- MCU
  ANT -. logical link .- MCU
  CAM -. logical link .- FPGA
  CAM -. logical link .- ANT
  FPGA -. logical link .- ANT
```

The logical routes are independent of whether Camera and Antenna currently execute on the same Raspberry Pi.

---

## 8) Heartbeat / Health Supervision

The OBC supervises every endpoint independently.

```mermaid
sequenceDiagram
  autonumber
  participant OBC as STM32 OBC
  participant CAM as PI-CAM
  participant ANT as Antenna
  participant PM as PI-MGMT
  participant FPGA as FPGA

  par Periodic heartbeat
    OBC->>CAM: TC[3,27] HEARTBEAT
    OBC->>ANT: TC[3,27] HEARTBEAT
    OBC->>PM: TC[3,27] HEARTBEAT
    OBC->>FPGA: TC[3,27] HEARTBEAT
  end
  CAM-->>OBC: TM[3,25]
  ANT-->>OBC: TM[3,25]
  PM-->>OBC: TM[3,25]
  FPGA-->>OBC: TM[3,25]
  OBC->>OBC: update endpoint registry / alarm state
```

Missing heartbeat intervals transition an endpoint through STALE/OFFLINE according to OBC Service 20 policy and produce Service 5 events. If automatic recovery is enabled and the configured recovery delay expires, the OBC records and attempts the configured recovery action; System HK exposes that state.

---

## 9) Restart Failed Camera Service

PI-MGMT performs process recovery so a failed Camera process is not required to receive its own restart command.

```mermaid
sequenceDiagram
  autonumber
  participant OBC as STM32 OBC
  participant PM as PI-MGMT
  participant CAM as PI-CAM

  OBC->>PM: TC[8,1] restart exn-camera.service
  PM->>PM: systemd restart Camera
  PM-->>OBC: Service 1 start/progress verification
  CAM-->>OBC: HEARTBEAT recovered
  PM-->>OBC: Service 1 completion verification
```

The same pattern applies to the Antenna service.

---

## 10) OBC Time Synchronization

The OBC owns the synchronized mission clock and distributes it to every endpoint.

```mermaid
sequenceDiagram
  autonumber
  participant GS as Ground
  participant ANT as Antenna
  participant OBC as STM32 OBC
  participant CAM as PI-CAM
  participant PM as PI-MGMT
  participant FPGA as FPGA

  GS->>ANT: time synchronization TC
  ANT->>OBC: TC[9,128] Set Mission Time
  OBC->>OBC: update unix_ms offset from monotonic_ms
  par Distribute time
    OBC->>CAM: TC[9,128]
    OBC->>ANT: TC[9,128]
    OBC->>PM: TC[9,128]
    OBC->>FPGA: TC[9,128]
  end
```

Endpoint timers remain monotonic; synchronized Unix-epoch time is used for mission timestamps.

---

## 11) Camera Acquisition and Multi-Format Products

One acquisition may preserve RAW and create multiple derived products.

```mermaid
sequenceDiagram
  autonumber
  participant OBC as STM32 OBC
  participant CAM as PI-CAM

  OBC->>CAM: TC[200,1] SINGLE, products=RAW|TIFF|JPEG
  CAM->>CAM: capture sensor frame
  CAM->>CAM: preserve RAW object
  CAM->>CAM: derive TIFF
  CAM->>CAM: derive JPEG
  CAM-->>OBC: Service 1 completion
  OBC->>CAM: TC[200,8] List Products
  CAM-->>OBC: TM[200,9] object catalog
```

The OBC may then select any product for processing, local retention or Ground downlink.

---

## 12) Software Update of Pi Camera Service

Update payload transfer and software activation are separate operations.

```mermaid
sequenceDiagram
  autonumber
  participant OBC as STM32 OBC
  participant PM as PI-MGMT
  participant CAM as PI-CAM

  OBC->>PM: Service 220 update package object
  OBC->>PM: TC[230,3] Stage Camera update
  PM->>PM: validate package / hash / compatibility
  OBC->>PM: TC[230,4] Activate update
  PM->>PM: install and restart exn-camera.service
  CAM-->>OBC: heartbeat + version
  PM-->>OBC: Service 1 completion / update status
```

An update is not successful until the updated component returns healthy.
