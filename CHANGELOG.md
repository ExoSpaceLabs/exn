# Change log
This document holds changes that have been performed on the documents. should be used for system, structural, definition changes.

## 2025-10-28
- Add changelog file.
- Fix: README Mermaid diagram failed to render on GitHub due to syntax error (subgraph label format). Updated to `subgraph <id> [Label]` and replaced `\n` with `<br/>` in node labels; adjusted arrow notation for GitHub Mermaid. Added ASCII fallback remains unchanged.
- Docs: Verified APIDs/IDs consistency across ICDs; no changes needed for this fix.
- Docs: Add two additional Mermaid sequence diagrams to README: System HK aggregation flow and GS-driven end-to-end capture→transfer→execute→result workflow.
- Docs: Move the additional sequence diagrams out of README into `docs/diagrams.md`; README now links to this page.
- Interfaces: Create `interfaces/ccsdspack/{tc,tm}` and seed core `.cfg` interfaces (HK req/report, System HK req/tm, Time set, Param set/get/value, Camera capture/ACK, FPGA exec/ACK, Xfer start/stop/meta/chunk/done, GS Link ACK).
- Interfaces: Add JSON mirrors under `interfaces/json/{tc,tm}` and seed representative files (`pkt_hk_req_tc.json`, `pkt_system_hk_req_tc.json`, `pkt_system_hk_tm.json`).
- Docs: Update `docs/ICD.md` Section 7 to reference the new interfaces directories and example filenames; link interfaces directories from README.
- MCU: Add header-only interfaces for STM32 builds under `interfaces/mcu-rtos/` (`exn_interfaces.h`) and README.
