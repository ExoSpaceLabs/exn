# EXN CCSDSPack v2 packet templates

These `.cfg` files are CCSDSPack v2 templates for the **current transitional wire profile** documented in `docs/ICD.md`. The target PUS service profile is `docs/services.md`.

## Current compatibility profile

All current templates use:

- CCSDS Space Packet Version Number `0`;
- CRC-16/CCITT-FALSE packet error control;
- unsegmented Space Packets;
- PUS revision A for the currently implemented EXN-GS/HIL service set;
- TC selector `PUS:revA:TC`, one-octet source ID, acknowledgement flags `0`;
- TM selector `PUS:revA:TM`, zero-octet destination ID, no secondary-header timestamp or packet subcounter.

PUS revision is independent from service number. Several legacy EXN service identifiers have meanings that do not match the published PUS-C catalogue. In particular, the current Service 17 time and Service 23 payload-transfer assignments are compatibility definitions, not target PUS service allocations.

Do **not** add new templates by extending those legacy assignments. New interface work follows `docs/services.md` and will migrate these templates to PUS-C in a coordinated change.

## APID policy

- TC APID identifies the destination EXN endpoint.
- TM APID identifies the producing EXN endpoint.

Templates whose endpoint can vary use a documented default APID and are expected to be overridden by the application when targeting another node.

The GS-specific System HK template is direct GS -> MCU and therefore uses APID `0x100`, Source ID `0x10`, Service `3/10`.

## Layout

- `tc/` — telecommand/request templates.
- `tm/` — telemetry/report templates.

`data_field_size` is a template capacity/default and is not the CCSDS Packet Data Length wire value. CCSDSPack calculates Packet Data Length from the finalized secondary header, Application Data, and packet error-control trailer.

The JSON directory is currently a partial tooling mirror, not an authoritative second definition. The `.cfg` files and master ICD are authoritative until JSON generation/validation is automated.


## Planned migration

The target interface set will use CCSDSPack PUS-C TC/TM headers, standard request verification and normalized PUS service semantics. Existing files remain authoritative only for supported transitional EXN-GS/HIL traffic until that migration lands. Service-number changes must update the master ICD, templates, JSON/tooling mirrors, MCU headers, GS builders and simulator fixtures together.
