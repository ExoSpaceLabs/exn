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


def validate(root=ROOT):
    """Validate topology JSON and normative ICD consistency for the selected hardware."""
    profile = root / "interfaces" / "network" / "reference-topology.json"
    d = json.loads(profile.read_text(encoding="utf-8"))
    require(d["schema_version"] == 3, "expected switchless USB/CDC v3 topology schema")
    require(d["packet_profile"] == "CCSDS-Space-Packet/PUS-Revision-C/CRC16", "PUS profile drift")
    budget = d["packet_budget"]
    require(1024 <= budget["max_decoded_packet_bytes"] <= 65542, "invalid packet budget")
    require(0 < budget["ethernet_object_chunk_payload_bytes"] < budget["max_decoded_packet_bytes"],
            "invalid object chunk budget")

    links = {x["id"]: x for x in d["physical_links"]}
    require(set(links) == {"ground", "obc_pi_control", "obc_fpga_control", "pi_fpga_data"},
            "unexpected physical links (no switch or OBC Ethernet)")
    profiles = d.get("deployment_profiles")
    require(isinstance(profiles, dict) and set(profiles) == {"obc_pi_ground_mvp", "full_reference"},
            "missing initial UART-only and full reference deployment profiles")
    minimal, full = profiles["obc_pi_ground_mvp"], profiles["full_reference"]
    require(minimal["priority"] == "P0", "OBC+Pi Ground must be P0 milestone")
    require(set(minimal["required_links"]) == {"ground", "obc_pi_control"},
            "P0 must not require USB, FPGA or Ethernet links")
    require(set(full["required_links"]) == set(links), "full profile must retain selected link ICD")
    require(minimal["required_pi_transport_process"] == "exn-linkd"
            and full["required_pi_transport_process"] == "exn-linkd", "Pi UART packet router required")
    require(minimal["requires_das_usb_device_controller"] is False
            and minimal["requires_kv260"] is False,
            "USB or KV260 incorrectly gates OBC+Pi ground profile")
    require(full["requires_das_usb_device_controller"] is True
            and full["requires_kv260"] is True,
            "full FPGA profile lost mandatory USB/processor dependencies")
    for name, endpoints in {
        "ground": {"gs", "pi"},
        "obc_pi_control": {"obc", "pi"},
        "obc_fpga_control": {"obc", "fpga"},
        "pi_fpga_data": {"pi", "fpga"},
    }.items():
        require(set(links[name]["endpoints"]) == endpoints, f"wrong peers: {name}")
    pi_uart = links["obc_pi_control"]
    require(pi_uart["media"] == "3.3V-UART", "Pi control must use 3.3V UART")
    require(pi_uart["framing"] == "COBS-complete-CCSDS-packet-delimiter-00", "Pi UART framing drift")
    require(pi_uart["baud_initial"] == 115200 and pi_uart["baud_qualification"] == 921600,
            "Pi UART baud mismatch")
    require(pi_uart["uart_mode"] == "8N1", "Pi UART mode mismatch")
    require(pi_uart["peripherals"] == ["STM32:USART1", "Pi:UART0-GPIO14/15"],
            "STM32 UART peripheral / Pi UART identity drift")
    fpga_usb = links["obc_fpga_control"]
    require(fpga_usb["media"] == "USB2.0-FS-CDC-ACM", "FPGA control must be USB CDC-ACM")
    require(fpga_usb["usb_host"] == "fpga" and fpga_usb["usb_device"] == "obc",
            "USB host/device roles inverted")
    require(fpga_usb["device_class"] == "CDC-ACM" and fpga_usb["linux_driver"] == "cdc_acm",
            "USB CDC driver/profile mismatch")
    require(fpga_usb["framing"] == "COBS-complete-CCSDS-packet-delimiter-00",
            "USB CDC framing drift")
    require(fpga_usb["requires_pl_firmware"] is False, "USB CDC must be PL-independent")
    require(fpga_usb["max_usb_bus_bps"] == 12_000_000, "USB FS rate mismatch")
    require(fpga_usb["needs_separate_stm32_power"] is True, "missing Nucleo power warning")
    require(links["ground"]["media"] == "Wi-Fi", "Ground must use Wi-Fi")
    require(links["pi_fpga_data"]["media"] == "direct-Gigabit-Ethernet", "data link drift")
    require(links["pi_fpga_data"]["requires_switch"] is False, "Ethernet switch introduced")
    require(links["pi_fpga_data"]["framing"] == "raw-concatenated-CCSDS-packets",
            "Ethernet packet framing drift")

    hosts = {x["id"]: x for x in d["physical_hosts"]}
    require(set(hosts) == {"obc", "pi", "fpga"}, "expected three hardware hosts")
    require(hosts["obc"]["ethernet_used"] is False, "OBC must not use Ethernet")
    obc_uart = hosts["obc"]["uart_a"]
    require((obc_uart["tx_pin"], obc_uart["rx_pin"]) == ("PB6", "PB7"),
            "STM32 UART A pins drift")
    require(obc_uart["peripheral"] == "USART1" and obc_uart["alternate_function"] == "AF7",
            "STM32 UART peripheral/alternate function drift")
    require(obc_uart["das_board_resource"] == "DAS_BOARD_UART_ARDUINO",
            "selected UART no longer matches the released DAS board resource")
    usb_device = hosts["obc"]["usb_cdc_device"]
    require(usb_device["connector"] == "CN13 Micro-AB" and usb_device["role"] == "USB-device",
            "STM32 USB connector/role mismatch")
    require(usb_device["device_class"] == "CDC-ACM", "STM32 must expose CDC-ACM")
    require(usb_device["not_the_stlink_usb_port"] is True, "ST-LINK must not be used as USB endpoint")
    require((hosts["pi"]["header_uart"]["tx_header_pin"], hosts["pi"]["header_uart"]["rx_header_pin"])
            == (8, 10), "Pi UART header mapping drift")
    kv_usb = hosts["fpga"]["control_usb"]
    require(kv_usb["role"] == "USB-host" and kv_usb["controller"] == "PS USB host",
            "KV260 USB controller must be PS/host")
    require(kv_usb["connector"] == "U44 or U46 Type-A host", "KV260 connector mismatch")
    require(kv_usb["requires_pl_logic"] is False, "USB control must not consume PL")

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
    minimal_endpoints = set(minimal["required_endpoints"])
    full_endpoints = set(full["required_endpoints"])
    require(minimal_endpoints == {"OBC", "PI-COMMS"},
            "P0 must require only OBC and Antenna mission endpoints")
    require(full_endpoints == (set(expected) - {"GS"}),
            "full reference endpoint catalogue drift")
    require(set(minimal["optional_endpoints_not_required"]) == (full_endpoints - minimal_endpoints),
            "P0 optional endpoint catalogue drift")
    for deployment in (minimal, full):
        require(set(deployment["required_links"]).issubset(links), "profile requires unknown link")
        require(set(deployment["required_endpoints"]).issubset(expected),
                "profile requires unknown endpoint")
        require(not (set(deployment["optional_endpoints_not_required"])
                     & set(deployment["required_endpoints"])), "optional and required overlap")
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
    doc = (root / "docs" / "icd" / "inter-node-links.md").read_text(encoding="utf-8")
    for v in ("USART1", "AF7", "DAS_BOARD_UART_ARDUINO", "PB6", "PB7", "CN13", "Micro-AB", "USB CDC-ACM",
              "U44", "U46", "cdc_acm", "COBS", "10.77.0.0/30",
              "10.77.0.1", "10.77.0.2", "47101", "47102", "47103",
              "47000", "115200", "8192", "1024"):
        require(v in doc, f"normative ICD missing profile detail: {v}")
    require("No switch" in doc, "ICD must affirm no switch")
    require("obc_pi_ground_mvp" in doc and "full_reference" in doc,
            "normative link ICD missing staged deployment profiles")
    require("not registered" in doc and "Service 1" in doc,
            "normative ICD missing absent-vs-offline handling")
    # All published reference ICD and architecture descriptions must select
    # the same peripheral and AF as the machine-readable deployment profile.
    for relpath in ("docs/icd/mcu-rtos.md", "docs/architecture.md"):
        companion = (root / relpath).read_text(encoding="utf-8")
        require("USART1" in companion and "AF7" in companion
                and "PB6" in companion and "PB7" in companion,
                f"reference UART route not reflected in {relpath}")
    print("PASS: EXN Wi-Fi + USART1/AF7 Pi UART + KV260 USB CDC-ACM + direct Ethernet profile validated")


def main():
    validate(ROOT)


if __name__ == "__main__":
    main()
