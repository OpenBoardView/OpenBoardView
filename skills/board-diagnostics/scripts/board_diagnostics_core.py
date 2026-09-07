#!/usr/bin/env python3
"""Storage and validation for ticket-based board diagnostic checklists."""

from __future__ import annotations

import json
import os
import re
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterator


SCHEMA_VERSION = 1
TICKET_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$")
ALLOWED_MODES = {"voltage", "diode", "resistance", "continuity", "other"}
UNPOWERED_MODES = {"diode", "resistance", "continuity"}
UNPOWERED_STATES = {"disconnected", "unpowered", "off"}
RESULT_STATES = {"untested", "pass", "fail", "not_measurable", "skipped"}
MEASUREMENT_KEY_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$")
COMPONENT_RE = re.compile(r"\b[A-Za-z]{1,4}\d{2,5}\b")


class DiagnosticError(ValueError):
    """A user-correctable checklist or case error."""


def now_utc() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")


def normalize_ticket(raw_ticket: str) -> str:
    ticket = str(raw_ticket or "").strip()
    if ticket.startswith("#"):
        ticket = ticket[1:]
    if not TICKET_RE.fullmatch(ticket):
        raise DiagnosticError(
            "ticket must be 1-64 letters, numbers, dots, underscores, or hyphens "
            "(an optional leading # is accepted)"
        )
    return ticket


def ticket_slug(ticket: str) -> str:
    return normalize_ticket(ticket).casefold()


def data_root() -> Path:
    override = os.environ.get("BOARD_DIAGNOSTICS_DATA_DIR")
    if override:
        return Path(override).expanduser().resolve()
    xdg_data = os.environ.get("XDG_DATA_HOME")
    base = Path(xdg_data).expanduser() if xdg_data else Path.home() / ".local" / "share"
    return base / "board-diagnostics" / "tickets"


def case_path(ticket: str) -> Path:
    return data_root() / ticket_slug(ticket) / "case.json"


def _read_json(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise DiagnosticError(f"ticket not found: {path.parent.name}") from error
    except json.JSONDecodeError as error:
        raise DiagnosticError(f"invalid JSON in {path}: {error}") from error
    except OSError as error:
        raise DiagnosticError(f"unable to read {path}: {error}") from error
    if not isinstance(payload, dict):
        raise DiagnosticError(f"{path} must contain a JSON object")
    return payload


def read_plan(path: Path | str) -> dict[str, Any]:
    plan_path = Path(path).expanduser().resolve()
    return _read_json(plan_path)


def atomic_write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            dir=path.parent,
            prefix=f".{path.name}.",
            suffix=".tmp",
            delete=False,
        ) as handle:
            temporary_name = handle.name
            json.dump(payload, handle, indent=2, ensure_ascii=False)
            handle.write("\n")
            handle.flush()
            os.fsync(handle.fileno())
        os.chmod(temporary_name, 0o600)
        os.replace(temporary_name, path)
    except OSError as error:
        if temporary_name:
            try:
                Path(temporary_name).unlink(missing_ok=True)
            except OSError:
                pass
        raise DiagnosticError(f"unable to save {path}: {error}") from error


def _required_text(mapping: dict[str, Any], key: str, location: str) -> str:
    value = mapping.get(key)
    if not isinstance(value, str) or not value.strip():
        raise DiagnosticError(f"{location}.{key} must be a non-empty string")
    return value.strip()


def _optional_text(mapping: dict[str, Any], key: str, location: str) -> str:
    value = mapping.get(key, "")
    if value is None:
        return ""
    if not isinstance(value, str):
        raise DiagnosticError(f"{location}.{key} must be a string")
    return value.strip()


def _slug(value: str, fallback: str) -> str:
    slug = re.sub(r"[^a-z0-9]+", "-", value.casefold()).strip("-")
    return slug[:64] or fallback


def _unique_id(base: str, used: set[str]) -> str:
    candidate = base
    suffix = 2
    while candidate in used:
        candidate = f"{base}-{suffix}"
        suffix += 1
    used.add(candidate)
    return candidate


