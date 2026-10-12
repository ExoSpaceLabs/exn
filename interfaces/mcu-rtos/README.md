# EXN Interfaces for MCU-RTOS (Header-only)

Bare-metal/STM32 builds may not have a filesystem, so EXN keeps the mission interface constants and Application Data layouts in a header-only form.

## Files

- `exn_interfaces.h` — compatibility-profile EXN APIDs, PUS-A TC Source IDs, legacy service/subservice identifiers, result/TLV enums, payload layouts, and big-endian helpers.
- `astrai_interfaces.h` — compatibility shim for older consumers. New code should include `exn_interfaces.h` directly.

## CCSDSPack v2 usage

The current **EXN-GS/HIL compatibility baseline** uses CCSDSPack v2.x with:

- CCSDS Packet Version Number 0;
- `PUS:revA:TC` for telecommands/requests;
- `PUS:revA:TM` for telemetry/reports;
- one-octet TC source ID;
- zero-octet TM destination ID;
- CRC-16 packet error control;
- big-endian mission Application Data.

The C header intentionally does not reproduce CCSDSPack C++ classes. Firmware should construct the packet with the CCSDSPack MCU API and use this header for mission identifiers/payload definitions.

APID policy is defined by `docs/ICD.md`: TC APID is the destination endpoint, while TM APID is the producing endpoint.

## Include

```c
#include "interfaces/mcu-rtos/exn_interfaces.h"
```

The packed structs document wire field order only. Multi-byte values must be written/read using the provided big-endian helpers rather than relying on host struct endianness.

Filesystem-capable consumers may use the `.cfg` templates under `interfaces/ccsdspack/`; JSON files under `interfaces/json/` are tooling mirrors and currently cover only a subset of interfaces.


## Service-profile status

This header is not the target OBC service API. It currently mirrors the transitional PUS-A wire profile so existing tools can share identifiers and payload layouts. `docs/services.md` defines the target PUS-C service profile. The MCU implementation shall migrate this header together with the CCSDSPack templates and EXN-GS rather than encoding legacy Service 17/20/23 assignments into new firmware.
