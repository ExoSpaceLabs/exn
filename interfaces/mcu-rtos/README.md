# ASTRAi Interfaces for MCU-RTOS (Header-only)

For STM32 MCU builds that lack a filesystem, use the header-only packet interface definitions in this directory.
These mirror the master ICD and allow you to construct CCSDS/PUS packets programmatically without loading
`.cfg` files at runtime.

Files:
- `astrai_interfaces.h` — APIDs, Source IDs, services/subservices, TLV types, result codes, and packed
  payload structs for common TCs/TMs (HK, System HK, Camera, FPGA, Data Transfer, GS Link ACK), plus
  small big-endian helper functions.

How to use:
- Include the header in your MCU project: `#include "interfaces/mcu-rtos/astrai_interfaces.h"`
- Build packets using CCSDSPack (recommended) or your own serialization by writing fields in big-endian order.
- Follow the service/subservice IDs exactly as defined in the enums; match Application Data layouts in the structs.

Notes:
- Multi-byte fields in Application Data are big-endian (network order) per `docs/ICD.md`.
- The provided structs describe layout; you are responsible for endian conversions when populating buffers.
- For PI/FPGA/GS on systems with filesystems, prefer the `.cfg` and JSON interfaces under `interfaces/ccsdspack/` and `interfaces/json/`.