def normalize_plan(plan: dict[str, Any], round_number: int) -> dict[str, Any]:
    if not isinstance(plan, dict):
        raise DiagnosticError("plan must be a JSON object")

    title = _required_text(plan, "title", "plan")
    summary = _optional_text(plan, "summary", "plan")
    raw_instructions = plan.get("instructions", [])
    if not isinstance(raw_instructions, list) or not all(
        isinstance(item, str) and item.strip() for item in raw_instructions
    ):
        raise DiagnosticError("plan.instructions must be an array of non-empty strings")

    raw_sides = plan.get("sides")
    if not isinstance(raw_sides, list) or len(raw_sides) < 2:
        raise DiagnosticError("plan.sides must contain at least Side A and Side B")

    normalized_sides: list[dict[str, Any]] = []
    used_side_ids: set[str] = set()
    used_measurement_keys: set[str] = set()
    dependency_records: list[tuple[str, str, list[str]]] = []
    side_names: list[str] = []
    measurement_total = 0

    for side_index, raw_side in enumerate(raw_sides):
        location = f"plan.sides[{side_index}]"
        if not isinstance(raw_side, dict):
            raise DiagnosticError(f"{location} must be an object")
        name = _required_text(raw_side, "name", location)
        side_names.append(name.casefold())
        side_id = _unique_id(_slug(name, f"side-{side_index + 1}"), used_side_ids)
        raw_sections = raw_side.get("sections", [])
        if not isinstance(raw_sections, list):
            raise DiagnosticError(f"{location}.sections must be an array")

        sections: list[dict[str, Any]] = []
        used_section_ids: set[str] = set()
        for section_index, raw_section in enumerate(raw_sections):
            section_location = f"{location}.sections[{section_index}]"
            if not isinstance(raw_section, dict):
                raise DiagnosticError(f"{section_location} must be an object")
            section_title = _required_text(raw_section, "title", section_location)
            mode = _required_text(raw_section, "mode", section_location).casefold()
            if mode not in ALLOWED_MODES:
                allowed = ", ".join(sorted(ALLOWED_MODES))
                raise DiagnosticError(f"{section_location}.mode must be one of: {allowed}")
            power_state = _required_text(raw_section, "power_state", section_location).casefold()
            if mode in UNPOWERED_MODES and power_state not in UNPOWERED_STATES:
                raise DiagnosticError(
                    f"{section_location} uses {mode} mode but power_state is {power_state!r}; "
                    "use disconnected, unpowered, or off"
                )
            safety = _optional_text(raw_section, "safety", section_location)
            raw_measurements = raw_section.get("measurements", [])
            if not isinstance(raw_measurements, list):
                raise DiagnosticError(f"{section_location}.measurements must be an array")

            measurements: list[dict[str, Any]] = []
            used_measurement_ids: set[str] = set()
            for measurement_index, raw_measurement in enumerate(raw_measurements):
                measurement_location = (
                    f"{section_location}.measurements[{measurement_index}]"
                )
                if not isinstance(raw_measurement, dict):
                    raise DiagnosticError(f"{measurement_location} must be an object")
                point = _required_text(raw_measurement, "point", measurement_location)
                reference = _required_text(raw_measurement, "reference", measurement_location)
                expected = _required_text(raw_measurement, "expected", measurement_location)
                raw_key = _optional_text(raw_measurement, "key", measurement_location)
                if raw_key:
                    if not MEASUREMENT_KEY_RE.fullmatch(raw_key):
                        raise DiagnosticError(
                            f"{measurement_location}.key must be 1-64 letters, numbers, "
                            "dots, underscores, or hyphens"
                        )
                    if raw_key in used_measurement_keys:
                        raise DiagnosticError(f"duplicate measurement key: {raw_key}")
                    used_measurement_keys.add(raw_key)
                    measurement_key = raw_key
                else:
                    measurement_key = _unique_id(
                        _slug(
                            f"{section_title}-{point}-{reference}",
                            f"measurement-{measurement_total + 1}",
                        ),
                        used_measurement_keys,
                    )

                raw_requires = raw_measurement.get("requires_pass", [])
                if not isinstance(raw_requires, list) or not all(
                    isinstance(item, str) and item.strip() for item in raw_requires
                ):
                    raise DiagnosticError(
                        f"{measurement_location}.requires_pass must be an array of "
                        "non-empty measurement keys"
                    )
                requires_pass = [item.strip() for item in raw_requires]
                if len(requires_pass) != len(set(requires_pass)):
                    raise DiagnosticError(
                        f"{measurement_location}.requires_pass contains duplicate keys"
                    )

                boardview = _optional_text(raw_measurement, "boardview", measurement_location)
                if not boardview:
                    component_match = COMPONENT_RE.search(point)
                    boardview = component_match.group(0) if component_match else ""
                measurement_id = _unique_id(
                    _slug(f"{point}-{reference}", f"measurement-{measurement_index + 1}"),
                    used_measurement_ids,
                )
                measurements.append(
                    {
                        "id": measurement_id,
                        "key": measurement_key,
                        "point": point,
                        "reference": reference,
                        "expected": expected,
                        "probe": _optional_text(raw_measurement, "probe", measurement_location),
                        "location": _optional_text(raw_measurement, "location", measurement_location),
                        "why": _optional_text(raw_measurement, "why", measurement_location),
                        "boardview": boardview,
                        "requires_pass": requires_pass,
                        # Seed the editable field with the expected result. The row remains
                        # untested until the technician explicitly marks its status.
                        "actual": expected,
                        "status": "untested",
                        "auto_skipped": False,
                        "note": "",
                    }
                )
                dependency_records.append(
                    (measurement_location, measurement_key, requires_pass)
                )
                measurement_total += 1

            section_id = _unique_id(
                _slug(section_title, f"section-{section_index + 1}"), used_section_ids
            )
            sections.append(
                {
                    "id": section_id,
                    "title": section_title,
                    "mode": mode,
                    "power_state": power_state,
                    "safety": safety,
                    "measurements": measurements,
                }
            )

        normalized_sides.append(
            {
                "id": side_id,
                "name": name,
                "comments": "",
                "sections": sections,
            }
        )

    if not any(name.startswith("side a") for name in side_names) or not any(
        name.startswith("side b") for name in side_names
    ):
        raise DiagnosticError("plan must include tabs named Side A and Side B")
    if measurement_total == 0:
        raise DiagnosticError("plan must contain at least one measurement")

    dependency_graph: dict[str, list[str]] = {}
    for location, measurement_key, requires_pass in dependency_records:
        for required_key in requires_pass:
            if required_key not in used_measurement_keys:
                raise DiagnosticError(
                    f"{location}.requires_pass references unknown key: {required_key}"
                )
            if required_key == measurement_key:
                raise DiagnosticError(f"{location} cannot depend on itself")
        dependency_graph[measurement_key] = requires_pass

    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(measurement_key: str) -> None:
        if measurement_key in visiting:
            raise DiagnosticError("measurement dependencies contain a cycle")
        if measurement_key in visited:
            return
        visiting.add(measurement_key)
        for required_key in dependency_graph.get(measurement_key, []):
            visit(required_key)
        visiting.remove(measurement_key)
        visited.add(measurement_key)

    for measurement_key in dependency_graph:
        visit(measurement_key)

    timestamp = now_utc()
    return {
        "id": f"round-{round_number}",
        "number": round_number,
        "title": title,
        "summary": summary,
        "instructions": [item.strip() for item in raw_instructions],
        "status": "in_progress",
        "created_at": timestamp,
        "updated_at": timestamp,
        "completed_at": None,
        "sides": normalized_sides,
    }


