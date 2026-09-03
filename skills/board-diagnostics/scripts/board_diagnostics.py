#!/usr/bin/env python3
"""Command-line bridge between AI assistants and OpenBoardView diagnostics."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
import uuid
from pathlib import Path

import board_diagnostics_core as core


BOARD_EXTENSIONS = {".brd", ".bdv", ".bvr", ".fz", ".asc", ".bom", ".cad", ".cae", ".cst", ".pcb"}


def board_files(case: dict) -> list[Path]:
    workspace = Path(str(case.get("source_workspace", ""))).expanduser()
    if not workspace.is_dir():
        return []
    return sorted(
        path.resolve()
        for path in workspace.rglob("*")
        if path.is_file() and path.suffix.casefold() in BOARD_EXTENSIONS
    )


def write_open_request(case: dict, board_file: Path, target: str = "") -> tuple[Path, str]:
    request_path = core.data_root().parent / "open-request.json"
    request_id = uuid.uuid4().hex
    core.atomic_write_json(
        request_path,
        {
            "request_id": request_id,
            "ticket": case["ticket"],
            "board_file": str(board_file),
            "target": str(target).strip(),
            "requested_at": core.now_utc(),
        },
    )
    return request_path, request_id


def request_was_handled(request_id: str) -> bool:
    acknowledgement = core.data_root().parent / "open-request-ack.json"
    try:
        payload = json.loads(acknowledgement.read_text(encoding="utf-8"))
    except (FileNotFoundError, OSError, json.JSONDecodeError):
        return False
    return isinstance(payload, dict) and payload.get("request_id") == request_id


def wait_for_acknowledgement(request_id: str, timeout: float) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if request_was_handled(request_id):
            return True
        time.sleep(0.05)
    return request_was_handled(request_id)


def add_common_ticket(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--ticket", required=True, help="Case ticket number, with optional leading #")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="board-diagnostics",
        description="Create, open, and read ticket-based motherboard diagnostic checklists.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    create = subparsers.add_parser("create", help="Create the first checklist round for a ticket")
    add_common_ticket(create)
    create.add_argument("--board", required=True, help="Board/device identifier")
    create.add_argument("--issue", required=True, help="Initial reported fault")
    create.add_argument("--plan", required=True, type=Path, help="Checklist plan JSON")
    create.add_argument("--workspace", type=Path, default=Path.cwd(), help="Source case folder")
    create.add_argument("--open", action="store_true", help="Open OpenBoardView after creation")

    follow_up = subparsers.add_parser("add-round", help="Append a follow-up checklist to a ticket")
    add_common_ticket(follow_up)
    follow_up.add_argument("--plan", required=True, type=Path, help="Checklist plan JSON")
    follow_up.add_argument("--open", action="store_true", help="Open OpenBoardView after appending")

    replace = subparsers.add_parser(
        "replace-round", help="Replace an untouched unfinished checklist round"
    )
    add_common_ticket(replace)
    replace.add_argument("--plan", required=True, type=Path, help="Replacement checklist plan JSON")
    replace.add_argument("--open", action="store_true", help="Open OpenBoardView after replacement")

    open_parser = subparsers.add_parser("open", help="Open a ticket in OpenBoardView")
    add_common_ticket(open_parser)
    open_parser.add_argument("--target", default="", help="Component or net to show on the board")

    results = subparsers.add_parser("results", help="Read all measurements for a ticket")
    add_common_ticket(results)
    results.add_argument("--json", action="store_true", help="Print raw case JSON")

    status = subparsers.add_parser("status", help="Show current progress for a ticket")
    add_common_ticket(status)
    status.add_argument("--json", action="store_true", help="Print machine-readable progress")

    list_parser = subparsers.add_parser("list", help="List projects in natural ticket order")
    list_parser.add_argument("--json", action="store_true", help="Print machine-readable case list")

    validate = subparsers.add_parser("validate-plan", help="Validate and normalize a plan without saving")
    validate.add_argument("--plan", required=True, type=Path, help="Checklist plan JSON")
    validate.add_argument("--round", type=int, default=1, help="Round number used for generated IDs")

    return parser


def launch_app(ticket: str, target: str = "") -> int:
    clean_ticket = core.normalize_ticket(ticket)
    case = core.load_case(clean_ticket)
    files = board_files(case)
    if not files:
        raise core.DiagnosticError(
            f"no supported boardview file found under {case.get('source_workspace', '')}"
        )

    board_file = files[0]
    _request_path, request_id = write_open_request(case, board_file, target)
    if wait_for_acknowledgement(request_id, 0.75):
        print(f"Opened ticket {clean_ticket} in the existing OpenBoardView window")
        return 0

    binary = os.environ.get("BOARD_DIAGNOSTICS_BOARDVIEW", "openboardview")
    command = [binary, "-i", str(board_file), "--ticket", clean_ticket]
    if target.strip():
        command.extend(["--locate", target.strip()])
    try:
        process = subprocess.Popen(
            command,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            start_new_session=True,
            close_fds=True,
        )
    except OSError as error:
        raise core.DiagnosticError(f"unable to start OpenBoardView: {error}") from error

    for _attempt in range(25):
        if wait_for_acknowledgement(request_id, 0.2):
            break
        exit_code = process.poll()
        if exit_code is not None:
            raise core.DiagnosticError(f"OpenBoardView exited during startup with code {exit_code}")

    print(f"Opened ticket {clean_ticket} in OpenBoardView (pid {process.pid})")
    return 0


def command_create(args: argparse.Namespace) -> int:
    plan = core.read_plan(args.plan)
    case, path = core.create_case(
        args.ticket,
        args.board,
        args.issue,
        plan,
        workspace=args.workspace,
        source_plan=args.plan,
    )
    print(f"Created ticket {case['ticket']} with round 1")
    print(f"Case file: {path}")
    if args.open:
        return launch_app(case["ticket"])
    return 0


def command_add_round(args: argparse.Namespace) -> int:
    plan = core.read_plan(args.plan)
    case, path = core.add_round(args.ticket, plan, source_plan=args.plan)
    round_data = core.active_round(case)
    print(f"Added round {round_data['number']} to ticket {case['ticket']}")
    print(f"Case file: {path}")
    if args.open:
        return launch_app(case["ticket"])
    return 0


def command_replace_round(args: argparse.Namespace) -> int:
    plan = core.read_plan(args.plan)
    case, path = core.replace_active_round(args.ticket, plan, source_plan=args.plan)
    round_data = core.active_round(case)
    print(f"Replaced round {round_data['number']} for ticket {case['ticket']}")
    print(f"Case file: {path}")
    if args.open:
        return launch_app(case["ticket"])
    return 0


def command_results(args: argparse.Namespace) -> int:
    case = core.load_case(args.ticket)
    if args.json:
        print(json.dumps(case, indent=2, ensure_ascii=False))
    else:
        print(core.format_results_markdown(case), end="")
    return 0


def status_payload(case: dict) -> dict:
    round_data = core.active_round(case)
    return {
        "ticket": case["ticket"],
        "case_status": case["status"],
        "round": round_data["number"],
        "round_title": round_data["title"],
        "round_status": round_data["status"],
        **core.round_progress(round_data),
    }


def command_status(args: argparse.Namespace) -> int:
    payload = status_payload(core.load_case(args.ticket))
    if args.json:
        print(json.dumps(payload, indent=2))
    else:
        print(
            f"Ticket {payload['ticket']} · round {payload['round']} · {payload['case_status']} · "
            f"{payload['measured']}/{payload['total']} checked · {payload['fail']} wrong · "
            f"{payload['not_measurable']} not measurable · {payload['skipped']} skipped"
        )
    return 0


def command_list(args: argparse.Namespace) -> int:
    cases = core.list_cases()
    if args.json:
        print(json.dumps(cases, indent=2, ensure_ascii=False))
        return 0
    if not cases:
        print("No board diagnostic tickets yet.")
        return 0
    for case in cases:
        print(
            f"{case['ticket']}\t{case['status']}\t{case['measured']}/{case['total']}\t"
            f"{case['board']}\t{case['issue']}"
        )
    return 0


def command_validate(args: argparse.Namespace) -> int:
    normalized = core.normalize_plan(core.read_plan(args.plan), args.round)
    print(json.dumps(normalized, indent=2, ensure_ascii=False))
    return 0


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    handlers = {
        "create": command_create,
        "add-round": command_add_round,
        "replace-round": command_replace_round,
        "open": lambda parsed: launch_app(parsed.ticket, parsed.target),
        "results": command_results,
        "status": command_status,
        "list": command_list,
        "validate-plan": command_validate,
    }
    try:
        return handlers[args.command](args)
    except core.DiagnosticError as error:
        print(f"board-diagnostics: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
