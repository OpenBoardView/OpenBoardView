# Checklist plan schema

Use this JSON format as input to `board_diagnostics.py create` and `add-round`. The CLI adds result fields, stable IDs, timestamps, and the round number.

```json
{
  "title": "Initial checks",
  "summary": "One sentence explaining the purpose of this round.",
  "instructions": [
    "Use the stated power condition for each section.",
    "Adjust the prefilled actual reading when the measured value differs."
  ],
  "sides": [
    {
      "name": "Side A",
      "sections": [
        {
          "title": "With power connected",
          "mode": "voltage",
          "power_state": "connected",
          "safety": "Use a current-limited supply and avoid probe slips.",
          "measurements": [
            {
              "key": "input_present",
              "point": "EVIDENCE_BACKED_TEST_POINT",
              "reference": "GROUND_OR_OTHER_REFERENCE",
              "expected": "EXPECTED_VALUE_OR_RANGE_WITH_UNIT",
              "boardview": "COMPONENT_OR_NET_TO_FIND",
              "requires_pass": [],
              "probe": "Describe probe placement when it is not obvious.",
              "location": "Optional boardview location hint.",
              "why": "Optional diagnostic reason for this check."
            }
          ]
        },
        {
          "title": "Diode mode",
          "mode": "diode",
          "power_state": "disconnected",
          "safety": "Disconnect charger and battery and let rails discharge.",
          "measurements": []
        },
        {
          "title": "Resistance",
          "mode": "resistance",
          "power_state": "disconnected",
          "safety": "Disconnect charger and battery and let rails discharge.",
          "measurements": []
        }
      ]
    },
    {
      "name": "Side B",
      "sections": []
    }
  ]
}
```

## Fields

- `title` is required for every round.
- `summary` is optional but useful for explaining the test strategy.
- `instructions` is an optional array of short strings shown above the tabs.
- `sides` must contain at least `Side A` and `Side B`.
- `sections` may be empty. Supported modes are `voltage`, `diode`, `resistance`, `continuity`, and `other`.
- `power_state` is required. Diode, resistance, and continuity accept only `disconnected`, `unpowered`, or `off`.
- Every measurement requires `point`, `reference`, and `expected`. `probe`, `location`, and `why` are optional.
- `key` is an optional unique stable identifier. Use it on prerequisite and dependent rows.
- `requires_pass` is an optional array of measurement keys. A dependent row waits until every prerequisite passes; a failed, not-measurable, or skipped prerequisite automatically marks it skipped. Do not use it when the downstream result would still add diagnostic value.
- `boardview` optionally names the exact component or net opened by the row's Boardview button. When omitted, the app infers the first component designator in `point`.
- Keep `expected` short enough to edit comfortably because it is also the initial Actual value. Put acceptable ranges and comparison details in `why`.
- Do not put measured results in a plan. The app initializes editable `actual` fields from `expected` and adds `status` and `note` fields. Prefilling does not mark a row as tested.

## Commands

Resolve the script path relative to the skill, then run:

```bash
python3 scripts/board_diagnostics.py validate-plan --plan <plan.json>

python3 scripts/board_diagnostics.py create \
  --ticket <ticket> \
  --board <board-identity> \
  --issue <initial-fault> \
  --workspace <case-folder> \
  --plan <plan.json> \
  --open

python3 scripts/board_diagnostics.py add-round \
  --ticket <ticket> \
  --plan <plan.json> \
  --open

python3 scripts/board_diagnostics.py replace-round \
  --ticket <ticket> \
  --plan <plan.json> \
  --open
```

Quote command arguments safely. Do not replace an existing ticket. Use `replace-round` only for an untouched unfinished active round that the user explicitly rejects; otherwise append a round so measurement history remains intact.