def create_case(
    ticket: str,
    board: str,
    issue: str,
    plan: dict[str, Any],
    *,
    workspace: Path | str,
    source_plan: Path | str | None = None,
) -> tuple[dict[str, Any], Path]:
    clean_ticket = normalize_ticket(ticket)
    path = case_path(clean_ticket)
    if path.exists():
        raise DiagnosticError(
            f"ticket {clean_ticket} already exists; add a follow-up round instead of replacing it"
        )
    board_name = str(board or "").strip()
    issue_text = str(issue or "").strip()
    if not board_name:
        raise DiagnosticError("board must be a non-empty string")
    if not issue_text:
        raise DiagnosticError("issue must be a non-empty string")
    workspace_path = Path(workspace).expanduser().resolve()
    if not workspace_path.is_dir():
        raise DiagnosticError(f"workspace is not a directory: {workspace_path}")

    first_round = normalize_plan(plan, 1)
    timestamp = now_utc()
    case: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "ticket": clean_ticket,
        "board": board_name,
        "issue": issue_text,
        "status": "in_progress",
        "source_workspace": str(workspace_path),
        "source_plan": str(Path(source_plan).expanduser().resolve()) if source_plan else "",
        "created_at": timestamp,
        "updated_at": timestamp,
        "active_round_id": first_round["id"],
        "rounds": [first_round],
    }
    atomic_write_json(path, case)
    return case, path


