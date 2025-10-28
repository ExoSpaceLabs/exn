
<div style="text-align: center;">
    <img alt="astrai_logo" src="docs/images/astrai_logo.png" width="200" />
</div>


# ASTRAi [[ExoSpaceLabs](https://github.com/ExoSpaceLabs)] – Advanced Satellite Telemetry, Recognition & AI 

**ASTRAi** is a modular open-source demonstration payload that combines a **Raspberry Pi**, **STM32 (RTOS)**, and **FPGA** into an integrated Earth Observation system.  
The goal is to showcase onboard image capture, CCSDS packetization, AI inference, and system-level coordination – all within a realistic space payload architecture.

---

## 🌌 Project Overview
ASTRAi demonstrates how different embedded platforms can work together as a cohesive payload:
- **Pi (Camera Node):** Captures images, pre-packets or streams raw data.
- **STM32 RTOS (Control Node):** Supervises the system, handles CCSDS TeleCommands/TeleMetry, performs health checks, and routes control.
- **FPGA (Processing Node):** Performs preprocessing (resize/normalize), inference acceleration, and postprocessing (argmax).
- **CCSDSPack:** Provides CCSDS packet definitions and handling across nodes.

All interfaces (CCSDS, SPI/UART/CAN links, FPGA register maps) are defined in the central **ASTRAi interfaces** specification, ensuring consistency across hardware.

---

## 📂 Repository Structure
This is the **meta-repo** that ties everything together.

- `interfaces/` → YAML specs + generated headers/packages (C, C++, VHDL)
- `docs/` → architecture docs, diagrams, and roadmaps
- `system/` → integration scripts, manifests, CI/CD workflows
- `submodules/`:
  - [astrai-pi-cam](https://github.com/ExoSpaceLabs/astrai-pi-cam)
  - [astrai-mcu-rtos](https://github.com/ExoSpaceLabs/astrai-mcu-rtos)
  - [astrai-fpga-ai](https://github.com/ExoSpaceLabs/astrai-fpga-ai)
  - [ccsdspack](https://github.com/ExoSpaceLabs/CCSDSPack)

---

## 🧭 Modular System Graph

```mermaid
graph LR
  %% GitHub-friendly Mermaid: quoted subgraph labels and explicit directions
  subgraph "Ground Station"
    direction TB
    GSNode[GS App APID 0x0F0 SrcID 0x10]
  end

  subgraph "Astrai Payload"
  
      subgraph "MCU RTOS"
        direction TB
        MCUNode[MCU Controller APID 0x100 SrcID 0x01]
      end
    
      subgraph "PI CAM"
        direction TB
        PiNode[Camera Node APID 0x101 SrcID 0x02]
      end
    
      subgraph "FPGA AI"
        direction TB
        FpgaNode[Processing Node APID 0x102 SrcID 0x03]
      end
      
  end

  GSNode --> MCUNode
  MCUNode --> GSNode

  MCUNode --> PiNode
  MCUNode --> FpgaNode

  PiNode --> MCUNode
  FpgaNode --> MCUNode

  PiNode --> FpgaNode
```

Notes:
- GS may request individual HK (Service 3/1) via MCU and may request a System HK aggregation (Service 3/10). MCU bridges/proxies; devices do not depend on GS.
- All APIDs are fixed: GS=0x0F0, MCU=0x100, PI=0x101, FPGA=0x102. Source IDs: MCU=0x01, PI=0x02, FPGA=0x03, GS=0x10.

### ➕ Additional Diagrams

System HK aggregation (GS requests 3/10, MCU aggregates device HK 3/2 into TM 3/100):

```mermaid
sequenceDiagram
  autonumber
  participant GS as GS (APID 0x0F0)
  participant MCU as MCU-RTOS (0x100)
  participant PI as PI-CAM (0x101)
  participant FPGA as FPGA-AI (0x102)

  GS->>MCU: TC 3/10 Request System HK
  par Fan-out HK requests
    MCU->>PI: TC 3/1 Request HK
    MCU->>FPGA: TC 3/1 Request HK
    MCU->>MCU: Collect self HK
  end
  PI-->>MCU: TM 3/2 HK Report (PI)
  FPGA-->>MCU: TM 3/2 HK Report (FPGA)
  MCU-->>GS: TM 3/100 System HK Report (aggregated)
```

Capture → Transfer → Execute → Result (GS-driven end-to-end):

```mermaid
sequenceDiagram
  autonumber
  participant GS as GS (0x0F0)
  participant MCU as MCU (0x100)
  participant PI as PI (0x101)
  participant FPGA as FPGA (0x102)

  GS->>MCU: TC 200/1 Capture (target=PI)
  MCU->>PI: TC 200/1 Capture
  PI-->>MCU: TM 200/5 ACK
  MCU-->>GS: TM 200/5 ACK (forwarded)

  GS->>MCU: TC 23/1 Start Transfer (dest=FPGA)
  MCU->>PI: TC 23/1 Start Transfer
  PI-->>MCU: TM 23/10 Metadata
  MCU-->>FPGA: TM 23/10 Metadata (bridged)
  loop Chunks
    PI-->>MCU: TM 23/11 Data Chunk
    MCU-->>FPGA: TM 23/11 Data Chunk (bridged)
  end
  PI-->>MCU: TM 23/12 Transfer Complete
  MCU-->>FPGA: TM 23/12 Transfer Complete

  GS->>MCU: TC 210/1 Execute (target=FPGA)
  MCU->>FPGA: TC 210/1 Execute
  FPGA-->>MCU: TM 210/5 ACK
  MCU-->>GS: TM 210/5 ACK (forwarded)
  FPGA-->>MCU: TM 23/11 Result Chunk(s)
  FPGA-->>MCU: TM 23/12 Result Complete
  MCU-->>GS: Result TMs (forwarded)
```

---

## 📑 Interfaces & ICD
- Master ICD: [docs/ICD.md](docs/ICD.md)
- Device/Service ICDs:
  - GS (Ground Station Simulator): [docs/icd/gs.md](docs/icd/gs.md)
  - MCU-RTOS: [docs/icd/mcu-rtos.md](docs/icd/mcu-rtos.md)
  - PI-CAM (Raspberry Pi 5 + Camera Module 3): [docs/icd/pi-cam.md](docs/icd/pi-cam.md)
  - FPGA-AI: [docs/icd/fpga-ai.md](docs/icd/fpga-ai.md)

---

## 🛠 Roadmap
1. **Phase 1 – Pi Camera**
   - Define ACK / NACK / TM and packets structures.
   - Bring up Pi camera, capture single frames
   - Export to CCSDS packets with CCSDSPack
3. **Phase 2 – STM32 RTOS**
   - Implement TC/TM service handler
   - Add health monitoring + basic CAN/SPI link
4. **Phase 3 – FPGA Integration**
   - AXI register map + resize block
   - Add inference model (ImageNet classifier)
5. **Phase 4 – Full System Integration**
   - Pi → STM → FPGA workflow
   - End-to-end CCSDS packet chain
   - Demo: classification result sent as telemetry

---

## 📜 License
Apache 2.0 (open source, research & demonstration use).
