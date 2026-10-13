#!/usr/bin/env python3
"""Validate EXN's reference deployment map (not a TCP/PHY implementation test)."""

import ipaddress
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROFILE = ROOT / "interfaces" / "network" / "reference-topology.json"


def check(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    d = json.loads(PROFILE.read_text(encoding="utf-8"))
    check(d["schema_version"] == 1, "unsupported topology schema")
    check(d["packet_profile"] == "CCSDS-Space-Packet/PUS-Revision-C/CRC16", "packet profile drift")
    check(d["transport"]["protocol"] == "TCP/IPv4", "reference transport drift")
    check(d["transport"]["framing"] == "raw-concatenated-CCSDS-space-packets", "framing drift")
    check(1024 <= d["transport"]["max_packet_bytes"] <= 65542, "invalid packet budget")
    check(0 < d["transport"]["object_chunk_payload_bytes"] < d["transport"]["max_packet_bytes"],
          "invalid object chunk budget")

    on = d["networks"]["onboard"]
    net = ipaddress.ip_network(on["network"], strict=True)
    check(net.is_private, "onboard LAN must use private addresses")
    check(on["configuration"] == "static", "onboard addresses must be static")
    check(on["default_gateway"] is None and on["nat_to_ground"] is False,
          "onboard network must not be routed to Ground")
    check(on["extra_switch_required"] is True, "missing external Ethernet switch requirement")
    check(d["networks"]["ground"]["medium"] == "Wi-Fi", "Ground reference is Wi-Fi")

    hosts = {x["id"]: x for x in d["hardware"]}
    check(set(hosts) == {"obc", "pi", "fpga"}, "expected exactly three physical onboard hosts")
    seen_ips = set()
    for name, h in hosts.items():
        ip = ipaddress.ip_address(h["ip"])
        check(ip in net and ip not in {net.network_address, net.broadcast_address},
              f"{name} IP outside onboard network")
        check(h["prefix_length"] == net.prefixlen, f"{name} prefix mismatch")
        check(h["ip"] not in seen_ips, f"{name} duplicate physical IP")
        seen_ips.add(h["ip"])

    normative = {
        "OBC": (0x100, 0x0001, "obc", 47100),
        "PI-CAM": (0x101, 0x0002, "pi", 47101),
        "FPGA": (0x102, 0x0003, "fpga", 47102),
        "PI-COMMS": (0x103, 0x0004, "pi", 47103),
        "PI-MGMT": (0x104, 0x0005, "pi", 47104),
        "GS": (0x0F0, 0x0010, "pc", None),
    }
    endpoints = d["endpoints"]
    check(len(endpoints) == len(normative), "unexpected endpoint count")
    seen_names, seen_apids, seen_ids, seen_sockets = set(), set(), set(), set()
    for x in endpoints:
        name = x["name"]
        check(name in normative and name not in seen_names, f"unknown/duplicate endpoint {name}")
        seen_names.add(name)
        check((x["apid"], x["pus_id"], x["host"], x["tcp_port"]) == normative[name],
              f"endpoint identity/host/port drift: {name}")
        check(x["apid"] not in seen_apids and x["pus_id"] not in seen_ids,
              f"duplicate mission identity: {name}")
        seen_apids.add(x["apid"])
        seen_ids.add(x["pus_id"])
        if x["tcp_port"] is not None:
            check(1024 <= x["tcp_port"] <= 65535, f"invalid listener port: {name}")
            sock = (hosts[x["host"]]["ip"], x["tcp_port"])
            check(sock not in seen_sockets, f"socket collision: {name}")
            seen_sockets.add(sock)

    gs = d["ground_listener"]
    check(gs["logical_endpoint"] == "PI-COMMS", "Ground ingress must be PI-COMMS")
    check(gs["binding_interface"] == "wlan0", "Ground listener must bind to Wi-Fi")
    check(gs["port"] == 47000, "unexpected Ground listener port")
    check(gs["max_active_command_sessions"] == 1, "must explicitly limit Ground command authority")
    check(gs["protocol"] == "TCP", "Ground listener must use TCP")

    icd = (ROOT / "docs" / "icd" / "inter-node-links.md").read_text(encoding="utf-8")
    for item in [
        "10.77.0.0/24", "10.77.0.10:47100", "10.77.0.20:47101",
        "10.77.0.30:47102", "10.77.0.20:47103", "10.77.0.20:47104",
        "47000", "8192", "1024", "PUS-C",
    ]:
        check(item in icd, f"ICD missing mirrored integration value: {item}")

    print("PASS: EXN network topology IDs, IPs, listeners, framing and ICD mirror")


if __name__ == "__main__":
    main()
