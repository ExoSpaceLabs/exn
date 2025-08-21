
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
