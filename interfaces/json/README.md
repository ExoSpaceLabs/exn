# ASTRAi Packet Interfaces (JSON)

This directory mirrors the CCSDSPack `.cfg` interfaces in a JSON format suitable for tooling, generation, and validation.

Layout:
- tc/ — TeleCommand packet definitions
- tm/ — TeleMetry packet definitions

Schema (per file):
{
  "name": "pkt_name",
  "primaryHeader": { "version":0, "type":"TC|TM", "apid":256, "secHdrFlag":1, "seqFlags":"UNSEG", "seqCount":0, "dataLength":0 },
  "pusHeader": { "type":"PusA|PusB|PusC", "version":1, "service":3, "subservice":1, "sourceId":1, "eventId":0, "timeCodeBytes":[] },
  "appData": [ { "name":"field", "type":"U16|U32|I16|BYTES|TLV|BOOL|F32|U8", "bits":16, "description":"..." } ]
}

Notes:
- `seqCount` and `dataLength` are typically filled by the application.
- For PUS-B include `eventId`. For PUS-C use `timeCodeBytes`.
- Types are informational for tools; CCSDSPack consumes `.cfg` files for runtime packet construction.
