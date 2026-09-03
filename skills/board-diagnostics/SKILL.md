---
name: board-diagnostics
description: Create, open, and review ticket-based motherboard diagnostic checklists inside OpenBoardView. Use when the user gives a board fault and wants a measurement plan, refers to a repair ticket, says board checks are done, or wants the next diagnostic round.
license: MIT
---

# Board Diagnostics

Turn a board-repair conversation into substantial, self-contained measurement rounds instead of conversational micro-steps.

## Local tool

Resolve `scripts/board_diagnostics.py` relative to this `SKILL.md`. Run it with Python 3. It stores durable cases under `~/.local/share/board-diagnostics/tickets/<ticket>/case.json`. Opening a ticket loads the matching board file, shows the Diagnostics pane, and raises an existing matching OpenBoardView window instead of starting a duplicate.

Read [the checklist plan schema](references/checklist-plan.md) before creating or appending a measurement round.

## Start a case

1. Require a ticket number, board identity, and initial fault. The ticket number is the project key. If it is missing, ask for it in one concise question; collect any other genuinely required missing fact in that same question.
2. Inspect the available schematic, boardview, prior notes, and other case artifacts before choosing test points. Reuse the user's established board orientation when available.
3. Build one useful first-pass round containing every currently justified check that can materially narrow the fault. Let diagnostic value determine the length: do not target a fixed count, pad the round with filler, or omit useful checks merely to keep it short. Organize it by physical side, then by meter mode and power state, to minimize board flips, power cycling, and meter changes.
   When existing evidence strongly supports one diagnosis, prefer the smallest decisive confirmation set over a broad exploratory round.
4. Give each decision-point measurement a stable `key`. Add `requires_pass` to downstream checks only when a failed/not-measurable prerequisite genuinely makes those checks unnecessary. The app will hold dependent rows until prerequisites pass and automatically mark them skipped when a prerequisite fails.
5. Keep the top-of-window summary to one concise sentence, or at most two. Do not repeat the full fault history there. Omit global `instructions` unless one short instruction is essential and cannot live in a section's safety or probe text.
6. Write the plan JSON in the case workspace at `.board-diagnostics/plans/<ticket>-round-<n>.json`. Preserve this source plan as part of the ticket record.
7. Run `validate-plan`, then `create --open` for round 1 or `add-round --open` for a later round. Fix validation errors before opening the app. If the user rejects an untouched unfinished round, use `replace-round --open`; it refuses to discard recorded readings, statuses, notes, or comments.
8. Ensure the ticket is visible in OpenBoardView before responding. If that board is already open, reuse and focus its window instead of launching another instance. Tell the user briefly that the whole round is ready and ask them to say they are done with the ticket after pressing **Finish round**.

Do not interrupt a round with conversational micro-steps. If new physical measurements are needed after review, create the complete next batch as another round and open it.

## Window handoff

Whenever ticket work leaves a checklist ready for the user, opening or focusing OpenBoardView must be the final task action before responding. Use the CLI's `--open` option when creating, adding, or replacing a round, or run `open --ticket <ticket>` when no write is needed. Use `open --ticket <ticket> --target <component-or-net>` when a specific point should be shown immediately. Do not give the user deep links, file links, or manual-opening instructions unless they explicitly ask how to open it themselves.

## Measurement quality and safety

- Never invent a rail, component reference, board location, expected value, or probe polarity. Ground each check in the available board/schematic evidence or clearly label the expectation as an investigative comparison.
- Include units and a meaningful expected value or range. State probe polarity for diode measurements and any non-obvious reference point.
- Keep `expected` compact because it seeds the editable Actual field; prefer a nominal such as `3.3 V` and put tolerances or comparison rules in `why`.
- Each row gets a Boardview button. The app normally infers its search target from the component designator in `point`; set `boardview` explicitly when the row should jump to a different component or net.
- Voltage checks may specify connected power when justified. Diode, resistance, and continuity sections must be unpowered: disconnect charger and battery and allow rails to discharge before measuring.
- Prefer checks with high diagnostic value and low risk. Cover the complete currently justified decision tree in one batch, including comparison points when they distinguish a local fault from a shared path. Avoid repeating readings already established unless they are needed as a live baseline for the new batch.
- Use Side A and Side B tabs even when one side has no checks in a particular round.

## Review completed work

When the user says they are done:

1. Use the ticket from the active conversation. If it is not unambiguous, ask only for the ticket number.
2. Run `status --ticket <ticket> --json`. If the case is not `waiting_for_ai`, report how many rows remain and do not pretend the round is complete.
3. Run `results --ticket <ticket>`. Treat `actual`, right/wrong/not-measurable states, and Side A/B comments as the evidence of record.
4. Explain the significant results, rank the likely fault area, state uncertainty, and distinguish measured facts from inference.
5. If more bench work is needed, create and open one complete follow-up round under the same ticket. If not, give the conclusion and repair/verification next steps directly.

Use `list` when the user asks which cases exist. It returns projects in natural ticket-number order.
