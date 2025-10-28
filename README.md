
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
flowchart LR
  %% GitHub-compatible Mermaid (no HTML breaks, simple labels)
  subgraph GS [Ground Station (PC)]
    GSNode[GS App (APID 0x0F0, SrcID 0x10)]
  end

  subgraph MCU [MCU-RTOS (STM32)]
    MCUNode[MCU Controller (APID 0x100, SrcID 0x01)]
  end

  subgraph PI [PI-CAM (Raspberry Pi 5 + Cam Module 3)]
    PiNode[Camera Node (APID 0x101, SrcID 0x02)]
  end

  subgraph FPGA [FPGA-AI]
    FpgaNode[Processing Node (APID 0x102, SrcID 0x03)]
  end

  GSNode -->|TC 3/1, 3/10, 20, 200, 210, 23 + Proxy| MCUNode
  MCUNode -->|Forwarded TMs 3/2, 20/3, 200/4-5, 210/4-5, 23/10-12, 5/x| GSNode
  GSNode -->|TM 250/1 Link/Proxy ACK| MCUNode

  MCUNode -->|TC 3/1, 20/1-2, 200/1-3, 210/1-3, 23/1-2| PiNode
  MCUNode -->|TC 3/1, 20/1-2, 210/1-3, 23/1-2| FpgaNode

  PiNode -->|TM 3/2, 200/4-5, 23/10-12, 5/x| MCUNode
  FpgaNode -->|TM 3/2, 210/4-5, 23/11-12, 5/x| MCUNode

  PiNode -->|Data path 23/10-12| FpgaNode
```

Notes:
- GS may request individual HK (Service 3/1) via MCU and may request a System HK aggregation (Service 3/10). MCU bridges/proxies; devices do not depend on GS.
- All APIDs are fixed: GS=0x0F0, MCU=0x100, PI=0x101, FPGA=0x102. Source IDs: MCU=0x01, PI=0x02, FPGA=0x03, GS=0x10.

ASCII fallback diagram (if Mermaid is not rendered):

```
[GS 0x0F0]
   |  TC: 3/1, 3/10, 20, 200, 210, 23 + Proxy Preamble
   v
[MCU 0x100] <----- TM 250/1 (ACK/NACK) from GS
  |  \
  |   \
  |    +--> [PI 0x101] <= TC: 3/1, 20, 200, 23
  |           ^
  |           | TM: 3/2, 200/4-5, 23/10-12, 5/x
  |
  +--> [FPGA 0x102] <= TC: 3/1, 20, 210, 23
              ^
              | TM: 3/2, 210/4-5, 23/11-12, 5/x

MCU forwards device TMs to GS and emits System HK TM 3/100 on GS request 3/10.
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
