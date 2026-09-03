#!/usr/bin/env python3

from __future__ import annotations

import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPTS = (
    Path(__file__).resolve().parents[1]
    / "skills"
    / "board-diagnostics"
    / "scripts"
)
sys.path.insert(0, str(SCRIPTS))

import board_diagnostics_core as core  # noqa: E402
import board_diagnostics as cli  # noqa: E402


def sample_plan(title: str = "Initial checks") -> dict:
    return {
        "title": title,
        "summary": "Verify the first diagnostic split.",
        "instructions": ["Follow each section's power state."],
        "sides": [
            {
                "name": "Side A",
                "sections": [
                    {
                        "title": "Powered voltage",
                        "mode": "voltage",
                        "power_state": "connected",
                        "safety": "Use a current-limited supply.",
                        "measurements": [
                            {
                                "point": "TP_A",
                                "reference": "GND",
                                "expected": "5 V",
                                "location": "Upper left",
                                "why": "Separates input from downstream fault.",
                            }
                        ],
                    }
                ],
            },
            {
                "name": "Side B",
                "sections": [
                    {
                        "title": "Unpowered resistance",
                        "mode": "resistance",
                        "power_state": "disconnected",
                        "safety": "Disconnect every power source.",
                        "measurements": [
                            {
                                "point": "TP_B",
                                "reference": "GND",
                                "expected": "0-1 ohm",
                            }
                        ],
                    }
                ],
            },
        ],
    }


class BoardDiagnosticsTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.previous_data_dir = os.environ.get("BOARD_DIAGNOSTICS_DATA_DIR")
        os.environ["BOARD_DIAGNOSTICS_DATA_DIR"] = str(Path(self.temporary.name) / "data")
        self.workspace = Path(self.temporary.name) / "workspace"
        self.workspace.mkdir()

    def tearDown(self) -> None:
        if self.previous_data_dir is None:
            os.environ.pop("BOARD_DIAGNOSTICS_DATA_DIR", None)
        else:
            os.environ["BOARD_DIAGNOSTICS_DATA_DIR"] = self.previous_data_dir
        self.temporary.cleanup()

    def create(self, ticket: str = "1002") -> dict:
        case, _path = core.create_case(
            ticket,
            "Example board",
            "No power",
            sample_plan(),
            workspace=self.workspace,
        )
        return case

    def test_ticket_normalization_and_validation(self) -> None:
        self.assertEqual(core.normalize_ticket(" #ABC-123 "), "ABC-123")
        with self.assertRaises(core.DiagnosticError):
            core.normalize_ticket("../123")
        with self.assertRaises(core.DiagnosticError):
            core.normalize_ticket("")

    def test_rejects_powered_resistance(self) -> None:
        plan = sample_plan()
        section = plan["sides"][1]["sections"][0]
        section["power_state"] = "connected"
        with self.assertRaisesRegex(core.DiagnosticError, "resistance mode"):
            core.normalize_plan(plan, 1)

    def test_requires_both_board_sides(self) -> None:
        plan = sample_plan()
        plan["sides"][1]["name"] = "Rear"
        with self.assertRaisesRegex(core.DiagnosticError, "Side A and Side B"):
            core.normalize_plan(plan, 1)

    def test_case_lifecycle_preserves_rounds(self) -> None:
        case = self.create()
        round_data = core.active_round(case)
        for side in round_data["sides"]:
            for section in side["sections"]:
                for measurement in section["measurements"]:
                    core.update_measurement(
                        case,
                        round_data["id"],
                        side["id"],
                        section["id"],
                        measurement["id"],
                        actual=measurement["expected"],
                        status="pass",
                    )
        core.update_side_comments(case, round_data["id"], "side-a", "Stable reading")
        core.finish_active_round(case)

        saved = core.load_case("1002")
        self.assertEqual(saved["status"], "waiting_for_ai")
        self.assertEqual(core.round_progress(core.active_round(saved))["pass"], 2)

        saved, _path = core.add_round("1002", sample_plan("Follow-up checks"))
        self.assertEqual(len(saved["rounds"]), 2)
        self.assertEqual(core.active_round(saved)["number"], 2)
        self.assertEqual(saved["rounds"][0]["status"], "complete")

    def test_actual_defaults_to_expected_without_marking_tested(self) -> None:
        case = self.create()
        round_data = core.active_round(case)
        measurements = list(core.iter_measurements(round_data))
        self.assertTrue(measurements)
        self.assertTrue(all(item["actual"] == item["expected"] for item in measurements))
        self.assertEqual(core.round_progress(round_data)["measured"], 0)

    def test_boardview_target_is_inferred_from_component(self) -> None:
        plan = sample_plan()
        measurement = plan["sides"][0]["sections"][0]["measurements"][0]
        measurement["point"] = "R7009 inner pad"
        normalized = core.normalize_plan(plan, 1)
        first = next(core.iter_measurements(normalized))
        self.assertEqual(first["boardview"], "R7009")

    def test_failed_prerequisite_skips_and_restores_downstream_checks(self) -> None:
        plan = sample_plan()
        measurements = plan["sides"][0]["sections"][0]["measurements"]
        measurements[0]["key"] = "input_ok"
        measurements.append(
            {
                "key": "downstream",
                "point": "R7009",
                "reference": "GND",
                "expected": "3.3 V",
                "requires_pass": ["input_ok"],
            }
        )
        case, _path = core.create_case(
            "branching", "Example board", "No power", plan, workspace=self.workspace
        )
        round_data = core.active_round(case)
        by_key = core.measurements_by_key(round_data)
        self.assertEqual(core.dependency_state(round_data, by_key["downstream"]), "pending")

        source = by_key["input_ok"]
        core.update_measurement(
            case,
            round_data["id"],
            "side-a",
            round_data["sides"][0]["sections"][0]["id"],
            source["id"],
            status="fail",
        )
        self.assertEqual(by_key["downstream"]["status"], "skipped")
        self.assertTrue(by_key["downstream"]["auto_skipped"])
        self.assertEqual(core.round_progress(round_data)["skipped"], 1)

        core.update_measurement(
            case,
            round_data["id"],
            "side-a",
            round_data["sides"][0]["sections"][0]["id"],
            source["id"],
            status="pass",
        )
        self.assertEqual(by_key["downstream"]["status"], "untested")
        self.assertFalse(by_key["downstream"]["auto_skipped"])
        self.assertEqual(core.dependency_state(round_data, by_key["downstream"]), "ready")

    def test_rejects_unknown_and_cyclic_dependencies(self) -> None:
        plan = sample_plan()
        first = plan["sides"][0]["sections"][0]["measurements"][0]
        first["key"] = "first"
        first["requires_pass"] = ["missing"]
        with self.assertRaisesRegex(core.DiagnosticError, "unknown key"):
            core.normalize_plan(plan, 1)

        first["requires_pass"] = ["second"]
        plan["sides"][1]["sections"][0]["measurements"][0].update(
            {"key": "second", "requires_pass": ["first"]}
        )
        with self.assertRaisesRegex(core.DiagnosticError, "cycle"):
            core.normalize_plan(plan, 1)

    def test_can_replace_untouched_round_but_not_edited_round(self) -> None:
        case = self.create()
        replacement = sample_plan("Broader checks")
        saved, _path = core.replace_active_round("1002", replacement)
        self.assertEqual(core.active_round(saved)["title"], "Broader checks")
        self.assertEqual(len(saved["rounds"]), 1)

        round_data = core.active_round(saved)
        first = next(core.iter_measurements(round_data))
        first["actual"] = "unexpected"
        core.save_case(saved)
        with self.assertRaisesRegex(core.DiagnosticError, "edited readings"):
            core.replace_active_round("1002", sample_plan("Rejected"))

    def test_cannot_replace_or_append_to_unfinished_ticket(self) -> None:
        self.create()
        with self.assertRaisesRegex(core.DiagnosticError, "already exists"):
            self.create()
        with self.assertRaisesRegex(core.DiagnosticError, "unfinished active round"):
            core.add_round("1002", sample_plan("Too soon"))

    def test_results_and_natural_ticket_order(self) -> None:
        for ticket in ("10", "2", "A3"):
            core.create_case(
                ticket,
                "Example board",
                "No power",
                sample_plan(),
                workspace=self.workspace,
            )
        tickets = [item["ticket"] for item in core.list_cases()]
        self.assertEqual(tickets, ["2", "10", "A3"])
        report = core.format_results_markdown(core.load_case("2"))
        self.assertIn("# Ticket 2", report)
        self.assertIn("Side A", report)
        self.assertIn("○ UNTESTED", report)

    def test_open_reuses_matching_boardview_window(self) -> None:
        case = self.create()
        board_file = self.workspace / "example.brd"
        board_file.touch()

        with (
            mock.patch.object(cli, "wait_for_acknowledgement", return_value=True),
            mock.patch.object(cli.subprocess, "Popen") as popen,
        ):
            self.assertEqual(cli.launch_app(case["ticket"], "TP_A"), 0)

        popen.assert_not_called()
        request = json.loads(
            (core.data_root().parent / "open-request.json").read_text(encoding="utf-8")
        )
        self.assertRegex(request["request_id"], r"^[0-9a-f]{32}$")
        self.assertEqual(request["ticket"], case["ticket"])
        self.assertEqual(request["board_file"], str(board_file.resolve()))
        self.assertEqual(request["target"], "TP_A")

    def test_open_starts_boardview_when_no_window_acknowledges(self) -> None:
        case = self.create()
        board_file = self.workspace / "example.brd"
        board_file.touch()
        process = mock.Mock(pid=4321)
        process.poll.return_value = None

        with (
            mock.patch.object(cli, "wait_for_acknowledgement", side_effect=[False, True]),
            mock.patch.object(cli.subprocess, "Popen", return_value=process) as popen,
        ):
            self.assertEqual(cli.launch_app(case["ticket"], "TP_A"), 0)

        popen.assert_called_once()
        self.assertEqual(
            popen.call_args.args[0],
            [
                "openboardview",
                "-i",
                str(board_file.resolve()),
                "--ticket",
                case["ticket"],
                "--locate",
                "TP_A",
            ],
        )


if __name__ == "__main__":
    unittest.main()
