# EXN Packet Interfaces (JSON)

This directory contains machine-readable mirrors used by tooling and validation. The authoritative wire contract is `docs/ICD.md`; authoritative CCSDSPack runtime templates are under `interfaces/ccsdspack/`.

The JSON set is currently **partial**, not a complete mirror of every `.cfg` file. Do not infer that a missing JSON file means the corresponding service is undefined.

## Current protocol model

- CCSDS Space Packet version 0.
- CRC-16 packet error control.
- PUS revision A for all currently defined services.
- TC selector: `PUS:revA:TC`, one-octet source ID.
- TM selector: `PUS:revA:TM`, zero-octet destination ID.
- TC APID identifies the destination endpoint.
- TM APID identifies the producing endpoint.
- Packet Data Length and sequence count are runtime packet state and are not fixed interface constants.

Example shape:

```json
{
  "name": "pkt_name",
  "packetErrorControl": "crc16",
  "primaryHeader": {
    "version": 0,
    "type": "TC",
    "apid": 256,
    "apidPolicy": "destination",
    "secHdrFlag": 1,
    "seqFlags": "UNSEG"
  },
  "pusHeader": {
    "selector": "PUS:revA:TC",
    "sourceIdOctets": 1,
    "acknowledgementFlags": 0,
    "service": 3,
    "subservice": 10,
    "sourceId": 16
  },
  "appData": []
}
```

Service-specific event IDs, CUC time values, transaction IDs, and similar fields belong to Application Data unless the master ICD explicitly states otherwise. In particular, EXN does not use the old JSON model that associated Service 5 with a PUS-B header or Service 17 with a PUS-C header.
