# EXN CCSDSPack v2 packet templates

These `.cfg` files are machine-readable realizations of the normative PUS-C EXN ICD.

## Active profile

All templates use:

- CCSDS Space Packet Version Number `0`;
- CRC-16/CCITT-FALSE packet error control;
- unsegmented Space Packets by default;
- **PUS Revision C** only;
- TC selector `PUS:revC:TC`;
- TM selector `PUS:revC:TM`;
- fixed two-octet PUS-C source/destination identifiers;
- command-specific PUS-C acknowledgement flags;
- PUS-C TM message type counters;
- six-octet CUC TM timestamps using the EXN agency-defined Unix epoch.

PUS-A selectors are not valid active EXN interface artifacts.

## APID and PUS identifier policy

- TC APID identifies the destination EXN endpoint.
- TM APID identifies the producing EXN endpoint.
- PUS-C source/destination IDs use the 16-bit endpoint identities from `docs/ICD.md`.

Templates whose endpoint can vary document a default APID/ID and are overridden by the application for another valid endpoint.

## Service examples

- one-shot HK: TC 3/27 -> TM 3/25;
- System HK: TC 3/128 -> TM 3/129;
- parameters: TC 20/1 -> TM 20/2, TC 20/3 set;
- mission time set: TC 9/128;
- Camera acquisition: TC 200/1;
- processing job: TC 210/1;
- object transfer: Service 220;
- request verification: Service 1.

Custom Camera/FPGA ACK packets and GS Service 250 are intentionally absent; Service 1 owns request verification.

## Authority

The normative meaning comes from `docs/ICD.md`, `docs/services.md`, `docs/component-services.md`, and `docs/icd/`.

These templates must agree with those documents. They are not an independent protocol definition.