def load_case(ticket: str) -> dict[str, Any]:
    clean_ticket = normalize_ticket(ticket)
    path = case_path(clean_ticket)
    case = _read_json(path)
    if case.get("schema_version") != SCHEMA_VERSION:
        raise DiagnosticError(
            f"ticket {clean_ticket} uses unsupported schema version {case.get('schema_version')!r}"
        )
    if str(case.get("ticket", "")).casefold() != clean_ticket.casefold():
        raise DiagnosticError(f"ticket data mismatch in {path}")
    if not isinstance(case.get("rounds"), list):
        raise DiagnosticError(f"ticket {clean_ticket} has an invalid rounds list")
    return case


def save_case(case: dict[str, Any]) -> Path:
    ticket = normalize_ticket(str(case.get("ticket", "")))
    case["updated_at"] = now_utc()
    path = case_path(ticket)
    atomic_write_json(path, case)
    return path


def active_round(case: dict[str, Any]) -> dict[str, Any]:
    active_id = case.get("active_round_id")
    for round_data in case.get("rounds", []):
        if isinstance(round_data, dict) and round_data.get("id") == active_id:
            return round_data
    raise DiagnosticError(f"ticket {case.get('ticket', '?')} has no active round")


def add_round(
    ticket: str,
    plan: dict[str, Any],
    *,
    source_plan: Path | str | None = None,
) -> tuple[dict[str, Any], Path]:
    case = load_case(ticket)
    current = active_round(case)
    if current.get("status") != "complete":
        raise DiagnosticError(
            f"ticket {case['ticket']} still has an unfinished active round; finish it before adding another"
        )
    round_number = len(case["rounds"]) + 1
    next_round = normalize_plan(plan, round_number)
    case["rounds"].append(next_round)
    case["active_round_id"] = next_round["id"]
    case["status"] = "in_progress"
    if source_plan:
        case["source_plan"] = str(Path(source_plan).expanduser().resolve())
    return case, save_case(case)


def replace_active_round(
    ticket: str,
    plan: dict[str, Any],
    *,
    source_plan: Path | str | None = None,
) -> tuple[dict[str, Any], Path]:
    """Replace an unfinished round that has no recorded technician decisions."""
    case = load_case(ticket)
    current = active_round(case)
    if current.get("status") == "complete":
        raise DiagnosticError("cannot replace a completed round")

    for side in current.get("sides", []):
        if str(side.get("comments", "")).strip():
            raise DiagnosticError("cannot replace a round with recorded comments")
        for section in side.get("sections", []):
            for measurement in section.get("measurements", []):
                if measurement.get("status", "untested") != "untested":
                    raise DiagnosticError("cannot replace a round with recorded results")
                actual = str(measurement.get("actual", "")).strip()
                expected = str(measurement.get("expected", "")).strip()
                if actual and actual != expected:
                    raise DiagnosticError("cannot replace a round with edited readings")
                if str(measurement.get("note", "")).strip():
                    raise DiagnosticError("cannot replace a round with recorded notes")

    replacement = normalize_plan(plan, int(current.get("number", 1)))
    for index, round_data in enumerate(case.get("rounds", [])):
        if round_data.get("id") == current.get("id"):
            case["rounds"][index] = replacement
            break
    else:
        raise DiagnosticError("active round was not found in the ticket")

    case["active_round_id"] = replacement["id"]
    case["status"] = "in_progress"
    if source_plan:
        case["source_plan"] = str(Path(source_plan).expanduser().resolve())
    return case, save_case(case)


