# Change log

This document records system, structural, interface-definition, and architecture-documentation changes.

## 2026-10-12

- Audit the current EXN architecture against the implemented/released dependency state and the published ECSS PUS service catalogue.
- Confirm the logical architecture remains valid: STM32/OBC control authority, independently supervised Linux Camera and Communications services, FPGA image/AI processing, controllable result routing, and direct bulk-data paths.
- Add `docs/services.md` as the service-tailoring authority.
- Classify the existing PUS-A Service 17/20/23 mappings as a transitional compatibility profile rather than the target PUS service baseline.
- Select PUS-C as the target EXN service profile and define the migration direction for standard Services 1, 3, 5, 9 and 20.
- Reserve mission-specific Service 220 for Payload Object Transfer; retain Services 200 and 210 for Camera and Processing.
- Deprecate legacy Service 250 as a target mission service; request verification moves to standard Service 1 and GS daemon acknowledgements remain local IPC.
- Align PI-COMMS identifiers across documentation and the MCU compatibility header: APID `0x103`, transitional Source ID `0x04`, proxy target 3, System-HK mask bit3.
- Remove the implied dynamic-heap requirement from the planned MCU HK definition and align the OBC documentation with HardRT/DAS static-oriented execution.
- Refresh the released reusable-component baseline: CCSDSPack v2.0.0, HardRT 0.5.1, DAS v0.1.0, SpWKit v0.7.0.
- Mark the existing CCSDSPack templates, JSON mirrors, and MCU header as compatibility artifacts that must migrate together with EXN-GS.
- Clarify the initial Raspberry Pi 5 as a **dual-role payload node**: PI-CAM camera/payload acquisition and PI-COMMS antenna/communications, including OBC communications and direct payload-data downlink to Ground.

## 2026-09-11

- Define the Raspberry Pi as an Embedded Linux Payload Platform hosting independent Camera and Antenna/Communications services.
- Add PI-COMMS as logical endpoint APID `0x103`.
- Keep STM32/OBC as command authority while allowing bulk payload data to bypass the MCU.
- Define FPGA as an image/AI processing endpoint with configurable result routing.
- Document Linux supervision/watchdog responsibilities and optional SpWKit logical links over shared Ethernet.
- Explicitly exclude private/commercial FPGA SpaceWire implementations from EXN scope.

## 2026-08-31

- Migrate the EXN shared packet baseline to CCSDSPack v2 / PUS revision-A packet headers for the existing HIL profile.
- Correct CCSDS Packet Data Length handling, APID direction policy, CRC profile, and shared packet templates.
- Establish `docs/ICD.md` and `interfaces/` as the central packet/interface authority.


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
