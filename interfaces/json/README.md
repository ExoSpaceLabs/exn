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


## Camera configuration mirrors

The JSON interface set includes the Service 200 batched Camera configuration messages:

- `pkt_cam_set_config_tc.json` — atomic bounded multi-setting update;
- `pkt_cam_get_config_tc.json` — configuration query by scope mask;
- `pkt_cam_config_tm.json` — scoped active configuration report with revision.

Exposure, gain, autofocus mode, white balance, geometry, ISP/HDR and encoding controls are fields inside these messages rather than separate TC subservices.
