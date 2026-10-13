# EXN Reference Network Allocations

The canonical reference inter-device interface is [`docs/icd/inter-node-links.md`](../../docs/icd/inter-node-links.md).

[`reference-topology.json`](reference-topology.json) is a machine-readable **deployment mirror**. It captures the switchless lab wiring choice, Pi UART and OBC↔KV260 USB CDC-ACM control links, direct Pi↔KV260 wired IP addresses, data listeners, framing rules, Pi transport-router role and Ground Wi-Fi connection. The normative CCSDS/PUS fields still come from the central [packet ICD](../../docs/ICD.md) and endpoint ICDs.

- Update the normative link ICD first; update this JSON in the same change.
- IP addresses, interface names, TCP port numbers, UART baud rates, USB host/device roles and target USB connector are deployment configuration, not Service 20 parameter IDs, APIDs or PUS Source IDs.
- No Wi-Fi credentials are stored here.
- This allocation does not imply that the Pi Wi-Fi gateway, STM32 UART drivers, Pi UART packet router, KV260 PL/Pmod UART, dedicated Pi↔KV260 Ethernet application or interop tests already exist.
- The validation script in `tools/validate_reference_topology.py` checks common structure and allocation errors.
