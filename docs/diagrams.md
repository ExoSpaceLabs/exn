# EXN — Sequence Diagrams

This page collects the rendered Mermaid sequence diagrams referenced by the root README. They illustrate key interactions among Ground Station (GS), MCU-RTOS (MCU), PI-CAM (PI), and FPGA-AI (FPGA).

Notes for GitHub rendering:
- Mermaid blocks use conservative, GitHub-friendly syntax.
- Subgraph quotations are not required for sequence diagrams, but labels are kept simple.

---

## 1) System Housekeeping Aggregation

GS requests a System HK from MCU. MCU fans out HK requests to devices, aggregates their HK, and returns a single System HK report.

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

---

## 2) Capture → Transfer → Execute → Result

A GS-driven end-to-end scenario: capture on PI, transfer to FPGA, execute inference, and receive results via MCU.

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
  PI-->>FPGA: TM 23/10 Metadata
  loop Chunks
    PI-->>FPGA: TM 23/11 Data Chunks
  end
  PI-->>FPGA: TM 23/12 Transfer Complete

  GS->>MCU: TC 210/1 Execute (target=FPGA)
  MCU->>FPGA: TC 210/1 Execute
  FPGA-->>MCU: TM 210/5 ACK
  MCU-->>GS: TM 210/5 ACK (forwarded)
  FPGA-->>MCU: TM 23/11 Result Chunk(s)
  FPGA-->>MCU: TM 23/12 Result Complete
  MCU-->>GS: Result TMs (forwarded)
```
