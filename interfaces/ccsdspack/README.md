# EXN CCSDSPack v2 packet templates

These `.cfg` files are CCSDSPack v2 templates aligned with `docs/ICD.md`.

## EXN v2 profile

All current templates use:

- CCSDS Space Packet Version Number `0`;
- CRC-16/CCITT-FALSE packet error control;
- unsegmented Space Packets;
- PUS revision A for every current EXN service;
- TC selector `PUS:revA:TC`, one-octet source ID, acknowledgement flags `0`;
- TM selector `PUS:revA:TM`, zero-octet destination ID, no secondary-header timestamp or packet subcounter.

PUS revision is independent from service number. Service 5 events and Service 17 time therefore remain Rev-A packets. Event IDs and mission time values belong to Application Data as defined by the ICD.

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
