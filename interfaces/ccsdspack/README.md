# CCSDSPack Packet Interfaces

This directory contains configuration interfaces for CCSDSPack to define ASTRAi TeleCommands (TC) and TeleMetry (TM) packets.

Layout:
- tc/ — TeleCommand interfaces (PUS-A/C)
- tm/ — TeleMetry interfaces (PUS-A/B/C)

Each `.cfg` file provides primary header defaults, PUS secondary header values, and an application data schema that matches the master ICD (docs/ICD.md) and device ICDs (docs/icd/*).

Notes:
- Multi-byte integers are big-endian (network order) unless noted in the ICD.
- Replace placeholders like `${APID}` and `${SRC_ID}` as needed per node at runtime or during configuration generation.
- These interfaces are intentionally minimal; extend the AppData sections as you implement more fields.

See also: ../json — JSON mirrors of the same interfaces for tooling and generation.
