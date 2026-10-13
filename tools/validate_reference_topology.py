#!/usr/bin/env python3
"""Validate the current EXN switchless reference inter-node transport profile."""

import ipaddress
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROFILE = ROOT / "interfaces" / "network" / "reference-topology.json"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    d = json.loads(PROFILE.read_text(encoding="utf-8"))
    require(d["schema_version"] == 2, "expected switchless v2 topology schema")
    require(d["packet_profile"] == "CCSDS-Space-Packet/PUS-Revision-C/CRC16", "PUS profile drift")
    budget = d["packet_budget"]
    require(1024 <= budget["max_decoded_packet_bytes"] <= 65542, "invalid packet budget")
    require(0 < budget["ethernet_object_chunk_payload_bytes"] < budget["max_decoded_packet_bytes"],
            "invalid object chunk budget")

    links = {x["id"]: x for x in d["physical_links"]}
    require(set(links) == {"ground", "obc_pi_control", "obc_fpga_control", "pi_fpga_data"},
            "unexpected physical links (no switch or OBC Ethernet)")
    for name, endpoints in {
        "ground": {"gs", "pi"},
        "obc_pi_control": {"obc", "pi"},
        "obc_fpga_control": {"obc", "fpga"},
        "pi_fpga_data": {"pi", "fpga"},
    }.items():
        require(set(links[name]["endpoints"]) == endpoints, f"wrong peers: {name}")
    for name in ("obc_pi_control", "obc_fpga_control"):
        lnk = links[name]
        require("UART" in lnk["media"], f"{name} must be UART")
        require(lnk["framing"] == "COBS-complete-CCSDS-packet-delimiter-00", f"{name} framing drift")
        require(lnk["baud_initial"] == 115200, f"{name} initial baud drift")
        require(lnk["baud_qualification"] == 921600, f"{name} qualification baud drift")
        require(lnk["uart_mode"] == "8N1", f"{name} UART mode drift")
    require(links["obc_fpga_control"]["requires_pl_firmware"], "Pmod UART must declare PL dependency")
    require(links["ground"]["media"] == "Wi-Fi", "Ground must use Wi-Fi")
    require(links["pi_fpga_data"]["media"] == "direct-Gigabit-Ethernet", "data link drift")
    require(links["pi_fpga_data"]["requires_switch"] is False, "Ethernet switch introduced")
    require(links["pi_fpga_data"]["framing"] == "raw-concatenated-CCSDS-packets",
            "Ethernet packet framing drift")

    hosts = {x["id"]: x for x in d["physical_hosts"]}
    require(set(hosts) == {"obc", "pi", "fpga"}, "expected three hardware hosts")
    require(hosts["obc"]["ethernet_used"] is False, "OBC must not use Ethernet")
    require((hosts["obc"]["uart_a"]["tx_pin"], hosts["obc"]["uart_a"]["rx_pin"])
            == ("PB6", "PB7"), "STM32 UART A pins drift")
    require((hosts["obc"]["uart_b"]["tx_pin"], hosts["obc"]["uart_b"]["rx_pin"])
            == ("PD5", "PD6"), "STM32 UART B pins drift")
    require((hosts["pi"]["header_uart"]["tx_header_pin"], hosts["pi"]["header_uart"]["rx_header_pin"])
            == (8, 10), "Pi UART header mapping drift")
    require(hosts["fpga"]["control_uart"]["requires_pl_logic"] is True,
            "KV260 requires a PL UART")
    require(hosts["fpga"]["control_uart"]["connector"] == "J2 Pmod",
            "incorrect KV260 control connector")

    netcfg = d["networks"]["pi_fpga_direct"]
    net = ipaddress.ip_network(netcfg["network"])
    require(net == ipaddress.ip_network("10.77.0.0/30"), "data subnet drift")
    require(netcfg["requires_switch"] is False, "switch is not part of profile")
    require(netcfg["default_gateway"] is None, "no gateway allowed on direct link")
    require(netcfg["wifi_ethernet_nat"] is False and netcfg["wifi_ethernet_bridge"] is False,
            "Pi must not route Ground to FPGA data network")
    peers = {x["host"]: x for x in netcfg["peers"]}
    require(set(peers) == {"pi", "fpga"}, "direct Ethernet is only Pi<->FPGA")
    for host, ip in {"pi": "10.77.0.1", "fpga": "10.77.0.2"}.items():
        require(peers[host]["ip"] == ip and peers[host]["prefix_length"] == 30,
                f"{host} IP address mismatch")
        require(ipaddress.ip_address(ip) in net, f"{host} not in subnet")

    expected = {
        "OBC": (0x100, 0x0001, "obc", None),
        "PI-CAM": (0x101, 0x0002, "pi", ("10.77.0.1", 47101)),
        "FPGA": (0x102, 0x0003, "fpga", ("10.77.0.2", 47102)),
        "PI-COMMS": (0x103, 0x0004, "pi", ("10.77.0.1", 47103)),
        "PI-MGMT": (0x104, 0x0005, "pi", None),
        "GS": (0x0F0, 0x0010, "gs", None),
    }
    eps = d["endpoints"]
    require(len(eps) == len(expected), "endpoint count mismatch")
    names, apids, pusids, sockets = set(), set(), set(), set()
    for ep in eps:
        name = ep["name"]
        require(name in expected and name not in names, f"unexpected/duplicate endpoint {name}")
        names.add(name)
        listener = ep["ethernet_listener"]
        actual = None if listener is None else (listener["ip"], listener["tcp_port"])
        require((ep["apid"], ep["pus_id"], ep["host"], actual) == expected[name],
                f"endpoint mapping mismatch: {name}")
        require(ep["apid"] not in apids and ep["pus_id"] not in pusids, "mission identity collision")
        apids.add(ep["apid"])
        pusids.add(ep["pus_id"])
        if actual:
            require(actual not in sockets, "data listener address collision")
            sockets.add(actual)
    router = d["pi_control_router"]
    require(router["name"] == "exn-linkd" and router["spacecraft_endpoint"] is False,
            "router is a transport process, not a new PUS endpoint")
    require(router["independently_supervised_from_pi_comms"], "router must survive COMMS failure")
    require(set(router["clients"]) == {"PI-CAM", "PI-COMMS", "PI-MGMT"}, "router clients drift")

    ground = d["ground_listener"]
    require((ground["logical_endpoint"], ground["binding_interface"],
             ground["protocol"], ground["port"]) == ("PI-COMMS", "wlan0", "TCP", 47000),
            "Ground Wi-Fi endpoint mismatch")
    require(ground["max_active_command_sessions"] == 1, "multiple Ground command sessions")
    doc = (ROOT / "docs" / "icd" / "inter-node-links.md").read_text(encoding="utf-8")
    for v in ("LPUART1", "USART2", "PB6", "PB7", "PD5", "PD6", "J2",
              "COBS", "10.77.0.0/30", "10.77.0.1", "10.77.0.2",
              "47101", "47102", "47103", "47000", "115200", "8192", "1024"):
        require(v in doc, f"normative ICD missing profile detail: {v}")
    require("No switch" in doc, "ICD must affirm no switch")
    print("PASS: EXN switchless Wi-Fi + dual UART + direct Ethernet profile validated")


if __name__ == "__main__":
    main()
