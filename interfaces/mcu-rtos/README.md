# EXN Interfaces for MCU/OBC

Bare-metal consumers may use `exn_interfaces.h` for mission identifiers and bounded Application Data layouts.

## Active profile

- CCSDS Space Packet Version 0;
- PUS Revision C;
- PUS-C 16-bit source/destination identifiers;
- CRC-16/CCITT-FALSE;
- big-endian mission Application Data;
- APID semantics and service allocations from the central EXN ICD.

The header contains mission constants only. The OBC packet implementation shall use the CCSDSPack PUS-C API.

## Authority

`docs/ICD.md`, `docs/services.md`, `docs/component-services.md`, and `docs/icd/` are normative.

If the MCU implementation needs a new field or identifier, it is defined in the central ICD first and then added to the shared interface header.

There is no active ASTRaI/PUS-A compatibility interface in this directory.
