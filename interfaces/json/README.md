# EXN Packet Interfaces (JSON)

This directory contains partial machine-readable mirrors of the normative EXN ICD for tooling/validation.

The active wire profile is **PUS Revision C**:

- CCSDS Space Packet version 0;
- CRC-16/CCITT-FALSE;
- TC selector `PUS:revC:TC`;
- TM selector `PUS:revC:TM`;
- fixed 16-bit PUS-C source/destination identifiers;
- PUS-C TM message type counter;
- six-octet CUC TM timestamp using the EXN agency-defined Unix epoch;
- TC APID = destination;
- TM APID = producer.

Example TC shape:

```json
{
  "pusHeader": {
    "selector": "PUS:revC:TC",
    "acknowledgementFlags": 8,
    "service": 3,
    "subservice": 128,
    "sourceId": 16
  }
}
```

The JSON set is currently partial. Missing JSON does not mean a service is undefined. Normative semantics live in the central ICD documents; JSON shall be generated or cross-validated against that source as the interface tooling matures.

PUS-A JSON definitions are obsolete and shall not be reintroduced.
