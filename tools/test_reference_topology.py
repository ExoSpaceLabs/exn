#!/usr/bin/env python3
"""Regression checks for the EXN reference UART pin/peripheral selection.

Tests intentionally mutate one part of a temporary topology to demonstrate
that the cross-document validator rejects drift. Only Python stdlib is used.
"""
import contextlib
import io
import json
import shutil
import tempfile
import unittest
from pathlib import Path

from validate_reference_topology import ROOT, validate


class ReferenceUARTMappingTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for rel in (
            "interfaces/network/reference-topology.json",
            "docs/icd/inter-node-links.md",
            "docs/icd/mcu-rtos.md",
            "docs/architecture.md",
        ):
            path = self.root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / rel, path)

    def check_valid(self):
        with contextlib.redirect_stdout(io.StringIO()):
            validate(self.root)

    def test_reference_contract(self):
        self.check_valid()

    def test_reject_unqualified_uart_peripheral_in_json(self):
        file = self.root / "interfaces/network/reference-topology.json"
        profile = json.loads(file.read_text(encoding="utf-8"))
        profile["physical_hosts"][0]["uart_a"]["peripheral"] = "LPUART1"
        file.write_text(json.dumps(profile), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "peripheral/alternate function"):
            self.check_valid()

    def test_reject_link_peripheral_drift(self):
        file = self.root / "interfaces/network/reference-topology.json"
        profile = json.loads(file.read_text(encoding="utf-8"))
        for link in profile["physical_links"]:
            if link["id"] == "obc_pi_control":
                link["peripherals"][0] = "STM32:LPUART1"
        file.write_text(json.dumps(profile), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "UART peripheral"):
            self.check_valid()

    def test_reject_documentation_drift(self):
        file = self.root / "docs/icd/mcu-rtos.md"
        content = file.read_text(encoding="utf-8")
        self.assertIn("USART1", content)
        file.write_text(content.replace("USART1", "LPUART1"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "reference UART route"):
            self.check_valid()

    def test_reject_af_drift(self):
        file = self.root / "interfaces/network/reference-topology.json"
        profile = json.loads(file.read_text(encoding="utf-8"))
        profile["physical_hosts"][0]["uart_a"]["alternate_function"] = "AF8"
        file.write_text(json.dumps(profile), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "peripheral/alternate function"):
            self.check_valid()


if __name__ == "__main__":
    unittest.main()