def iter_measurements(round_data: dict[str, Any]) -> Iterator[dict[str, Any]]:
    for side in round_data.get("sides", []):
        for section in side.get("sections", []):
            yield from section.get("measurements", [])


def measurements_by_key(round_data: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {
        str(measurement.get("key", measurement.get("id", ""))): measurement
        for measurement in iter_measurements(round_data)
    }


def dependency_state(round_data: dict[str, Any], measurement: dict[str, Any]) -> str:
    """Return ready, pending, or skipped for a conditional measurement."""
    required_keys = measurement.get("requires_pass", [])
    if not required_keys:
        return "ready"
    by_key = measurements_by_key(round_data)
    statuses = [by_key[key].get("status", "untested") for key in required_keys if key in by_key]
    if len(statuses) != len(required_keys):
        return "pending"
    if any(status in {"fail", "not_measurable", "skipped"} for status in statuses):
        return "skipped"
    if all(status == "pass" for status in statuses):
        return "ready"
    return "pending"


def apply_dependencies(round_data: dict[str, Any]) -> bool:
    """Propagate automatic skips without overwriting measurements already performed."""
    changed = False
    made_change = True
    while made_change:
        made_change = False
        for measurement in iter_measurements(round_data):
            state = dependency_state(round_data, measurement)
            status = measurement.get("status", "untested")
            auto_skipped = bool(measurement.get("auto_skipped", False))
            if state == "skipped" and status in {"untested", "skipped"}:
                if status != "skipped" or not auto_skipped:
                    measurement["status"] = "skipped"
                    measurement["auto_skipped"] = True
                    changed = made_change = True
            elif state != "skipped" and auto_skipped:
                measurement["status"] = "untested"
                measurement["auto_skipped"] = False
                changed = made_change = True
    return changed


def round_progress(round_data: dict[str, Any]) -> dict[str, int]:
    counts = {state: 0 for state in RESULT_STATES}
    for measurement in iter_measurements(round_data):
        status = measurement.get("status", "untested")
        counts[status if status in RESULT_STATES else "untested"] += 1
    counts["total"] = sum(counts.values())
    counts["measured"] = counts["total"] - counts["untested"]
    return counts


def _mark_edited(case: dict[str, Any], round_data: dict[str, Any]) -> None:
    timestamp = now_utc()
    round_data["updated_at"] = timestamp
    if round_data.get("status") == "complete":
        round_data["status"] = "in_progress"
        round_data["completed_at"] = None
        case["status"] = "in_progress"


def update_measurement(
    case: dict[str, Any],
    round_id: str,
    side_id: str,
    section_id: str,
    measurement_id: str,
    *,
    actual: str | None = None,
    status: str | None = None,
    note: str | None = None,
) -> Path:
    if status is not None and status not in RESULT_STATES:
        raise DiagnosticError(f"invalid measurement status: {status}")
    for round_data in case.get("rounds", []):
        if round_data.get("id") != round_id:
            continue
        for side in round_data.get("sides", []):
            if side.get("id") != side_id:
                continue
            for section in side.get("sections", []):
                if section.get("id") != section_id:
                    continue
                for measurement in section.get("measurements", []):
                    if measurement.get("id") != measurement_id:
                        continue
                    if actual is not None:
                        measurement["actual"] = str(actual).strip()
                    if status is not None:
                        measurement["status"] = status
                        measurement["auto_skipped"] = False
                    if note is not None:
                        measurement["note"] = str(note).strip()
                    apply_dependencies(round_data)
                    _mark_edited(case, round_data)
                    return save_case(case)
    raise DiagnosticError(f"measurement not found: {measurement_id}")


def update_side_comments(
    case: dict[str, Any], round_id: str, side_id: str, comments: str
) -> Path:
    for round_data in case.get("rounds", []):
        if round_data.get("id") != round_id:
            continue
        for side in round_data.get("sides", []):
            if side.get("id") == side_id:
                side["comments"] = str(comments).strip()
                _mark_edited(case, round_data)
                return save_case(case)
    raise DiagnosticError(f"side not found: {side_id}")


def finish_active_round(case: dict[str, Any]) -> Path:
    round_data = active_round(case)
    timestamp = now_utc()
    round_data["status"] = "complete"
    round_data["updated_at"] = timestamp
    round_data["completed_at"] = timestamp
    case["status"] = "waiting_for_ai"
    return save_case(case)


def _natural_key(value: str) -> list[Any]:
    return [int(part) if part.isdigit() else part.casefold() for part in re.split(r"(\d+)", value)]


def list_cases() -> list[dict[str, Any]]:
    cases: list[dict[str, Any]] = []
    root = data_root()
    if not root.exists():
        return cases
    for path in root.glob("*/case.json"):
        try:
            case = _read_json(path)
            latest = active_round(case)
            progress = round_progress(latest)
            cases.append(
                {
                    "ticket": str(case.get("ticket", path.parent.name)),
                    "board": str(case.get("board", "")),
                    "issue": str(case.get("issue", "")),
                    "status": str(case.get("status", "unknown")),
                    "round": latest.get("number"),
                    "measured": progress["measured"],
                    "total": progress["total"],
                    "updated_at": case.get("updated_at"),
                    "path": str(path),
                }
            )
        except DiagnosticError:
            continue
    return sorted(cases, key=lambda item: _natural_key(item["ticket"]))


def format_results_markdown(case: dict[str, Any]) -> str:
    lines = [
        f"# Ticket {case['ticket']} — {case['board']}",
        "",
        f"Issue: {case['issue']}",
        f"Case status: {case['status']}",
    ]
    status_labels = {
        "untested": "○ UNTESTED",
        "pass": "✓ RIGHT",
        "fail": "✕ WRONG",
        "not_measurable": "— NOT MEASURABLE",
        "skipped": "↷ SKIPPED",
    }
    for round_data in case.get("rounds", []):
        progress = round_progress(round_data)
        lines.extend(
            [
                "",
                f"## Round {round_data.get('number')} — {round_data.get('title')}",
                "",
                f"Round status: {round_data.get('status')} · "
                f"{progress['measured']}/{progress['total']} checked · "
                f"{progress['fail']} wrong · {progress['not_measurable']} not measurable · "
                f"{progress['skipped']} skipped",
            ]
        )
        summary = str(round_data.get("summary", "")).strip()
        if summary:
            lines.extend(["", summary])
        for side in round_data.get("sides", []):
            lines.extend(["", f"### {side.get('name')}"])
            for section in side.get("sections", []):
                lines.extend(
                    [
                        "",
                        f"#### {section.get('title')} "
                        f"[{section.get('mode')} · power {section.get('power_state')}]",
                    ]
                )
                for measurement in section.get("measurements", []):
                    status = measurement.get("status", "untested")
                    label = status_labels.get(status, status.upper())
                    actual = str(measurement.get("actual", "")).strip() or "—"
                    note = str(measurement.get("note", "")).strip()
                    detail = (
                        f"- {label} · {measurement.get('point')} → "
                        f"{measurement.get('reference')} · expected {measurement.get('expected')} · "
                        f"actual {actual}"
                    )
                    if note:
                        detail += f" · note: {note}"
                    lines.append(detail)
            comments = str(side.get("comments", "")).strip()
            if comments:
                lines.extend(["", f"Comments: {comments}"])
    return "\n".join(lines) + "\n"
